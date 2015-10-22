/*
 * STREAM_DECODER_DBusService:
 *
 * This class provides the DBus service.
 */

#include <gio/gio.h>
#include <gio/gunixfdlist.h>
#include <stdio.h>

#include "memory.h"
#include "stream-decoder-dbus-service.h"
#include "stream-decoder-gdbus.h"
#include "Trace.h"

enum
{
  SIGNAL_DECODE,
  SIGNAL_STOP,
  SIGNAL_GET_SUPPORTED_PROTOCOLS,
  SIGNAL_RESET,
  SIGNAL_LAST
};

struct _StreamDecoderDBusServiceClass
{
    StreamDecoderSkeletonClass parent_class;
};

struct _StreamDecoderDBusService
{
    StreamDecoderSkeleton parent_instance;

    guint owner_id;

    //on_decode
    static gboolean on_decode (StreamDecoder *object, GDBusMethodInvocation *invocation, GUnixFDList *fd_list, uint64_t stream_id, const gchar *arg_uri,
                               GVariant *arg_allowed_samplerates);

    static gboolean on_stop (StreamDecoder *object, GDBusMethodInvocation *invocation, uint64_t stream_id, gpointer user_data);

    static gboolean on_get_supported_protocols (StreamDecoder *object, GDBusMethodInvocation *invocation, gpointer user_data);

    static gboolean on_reset (StreamDecoder *object, GDBusMethodInvocation *invocation, gpointer user_data);

    static void on_connection_closed (GDBusConnection *connection, gboolean remote_peer_vanished, GError *error, gpointer user_data);
};

static guint stream_decoder_signals[SIGNAL_LAST] =
{0 };

gboolean _StreamDecoderDBusService::on_decode (StreamDecoder *object,
    GDBusMethodInvocation *invocation,
    GUnixFDList *fd_list,
    uint64_t stream_id,
    const gchar *uri,
    GVariant *allowed_samplerates)
{
  Tracer::overdose( __PRETTY_FUNCTION__, "stream_id:", stream_id );

  if( 0 == stream_id )
  {
    Tracer::alarm("StreamDecoderDBusService::on_decode, stream_id == 0");
    stream_decoder_emit_message_signal (STREAM_DECODER_DBUS_SERVICE(object), stream_id, "error", "Wrong stream_id parameter");
    g_dbus_method_invocation_return_dbus_error (invocation, "error", "Wrong stream_id parameter");
    return false;
  }

  gint32 pipe = 0;
  gboolean result = FALSE;
  g_signal_emit (object, stream_decoder_signals[SIGNAL_DECODE], 0, stream_id, uri, allowed_samplerates, &pipe, &result );
  if( FALSE == result )
  {
    Tracer::alarm("onDecode failed");
    stream_decoder_emit_message_signal (STREAM_DECODER_DBUS_SERVICE(object), stream_id, "error", "onDecode failed");
    g_dbus_method_invocation_return_dbus_error (invocation, "error", "onDecode failed");
    return false;
  }

  GError* error = NULL;
  GUnixFDList *local_fdlist = g_unix_fd_list_new ();
  g_unix_fd_list_append (local_fdlist, pipe, &error);
  g_assert_no_error (error);

  Tracer::warning("_StreamDecoderDBusService::on_decode, stream_id:", stream_id, "pipe:", pipe );

  stream_decoder_complete_decode (object, invocation, local_fdlist, g_variant_new_handle (0));

  g_object_unref (local_fdlist);

  return true;
}

gboolean _StreamDecoderDBusService::on_stop (StreamDecoder *object, GDBusMethodInvocation *invocation, uint64_t stream_id, gpointer user_data)
{
  Tracer::warning( __PRETTY_FUNCTION__, "stream_id:", stream_id );

  if( 0 == stream_id )
  {
    Tracer::alarm("StreamDecoderDBusService::on_decode, stream_id == 0");
    stream_decoder_emit_message_signal( STREAM_DECODER_DBUS_SERVICE(object), stream_id, "error", "Wrong stream id");
    g_dbus_method_invocation_return_dbus_error (invocation, "error", "Wrong stream_id parameter");
    return false;
  }

  g_signal_emit (object, stream_decoder_signals[SIGNAL_STOP], 0, stream_id);

  stream_decoder_complete_stop (object, invocation);

  return true;
}


gboolean _StreamDecoderDBusService::on_get_supported_protocols (StreamDecoder *object, GDBusMethodInvocation *invocation, gpointer user_data)
{
  Tracer::overdose( __PRETTY_FUNCTION__ );

  gchar** protocols = NULL;

  g_signal_emit (object, stream_decoder_signals[SIGNAL_GET_SUPPORTED_PROTOCOLS], 0, &protocols);

  stream_decoder_complete_get_supported_protocols (object, invocation, protocols);
  g_strfreev (protocols);

  return true;
}

gboolean _StreamDecoderDBusService::on_reset(StreamDecoder* object, GDBusMethodInvocation* invocation, gpointer user_data)
{
  Tracer::overdose( __PRETTY_FUNCTION__ );

  g_signal_emit (object, stream_decoder_signals[SIGNAL_RESET], 0 );

  stream_decoder_complete_reset (object, invocation );

  return true;
}

void _StreamDecoderDBusService::on_connection_closed( GDBusConnection* connection, gboolean remote_peer_vanished, GError* error, gpointer user_data )
{
  Tracer::overdose( __PRETTY_FUNCTION__ );
}

static void stream_decoder_dbus_service_dispose (GObject *object);

G_DEFINE_TYPE (StreamDecoderDBusService, stream_decoder_dbus_service, TYPE_STREAM_DECODER_SKELETON)

static void stream_decoder_dbus_service_class_init (StreamDecoderDBusServiceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->dispose = stream_decoder_dbus_service_dispose;

  stream_decoder_signals[SIGNAL_DECODE] =
  g_signal_new ("decode",
                G_TYPE_FROM_CLASS (klass),
                GSignalFlags (G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS),
                0, NULL, NULL,
                NULL,
                G_TYPE_BOOLEAN, 4, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_VARIANT, G_TYPE_POINTER);

  stream_decoder_signals[SIGNAL_STOP] =
  g_signal_new ("stop",
                G_TYPE_FROM_CLASS (klass),
                GSignalFlags (G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS),
                0, NULL, NULL,
                NULL,
                G_TYPE_NONE, 1, G_TYPE_UINT64);

  stream_decoder_signals[SIGNAL_GET_SUPPORTED_PROTOCOLS] =
  g_signal_new ("get-supported-protocols",
                G_TYPE_FROM_CLASS (klass),
                GSignalFlags (G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS),
                0, NULL, NULL,
                NULL,
                G_TYPE_STRV, 0);

  stream_decoder_signals[SIGNAL_RESET] =
  g_signal_new ("reset",
                G_TYPE_FROM_CLASS (klass),
                GSignalFlags (G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS),
                0, NULL, NULL,
                NULL,
                G_TYPE_NONE, 0 );
}

static void stream_decoder_dbus_service_bus_acquired (GDBusConnection *connection, const gchar *name, gpointer user_data)
{
  GDBusInterfaceSkeleton *skeleton = G_DBUS_INTERFACE_SKELETON (user_data);
  GError *error = NULL;

  g_signal_connect (skeleton, "handle-decode", G_CALLBACK (StreamDecoderDBusService::on_decode), user_data);
  g_signal_connect (skeleton, "handle-stop", G_CALLBACK (StreamDecoderDBusService::on_stop), user_data);
  g_signal_connect (skeleton, "handle-get-supported-protocols", G_CALLBACK (StreamDecoderDBusService::on_get_supported_protocols), user_data);
  g_signal_connect (skeleton, "handle-reset", G_CALLBACK (StreamDecoderDBusService::on_reset), user_data);

  int ret = g_signal_connect( G_DBUS_CONNECTION(connection), "closed", G_CALLBACK (StreamDecoderDBusService::on_connection_closed), user_data);
  if( 0 >= ret ){
    Tracer::warning( "Cannot connect to closed signal on GDBusConnection");
  }
  else {
    Tracer::overdose("Connected to closed signal on GDBusConnection, id:", ret );
  }

  if (g_dbus_interface_skeleton_export (skeleton, connection, "/com/raumfeld/StreamDecoder", &error))
  {
    g_print ("->DBus: exported interface at %s\n", g_dbus_interface_skeleton_get_object_path (skeleton));
  }
  else
  {
    g_printerr ("->DBus: error exporting interface: %s\n", error->message);
    g_error_free (error);
  }
  g_print ("->DBus: got message bus, now requesting to own %s\n", name);
}

static void stream_decoder_dbus_service_name_acquired (GDBusConnection *connection, const gchar *name, gpointer user_data)
{
  g_print ("->DBus: name aquired %s\n", name);
}

static void stream_decoder_dbus_service_name_lost (GDBusConnection *connection, const gchar *name, gpointer user_data)
{
  g_printerr ("->DBus: lost bus name, this shouldn't happen\n");
}

static void stream_decoder_dbus_service_init (StreamDecoderDBusService *service)
{
  service->owner_id = g_bus_own_name (G_BUS_TYPE_SESSION, "com.raumfeld.StreamDecoder",
                                      G_BUS_NAME_OWNER_FLAGS_NONE,
                                      stream_decoder_dbus_service_bus_acquired,
                                      stream_decoder_dbus_service_name_acquired,
                                      stream_decoder_dbus_service_name_lost,
                                      service, NULL);
}

static void stream_decoder_dbus_service_dispose (GObject *object)
{
  Tracer::overdose( __PRETTY_FUNCTION__ );

  StreamDecoderDBusService *service = STREAM_DECODER_DBUS_SERVICE(object);

  if (service->owner_id)
  {
    GDBusInterfaceSkeleton *skeleton = G_DBUS_INTERFACE_SKELETON (service);

    if (g_dbus_interface_skeleton_get_connection (skeleton) != NULL)
      g_dbus_interface_skeleton_unexport (skeleton);

    g_bus_unown_name (service->owner_id);
    service->owner_id = 0;
  }

  G_OBJECT_CLASS (stream_decoder_dbus_service_parent_class)->dispose (object);

}

StreamDecoderDBusService *stream_decoder_dbus_service_new ()
{
  StreamDecoderDBusService *service = (StreamDecoderDBusService *) g_object_new (TYPE_STREAM_DECODER_DBUS_SERVICE, NULL);

  return service;
}

void stream_decoder_emit_message_signal (StreamDecoderDBusService* object, uint64_t stream_id, const gchar* type, const gchar* str)
{
  stream_decoder_emit_message (STREAM_DECODER(object), type, str, stream_id);
}
