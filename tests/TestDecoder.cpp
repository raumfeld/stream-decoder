
#include <stdlib.h>
#include <glib.h>
#include <gst/gst.h>

#include "AudioConverter.h"
#include "Resampler.h"

extern "C"
{
  static GMainLoop *s_theMainLoop = NULL;
  static bool       s_bQuit       = FALSE;

  static AudioConverter *s_audioConverter = NULL;
  static Resampler      *s_resampler      = NULL;
  static GstElement     *s_pipeline       = NULL;
  static GTimer         *s_converterTimer = NULL;
  static GTimer         *s_resamplerTimer = NULL;
  static guint           s_sampleRate     = 44100;

  static void quit (int sig)
  {
    if (s_theMainLoop && !s_bQuit)
    {
      s_bQuit = true;
      g_main_loop_quit (s_theMainLoop);
    }
  }

  static void sigPipe (int sig)
  {
  }

  static void hand_off_data (GstElement *fakesink,
                             GstBuffer  *buffer,
                             GstPad     *pad)
  {
    GstCaps *caps = pad ? gst_pad_get_current_caps(pad) : NULL;

    if (caps)
    {
      GstStructure *structure = gst_caps_get_structure (caps, 0);

      if (! s_audioConverter)
      {
        s_audioConverter = new AudioConverter (caps);
      }

      gint srcSR = 44100;
      gst_structure_get_int (structure, "rate", &srcSR);

      if (! s_resampler)
      {
	s_resampler = new Resampler (srcSR, s_sampleRate);
	g_print ("Created audio resampler (%d -> %u)\n", srcSR, s_sampleRate);
      }

      gst_caps_unref (caps);

      if (s_resampler && s_audioConverter)
      {
        g_timer_continue (s_converterTimer);
        GstBuffer *converted = s_audioConverter->eat (buffer);
        g_timer_stop (s_converterTimer);

        g_timer_continue (s_resamplerTimer);
        GstBuffer *resampled = s_resampler->eat (converted);
        g_timer_stop (s_resamplerTimer);

        GstMapInfo info;

        if (gst_buffer_map (resampled, &info, GST_MAP_READ))
        {
          // ssize_t written = write(pipe, info.data, info.size);

          gst_buffer_unmap (resampled, &info);
        }

        gst_buffer_unref (resampled);
        gst_buffer_unref (converted);
      }
    }
  }

  static void pad_added_cb (GstElement *element,
			    GstPad     *pad,
			    GstElement *pipeline)
  {
    GstElement *sink = gst_bin_get_by_name (GST_BIN (pipeline), "sink");
    GstPad     *sinkpad = gst_element_get_static_pad (sink, "sink");

    if (gst_pad_link (pad, sinkpad) != GST_PAD_LINK_OK)
    {
      g_warning ("Stream type not supported");
    }

    gst_object_unref (sinkpad);
  }

  static gboolean on_bus_event (GstBus     *bus,
				GstMessage *message)
  {
    GError *error = NULL;

    switch (GST_MESSAGE_TYPE (message))
    {
      case GST_MESSAGE_EOS:
        g_print ("End of input stream\n");
        quit (0);
        break;

      case GST_MESSAGE_ERROR:
        gst_message_parse_error (message, &error, NULL);
        g_print("Error: %s", error->message);
	quit (0);
      break;

      default:
        break;
    }

    return true;
  }

  static GstElement *build_pipeline (void)
  {
    GstElement *pipeline = gst_pipeline_new ("pipeline");
    GstElement *source   = gst_element_factory_make ("filesrc", "source");
    GstElement *decoder  = gst_element_factory_make ("decodebin", "decoder");
    GstElement *sink     = gst_element_factory_make ("fakesink", "sink");

    gst_bin_add_many (GST_BIN (pipeline), source, decoder, sink, NULL);
    gst_element_link_many (source, decoder, NULL);

    /* listen for newly created pads */
    g_signal_connect (decoder, "pad-added",
		      G_CALLBACK (pad_added_cb),
		      pipeline);

    /* setup the fake sink */
    g_object_set (G_OBJECT (sink), "signal-handoffs", TRUE, NULL);

    /* callback for audio data */
    g_signal_connect (sink, "handoff",
		      G_CALLBACK (hand_off_data),
		      pipeline);

    GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
    gst_bus_add_watch (bus, (GstBusFunc) on_bus_event, NULL);
    gst_object_unref (bus);

    return pipeline;
  }

  static gboolean start (const gchar *location)
  {
    GstElement *src = gst_bin_get_by_name (GST_BIN (s_pipeline), "source");

    g_object_set (src, "location", location, NULL);
    gst_element_set_state (s_pipeline, GST_STATE_PLAYING);

    return FALSE;
  }

  int main (int argc,
	    char *argv[])
  {
    GTimer *overallTimer;

    signal (SIGINT, quit);
    signal (SIGQUIT, quit);
    signal (SIGTERM, quit);
    signal (SIGPIPE, sigPipe);

    gst_init (&argc, &argv);

    gst_debug_set_default_threshold (GST_LEVEL_WARNING);

    s_pipeline = build_pipeline ();

    if (argc > 1)
      g_idle_add ((GSourceFunc) start, argv[1]);

    s_converterTimer = g_timer_new ();
    s_resamplerTimer = g_timer_new ();

    g_timer_stop (s_converterTimer);
    g_timer_stop (s_resamplerTimer);

    overallTimer = g_timer_new ();

    s_theMainLoop = g_main_loop_new (NULL, TRUE);
    g_main_loop_run (s_theMainLoop);

    g_timer_stop (overallTimer);

    g_main_loop_unref (s_theMainLoop);

    gfloat timeElapsed         = g_timer_elapsed (overallTimer, NULL);
    gfloat timeSpentConverting = g_timer_elapsed (s_converterTimer, NULL);
    gfloat timeSpentResampling = g_timer_elapsed (s_resamplerTimer, NULL);

    g_print (" time elapsed:                 %7.3f sec\n", timeElapsed);
    g_print (" time spent in AudioConverter: %7.3f sec (%4.1f %%)\n",
             timeSpentConverting, timeSpentConverting * 100.0 / timeElapsed);
    g_print (" time spent in Resampler:      %7.3f sec (%4.1f %%)\n",
             timeSpentResampling, timeSpentResampling * 100.0 / timeElapsed);

    return EXIT_SUCCESS;
  }

}

