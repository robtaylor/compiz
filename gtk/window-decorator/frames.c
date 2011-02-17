#include "gtk-window-decorator.h"

typedef struct _decor_frame_type_info
{
    create_frame_proc *create_func;
    destroy_frame_proc *destroy_func;
} decor_frame_type_info_t;

decor_frame_t decor_frames[NUM_DECOR_FRAMES];
GHashTable    *frame_info_table;

decor_frame_t *
gwd_get_decor_frame (decor_frame_type type)
{
    decor_frames[type].refcount++;
    return &decor_frames[type];
}

decor_frame_t *
gwd_decor_frame_ref (decor_frame_t *frame)
{
    frame->refcount++;
    return frame;
}

decor_frame_t *
gwd_decor_frame_unref (decor_frame_t *frame)
{
    frame->refcount--;
    return frame;
}

gboolean
gwd_decor_frame_add_type (const gchar	     *name,
			  create_frame_proc  *create_func,
			  destroy_frame_proc *destroy_func)
{
    decor_frame_type_info_t *frame_type = malloc (sizeof (decor_frame_type_info_t));

    if (!frame_type)
	return FALSE;

    g_hash_table_replace (frame_info_table, strdup (name), frame_type);

    return TRUE;
}

void
gwd_decor_frame_remove_type (const gchar	     *name)
{
    g_hash_table_remove (frame_info_table, name);
}

void
destroy_frame_type (gpointer data)
{
    decor_frame_type_info_t *info = (decor_frame_type_info_t *) data;

    if (info)
    {
	/* TODO: Destroy all frames with this type using
	 * the frame destroy function */
    }

    free (info);
}

void
initialize_decorations ()
{
    GdkScreen *gdkscreen = gdk_screen_get_default ();
    GdkColormap *colormap;
    decor_extents_t _win_extents         = { 6, 6, 6, 6 };
    decor_extents_t _max_win_extents     = { 6, 6, 4, 6 };
    decor_extents_t _switcher_extents    = { 6, 6, 6, 6 + SWITCHER_SPACE };
    decor_context_t _window_context = {
	{ 0, 0, 0, 0 },
	6, 6, 4, 6,
	0, 0, 0, 0
    };

    decor_context_t _max_window_context = {
	{ 0, 0, 0, 0 },
	6, 6, 4, 6,
	0, 0, 0, 0
    };

    decor_context_t _window_context_no_shadow = {
	{ 0, 0, 0, 0 },
	6, 6, 4, 6,
	0, 0, 0, 0
    };

    decor_context_t _max_window_context_no_shadow = {
	{ 0, 0, 0, 0 },
	6, 6, 4, 6,
	0, 0, 0, 0
    };

    decor_context_t _switcher_context = {
	{ 0, 0, 0, 0 },
	6, 6, 6, 6 + SWITCHER_SPACE,
	0, 0, 0, 0
    };

    decor_context_t _shadow_context = {
	{ 0, 0, 0, 0 },
	0, 0, 0, 0,
	0, 0, 0, 0,
    };

    decor_extents_t _shadow_extents      = { 0, 0, 0, 0 };

    frame_info_table = g_hash_table_new_full (NULL, NULL, NULL, destroy_frame_type);

    unsigned int i;

    for (i = 0; i < NUM_DECOR_FRAMES; i++)
    {
	if (i == DECOR_FRAME_TYPE_SWITCHER)
	{
	    decor_frames[i].win_extents = _switcher_extents;
	    decor_frames[i].max_win_extents = _switcher_extents;
	    decor_frames[i].win_extents = _switcher_extents;
	    decor_frames[i].window_context = _switcher_context;
	    decor_frames[i].window_context_no_shadow = _switcher_context;
	    decor_frames[i].max_window_context = _switcher_context;
	    decor_frames[i].max_window_context_no_shadow = _switcher_context;
	    decor_frames[i].update_shadow = switcher_frame_update_shadow;
	}
	else if (i == DECOR_FRAME_TYPE_BARE)
	{
	    decor_frames[i].win_extents = _shadow_extents;
	    decor_frames[i].max_win_extents = _shadow_extents;
	    decor_frames[i].win_extents = _shadow_extents;
	    decor_frames[i].window_context = _shadow_context;
	    decor_frames[i].window_context_no_shadow = _shadow_context;
	    decor_frames[i].max_window_context = _shadow_context;
	    decor_frames[i].max_window_context_no_shadow = _shadow_context;
	    decor_frames[i].update_shadow = bare_frame_update_shadow;
	}
	else
	{
	    decor_frames[i].win_extents = _win_extents;
	    decor_frames[i].max_win_extents = _max_win_extents;
	    decor_frames[i].update_shadow = decor_frame_update_shadow;
	    decor_frames[i].window_context = _window_context;
	    decor_frames[i].window_context_no_shadow = _window_context_no_shadow;
	    decor_frames[i].max_window_context = _max_window_context;
	    decor_frames[i].max_window_context_no_shadow = _max_window_context_no_shadow;
	}

	decor_frames[i].titlebar_height = 17;
	decor_frames[i].max_titlebar_height = 17;
	decor_frames[i].border_shadow = NULL;
	decor_frames[i].border_no_shadow = NULL;
	decor_frames[i].max_border_no_shadow = NULL;
	decor_frames[i].max_border_shadow = NULL;
	decor_frames[i].titlebar_font = NULL;
	decor_frames[i].type = i;

	decor_frames[i].style_window_rgba = gtk_window_new (GTK_WINDOW_POPUP);

	colormap = gdk_screen_get_rgba_colormap (gdkscreen);
	if (colormap)
	    gtk_widget_set_colormap (decor_frames[i].style_window_rgba, colormap);

	gtk_widget_realize (decor_frames[i].style_window_rgba);

	gtk_widget_set_size_request (decor_frames[i].style_window_rgba, 0, 0);
	gtk_window_move (GTK_WINDOW (decor_frames[i].style_window_rgba), -100, -100);
	gtk_widget_show_all (decor_frames[i].style_window_rgba);

	decor_frames[i].pango_context = gtk_widget_create_pango_context (decor_frames[i].style_window_rgba);

	g_signal_connect_data (decor_frames[i].style_window_rgba, "style-set",
			       G_CALLBACK (style_changed),
			       (gpointer) decor_frames[i].pango_context, 0, 0);

	decor_frames[i].style_window_rgb = gtk_window_new (GTK_WINDOW_POPUP);

	colormap = gdk_screen_get_rgb_colormap (gdkscreen);
	if (colormap)
	    gtk_widget_set_colormap (decor_frames[i].style_window_rgb, colormap);

	gtk_widget_realize (decor_frames[i].style_window_rgb);

	gtk_widget_set_size_request (decor_frames[i].style_window_rgb, 0, 0);
	gtk_window_move (GTK_WINDOW (decor_frames[i].style_window_rgb), -100, -100);
	gtk_widget_show_all (decor_frames[i].style_window_rgb);

	g_signal_connect_data (decor_frames[i].style_window_rgb, "style-set",
			       G_CALLBACK (style_changed),
			       (gpointer) decor_frames[i].pango_context, 0, 0);

	update_style (decor_frames[i].style_window_rgba);
	update_style (decor_frames[i].style_window_rgb);
    }
}
