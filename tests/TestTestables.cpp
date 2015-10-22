#include <glib.h>

#include "AudioConverter.h"

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  AudioConverter::registerTests ();

  return g_test_run ();
}
