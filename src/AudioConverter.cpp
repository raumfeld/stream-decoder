#include <stdlib.h>
#include <string.h>
#include <limits>
#include <type_traits>
#include <glib.h>
#include <gst/gst.h>
#include "AudioConverter.h"
#include "StreamDecoder.h"

#ifdef USE32BIT
#define MAXVALUE ((gint32) 0x007fffff)
#define MINVALUE ((gint32) 0xff800000)
#else
#define MAXVALUE (G_MAXINT16)
#define MINVALUE (G_MININT16)
#endif


namespace
{
  struct tUInt24
  {
    guint8 a;
    guint8 b;
    guint8 c;
  };

  struct tInt24
  {
    guint8 a;
    guint8 b;
    guint8 c;
  };

  template<typename T, int channels = 0>
  class SampleReader
  {
    public:
      inline static const T *readSampleFrame (const T *p, T &left, T &right, int numChannels)
      {
        left = *(p++);
        right = *(p++);
        return p + sizeof (T) * (numChannels - 2);
      }
  };

  template<typename T>
  class SampleReader<T, 1>
  {
    public:
      inline static const T *readSampleFrame (const T *p, T &left, T &right, int numChannels)
      {
        left = right = *(p++);
        return p;
      }
  };

  template<typename T>
  class SampleReader<T, 2>
  {
    public:
      inline static const T *readSampleFrame (const T *p, T &left, T &right, int numChannels)
      {
        left = *(p++);
        right = *(p++);
        return p;
      }
  };

  inline guint8 toLittleEndian (guint8 in)
  {
    return in;
  }

  inline gint8 toLittleEndian (gint8 in)
  {
    return in;
  }

  inline guint16 toLittleEndian (guint16 in)
  {
    return GUINT16_SWAP_LE_BE (in);
  }

  inline gint16 toLittleEndian (gint16 in)
  {
    return GUINT16_SWAP_LE_BE (in);
  }

  inline tUInt24 toLittleEndian (tUInt24 in)
  {
    guint8 *u = (guint8*) &in;
    guint8 tmp = u[0];
    u[0] = u[2];
    u[2] = tmp;
    return in;
  }

  inline tInt24 toLittleEndian (tInt24 in)
  {
    guint8 *u = (guint8*) &in;
    guint8 tmp = u[0];
    u[0] = u[2];
    u[2] = tmp;
    return in;
  }

  inline guint32 toLittleEndian (guint32 in)
  {
    return GUINT32_SWAP_LE_BE (in);
  }

  inline gint32 toLittleEndian (gint32 in)
  {
    return GUINT32_SWAP_LE_BE (in);
  }

  inline gfloat toLittleEndian (gfloat in)
  {
    return in;
  }

  template<typename tIn>
  typename std::make_signed<tIn>::type makeSigned (tIn in)
  {
    if (std::is_signed<tIn>::value)
      return in;

    return in + std::numeric_limits<typename std::make_signed<tIn>::type>::min ();
  }

  template<typename tFrom>
  struct LeftShifter
  {
      static tSample shift (tFrom a)
      {
        return ((tSample) a) << (BITDEPTH - 8 * sizeof (tFrom));
      }
  };

  template<typename tFrom>
  struct RightShifter
  {
      static tSample shift (tFrom a)
      {
        return a >> (8 * sizeof (tFrom) - BITDEPTH);
      }
  };


  template<typename tFrom>
  struct Shifter
  {
      static tSample shift (tFrom in)
      {
        typedef typename std::conditional<(8 * sizeof (tFrom) > BITDEPTH), RightShifter<tFrom>, LeftShifter<tFrom> >::type tShifter;
        return tShifter::shift (in);
      }
  };

  template<typename tIn>
  tSample shiftToSample (tIn in)
  {
    return Shifter<tIn>::shift (in);
  }

  template<typename tIn>
  inline tSample makeSample (tIn v)
  {
    return shiftToSample (makeSigned (v));
  }

  inline tSample makeSample (tUInt24 v)
  {
    guint32 p = v.c << 24 | v.b << 16 | v.a << 8;
    return makeSample (p);
  }

  inline tSample makeSample (tInt24 v)
  {
    gint32 p = v.c << 24 | v.b << 16 | v.a << 8;
    return makeSample (p);
  }

  inline tSample makeSample (gfloat v)
  {
    gint32 i = v * (G_MAXINT32 - (1 << (32 - 8 * sizeof (tSample)) / 2));
    return makeSample (i);
  }

  template<typename T>
  inline T shiftLeft (T v, int s)
  {
    return v << s;
  }

  inline float shiftLeft (float f, int s)
  {
    return f;
  }

  inline tInt24 shiftLeft (tInt24 f, int s)
  {
    return f;
  }

  inline tUInt24 shiftLeft (tUInt24 f, int s)
  {
    return f;
  }
}

AudioConverter::AudioConverter (GstCaps *srcCaps)
{
  srcIsBigEndian = FALSE;
  srcIsFloat = FALSE;
  srcWidth = 16;
  srcDepth = 16;
  srcChannels = 2;
  srcIsSigned = TRUE;

  GstStructure* s = gst_caps_get_structure (srcCaps, 0);

  char *capsAsString = gst_caps_to_string (srcCaps);
  g_print ("AudioConverter: caps to convert from are %s\n", capsAsString);
  g_free (capsAsString);

  if (s)
  {
    gst_structure_get_int (s, "channels", &srcChannels);

    if (srcChannels > 2)
      srcChannels = 2;

    const gchar *format = gst_structure_get_string (s, "format");
    if (format)
    {
      switch (format[0])
      {
      case 'S':
        srcIsSigned = TRUE;
        break;

      case 'U':
        srcIsSigned = FALSE;
        break;

      case 'F':
        srcIsFloat = TRUE;
        break;

      default:
        g_warn_if_reached ();

      case '\0':
        return;
      }

      srcDepth = strtoul (format + 1, (gchar **) &format, 10);

      if (format[0] == '_')
      {
        srcWidth = strtoul (format + 1, (gchar **) &format, 10);
      }
      else
      {
        srcWidth = srcDepth;
      }

      srcIsBigEndian = (strcmp (format, "BE") == 0);
    }
  }

  g_print ("AudioConverter: %s %s %d %d\n",
           srcIsBigEndian ? "BE" : "LE",
           srcIsFloat ? "float" : (srcIsSigned ? "signed" : "unsigned"),
           srcDepth, srcWidth);
}

GstBuffer *AudioConverter::eat (GstBuffer *inBuffer)
{
  const int outSampleWidth = sizeof (tSample) * 8;
  const int outSampleDepth = BITDEPTH;

  if (!srcIsBigEndian && !srcIsFloat &&
      srcIsSigned && srcWidth == outSampleWidth &&
      srcDepth == outSampleDepth && srcChannels == 2)
  {
    gst_buffer_ref (inBuffer);
    return inBuffer;
  }

  const int bytesPerSample = srcWidth / 8;

  if (!bytesPerSample || !srcChannels)
  {
    return gst_buffer_new ();
  }

  const int numInSampleFrames = gst_buffer_get_size (inBuffer) / bytesPerSample / srcChannels;
  const int neededSize = 2 * sizeof (tSample) * numInSampleFrames;

  GstBuffer *outBuffer = gst_buffer_new_allocate (NULL, neededSize, NULL);

  GstMapInfo in;
  GstMapInfo out;

  if (gst_buffer_map (inBuffer, &in, GST_MAP_READ) &&
      gst_buffer_map (outBuffer, &out, GST_MAP_WRITE))
  {
    tSample *dest = (tSample*) out.data;

    if (srcIsFloat)
    {
      doLoop (dest, (const gfloat*) in.data, numInSampleFrames);
    }
    else if (srcIsSigned)
    {
      switch (srcWidth)
      {
      case 8:
        doLoop (dest, (const gint8*) in.data, numInSampleFrames);
        break;

      case 16:
        doLoop (dest, (const gint16*) in.data, numInSampleFrames);
        break;

      case 24:
        doLoop (dest, (const tInt24*) in.data, numInSampleFrames);
        break;

      case 32:
        doLoop (dest, (const gint32*) in.data, numInSampleFrames);
        break;
      }
    }
    else // !srcIsSigned)
    {
      switch (srcWidth)
      {
      case 8:
        doLoop (dest, (const guint8*) in.data, numInSampleFrames);
        break;

      case 16:
        doLoop (dest, (const guint16*) in.data, numInSampleFrames);
        break;

      case 24:
        doLoop (dest, (const tUInt24*) in.data, numInSampleFrames);
        break;

      case 32:
        doLoop (dest, (const guint32*) in.data, numInSampleFrames);
        break;
      }
    }

    gst_buffer_unmap (outBuffer, &out);
    gst_buffer_unmap (inBuffer, &in);
  }

  return outBuffer;
}


template<typename T, typename tFrameReader>
void AudioConverter::doLoop (tSample* __restrict__ out, const T* __restrict__ src, size_t numFrames)
{
  T left;
  T right;

  const bool convertEndianess = srcIsBigEndian;
  const int paddingShiftAmount = srcIsFloat ? 0 : srcWidth - srcDepth;

  while (numFrames--)
  {
    src = tFrameReader::readSampleFrame (src, left, right, srcChannels);

    if (convertEndianess)
    {
      left = toLittleEndian (left);
      right = toLittleEndian (right);
    }

    if (paddingShiftAmount)
    {
      left = shiftLeft (left, paddingShiftAmount);
      right = shiftLeft (right, paddingShiftAmount);
    }

    *(out++) = makeSample (left);
    *(out++) = makeSample (right);
  }
}

template<typename T>
void AudioConverter::doLoop (tSample* __restrict__ out, const T* __restrict__ src, size_t numFrames)
{
  switch(srcChannels)
  {
    case 1:
      return doLoop<T, SampleReader<T, 1>>(out,  src, numFrames);

    case 2:
      return doLoop<T, SampleReader<T, 2>>(out,  src, numFrames);

    default:
      return doLoop<T, SampleReader<T>>(out,  src, numFrames);
  }
}


static void test_makeSigned ()
{
  g_assert_cmpint (makeSigned ((gint8) G_MAXINT8), ==, G_MAXINT8);
  g_assert_cmpint (makeSigned ((gint8) G_MININT8), ==, G_MININT8);
  g_assert_cmpint (makeSigned ((gint8) 0), ==, 0);
  g_assert_cmpint (makeSigned ((gint8) 5), ==, 5);

  g_assert_cmpint (makeSigned ((guint8) 0), ==, G_MININT8);
  g_assert_cmpint (makeSigned ((guint8) G_MAXUINT8), ==, G_MAXINT8);

  g_assert_cmpint (makeSigned ((gint16) G_MAXINT16), ==, G_MAXINT16);
  g_assert_cmpint (makeSigned ((gint16) G_MININT16), ==, G_MININT16);
  g_assert_cmpint (makeSigned ((gint16) 0), ==, 0);
  g_assert_cmpint (makeSigned ((gint16) 5), ==, 5);

  g_assert_cmpint (makeSigned ((guint16) 0), ==, G_MININT16);
  g_assert_cmpint (makeSigned ((guint16) G_MAXUINT16), ==, G_MAXINT16);

  g_assert_cmpint (makeSigned ((gint32) 123456), ==, 123456);
  g_assert_cmpint (makeSigned ((guint32) 0), ==, G_MININT32);
}

static void test_makeSample ()
{
#ifdef USE32BIT

  g_assert_cmpint (makeSample (1.0f), ==, MAXVALUE);
  g_assert_cmpint (makeSample (-1.0f), ==, MINVALUE);
  g_assert_cmpint (makeSample (0.0f), ==, 0);

  g_assert_cmpint (makeSample ((gint16) G_MAXINT16), ==, (gint32) (MAXVALUE & 0xFFFFFF00));
  g_assert_cmpint (makeSample ((gint16) G_MININT16), ==, (gint32) (MINVALUE));
  g_assert_cmpint (makeSample ((gint16) 0), ==, (gint32) 0);

  g_assert_cmpint (makeSample ((guint16) G_MAXUINT16), ==, (gint32) (MAXVALUE & 0xFFFFFF00));
  g_assert_cmpint (makeSample ((guint16) 0), ==, (gint32) MINVALUE);

  g_assert_cmpint (makeSample ((tUInt24) { 0xFF, 0xFF, 0xFF }), ==, (gint32) (MAXVALUE));
  g_assert_cmpint (makeSample ((tUInt24) { 0x0, 0x0, 0x0 }), ==, (gint32) (MINVALUE));
  g_assert_cmpint (abs (makeSample ((tUInt24) { 0xFF, 0xFF, 0x7F })), <=, 256);  // tolerated error

  g_assert_cmpint (makeSample ((tInt24) { 0xFF, 0xFF, 0x7F }), ==, (gint32) (MAXVALUE));
  g_assert_cmpint (makeSample ((tInt24) { 0x0, 0x0, 0x0 }), ==, 0);

#else

  g_assert_cmpint (makeSample (1.0f), ==, MAXVALUE);
  g_assert_cmpint (makeSample (-1.0f), ==, MINVALUE);
  g_assert_cmpint (makeSample (0.0f), ==, 0);

  g_assert_cmpint (makeSample ((gint16) G_MAXINT16), ==, (gint16) (MAXVALUE));
  g_assert_cmpint (makeSample ((gint16) G_MININT16), ==, (gint16) (MINVALUE));
  g_assert_cmpint (makeSample ((gint16) 0), ==, (gint16) 0);

  g_assert_cmpint (makeSample ((guint16) G_MAXUINT16), ==, (gint16) (MAXVALUE));
  g_assert_cmpint (makeSample ((guint16) 0), ==, (gint16) MINVALUE);

  g_assert_cmpint (makeSample ((tUInt24) { 0xFF, 0xFF, 0xFF }), ==, (gint16) (MAXVALUE));
  g_assert_cmpint (makeSample ((tUInt24) { 0x0, 0x0, 0x0 }), ==, (gint16) (MINVALUE));
  g_assert_cmpint (abs (makeSample ((tUInt24) { 0xFF, 0xFF, 0x7F })), <=, 1);  // tolerated error

  g_assert_cmpint (makeSample ((tInt24) { 0xFF, 0xFF, 0x7F }), ==, (gint16) (MAXVALUE));
  g_assert_cmpint (makeSample ((tInt24) { 0x0, 0x0, 0x0 }), ==, 0);

#endif
}

void AudioConverter::registerTests ()
{
  g_test_add_func ("/AudioConverter/makeSigned", test_makeSigned);
  g_test_add_func ("/AudioConverter/makeSample", test_makeSample);
}
