#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>

#include "rs_driver_ioctl.h"

#define MAX_BUFFER_SIZE 256

void cleanup_func(){
    
}

int main()
{
    int encode_fd, decode_fd;
    char original_data[MAX_BUFFER_SIZE] = "TEsting data for rs enodding";
    char encoded_data[MAX_BUFFER_SIZE + 33];
    char decoded_data[MAX_BUFFER_SIZE];
    ssize_t bytes_written, bytes_read;
    
    struct rs_params params = {
        .symsize = 8,
        .gfpoly = 0x11d, 
        .fcr = 0,
        .prim = 1,
        .nroots = 32,
    };

    encode_fd = open("/dev/encoded", O_RDWR);
    if (encode_fd < 0) {
        perror("error opening /dev/encoded");
        return 1;
    }
    
    decode_fd = open("/dev/decoded", O_RDWR);
    if (decode_fd < 0) {
        perror("error opening /dev/decoded");
        close(encode_fd);
        return 1;
    }
    
    printf("Setting RS parameters via ioctl\n");
    if (ioctl(encode_fd, RS_SET_PARAMS, &params) < 0) {
        perror("ioctl failed on /dev/encoded");
        goto cleanup;
    }

    printf("Writing original data to /dev/encoded...\n");
    bytes_written = write(encode_fd, original_data, strlen(original_data) + 1);
    if (bytes_written < 0) {
        perror("Write to /dev/encoded failed");
        goto cleanup;
    }
    
    printf("Successfully wrote %ld bytes.\n", bytes_written);

    printf("Reading encoded data from /dev/encoded...\n");
    bytes_read = read(encode_fd, encoded_data, sizeof(encoded_data));
    if (bytes_read < 0) {
        perror("Read from /dev/encoded failed");
        goto cleanup;
    }
    
    printf("Successfully read %ld bytes of encoded data.\n", bytes_read);

    printf("Writing encoded data to /dev/decoded...\n");
    bytes_written = write(decode_fd, encoded_data, bytes_read);
    if (bytes_written < 0) {
        perror("Write to /dev/decoded failed");
        goto cleanup;
    }
    
    printf("Successfully wrote %ld bytes to /dev/decoded.\n", bytes_written);
    
    printf("Reading decoded data from /dev/decoded...\n");
    bytes_read = read(decode_fd, decoded_data, sizeof(decoded_data));
    if (bytes_read < 0) {
        perror("Read from /dev/decoded failed");
        goto cleanup;
    }

    printf("Successfully read %ld bytes of decoded data.\n", bytes_read);

    if (strcmp(original_data, decoded_data) == 0) {
        printf("Original data and decoded data matching \n");
    } else {
        printf("Original data and decoded data not not matching\n");
        printf("Original: %s\n", original_data);
        printf("Decoded:  %s\n", decoded_data);
    }
    
cleanup:
    close(encode_fd);
    close(decode_fd);

    return 0;
}