#include <gst/gst.h>
#include <cairo.h>
#include <cairo-gobject.h>

static gboolean feed_overlay(GstAppSrc *src, guint size, gpointer user_data) {
    gchar *text = "Hello, world!";
    GstBuffer *buffer = gst_buffer_new_wrapped_full(0, text, strlen(text), 0, strlen(text), NULL, NULL);
    return gst_app_src_push_buffer(src, buffer) == GST_FLOW_OK;
}

int main(int argc, char* argv[]) {
    GstElement *pipeline, *src, *overlay, *enc, *pay, *sink, *appsrc;
    GstCaps *caps;
    GstBus *bus;
    GstMessage *msg;
    GMainLoop *loop;

    /* Initialize GStreamer */
    gst_init(&argc, &argv);
    loop = g_main_loop_new(NULL, FALSE);

    /* Create elements */
    pipeline = gst_pipeline_new("mypipeline");
    src = gst_element_factory_make("videotestsrc", "mysrc");
    overlay = gst_element_factory_make("textoverlay", "myoverlay");
    enc = gst_element_factory_make("x264enc", "myenc");
    pay = gst_element_factory_make("rtph264pay", "mypay");
    sink = gst_element_factory_make("udpsink", "mysink");
    appsrc = gst_element_factory_make("appsrc", "myappsrc");

    /* Set properties */
    g_object_set(G_OBJECT(pay), "config-interval", 10, NULL);
    g_object_set(G_OBJECT(pay), "pt", 96, NULL);
    g_object_set(G_OBJECT(sink), "host", "127.0.0.1", NULL);
    g_object_set(G_OBJECT(sink), "port", 5000, NULL);

    /* Add elements to pipeline */
    gst_bin_add_many(GST_BIN(pipeline), src, overlay, enc, pay, sink, appsrc, NULL);

    /* Link elements */
    if (!gst_element_link_many(src, enc, pay, sink, NULL)) {
        g_printerr("Failed to link video elements\n");
        return -1;
    }
    if (!gst_element_link_many(appsrc, overlay, enc, pay, sink, NULL)) {
        g_printerr("Failed to link overlay elements\n");
        return -1;
    }

    /* Set up the appsrc */
    g_object_set(G_OBJECT(appsrc), "format", GST_FORMAT_TIME, NULL);
    g_object_set(G_OBJECT(appsrc), "is-live", TRUE, NULL);
    g_object_set(G_OBJECT(appsrc), "emit-signals", TRUE, NULL);
    g_signal_connect(appsrc, "need-data", G_CALLBACK(feed_overlay), NULL);

    /* Start playing */
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_print("Running...\n");

    /* Wait until error or EOS */
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
        GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    /* Free resources */
    if (msg != NULL) {
        gst_message_unref(msg);
    }
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_main_loop_unref(loop);

    return 0;
}
