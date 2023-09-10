#include "VideoPipe.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

VideoPipe* VideoPipeCreate(int fd, unsigned int maxPacketLength) {
    VideoPipe* videoPipe = malloc(sizeof(VideoPipe));
    if (videoPipe == NULL) {
        fprintf(stderr, "Unable to allocate memory for VideoPipe.\n");
        exit(EXIT_FAILURE);
    }
    videoPipe->fd              = fd;
    videoPipe->maxPacketLength = maxPacketLength;
    return videoPipe;
}

void VideoPipeWriteFrame(VideoPipe* videoPipe, uint64_t uTimestamp, void* start, uint32_t length) {
    uTimestamp           = htobe64(uTimestamp);
    ssize_t bytesWritten = write(videoPipe->fd, &uTimestamp, sizeof(uint64_t));
    if (bytesWritten < 0) {
        perror("Error writing timestamp to pipe.");
        exit(EXIT_FAILURE);
    }
    if (bytesWritten != sizeof(uint64_t)) {
        fprintf(stderr, "Error writing timestamp to pipe.\n");
        exit(EXIT_FAILURE);
    }
    length       = htobe32(length);
    bytesWritten = write(videoPipe->fd, &length, sizeof(uint32_t));
    if (bytesWritten < 0) {
        perror("Error writing length to pipe.");
        exit(EXIT_FAILURE);
    }
    if (bytesWritten != sizeof(uint32_t)) {
        fprintf(stderr, "Error writing length to pipe.\n");
        exit(EXIT_FAILURE);
    }
    uint32_t bytesRemaining = length;
    while (bytesRemaining > 0) {
        uint32_t bytesToWrite = bytesRemaining > videoPipe->maxPacketLength ? videoPipe->maxPacketLength : bytesRemaining;
        bytesWritten          = write(videoPipe->fd, start + length - bytesRemaining, bytesToWrite);
        if (bytesWritten < 0) {
            perror("Error writing frame to pipe.");
            exit(EXIT_FAILURE);
        }
        if (bytesWritten != bytesToWrite) {
            fprintf(stderr, "Error writing frame to pipe.\n");
            exit(EXIT_FAILURE);
        }
        bytesRemaining -= bytesWritten;
    }
}

void VideoPipeFree(VideoPipe* videoPipe) {
    free(videoPipe);
}