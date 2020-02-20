
#include "../lib/libhydrogen/hydrogen.h"

#define CONTEXT "Examples"
#define MESSAGE "test"
#define MESSAGE_LEN 4
#define CIPHERTEXT_LEN (hydro_secretbox_HEADERBYTES + MESSAGE_LEN)

using namespace std;


uint8_t key[hydro_secretbox_KEYBYTES];
uint8_t ciphertext[CIPHERTEXT_LEN];

char decrypted[MESSAGE_LEN];

int main(){
     
    hydro_secretbox_keygen(key);
    hydro_secretbox_encrypt(ciphertext, MESSAGE, MESSAGE_LEN, 0, CONTEXT, key);

    if (hydro_secretbox_decrypt(decrypted, ciphertext, CIPHERTEXT_LEN, 0, CONTEXT, key) != 0) {
        /* message forged! */
    }
    
    return 1;
}
