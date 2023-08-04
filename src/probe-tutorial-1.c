#include <gst/gst.h>
#include <time.h>

typedef struct _CustomData {
  GstElement *pipeline;  /* Our one and only element */
  gboolean playing;      /* Are we in the PLAYING state? */
  gboolean terminate;    /* Should we terminate execution? */
  gboolean seek_enabled; /* Is seeking enabled for this media? */
  gboolean seek_done;    /* Have we performed the seek already? */
  gint64 duration;       /* How long does this media last, in nanoseconds */
} CustomData;


static GstElement *pipeline, *video_src, *video_sink, *videoconvert, *videoencode, *qtdemux, *identity;

static GstPad *blockpad;

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
    case GST_MESSAGE_EOS:
        
        g_print ("End-Of-Stream reached.\n");
        break;
    default:
      break;
  }
  return TRUE;
}


static GstPadProbeReturn
fake_event_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
    GMainLoop *loop = user_data;

    g_print("GST EVENT TYPE on FAKE VIDEO SINK %d %d\n", GST_EVENT_EOS,GST_EVENT_TYPE (GST_PAD_PROBE_INFO_DATA (info)));  
    if (GST_EVENT_TYPE (GST_PAD_PROBE_INFO_DATA (info)) != GST_EVENT_EOS){
      gst_pad_remove_probe (pad, GST_PAD_PROBE_INFO_ID (info));
      return GST_PAD_PROBE_DROP;
    }  

    system("cp /home/jafar/gs.mp4 /home/jafar/gss.mp4");
    gst_pad_remove_probe (pad, GST_PAD_PROBE_INFO_ID (info));
    
    return GST_PAD_PROBE_REMOVE;
}


static GstPadProbeReturn
event_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  GMainLoop *loop = user_data;

  g_print("GST EVENT TYPE %d %d\n", GST_EVENT_EOS,GST_EVENT_TYPE (GST_PAD_PROBE_INFO_DATA (info)));  
  if (GST_EVENT_TYPE (GST_PAD_PROBE_INFO_DATA (info)) != GST_EVENT_EOS)
    return GST_PAD_PROBE_OK;

//   gst_pad_remove_probe (pad, GST_PAD_PROBE_INFO_ID (info));
    gst_element_set_state (identity, GST_STATE_PAUSED);
    GST_DEBUG_OBJECT (identity, "adding   %" GST_PTR_FORMAT, identity);
    system("cp /home/jafar/gs.mp4 /home/jafar/gss.mp4");
    gst_element_set_state (identity, GST_STATE_PLAYING);
    
    gst_pad_remove_probe (pad, GST_PAD_PROBE_INFO_ID (info));
    GST_DEBUG_OBJECT (identity, "done");

  return GST_PAD_PROBE_DROP;
}


static GstPadProbeReturn
pad_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  GstPad *srcpad, *sinkpad; ;//*custsinkpad;

  GST_DEBUG_OBJECT (pad, "pad is blocked now");
  g_print("source pad is blocked now\n");

  /* remove the probe first */
  gst_pad_remove_probe (pad, GST_PAD_PROBE_INFO_ID (info));

  /* install new probe for EOS */
  srcpad = gst_element_get_static_pad (identity, "src");
//   custsinkpad = gst_element_get_static_pad (video_sink, "sink");
  gst_pad_add_probe (srcpad, GST_PAD_PROBE_TYPE_BLOCK |GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM , event_probe_cb, user_data, NULL);
//   gst_pad_add_probe (custsinkpad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM , fake_event_probe_cb, user_data, NULL);
  gst_object_unref (srcpad);

  /* push EOS into the element, the probe will be fired when the
   * EOS leaves the effect and it has thus drained all of its data */
  sinkpad = gst_element_get_static_pad (identity, "sink");
  g_print("sinkpad name %s\n",gst_pad_get_name(sinkpad));
  gst_pad_send_event (sinkpad, gst_event_new_eos ());
  
//   g_print("custom sinkpad for faek event name %s\n",gst_pad_get_name(custsinkpad));
//   gst_pad_send_event (custsinkpad, gst_event_new_eos ());
//   gst_pad_remove_probe (srcpad, GST_PAD_PROBE_INFO_ID (info));
  
  gst_object_unref (sinkpad);
//   gst_object_unref (custsinkpad);

  return GST_PAD_PROBE_OK;
}



static gboolean 
timeout_cb (gpointer user_data)
{
  gst_pad_add_probe (blockpad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, pad_probe_cb, user_data, NULL);
  g_print("In time out call bak\n");
  return TRUE;
}

int tutorial_main(int argc, char *argv[])
{

    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;
    GstStateChangeReturn sret;
    GstStateChangeReturn pause_flag =0;
    GstState state;
    GstPad *srcpad;
    GMainLoop *loop;

    CustomData data;
    data.playing = FALSE;
    data.terminate = FALSE;
    data.seek_enabled = FALSE;
    data.seek_done = FALSE;
    data.duration = GST_CLOCK_TIME_NONE;

    gst_init (&argc, &argv);
    video_src = gst_element_factory_make ("v4l2src", "source");

    videoconvert = gst_element_factory_make("videoconvert","convert");
  
    videoencode = gst_element_factory_make("x264enc","encode");

    video_sink = gst_element_factory_make ("filesink", "sink");

    identity = gst_element_factory_make ("identity", "identity");

    pipeline = gst_pipeline_new ("test-pipeline");

    if (!pipeline || !video_src || !video_sink ||!videoconvert ||!videoencode ||!identity) {
        g_printerr ("Not all elements could be created.\n");
        return -1;
    }

    gst_bin_add_many (GST_BIN (pipeline), video_src,videoconvert,videoencode,identity, video_sink, NULL);
    // gst_element_link_many(video_src,videoconvert,videoencode,video_sink,NULL);
    if (gst_element_link_many(video_src,videoconvert,videoencode,identity,video_sink,NULL) != TRUE) {
        g_printerr ("Elements could not be linked.\n");
        gst_object_unref (pipeline);
        return -1;
      }


    g_object_set(video_sink,"location","/home/jafar/gs.mp4",NULL);

    /* Start playing */
    ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);

    data.pipeline = pipeline;
    /* Wait until error or EOS */
    bus = gst_element_get_bus (pipeline);
    
    g_print("COde blocks here!!!\n");
    //Get forurce pad
    blockpad = gst_element_get_static_pad (video_src, "src");

    gchar* blockpadname = gst_pad_get_name(blockpad);
    g_print("%s \n", blockpadname);
    time_t now = time(NULL);
    loop = g_main_loop_new (NULL, FALSE);

    gst_bus_add_watch (GST_ELEMENT_BUS (data.pipeline), bus_cb, loop);
    g_timeout_add_seconds (20, timeout_cb, loop);
    GstPad *video_srcpad = gst_element_get_static_pad(video_src, "src");
    GstPad *videoconvertsrcpad = gst_element_get_static_pad(videoconvert, "src");
    GstPad *videoconvertsinkpad = gst_element_get_static_pad(videoconvert, "sink");
    GstPad *videoencodesrcpad = gst_element_get_static_pad(videoencode, "src");
    GstPad *videoencodesinkpad = gst_element_get_static_pad(videoencode, "sink");
    GstPad *videosinkpad = gst_element_get_static_pad(video_sink, "sink");

    // g_signal_connect(video_src, "message::eos", G_CALLBACK(eos_callback), video_src);
    // g_signal_connect(videoconvert, "message::eos", G_CALLBACK(eos_callback), videoconvert);
    // g_signal_connect(videoconvertsinkpad, "message::eos", G_CALLBACK(eos_callback), videoconvertsinkpad);
    // g_signal_connect(G_OBJECT(videoencodesinkpad), "message::eos", G_CALLBACK(eos_callback),&data );
    // g_signal_connect(videoencodesinkpad, "eos", G_CALLBACK(eos_callback), videoencodesinkpad);
    // g_signal_connect(identity, "handoff", G_CALLBACK(eos_callback), &data);
    
    g_main_loop_run (loop);
    
    


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
  gst_bus_remove_watch (GST_ELEMENT_BUS (pipeline));
  gst_object_unref (pipeline);
  g_main_loop_unref (loop);
  return 0;


}


static void handle_message (CustomData *data, GstMessage *msg) {
  GError *err;
  gchar *debug_info;

  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_ERROR:
      gst_message_parse_error (msg, &err, &debug_info);
      g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
      g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
      g_clear_error (&err);
      g_free (debug_info);
      data->terminate = TRUE;
      break;
    case GST_MESSAGE_EOS:
      g_print ("\nEnd-Of-Stream reached.\n");
      data->terminate = TRUE;
      break;
    case GST_MESSAGE_DURATION:
      /* The duration has changed, mark the current one as invalid */
      data->duration = GST_CLOCK_TIME_NONE;
      break;
    case GST_MESSAGE_STATE_CHANGED: {
      GstState old_state, new_state, pending_state;
      gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
      if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data->pipeline)) {
        g_print ("Pipeline state changed from %s to %s:\n",
            gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));

        /* Remember whether we are in the PLAYING state or not */
        data->playing = (new_state == GST_STATE_PLAYING);

        if (data->playing) {
          /* We just moved to PLAYING. Check if seeking is possible */
          GstQuery *query;
          gint64 start, end;
          query = gst_query_new_seeking (GST_FORMAT_TIME);
          if (gst_element_query (data->pipeline, query)) {
            gst_query_parse_seeking (query, NULL, &data->seek_enabled, &start, &end);
            if (data->seek_enabled) {
              g_print ("Seeking is ENABLED from %" GST_TIME_FORMAT " to %" GST_TIME_FORMAT "\n",
                  GST_TIME_ARGS (start), GST_TIME_ARGS (end));
            } else {
              g_print ("Seeking is DISABLED for this stream.\n");
            }
          }
          else {
            g_printerr ("Seeking query failed.");
          }
          gst_query_unref (query);
        }
      }
    } break;
    default:
      /* We should not reach here */
      g_printerr ("Unexpected message received.\n");
      break;
  }
  gst_message_unref (msg);
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