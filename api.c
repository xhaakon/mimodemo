#include "api.h"

/* Switch to the next input after playing this number of frames. */
#define NUM_INPUT_BUFFERS 150

typedef struct _MimoSession {
	guint filter_num;

	guint num_buffers;
	guint active_input;

	struct {
		GQueue *queue;
	} outputs[5];

} MimoSession;

static MimoFilterInfo filters[] = {
		{"filter 1", 1, 1, GST_VIDEO_FORMAT_I420},
		{"filter 2", 2, 3, GST_VIDEO_FORMAT_NV12},
		{"filter 3", 4, 2, GST_VIDEO_FORMAT_BGR},
		{NULL},
};

MimoFilterInfo *
mimo_enumerate_filters ()
{
	return filters;
}

MimoSession *
mimo_session_create (guint filter_num)
{
	MimoFilterInfo filter = filters[filter_num];
	MimoSession *session = g_new0 (MimoSession, 1);
	guint i;

	session->filter_num = filter_num;


	for (i = 0; i != filter.num_outputs; ++i) {
		session->outputs[i].queue = g_queue_new ();
	}

	return session;
}

gboolean
mimo_session_send_frame (MimoSession *session, guint input, GstBuffer *buffer)
{
	MimoFilterInfo filter = filters[session->filter_num];
	guint i;

	if (input >= filter.num_inputs) {
		gst_clear_buffer (&buffer);
		return FALSE;
	}

	if (input != session->active_input) {
		gst_clear_buffer (&buffer);
		return TRUE;
	}

	for (i = 0; i != filter.num_outputs; ++i) {
		g_queue_push_tail (session->outputs[i].queue, gst_buffer_ref (buffer));
	}

	if (++session->num_buffers == NUM_INPUT_BUFFERS) {
		session->active_input++;
		session->active_input %= filter.num_inputs;
		session->num_buffers = 0;
	}

	return TRUE;
}

GstBuffer *
mimo_session_receive_frame (MimoSession *session, guint output)
{
	MimoFilterInfo filter = filters[session->filter_num];

	if (output >= filter.num_outputs) {
		return NULL;
	}

	return g_queue_pop_head (session->outputs[output].queue);
}

void
mimo_session_free (MimoSession *session)
{
	MimoFilterInfo filter = filters[session->filter_num];
	guint i;

	for (i = 0; i != filter.num_outputs; ++i) {
		GQueue *q = session->outputs[i].queue;
		g_queue_clear (q);
		g_queue_free (q);
	}

	g_free (session);
}
