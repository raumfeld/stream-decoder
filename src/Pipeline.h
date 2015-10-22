#pragma once

#include <glib.h>
#include <gio/gio.h>
#include <glib-object.h>
#include <gst/gst.h>
#include "stream-decoder-dbus-service.h"
#include <list>
#include <functional>
#include <memory>
#include "AudioConverter.h"
#include "Resampler.h"

using namespace std;


class Pipeline
{
  public:
    Pipeline (uint64_t stream_id, const gchar* uri, GVariant *allowed_samplerates);
    virtual ~Pipeline ();

    typedef function<void (const std::string &type, const std::string &msg)> tMessageCallback;
    gint32 init ();
    void setMessageCallback (tMessageCallback cb);

    unsigned int getStats() {
      return m_stats;
    }
    void resetStats();

  private:
    void setupGStreamer ();
    void sendMessage(const string &type, const string &msg);

    static gint32 onDecode (gpointer instance, const gchar* uri, GVariant *allowed_samplerates, Pipeline *pThis);
    static gpointer getSupportedProtocols (gpointer instance, Pipeline *pThis);
    static void onStop (gpointer instance, Pipeline *pThis);
    static void onDecodeDone (gpointer instance, Pipeline *pThis);
    static void onPadAdded (GstElement *element, GstPad *pad, Pipeline *pThis);
    static void onAudioDataDecoded (GstElement *fakesink, GstBuffer *buffer, GstPad *pad, Pipeline *pThis);
    static gboolean onBusEvent (GstBus *bus, GstMessage *message, Pipeline *pThis);

    static gint32 getID();

    void handOffData (GstCaps *caps, GstBuffer *buffer);
    void setupAudioProcessors (GstCaps* caps);
    void setupAudioConverter (GstCaps* caps);
    void setupResampler (GstCaps* caps);
    void processAndSendAudioData (GstBuffer* buffer);

    void setAllowedSamplerates (GVariant *allowed_samplerates);
    guint32 chooseSamplerate (guint32 sourceRate) const;

private:
    const uint64_t m_id = 0;
    std::string m_uri;
    std::list<guint32> m_allowedSampleRates;

    GstElement *m_pipeline;
    guint m_pipelineWatch;

    std::shared_ptr<AudioConverter> m_audioConverter;
    std::shared_ptr<Resampler> m_resampler;

    GCancellable *m_cancellable;
    GOutputStream *m_audioPipe;
    int m_renderersPipe;
    tMessageCallback m_messageCallback;

    bool m_close;

    unsigned int m_stats = 0;
};

