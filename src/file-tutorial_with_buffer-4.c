#include <gst/gst.h>
#include<stdio.h>
#include <gst/app/gstappsrc.h>
#include <time.h>

//gcc file-tutorial-4.c -o file-tutorial-4 `pkg-config --cflags --libs gstreamer-1.0 gstreamer-audio-1.0` -lgstapp-1.0
// GST_DEBUG=3,que*:5 ./file-tutorial-4 
typedef struct _queue
{
    GstElement *aqueue;
    GstElement *vqueue;
} queue;


static GstPad *blockpad, *probepad;
GstBufferList *buflist, *copy_buflist;
GstBuffer *buff;
GstElement *pipeline2;
GstElement *appsrc;

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
   
  g_print("!!!!!!!!!!!!!!%s \n",new_pad_type);
//   if (!g_str_has_prefix (new_pad_type, "audio/x-raw")) {
//     g_print ("It has type '%s' which is not raw audio. Ignoring.\n", new_pad_type);
    
//   }
  
  if (g_str_has_prefix (new_pad_type, "audio/mpeg"))
  {
    sinkpad = gst_element_get_static_pad (data->aqueue, "sink");
    g_print("comes in audio padd added \n");
    gst_pad_link (pad, sinkpad); 
    gst_object_unref (sinkpad); 
  }
  else if(g_str_has_prefix (new_pad_type, "video/x-h264")){
    g_print("comes in video padd added \n");
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



static gboolean  createvideo_cb (gpointer user_data)
{

    g_print("COmes in timeout callback");
    copy_buflist = gst_buffer_list_copy_deep (buflist);
    gst_element_set_state (pipeline2, GST_STATE_PLAYING);

    GstFlowReturn retval = gst_app_src_push_buffer_list((GstAppSrc*)appsrc,copy_buflist);
    g_print("RETVAL %d\n", retval);
    g_print("Sending EOS!!!!!!!");
    g_signal_emit_by_name (appsrc, "end-of-stream", &retval);
    // gst_buffer_list_unref(buflist);

    return TRUE;
}

static GstPadProbeReturn
pad_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  
  GstFlowReturn retval;
  static GstClockTime timestamp = 0;
  GstMapInfo map;
//   GstBuffer *buff;
  GstBuffer *new_buffer = gst_buffer_new();
//   GstBuffer *new_buffer;
//   g_print("pointer comes in pad probe\n");
//   g_print("probe type %d \n",info->type);
  buff = gst_pad_probe_info_get_buffer(info);

  // GST_BUFFER_PTS (buff) = timestamp;
  // GST_BUFFER_DURATION (buff) = gst_util_uint64_scale_int (1, GST_SECOND, 2);
  // timestamp += GST_BUFFER_DURATION (buff);
  
  guint buflen = gst_buffer_list_length (buflist);
  
  //g_print("%ld\n",gst_buffer_get_size(buff));

//   g_print("buffer length is %d \n",buflen);

//   g_print("writable buffer %d\n",gst_buffer_is_writable (buff));
  
  new_buffer =  gst_buffer_copy_deep(buff);
//   g_print(" timestamp : %ld\n",GST_BUFFER_PTS (new_buffer)/1000000000);
  //gst_clear_buffer (&buff);
//   g_print("new buffer size %ld\n",gst_buffer_get_size(new_buffer));

  if(buflen==3000){
    gst_buffer_list_remove(buflist,0,1);
  }
  gst_buffer_list_add(buflist, new_buffer);

//   gst_buffer_unref (buff);
  
  return GST_PAD_PROBE_OK;
}




static GstElement *filesink, *identity, *identity2, *file_sink2;

int  main (int argc, char *argv[]) 
{ 
  GMainLoop *loop, *loop2; 
  GstElement *pipeline, *source, *demuxer, *muxer, *video_parser, *video_encoder, *x265parser,*x265parser2, *video_convert, *video_decoder, *video_sink; 
  GstElement *audio_conv, *audio_decoder, *audio_resample, *audio_encoder, *audio_rate,*audio_sink; 
  GstElement *aqueue, *vqueue, *vqueue2, *aqueue2, *vqueue3, *aqueue3, *vqueue4, *aqueue4, *vqueue5, *vqueue6;
  GstElement *decodebin;
  


  GstBus *bus;
  /* Initialisation */ 
  gst_init (&argc, &argv); 
  loop = g_main_loop_new (NULL, FALSE); 
  loop2 = g_main_loop_new (NULL, FALSE); 



  /* Create gstreamer elements */
  pipeline = gst_pipeline_new ("mp4-player"); 
  source = gst_element_factory_make ("filesrc", "file-source"); 
  demuxer = gst_element_factory_make ("qtdemux", "demuxer"); 
  // muxer = gst_element_factory_make ("mpegtsmux", "muxer");
  muxer = gst_element_factory_make ("qtmux", "muxer"); 
  video_parser = gst_element_factory_make ("h264parse", "video_parser"); 
  video_decoder = gst_element_factory_make ("avdec_h264", "video_decoder");
  video_convert= gst_element_factory_make ("videoconvert", "video_convert");
  video_encoder = gst_element_factory_make ("x265enc", "video_encoder");
  x265parser = gst_element_factory_make ("h265parse", "h265parse");
  x265parser2 = gst_element_factory_make ("h265parse", "h265parse2");
  video_sink = gst_element_factory_make ("autovideosink", "video-output");


  audio_decoder = gst_element_factory_make ("avdec_aac", "vorbis-decoder");
  audio_conv = gst_element_factory_make ("audioconvert", "audioconverter");
  audio_rate = gst_element_factory_make ("audiorate", "audiorate");
  audio_encoder = gst_element_factory_make ("avenc_aac", "audio_encoder");
  audio_sink = gst_element_factory_make ("autoaudiosink", "audio-output");
  decodebin = gst_element_factory_make ("decodebin", "decodebin");
  identity = gst_element_factory_make ("identity", "identity1");
  identity2 = gst_element_factory_make ("identity", "identity2");
  filesink = gst_element_factory_make ("filesink", "filesink");
  GstElement *fakesink = gst_element_factory_make ("fakesink", "fakesink");
  

  queue Q; 
  Q.vqueue = gst_element_factory_make ("queue", "vqueue");
  Q.aqueue = gst_element_factory_make ("queue", "aqueue");
  vqueue2 = gst_element_factory_make ("queue", "vqueue2");
  aqueue2 = gst_element_factory_make ("queue", "aqueue2");
  vqueue3 = gst_element_factory_make ("queue", "vqueue3");
  aqueue3 = gst_element_factory_make ("queue", "aqueue3");
  vqueue4 = gst_element_factory_make ("queue", "vqueue4");
  vqueue5 = gst_element_factory_make ("queue", "vqueue5");
  vqueue6 = gst_element_factory_make ("queue", "vqueue6");
  aqueue4 = gst_element_factory_make ("queue", "aqueue4");
  GstElement *typefind = gst_element_factory_make("typefind","typefind");
  
  GstElement *capsfilter = gst_element_factory_make("capsfilter", "capsfilter");


  if (!pipeline || !source || !demuxer || !video_parser || !video_decoder || !video_sink ||!filesink ||!muxer ||!Q.aqueue ||!Q.vqueue||
        !audio_encoder ||!audio_decoder || !audio_conv ||!identity ) { 
  g_printerr ("One element could not be created. Exiting.\n"); 
  return -1; 
  } 
  /* Set up the pipeline */ 
  /* we set the input filename to the source element */ 
  

  g_object_set (G_OBJECT (source), "location", "/home/jafar/Downloads/london_walk_trim.mp4",  NULL); 

  g_object_set(G_OBJECT(filesink),"location","/home/jafar/Downloads/london_walk_trim_gst.mp4",NULL);
  
  g_object_set(G_OBJECT(vqueue6),"max-size-buffers",20000,"max-size-bytes", 1004857600,"max-size-time",30, NULL);
  
  /* we add a message handler */ 
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline)); 
  gst_bus_add_watch (bus, bus_call, loop); 
  gst_object_unref (bus); 
  /* we add all elements into the pipeline */ 

  
  g_print ("Elemejts creaadded to pilelineted.\n");
  
//   gst_bin_add_many (GST_BIN (pipeline), source, demuxer, Q.vqueue,video_parser, video_decoder,video_convert, video_encoder,x265parser,typefind,vqueue6,muxer,identity2,filesink, NULL);  
  // gst_bin_add_many (GST_BIN (pipeline), source, demuxer, Q.aqueue, audio_decoder,typefind,audio_conv,audio_encoder,muxer,identity,filesink, NULL);
  gst_bin_add_many (GST_BIN (pipeline), source, demuxer, Q.vqueue, video_parser, video_decoder, video_convert,video_encoder,
                    Q.aqueue, audio_decoder, typefind, audio_conv,audio_encoder,muxer,identity, identity2,filesink, vqueue2, vqueue3, aqueue3, vqueue6, aqueue4,x265parser, NULL);


  /* we link the source demuxer together */ 
  gst_element_link (source, demuxer);

  /* Link video elements */
  if(gst_element_link_many (Q.vqueue,video_parser,video_decoder,video_convert,video_encoder,x265parser,vqueue6,identity2, NULL)!=TRUE)
  {
    g_printerr ("Elements could not be linkedin video pipeline.\n");
    gst_object_unref (pipeline);
    return -1;
  }


  /* Link audio elements */
  if(gst_element_link_many (Q.aqueue,audio_decoder, audio_conv,aqueue3,audio_encoder,identity,NULL)!=TRUE)
  {
    g_printerr ("Elements could not be linkedin audio pipeline.\n");
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
  print_pad_capabilities(video_encoder,"src");

  vo_templ = gst_element_class_get_pad_template(GST_ELEMENT_GET_CLASS(muxer), "video_%u");
  /* Manually link the Tee, which has "Request" pads */
  mux_video_pad = gst_element_request_pad (muxer,vo_templ,NULL,NULL);
  vsrcpad = gst_element_get_static_pad (identity2, "src");
  if (gst_pad_link (vsrcpad, mux_video_pad) != GST_PAD_LINK_OK){
         g_printerr ("muxer could not be linked to videopad.\n");
          gst_object_unref (pipeline);
          return -1;
  }

  au_templ = gst_element_class_get_pad_template(GST_ELEMENT_GET_CLASS(muxer), "audio_%u");
  /* Manually link the Tee, which has "Request" pads */
  mux_audio_pad = gst_element_request_pad (muxer,au_templ,NULL,NULL);
  asrcpad = gst_element_get_static_pad (identity, "src");
  
  if (gst_pad_link (asrcpad, mux_audio_pad) != GST_PAD_LINK_OK){
    g_printerr ("muxer could not be linked to audiopad.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  
//   gst_object_unref (au_templ);
//   gst_object_unref (vo_templ);
//   gst_object_unref (mux_video_pad);
//   gst_object_unref (mux_audio_pad);
  
  g_signal_connect (demuxer, "pad-added", G_CALLBACK (on_pad_added), &Q);

  probepad = gst_element_get_static_pad (muxer, "src");
  buflist = gst_buffer_list_new();
  gst_pad_add_probe (probepad, GST_PAD_PROBE_TYPE_BUFFER, pad_probe_cb, loop, NULL);

  appsrc = gst_element_factory_make ("appsrc", "source");
  file_sink2 = gst_element_factory_make ("filesink", "filesink");
  pipeline2 = gst_pipeline_new ("filesink-pipeline");

  g_object_set (G_OBJECT (appsrc), "caps", gst_caps_new_simple ("video/x-h265",
                                                                "width", G_TYPE_INT, 1280,
                                                                "height", G_TYPE_INT, 720,
                                                                "framerate", GST_TYPE_FRACTION, 30, 1,
                                                                "colorimetry", G_TYPE_STRING, "bt709",
                                                                "interlace-mode", G_TYPE_STRING,"progressive",
                                                                "stream-format",G_TYPE_STRING,"byte-stream",
                                                                "chroma-site",G_TYPE_STRING,"mpeg2",NULL), NULL);
  
  g_object_set (G_OBJECT (appsrc),"stream-type", 0,"format", GST_FORMAT_TIME, NULL);  
  g_object_set(file_sink2,"location","/home/jafar/Downloads/buffer.mp4",NULL);

  gst_bin_add_many (GST_BIN (pipeline2), appsrc,file_sink2,  NULL);

  if ((gst_element_link_many(appsrc,file_sink2, NULL) != TRUE ))
  {
    g_printerr ("Elements could not be linked in pipeline2.\n");
    gst_object_unref (pipeline2);
    return -1;
  } 

  gst_element_set_state (pipeline, GST_STATE_PLAYING);
//   guint64 current_time = g_object_get(G_OBJECT (appsrc),"current-level-time");
  /* Iterate */ 
  g_print ("Running...\n"); 
  g_timeout_add_seconds (30, createvideo_cb, loop2);




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