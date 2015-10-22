#include "Pipeline.h"
#include "SupportedProtocols.h"
#include <unistd.h>
#include <thread>
#include <gio/gunixoutputstream.h>
#include "Trace.h"
#include <string.h>
#include <errno.h>

Pipeline::Pipeline (uint64_t stream_id, const gchar* uri, GVariant *allowedSamplerates) :
    m_id (stream_id), m_uri (uri), m_pipelineWatch(0), m_audioPipe(NULL), m_renderersPipe(0), m_close(false)
{
  m_cancellable = g_cancellable_new();
  setAllowedSamplerates (allowedSamplerates);
}

Pipeline::~Pipeline ()
{
  Tracer::overdose( __PRETTY_FUNCTION__ );

  m_close = true;

  close(m_renderersPipe);

  if (m_pipelineWatch > 0)
    g_source_remove (m_pipelineWatch);

  if (m_pipeline)
  {
    gst_element_set_state(m_pipeline, GST_STATE_NULL);
    g_cancellable_cancel(m_cancellable);
    gst_object_unref (m_pipeline);
  }

  if(m_audioPipe)
    g_object_unref(m_audioPipe);

  g_object_unref(m_cancellable);
}

void Pipeline::setAllowedSamplerates (GVariant *allowedSamplerates)
{
  gsize numRates = 0;
  const guint32 *allowedRates = (const guint32 *) g_variant_get_fixed_array (allowedSamplerates, &numRates, sizeof(guint32));

  for (gsize i = 0; i < numRates; i++)
  {
    m_allowedSampleRates.push_back (allowedRates[i]);
    Tracer::overdose("sample rate:", allowedRates[i]);
  }
}

gint32 Pipeline::init ()
{
  Tracer::overdose( __PRETTY_FUNCTION__ );

  int pipefd[2];
  int ret = ::pipe (pipefd);
  if (ret == 0)
  {
    m_audioPipe = g_unix_output_stream_new (pipefd[1], TRUE);
    m_renderersPipe = pipefd[0];
    setupGStreamer ();

    return m_renderersPipe;
  }
  else {
    Tracer::alarm( "Failed to create pipe", strerror(errno) );
  }

  perror("pipe");
  return -1;
}

void Pipeline::setMessageCallback(tMessageCallback cb)
{
  m_messageCallback = cb;
}

void Pipeline::resetStats()
{
  m_stats = 0;
}

void Pipeline::setupGStreamer ()
{
  m_pipeline = gst_pipeline_new ("pipeline");

  if (GstElement* httpsource = gst_element_factory_make ("souphttpsrc", "httpsource"))
  {
    if (GstElement* decodebin = gst_element_factory_make ("decodebin", "decoder"))
    {
      if (GstElement* sink = gst_element_factory_make ("fakesink", "sink"))
      {
        if (gst_bin_add (GST_BIN (m_pipeline), httpsource))
        {
          if (gst_bin_add (GST_BIN (m_pipeline), decodebin))
          {
            if (gst_bin_add (GST_BIN (m_pipeline), sink))
            {
              if (gst_element_link_many (httpsource, decodebin, NULL))
              {
                g_signal_connect (decodebin, "pad-added", G_CALLBACK (&Pipeline::onPadAdded), this);
                g_object_set (G_OBJECT (sink), "signal-handoffs", TRUE, NULL);
                g_signal_connect (sink, "handoff", G_CALLBACK (&Pipeline::onAudioDataDecoded), this);

                GstBus* bus = gst_pipeline_get_bus (GST_PIPELINE (m_pipeline));
                m_pipelineWatch = gst_bus_add_watch (bus, (GstBusFunc) (&Pipeline::onBusEvent), this);
                gst_object_unref (bus);
                gst_debug_set_default_threshold (GST_LEVEL_WARNING);

                g_object_set (httpsource, "location", m_uri.c_str(), NULL);
                gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
                return;
              }
              else
                Tracer::warning ("Pipeline: unable to add link from httpsource to decodebin");

              sink = NULL;
            }
            else
              Tracer::warning ("Pipeline: unable to add sink");

            decodebin = NULL;
          }
          else
            Tracer::warning ("Pipeline: unable to add decodebin");

          httpsource = NULL;
        }
        else
          Tracer::warning ("Pipeline: unable to add httpsource");

        if (sink)
          gst_object_unref (sink);
      }
      if (decodebin)
        gst_object_unref (decodebin);
    }
    if (httpsource)
      gst_object_unref (httpsource);
  }
}

void Pipeline::onPadAdded (GstElement *element, GstPad *pad, Pipeline *pThis)
{
  if(GstElement *fakeSink = gst_bin_get_by_name (GST_BIN (pThis->m_pipeline), "sink"))
  {
    if(GstPad *sinkpad = gst_element_get_static_pad (fakeSink, "sink"))
    {
      if (gst_pad_link (pad, sinkpad) != GST_PAD_LINK_OK)
      {
        g_print ("!!->StreamDecoder: pad added failed!\n");
        pThis->sendMessage("error", "Stream type not supported");
      }

      gst_object_unref (sinkpad);
    }
    gst_object_unref (fakeSink);
  }
}

void Pipeline::onAudioDataDecoded (GstElement *fakesink, GstBuffer *buffer, GstPad *pad, Pipeline *pThis)
{
  GstCaps *caps = pad ? gst_pad_get_current_caps (pad) : NULL;
  pThis->handOffData (caps, buffer);

  if(caps)
    gst_caps_unref (caps);
}

void Pipeline::handOffData (GstCaps *caps, GstBuffer *buffer)
{
  if (caps && !m_close)
    setupAudioProcessors (caps);

  if (m_resampler && m_audioConverter && !m_close)
    processAndSendAudioData (buffer);
}

void Pipeline::setupAudioProcessors (GstCaps* caps)
{
  setupAudioConverter (caps);
  setupResampler (caps);
}

void Pipeline::setupAudioConverter (GstCaps* caps)
{
  if (!m_audioConverter)
    m_audioConverter.reset (new AudioConverter (caps));
}

void Pipeline::setupResampler (GstCaps* caps)
{
  GstStructure* structure = gst_caps_get_structure (caps, 0);
  gint srcSR = 44100;
  gint tgtSR = srcSR;
  gst_structure_get_int (structure, "rate", &srcSR);

  if (!m_resampler)
  {
    tgtSR = chooseSamplerate (srcSR);
    m_resampler.reset (new Resampler (srcSR, tgtSR));
    gsize bytesWritten = 0;
    g_output_stream_write_all (m_audioPipe, &tgtSR, 4, &bytesWritten, m_cancellable, NULL);
  }
  else if (m_resampler->getSourceSR () != srcSR)
  {
    // some radio stations change the sample frequency in the middle of the stream due to ads
    // in this case, resample to the sample rate transmitted to the renderer before
    m_resampler.reset (new Resampler (srcSR, tgtSR));
  }
}

void Pipeline::processAndSendAudioData (GstBuffer* buffer)
{
  Tracer::overdose( __PRETTY_FUNCTION__ );

  gsize bytesWritten = 0;

  GstBuffer* converted = m_audioConverter->eat (buffer);
  GstBuffer* resampled = m_resampler->eat (converted);

  GstMapInfo info;

  if (gst_buffer_map (resampled, &info, GST_MAP_READ))
  {
    if (info.size > 0)
    {
      m_stats += info.size;

      GError *error = nullptr;
      if( !g_output_stream_write_all (m_audioPipe, info.data, info.size, &bytesWritten, m_cancellable, &error))
      {
        g_printerr ("g_output_stream_write_all failed: %s\n", error->message);
        g_error_free (error);
      }
    }

    gst_buffer_unmap (resampled, &info);
  }

  gst_buffer_unref (resampled);
  gst_buffer_unref (converted);
}

guint32 Pipeline::chooseSamplerate (guint32 sourceRate) const
{
  Tracer::overdose( __PRETTY_FUNCTION__, "sourcerate:", sourceRate );

  guint32 chosenRate = 0;

  for (guint32 sr : m_allowedSampleRates)
  {
    g_assert (chosenRate < sr); // rates have to be sorted!

    if (chosenRate < sourceRate && chosenRate < sr)
      chosenRate = sr;
  }

  Tracer::overdose( "chosenRate:", chosenRate );
  return chosenRate;
}

gboolean Pipeline::onBusEvent (GstBus *bus, GstMessage *message, Pipeline *pThis)
{
  Tracer::overdose( __PRETTY_FUNCTION__ );

  GError *error = NULL;
  gchar *debugString = NULL;

  switch (GST_MESSAGE_TYPE (message))
  {
    case GST_MESSAGE_STATE_CHANGED:
      break;

    case GST_MESSAGE_EOS:
      pThis->sendMessage("eos", "End of stream");
      g_output_stream_close (pThis->m_audioPipe, pThis->m_cancellable, NULL);
      break;

    case GST_MESSAGE_ERROR:
      gst_message_parse_error (message, &error, &debugString);

      if (debugString)
      {
        g_print ("->StreamDecoder: %s\n", debugString);
        g_free (debugString);
      }

      pThis->sendMessage("error", error->message);
      g_error_free (error);
      break;

    default:
      break;
  }

  return true;
}

void Pipeline::sendMessage(const string &type, const string &msg)
{
  if(m_messageCallback)
    m_messageCallback(type, msg);
}
