## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-I$(top_srcdir)/src	\
	$(STREAM_DECODER_CFLAGS)

TESTS = test-testables

noinst_PROGRAMS = $(TESTS) test-decoder


test_decoder_SOURCES = TestDecoder.cpp
test_decoder_LDADD = \
	$(top_builddir)/src/AudioConverter.o	\
	$(top_builddir)/src/Resampler.o		\
	$(STREAM_DECODER_LIBS)

test_testables_SOURCES = TestTestables.cpp
test_testables_LDADD = 	\
	$(top_builddir)/src/AudioConverter.o	\
	$(STREAM_DECODER_LIBS)
