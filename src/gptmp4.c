#include <gst/gst.h>

int main(int argc, char *argv[]) {
    GstElement *pipeline, *source, *audioQueue, *audioDecoder, *audioConverter, *videoQueue, *videoDecoder, *videoConverter, *muxer, *sink;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;

    // Initialize GStreamer
    gst_init(&argc, &argv);

    // Create the elements
    pipeline = gst_pipeline_new("muxer-pipeline");
    source = gst_element_factory_make("filesrc", "file-source");
    audioQueue = gst_element_factory_make("queue", "audio-queue");
    audioDecoder = gst_element_factory_make("decodebin", "audio-decoder");
    audioConverter = gst_element_factory_make("audioconvert", "audio-converter");
    videoQueue = gst_element_factory_make("queue", "video-queue");
    videoDecoder = gst_element_factory_make("decodebin", "video-decoder");
    videoConverter = gst_element_factory_make("videoconvert", "video-converter");
    muxer = gst_element_factory_make("mp4mux", "muxer");
    sink = gst_element_factory_make("filesink", "file-sink");

    if (!pipeline || !source || !audioQueue || !audioDecoder || !audioConverter || !videoQueue || !videoDecoder || !videoConverter || !muxer || !sink) {
        g_printerr("One or more elements could not be created. Exiting.\n");
        return -1;
    }

    // Set the input file path
    g_object_set(G_OBJECT(source), "location", "/home/jafar/Downloads/london_walk.mp4", NULL);

    // Set the output file path
    g_object_set(G_OBJECT(sink), "location", "/home/jafar/lon.mp4", NULL);

    // Add elements to the pipeline
    gst_bin_add_many(GST_BIN(pipeline), source, audioQueue, audioDecoder, audioConverter, videoQueue, videoDecoder, videoConverter, muxer, sink, NULL);

    // Link the elements
    if (!gst_element_link_many(source, audioQueue, audioDecoder,audioConverter,muxer, NULL))
    {
        g_printerr("Elements audio could not be linked. Exiting.\n");
        return -1;
    }
    

    if (!gst_element_link_many(source, videoQueue, videoDecoder, videoConverter, muxer, NULL))
    {
        g_printerr("Elements mux could not be linked. Exiting.\n");
        return -1;
    }

    if (!gst_element_link(muxer, sink)) 
    {
        g_printerr("Elements could not be linked. Exiting.\n");
        return -1;   
    }
    // if (!gst_element_link_many(source, audioQueue, audioDecoder, NULL) ||
    //     !gst_element_link_many(audioDecoder, audioConverter, muxer, NULL) ||
    //     !gst_element_link_many(source, videoQueue, videoDecoder, videoConverter, muxer, NULL) ||
    //     !gst_element_link(muxer, sink)) {
    //     g_printerr("Elements could not be linked. Exiting.\n");
    //     return -1;
    // }

    // Set the pipeline to the playing state
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state. Exiting.\n");
        return -1;
    }

    // Wait until error or EOS (End-Of-Stream) message occurs
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    // Parse message
    if (msg != NULL) {
        GError *err;
        gchar *debug_info;

        switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(msg, &err, &debug_info);
                g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
                g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
                g_clear_error(&err);
                g_free(debug_info);
                break;
            case GST_MESSAGE_EOS:
                g_print("End-Of-Stream reached.\n");
                break;
            default:
                // Unexpected message received
                g_printerr("Unexpected message received.\n");
                break;
        }
        gst_message_unref(msg);
    }

    // Free resources
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}