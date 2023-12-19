#include <gst/gst.h>

#include "config.h"

#include "gstmimofilter.h"

static gboolean
plugin_init (GstPlugin * plugin)
{
  gboolean ret = FALSE;

  ret |= GST_ELEMENT_REGISTER (mimofilter, plugin);

  return ret;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    mimo,
    "MIMO demo plugin",
    plugin_init, "0.1", "LGPL", "MIMO", "http://www.collabora.com")
