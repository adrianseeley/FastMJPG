
#include "../../include/VideoCapture.h"
#include "../../include/VideoDecoder.h"
#include "../../include/VideoPipe.h"
#include "../../include/VideoRecorder.h"
#include "../../include/VideoRenderer.h"
#include "../../include/VideoUDPReceiver.h"
#include "../../include/VideoUDPSender.h"
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define MAX_PACKET_LENGTH 1400
#define MAX_JPEG_LENGTH 10000000

uint64_t last_capture_time = 0;
uint64_t total_delta_time  = 0;
uint64_t total_delta_times = 0;

static inline uint64_t now() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

void* sendEntry(void* arg) {
    (void)arg;
    struct sockaddr_in bindAddr = { 0 };
    bindAddr.sin_family         = AF_INET;
    bindAddr.sin_port           = htons(5000);
    bindAddr.sin_addr.s_addr    = inet_addr("127.0.0.1");
    struct sockaddr_in sendAddr = { 0 };
    sendAddr.sin_family         = AF_INET;
    sendAddr.sin_port           = htons(5001);
    sendAddr.sin_addr.s_addr    = inet_addr("127.0.0.1");

    VideoCapture*   videoCapture   = VideoCaptureCreate("/dev/video2", 1280, 720, 1, 30);
    VideoUDPSender* videoUDPSender = VideoUDPSenderCreate(MAX_PACKET_LENGTH, MAX_JPEG_LENGTH, &bindAddr, &sendAddr);
    for (;;) {
        VideoCaptureGetFrame(videoCapture);
        last_capture_time = now();
        VideoUDPSenderSendFrame(videoUDPSender, videoCapture->leasedFrameBuffer->uTimestamp, videoCapture->leasedFrameBuffer->start, videoCapture->leasedFrameBuffer->length, 1);
        VideoCaptureReturnFrame(videoCapture);
    }
}

void* receiveEntry(void* arg) {
    (void)arg;
    struct sockaddr_in bindAddr        = { 0 };
    bindAddr.sin_family                = AF_INET;
    bindAddr.sin_port                  = htons(5001);
    bindAddr.sin_addr.s_addr           = inet_addr("127.0.0.1");
    VideoUDPReceiver* videoUDPReceiver = VideoUDPReceiverCreate(MAX_PACKET_LENGTH, MAX_JPEG_LENGTH, &bindAddr);
    VideoDecoder*     videoDecoder     = VideoDecoderCreate(1280, 720);
    VideoRenderer*    videoRenderer    = VideoRendererCreate(1280, 720, 1280, 720, "benchmark");

    for (;;) {
        VideoUDPReceiverReceiveFrame(videoUDPReceiver);
        VideoDecoderDecodeFrame(videoDecoder, videoUDPReceiver->jpegBuffer, videoUDPReceiver->jpegBufferLength);
        VideoRendererRender(videoRenderer, videoDecoder->rgbBuffer);
        uint64_t render_time = now();
        uint64_t delta_time  = render_time - last_capture_time;
        total_delta_time += delta_time;
        total_delta_times++;
        if (total_delta_times % (30 * 5) == 0) {
            printf("delta: %lu useconds, frames: %lu\n", total_delta_time / total_delta_times, total_delta_times);
        }
    }
}

int main() {
    pthread_t send_thread;
    pthread_t receive_thread;
    pthread_create(&send_thread, NULL, sendEntry, NULL);
    pthread_create(&receive_thread, NULL, receiveEntry, NULL);
    pthread_join(send_thread, NULL);
    pthread_join(receive_thread, NULL);
    return 0;
}
