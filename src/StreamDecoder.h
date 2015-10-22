#pragma once

#include <glib.h>

using namespace std;

#ifdef USE32BIT
typedef gint32 tSample;
typedef gint64 dSample;
#define BITDEPTH 24
#else
typedef gint16 tSample;
typedef gint32 dSample;
#define BITDEPTH 16
#endif
