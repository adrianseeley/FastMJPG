#include <gst/gst.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>

typedef struct LaunchArgs {
    int    argc;
    char** argv;
} LaunchArgs;

uint64_t last_capture_time = 0;
uint64_t total_delta_time  = 0;
uint64_t total_delta_times = 0;

static inline uint64_t now() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

static GstPadProbeReturn cb_have_data(GstPad* pad, GstPadProbeInfo* info, gpointer user_data) {
    last_capture_time = now();
    return GST_PAD_PROBE_OK;
}

static GstPadProbeReturn cb_after_frame_rendered(GstPad* pad, GstPadProbeInfo* info, gpointer user_data) {
    uint64_t render_time = now();
    uint64_t delta_time  = render_time - last_capture_time;
    total_delta_time += delta_time;
    total_delta_times++;

    if (total_delta_times % (30 * 5) == 0) {
        printf("delta: %lu useconds, frames: %lu\n", total_delta_time / total_delta_times, total_delta_times);
    }
    return GST_PAD_PROBE_OK;
}

void* send(void* arg) {
    LaunchArgs* args = (LaunchArgs*)arg;
    int         argc = args->argc;
    char**      argv = args->argv;

    GstElement* tx_pipeline;
    GstElement *source, *pay, *sink;
    GstCaps*    caps;

    gst_init(&argc, &argv);

    tx_pipeline = gst_pipeline_new("send-pipeline");
    source      = gst_element_factory_make("v4l2src", "source");
    pay         = gst_element_factory_make("rtpjpegpay", "pay");
    sink        = gst_element_factory_make("udpsink", "sink");

    if (!tx_pipeline || !source || !pay || !sink) {
        g_printerr("Not all elements could be created.\n");
        exit(-1);
    }

    g_object_set(source, "device", "/dev/video2", "do-timestamp", TRUE, NULL);
    g_object_set(sink, "host", "127.0.0.1", "port", 5000, NULL);

    // Attach a probe to the "src" pad of the source element
    GstPad* srcpad = gst_element_get_static_pad(source, "src");
    gst_pad_add_probe(srcpad, GST_PAD_PROBE_TYPE_BUFFER, cb_have_data, NULL, NULL);
    gst_object_unref(srcpad);

    caps = gst_caps_from_string("image/jpeg,width=1280,height=720,framerate=30/1");
    gst_bin_add_many(GST_BIN(tx_pipeline), source, pay, sink, NULL);
    gst_element_link_filtered(source, pay, caps);
    gst_element_link_many(pay, sink, NULL);
    gst_caps_unref(caps);

    gst_element_set_state(tx_pipeline, GST_STATE_PLAYING);

    // Wait until error or EOS
    GstBus*     bus = gst_element_get_bus(tx_pipeline);
    GstMessage* msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
    if (msg != NULL) {
        gst_message_unref(msg);
    }

    gst_object_unref(bus);
    gst_element_set_state(tx_pipeline, GST_STATE_NULL);
    gst_object_unref(tx_pipeline);
}

void* receive(void* arg) {
    LaunchArgs* args = (LaunchArgs*)arg;
    int         argc = args->argc;
    char**      argv = args->argv;

    GstElement* rx_pipeline;
    GstElement *source, *depay, *decoder, *sink;
    GstPad*     sinkpad;
    GstCaps*    caps;

    gst_init(&argc, &argv);

    rx_pipeline = gst_pipeline_new("receive-pipeline");
    source      = gst_element_factory_make("udpsrc", "source");
    depay       = gst_element_factory_make("rtpjpegdepay", "depay");
    decoder     = gst_element_factory_make("jpegdec", "decoder");
    sink        = gst_element_factory_make("autovideosink", "sink");

    if (!rx_pipeline || !source || !depay || !decoder || !sink) {
        g_printerr("Not all elements could be created.\n");
        exit(-1);
    }

    g_object_set(source, "port", 5000, NULL);

    caps = gst_caps_from_string("application/x-rtp,encoding-name=JPEG,payload=26");
    g_object_set(source, "caps", caps, NULL);
    gst_caps_unref(caps);

    sinkpad = gst_element_get_static_pad(sink, "sink");
    gst_pad_add_probe(sinkpad, GST_PAD_PROBE_TYPE_BUFFER, cb_after_frame_rendered, NULL, NULL);
    gst_object_unref(sinkpad);

    gst_bin_add_many(GST_BIN(rx_pipeline), source, depay, decoder, sink, NULL);
    gst_element_link_many(source, depay, decoder, sink, NULL);

    gst_element_set_state(rx_pipeline, GST_STATE_PLAYING);

    // Wait until error or EOS
    GstBus*     bus = gst_element_get_bus(rx_pipeline);
    GstMessage* msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
    if (msg != NULL) {
        gst_message_unref(msg);
    }

    gst_object_unref(bus);
    gst_element_set_state(rx_pipeline, GST_STATE_NULL);
    gst_object_unref(rx_pipeline);
}

int main(int argc, char* argv[]) {
    LaunchArgs* args = malloc(sizeof(LaunchArgs));
    args->argc       = argc;
    args->argv       = argv;
    pthread_t send_thread;
    pthread_t receive_thread;
    pthread_create(&send_thread, NULL, send, args);
    pthread_create(&receive_thread, NULL, receive, args);
    pthread_join(send_thread, NULL);
    pthread_join(receive_thread, NULL);
    return 0;
}
