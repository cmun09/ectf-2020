/*
 * eCTF Collegiate 2020 MicroBlaze Example Code
 * Audio Digital Rights Management
 */

#include <stdio.h>
#include "platform.h"
#include "xparameters.h"
#include "xil_exception.h"
#include "xstatus.h"
#include "xaxidma.h"
#include "xil_mem.h"
#include "util.h"
#include "secrets.h"
#include "xintc.h"
#include "constants.h"
#include "sleep.h"
#include "hydrogen.h"


#define OPSLIMIT_MAX 50000
#define MEMLIMIT_MAX 0
#define THREADS_MAX  1

#define OPSLIMIT 10000
#define MEMLIMIT 0
//////////////////////// GLOBALS ////////////////////////


// audio DMA access
static XAxiDma sAxiDma;

// LED colors and controller
u32 *led = (u32*) XPAR_RGB_PWM_0_PWM_AXI_BASEADDR;
const struct color RED =    {0x01ff, 0x0000, 0x0000};
const struct color YELLOW = {0x01ff, 0x01ff, 0x0000};
const struct color GREEN =  {0x0000, 0x01ff, 0x0000};
const struct color BLUE =   {0x0000, 0x0000, 0x01ff};

// change states
#define change_state(state, color) c->drm_state = state; setLED(led, color);
#define set_stopped() change_state(STOPPED, RED)
#define set_working() change_state(WORKING, YELLOW)
#define set_playing() change_state(PLAYING, GREEN)
#define set_paused()  change_state(PAUSED, BLUE)

// shared command channel -- read/write for both PS and PL
volatile cmd_channel *c = (cmd_channel*)SHARED_DDR_BASE;

// internal state store
internal_state s;


//////////////////////// INTERRUPT HANDLING ////////////////////////


// shared variable between main thread and interrupt processing thread
volatile static int InterruptProcessed = FALSE;
static XIntc InterruptController;

void myISR(void) {
    InterruptProcessed = TRUE;
}


//////////////////////// UTILITY FUNCTIONS ////////////////////////


// returns whether an rid has been provisioned
int is_provisioned_rid(char rid) {
    for (int i = 0; i < NUM_PROVISIONED_REGIONS; i++) {
        if (rid == PROVISIONED_RIDS[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

// looks up the region name corresponding to the rid
int rid_to_region_name(char rid, char **region_name, int provisioned_only) {
    for (int i = 0; i < NUM_REGIONS; i++) {
        if (rid == REGION_IDS[i] &&
            (!provisioned_only || is_provisioned_rid(rid))) {
            *region_name = (char *)REGION_NAMES[i];
            return TRUE;
        }
    }

    mb_printf("Could not find region ID '%d'\r\n", rid);
    *region_name = "<unknown region>";
    return FALSE;
}


// looks up the rid corresponding to the region name
int region_name_to_rid(char *region_name, char *rid, int provisioned_only) {
    for (int i = 0; i < NUM_REGIONS; i++) {
        if (!strcmp(region_name, REGION_NAMES[i]) &&
            (!provisioned_only || is_provisioned_rid(REGION_IDS[i]))) {
            *rid = REGION_IDS[i];
            return TRUE;
        }
    }

    mb_printf("Could not find region name '%s'\r\n", region_name);
    *rid = -1;
    return FALSE;
}


// returns whether a uid has been provisioned
int is_provisioned_uid(char uid) {
    for (int i = 0; i < NUM_PROVISIONED_USERS; i++) {
        if (uid == PROVISIONED_UIDS[i]) {
            return TRUE;
        }
    }
    return FALSE;
}


// looks up the username corresponding to the uid
int uid_to_username(char uid, char **username, int provisioned_only) {
    for (int i = 0; i < NUM_USERS; i++) {
        if (uid == USER_IDS[i] &&
            (!provisioned_only || is_provisioned_uid(uid))) {
            *username = (char *)USERNAMES[i];
            return TRUE;
        }
    }

    mb_printf("Could not find uid '%d'\r\n", uid);
    *username = "<unknown user>";
    return FALSE;
}


// looks up the uid corresponding to the username
int username_to_uid(char *username, char *uid, int provisioned_only) {
    for (int i = 0; i < NUM_USERS; i++) {
        if (!strcmp(username, USERNAMES[USER_IDS[i]]) &&
            (!provisioned_only || is_provisioned_uid(USER_IDS[i]))) {
            *uid = USER_IDS[i];
            return TRUE;
        }
    }

    mb_printf("Could not find username '%s'\r\n", username);
    *uid = -1;
    return FALSE;
}


// loads the song metadata in the shared buffer into the local struct // fixed
void load_song_md() {
    s.md.owner_id = c->drm_song.owner_id;
    s.md.region_list = c->drm_song.region_list;
    s.md.preview_length = c->drm_song.preview_length;
    s.md.audio_length = c->drm_song.audio_length;
    s.md.pkt1_owner = c->drm_song.pkt1_owner;
    s.md.owner_key = c->drm_song.owner_key;
    memcpy(s.md_extended.header_signature, c->drm_song.header_signature, hydro_sign_BYTES);
    memcpy(s.md_extended.song_signature, c->drm_song.song_signature, hydro_sign_BYTES);

    // s.song_md.num_regions = MIN(MAX(c->song.md.num_regions, 0), NUM_REGIONS);
    // memcpy(s.song_md.rids, (void *)get_drm_rids(c->song), s.song_md.num_regions);
    // memcpy(s.song_md.uids, (void *)get_drm_uids(c->song), s.song_md.num_users);
}


// checks if the song loaded into the shared buffer is locked for the current user
int is_locked() {
    int locked = TRUE;
    int shared = FALSE;
    // check for authorized user
    if (!s.logged_in) {
        mb_printf("No user logged in");
    } else {

        // check if user is authorized to play song
        if (s.uid == s.md.owner_id) {
            locked = FALSE;
        if(s.md.sharee_uids >> (s.md.uid - 1) & 1){ // shift the bit field and check it is set
            locked = FALSE;
        }   
        if (locked) {
            mb_printf("User '%s' does not have access to this song", s.username);
            return locked;
        }
        mb_printf("User '%s' has access to this song", s.username);
        locked = TRUE; // reset lock for region check


        // Check the region bit field and see if there is at least one matching one
        for (int i = 0; i < sizeof(REGION_IDS); i++){
            if(!(s.md.region_list >> (REGION_IDS[i]) & 1)){
                locked = FALSE;
                shared = TRUE;
                break;
            }
        }

        if (!locked) {
            mb_printf("Region Match. Full Song can be played. Unlocking...");
            if(shared){ // derive symmetryic key with shared public key  

            }
            else{ // use the owner's private key to unencrypt song
                kp = hydro_kx_keypair();
            }
        } else {
            mb_printf("Invalid region");
        }
    }
    return locked;
}


// copy the local song metadata into buf in the correct format
// returns the size of the metadata in buf (including the metadata size field)
// song metadata should be loaded before call

// this is used for sharing, however, this is no longer needed
int gen_song_md(char *buf) {
    buf[0] = ((5 + s.song_md.num_regions + s.song_md.num_users) / 2) * 2; // account for parity
    buf[1] = s.song_md.owner_id;
    buf[2] = s.song_md.num_regions;
    buf[3] = s.song_md.num_users;
    memcpy(buf + 4, s.song_md.rids, s.song_md.num_regions);
    memcpy(buf + 4 + s.song_md.num_regions, s.song_md.uids, s.song_md.num_users);
    return buf[0];
}



//////////////////////// COMMAND FUNCTIONS ////////////////////////

// attempt to log in to the credentials in the shared buffer
void login() {
    if (s.logged_in) {
        mb_printf("Already logged in. Please log out first.\r\n");
    } else {
        memcpy((void*)c->username, s.username, USERNAME_SZ); // make a copy of the username
        memcpy((void*)c->pin, s.pin, MAX_PIN_SZ);   // make a copy of the pin
        for (int i = 0; i < NUM_PROVISIONED_USERS; i++) {
            // search for matching username
            if (!strcmp(s.username, USERNAMES[PROVISIONED_UIDS[i]])) {
                uint8_t tohash[64 + sizeof(PROVISIONED_USER_SALT1S[i])]; // allocate memory for salted password
                memcpy(tohash, s.pin, MAX_PIN_SZ);  // copy stored pin
                memcpy(tohash + 64, PROVISIONED_USER_SALT1S[i], sizeof(PROVISIONED_USER_SALT1S[i]))); // append the salt
                

                // verify the hash with the padding + salt
                if(hydro_pwhash_verify((void *)PROVISIONED_PIN_HASHES[i], tohash, // line1 start
                  sizeof(tohash), (uint8_t *)MASTER_KEY,  // line 2
                  OPSLIMIT_MAX, MEMLIMIT_MAX, THREADS_MAX) != 0){ // line 3 end
                    // reject login attempt
                    mb_printf("Incorrect pin for user '%s'\r\n", s.username);
                    memset(s.username, 0, USERNAME_SZ);
                    memset(s.pin, 0, MAX_PIN_SZ);
                    return;
                } 
                else {
                    // determine the hash to derive private key of user
                    uint8_t derived_hash1[32];
                    hydro_pwhash_deterministic(derived_hash1, 
                    sizeof derived_hash1, 
                    tohash, sizeof(tohash), CONTEXT, 
                    (uint8_t *)MASTER_KEY, OPSLIMIT, MEMLIMIT, THREADS_MAX); 


                    uint8_t tohash2[64 + sizeof(PROVISIONED_USER_SALT2S[i])]; // allocate memory for salted password
                    memcpy(tohash2, s.pin, MAX_PIN_SZ);  // copy stored pin
                    memcpy(tohash2 + 64, PROVISIONED_USER_SALT2S[i], sizeof(PROVISIONED_USER_SALT2S[i]))); // append the salt
                
                    uint8_t derived_hash2[hydro_hash_BYTES];
                    hydro_hash_hash(derived_hash2, sizeof(derived_hash2), tohash2, sizeof(tohash2), CONTEXT, NULL);

                    s.private_key = derived_hash1 ^ derived_hash2;
                    //update states
                    s.logged_in = 1;
                    c->login_status = 1; // probably not needed?
                    s.uid = PROVISIONED_UIDS[i];
                    mb_printf("Logged in for user '%s'\r\n", s.username);

                    // determine the private key of the user

                    return;
                }
            }
        }

        // reject login attempt
        mb_printf("User not found\r\n");
        memset((void*)s.username, 0, USERNAME_SZ);
        memset((void*)s.pin, 0, MAX_PIN_SZ);
    }
}


// attempt to log out
void logout() {
    if (s.logged_in) {
        mb_printf("Logging out...\r\n");
        s.logged_in = 0;
        c->login_status = 0;
        memset(s.username, 0, USERNAME_SZ);
        memset(s.pin, 0, MAX_PIN_SZ);
        s.uid = 0;
    } else {
        mb_printf("Not logged in\r\n");
    }
}

// handles a request to query the player's metadata ( DONT NOT CHANGE)
void query_player() {
    c->query.num_regions = NUM_PROVISIONED_REGIONS;
    c->query.num_users = NUM_PROVISIONED_USERS;

    for (int i = 0; i < NUM_PROVISIONED_REGIONS; i++) {
        strcpy((char *)q_region_lookup(c->query, i), REGION_NAMES[PROVISIONED_RIDS[i]]);
    }

    for (int i = 0; i < NUM_PROVISIONED_USERS; i++) {
        strcpy((char *)q_user_lookup(c->query, i), USERNAMES[i]);
    }

    mb_printf("Queried player (%d regions, %d users)\r\n", c->query.num_regions, c->query.num_users);
}


// handles a request to query song metadata
void query_song() {
    char *name;

    // load song
    load_song_md();
    memset((void *)&c->query, 0, sizeof(query));

    c->query.num_regions = s.song_md.num_regions;
    c->query.num_users = s.song_md.num_users;

    // copy owner name
    uid_to_username(s.song_md.owner_id, &name, FALSE);
    strcpy((char *)c->query.owner, name);

    // copy region names
    for (int i = 0; i < s.song_md.num_regions; i++) {
        rid_to_region_name(s.song_md.rids[i], &name, FALSE);
        strcpy((char *)q_region_lookup(c->query, i), name);
    }

    // copy authorized uid names
    for (int i = 0; i < s.song_md.num_users; i++) {
        uid_to_username(s.song_md.uids[i], &name, FALSE);
        strcpy((char *)q_user_lookup(c->query, i), name);
    }

    mb_printf("Queried song (%d regions, %d users)\r\n", c->query.num_regions, c->query.num_users);
}

// verifies that the drm_md_header has not been tampered
int verify_drm_md_header(){
    if (hydro_sign_verify(s.md_extended.header_signature, 
        s.md, sizeof(s.md), CONTEXT, DEVICE_PK) != 0) {
    /* forged */
        return false;
    }
    return true;
}

// verifies that the song has not been tampered
int verify_song(){
    if (hydro_sign_verify(s.md_extended.song_signature, 
        c->drm_song.song_raw, sizeof(s.md), CONTEXT, DEVICE_PK) != 0) {
    /* forged */
        return false;
    }
    return true;
}


// add a user to the song's list of users
void share_song() {
    int new_md_len, shift;
    char new_md[256], uid;

    // reject non-owner attempts to share
    load_song_md();
    if (!s.logged_in) {
        mb_printf("No user is logged in. Cannot share song\r\n");
        c->song.wav_size = 0;
        return;
    } else if (s.uid != s.song_md.owner_id) {
        mb_printf("User '%s' is not song's owner. Cannot share song\r\n", s.username);
        c->song.wav_size = 0;
        return;
    } else if (!username_to_uid((char *)c->username, &uid, TRUE)) {
        mb_printf("Username not found\r\n");
        c->song.wav_size = 0;
        return;
    }
    // basic song sharing check finished
    hydro_kx_session_keypair session_kp;

    hydro_kx_keypair kp = {PROVISIONED_USER_PKS[s.uid], s.private_key};
    hydro_kx_n_2(*session_kp, PROVISIONED_USER_PKS[s.uid], NULL,);



    // generate new song metadata
    s.song_md.uids[s.song_md.num_users++] = uid;
    new_md_len = gen_song_md(new_md);
    shift = new_md_len - s.song_md.md_size;

    // shift over song and add new metadata
    if (shift) {
        memmove((void *)get_drm_song(c->song) + shift, (void *)get_drm_song(c->song), c->song.wav_size);
    }
    memcpy((void *)&c->song.md, new_md, new_md_len);

    // update file size
    c->song.file_size += shift;
    c->song.wav_size  += shift;

    mb_printf("Shared song with '%s'\r\n", c->username);
}


// plays a song and looks for play-time commands
void play_song() {
    u32 counter = 0, rem, cp_num, cp_xfil_cnt, offset, dma_cnt, length, *fifo_fill;

    mb_printf("Reading Audio File...");
    load_song_md();
    if(verify_song() && verify_drm_md_header()){
        mb_printf("Song verification success.");
    }{
        // return if the song verification failes using the public key
        mb_printf("Song with invalid signature. Has the song been tampered?");
        return;
    }
    // get WAV length
    length = s.md.preview_length + s.md.audio_length;
    mb_printf("Song length = %dB", length);

    // truncate song if locked
    if (length > PREVIEW_SZ && is_locked()) {
        length = PREVIEW_SZ;
        mb_printf("Song is locked.  Playing only %ds = %dB\r\n",
                   PREVIEW_TIME_SEC, PREVIEW_SZ);
    } else {
        mb_printf("Song is unlocked. Playing full song\r\n");
    }

    rem = length;
    fifo_fill = (u32 *)XPAR_FIFO_COUNT_AXI_GPIO_0_BASEADDR;

    // write entire file to two-block codec fifo
    // writes to one block while the other is being played
    set_playing();
    while(rem > 0) {
        // check for interrupt to stop playback
        while (InterruptProcessed) {
            InterruptProcessed = FALSE;

            switch (c->cmd) {
            case PAUSE:
                mb_printf("Pausing... \r\n");
                set_paused();
                while (!InterruptProcessed) continue; // wait for interrupt
                break;
            case PLAY:
                mb_printf("Resuming... \r\n");
                set_playing();
                break;
            case STOP:
                mb_printf("Stopping playback...");
                return;
            case RESTART:
                mb_printf("Restarting song... \r\n");
                rem = length; // reset song counter
                set_playing();
            default:
                break;
            }
        }

        // calculate write size and offset
        cp_num = (rem > CHUNK_SZ) ? CHUNK_SZ : rem;
        offset = (counter++ % 2 == 0) ? 0 : CHUNK_SZ;

        // do first mem cpy here into DMA BRAM
        Xil_MemCpy((void *)(XPAR_MB_DMA_AXI_BRAM_CTRL_0_S_AXI_BASEADDR + offset),
                   (void *)(get_drm_song(c->song) + length - rem),
                   (u32)(cp_num));

        cp_xfil_cnt = cp_num;

        while (cp_xfil_cnt > 0) {

            // polling while loop to wait for DMA to be ready
            // DMA must run first for this to yield the proper state
            // rem != length checks for first run
            while (XAxiDma_Busy(&sAxiDma, XAXIDMA_DMA_TO_DEVICE)
                   && rem != length && *fifo_fill < (FIFO_CAP - 32));

            // do DMA
            dma_cnt = (FIFO_CAP - *fifo_fill > cp_xfil_cnt)
                      ? FIFO_CAP - *fifo_fill
                      : cp_xfil_cnt;
            fnAudioPlay(sAxiDma, offset, dma_cnt);
            cp_xfil_cnt -= dma_cnt;
        }

        rem -= cp_num;
    }
}


// removes DRM data from song for digital out
// write the sound to a file (sound file)
void digital_out() {
    // remove metadata size from file and chunk sizes
    c->song.file_size -= c->song.md.md_size;
    c->song.wav_size -= c->song.md.md_size;

    if (is_locked() && PREVIEW_SZ < c->song.wav_size) {
        mb_printf("Only playing 30 seconds");
        c->song.file_size -= c->song.wav_size - PREVIEW_SZ;
        c->song.wav_size = PREVIEW_SZ;
    }

    // move WAV file up in buffer, skipping metadata
    mb_printf(MB_PROMPT "Dumping song (%dB)...", c->song.wav_size);
    memmove((void *)&c->song.md, (void *)get_drm_song(c->song), c->song.wav_size);

    mb_printf("Song dump finished\r\n");
}


//////////////////////// MAIN ////////////////////////


int main() {
    u32 status;

    init_platform();
    microblaze_register_handler((XInterruptHandler)myISR, (void *)0);
    microblaze_enable_interrupts();

    // Initialize the interrupt controller driver so that it is ready to use.
    status = XIntc_Initialize(&InterruptController, XPAR_INTC_0_DEVICE_ID);
    if (status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    // Set up the Interrupt System.
    status = SetUpInterruptSystem(&InterruptController, (XInterruptHandler)myISR);
    if (status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    // Congigure the DMA
    status = fnConfigDma(&sAxiDma);
    if(status != XST_SUCCESS) {
        mb_printf("DMA configuration ERROR\r\n");
        return XST_FAILURE;
    }

    // Start the LED
    enableLED(led);
    set_stopped();

    // clear command channel
    memset((void*)c, 0, sizeof(cmd_channel));

    mb_printf("Audio DRM Module has Booted\n\r");

    // Handle commands forever
    while(1) {
        // wait for interrupt to start
        if (InterruptProcessed) {
            InterruptProcessed = FALSE;
            set_working();

            // c->cmd is set by the miPod player
            switch (c->cmd) {
            case LOGIN:
                login();
                break;
            case LOGOUT:
                logout();
                break;
            case QUERY_PLAYER:
                query_player();
                break;
            case QUERY_SONG:
                query_song();
                break;
            case SHARE:
                share_song();
                break;
            case PLAY:
                play_song();
                mb_printf("Done Playing Song\r\n");
                break;
            case DIGITAL_OUT:
                digital_out();
                break;
            default:
                break;
            }

            // reset statuses and sleep to allowe player to recognize WORKING state
            strcpy((char *)c->username, s.username);
            c->login_status = s.logged_in;
            usleep(500);
            set_stopped();
        }
    }

    cleanup_platform();
    return 0;
}