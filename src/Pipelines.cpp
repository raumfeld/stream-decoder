#include "Pipelines.h"
#include "Pipeline.h"
#include "SupportedProtocols.h"
#include <stdio.h>
#include "Trace.h"

Pipelines::Pipelines (StreamDecoderDBusService *service) :
    m_service (service)
{
  g_object_ref (m_service);
  connect ();
}

Pipelines::~Pipelines ()
{
  g_object_unref (m_service);
}

void Pipelines::iteratePipelines(const std::function<void(uint64_t, std::shared_ptr<Pipeline>&)>& func)
{
  for( auto& pipeline: m_pipelines )
  {
    func( pipeline.first, pipeline.second );
  }
}

void Pipelines::connect ()
{
  g_signal_connect_swapped (m_service, "decode", G_CALLBACK (&Pipelines::onDecode), this);
  g_signal_connect_swapped (m_service, "stop", G_CALLBACK (&Pipelines::onStop), this);
  g_signal_connect_swapped (m_service, "get-supported-protocols", G_CALLBACK (&Pipelines::getSupportedProtocols), this);
  g_signal_connect_swapped (m_service, "reset", G_CALLBACK (&Pipelines::reset), this);
}

bool Pipelines::onDecode (Pipelines *pThis, guint64 stream_id, const gchar* uri, GVariant *allowed_samplerates, gint32 *pipe)
{
  tPipeline pipeline ( std::make_shared<Pipeline> (stream_id, uri, allowed_samplerates));
  gint32 pipe_fd = pipeline->init ();
  if( -1 == pipe_fd )
  {
    Tracer::alarm("Pipeline init failed");
    return false;
  }

  if( pipe ){
    *pipe = pipe_fd;
  }

  Tracer::overdose( "Pipelines::onDecode, stream:", stream_id );

  pipeline->setMessageCallback([=](const std::string &type, const std::string &msg)
  {
    g_print ("->Streamdecoder: emit error type=%s, str=%s\n", type.c_str(), msg.c_str());
    stream_decoder_emit_message_signal (pThis->m_service, stream_id, type.c_str(), msg.c_str());
  });

  pThis->m_pipelines[stream_id] = pipeline;

  return true;
}

void Pipelines::onStop (Pipelines *pThis, uint64_t stream_id)
{
  Tracer::info( "Pipelines::onStop, stream_id:", stream_id );

  auto it = pThis->m_pipelines.find (stream_id);
  if (it == pThis->m_pipelines.end ())
  {
    Tracer::alarm( "trying to stop unknown stream,", stream_id);
    return;
  }

  auto pipeline = it->second;
  pThis->m_pipelines.erase (it);

  Tracer::overdose( "pipeline.use_count():", pipeline.use_count());
}

gchar ** Pipelines::getSupportedProtocols (Pipelines *pThis)
{
  return SupportedProtocols::get ().asArrayOfStrings ();
}

void Pipelines::reset (Pipelines* pThis)
{
  Tracer::overdose( __PRETTY_FUNCTION__ );
  pThis->m_pipelines.clear();
}
