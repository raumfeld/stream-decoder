#pragma once

#include "StreamDecoder.h"
#include "gst/gst.h"

class AudioConverter
{
  public:
    AudioConverter (GstCaps *srcCaps);
    GstBuffer *eat (GstBuffer *in);

    static void registerTests ();

  private:
    template<typename T> void doLoop (tSample* __restrict__ out, const T* __restrict__ src, size_t numFrames);
    template<typename T, typename tFrameReader> void doLoop (tSample* __restrict__ out, const T* __restrict__ src, size_t numFrames);

    gboolean srcIsBigEndian;
    gboolean srcIsFloat;
    gboolean srcIsSigned;
    int srcWidth;
    int srcDepth;
    int srcChannels;
};
