#ifndef __STREAM_DECODER_DBUS_SERVICE_H__
#define __STREAM_DECODER_DBUS_SERVICE_H__

#include <glib-object.h>
#include <cstdint>

#define TYPE_STREAM_DECODER_DBUS_SERVICE            (stream_decoder_dbus_service_get_type ())
#define STREAM_DECODER_DBUS_SERVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_STREAM_DECODER_DBUS_SERVICE, StreamDecoderDBusService))
#define STREAM_DECODER_DBUS_SERVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_STREAM_DECODER_DBUS_SERVICE, StreamDecoderDBusServiceClass))
#define IS_SDDBUS_SERVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_STREAM_DECODER_DBUS_SERVICE))
#define IS_STREAM_DECODER_DBUS_SERVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_STREAM_DECODER_DBUS_SERVICE))
#define STREAM_DECODER_DBUS_SERVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_STREAM_DECODER_DBUS_SERVICE, StreamDecoderDBusServiceClass))

typedef struct _StreamDecoderDBusService StreamDecoderDBusService;
typedef struct _StreamDecoderDBusServiceClass StreamDecoderDBusServiceClass;

typedef gint (*STOP_CALLBACK) (gpointer client_data);
typedef gint (*GET_SUPPORTED_PROTOCOLS_CALLBACK) (gchar** &protocols, gpointer client_data);

GType stream_decoder_dbus_service_get_type (void) G_GNUC_CONST;
StreamDecoderDBusService *stream_decoder_dbus_service_new ();
void stream_decoder_emit_message_signal (StreamDecoderDBusService* object, uint64_t stream_id, const gchar* type, const gchar* str);

#endif  /*  __STREAM_DECODER_DBUS_SERVICE_H__  */

