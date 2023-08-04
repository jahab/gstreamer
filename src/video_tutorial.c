#include <gst/gst.h>
#include<stdio.h>
#include <gst/app/gstappsrc.h>
#include <time.h>

//gcc video-tutorial.c -o video-tutorial `pkg-config --cflags --libs gstreamer-1.0 gstreamer-audio-1.0` -lgstapp-1.0

typedef struct _queue
{
    GstElement *aqueue;
    GstElement *vqueue;
} queue;


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
    g_print("Querying the CAPS\n");
    caps = gst_pad_query_caps (pad, NULL);

  /* Print and free */
  g_print ("Caps for the %s pad:\n", pad_name);
  print_caps (caps, "      ");
  gst_caps_unref (caps);
  gst_object_unref (pad);
}


static gboolean 
bus_call (GstBus *bus,  GstMessage *msg,  gpointer data) 

{ 
  GMainLoop *loop = (GMainLoop *) data; 
  switch (GST_MESSAGE_TYPE (msg)) { 
  case GST_MESSAGE_EOS: 
    g_print ("End of stream\n"); 
    g_main_loop_quit (loop); 
    break; 
  case GST_MESSAGE_ERROR: { 
    gchar *debug; 
    GError *error; 
    gst_message_parse_error (msg, &error, &debug); 
    g_free (debug); 
    g_printerr ("Error: %s\n", error->message); 
    g_error_free (error); 
    g_main_loop_quit (loop); 
    break; 
  } 
  default: 
  break; 
  } 
  return TRUE; 
} 

static void 
on_pad_added (GstElement *element,  GstPad *pad,  queue* data)
{ 
  GstPad *sinkpad; 
  GstCaps *caps;
//   GstElement *parser = (que *) data;
  GstStructure *new_pad_struct;
  const gchar *new_pad_type = NULL;
  /* We can now link this pad with the h264parse sink pad */ 
  caps =  gst_pad_get_current_caps (pad);

  

//   g_print ("Dynamic pad created, linking demuxer/parser\n"); 
  g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (pad), GST_ELEMENT_NAME (element));
//   g_print("%s\n", gst_caps_to_string(caps));
  caps = gst_caps_make_writable(caps);
  new_pad_struct = gst_caps_get_structure (caps, 0);
  new_pad_type = gst_structure_get_name (new_pad_struct);
//  gst_structure_remove_fields (str,"level", "profile", "height", "width", "framerate", "pixel-aspect-ratio", NULL);
  const char * lala = gst_caps_to_string (caps);
  GstCaps *ee = gst_caps_from_string(lala);
   
  // g_print("eeeeeeeeeee: %s\n",gst_caps_to_string(ee));
  g_print("%s \n",new_pad_type);
//   if (!g_str_has_prefix (new_pad_type, "audio/x-raw")) {
//     g_print ("It has type '%s' which is not raw audio. Ignoring.\n", new_pad_type);
    
//   }
  
  if (g_str_has_prefix (new_pad_type, "audio/mpeg"))
  {
    sinkpad = gst_element_get_static_pad (data->aqueue, "sink");
    gst_pad_link (pad, sinkpad); 
    gst_object_unref (sinkpad); 
  }
  else if(g_str_has_prefix (new_pad_type, "video/x-h264")){
    sinkpad = gst_element_get_static_pad (data->vqueue, "sink");
    gst_pad_link (pad, sinkpad); 
    gst_object_unref (sinkpad); 
  }
  
} 


static void
cb_typefound (GstElement *typefind, guint probability, GstCaps *caps, gpointer data)
{
  GMainLoop *loop = data;
  gchar *type, *pfx;
  pfx = "";
  g_print("ComES IN TYPEFIND CALLBACK\n");
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
}

static GstElement *filesink, *identity, *identity2;
int  main (int argc, char *argv[]) 
{ 
  GMainLoop *loop; 
  GstElement *pipeline, *source, *demuxer, *muxer, *video_parser, *video_encoder,*video_convert, *video_decoder, *video_sink; 
  GstElement *audio_conv, *audio_decoder, *audio_resample, *audio_encoder, *audio_sink; 
  GstElement *aqueue, *vqueue;
  
  GstBus *bus; 
  /* Initialisation */ 
  gst_init (&argc, &argv); 
  loop = g_main_loop_new (NULL, FALSE); 
  /* Check input arguments */ 
//   std::string filename;
  
  
//   if (argc != 2) { 
//     filename = "/home/jafar/Downloads/london_walk.mp4";
//   }else
//     filename = argv[1];


  /* Create gstreamer elements */
  pipeline = gst_pipeline_new ("mp4-player"); 
  source = gst_element_factory_make ("filesrc", "file-source"); 
  demuxer = gst_element_factory_make ("qtdemux", "demuxer"); 
  muxer = gst_element_factory_make ("mpegtsmux", "muxer");
  // muxer = gst_element_factory_make ("matroskamux", "muxer"); 
  video_parser = gst_element_factory_make ("h264parse", "video_parser"); 
  video_decoder = gst_element_factory_make ("avdec_h264", "video_decoder");
  video_convert= gst_element_factory_make ("videoconvert", "video_convert");
  video_encoder = gst_element_factory_make ("x265enc", "video_encoder");
  video_sink = gst_element_factory_make ("autovideosink", "video-output");


  audio_decoder = gst_element_factory_make ("avdec_aac", "vorbis-decoder");
  audio_conv = gst_element_factory_make ("audioconvert", "audioconverter");
  audio_resample = gst_element_factory_make ("audioresample", "audioresample");
  audio_encoder = gst_element_factory_make ("avenc_aac", "audio_encoder");
  audio_sink = gst_element_factory_make ("autoaudiosink", "audio-output");

  identity = gst_element_factory_make ("identity", "identity1");
  identity2 = gst_element_factory_make ("identity", "identity2");
  filesink = gst_element_factory_make ("filesink", "filesink");
  GstElement *fakesink = gst_element_factory_make ("fakesink", "fakesink");
  

  queue Q; 
  Q.vqueue = gst_element_factory_make ("queue", "vqueue");
  Q.aqueue = gst_element_factory_make ("queue", "aqueue");
  
  GstElement *typefind = gst_element_factory_make("typefind","typefind");
  
  GstElement *capsfilter = gst_element_factory_make("capsfilter", "capsfilter");


  if (!pipeline || !source || !demuxer || !video_parser || !video_decoder || !video_sink ||!filesink ||!muxer ||!Q.aqueue ||!Q.vqueue||
        !audio_encoder ||!audio_decoder || !audio_conv ||!identity ) { 
  g_printerr ("One element could not be created. Exiting.\n"); 
  return -1; 
  } 
  /* Set up the pipeline */ 
  /* we set the input filename to the source element */ 
  

  // GstCaps *caps = gst_caps_from_string("audio/x-raw,format=F32LE");
  // g_object_set(capsfilter, "caps", caps, NULL);
  // gst_caps_unref(caps);

  g_object_set (G_OBJECT (source), "location", "/home/jafar/Downloads/london_walk_trim.mp4",  NULL); 
  // g_object_set (G_OBJECT (source), "location", "/home/jafar/Downloads/8249720a-47f7-476c-9240-50bccc4daa23.mp4",  NULL); 

  g_object_set(G_OBJECT(filesink),"location","/home/jafar/lond_video.mkv",NULL);
  
  // GstPad *vsrcpad = gst_element_get_static_pad (audio_conv, "src");
  // GstCaps *caps = gst_pad_get_current_caps (vsrcpad);
  // GstCaps *caps = gst_pad_query_caps (vsrcpad, NULL);
  // g_print("nnnnnnnnnnnnnnnnnn%s\n",gst_caps_to_string (caps));
  // gst_caps_make_writable(caps);
  
  // GstCaps *src_caps = gst_caps_copy (caps);
  // gst_caps_set_simple(src_caps,"format",G_TYPE_STRING, "S16LE",NULL);

  // GstCaps *caps = gst_caps_new_simple ("audio/x-raw",
  //       "format", G_TYPE_STRING, "S16LE",
  //       "channels", G_TYPE_INT, 2, 
  //       "layout",G_TYPE_STRING, "non-interleaved",
  //       NULL);


  // g_object_set (vsrcpad, "caps",
  //       gst_caps_new_simple ("audio/x-raw",
  //                    "format", G_TYPE_STRING, "S16LE",
  //                    "channels",G_TYPE_INT, 2,
  //                    "layout",G_TYPE_STRING, "non-interleaved",
  //                    NULL), NULL);

  // gst_pad_use_fixed_caps(vsrcpad);
  // int aa = gst_pad_set_caps (vsrcpad, caps);
  // g_print("aa: %d\n",aa);
  // if (!aa) {
      
  //     g_print("!!!!!!!!!!Caps not set on audio convert\n");
  //   }

  

  // caps = gst_pad_query_caps (vsrcpad, NULL);
  // g_print("nnnnnnnnnnnnnnnnnn%s\n",gst_caps_to_string (caps));

  /* we add a message handler */ 
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline)); 
  gst_bus_add_watch (bus, bus_call, loop); 
  gst_object_unref (bus); 
  /* we add all elements into the pipeline */ 

  
  g_print ("Elemejts creaadded to pilelineted.\n");

  gst_bin_add_many (GST_BIN (pipeline), source, demuxer, Q.vqueue,video_parser, video_decoder,video_convert, video_encoder,muxer,filesink, NULL);  




//   gst_bin_add_many (GST_BIN (pipeline), source, demuxer, Q.vqueue, video_parser, video_decoder, video_convert,video_encoder,
//                     Q.aqueue, audio_decoder, audio_conv,audio_encoder,muxer,typefind,identity,identity2, filesink, NULL);

  
  /* we link the source demuxer together */ 
  gst_element_link (source, demuxer);

  /* Link video elements */
  if(gst_element_link_many (Q.vqueue,video_parser,video_decoder,video_convert,video_encoder,NULL)!=TRUE)
  {
    g_printerr ("Elements could not be linkedin video pipeline.\n");
    gst_object_unref (pipeline);
    return -1;
  }



  /* Link muxer elements */
  if(gst_element_link_many (muxer,filesink,NULL)!=TRUE)
  {
    g_printerr ("Elements could not be linkedin muxer pipeline.\n");
        gst_object_unref (pipeline);
        return -1;
  }


  GstPadTemplate *au_templ, *vo_templ;
  GstPad *vsrcpad,*asrcpad, *mux_video_pad, *mux_audio_pad;
  // print_pad_capabilities(video_encoder,"src");

  vo_templ = gst_element_class_get_pad_template(GST_ELEMENT_GET_CLASS(muxer), "sink_%d");
  /* Manually link the Tee, which has "Request" pads */
  mux_video_pad = gst_element_request_pad (muxer,vo_templ,NULL,NULL);
  
  vsrcpad = gst_element_get_static_pad (video_encoder, "src");
  
  if (gst_pad_link (vsrcpad, mux_video_pad) != GST_PAD_LINK_OK){
         g_printerr ("muxer could not be linked to videopad.\n");
          gst_object_unref (pipeline);
          return -1;
  }

  
  // gst_object_unref (srcpad);
  // gst_object_unref (au_templ);
  // gst_object_unref (vo_templ);
  // gst_object_unref (mux_video_pad);
  // gst_object_unref (mux_audio_pad);



  g_signal_connect (demuxer, "pad-added", G_CALLBACK (on_pad_added), &Q);
  // g_signal_connect (muxer, "pad-added", G_CALLBACK (on_pad_added_mux), &Q); 



  //   g_signal_connect (demuxer, "pad-added", G_CALLBACK (on_pad_added), aqueue); 
  //   g_signal_connect (parser, "pad-added", G_CALLBACK (on_pad_added_parser), decoder); 
  /* note that the demuxer will be linked to the decoder dynamically. 
  The reason is that Mp4 may contain various streams (for example 
  audio and video). The source pad(s) will be created at run time, 
  by the demuxer when it detects the amount and nature of streams. 
  Therefore we connect a callback function which will be executed 
  when the "pad-added" is emitted.*/ 
  /* Set the pipeline to "playing" state*/ 
  //   g_print ("Now playing: %s\n", filename.c_str());

  gst_element_set_state (pipeline, GST_STATE_PLAYING); 
  /* Iterate */ 
  g_print ("Running...\n"); 
  g_signal_connect (typefind, "have-type", G_CALLBACK (cb_typefound), loop);
  g_main_loop_run (loop); 
  /* Out of the main loop, clean up nicely */ 
  g_print ("Returned, stopping playback\n"); 
  gst_element_set_state (pipeline, GST_STATE_NULL); 
  g_print ("Deleting pipeline\n"); 
  gst_object_unref (GST_OBJECT (pipeline)); 
  return 0; 
}