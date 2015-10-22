#pragma once

#include <string>
#include <sstream>
#include <set>
#include <map>

#define ALARM_ASSERT( cond, format, ...) if (!(cond)) Tools::Tracer::alarm (" #cond :", __VA_ARGS__)

namespace Tools
{
  static inline void buildString_unpackStrings (std::stringstream &str, const char *separator)
  {
  }

  template<typename tFirst, typename ... tParts>
  static inline void buildString_unpackStrings (std::stringstream &str, const char *separator, const tFirst &f, const tParts&... parts)
  {
    str << f;

    if(separator)
      str << separator;

    buildString_unpackStrings (str, separator, parts...);
  }

  /**
   * Tracer is a class full of static method for logging, warning, alarming...
   */
  class Tracer
  {
    static bool s_printTimestamps;

  public:

    enum TraceLevel
    {
      TRACELEVEL_NONE,
      TRACELEVEL_OVERDOSE,
      TRACELEVEL_INFO,
      TRACELEVEL_WARNING,
      TRACELEVEL_ALARM,
      TRACELEVEL_ERROR
    };

    template<typename ...tArgs> static void overdose(const tArgs&... args)
    {
      trace(TRACELEVEL_OVERDOSE, args...);
    }

    template<typename ...tArgs> static void info(const tArgs&... args)
    {
      trace(TRACELEVEL_INFO, args...);
    }

    template<typename ...tArgs> static void warning(const tArgs&... args)
    {
      trace(TRACELEVEL_WARNING, args...);
    }

    template<typename ...tArgs> static void alarm(const tArgs&... args)
    {
      trace(TRACELEVEL_ALARM, args...);
    }

    template<typename ...tArgs> static void error(const tArgs&... args)
    {
      trace(TRACELEVEL_ERROR, args...);
    }

    template<typename ...tArgs> static void trace(TraceLevel level, const tArgs&... args)
    {
      if (level >= Tracer::minLevel)
      {
        std::stringstream str;
        buildString_unpackStrings( str, " ", args...);
        printToConsole(level, str.str());
      }
    }

    static void printTimestamps();

    template<typename tCondition, typename... tArgs>
    static void assert(const tCondition &cond, const char *format, const tArgs&... args)
    {
      if(!cond)
      {
        std::string msg = formatString(format, args...);
        output( TRACELEVEL_ALARM, "%s", msg.c_str());
      }
    }

    static TraceLevel minLevel;

    static void output( TraceLevel level, const char *msg, ... );

  private:
    static void printToConsole( TraceLevel level, const std::string &msg );
  };

}

using namespace Tools;
