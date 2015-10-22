#pragma once

#include <glib.h>
#include <list>
#include <string>

using namespace std;

class SupportedProtocols
{
  public:
    static const SupportedProtocols& get();

    gchar **asArrayOfStrings () const;

  private:
    SupportedProtocols ();
    virtual ~SupportedProtocols ();

    void init();

    list<string> m_supportedProtocols;
};

