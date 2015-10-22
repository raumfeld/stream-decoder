#include "WatchDog.h"

WatchDog::WatchDog ()
{
  m_close = false;
  m_counter = 0;
  m_source = g_timeout_add_seconds (1, (GSourceFunc) WatchDog::heartBeat, this);

  m_thread = thread([=]()
  {
    int lastValue = m_counter;

    while(!m_close)
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_condition.wait_for(lock, chrono::seconds(5));

      if(!m_close)
      {
        if(m_counter == lastValue)
        {
#if _DEBUG
          g_print ("StreamDecoder is possibly dead locked");
#else
          g_error ("StreamDecoder is possibly dead locked");
#endif
        }

        lastValue = m_counter;
      }
    }
  });
}

WatchDog::~WatchDog ()
{
  g_source_remove(m_source);

  m_close = true;
  m_condition.notify_all();

  if(m_thread.joinable())
    m_thread.join();
}


gboolean WatchDog::heartBeat (WatchDog *pThis)
{
  pThis->m_counter++;
  return TRUE;
}


