#include "SupportedProtocols.h"
#include <string.h>
#include <gst/gst.h>
#include <gio/gio.h>

const SupportedProtocols& SupportedProtocols::get ()
{
  static SupportedProtocols sp;
  return sp;
}

SupportedProtocols::SupportedProtocols ()
{
  init ();
}

SupportedProtocols::~SupportedProtocols ()
{
}

void SupportedProtocols::init ()
{
  GList *list = gst_type_find_factory_get_list ();

  for (GList *iter = list; iter; iter = iter->next)
  {
    GstTypeFindFactory *fac = (GstTypeFindFactory *) iter->data;
    GstCaps *caps = gst_type_find_factory_get_caps (fac);
    char *mime = gst_caps_to_string (caps);

    if (mime && *mime)
    {
      if (g_str_has_prefix (mime, "audio/") || g_strcmp0 (mime, "application/ogg") == 0 || g_strcmp0 (mime, "application/x-ogg") == 0)
      {
        char *comma = strchr (mime, ',');

        if (comma)
          *comma = '\0';

        string str;
        str += "http-get:*:";
        str += (const char *) mime;
        str += ":*";

        m_supportedProtocols.push_back (str);

        gchar *type = g_content_type_from_mime_type (mime);

        if (type && strcmp (type, mime))
        {
          string str;
          str += "http-get:*:";
          str += (const char *) type;
          str += ":*";

          m_supportedProtocols.push_back (str);
        }

        g_free (type);
      }
    }

    g_free (mime);
  }

  gst_plugin_feature_list_free (list);

  m_supportedProtocols.push_back ("http-get:*:" "audio/x-ms-wma" ":*");
}

gchar **SupportedProtocols::asArrayOfStrings () const
{
  gchar** ret = g_new (gchar *, m_supportedProtocols.size() + 1);
  gchar** walker = ret;

  for (const string &item : m_supportedProtocols)
  {
    *walker = g_strdup (item.c_str ());
    walker++;
  }

  *walker = NULL;
  return ret;
}
