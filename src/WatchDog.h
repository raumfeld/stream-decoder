#pragma once

#include <glib.h>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>

using namespace std;

class WatchDog
{
  public:
    WatchDog ();
    virtual ~WatchDog ();

  private:
    static gboolean heartBeat (WatchDog *pThis);

    thread m_thread;
    condition_variable m_condition;
    mutex m_mutex;
    atomic<bool> m_close;
    atomic<int> m_counter;
    int m_source;
};

