#pragma once
#include <string>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <cairo.h>
#include <cairo-gobject.h>
#include <glib.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <overlay_manager.h>
#include <chrono>	// for measuring time
#include <iostream> // for console output
#define DEFAULT_RTSP_PORT "5001"
#define DEFAULT_RTP_PORT 5002
#define DEFAULT_FILE_PATH "../Files/video_test_2.mp4"
#define DEFAULT_RTSP_HOST "127.0.0.1"
#define DEFAULT_RTP_HOST "127.0.0.1"
#define VIDEO_WIDTH 720
#define VIDEO_HEIGHT 480
#define VIDEO_FPS 50
using namespace std;
using namespace std;
class pipeline_manager
{
public:
	// define constructor and destructor
	pipeline_manager();
	pipeline_manager(int argc, char *argv[]);
	~pipeline_manager();

public:
	static gboolean on_message(GstBus *bus, GstMessage *message, gpointer user_data);
	static gboolean my_callback_loop(GstBus *bus, GstMessage *message, gpointer user_data);

	static GstElement *setup_gst_pipeline(CairoOverlayState * overlay_state,std::string source_type, std::string host, int port);

public:
	std::string source_type = "autovideosrc";
	std::string host = DEFAULT_RTP_HOST;
	int port = DEFAULT_RTP_PORT;
	std::string RTSP_port = DEFAULT_RTSP_PORT;

};
