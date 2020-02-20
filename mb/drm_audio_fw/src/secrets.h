
#ifndef SECRETS_H
#define SECRETS_H

#define NUM_REGIONS 2
const char *REGION_NAMES[] = { "china", "america" };
const u8 REGION_IDS[] = { 0, 1 };

#define NUM_PROVISIONED_REGIONS 2
const u8 PROVISIONED_RIDS[] = { 0, 1 };

const char *MASTER_KEY = "\x7d\x9c\x94\x85\x77\x72\x2b\x98\x7d\x12\x14\xee\x82\xab\x6e\x74\x24\x4c\x51\x32\xde\xb8\x97\x38\x13\x0a\xe3\xdc\xd0\x2c\x66\xcc";

#define NUM_USERS 2
const char *USERNAMES[] = { "bill", "john" };
const u8 USER_IDS[] = { 0, 1 };

#define NUM_PROVISIONED_USERS 2
const u8 PROVISIONED_UIDS[] = { 0, 1 };
const char *PROVISIONED_PIN_HASHES[] = { "\x98\xf1\x31\x1c\xd8\x67\x84\x08\x16\xdd\xc8\x72\x97\xe5\x18\x89\x7c\x33\x8b\x22\x83\xe4\x51\x2d\x2e\x9e\x95\x6e\x71\x70\xf9\x72", "\xf1\xa2\xe7\x09\x0d\x68\x07\xc8\x36\xac\xf0\x33\x4d\xf9\xdb\x25\x4e\xb3\xc3\xb6\x6d\xaa\xb9\x05\x7f\x21\xe0\x95\x53\x46\x11\xca" };
const char *PROVISIONED_USER_PKS[] = { "\x97\xb0\x5f\xd2\x82\x28\x93\xe1\x33\x27\xd4\xbb\x3f\xfb\x33\x63\x22\xbc\x6a\x7b\x61\x67\xf8\xdd\xf2\x2e\x7d\xd9\x98\xc0\xa2\x07", "\x75\x27\xc0\x1d\x0e\xbf\x2a\x19\xd1\xb1\x58\x4b\x42\x26\x9c\x62\xa4\x40\x96\x0b\x91\x49\x0d\xd7\x9d\x80\x41\x90\x49\xf5\x8b\x56" };
const char *PROVISIONED_USER_SKS[] = { "\x1b\x62\xb7\x16\xe0\xd6\x50\x67\xd7\x24\xcf\xe8\x31\xcb\x19\xf8\x57\x9f\x77\x66\x69\x5d\x32\x81\xc1\x9a\xd1\x70\xd7\x22\x02\x9c", "\x88\x44\x33\xdb\xfd\x15\xec\x2d\x01\x08\x23\x43\x36\xe0\xca\x14\x44\x17\xc2\x38\x89\xa9\x26\x22\x54\x83\xec\xd7\x9c\xa8\x6c\x7c" };

#endif // SECRETS_H