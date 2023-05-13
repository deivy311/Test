

#include <gst/gst.h>
//#include <gst/rtsp-server/rtsp-server.h>

#include <cairo.h>
#include <cairo-gobject.h>
int main(int argc, char* argv[]) {
    GstElement* pipeline, * src, * overlay,* enc, * pay, * sink;
    GstCaps* caps;
    GstBus* bus;
    GstMessage* msg;
    GMainLoop* loop;

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

    /* Set properties */
    g_object_set(G_OBJECT(pay), "config-interval", 10, NULL);
    g_object_set(G_OBJECT(pay), "pt", 96, NULL);
    //g_object_set(G_OBJECT(sink), "protocols", GST_RTSP_LOWER_TRANS_TCP, NULL);

    g_object_set(G_OBJECT(sink), "host", "127.0.0.1", NULL);
    g_object_set(G_OBJECT(sink), "port", 5000, NULL);

    /* Add elements to pipeline */
    gst_bin_add_many(GST_BIN(pipeline), src, overlay, enc, pay, sink, NULL);

   /* if (!gst_element_link_many(enc, overlay, NULL)) {
        g_printerr("Failed to link elements\n");
        return -1;
    }*/
    /* Link elements */
    if (!gst_element_link_many(src, enc,pay, sink, NULL)) {
        g_printerr("Failed to link elements\n");
        return -1;
    }
    /* Set up the overlay */
  /* Set overlay properties */
 /*   g_object_set(G_OBJECT(overlay), "xpos", 100, NULL);
    g_object_set(G_OBJECT(overlay), "ypos", 100, NULL);
    g_object_set(G_OBJECT(overlay), "valignment", "center", NULL);
    g_object_set(G_OBJECT(overlay), "halignment", "center", NULL);
    g_object_set(G_OBJECT(overlay), "draw", (GstElement*)gst_element_factory_make("textoverlay", "mytextoverlay"), NULL);
*/
    g_object_set(G_OBJECT(overlay), "text", "Hello, world!", NULL);

    /* Start playing */
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_print("Running...\n");
    int messageType = GST_MESSAGE_ERROR | GST_MESSAGE_EOS;

    /* Wait until error or EOS */
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
        (GstMessageType)messageType);

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