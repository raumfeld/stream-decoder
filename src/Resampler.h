/* StreamDecoder
 * Copyright (C) 2009-2013 Raumfeld GmbH
 *
 * Author: Henry Hoegelow <h.hoegelo@raumfeld.com>
 *
 * This code is based on the resample-project (version -1.7) which has the
 * following copyright note:
 *
 * Copyright 1994-2002 by Julius O. Smith III <jos@ccrma.stanford.edu>,
 * all rights reserved.  Permission to use and copy is granted subject to
 * the terms of the "GNU Lesser General Public License" (LGPL) as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or any later version.  In addition, we request that a copy of
 * any modified files be sent by email to jos@ccrma.stanford.edu so that
 * we may incorporate them into the CCRMA version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#ifndef RESAMPLER_H_
#define RESAMPLER_H_

#include "StreamDecoder.h"
#include "RingBuffer.h"

class Resampler
{
  public:
    Resampler (int srcSR, int tgtSR);
    ~Resampler();

    GstBuffer *eat (GstBuffer *in);
    int getSourceSR () const;

  private:
    struct Frame
    {
      tSample samples[2];
    };

    double getRatio () const;
    void calcInterpolatedFrame (Frame &target) const;

    void writeToScratch (GstBuffer* in);
    GstBuffer* createOutBuffer (size_t numOutFrames) const;
    void doResampling (GstMapInfo outInfo, size_t numOutFrames);
    gint64 getNumFramesAvailable () const;
    GstBuffer* produceResampledBuffer ();

    int m_sourceSR;
    int m_targetSR;

    RingBuffer<Frame> m_scratchBuffer;

    guint32 m_srcPositionFracFP;
    guint32 m_srcIncrementFP;
    guint64 m_srcPositionInt;
};

#endif /* RESAMPLER_H_ */
