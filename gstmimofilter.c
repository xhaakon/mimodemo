#include "gstmimofilter.h"

G_DEFINE_FINAL_TYPE (GstMIMOFilter, gst_mimofilter, GST_TYPE_ELEMENT);
GST_ELEMENT_REGISTER_DEFINE (mimofilter, "mimofilter", GST_RANK_NONE, GST_TYPE_MIMOFILTER);

#define GST_TYPE_MIMOFILTER_FILTER (gst_mimofilter_filter_get_type ())
static GType
gst_mimofilter_filter_get_type (void)
{
  static GType type = 0;

  static const GEnumValue filters[] = {
    {GST_MIMOFILTER_FILTER_1, "First filter", "filter1"},
    {GST_MIMOFILTER_FILTER_2, "Second filter", "filter2"},
    {GST_MIMOFILTER_FILTER_3, "Third filter", "filter3"},
    {0, NULL, NULL},
  };

  if (g_once_init_enter (&type)) {
    GType tmp = g_enum_register_static ("GstMIMOFilterFilter", filters);
    g_once_init_leave (&type, tmp);
  }

  return type;
}

enum
{
	PROP_0,
	PROP_FILTER,
};

static GstStaticPadTemplate gst_mimofilter_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink_%u",
    GST_PAD_SINK,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS ("video/x-raw"));

static GstStaticPadTemplate gst_mimofilter_src_template =
GST_STATIC_PAD_TEMPLATE ("src_%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS_ANY);

static void
gst_mimofilter_init (GstMIMOFilter * self)
{
}

static gboolean
gst_mimofilter_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstMIMOFilter *self = GST_MIMOFILTER (parent);

  gboolean ret = TRUE;

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:{
      GstCaps *caps;
      GList *it;

      gst_event_parse_caps (event, &caps);

      for (it = self->srcpads; it; it = it->next) {
    	  GstPad *srcpad = it->data;
		  if (!gst_pad_set_caps (srcpad, caps)) {
			  ret = FALSE;
			  break;
		  }
      }

      gst_clear_event (&event);
      break;
    }
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }

  return ret;
}

static GstFlowReturn
gst_mimofilter_chain (GstPad * pad, GstObject * parent, GstBuffer * inbuf)
{
  GstMIMOFilter *self = GST_MIMOFILTER (parent);
  GstFlowReturn ret = GST_FLOW_OK;

  MimoFilterInfo filter = mimo_enumerate_filters ()[self->filter];

  guint mimo_input_num = GPOINTER_TO_UINT (pad->chaindata);
  guint i;

  GST_OBJECT_LOCK (self);

  if (!mimo_session_send_frame (self->session, mimo_input_num, inbuf)) {
	  ret = GST_FLOW_ERROR;
	  GST_OBJECT_UNLOCK (self);
	  goto out;
  }

  GST_OBJECT_UNLOCK (self);

  for (i = 0; i != filter.num_outputs; ++i) {
	  GST_OBJECT_LOCK (self);
	  GstBuffer *outbuf = mimo_session_receive_frame (self->session, i);
	  GST_OBJECT_UNLOCK (self);
	  if (outbuf) {
		  gst_pad_push (g_list_nth_data (self->srcpads, i), outbuf);
	  }
  }

out:
  return ret;
}

static void
gst_mimofilter_destroy_pads (GstMIMOFilter * self)
{
	GList *it;

	for (it = self->srcpads; it; it = it->next) {
		GstPad *pad = it->data;
		gst_element_remove_pad (GST_ELEMENT (self), pad);
	}

	for (it = self->sinkpads; it; it = it->next) {
		GstPad *pad = it->data;
		gst_element_remove_pad (GST_ELEMENT (self), pad);
	}

	g_clear_list (&self->srcpads, NULL);
	g_clear_list (&self->sinkpads, NULL);
}

static void
gst_mimofilter_update_pads (GstMIMOFilter * self)
{
  MimoFilterInfo filter = mimo_enumerate_filters ()[self->filter];
  GstPadTemplate *template;
  GstCaps *caps;
  gint i;

  gst_mimofilter_destroy_pads (self);

  caps = gst_caps_new_simple ("video/x-raw",
 	  "format", G_TYPE_STRING, gst_video_format_to_string (filter.format),
 	  NULL);

  template = gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_SOMETIMES, caps);

  for (i = 0; i != filter.num_inputs; ++i)
  {
	gchar *pad_name = g_strdup_printf ("sink_%d", i);

	GstPad *pad = gst_pad_new_from_template (template, pad_name);
	gst_pad_set_caps (pad, caps);

	gst_pad_set_event_function (pad, gst_mimofilter_sink_event);
	gst_pad_set_chain_function_full (pad, gst_mimofilter_chain, GINT_TO_POINTER (i), NULL);

	gst_pad_set_active (pad, TRUE);
	gst_element_add_pad (GST_ELEMENT (self), pad);

	self->sinkpads = g_list_append (self->sinkpads, pad);

	g_clear_pointer (&pad_name, g_free);
  }

  gst_clear_object (&template);
  template = gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_SOMETIMES, caps);

  for (i = 0; i != filter.num_outputs; ++i)
  {
	gchar *pad_name = g_strdup_printf ("src_%d", i);

	GstPad *pad = gst_pad_new_from_template (template, pad_name);
	gst_pad_set_caps (pad, caps);

	gst_pad_set_active (pad, TRUE);
	gst_element_add_pad (GST_ELEMENT (self), pad);

	self->srcpads = g_list_append (self->srcpads, pad);

	g_clear_pointer (&pad_name, g_free);
  }

  gst_clear_caps (&caps);
}

static gboolean
gst_mimofilter_start (GstMIMOFilter * self)
{
  self->session = mimo_session_create (self->filter);

  return self->session != NULL;
}

static gboolean
gst_mimofilter_stop (GstMIMOFilter * self)
{
  g_clear_pointer (&self->session, mimo_session_free);

  return TRUE;
}

static GstStateChangeReturn
gst_mimofilter_change_state (GstElement * element, GstStateChange transition)
{
  GstMIMOFilter *self = GST_MIMOFILTER (element);
  GstStateChangeReturn ret;

  gst_println("CHANGE STATE");

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      if (!gst_mimofilter_start (self)) {
        return GST_STATE_CHANGE_FAILURE;
      }
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (gst_mimofilter_parent_class)->change_state (element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE)
    return ret;

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_NULL:
      if (!gst_mimofilter_stop (self)) {
        return GST_STATE_CHANGE_FAILURE;
      }
      break;
    default:
      break;
  }

  return ret;
}

static void
gst_mimofilter_set_property (GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GstMIMOFilter *self = GST_MIMOFILTER (object);

  switch (prop_id) {
    case PROP_FILTER:
      self->filter = g_value_get_enum (value);
      gst_mimofilter_update_pads (self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_mimofilter_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstMIMOFilter *self = GST_MIMOFILTER (object);

  switch (prop_id) {
    case PROP_FILTER:
    	g_value_set_enum (value, self->filter);
    	break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_mimofilter_dispose (GObject * object)
{
	GstMIMOFilter *self = GST_MIMOFILTER (object);

	g_clear_list (&self->srcpads, NULL);
	g_clear_list (&self->sinkpads, NULL);
}

static void
gst_mimofilter_class_init (GstMIMOFilterClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gobject_class->set_property = gst_mimofilter_set_property;
  gobject_class->get_property = gst_mimofilter_get_property;
  gobject_class->dispose = gst_mimofilter_dispose;

  element_class->change_state = gst_mimofilter_change_state;

  g_object_class_install_property (gobject_class, PROP_FILTER,
        g_param_spec_enum ("filter", "MIMO filter to use",
        	"MIMO filter to use", GST_TYPE_MIMOFILTER_FILTER,
            GST_MIMOFILTER_FILTER_DEFAULT,
            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT));

  gst_element_class_add_static_pad_template (element_class,
      &gst_mimofilter_sink_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_mimofilter_src_template);

  gst_element_class_set_static_metadata (element_class,
      "MIMO Filter", "Filter/Converter/Video",
      "MIMO Filter", "Collabora Inc., https://www.collabora.com");

}
