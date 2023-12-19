#pragma once

#include <gst/gst.h>

#include "api.h"

G_BEGIN_DECLS

#define GST_TYPE_MIMOFILTER (gst_mimofilter_get_type ())
G_DECLARE_FINAL_TYPE (GstMIMOFilter, gst_mimofilter, GST, MIMOFILTER, GstElement)

typedef enum
{
  GST_MIMOFILTER_FILTER_1,
  GST_MIMOFILTER_FILTER_2,
  GST_MIMOFILTER_FILTER_3,
  GST_MIMOFILTER_FILTER_DEFAULT = GST_MIMOFILTER_FILTER_1
} GstMIMOFilterFilter;

struct _GstMIMOFilter
{
  GstElement parent;

  GstMIMOFilterFilter filter;

  MimoSession *session;

  GList *sinkpads;
  GList *srcpads;
};

GST_ELEMENT_REGISTER_DECLARE (mimofilter);

G_END_DECLS
