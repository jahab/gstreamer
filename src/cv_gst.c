#include <gst/gst.h>
#include "opencv/cv.h"
#include "opencv/highgui.h"
#include "opencv2/videoio/videoio_c.h"

static GMainLoop *loop;
CvCapture *capture;
GstBufferList *buflist;
static GstElement *pipeline;


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
cb_need_data (GstElement *appsrc,
          guint       unused_size,
          gpointer    user_data)
{

    
  static gboolean white = FALSE;
  static GstClockTime timestamp = 0;
  GstBuffer *buffer;
  guint size,depth,height,width,step,channels;
  GstFlowReturn ret;
  IplImage* img;
  guchar *data1;
  GstMapInfo map;
  
//   img=cvLoadImage("/home/jafar/2022-10-28_12-42-27.jpg",1); 
  
  img = cvQueryFrame(capture);
  
//   resizeImg2 = cvCreateImage(cvSize(416, 416), IPL_DEPTH_8U,3);

  g_print("%d\n",img->height);
  height    = img->height;  
  width     = img->width;
  
  step      = img->widthStep;
  channels  = img->nChannels;
  depth     = img->depth;
  
  data1      = (guchar *)img->imageData;
  size = height*width*channels;
  
  buffer = gst_buffer_new_allocate (NULL, size, NULL);
  gst_buffer_map (buffer, &map, GST_MAP_WRITE);
  memcpy( (guchar *)map.data, data1,  gst_buffer_get_size( buffer ) );

  /* this makes the image black/white */
  //gst_buffer_memset (buffer, 0, white ? 0xff : 0x0, size);

  white = !white;

//   GST_BUFFER_PTS (buffer) = timestamp;
//   GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale_int (1, GST_SECOND, 2);

//   timestamp += GST_BUFFER_DURATION (buffer);
  
  gst_buffer_list_add(buflist,buffer);
  
  gint buflen = gst_buffer_list_length (buflist);
  g_print("%d\n", buflen);
  if(buflen==300)
  {
    GstFlowReturn retval;
    GstBufferList *copy_buflist;
    copy_buflist = gst_buffer_list_copy_deep (buflist);
    g_signal_emit_by_name (appsrc, "push-buffer-list", copy_buflist, &retval);
    // g_signal_emit_by_name (appsrc, "end-of-stream", &retval);
    // gst_element_send_event(pipeline, gst_event_new_eos()); 
    // exit(0);
    return;
  }
  g_print("pushbuffer gets hit\n");
  g_signal_emit_by_name (appsrc, "push-buffer", buffer, &ret);
  


  if (ret != GST_FLOW_OK) {
    /* something wrong, stop pushing */
    g_main_loop_quit (loop);
  }
}

gint
main (gint   argc, gchar *argv[])
{
  GstElement *appsrc, *conv, *videosink, *enc, *muxer;

  /* init GStreamer */
  gst_init (&argc, &argv);
  loop = g_main_loop_new (NULL, FALSE);
  capture = cvCreateCameraCapture(0); 
  
  buflist = gst_buffer_list_new();


  /* setup pipeline */
  pipeline = gst_pipeline_new ("pipeline");
  appsrc = gst_element_factory_make ("appsrc", "source");
  conv = gst_element_factory_make ("videoconvert", "conv");
  enc = gst_element_factory_make ("x264enc", "enc");
  muxer = gst_element_factory_make ("flvmux", "mux");
  videosink = gst_element_factory_make ("autovideosink", "videosink");
//   videosink = gst_element_factory_make ("filesink", "videosink");

  /* setup */
  g_object_set (G_OBJECT (appsrc), "caps",
        gst_caps_new_simple ("video/x-raw",
                     "format", G_TYPE_STRING, "RGB",
                     "width", G_TYPE_INT, 640,
                     "height", G_TYPE_INT, 480,
                     "framerate", GST_TYPE_FRACTION, 20, 1,
                     NULL), NULL);


    g_object_set (G_OBJECT (appsrc), "caps",
        gst_caps_new_any() , NULL);


  gst_bin_add_many (GST_BIN (pipeline), appsrc, conv,enc,muxer,videosink, NULL);
  gst_element_link_many (appsrc, conv,videosink, NULL);
  //g_object_set (videosink, "device", "/dev/video0", NULL);
  /* setup appsrc */
  g_object_set (G_OBJECT (appsrc),
        "stream-type", 0,
        "format", GST_FORMAT_TIME, NULL);

//   g_object_set (G_OBJECT (videosink),"location","/home/jafar/jj.mp4",NULL);
  g_signal_connect (appsrc, "need-data", G_CALLBACK (cb_need_data), NULL);
  
  print_pad_capabilities (videosink, "sink");
  print_pad_capabilities (conv, "sink");
  print_pad_capabilities (conv, "src");
  
  /* play */
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  g_main_loop_run (loop);

  /* clean up */
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (pipeline));
  g_main_loop_unref (loop);

  return 0;
  }