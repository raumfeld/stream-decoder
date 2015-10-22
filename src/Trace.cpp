#include "Trace.h"
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

namespace Tools
{

  Tracer::TraceLevel Tracer::minLevel = TRACELEVEL_WARNING;
  bool Tracer::s_printTimestamps = false;

  void Tracer::printTimestamps ()
  {
    s_printTimestamps = true;
  }

  void Tracer::printToConsole(TraceLevel level, const std::string& msg)
  {
    if (level >= minLevel)
    {
      {
        if (s_printTimestamps)
        {
          static int numTraces = 1;
          static time_t startTime = time (NULL);
          time_t t = time (NULL) - startTime;

          int hrs = t / 3600;
          int mins = (t - hrs * 3600) / 60;
          int secs = t - hrs * 3600 - mins * 60;

          fprintf ( stderr, "#%d @ %02d:%02d:%02d -> ", numTraces++, hrs, mins, secs);
        }

        if (level == TRACELEVEL_ERROR)
        {
          fprintf ( stderr, "%s\n", msg.c_str());
          assert(NULL);
        }
        else if (level == TRACELEVEL_ALARM)
        {
          fprintf ( stderr, "!!! ALARM : %s !!!\n", msg.c_str());
        }
        else
        {
          fprintf ( stderr, "%s\n", msg.c_str());
        }

        fflush( stderr );
      }
    }
  }

  void Tracer::output (TraceLevel level, const char *msg, ...)
  {
    if (level < minLevel){
      return;
    }

    {
      va_list ap;
      va_start (ap, msg);
      int len = vsnprintf (NULL, 0, msg, ap);
      va_end (ap);

      char out[len+1];

      va_start (ap, msg);
      vsnprintf (out, len+1, msg, ap);
      va_end (ap);

      printToConsole(level, out);
    }
  }
}
