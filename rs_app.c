#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>

#include "rs_driver_ioctl.h"

#define MAX_DATA_SIZE 256 
#define MAX_ENCODED_SIZE (MAX_DATA_SIZE + 32) 

void print_buffer_hex(const char *label, const char *buf, size_t len) {
    printf("%s (Length: %zu bytes):\n", label, len);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", (unsigned char)buf[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n\n");
}

int main()
{
    int encode_fd, decode_fd;
    char original_data[MAX_DATA_SIZE];
    char encoded_data[MAX_ENCODED_SIZE];
    char decoded_data[MAX_DATA_SIZE];
    ssize_t bytes_written, bytes_read;
    size_t original_len;
    
    struct rs_params params = {
        .symsize = 8,
        .gfpoly = 0x11d, 
        .fcr = 0,
        .prim = 1,
        .nroots = 32, 
    };

    printf("Enter the message to encode (max %d characters):\n", MAX_DATA_SIZE - 1);
    printf("> ");
    
    if (fgets(original_data, sizeof(original_data), stdin) == NULL) {
        fprintf(stderr, "Error reading input.\n");
        return 1;
    }

    original_len = strlen(original_data);
    if (original_len > 0 && original_data[original_len - 1] == '\n') {
        original_data[--original_len] = '\0';
    } else {
        // If input was too long, ensure it's null-terminated
        original_data[MAX_DATA_SIZE - 1] = '\0';
        original_len = strlen(original_data);
    }
    
    size_t write_len = original_len + 1; 
    
    printf("\nOriginal Message: \"%s\"\n", original_data);
    printf("Writing %zu bytes.\n\n", write_len);


    encode_fd = open("/dev/encoded", O_RDWR);
    if (encode_fd < 0) {
        perror("Error opening /dev/encoded");
        return 1;
    }
    
    decode_fd = open("/dev/decoded", O_RDWR);
    if (decode_fd < 0) {
        perror("Error opening /dev/decoded");
        close(encode_fd);
        return 1;
    }
    
    if (ioctl(encode_fd, RS_SET_PARAMS, &params) < 0) {
        perror("ioctl failed to set parameters");
        goto cleanup;
    }
    printf("RS Parameters set successfully (nroots = %d).\n\n", params.nroots);

    printf("--- Encoding ---\n");
    bytes_written = write(encode_fd, original_data, write_len);
    if (bytes_written < 0) {
        perror("Write to /dev/encoded failed");
        goto cleanup;
    }
    printf("Encoded %ld bytes of data written\n", bytes_written);

    bytes_read = read(encode_fd, encoded_data, sizeof(encoded_data));
    if (bytes_read < 0) {
        perror("Read from /dev/encoded failed");
        goto cleanup;
    }
    //printf("Read %ld bytes of full encoded block.\n", bytes_read);
    
    print_buffer_hex("Encoded Block (Data + Parity)", encoded_data, bytes_read);

    printf("--- Dencoding ---\n");

    bytes_written = write(decode_fd, encoded_data, bytes_read);
    if (bytes_written < 0) {
        perror("Write to /dev/decoded failed");
        goto cleanup;
    }
    printf("Decoded %ld bytes of data written\n", bytes_written);
    
    memset(decoded_data, 0, sizeof(decoded_data)); 
    bytes_read = read(decode_fd, decoded_data, sizeof(decoded_data));
    if (bytes_read < 0) {
        perror("Read from /dev/decoded failed");
        goto cleanup;
    }

    //printf("Read %ld bytes of full decoded block.\n", bytes_read);
    printf("Decoded Message: \"%s\"\n\n", decoded_data);

    if (strcmp(original_data, decoded_data) == 0) {
        printf("RESULT: Original data and decoded data match!\n");
    } else {
        printf("RESULT: Original data and decoded data DO NOT match!\n");
        printf("Original: %s\n", original_data);
        printf("Decoded:  %s\n", decoded_data);
    }
    
cleanup:
    close(encode_fd);
    close(decode_fd);

    return 0;
}

