
#pragma once
#include "common.h"
#include "sha256.h"

// container for hasher
typedef uint8_t hashdata_t[SHA256_HASH_SIZE];

// container for assembling password with salt
typedef struct PasswordAndSalt {
    char password[PASSWORD_LEN];
    char salt[SALT_LEN];
} PasswordAndSalt_t;

// container for assembling request message headers
typedef struct RequestHeader {
    char username[USERNAME_LEN];
    hashdata_t salted_and_hashed;
    uint32_t length;
} RequestHeader_t;

// container for assembling request messages
typedef struct Request {
    RequestHeader_t header;
    char payload[PATH_LEN];
} Request_t;

// Container for parts of the servers response headers.
typedef struct ServerHeader {
    uint32_t responseLen;
    uint32_t status;
    uint32_t blockNum;
    uint32_t blockCount;
    hashdata_t blockHash;
    hashdata_t totalHash;
} ServerHeader_t;

// Container for parts of the servers response.
typedef struct ServerResponse {
    struct ServerHeader header;
    char payload[MAX_PAYLOAD];
} ServerResponse_t;

