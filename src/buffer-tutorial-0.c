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


static GstElement *pipeline, *video_src, *video_sink, *videoconvert, *videoencode,*identity, *identity1, *identity2,*queue, *fake_sink, *tee, *file_sink, *muxer;
static GstElement *pipeline2, *appsrc, *file_sink2;
GstBufferList *buflist, *copy_buflist;
static GstPad *blockpad, *probepad,*probepad2, *id2sink_pad, *fakesink_pad;
GstPad *tee_src1_pad, *tee_src2_pad;
GstPadTemplate *templ;
GMainLoop *loop, *loop2;
GstBuffer *buff;

/* Forward definition of the message processing function */
static void handle_message (CustomData *data, GstMessage *msg);


static void eos_callback(GstElement *element, gpointer data)
{
    GstPad *pad = (GstPad *)data;
    g_print("EOS received on pad %s\n", GST_PAD_NAME(pad));
}

static gboolean 
bus_cb (GstBus * bus, GstMessage * msg, gpointer user_data)
{
  GMainLoop *loop = user_data;
  

//   g_print ("Position %" GST_TIME_FORMAT " / %" GST_TIME_FORMAT "\r",
//             GST_TIME_ARGS (videoconvert), GST_TIME_ARGS (data.duration));
//   g_print("bla bla %d\n",GST_MESSAGE_TYPE (msg));
//   g_print("bla bla %d\n",GST_MESSAGE_TYPE (msg));
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
		// gst_element_set_state (pipeline2, GST_STATE_NULL);
		// g_main_loop_unref (loop2);
		// gst_object_unref (pipeline2);
        exit(0);
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


static GstPadProbeReturn
block_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)    
{   
    g_print("in block prode\n");
    return GST_PAD_PROBE_OK;
}



static void start_feed (GstElement *source, guint size, gpointer user_data) {
//   if (data->sourceid == 0) {
    g_print ("Start feeding\n");
    // data->sourceid = g_idle_add ((GSourceFunc) push_data, data);
  
}


static void stop_feed (GstElement *source, guint size, gpointer user_data) {
//   if (data->sourceid == 0) {
    g_print ("enough data\n");
    // data->sourceid = g_idle_add ((GSourceFunc) push_data, data);
  
}


static GstPadProbeReturn
debug_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{   
    g_print("coming in fakesink \n");
    return GST_PAD_PROBE_OK;
}


static GstPadProbeReturn
pad_probe_buflist_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{   
    GstFlowReturn retval;
    g_print("coming in filesink Senfing EOS NOW\n");
    // g_signal_emit_by_name (appsrc, "end-of-stream", &retval);
    gst_element_send_event(pipeline2, gst_event_new_eos()); 

    return GST_PAD_PROBE_OK;
}

static GstPadProbeReturn
pad_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  
  GstFlowReturn retval;
  static GstClockTime timestamp = 0;
  
  g_print("pointer comes in pad probe\n");
//   g_print("probe type %d \n",info->type);
  buff = gst_pad_probe_info_get_buffer(info);
//   GST_BUFFER_PTS (buff) = timestamp;
//   GST_BUFFER_DURATION (buff) = gst_util_uint64_scale_int (1, GST_SECOND, 2);
//   timestamp += GST_BUFFER_DURATION (buff);
  guint buflen = gst_buffer_list_length (buflist);
  g_print("buffer length is %d \n",buflen);
  if (buflen==50)
  {
    copy_buflist = gst_buffer_list_copy_deep (buflist);
    gst_element_set_state (pipeline2, GST_STATE_PLAYING);
    // g_signal_emit_by_name (appsrc, "push-buffer-list", copy_buflist, &retval);
    // GstFlowReturn retval = gst_app_src_push_buffer_list((GstAppSrc*)appsrc,copy_buflist);

    // g_print("RETVAL %d\n", retval);

    // g_signal_emit_by_name (appsrc, "end-of-stream", &retval);
    // gst_element_send_event(pipeline2, gst_event_new_eos()); 

    for (unsigned i = 0; i < gst_buffer_list_length(copy_buflist); i++) 
    {
        // g_print("send buffer \n");
        GstBuffer* buffer = gst_buffer_list_get(copy_buflist, i);
        
        // g_print("%d ",GST_IS_BUFFER(buffer));
        // g_print("%ld\n",GST_TYPE_BUFFER);

        g_signal_emit_by_name (appsrc, "push-buffer",buffer, &retval);
        g_usleep(10000);
        // g_print("%d\n",size);
        gst_buffer_unref (buffer);
    }
    gst_element_send_event(pipeline2, gst_event_new_eos()); 
    
  }

//   retval = gst_app_src_end_of_stream ((GstAppSrc*)appsrc);
//   g_print("RETVAL %d\n", retval);

  gst_buffer_list_add(buflist, buff);
  

//   gst_buffer_unref (buff);
//   gst_buffer (copy_buflist);
  
  return GST_PAD_PROBE_OK;
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

    identity2 = gst_element_factory_make ("identity", "identity2");
    
    muxer = gst_element_factory_make("flvmux", "muxer");

    queue = gst_element_factory_make ("queue", "queue");

    tee = gst_element_factory_make("tee","tee");

    file_sink = gst_element_factory_make ("filesink", "filesink");
    file_sink2 = gst_element_factory_make ("filesink", "filesink");
    

    pipeline = gst_pipeline_new ("test-pipeline");
    pipeline2 = gst_pipeline_new ("filesink-pipeline");



    g_object_set(file_sink,"location","/home/jafar/gs.mp4",NULL);
    g_object_set(file_sink2,"location","/home/jafar/gps.mp4",NULL);

    if (!pipeline || !video_src || !video_sink ||!videoconvert ||!videoencode ||!identity1 ||!identity2 ||!queue ||!fake_sink ||! tee ||!file_sink) {
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
    gst_bin_add_many (GST_BIN (pipeline), video_src,videoconvert,videoencode,identity1,identity2,fake_sink, NULL);
    if ((gst_element_link_many(video_src,videoconvert,videoencode,identity1, identity2, fake_sink, NULL) != TRUE ))  {
        g_printerr ("Elements could not be linked.\n");
        gst_object_unref (pipeline);
        return -1;
      }

    //pipeline2 appsrc---->filesink
    gst_bin_add_many (GST_BIN (pipeline2), appsrc,muxer,file_sink2, NULL);
    if ((gst_element_link_many(appsrc,muxer,file_sink2, NULL) != TRUE ))  {
        g_printerr ("Elements could not be linked.\n");
        gst_object_unref (pipeline2);
        return -1;
      } 
    


    //Link Tee elements to fakesink and filesink
    //link tee srcpad to sinkpad of filesink
        
    // templ = gst_element_class_get_pad_template(GST_ELEMENT_GET_CLASS(tee), "src_%u");
    // tee_src1_pad = gst_element_request_pad(tee, templ,"src_1",NULL);
    fakesink_pad = gst_element_get_static_pad(fake_sink, "sink");

    // tee_src2_pad = gst_element_request_pad(tee, templ,"src_2",NULL);
    id2sink_pad = gst_element_get_static_pad(identity2, "sink");

    // if (gst_pad_link (tee_src1_pad, fakesink_pad) != GST_PAD_LINK_OK || 
    //     gst_pad_link (tee_src2_pad, id2sink_pad) != GST_PAD_LINK_OK)
    // {
        
    //      g_printerr ("Tee could not be linked\n");
    //      gst_object_unref (data.pipeline);
    //      return -1;
    // }

    
    //Get forurce pad
    probepad = gst_element_get_static_pad (identity1, "src");
    probepad2 = gst_element_get_static_pad (file_sink2, "sink");
    // blockpad = gst_element_get_static_pad (tee, "src_2");

    buflist = gst_buffer_list_new();

    
    
    gst_pad_add_probe (probepad, GST_PAD_PROBE_TYPE_BUFFER, pad_probe_cb, loop, NULL);

    // gst_pad_add_probe (probepad2, GST_PAD_PROBE_TYPE_BUFFER_LIST, pad_probe_buflist_cb, loop2, NULL);

    // gst_pad_add_probe (id2sink_pad, GST_PAD_PROBE_TYPE_IDLE, block_probe_cb, loop, NULL);

    // gst_pad_add_probe (fakesink_pad, GST_PAD_PROBE_TYPE_BUFFER, debug_probe_cb, loop, NULL);

    g_signal_connect (appsrc, "need-data", G_CALLBACK (start_feed), loop2);
    g_signal_connect (appsrc, "enough-data", G_CALLBACK (stop_feed), loop2);

    g_print("COde blocks here!!!\n");
    time_t now = time(NULL);

    /* Start playing */
    ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
    ret2 = gst_element_set_state (pipeline2, GST_STATE_PAUSED);

    data.pipeline = pipeline;
    /* Wait until error or EOS */
    bus = gst_element_get_bus (pipeline);    

    loop = g_main_loop_new (NULL, FALSE);
    loop2 = g_main_loop_new (NULL, FALSE);


    gst_bus_add_watch (GST_ELEMENT_BUS (data.pipeline), bus_cb, loop);
    gst_bus_add_watch (GST_ELEMENT_BUS (pipeline2), bus_cb, loop2);

    // g_timeout_add_seconds (20, timeout_cb, loop);
    
    g_main_loop_run (loop);
    g_main_loop_run (loop2);
    
    


//   do {
//     msg = gst_bus_timed_pop_filtered (bus, 100 * GST_MSECOND,
//         GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_DURATION);
//     // g_print("comes here\n");
//     if (pause_flag && (time(NULL)-now>8))
//     {
//         ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
//         pause_flag = 0;
//     }
//     /* Parse message */
//     if (msg != NULL) {
        
//       handle_message (&data, msg);
//     } else 
//     {
//       /* We got no message, this means the timeout expired */
//       if (data.playing) {
//         gint64 current = -1;
        
//         if (time(NULL)-now>20){
//             g_print("\nSet states to pause\n\n");
//             now = time(NULL);
//             ret = gst_element_set_state (data.pipeline, GST_STATE_PAUSED);
//             pause_flag =1;    
//         }

        
//         /* Query the current position of the stream */
//         if (!gst_element_query_position (data.pipeline, GST_FORMAT_TIME, &current)) {
//           g_printerr ("Could not query current position.\n");
//         }

//         /* If we didn't know it yet, query the stream duration */
//         if (!GST_CLOCK_TIME_IS_VALID (data.duration)) {
//           if (!gst_element_query_duration (data.pipeline, GST_FORMAT_TIME, &data.duration)) {
//             g_printerr ("Could not query current duration.\n");
//           }
//         }

//         /* Print current position and total duration */
//         g_print ("Position %" GST_TIME_FORMAT " / %" GST_TIME_FORMAT "\r",
//             GST_TIME_ARGS (current), GST_TIME_ARGS (data.duration));
        
//         // sret = gst_element_get_state(GST_ELEMENT (data.pipeline), &state, NULL, -1);
//         // g_print("blaaa %d\n",sret);



//         /* If seeking is enabled, we have not done it yet, and the time is right, seek */
//         // if (data.seek_enabled && !data.seek_done && current > 10 * GST_SECOND) {
//         //   g_print ("\nReached 10s, performing seek...\n");
//         //   gst_element_seek_simple (data.pipeline, GST_FORMAT_TIME,
//         //       GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, 30 * GST_SECOND);
//         //   data.seek_done = TRUE;
//         // }
//       }
//     }
//   } while (!data.terminate);


  /* Free resources */
 
  gst_object_unref (bus);
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (blockpad);
  gst_object_unref (probepad);
  gst_bus_remove_watch (GST_ELEMENT_BUS (pipeline));
  gst_object_unref (pipeline);
  g_main_loop_unref (loop);
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