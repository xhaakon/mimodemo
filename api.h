#pragma once

#include <gst/video/video-format.h>

typedef struct
{
	const gchar *name;
	gint num_inputs;
	gint num_outputs;
	GstVideoFormat format;
} MimoFilterInfo;

typedef struct _MimoSession MimoSession;

MimoFilterInfo *mimo_enumerate_filters ();

MimoSession *mimo_session_create (guint filter_num);

gboolean mimo_session_send_frame (MimoSession *session, guint input, GstBuffer *buffer);
GstBuffer * mimo_session_receive_frame (MimoSession *session, guint output);

void mimo_session_free (MimoSession *session);
