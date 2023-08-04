#include <gst/gst.h>
#include<stdio.h>
#include <gst/app/gstappsrc.h>
#include <time.h>

typedef struct Queue {
    GstElement *queue[2];
} Queue;

static GstElement *pipeline, *video_src, *video_sink, *videoconvert, *videoencode,*identity, *identity1, *identity2,*identity3,*queue, *fake_sink, *tee, *file_sink, *muxer;
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
        gst_element_set_state (pipeline2, GST_STATE_NULL);
        // gst_element_set_state (pipeline2, GST_STATE_PLAYING);
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

static void
cb_typefound (GstElement *typefind,
          guint       probability,
          GstCaps    *caps,
          gpointer    data)
{

  GMainLoop *loop = data;
  gchar *type;

  type = gst_caps_to_string (caps);
  g_print ("Media type %s found, probability %d%%\n", type, probability);
  g_free (type);

  /* since we connect to a signal in the pipeline thread context, we need
   * to set an idle handler to exit the main loop in the mainloop context.
   * Normally, your app should not need to worry about such things. */
  // g_idle_add (idle_exit_loop, loop);
}



static void on_pad_added (GstElement *element, GstPad *pad, gpointer data)
{
    GstPad *sinkpad_audio, *sinkpad_vdo;
    Queue *queue = (Queue *) data;

  

    /* We can now link this pad with the vorbis-decoder sink pad */
    g_print ("Dynamic pad created, linking demuxer/decoder\n");
    g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (pad), GST_ELEMENT_NAME (element));

    GstCaps *caps =  gst_pad_get_current_caps (pad);
    g_print("%s\n", gst_caps_to_string(caps));

    /*queue*/
    if ( queue->queue[0] ) {
        sinkpad_audio = gst_element_get_static_pad (queue->queue[0], "sink");
        GstPad *pad1 = gst_element_get_compatible_pad (element, sinkpad_audio, NULL);
        if ( pad1 ) {
            gst_pad_link(pad1, sinkpad_audio);
            gst_object_unref (GST_OBJECT (pad1));
            g_print("link audio\n");
        }
        gst_object_unref (sinkpad_audio);
    }
    if ( queue->queue[1] ) {
        sinkpad_vdo = gst_element_get_static_pad (queue->queue[1], "sink");
        
        // GstPad *pad2 = gst_element_get_compatible_pad (element, sinkpad_vdo, NULL);

        if ( 1 ) {
            // gst_pad_link(pad, sinkpad_vdo);
            gst_pad_link (pad, sinkpad_vdo); 
            // gst_object_unref (sinkpad_vdo); 
            // gst_element_link(element, queue->queue[1]);
            
            // gst_object_unref (GST_OBJECT (pad2));
            g_print("link video\n");
        }
        gst_object_unref (sinkpad_vdo);
    }
}



// static void 
// on_pad_added (GstElement *element, 
// GstPad *pad, 
// gpointer data) 
// { 
//   GstPad *sinkpad; 
//   GstCaps *caps;
//   GstElement *parser = (GstElement *) data; 
//   GstStructure *str;
//   /* We can now link this pad with the h264parse sink pad */ 
//   caps =  gst_pad_get_current_caps (pad);

//   g_print ("Dynamic pad created, linking demuxer/parser\n"); 
//   g_print("%s\n", gst_caps_to_string(caps));
//   caps = gst_caps_make_writable(caps);
//   str = gst_caps_get_structure (caps, 0);
// //  gst_structure_remove_fields (str,"level", "profile", "height", "width", "framerate", "pixel-aspect-ratio", NULL);
//   const char * lala = gst_caps_to_string (caps);
//   GstCaps *ee = gst_caps_from_string(lala);
   
// //   std::cout << "ee   " << gst_caps_to_string (ee) << std::endl;
//   sinkpad = gst_element_get_static_pad (parser, "sink"); 
//   gst_pad_link (pad, sinkpad); 
//   gst_object_unref (sinkpad); 
// } 



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
//   static GstClockTime timestamp = 0;
  GstMapInfo map;
//   GstBuffer *buff;
  GstBuffer *new_buffer;

// GstCaps *caps =  gst_pad_get_current_caps (pad);
// g_print("blasssss  %s\n", gst_caps_to_string(caps));


//   g_print("probe type %d \n",info->type);
  buff = gst_pad_probe_info_get_buffer(info);

//   GST_BUFFER_PTS (buff) = timestamp;
  // GST_BUFFER_DURATION (buff) = gst_util_uint64_scale_int (1, GST_SECOND, 2);
  // timestamp += GST_BUFFER_DURATION (buff);
  
  guint buflen = gst_buffer_list_length (buflist);

//   g_print("%ld\n",gst_buffer_get_size(buflen));


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

//   g_print("writable buffer %d\n",gst_buffer_is_writable (buff));
  // gst_clear_buffer (&buff);
  new_buffer =  gst_buffer_copy_deep(buff);
  g_print(" timestamp : %ld\n",GST_BUFFER_PTS (new_buffer)/1000000000);
  // g_signal_emit_by_name (appsrc, "push-buffer", new_buffer, &retval);

  g_print("new buffer size %ld\n",gst_buffer_get_size(new_buffer));

  if(buflen==1200){
    gst_buffer_list_remove(buflist,0,1);
  }
  gst_buffer_list_add(buflist, new_buffer);

//   gst_buffer_unref (new_buffer);
  
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


    gst_init (&argc, &argv);
    video_src = gst_element_factory_make ("filesrc", "source");
    GstElement *demux = gst_element_factory_make ("qtdemux", "demux");
    
    GstElement *audio_queue = gst_element_factory_make ("queue", "aqueue");
    GstElement *video_queue = gst_element_factory_make ("queue", "vqueue");

    GstElement *video_dec = gst_element_factory_make ("mpeg2dec", "vdecoder");

    videoconvert = gst_element_factory_make("videoconvert","convert");
    videoencode = gst_element_factory_make("x264enc","encode");

    GstElement *decoder_audio = gst_element_factory_make ("vorbisdec", "audio-decoder");
    GstElement *parser = gst_element_factory_make ("h264parse", "parser"); 
    GstElement *decoder = gst_element_factory_make ("avdec_h264", "decoder"); 

    GstElement *audioconvert = gst_element_factory_make("audioconvert","aconvert");
    GstElement *audioencode = gst_element_factory_make("avenc_aac","aencode");

    identity1 = gst_element_factory_make ("identity", "identity1");
    identity2 = gst_element_factory_make ("identity", "identity2");
    identity3 = gst_element_factory_make ("identity", "identity3");
    
    decodebin = gst_element_factory_make ("decodebin", "decodebin");

    muxer = gst_element_factory_make("matroskamux", "muxer");


    fake_sink = gst_element_factory_make ("fakesink", "fakesink");


    appsrc = gst_element_factory_make ("appsrc", "source");
    video_sink = gst_element_factory_make ("filesink", "sink");
    

    queue = gst_element_factory_make ("queue", "queue");

    tee = gst_element_factory_make("tee","tee");

    GstElement *typefind = gst_element_factory_make("typefind","typefind");

    file_sink = gst_element_factory_make ("filesink", "filesink");
    file_sink2 = gst_element_factory_make ("filesink", "filesink");
    GstElement *autovideosink = gst_element_factory_make ("autovideosink", "vsink");
    GstElement *autoaudiosink = gst_element_factory_make ("autoaudiosink", "asink");
    

    pipeline = gst_pipeline_new ("test-pipeline");
    pipeline2 = gst_pipeline_new ("filesink-pipeline");


    // g_object_set (G_OBJECT (appsrc), "caps",
    //     gst_caps_new_simple ("video/x-raw",
    //                  "format", G_TYPE_STRING, "I420",
    //                  "width", G_TYPE_INT, 1280,
    //                  "height", G_TYPE_INT, 720,
    //                  "framerate", GST_TYPE_FRACTION, 30, 1,
    //                  "colorimetry", G_TYPE_STRING, "bt709",
    //                  "interlace-mode", G_TYPE_STRING,"progressive",
    //                  NULL), NULL);


    // g_object_set (G_OBJECT (appsrc), "caps",
    //     gst_caps_new_simple ("video/x-flv",
    //                  "width", G_TYPE_INT, 1920,
    //                  "height", G_TYPE_INT, 1080,
    //                  "framerate", GST_TYPE_FRACTION, 20, 1,
    //                  NULL), NULL);


    // g_object_set (G_OBJECT (appsrc), "caps",
    //         gst_caps_new_simple ("video/x-flv",
    //                     "stream-format", G_TYPE_STRING, "byte-stream",
    //                     "width", G_TYPE_INT, 1280,
    //                     "height", G_TYPE_INT, 720,
    //                     "framerate", GST_TYPE_FRACTION, 30, 1,
    //                     "colorimetry", G_TYPE_STRING, "bt709",
    //                     "interlace-mode", G_TYPE_STRING,"progressive",
    //                     NULL), NULL);

    g_object_set (G_OBJECT (appsrc), "caps",
        gst_caps_new_simple ("video/x-matroska",
                            "width", G_TYPE_INT, 1280,
                            "height", G_TYPE_INT, 720, NULL), NULL);


    // g_object_set (G_OBJECT (appsrc),
    //     "stream-type", 0,
    //     "format", GST_FORMAT_TIME, NULL);

    g_object_set(G_OBJECT(video_src),"location","/home/jafar/Downloads/london_walk.mp4",NULL);
    g_object_set(file_sink,"location","/home/jafar/gs.mp4",NULL);
    g_object_set(file_sink2,"location","/home/jafar/gpps.mp4",NULL);

    // g_object_set(G_OBJECT(appsrc), "caps", caps, NULL);

    if (!pipeline || !video_src || !video_sink ||!videoconvert ||!videoencode ||!identity1 ||!identity2 ||!queue ||!fake_sink ||! tee ||!file_sink) {
        g_printerr ("Not all elements could be created.\n");
        return -1;
    }



    gst_bin_add_many (GST_BIN (pipeline), video_src,demux , identity1,decoder,videoconvert,videoencode,parser,muxer,audio_queue,video_queue,typefind,fake_sink, NULL);

    // gst_bin_add_many (GST_BIN (pipeline), video_src,demux , identity1,decoder,parser,muxer,video_queue,autovideosink, NULL);

    if (gst_element_link_many(video_src,demux,NULL)!=TRUE)  
    {
        g_printerr ("Elements could not be linked in 2.\n");
        gst_object_unref (pipeline);
        return -1;
      }
 
    //videoconvert,videoencode,identity1,muxer,
    if (gst_element_link_many(video_queue,parser,decoder,videoconvert,videoencode,muxer,identity1,typefind,fake_sink, NULL) !=TRUE)  
    {
        g_printerr ("Elements could not be linkedin 1.\n");
        gst_object_unref (pipeline);
        return -1;
      }
    


    Queue queue;
    queue.queue[0] = NULL;//audio_queue;
    queue.queue[1] = video_queue;


    g_signal_connect (demux, "pad-added", G_CALLBACK (on_pad_added), (void*)&queue);
    // g_signal_connect (demux, "pad-added", G_CALLBACK (on_pad_added), video_queue);


    //Pipeline1 v4l2---->buffer---->fakesink
    // gst_bin_add_many (GST_BIN (pipeline), video_src,demux,decodebin,videoconvert,videoencode,muxer, typefind,identity1,identity2,fake_sink,video_queue,audio_queue, audio_convert,NULL);
    // if ( (gst_element_link_many(video_src,demux, NULL) != TRUE ) ||
    //      (gst_element_link_many(audio_queue,decodebin1,audioconvert,audioresample, NULL) != TRUE ) ||
    //      (gst_element_link_many(video_queue,decodebin2,vidioconvert, NULL) != TRUE )
    //      )  
    // {
    //     g_printerr ("Elements could not be linked.\n");
    //     gst_object_unref (pipeline);
    //     return -1;
    // }

     
    // pipeline2 appsrc---->filesink

    gst_bin_add_many (GST_BIN (pipeline2), appsrc,file_sink2,identity3, NULL);
    if ((gst_element_link_many(appsrc,file_sink2, NULL) != TRUE ))  {
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

    // GstCaps *caps;

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


    
    time_t now = time(NULL);

    /* Start playing */
    ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
    ret2 = gst_element_set_state (pipeline2, GST_STATE_PLAYING);


    /* Wait until error or EOS */
    bus = gst_element_get_bus (pipeline);    
    loop = g_main_loop_new (NULL, FALSE);
    loop2 = g_main_loop_new (NULL, FALSE);
    gst_bus_add_watch (GST_ELEMENT_BUS (pipeline), bus_cb, loop);
    // gst_bus_add_watch (GST_ELEMENT_BUS (pipeline2), bus_cb, loop2);
    g_signal_connect (typefind, "have-type", G_CALLBACK (cb_typefound), loop);
    g_timeout_add_seconds (30, createvideo_cb, loop);
    
    g_main_loop_run (loop);
    g_print("COde blocks here!!!\n");
    g_main_loop_run (loop2);

    printf("G sloop starts\n");
    
    

  /* Free resources */
 
  gst_object_unref (bus);
  gst_element_set_state (pipeline, GST_STATE_NULL);
//   gst_object_unref (blockpad);
//   gst_object_unref (probepad);
  gst_bus_remove_watch (GST_ELEMENT_BUS (pipeline));
  gst_element_set_state (pipeline, GST_STATE_NULL); 
  gst_object_unref (GST_OBJECT(pipeline));
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