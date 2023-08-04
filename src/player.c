#include <gst/gst.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Queue {
    GstElement *queue[2];
} Queue;

void get_stream_type (char **result, char *file);
static gboolean bus_call (GstBus *bus, GstMessage *msg, gpointer data);
static void on_pad_added (GstElement *element, GstPad *pad, gpointer data);

int main (int argc, char *argv[])
{
    GMainLoop *loop;

    GstElement *pipeline, *source, *demuxer, *decoder, *conv, *sink;
    GstBus *bus;
    guint bus_watch_id;

    /* Check input arguments */
    if (argc != 2) {
        g_printerr ("Usage: %s <Ogg/Vorbis filename>\n", argv[0]);
        return -1;
    }

    /* Initialisation */
    gst_init (&argc, &argv);

    loop = g_main_loop_new (NULL, FALSE);

    /* Create gstreamer elements */
    pipeline = gst_pipeline_new ("audio-player");
    source   = gst_element_factory_make ("filesrc",       "file-source");
    demuxer  = gst_element_factory_make ("oggdemux",      "ogg-demuxer");


    decoder  = gst_element_factory_make ("theoradec",     "vorbis-decoder"); 
    conv     = gst_element_factory_make ("videoconvert",  "converter");     
    sink     = gst_element_factory_make ("autovideosink", "video-output"); 

    GstElement *decoder_audio = gst_element_factory_make ("vorbisdec", "audio-decoder");
    GstElement *converter_audio = gst_element_factory_make ("audioconvert", "audio_converter");
    GstElement *sink_audio = gst_element_factory_make ("autoaudiosink", "audio-ouput");

    if (!pipeline || !source || !demuxer || !decoder || !conv || !sink ) {
        g_printerr ("One element could not be created. Exiting.\n");
        return -1;
    }

    /* Set up the pipeline */

    g_object_set (G_OBJECT (source), "location", argv[1], NULL);

    GstElement *vdoQueue = NULL;
    GstElement *audioQueue = NULL;

    char format1[10] = { 0 };
    char format2[10] = { 0 };
    char *result[2];
    result[0] = format1;
    result[1] = format2;


    get_stream_type(result, argv[1]);

    /*theora*/
    if (strcmp (format1, "theora") == 0 || strcmp (format2, "theora") == 0)  {

        vdoQueue = gst_element_factory_make ("queue", "video-queue");
        gst_bin_add_many (GST_BIN(pipeline), vdoQueue, decoder, conv, sink , NULL);
        gst_element_link_many (vdoQueue, decoder, conv, sink, NULL);
    }

    if (strcmp (format1, "vorbis") == 0 || strcmp (format2, "vorbis") == 0) {

        audioQueue = gst_element_factory_make ("queue", "audio-queue");
        gst_bin_add_many (GST_BIN(pipeline), audioQueue, 
                decoder_audio, converter_audio, sink_audio, NULL);

        gst_element_link_many (audioQueue, decoder_audio, 
                converter_audio, sink_audio, NULL);
    }

    /* we add a message handler */
    bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
    bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
    gst_object_unref (bus);

    gst_bin_add_many (GST_BIN (pipeline), source, demuxer,NULL);

    if ( gst_element_link (source, demuxer) != TRUE ){
        g_print("Elements couldn't be linked\n");
        return 1;
    }

    Queue queue;
    queue.queue[0] = audioQueue;
    queue.queue[1] = vdoQueue;

    g_signal_connect (demuxer, "pad-added", G_CALLBACK (on_pad_added), (void*)&queue);

    /* note that the demuxer will be linked to the decoder dynamically.
       The reason is that Ogg may contain various streams (for example
       audio and video). The source pad(s) will be created at run time,
       by the demuxer when it detects the amount and nature of streams.
       Therefore we connect a callback function which will be executed
       when the "pad-added" is emitted.*/

    /* Set the pipeline to "playing" state*/
    g_print ("Now playing: %s\n", argv[1]);
    gst_element_set_state (pipeline, GST_STATE_PLAYING);

    /* Iterate */
    g_print ("Running...\n");
    g_main_loop_run (loop);

    /* Out of the main loop, clean up nicely */
    g_print ("Returned, stopping playback\n");
    gst_element_set_state (pipeline, GST_STATE_NULL);

    g_print ("Deleting pipeline\n");
    gst_object_unref (GST_OBJECT (pipeline));
    g_source_remove (bus_watch_id);
    g_main_loop_unref (loop);

    return 0;
}

static void on_pad_added (GstElement *element, GstPad *pad, gpointer data)
{
    GstPad *sinkpad_audio, *sinkpad_vdo;
    Queue *queue = (Queue *) data;

    /* We can now link this pad with the vorbis-decoder sink pad */
    g_print ("Dynamic pad created, linking demuxer/decoder\n");

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
        GstPad *pad2 = gst_element_get_compatible_pad (element, sinkpad_vdo, NULL);

        if ( pad2 ) {
            gst_pad_link(pad2, sinkpad_vdo);
            gst_object_unref (GST_OBJECT (pad2));
            g_print("link video\n");
        }
        gst_object_unref (sinkpad_vdo);
    }
}

void get_stream_type(char **result, char *file) {

    char buf[110];
    FILE *fp = fopen(file, "r");

    if ( !fp ) {
        fprintf(stderr, "Open file failed\n");
        exit(1);
    }

    if ( fread (buf, sizeof(char), sizeof(buf), fp) < 0) {

        fprintf (stderr, "fread failed\n");
        exit(1);
    }

    /*extract vorbis or theora format name if it's exist
     * and record its index*/
    int i1 = 0, i2 = 0;
    for ( int i=0; i<110; i++ ) {
        if (buf[i]=='t' || buf[i]=='v') {
            if ( !i1 )
                i1 = i;
            else
                i2 = i;
        }
    }

    buf[i1+6] = buf[i2+6] = '\0';
    strcpy(result[0], &buf[i1]);
    strcpy(result[1], &buf[i2]);

    result[0][6] = result[1][6] = '\0';

    printf("%s\n", result[0]);
    printf("%s\n", result[1]);

    fclose (fp);
}

static gboolean bus_call (GstBus *bus, GstMessage *msg, gpointer data)
{
    GMainLoop *loop = (GMainLoop *) data;

    switch (GST_MESSAGE_TYPE (msg)) {

        case GST_MESSAGE_EOS:
            g_print ("End of stream\n");
            g_main_loop_quit (loop);
            break;

        case GST_MESSAGE_ERROR: 
            {
                gchar  *debug;
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