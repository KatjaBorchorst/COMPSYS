#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#ifdef __APPLE__
#include "./endian.h"
#else
#include <endian.h>
#endif

#include "./networking.h"
#include "./sha256.h"

char server_ip[IP_LEN];
char server_port[PORT_LEN];
char my_ip[IP_LEN];
char my_port[PORT_LEN];

int c;

rio_t rio;
int clientfd;
int registered = 0; //1 if client is registered with server, 0 if not.

unsigned char reqHeader[REQUEST_HEADER_LEN];

// Gets a sha256 hash of specified data, sourcedata. The hash itself is
// placed into the given variable 'hash'. Any size can be created, but a
// a normal size for the hash would be given by the global variable
// 'SHA256_HASH_SIZE', that has been defined in sha256.h
void get_data_sha(const char* sourcedata, hashdata_t hash, uint32_t data_size, 
    int hash_size){
  SHA256_CTX shactx;
  unsigned char shabuffer[hash_size];
  sha256_init(&shactx);
  sha256_update(&shactx, sourcedata, data_size);
  sha256_final(&shactx, shabuffer);

  for (int i=0; i<hash_size; i++){
    hash[i] = shabuffer[i];
  }
}

// Gets a sha256 hash of specified data file, sourcefile. The hash itself is
//placed into the given variable 'hash'. Any size can be created, but a
//a normal size for the hash would be given by the global variable
//'SHA256_HASH_SIZE', that has been defined in sha256.h
void get_file_sha(const char* sourcefile, hashdata_t hash, int size){
    int casc_file_size;

    FILE* fp = Fopen(sourcefile, "rb");
    if (fp == 0)
    {
        printf("Failed to open source: %s\n", sourcefile);
        return;
    }

    fseek(fp, 0L, SEEK_END);
    casc_file_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    char buffer[casc_file_size];
    Fread(buffer, casc_file_size, 1, fp);
    Fclose(fp);

    get_data_sha(buffer, hash, casc_file_size, size);
}


//  Combine a password and salt together and hash the result to form the 
//  'signature'. The result should be written to the 'hash' variable. Note that 
//  as handed out, this function is never called. You will need to decide where 
//  it is sensible to do so.
void get_signature(char* password, char* salt, hashdata_t* hash){
    char to_hash[strlen(password) + strlen(salt)];
    memcpy(to_hash, password, PASSWORD_LEN);
    memcpy(&to_hash[PASSWORD_LEN], salt, SALT_LEN);
    get_data_sha(to_hash, *hash, strlen(to_hash), SHA256_HASH_SIZE);
}

// Salts signature and places the username and signature in usigned char array with padding.
void build_header(char* username, char* password, char* salt, unsigned char *header) {
    hashdata_t signature;
    get_signature(password, salt, &signature);
    memcpy(header, username, USERNAME_LEN);
    memcpy(&header[USERNAME_LEN], &signature, SHA256_HASH_SIZE);
}

// A function for comparing two hashes byte by byte.
int compare_hashes (hashdata_t hash1, hashdata_t hash2) {
    int same = 1;
    for (size_t i = 0; i < SHA256_HASH_SIZE; i++) {
        if (hash1[i] != hash2[i]) {
            same = 0;
            return same;
        }
    }
    return same;
}

// Processes the response received from the server. Returns 1 if status is not 1 or if blockhash 
//doesn't match, 0 otherwise.
int process_server_response(struct ServerResponse* serverResponse) {
    // Extract header.
    unsigned char responseHeader[RESPONSE_HEADER_LEN];
    assert(Rio_readnb(&rio, responseHeader, RESPONSE_HEADER_LEN) != -1);

    // Parse and put elements into their containers.
    serverResponse->header.responseLen = ntohl(*(uint32_t*)&responseHeader[0]); //Convert to host-byte-order.
    serverResponse->header.status = ntohl(*(uint32_t*)&responseHeader[4]); 
    serverResponse->header.blockNum = ntohl(*(uint32_t*)&responseHeader[8]); 
    serverResponse->header.blockCount = ntohl(*(uint32_t*)&responseHeader[12]);
    memcpy(&serverResponse->header.blockHash, (hashdata_t*)&responseHeader[16], SHA256_HASH_SIZE);
    memcpy(&serverResponse->header.totalHash, (hashdata_t*)&responseHeader[16+SHA256_HASH_SIZE], 
                                                                                SHA256_HASH_SIZE);
    
    // Extract payload.
    char msg[serverResponse->header.responseLen];
    assert(Rio_readnb(&rio, &msg, serverResponse->header.responseLen) != -1);
    memcpy(&serverResponse->payload, msg, serverResponse->header.responseLen);

    // Check status.
    if (serverResponse->header.status != 1) {
        printf ("Got unexpected status code: %d - ", serverResponse->header.status);
        printf("%s\n", serverResponse->payload);
        return 1;
    }

    // Check blockhash.
    hashdata_t blockHash;
    get_data_sha(serverResponse->payload, blockHash, serverResponse->header.responseLen,
        SHA256_HASH_SIZE);
    if (compare_hashes(serverResponse->header.blockHash, blockHash) == 0) {
        printf("For block %d {%d/%d} checksum did not match.\n",
                serverResponse->header.blockNum,serverResponse->header.blockNum+1, serverResponse->header.blockCount);
        return 1;
    }
    printf("block: %d (%d/%d)\n", serverResponse->header.blockNum, serverResponse->header.blockNum+1,  
        serverResponse->header.blockCount);
    return 0;
}

// Register a new user with a server by sending the username and signature to 
// the server.
void register_user(char* username, char* password, char* salt){
    unsigned char header[REQUEST_HEADER_LEN-4]; 
    build_header(username, password, salt, header);

    //concat
    memcpy(&reqHeader, &header, REQUEST_HEADER_LEN-4);

    //insert 0's in last 4 bytes
    uint32_t empty;
    memcpy(&reqHeader[REQUEST_HEADER_LEN-4], &empty, 4);
   
    // Initialise connection and rio.
    clientfd = Open_clientfd(server_ip, server_port); 
    Rio_readinitb(&rio, clientfd);

    Rio_writen(clientfd, reqHeader, REQUEST_HEADER_LEN); // Send reqHeader.
    
    // Process response from server.
    struct ServerResponse *ServerResponse = malloc(sizeof(struct ServerResponse)); 
    if (process_server_response(ServerResponse) == 1) {
        exit(EXIT_FAILURE); // Stop if status or hash checks fail.
    }    
    
    registered = 1;
    printf("Got response: %s\n", ServerResponse->payload);
    free(ServerResponse);
}

// Get a file from the server by sending the username and signature, along with
// file path. Note that this function should be able to deal with both small 
// and large files. 
void get_file(char* username, char* password, char* salt, char* to_get) {
    // Build header from scratch only if it hasn't been done before.
    if (registered == 0) {
        unsigned char header[REQUEST_HEADER_LEN-4]; 
        build_header(username, password, salt, header);
        memcpy(&reqHeader, &header, REQUEST_HEADER_LEN-4);
    }
    
    // Initialize connection and rio.
    clientfd = Open_clientfd(server_ip, server_port);
    Rio_readinitb(&rio, clientfd);

    // Assemble request.
    uint32_t REQ_LENGTH = strlen(to_get); 
    uint32_t N_ORDER_LENGTH = htonl(REQ_LENGTH); // Convert to network-byte-order.
    char reqBody[REQUEST_HEADER_LEN+strlen(to_get)];
    memcpy(reqBody, reqHeader, REQUEST_HEADER_LEN-4);
    memcpy(&reqBody[REQUEST_HEADER_LEN-4], &N_ORDER_LENGTH, 4);  
    memcpy(&reqBody[REQUEST_HEADER_LEN], to_get, strlen(to_get));
    
    // Send request.
    Rio_writen(clientfd, reqBody, REQUEST_HEADER_LEN+strlen(to_get)); 

    // Process response from server.
    struct ServerResponse *ServerResponse = malloc(sizeof(struct ServerResponse)); 
    if (process_server_response(ServerResponse) == 1) {
        exit(EXIT_FAILURE); // Stop if status or hash checks fail.
    }   
    size_t datasize = (MAX_PAYLOAD)*(ServerResponse->header.blockCount);
    char allData [datasize];

    // Loop if the block extracted is not the last/only one.
    while (ServerResponse->header.blockNum+1 != ServerResponse->header.blockCount) {
        memcpy(&allData[(MAX_PAYLOAD)*(ServerResponse->header.blockNum)],
                ServerResponse->payload, ServerResponse->header.responseLen);
        if (process_server_response(ServerResponse) == 1) {
            exit(EXIT_FAILURE); // Stop if status or hash checks fail.
        } 
    }
    memcpy(&allData[(MAX_PAYLOAD)*(ServerResponse->header.blockNum)],
                ServerResponse->payload, ServerResponse->header.responseLen);
    // Update now that we know the actual size of the last block's payload.
    datasize = datasize - ((MAX_PAYLOAD)-(ServerResponse->header.responseLen));
    free(ServerResponse);

    // Create file and write data into file.
    FILE *fp;
    fp = fopen(to_get, "w");
    for (size_t i = 0; i < datasize; i++){
        fputc(allData[i], fp);
    }
    printf("Retrieved data written to %s\n", to_get);
    fclose(fp);

    hashdata_t fileHash;
    get_file_sha(to_get, fileHash, SHA256_HASH_SIZE);
    if (compare_hashes (fileHash, ServerResponse->header.totalHash) == 0) {
        printf ("Hashes to not match. ");
    }
}

int main(int argc, char **argv){
    // Users should call this script with a single argument describing what 
    // config to use.
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <config file>\n", argv[0]);
        exit(EXIT_FAILURE);
    } 

    // Read in configuration options. Should include a client_directory, 
    // client_ip, client_port, server_ip, and server_port.
    char buffer[128];
    fprintf(stderr, "Got config path at: %s\n", argv[1]);
    FILE* fp = Fopen(argv[1], "r");
    while (fgets(buffer, 128, fp)) {
        if (starts_with(buffer, CLIENT_IP)) {
            memcpy(my_ip, &buffer[strlen(CLIENT_IP)], 
                strcspn(buffer, "\r\n")-strlen(CLIENT_IP));
            if (!is_valid_ip(my_ip)) {
                fprintf(stderr, ">> Invalid client IP: %s\n", my_ip);
                exit(EXIT_FAILURE);
            }
        }else if (starts_with(buffer, CLIENT_PORT)) {
            memcpy(my_port, &buffer[strlen(CLIENT_PORT)], 
                strcspn(buffer, "\r\n")-strlen(CLIENT_PORT));
            if (!is_valid_port(my_port)) {
                fprintf(stderr, ">> Invalid client port: %s\n", my_port);
                exit(EXIT_FAILURE);
            }
        }else if (starts_with(buffer, SERVER_IP)) {
            memcpy(server_ip, &buffer[strlen(SERVER_IP)], 
                strcspn(buffer, "\r\n")-strlen(SERVER_IP));
            if (!is_valid_ip(server_ip)) {
                fprintf(stderr, ">> Invalid server IP: %s\n", server_ip);
                exit(EXIT_FAILURE);
            }
        }else if (starts_with(buffer, SERVER_PORT)) {
            memcpy(server_port, &buffer[strlen(SERVER_PORT)], 
                strcspn(buffer, "\r\n")-strlen(SERVER_PORT));
            if (!is_valid_port(server_port)) {
                fprintf(stderr, ">> Invalid server port: %s\n", server_port);
                exit(EXIT_FAILURE);
            }
        }        
    }
    fclose(fp);

    fprintf(stdout, "Client at: %s:%s\n", my_ip, my_port);
    fprintf(stdout, "Server at: %s:%s\n", server_ip, server_port);

    char username[USERNAME_LEN];
    char password[PASSWORD_LEN];
    char user_salt[SALT_LEN+1];
    
    fprintf(stdout, "Enter a username to proceed: ");
    scanf("%16s", username);
    while ((c = getchar()) != '\n' && c != EOF);
    // Clean up username string as otherwise some extra chars can sneak in.
    for (int i=strlen(username); i<USERNAME_LEN; i++)
    {
        username[i] = '\0';
    }
 
    fprintf(stdout, "Enter your password to proceed: ");
    scanf("%16s", password);
    while ((c = getchar()) != '\n' && c != EOF);
    // Clean up password string as otherwise some extra chars can sneak in.
    for (int i=strlen(password); i<PASSWORD_LEN; i++)
    {
        password[i] = '\0';
    }

    // Note that a random salt should be used, but you may find it easier to
    // repeatedly test the same user credentials by using the hard coded value
    // below instead, and commenting out this randomly generating section.
    for (int i=0; i<SALT_LEN; i++){
        user_salt[i] = 'a' + (random() % 26);
    }
    user_salt[SALT_LEN] = '\0';
    // strncpy(user_salt, 
    //    "0123456789012345678901234567890123456789012345678901234567890123\0", 
    //    SALT_LEN+1);

    fprintf(stdout, "Using salt: %s\n", user_salt);

    // The following function calls have been added as a structure to a 
    // potential solution demonstrating the core functionality. Feel free to 
    // add, remove or otherwise edit. 

    // get_file(username, password, user_salt, "tiny.txt"); //TEST #1

    // Register the given user
    register_user(username, password, user_salt);
    
    // Retrieve the smaller file, that doesn't not require support for blocks
    get_file(username, password, user_salt, "tiny.txt"); //TEST #2

    // Retrieve the larger file, that requires support for blocked messages
    get_file(username, password, user_salt, "hamlet.txt"); //TEST #3

    // get_file(username, password, user_salt, "many_hamlets.txt"); //TEST #4
    // get_file(username, password, user_salt, "server.py"); //TEST #5


    // get_file(username, password, user_salt, "does_not_exist.txt"); //TEST #6
    // get_file(username, password, user_salt, "does_not_exist"); //TEST #7

    exit(EXIT_SUCCESS);
}
