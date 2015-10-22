#include <glib.h>
#include <gst/gst.h>

#include "Resampler.h"
#include "StreamDecoder.h"

const int FP_PRE = (5);
const int FP_POST = (32 - FP_PRE);
const int numChannels = 2;

template<typename tIn>
  inline tIn interpolate (tIn prev, tIn next, guint32 frac);

template<>
  inline gint16 interpolate<gint16> (gint16 prev, gint16 next, guint32 frac)
  {
    gint32 prevBig = prev;
    gint32 nextBig = next;

    prevBig <<= 16;
    nextBig <<= 16;

    frac >>= FP_POST - 16;

    gint32 one = 1;
    one <<= 16;

    return (next * frac + prev * (one - frac)) >> 16;
  }

template<>
  inline gint32 interpolate<gint32> (gint32 prev, gint32 next, guint32 frac)
  {
    gint64 prevBig = prev;
    gint64 nextBig = next;

    prevBig <<= 32;
    nextBig <<= 32;

    gint64 fracBig = frac;
    fracBig <<= 32 - FP_POST;

    gint64 one = 1;
    one <<= 32;

    return (next * fracBig + prev * (one - fracBig)) >> 32;
  }

Resampler::Resampler (int srcSR, int tgtSR) :
    m_sourceSR (srcSR),
    m_targetSR (tgtSR),
    m_scratchBuffer (std::max (srcSR, tgtSR)),
    m_srcPositionFracFP (0),
    m_srcIncrementFP (0),
    m_srcPositionInt (0)
{
  m_srcIncrementFP = (1 << FP_POST) * 1 / getRatio ();
}

Resampler::~Resampler ()
{
}

int Resampler::getSourceSR () const
{
  return m_sourceSR;
}

double Resampler::getRatio () const
{
  double inRate = m_sourceSR;
  double outRate = m_targetSR;
  return outRate / inRate;
}

GstBuffer *Resampler::eat (GstBuffer *in)
{
  if (m_sourceSR == m_targetSR)
  {
    gst_buffer_ref (in);
    return in;
  }

  writeToScratch (in);
  return produceResampledBuffer ();
}

void Resampler::writeToScratch (GstBuffer* in)
{
  GstMapInfo inInfo;
  if (gst_buffer_map (in, &inInfo, GST_MAP_READ))
  {
    tSample* srcSamples = (tSample*) (inInfo.data);
    const int numInSampleFrames = gst_buffer_get_size (in) / sizeof(tSample) / numChannels;
    m_scratchBuffer.write ((const Frame*) (srcSamples), numInSampleFrames);
    gst_buffer_unmap (in, &inInfo);
  }
}

gint64 Resampler::getNumFramesAvailable () const
{
  gint64 numFramesAvailable = m_scratchBuffer.getWriteHead () - m_srcPositionInt;
  numFramesAvailable--; // need one more to interpolate
  return numFramesAvailable;
}

GstBuffer* Resampler::produceResampledBuffer ()
{
  gint64 numFramesAvailable = getNumFramesAvailable ();
  size_t numOutFrames = numFramesAvailable * m_targetSR / m_sourceSR;

  if (numFramesAvailable <= 0)
    numOutFrames = 0;

  GstBuffer* out = createOutBuffer (numOutFrames);
  GstMapInfo outInfo;
  if (gst_buffer_map (out, &outInfo, GST_MAP_WRITE))
  {
    doResampling (outInfo, numOutFrames);
    gst_buffer_unmap (out, &outInfo);
  }
  return out;
}

GstBuffer* Resampler::createOutBuffer (size_t numOutFrames) const
{
  const int neededSize = numChannels * sizeof(tSample) * numOutFrames;
  return gst_buffer_new_allocate (NULL, neededSize, NULL);
}

void Resampler::doResampling (GstMapInfo outInfo, size_t numOutFrames)
{
  Frame* outData = (Frame*) (outInfo.data);
  for (size_t i = 0; i < numOutFrames; i++)
  {
    calcInterpolatedFrame (outData[i]);

    m_srcPositionFracFP = m_srcPositionFracFP + m_srcIncrementFP;
    size_t preComma = m_srcPositionFracFP >> FP_POST;

    if (preComma)
    {
      m_srcPositionInt += preComma;
      m_srcPositionFracFP = m_srcPositionFracFP & ((1 << FP_POST) - 1);
    }
  }
}

inline void Resampler::calcInterpolatedFrame (Frame &target) const
{
  guint32 prevFramePos = m_srcPositionInt;
  guint32 frac = m_srcPositionFracFP & ((1 << FP_POST) - 1);

  if (frac == 0)
  {
    target = m_scratchBuffer.peek (prevFramePos);
  }
  else
  {
    guint32 nextFramePos = prevFramePos + 1;

    const Frame &prev = m_scratchBuffer.peek (prevFramePos);
    const Frame &next = m_scratchBuffer.peek (nextFramePos);

    for(size_t i = 0; i < numChannels; i++)
      target.samples[i] = interpolate (prev.samples[i], next.samples[i], frac);
  }
}
