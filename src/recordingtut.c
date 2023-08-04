#include <string.h>
#include <gst/gst.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

// gst-launch-1.0 v4l2src ! x264enc ! mp4mux ! filesink location=/home/rish/Desktop/okay.264 -e
static GMainLoop *loop;
static GstElement *pipeline, *src, *encoder, *muxer, *sink, *videoconvert, *identity1, *identity2;
static GstBus *bus;

static gboolean
message_cb (GstBus * bus, GstMessage * message, gpointer user_data)
{
  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:{
      GError *err = NULL;
      gchar *name, *debug = NULL;

      name = gst_object_get_path_string (message->src);
      gst_message_parse_error (message, &err, &debug);

      g_printerr ("ERROR: from element %s: %s\n", name, err->message);
      if (debug != NULL)
        g_printerr ("Additional debug info:\n%s\n", debug);

      g_error_free (err);
      g_free (debug);
      g_free (name);

      g_main_loop_quit (loop);
      break;
    }
    case GST_MESSAGE_WARNING:{
		GError *err = NULL;
		gchar *name, *debug = NULL;

		name = gst_object_get_path_string (message->src);
		gst_message_parse_warning (message, &err, &debug);

		g_printerr ("ERROR: from element %s: %s\n", name, err->message);
		if (debug != NULL)
		g_printerr ("Additional debug info:\n%s\n", debug);

		g_error_free (err);
		g_free (debug);
		g_free (name);
		break;
    }
    case GST_MESSAGE_EOS:{
		g_print ("Got EOS\n");
		g_main_loop_quit (loop);
		gst_element_set_state (pipeline, GST_STATE_NULL);
		g_main_loop_unref (loop);
		gst_object_unref (pipeline);
		exit(0);
		break;
	}
    default:
		break;
  }

  return TRUE;
}

int sigintHandler(int unused) {
	g_print("You ctrl-c-ed! Sending EoS");
	gst_element_send_event(pipeline, gst_event_new_eos()); 
	return 0;
}

static GstPadProbeReturn
debug_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{   
    g_print("coming in fakesink \n");
    return GST_PAD_PROBE_OK;
}

int main(int argc, char *argv[])
{
	signal(SIGINT, sigintHandler);
	gst_init (&argc, &argv);

	pipeline = gst_pipeline_new(NULL);
	src = gst_element_factory_make("v4l2src", "source");
    videoconvert = gst_element_factory_make("videoconvert","convert");
	encoder = gst_element_factory_make("x264enc", "encode");
	muxer = gst_element_factory_make("flvmux", "muxer");
	sink = gst_element_factory_make("filesink", "sink");
    identity1 = gst_element_factory_make("identity", "identity1");
    identity2 = gst_element_factory_make("identity", "identity");

	if (!pipeline || !src || !encoder || !muxer || !sink) {
		g_error("Failed to create elements");
		return -1;
	}

	g_object_set(sink, "location", "/home/jafar/rec.mp4", NULL);
	gst_bin_add_many(GST_BIN(pipeline), src, videoconvert,encoder,muxer, identity1, identity2, sink, NULL);
	if (!gst_element_link_many(src, videoconvert,encoder,muxer,identity1, identity2, sink, NULL)){
		g_error("Failed to link elements");
		return -2;
	}

    

	loop = g_main_loop_new(NULL, FALSE);

	bus = gst_pipeline_get_bus(GST_PIPELINE (pipeline));
	gst_bus_add_signal_watch(bus);
	g_signal_connect(G_OBJECT(bus), "message", G_CALLBACK(message_cb), NULL);
	gst_object_unref(GST_OBJECT(bus));

    GstPad *probepad = gst_element_get_static_pad (identity1, "src");
    gst_pad_add_probe (probepad, GST_PAD_PROBE_TYPE_BUFFER, debug_probe_cb, loop, NULL);


	gst_element_set_state(pipeline, GST_STATE_PLAYING);

	g_print("Starting loop");
	g_main_loop_run(loop);

	return 0;
}