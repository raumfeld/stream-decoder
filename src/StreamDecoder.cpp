#include <glib.h>
#include <atomic>
#include <gst/gst.h>
#include "StreamDecoder.h"
#include "WatchDog.h"
#include "Pipelines.h"
#include "Pipeline.h"
#include "Trace.h"

static GMainLoop *s_theMainLoop = NULL;
static std::atomic<bool> s_bQuit(false);

static void quit (int sig)
{
  if (!s_bQuit.exchange(true))
    g_main_loop_quit (s_theMainLoop);
}

static void sigPipe (int sig)
{
  Tracer::alarm( "sigPipe:", sig);
}

int main (int numArgs, char *argv[])
{
//  Tracer::minLevel = Tracer::TRACELEVEL_OVERDOSE;

  signal (SIGINT, quit);
  signal (SIGQUIT, quit);
  signal (SIGTERM, quit);
  signal (SIGPIPE, sigPipe);

  Tracer::info( "StreamDecoder is configured to output %ld bit audio data", sizeof(tSample) * 8 );

  gst_init (&numArgs, &argv);

  s_theMainLoop = g_main_loop_new (NULL, TRUE);

  while (!s_bQuit)
  {
    WatchDog watchdog;
    StreamDecoderDBusService *service = stream_decoder_dbus_service_new ();
    Pipelines pipeline (service);

#ifdef DESKTOP_BUILD
    bool doStats = true;
    std::thread pipelineStats([&]{
      while( doStats )
      {
        std::this_thread::sleep_for( std::chrono::seconds(10));
        pipeline.iteratePipelines([](uint64_t stream_id, std::shared_ptr<Pipeline> &pipeline){
          Tracer::warning( "stream:", stream_id, "stats:", pipeline->getStats() );
          pipeline->resetStats();
        });
      }
    });
#endif // DESKTOP_BUILD

    g_main_loop_run (s_theMainLoop);
    g_object_unref(service);

#ifdef DESKTOP_BUILD
    doStats = false;
    pipelineStats.join();
#endif // DESKTOP_BUILD
  }

  g_main_loop_unref (s_theMainLoop);

  return 0;
}
