#ifndef SRC_CONSTANTS_H_
#define SRC_CONSTANTS_H_

#include "xil_printf.h"

// shared DDR address
#define SHARED_DDR_BASE (0x20000000 + 0x1CC00000)

// memory constants
#define CHUNK_SZ 16000
#define FIFO_CAP 4096*4

// number of seconds to record/playback
#define PREVIEW_TIME_SEC 30

// ADC/DAC sampling rate in Hz
#define AUDIO_SAMPLING_RATE 48000
#define BYTES_PER_SAMP 2
#define PREVIEW_SZ (PREVIEW_TIME_SEC * AUDIO_SAMPLING_RATE * BYTES_PER_SAMP)

// printing utility
#define MB_PROMPT "\r\nMB> "
#define mb_printf(...) xil_printf(MB_PROMPT __VA_ARGS__)

// protocol constants
#define MAX_REGIONS 64
#define REGION_NAME_SZ 64
#define MAX_USERS 64
#define USERNAME_SZ 64
#define MAX_PIN_SZ 64
#define MAX_SONG_SZ (1<<25)


// LED colors and controller
struct color {
    u32 r;
    u32 g;
    u32 b;
};


// struct to interpret shared buffer as a query
typedef struct {
    int num_regions;
    int num_users;
    char owner[USERNAME_SZ];
    char regions[MAX_REGIONS * REGION_NAME_SZ];
    char users[MAX_USERS * USERNAME_SZ];
} query;

// simulate array of 64B names without pointer indirection
#define q_region_lookup(q, i) (q.regions + (i * REGION_NAME_SZ))
#define q_user_lookup(q, i) (q.users + (i * USERNAME_SZ))

// accessors for variable-length metadata fields
#define get_drm_rids(d) (d.md.buf)
#define get_drm_uids(d) (d.md.buf + d.md.num_regions)
#define get_drm_song(d) ((char *)(&d.md) + d.md.md_size)

// shared buffer values
enum commands { QUERY_PLAYER, QUERY_SONG, LOGIN, LOGOUT, SHARE, PLAY, STOP, DIGITAL_OUT, PAUSE, RESTART, FF, RW };
enum states   { STOPPED, WORKING, PLAYING, PAUSED };


typedef struct __attribute__((__packed__)){
    uint8_t pkt[48];
} pkt;

typedef struct __attribute__((__packed__)){
    uint8_t sym_key[68];
} sym_key;

// struct to interpret drm metadata
typedef struct __attribute__((__packed__)) {
    uint8_t owner_id;
    uint32_t region_list;
    uint32_t preview_length;
    uint32_t audio_length;
    pkt pkt1_owner;
    sym_key owner_key;
    uint8_t header_signature[hydro_sign_BYTES];
    uint64_t sharee_uids;
    pkt sharees[64];
    sym_key sharee_keys[64];
    uint8_t song_signature[hydro_sign_BYTES]
    uint8_t song_raw[]; 
} drm_file;

typedef struct __attribute__((__packed__)){
    uint8_t owner_id;
    uint32_t region_list;
    uint32_t preview_length;
    uint32_t audio_length;
    pkt pkt1_owner;
    sym_key owner_key;
}drm_md_header;

typedef struct __attribute__(){
    uint8_t header_signature[hydro_sign_BYTES];
    uint64_t sharee_uids;
    pkt sharees[64];
    sym_key sharee_keys[64];
    uint8_t song_signature[hydro_sign_BYTES]
}drm_md_header_extended;

// struct to interpret shared command channel
typedef volatile struct __attribute__((__packed__)) {
    char cmd;                   // from commands enum
    char drm_state;             // from states enum
    char login_status;          // 0 = logged off, 1 = logged on
    char padding;               // not used
    char username[USERNAME_SZ]; // stores logged in or attempted username
    char pin[MAX_PIN_SZ];       // stores logged in or attempted pin

    // shared buffer is either a drm song or a query
    union {
        drm_file drm_song;
        query  query;
    };
} cmd_channel;


// store of internal state
typedef struct {
    uint8_t logged_in = 0;             // whether or not a user is logged on
    uint8_t uid = 0;                   // logged on user id
    char username[USERNAME_SZ]; // logged on username
    char pin[MAX_PIN_SZ];       // logged on pin
    uint8_t private_key[hydro_hash_BYTES]; // double check on this
    drm_md_header md;           // current song header data
    drm_md_header_extended md_extended;
} internal_state;


#endif /* SRC_CONSTANTS_H_ */
