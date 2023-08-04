#include <gst/gst.h>
#include<stdio.h>
#include <gst/app/gstappsrc.h>
#include <time.h>

/*                                        
                                                                         src_2       ___________       __________
                                                                              __--> |           |     |          |
 _____________        _____________      ___________       ____________      /      | identity2 |---->| filesink |
|            |       |            |     |           |     |            |   /        |___________|     |__________| 
|   appsrc   |-----> |  convert   |---->|  encoder  |---->| identity1  | /  Tee element
|____________|       |____________|     |___________|     |____________| \   
                                                                           \  
                                                                             \__     _____________
                                                                                --> |             |   
                                                                           src_1    |  fakesink   | 
                                                                                    |_____________| 
*/





typedef struct _CustomData {
  GstElement *pipeline;  /* Our one and only element */
  gboolean playing;      /* Are we in the PLAYING state? */
  gboolean terminate;    /* Should we terminate execution? */
  gboolean seek_enabled; /* Is seeking enabled for this media? */
  gboolean seek_done;    /* Have we performed the seek already? */
  gint64 duration;       /* How long does this media last, in nanoseconds */
} CustomData;


static GstElement *pipeline, *video_src, *video_sink, *videoconvert, *videoencode,*identity, *identity1, *identity2,*identity3,*queue, *fake_sink, *tee, *muxer;
static GstElement *pipeline2, *appsrc, *file_sink2, *decodebin;
GstBufferList *buflist, *copy_buflist;
static GstPad *blockpad, *probepad,*probepad2, *id2sink_pad, *fakesink_pad, *srccpad;
GstPad *tee_src1_pad, *tee_src2_pad;
GstPadTemplate *templ;
GMainLoop *loop, *loop2;
GstBuffer *buff;


static gboolean 
bus_cb (GstBus * bus, GstMessage * msg, gpointer user_data)
{
  GMainLoop *loop = user_data;
  

  switch (GST_MESSAGE_TYPE (msg)) {

    
    case GST_MESSAGE_ERROR:{
      GError *err = NULL;
      gchar *dbg;

      gst_message_parse_error (msg, &err, &dbg);
      gst_object_default_error (msg->src, err, dbg);
      g_clear_error (&err);
      g_free (dbg);
      g_main_loop_quit (loop);
      break;
    }
    case GST_MESSAGE_EOS:{
        g_print ("End-Of-Stream reached.\n");
        // g_main_loop_quit (loop2);
        gst_element_set_state (pipeline2, GST_STATE_NULL);
        // g_main_loop_unref (loop2);
        // gst_object_unref (pipeline2);
        // exit(0);
        break;
    }

    case GST_MESSAGE_WARNING:{
		GError *err = NULL;
		gchar *name, *debug = NULL;

		name = gst_object_get_path_string (msg->src);
		gst_message_parse_warning (msg, &err, &debug);

		g_printerr ("ERROR: from element %s: %s\n", name, err->message);
		if (debug != NULL)
		g_printerr ("Additional debug info:\n%s\n", debug);

		g_error_free (err);
		g_free (debug);
		g_free (name);
		break;
    }

    default:
      break;
  }
  return TRUE;
}



static gboolean 
createvideo_cb (gpointer user_data)
{

    g_print("COmes in timeout callback");
    copy_buflist = gst_buffer_list_copy_deep (buflist);
    gst_element_set_state (pipeline2, GST_STATE_PLAYING);

    GstFlowReturn retval = gst_app_src_push_buffer_list((GstAppSrc*)appsrc,copy_buflist);
    g_print("RETVAL %d\n", retval);
    g_print("Sending EOS!!!!!!!");
    g_signal_emit_by_name (appsrc, "end-of-stream", &retval);
    // gst_buffer_list_unref(copy_buflist);

    return TRUE;
}

static GstPadProbeReturn
pad_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  
  GstFlowReturn retval;
  static GstClockTime timestamp = 0;
  GstMapInfo map;
//   GstBuffer *buff;
  GstBuffer *new_buffer;

  
  g_print("pointer comes in pad probe\n");
//   g_print("probe type %d \n",info->type);
  buff = gst_pad_probe_info_get_buffer(info);

  // GST_BUFFER_PTS (buff) = timestamp;
  // GST_BUFFER_DURATION (buff) = gst_util_uint64_scale_int (1, GST_SECOND, 2);
  // timestamp += GST_BUFFER_DURATION (buff);
  
  guint buflen = gst_buffer_list_length (buflist);
  
  g_print("%ld\n",gst_buffer_get_size(buff));


  g_print("buffer length is %d \n",buflen);
//   if (buflen==200)
//   {
//     copy_buflist = gst_buffer_list_copy_deep (buflist);
//     gst_element_set_state (pipeline2, GST_STATE_PLAYING);
//     GstFlowReturn retval = gst_app_src_push_buffer_list((GstAppSrc*)appsrc,copy_buflist);
//     g_print("RETVAL %d\n", retval);
//     g_print("Sending EOS!!!!!!!");
//     g_signal_emit_by_name (appsrc, "end-of-stream", &retval);
//   }

  g_print("writable buffer %d\n",gst_buffer_is_writable (buff));
  // gst_clear_buffer (&buff);
  new_buffer =  gst_buffer_copy_deep(buff);
  g_print(" timestamp : %ld\n",GST_BUFFER_PTS (new_buffer)/1000000000);
  // g_signal_emit_by_name (appsrc, "push-buffer", new_buffer, &retval);

  g_print("new buffer size %ld\n",gst_buffer_get_size(new_buffer));

  if(buflen==50){
    gst_buffer_list_remove(buflist,0,1);
  }
  gst_buffer_list_add(buflist, new_buffer);

//   gst_buffer_unref (new_buffer);
  
  return GST_PAD_PROBE_OK;
}


static gboolean print_field (GQuark field, const GValue * value, gpointer pfx) {
  gchar *str = gst_value_serialize (value);

  g_print ("%s  %15s: %s\n", (gchar *) pfx, g_quark_to_string (field), str);
  g_free (str);
  return TRUE;
}

static void print_caps (const GstCaps * caps, const gchar * pfx) {
  guint i;

  g_return_if_fail (caps != NULL);

  if (gst_caps_is_any (caps)) {
    g_print ("%sANY\n", pfx);
    return;
  }
  if (gst_caps_is_empty (caps)) {
    g_print ("%sEMPTY\n", pfx);
    return;
  }

  for (i = 0; i < gst_caps_get_size (caps); i++) {
    GstStructure *structure = gst_caps_get_structure (caps, i);

    g_print ("%s%s\n", pfx, gst_structure_get_name (structure));
    gst_structure_foreach (structure, print_field, (gpointer) pfx);
  }
}



/* Shows the CURRENT capabilities of the requested pad in the given element */
static void print_pad_capabilities (GstElement *element, gchar *pad_name) {
  GstPad *pad = NULL;
  GstCaps *caps = NULL;

  /* Retrieve pad */
  pad = gst_element_get_static_pad (element, pad_name);
  if (!pad) {
    g_printerr ("Could not retrieve pad '%s'\n", pad_name);
    return;
  }

  /* Retrieve negotiated caps (or acceptable caps if negotiation is not finished yet) */
  caps = gst_pad_get_current_caps (pad);
  if (!caps)
    caps = gst_pad_query_caps (pad, NULL);

  /* Print and free */
  g_print ("Caps for the %s pad:\n", pad_name);
  print_caps (caps, "      ");
  gst_caps_unref (caps);
  gst_object_unref (pad);
}



static void
cb_typefound (GstElement *typefind, guint probability, GstCaps *caps, gpointer data)
{
  GMainLoop *loop = data;
  gchar *type, *pfx;
  pfx = "";
  g_print("%d\n",gst_caps_get_size (caps));

  for (guint i = 0; i < gst_caps_get_size (caps); i++) {
    GstStructure *structure = gst_caps_get_structure (caps, i);
    GstCapsFeatures *features = gst_caps_get_features (caps, i);

    g_print("features %d\n",gst_caps_features_get_size (features));
    if (features && (gst_caps_features_is_any (features) ||
            !gst_caps_features_is_equal (features,
                GST_CAPS_FEATURES_MEMORY_SYSTEM_MEMORY))) {
      gchar *features_string = gst_caps_features_to_string (features);

      g_print ("blasss %s(%s)\n", gst_structure_get_name (structure),
          features_string);
      g_free (features_string);
    } else {
      gchar *features_string = gst_caps_features_to_string (features);
      g_print ("basss%s %s %s\n", pfx, gst_structure_get_name (structure), features_string);
      gst_structure_foreach (structure, print_field, (gpointer) pfx);
    }

  }






  type = gst_caps_to_string (caps);
  g_print ("Media type %s found, probability %d%%\n", type, probability);
  g_free (type);

  /* since we connect to a signal in the pipeline thread context, we need
   * to set an idle handler to exit the main loop in the mainloop context.
   * Normally, your app should not need to worry about such things. */
  // g_idle_add (idle_exit_loop, loop);
}




int tutorial_main(int argc, char *argv[])
{

    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret,ret2;
    GstStateChangeReturn sret;
    GstStateChangeReturn pause_flag =0;
    GstState state;
    GstPad *srcpad;


    CustomData data;
    data.playing = FALSE;
    data.terminate = FALSE;
    data.seek_enabled = FALSE;
    data.seek_done = FALSE;
    data.duration = GST_CLOCK_TIME_NONE;

    gst_init (&argc, &argv);
    video_src = gst_element_factory_make ("v4l2src", "source");
    appsrc = gst_element_factory_make ("appsrc", "source");

    videoconvert = gst_element_factory_make("videoconvert","convert");
  
    videoencode = gst_element_factory_make("x264enc","encode");

    video_sink = gst_element_factory_make ("filesink", "sink");

    fake_sink = gst_element_factory_make ("fakesink", "fakesink");

    identity1 = gst_element_factory_make ("identity", "identity1");

    identity3 = gst_element_factory_make ("identity", "identity3");
    
    decodebin = gst_element_factory_make ("decodebin", "dec");

    muxer = gst_element_factory_make("mp4mux", "muxer");

    queue = gst_element_factory_make ("queue", "queue");

    tee = gst_element_factory_make("tee","tee");

    GstElement *typefind = gst_element_factory_make("typefind","typefind");

    file_sink2 = gst_element_factory_make ("filesink", "filesink");
    // file_sink2 = gst_element_factory_make ("autovideosink", "filesink");
    
    pipeline = gst_pipeline_new ("test-pipeline");
    pipeline2 = gst_pipeline_new ("filesink-pipeline");


    g_object_set (G_OBJECT (appsrc), "caps",
        gst_caps_new_simple ("video/x-raw",
                     "format", G_TYPE_STRING, "YUY2",
                     "width", G_TYPE_INT, 1920,
                     "height", G_TYPE_INT, 1080,
                     "framerate", GST_TYPE_FRACTION, 5, 1,
                     "colorimetry", G_TYPE_STRING, "2:4:5:1",
                     "interlace-mode", G_TYPE_STRING,"progressive",
                     NULL), NULL);

    g_object_set (G_OBJECT (appsrc),
        "stream-type", 0,
        "format", GST_FORMAT_TIME, NULL);

    // g_object_set(file_sink,"location","/home/jafar/gs.mp4",NULL);
    g_object_set(file_sink2,"location","/home/jafar/gpps.mp4",NULL);

    // g_object_set(G_OBJECT(appsrc), "caps", caps, NULL);

    if (!pipeline || !video_src || !video_sink ||!videoconvert ||!videoencode ||!identity1 ||!queue ||!fake_sink ||! tee ||!file_sink2) {
        g_printerr ("Not all elements could be created.\n");
        return -1;
    }

    // gst_bin_add_many (GST_BIN (pipeline), video_src,videoconvert,videoencode, identity1, tee, fake_sink, identity2,file_sink, NULL);
    // if ((gst_element_link_many(video_src,videoconvert,videoencode,identity1, tee, NULL) != TRUE )|| 
    //     (gst_element_link_many(identity2,file_sink,NULL))!=TRUE)  {
    //     g_printerr ("Elements could not be linked.\n");
    //     gst_object_unref (pipeline);
    //     return -1;
    //   }
    
    //Pipeline1 v4l2---->buffer---->fakesink
    gst_bin_add_many (GST_BIN (pipeline), video_src,typefind,identity1,fake_sink, NULL);
    if ((gst_element_link_many(video_src,typefind,identity1, fake_sink, NULL) != TRUE ))  {
        g_printerr ("Elements could not be linked.\n");
        gst_object_unref (pipeline);
        return -1;
      }

    //pipeline2 appsrc---->filesink
    //videoencode,identity3,muxer
    gst_bin_add_many (GST_BIN (pipeline2), appsrc,videoconvert,videoencode,muxer,file_sink2, NULL);
    if ((gst_element_link_many(appsrc,videoconvert,videoencode,muxer,file_sink2, NULL) != TRUE ))  {
        g_printerr ("Elements could not be linkedin pipeline2.\n");
        gst_object_unref (pipeline2);
        return -1;
      } 
    

    //Link Tee elements to fakesink and filesink
    //link tee srcpad to sinkpad of filesink
        
    // templ = gst_element_class_get_pad_template(GST_ELEMENT_GET_CLASS(tee), "src_%u");
    // tee_src1_pad = gst_element_request_pad(tee, templ,"src_1",NULL);
    // fakesink_pad = gst_element_get_static_pad(fake_sink, "sink");

    // tee_src2_pad = gst_element_request_pad(tee, templ,"src_2",NULL);
    // id2sink_pad = gst_element_get_static_pad(identity2, "sink");

    // if (gst_pad_link (tee_src1_pad, fakesink_pad) != GST_PAD_LINK_OK || 
    //     gst_pad_link (tee_src2_pad, id2sink_pad) != GST_PAD_LINK_OK)
    // {
        
    //      g_printerr ("Tee could not be linked\n");
    //      gst_object_unref (data.pipeline);
    //      return -1;
    // }


    // print_pad_capabilities (appsrc, "src");
    // print_pad_capabilities (videoconvert, "src");
    // print_pad_capabilities (videoconvert, "sink");

    // srccpad = gst_element_get_static_pad(video_src,"src");
    // gst_pad_set_caps (srccpad, caps);
    // if (!gst_pad_set_caps (srccpad, caps)) 
    // {
      
    //     g_print("ERROR in CAPS");
    //   return GST_FLOW_ERROR;
    // }
    

    //Get forurce pad
    probepad = gst_element_get_static_pad (identity1, "src");
    // probepad2 = gst_element_get_static_pad (file_sink2, "sink");
    // blockpad = gst_element_get_static_pad (tee, "src_2");

    buflist = gst_buffer_list_new();

    gst_pad_add_probe (probepad, GST_PAD_PROBE_TYPE_BUFFER, pad_probe_cb, loop, NULL);


    g_print("COde blocks here!!!\n");
    time_t now = time(NULL);

    /* Start playing */
    ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
    ret2 = gst_element_set_state (pipeline2, GST_STATE_PLAYING);

    data.pipeline = pipeline;
    /* Wait until error or EOS */
    bus = gst_element_get_bus (pipeline);    

    loop = g_main_loop_new (NULL, FALSE);
    loop2 = g_main_loop_new (NULL, FALSE);

    gst_bus_add_watch (GST_ELEMENT_BUS (data.pipeline), bus_cb, loop);
    gst_bus_add_watch (GST_ELEMENT_BUS (pipeline2), bus_cb, loop2);

    g_signal_connect (typefind, "have-type", G_CALLBACK (cb_typefound), loop);
    g_timeout_add_seconds (10, createvideo_cb, loop2);

    g_main_loop_run (loop);
    g_main_loop_run (loop2);
    
  /* Free resources */
 
  gst_object_unref (bus);
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_element_set_state (pipeline2, GST_STATE_NULL);
  gst_object_unref (blockpad);
  gst_object_unref (probepad);
  gst_bus_remove_watch (GST_ELEMENT_BUS (pipeline));
  gst_object_unref (pipeline);
  gst_object_unref (pipeline2);
  g_main_loop_unref (loop);
  g_main_loop_unref (loop2);
  return 0;

}


int
main (int argc, char *argv[])
{
#if defined(__APPLE__) && TARGET_OS_MAC && !TARGET_OS_IPHONE
  return gst_macos_main (tutorial_main, argc, argv, NULL);
#else
  return tutorial_main (argc, argv);
#endif
}