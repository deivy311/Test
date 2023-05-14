#include <gst/gst.h>
#include <gst/video/video.h>
#include <cairo.h>
#include <cairo-gobject.h>
#include <glib.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <RTP_manager.h>
#include <RTSP_manager.h>
#include <overlay_manager.h>
#define DEFAULT_RTSP_PORT "5001"
#define DEFAULT_RTP_PORT 5002
#define DEFAULT_FILE_PATH "../Files/video_test_2.mp4"
#define DEFAULT_RTSP_HOST "127.0.0.1"
#define DEFAULT_RTP_HOST "127.0.0.1"
#define VIDEO_WIDTH 720
#define VIDEO_HEIGHT 480
#define VIDEO_FPS 50
using namespace std;

static gboolean
on_message(GstBus *bus, GstMessage *message, gpointer user_data)
{
  GMainLoop *loop = (GMainLoop *)user_data;

  switch (GST_MESSAGE_TYPE(message))
  {
  case GST_MESSAGE_ERROR: // if error message is received
  {
    GError *err = NULL;
    gchar *debug;

    // parse the error message and get error and debug information
    gst_message_parse_error(message, &err, &debug);

    // print the error message and debug information
    g_critical("Got ERROR: %s (%s)", err->message, GST_STR_NULL(debug));

    // quit the main loop
    g_main_loop_quit(loop);
    break;
  }
  case GST_MESSAGE_WARNING: // if warning message is received
  {
    GError *err = NULL;
    gchar *debug;

    // parse the warning message and get error and debug information
    gst_message_parse_warning(message, &err, &debug);

    // print the warning message and debug information
    g_warning("Got WARNING: %s (%s)", err->message, GST_STR_NULL(debug));

    // quit the main loop
    g_main_loop_quit(loop);
    break;
  }
  case GST_MESSAGE_EOS: // if end-of-stream message is received
    // quit the main loop
    g_main_loop_quit(loop);
    break;
  default:
    break;
  }

  return TRUE;
}

/* Datastructure to share the state we are interested in between
 * prepare and render function. */
typedef struct
{
  gboolean valid;
  GstVideoInfo vinfo;
} CairoOverlayState;

static GstElement *setup_gst_pipeline(CairoOverlayState *overlay_state)
{
  /* Define variables */
  GstElement *cairo_overlay;
  GstElement *adaptor1, *adaptor2;
  GstElement *pipeline, *src, *overlay, *capsfilter, *videoconvert, *pay, *sink;
  GstRTSPMediaFactory *factory;
  gchar *str;
  GstRTSPServer *server;
  GstRTSPMountPoints *mounts;
  GstElement *videoscale;
  GstElement *encoder;
  GstElement *muxer;
  GstCaps *caps;
  GMainLoop *loop;

  /* Create pipeline and elements */
  pipeline = gst_pipeline_new("mypipeline");
  src = gst_element_factory_make("autovideosrc", "autovideosrc");          // Creates element for video capture from a device
  adaptor1 = gst_element_factory_make("videoconvert", "adaptor1");         // Converts between various video formats
  overlay = gst_element_factory_make("cairooverlay", "myoverlay");         // Adds cairo drawing on top of the video
  videoconvert = gst_element_factory_make("videoconvert", "videoconvert"); // Converts between various video formats
  videoscale = gst_element_factory_make("videoscale", "videoscale");       // Scales the video frame
  capsfilter = gst_element_factory_make("capsfilter", "capsfilter");       // Limits the capabilities of the pipeline
  caps = gst_caps_from_string("video/x-raw,width=640,height=480");         // Set the resolution of the video
  g_object_set(capsfilter, "caps", caps, NULL);                            // Set the capsfilter to limit capabilities
  gst_caps_unref(caps);
  encoder = gst_element_factory_make("x264enc", "x264enc");          // Compresses video with the x264 codec
  muxer = gst_element_factory_make("matroskamux", "matroskamux");    // Muxes different streams of data into a Matroska file format
  sink = gst_element_factory_make("tcpserversink", "tcpserversink"); // Sends video data to the client over TCP
  RTSP_manager* rtsp_server_manager = new RTSP_manager();
  overlay_manager* local_overlay_manager = new overlay_manager();
  // RTSP_manager* rtsp_server_manager = new RTSP_manager(server, mounts, factory,DEFAULT_RTSP_HOST,DEFAULT_RTSP_PORT,DEFAULT_FILE_PATH);
  
  /* Create RTSP server */
  server = gst_rtsp_server_new();
  g_object_set(server, "service", DEFAULT_RTSP_PORT, NULL); // Set the server port

  /* Create RTSP media factory */
  mounts = gst_rtsp_server_get_mount_points(server);
  str = g_strdup_printf("( "
                        "filesrc location=\"%s\" ! qtdemux name=d "
                        "d. ! queue ! rtph264pay pt=96 name=pay0 "
                        "d. ! queue ! rtpmp4apay pt=97 name=pay1 "
                        ")",
                        "../Files/video_test_2.mp4"); // Define the pipeline as a GStreamer launch command string
  factory = gst_rtsp_media_factory_new();
  gst_rtsp_media_factory_set_launch(factory, str); // Set the launch command for the factory
  g_signal_connect(factory, "media-configure", (GCallback)rtsp_server_manager->media_configure_cb,
                   factory); // Connect the media-configure callback function
  g_free(str);
  gst_rtsp_mount_points_add_factory(mounts, "/test", factory); // Add the factory to the mount points
  g_object_unref(mounts);

  /* Attach the server to the default maincontext */
  gst_rtsp_server_attach(server, NULL);
  /* Set TCP server sink properties */
  g_object_set(G_OBJECT(sink), "host", DEFAULT_RTP_HOST, NULL); // Set the host IP

  // Set the "port" property of the "sink" element to 5002
  g_object_set(G_OBJECT(sink), "port", DEFAULT_RTP_PORT, NULL);

  // Add elements to the pipeline
  gst_bin_add_many(GST_BIN(pipeline), src, adaptor1, overlay, videoconvert, videoscale, capsfilter, encoder, muxer, sink, NULL);

  // Link the elements in the pipeline
  if (!gst_element_link_many(src, adaptor1, overlay, videoconvert, videoscale, capsfilter, encoder, muxer, sink, NULL))
  {
    // If linking fails, print an error message and warning
    g_printerr("Failed to link elements\n");
    g_warning("Failed to link elements!");
  }

  // Connect the "draw" signal to the "overlay" element with the "draw_overlay" callback function and the "overlay_state" data
  g_signal_connect(overlay, "draw", G_CALLBACK(draw_overlay), overlay_state);

  // Connect the "caps-changed" signal to the "overlay" element with the "prepare_overlay" callback function and the "overlay_state" data
  g_signal_connect(overlay, "caps-changed", G_CALLBACK(prepare_overlay), overlay_state);

  return pipeline;
}

int main(int argc, char *argv[])
{
  GstElement *pipeline;             // Declare GStreamer pipeline
  GstBus *bus;                      // Declare GStreamer bus
  GMainLoop *loop;                  // Declare GMainLoop
  CairoOverlayState *overlay_state; // Declare Cairo overlay state struct

  /* Initialize GStreamer */
  gst_init(&argc, &argv);

  /* Create a new GMainLoop */
  loop = g_main_loop_new(NULL, FALSE);

  /* Allocate memory for Cairo overlay state struct on heap */
  overlay_state = g_new0(CairoOverlayState, 1);

  /* Set up the GStreamer pipeline */
  pipeline = setup_gst_pipeline(overlay_state);

  /* Start playing the pipeline */
  gst_element_set_state(pipeline, GST_STATE_PLAYING);
  g_print("Running...\n");

  /* Get the pipeline's bus */
  bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));

  /* Add a signal watch for the bus */
  gst_bus_add_signal_watch(bus);

  /* Connect the bus's "message" signal to the on_message callback function */
  g_signal_connect(G_OBJECT(bus), "message", G_CALLBACK(on_message), loop);

  /* Unreference the bus */
  gst_object_unref(GST_OBJECT(bus));

  /* Run the GMainLoop */
  g_main_loop_run(loop);

  /* Stop the pipeline */
  gst_element_set_state(pipeline, GST_STATE_NULL);

  /* Unreference the pipeline */
  gst_object_unref(pipeline);

  /* Free the Cairo overlay state struct */
  g_free(overlay_state);

  return 0;
}
