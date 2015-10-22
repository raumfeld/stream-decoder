## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = $(STREAM_DECODER_CFLAGS)

BUILT_SOURCES = \
	stream-decoder-gdbus.h	\
	stream-decoder-gdbus.c 

bin_PROGRAMS = stream-decoder

stream_decoder_SOURCES = \
	$(BUILT_SOURCES) \
	AudioConverter.h \
	AudioConverter.cpp \
	Pipeline.h \
	Pipeline.cpp \
	Pipelines.h \
	Pipelines.cpp \
  Trace.h \
	Trace.cpp \
	Resampler.h \
	Resampler.cpp \
	RingBuffer.h \
	StreamDecoder.h \
	StreamDecoder.cpp \
	stream-decoder-dbus-service.h \
	stream-decoder-dbus-service.cpp \
	SupportedProtocols.h \
	SupportedProtocols.cpp \
	WatchDog.h \
	WatchDog.cpp

EXTRA_DIST = com.raumfeld.StreamDecoder.xml

stream-decoder-gdbus.c: stream-decoder-gdbus.h com.raumfeld.StreamDecoder.xml
stream-decoder-gdbus.h: com.raumfeld.StreamDecoder.xml
	$(GDBUS_CODEGEN) --generate-c-code stream-decoder-gdbus --interface-prefix com.raumfeld $<


stream_decoder_LDADD = $(STREAM_DECODER_LIBS)
