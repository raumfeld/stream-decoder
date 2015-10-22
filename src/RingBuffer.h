#pragma once

#include "StreamDecoder.h"
#include <vector>
#include <string.h>

using namespace std;

template<typename tElement>
class RingBuffer
{
  public:
    RingBuffer (guint32 numElements) :
        m_writeHead (0)
    {
      gint s = 1 << (g_bit_nth_msf(numElements, 32) + 1);
      m_buffer.resize (s);
    }

    virtual ~RingBuffer ()
    {
    }

    const tElement &peek (guint64 readHead) const
    {
      size_t readHeadIdx = readHead & (m_buffer.size () - 1);
      return m_buffer[readHeadIdx];
    }

    void write (const tElement *buffer, size_t numElements)
    {
      if (numElements > 0)
      {
        size_t writeHeadIdx = m_writeHead & (m_buffer.size () - 1);

        if (writeHeadIdx + numElements <= m_buffer.size ())
        {
          for (size_t i = 0; i < numElements; i++)
          {
            m_buffer[writeHeadIdx + i] = buffer[i];
          }
          m_writeHead += numElements;
        }
        else
        {
          size_t numFittingElements = m_buffer.size () - writeHeadIdx;
          write (buffer, numFittingElements);
          write (buffer + numFittingElements, numElements - numFittingElements);
        }
      }
    }

    guint64 getWriteHead () const
    {
      return m_writeHead;
    }

  private:
    RingBuffer (const RingBuffer &other);
    RingBuffer (RingBuffer &other);
    RingBuffer &operator= (const RingBuffer &other);
    RingBuffer &operator= (RingBuffer &other);

    vector<tElement> m_buffer;
    guint64 m_writeHead;
};


