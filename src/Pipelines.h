#pragma once

#include "StreamDecoder.h"
#include "stream-decoder-dbus-service.h"
#include <memory>
#include <map>
#include <cstdint>

class Pipeline;

class Pipelines
{
  public:
    Pipelines (StreamDecoderDBusService *service);
    virtual ~Pipelines ();

    void iteratePipelines (const std::function<void(uint64_t, std::shared_ptr<Pipeline>&)>& func );

  private:
    void connect();

    static bool onDecode (Pipelines *pThis, uint64_t stream_id, const gchar* uri, GVariant *allowed_samplerates, gint32* pipe);
    static void onStop (Pipelines *pThis, uint64_t stream_id);
    static gchar ** getSupportedProtocols (Pipelines *pThis);
    static void reset (Pipelines *pThis);

    typedef std::shared_ptr<Pipeline> tPipeline;
    std::map<uint64_t, tPipeline> m_pipelines;
    StreamDecoderDBusService *m_service;
};

