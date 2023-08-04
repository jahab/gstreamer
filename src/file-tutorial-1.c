#include<stdio.h>
#include <gst/gst.h>
#include <glib.h>
// #include <string>

// #include <iostream>

static gboolean 
bus_call (GstBus *bus, 
GstMessage *msg, 
gpointer data) 

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
on_pad_added (GstElement *element, 
GstPad *pad, 
gpointer data) 
{ 
  GstPad *sinkpad; 
  GstCaps *caps;
  GstElement *parser = (GstElement *) data; 
  GstStructure *str;
  /* We can now link this pad with the h264parse sink pad */ 
  caps =  gst_pad_get_current_caps (pad);

  g_print ("Dynamic pad created, linking demuxer/parser\n"); 
  g_print("%s\n", gst_caps_to_string(caps));
  caps = gst_caps_make_writable(caps);
  str = gst_caps_get_structure (caps, 0);
//  gst_structure_remove_fields (str,"level", "profile", "height", "width", "framerate", "pixel-aspect-ratio", NULL);
  const char * lala = gst_caps_to_string (caps);
  GstCaps *ee = gst_caps_from_string(lala);
   
//   std::cout << "ee   " << gst_caps_to_string (ee) << std::endl;
  sinkpad = gst_element_get_static_pad (parser, "sink"); 
  gst_pad_link (pad, sinkpad); 
  gst_object_unref (sinkpad); 
} 

static void 
on_pad_added_parser (GstElement *element, 
GstPad *pad, 
gpointer data) 
{ 
  GstPad *sinkpad; 
  GstCaps *caps;
  GstElement *decoder = (GstElement *) data; 
  /* We can now link this pad with the h264parse sink pad */ 
//  gst_pad_use_fixed_caps (pad);
//  caps =  gst_pad_get_current_caps (pad);
//
//  g_print ("Dynamic pad created, linking parser/decoder\n"); 
//  g_print("caps: %s\n", gst_caps_to_string(caps));
//  sinkpad = gst_element_get_static_pad (decoder, "sink"); 
//  gst_pad_link (pad, sinkpad); 
//  gst_object_unref (sinkpad); 
//  gst_element_link(element, decoder);
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






int main (int argc, char *argv[]) 
{ 
  GMainLoop *loop; 
  GstElement *pipeline, *source, *demuxer, *parser, *decoder, *conv, *sink; 
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
  parser = gst_element_factory_make ("h264parse", "parser"); 
  decoder = gst_element_factory_make ("avdec_h264", "decoder");
  GstElement* video_encoder = gst_element_factory_make ("x265enc", "video_encoder");
  GstElement *muxer = gst_element_factory_make ("mpegtsmux", "muxer"); 
  // sink = gst_element_factory_make ("autovideosink", "video-output");
  sink = gst_element_factory_make ("filesink", "video-output");
  
  GstElement *typefind = gst_element_factory_make("typefind","typefind");

  if (!pipeline || !source || !demuxer || !parser || !decoder || !sink) { 
  g_printerr ("One element could not be created. Exiting.\n"); 
  return -1; 
  } 
  /* Set up the pipeline */ 
  /* we set the input filename to the source element */ 
  
  g_object_set (G_OBJECT (source), "location", "/home/jafar/Downloads/london_walk.mp4",  NULL); 
  g_object_set (G_OBJECT (sink), "location", "/home/jafar/lond_walk.mp4",  NULL); 
  /* we add a message handler */ 
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline)); 
  gst_bus_add_watch (bus, bus_call, loop); 
  gst_object_unref (bus); 
  /* we add all elements into the pipeline */ 
  gst_bin_add_many (GST_BIN (pipeline),  source, demuxer, parser,decoder,video_encoder,muxer,typefind,sink, NULL); 
  /* we link the elements together */ 
  gst_element_link (source, demuxer); 
  gst_element_link_many (parser, decoder,video_encoder,typefind,muxer,sink, NULL); 
  g_signal_connect (demuxer, "pad-added", G_CALLBACK (on_pad_added), parser); 
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