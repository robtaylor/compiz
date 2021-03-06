/*
 * Copyright © 2006 Novell, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: David Reveman <davidr@novell.com>
 *
 * 2D Mode: Copyright © 2010 Sam Spilsbury <smspillaz@gmail.com>
 * Frames Management: Copright © 2011 Canonical Ltd.
 *        Authored By: Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#include "gtk-window-decorator.h"

decor_frame_t *
create_normal_frame (const gchar *type)
{
    decor_frame_t *frame = decor_frame_new (type);

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

    decor_extents_t _win_extents         = { 6, 6, 6, 6 };
    decor_extents_t _max_win_extents     = { 6, 6, 4, 6 };

    frame->win_extents = _win_extents;
    frame->max_win_extents = _max_win_extents;
    frame->update_shadow = decor_frame_update_shadow;
    frame->window_context = _window_context;
    frame->window_context_no_shadow = _window_context_no_shadow;
    frame->max_window_context = _max_window_context;
    frame->max_window_context_no_shadow = _max_window_context_no_shadow;

    return frame;
}

void
destroy_normal_frame (decor_frame_t *frame)
{
    decor_frame_destroy (frame);
}

decor_frame_t *
create_bare_frame (const gchar *type)
{
    decor_frame_t *frame = decor_frame_new (type);
    decor_context_t _shadow_context = {
	{ 0, 0, 0, 0 },
	0, 0, 0, 0,
	0, 0, 0, 0,
    };

    decor_extents_t _shadow_extents      = { 0, 0, 0, 0 };

    frame->win_extents = _shadow_extents;
    frame->max_win_extents = _shadow_extents;
    frame->win_extents = _shadow_extents;
    frame->window_context = _shadow_context;
    frame->window_context_no_shadow = _shadow_context;
    frame->max_window_context = _shadow_context;
    frame->max_window_context_no_shadow = _shadow_context;
    frame->update_shadow = bare_frame_update_shadow;

    return frame;
}

void
destroy_bare_frame (decor_frame_t *frame)
{
    decor_frame_destroy (frame);
}


/*
 * get_titlebar_font
 *
 * Returns: PangoFontDescription * or NULL if using system font
 * Description: Helper function to get the font for the titlebar
 */
static const PangoFontDescription *
get_titlebar_font (decor_frame_t *frame)
{
    if (settings->use_system_font)
	return NULL;
    else
	return frame->titlebar_font;
}

/*
 * frame_update_titlebar_font
 *
 * Returns: void
 * Description: updates the titlebar font from the pango context, should
 * be called whenever the gtk style or font has changed
 */
void
frame_update_titlebar_font (decor_frame_t *frame)
{
    const PangoFontDescription *font_desc;
    PangoFontMetrics	       *metrics;
    PangoLanguage	       *lang;

    frame = gwd_decor_frame_ref (frame);

    font_desc = get_titlebar_font (frame);
    if (!font_desc)
    {
	GtkStyle *default_style;

	default_style = gtk_widget_get_default_style ();
	font_desc = default_style->font_desc;
    }

    pango_context_set_font_description (frame->pango_context, font_desc);

    lang    = pango_context_get_language (frame->pango_context);
    metrics = pango_context_get_metrics (frame->pango_context, font_desc, lang);

    frame->text_height = PANGO_PIXELS (pango_font_metrics_get_ascent (metrics) +
				pango_font_metrics_get_descent (metrics));

    gwd_decor_frame_unref (frame);

    pango_font_metrics_unref (metrics);
}

void
update_frames_titlebar_fonts (gpointer key,
			      gpointer value,
			      gpointer user_data)
{
    frame_update_titlebar_font ((decor_frame_t *) value);
}

void
update_titlebar_font ()
{
    gwd_frames_foreach (update_frames_titlebar_fonts, NULL);
}


/*
 * update_event_windows
 *
 * Returns: void
 * Description: creates small "event windows" for the buttons specified to be
 * on the titlebar by wnck. Note here that for the pixmap mode we create actual
 * X windows but in the reparenting mode this is not possible so we create event
 * capture boxes on the window instead. The geometry of the decoration is retrieved
 * with wnck_window_get_client_window_geometry and adjusted for shade. Also we
 * need to query the theme for what window positions are appropriate here.
 *
 * This function works on the buttons and also the small event regions that we need
 * in order to toggle certain actions on the window decoration (eg resize, move)
 *
 * So your window decoration might look something like this (not to scale):
 *
 * -----------------------------------------------------------
 * | rtl |                   rt                        | rtr |
 * | --- |---------------------------------------------| --- |
 * |     | [i][s][m]         mv              [_][M][X] |     |
 * |     |---------------------------------------------|     |
 * |     |                                             |     |
 * | rl  |             window contents                 | rr  |
 * |     |                                             |     |
 * |     |                                             |     |
 * | --- |---------------------------------------------| --- |
 * | rbl |                  rb                         | rbr |
 * -----------------------------------------------------------
 *
 * Where:
 * - rtl = resize top left
 * - rtr = resize top right
 * - rbl = resize bottom left
 * - rbr = resize bottom right
 * - rt = resize top
 * - rb = resize bottom
 * - rl = resize left
 * - rr = resize right
 * - mv = "grab move" area (eg titlebar)
 * - i = icon
 * - s = shade
 * - m = menu
 * - _ = minimize
 * - M = maximize
 * - X = close
 *
 * For the reparenting mode we use button_windows[i].pos and for the pixmap mode
 * we use buttons_windows[i].window
 *
 */
void
update_event_windows (WnckWindow *win)
{
    Display *xdisplay;
    decor_t *d = g_object_get_data (G_OBJECT (win), "decor");
    gint    x0, y0, width, height, x, y, w, h;
    gint    i, j, k, l;
    gint    actions = d->actions;

    xdisplay = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());

    /* Get the geometry of the client */
    wnck_window_get_client_window_geometry (win, &x0, &y0, &width, &height);

    /* Shaded windows have no height - also skip some event windows */
    if (d->state & WNCK_WINDOW_STATE_SHADED)
    {
	height = 0;
	k = l = 1;
    }
    else
    {
	k = 0;
	l = 2;
    }

    gdk_error_trap_push ();

    /* [rtl, ru, rtr], [rl, mv, rr], [rbl, rb, rbr] */
    for (i = 0; i < 3; i++)
    {
	static guint event_window_actions[3][3] = {
	    {
		WNCK_WINDOW_ACTION_RESIZE,
		WNCK_WINDOW_ACTION_RESIZE,
		WNCK_WINDOW_ACTION_RESIZE
	    }, {
		WNCK_WINDOW_ACTION_RESIZE,
		WNCK_WINDOW_ACTION_MOVE,
		WNCK_WINDOW_ACTION_RESIZE
	    }, {
		WNCK_WINDOW_ACTION_RESIZE,
		WNCK_WINDOW_ACTION_RESIZE,
		WNCK_WINDOW_ACTION_RESIZE
	    }
	};

	for (j = 0; j < 3; j++)
	{
	    w = 0;
	    h = 0;

	    if (actions & event_window_actions[i][j] && i >= k && i <= l)
		(*theme_get_event_window_position) (d, i, j, width, height,
						    &x, &y, &w, &h);

	    /* Reparenting mode - create boxes which we monitor motionnotify on */
	    if (d->frame_window)
	    {
		BoxPtr box = &d->event_windows[i][j].pos;
		box->x1  = x;
		box->x2 = x + w;
		box->y1 = y;
		box->y2 = y + h;
	    }
	    /* Pixmap mode with window geometry - create small event windows */
	    else if (!d->frame_window && w != 0 && h != 0)
	    {
		XMapWindow (xdisplay, d->event_windows[i][j].window);
		XMoveResizeWindow (xdisplay, d->event_windows[i][j].window,
				   x, y, w, h);
	    }
	    /* No parent and no geometry - unmap all event windows */
	    else if (!d->frame_window)
	    {
		XUnmapWindow (xdisplay, d->event_windows[i][j].window);
	    }
	}
    }

    /* no button event windows if width is less than minimum width */
    if (width < ICON_SPACE + d->button_width)
	actions = 0;

    /* Above, stick, unshade and unstick are only available in wnck => 2.18.1 */
    for (i = 0; i < BUTTON_NUM; i++)
    {
	static guint button_actions[BUTTON_NUM] = {
	    WNCK_WINDOW_ACTION_CLOSE,
	    WNCK_WINDOW_ACTION_MAXIMIZE,
	    WNCK_WINDOW_ACTION_MINIMIZE,
	    0,
	    WNCK_WINDOW_ACTION_SHADE,

#ifdef HAVE_LIBWNCK_2_18_1
	    WNCK_WINDOW_ACTION_ABOVE,
	    WNCK_WINDOW_ACTION_STICK,
	    WNCK_WINDOW_ACTION_UNSHADE,
	    WNCK_WINDOW_ACTION_ABOVE,
	    WNCK_WINDOW_ACTION_UNSTICK
#else
	    0,
	    0,
	    0,
	    0,
	    0
#endif

	};

	/* Reparenting mode - if a box was set and we no longer need it reset its geometry */
	if (d->frame_window &&
	    button_actions[i] && !(actions & button_actions[i]))
	{
	    memset (&d->button_windows[i].pos, 0, sizeof (Box));
	}
	/* Pixmap mode - if a box was set and we no longer need it unmap its window */
	else if (!d->frame_window &&
		 button_actions[i] && !(actions & button_actions[i]))
	{
	    XUnmapWindow (xdisplay, d->button_windows[i].window);
	    continue;
	}

	/* Reparenting mode - if there is a button position for this
	 * button then set the geometry */
	if (d->frame_window &&
	    (*theme_get_button_position) (d, i, width, height, &x, &y, &w, &h))
	{
	    BoxPtr box = &d->button_windows[i].pos;
	    box->x1 = x;
	    box->y1 = y;
	    box->x2 = x + w;
	    box->y2 = y + h;
	}
	/* Pixmap mode - if there is a button position for this button then map the window
	 * and resize it to this position */
	else if (!d->frame_window &&
		 (*theme_get_button_position) (d, i, width, height,
					       &x, &y, &w, &h))
	{
	    Window win = d->button_windows[i].window;
	    XMapWindow (xdisplay, win);
	    XMoveResizeWindow (xdisplay, win, x, y, w, h);
	}
	else if (!d->frame_window)
	{
	    XUnmapWindow (xdisplay, d->button_windows[i].window);
	}
    }

    gdk_display_sync (gdk_display_get_default ());
    gdk_error_trap_pop ();
}

/*
 * wnck_window_get_real_name
 *
 * Returns: const char * or NULL
 * Description: Wrapper function to either get the name of the window or
 * return NULL
 */

#ifdef HAVE_WNCK_WINDOW_HAS_NAME
static const char *
wnck_window_get_real_name (WnckWindow *win)
{
    return wnck_window_has_name (win) ? wnck_window_get_name (win) : NULL;
}
#define wnck_window_get_name wnck_window_get_real_name
#endif

/*
 * max_window_name_width
 *
 * Returns: gint
 * Description: Calculate the width of the decoration required to display
 * the window name using pango (with 6px padding)
 * Returns zero if window has no name.
 */
gint
max_window_name_width (WnckWindow *win)
{
    decor_t     *d = g_object_get_data (G_OBJECT (win), "decor");
    const gchar *name;
    gint	w;

    /* Ensure that a layout is created */
    if (!d->layout)
    {
	d->layout = pango_layout_new (d->frame->pango_context);
	if (!d->layout)
	    return 0;

	pango_layout_set_wrap (d->layout, PANGO_WRAP_CHAR);
    }

    /* Return zero if window has no name */
    name = wnck_window_get_name (win);
    if (!name)
	return 0;

    /* Reset the width, set hte text and get the size required */
    pango_layout_set_auto_dir (d->layout, FALSE);
    pango_layout_set_width (d->layout, -1);
    pango_layout_set_text (d->layout, name, strlen (name));
    pango_layout_get_pixel_size (d->layout, &w, NULL);

    if (d->name)
	pango_layout_set_text (d->layout, d->name, strlen (d->name));

    return w + 6;
}

/*
 * update_window_decoration_name
 *
 * Returns: void
 * Description: frees the last window name and gets the new one from
 * wnck. Also checks to see if the name has a length (slight optimization)
 * and re-creates the pango context to re-render the name
 */
void
update_window_decoration_name (WnckWindow *win)
{
    decor_t	    *d = g_object_get_data (G_OBJECT (win), "decor");
    const gchar	    *name;
    glong	    name_length;
    PangoLayoutLine *line;

    if (d->name)
    {
	g_free (d->name);
	d->name = NULL;
    }

    /* Only operate if the window name has a length */
    name = wnck_window_get_name (win);
    if (name && (name_length = strlen (name)))
    {
	gint w;

	/* Cairo mode: w = SHRT_MAX */
	if (theme_draw_window_decoration != draw_window_decoration)
	{
	    w = SHRT_MAX;
	}
	/* Need to get a minimum width for the name */
	else
	{
	    gint width;

	    wnck_window_get_client_window_geometry (win, NULL, NULL,
						    &width, NULL);

	    w = width - ICON_SPACE - 2 - d->button_width;
	    if (w < 1)
		w = 1;
	}

	/* Set the maximum width for the layout (in case
	 * decoration size < text width) since we
	 * still need to show the buttons and the window name */
	pango_layout_set_auto_dir (d->layout, FALSE);
	pango_layout_set_width (d->layout, w * PANGO_SCALE);
	pango_layout_set_text (d->layout, name, name_length);

	line = pango_layout_get_line (d->layout, 0);

	name_length = line->length;
	if (pango_layout_get_line_count (d->layout) > 1)
	{
	    if (name_length < 4)
	    {
		pango_layout_set_text (d->layout, NULL, 0);
		return;
	    }

	    d->name = g_strndup (name, name_length);
	    strcpy (d->name + name_length - 3, "...");
	}
	else
	    d->name = g_strndup (name, name_length);

	/* Truncate the text */
	pango_layout_set_text (d->layout, d->name, name_length);
    }
}

/*
 * update_window_decoration_icon
 *
 * Updates the window icon (destroys the existing cairo pattern
 * and creates a new one for the pixmap)
 */
void
update_window_decoration_icon (WnckWindow *win)
{
    decor_t *d = g_object_get_data (G_OBJECT (win), "decor");

    /* Destroy old stuff */
    if (d->icon)
    {
	cairo_pattern_destroy (d->icon);
	d->icon = NULL;
    }

    if (d->icon_pixmap)
    {
	g_object_unref (G_OBJECT (d->icon_pixmap));
	d->icon_pixmap = NULL;
    }

    if (d->icon_pixbuf)
	g_object_unref (G_OBJECT (d->icon_pixbuf));

    /* Get the mini icon pixbuf from libwnck */
    d->icon_pixbuf = wnck_window_get_mini_icon (win);
    if (d->icon_pixbuf)
    {
	cairo_t	*cr;

	g_object_ref (G_OBJECT (d->icon_pixbuf));

	/* 32 bit pixmap on pixmap mode, 24 for reparenting */
	if (d->frame_window)
	    d->icon_pixmap = pixmap_new_from_pixbuf (d->icon_pixbuf,
						     d->frame->style_window_rgba);
	else
	    d->icon_pixmap = pixmap_new_from_pixbuf (d->icon_pixbuf,
						     d->frame->style_window_rgb);
	cr = gdk_cairo_create (GDK_DRAWABLE (d->icon_pixmap));
	d->icon = cairo_pattern_create_for_surface (cairo_get_target (cr));
	cairo_destroy (cr);
    }
}


/*
 * update_window_decoration_size
 * Returns: FALSE for failure, TRUE for success
 * Description: Calculates the minimum size of the decoration that we need
 * to render. This is mostly done by the theme but there is some work that
 * we need to do here first, such as getting the client geometry, setting
 * drawable depths, creating pixmaps, creating XRenderPictures and
 * updating the window decoration name
 */
 
gboolean
update_window_decoration_size (WnckWindow *win)
{
    decor_t           *d = g_object_get_data (G_OBJECT (win), "decor");
    GdkPixmap         *pixmap, *buffer_pixmap = NULL;
    Picture           picture;
    gint              width, height;
    gint              x, y, w, h, name_width;
    Display           *xdisplay;
    XRenderPictFormat *format;

    xdisplay = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());

    /* Get the geometry of the window, we'll need it later */
    wnck_window_get_client_window_geometry (win, &x, &y, &w, &h);

    /* Get the width of the name */
    name_width = max_window_name_width (win);

    /* Ask the theme to tell us how much space it needs. If this is not successful
     * update the decoration name and return false */
    if (!(*theme_calc_decoration_size) (d, w, h, name_width, &width, &height))
    {
	update_window_decoration_name (win);
	return FALSE;
    }

    gdk_error_trap_push ();

    /* Get the correct depth for the frame window in reparenting mode, otherwise
     * enforce 32 */
    if (d->frame_window)
	pixmap = create_pixmap (width, height, d->frame->style_window_rgb);
    else
	pixmap = create_pixmap (width, height, d->frame->style_window_rgba);

    gdk_flush ();

    /* Handle failure */
    if (!pixmap || gdk_error_trap_pop ())
    {
	memset (pixmap, 0, sizeof (pixmap));
	return FALSE;
    }

    gdk_error_trap_push ();

    if (d->frame_window)
	buffer_pixmap = create_pixmap (width, height, d->frame->style_window_rgb);
    else
	buffer_pixmap = create_pixmap (width, height, d->frame->style_window_rgba);

    gdk_flush ();

    /* Handle failure */
    if (!buffer_pixmap || gdk_error_trap_pop ())
    {
	memset (buffer_pixmap, 0, sizeof (buffer_pixmap));
	g_object_unref (G_OBJECT (pixmap));
	return FALSE;
    }

    /* Create XRender context */
    format = get_format_for_drawable (d, GDK_DRAWABLE (buffer_pixmap));
    picture = XRenderCreatePicture (xdisplay, GDK_PIXMAP_XID (buffer_pixmap),
				    format, 0, NULL);

    /* Destroy the old pixmaps and pictures */
    if (d->pixmap)
	g_object_unref (G_OBJECT (d->pixmap));

    if (d->buffer_pixmap)
	g_object_unref (G_OBJECT (d->buffer_pixmap));

    if (d->picture)
	XRenderFreePicture (xdisplay, d->picture);

    if (d->cr)
	cairo_destroy (d->cr);

    /* Assign new pixmaps and pictures */
    d->pixmap	     = pixmap;
    d->buffer_pixmap = buffer_pixmap;
    d->cr	     = gdk_cairo_create (pixmap);

    d->picture = picture;

    d->width  = width;
    d->height = height;

    d->prop_xid = wnck_window_get_xid (win);

    update_window_decoration_name (win);

    /* Redraw decoration on idle */
    queue_decor_draw (d);

    return TRUE;
}

/* to save some memory, value is specific to current decorations */
#define TRANSLUCENT_CORNER_SIZE 3

/*
 * draw_border_shape
 * Returns: void
 * Description: Draws a slight border around the decoration
 */
static void
draw_border_shape (Display	   *xdisplay,
		   Pixmap	   pixmap,
		   Picture	   picture,
		   int		   width,
		   int		   height,
		   decor_context_t *c,
		   void		   *closure)
{
    static XRenderColor white = { 0xffff, 0xffff, 0xffff, 0xffff };
    GdkColormap		*colormap;
    decor_t		d;
    decor_shadow_info_t *info = (decor_shadow_info_t *) closure;
    double		save_decoration_alpha;

    memset (&d, 0, sizeof (d));

    if (info)
	d.frame = info->frame;
    else
	d.frame = gwd_get_decor_frame ("default");

    d.pixmap  = gdk_pixmap_foreign_new_for_display (gdk_display_get_default (),
						    pixmap);
    d.width   = width;
    d.height  = height;
    d.active  = TRUE;
    d.draw    = theme_draw_window_decoration;
    d.picture = picture;
    d.context = c;

    /* we use closure argument if maximized */
    if (info)
	d.state = info->state;
    else
	d.state = 0;

    decor_get_default_layout (c, 1, 1, &d.border_layout);

    colormap = get_colormap_for_drawable (GDK_DRAWABLE (d.pixmap));
    gdk_drawable_set_colormap (d.pixmap, colormap);

    /* create shadow from opaque decoration
     * FIXME: Should not modify settings value
     * like this */
    save_decoration_alpha = settings->decoration_alpha;
    settings->decoration_alpha = 1.0;

    (*d.draw) (&d);

    settings->decoration_alpha = save_decoration_alpha;

    XRenderFillRectangle (xdisplay, PictOpSrc, picture, &white,
			  c->left_space,
			  c->top_space,
			  width - c->left_space - c->right_space,
			  height - c->top_space - c->bottom_space);

    if (!info)
	gwd_decor_frame_unref (d.frame);

    g_object_unref (G_OBJECT (d.pixmap));
}


/*
 * update_shadow
 * Returns: 1 for success, 0 for failure
 * Description: creates a libdecoration shadow context and updates
 * the decoration context for the shadow for the properties that we
 * have already read from the root window.
 *
 * For the pixmap mode we have opt_shadow which is passed to
 * decor_shadow_create (which contains the shadow settings from
 * the root window)
 *
 * For the reparenting mode we always enforce a zero-shadow in
 * the opt_no_shadow passed to decor_shadow_create.
 *
 * We do something similar  for the maximimzed mode as well
 */
void
bare_frame_update_shadow (Display		  *xdisplay,
			  Screen		  *screen,
			  decor_frame_t		 *frame,
			  decor_shadow_info_t    *info,
			  decor_shadow_options_t *opt_shadow,
			  decor_shadow_options_t *opt_no_shadow)
{
    if (frame->border_shadow)
    {
	decor_shadow_destroy (xdisplay, frame->border_shadow);
	frame->border_shadow = NULL;
    }

    frame->border_shadow = decor_shadow_create (xdisplay,
					    screen,
					    1, 1,
					    0,
					    0,
					    0,
					    0,
					    0, 0, 0, 0,
					    opt_shadow,
					    &frame->window_context,
					    decor_draw_simple,
					    NULL);
}

void
switcher_frame_update_shadow (Display		  *xdisplay,
			      Screen		  *screen,
			      decor_frame_t	  *frame,
			      decor_shadow_info_t    *info,
			      decor_shadow_options_t *opt_shadow,
			      decor_shadow_options_t *opt_no_shadow)
{
    if (frame->border_shadow)
    {
	decor_shadow_destroy (xdisplay, frame->border_shadow);
	frame->border_shadow = NULL;
    }

    frame->border_shadow = decor_shadow_create (xdisplay,
						screen,
						1, 1,
						frame->win_extents.left,
						frame->win_extents.right,
						frame->win_extents.top,
						frame->win_extents.bottom,
						frame->win_extents.left -
						TRANSLUCENT_CORNER_SIZE,
						frame->win_extents.right -
						TRANSLUCENT_CORNER_SIZE,
						frame->win_extents.top -
						TRANSLUCENT_CORNER_SIZE,
						frame->win_extents.bottom -
						TRANSLUCENT_CORNER_SIZE,
						opt_shadow,
						&frame->window_context,
						decor_draw_simple,
						NULL);
}

void
decor_frame_update_shadow (Display		  *xdisplay,
			   Screen		  *screen,
			   decor_frame_t	  *frame,
			   decor_shadow_info_t    *info,
			   decor_shadow_options_t *opt_shadow,
			   decor_shadow_options_t *opt_no_shadow)
{
    if (frame->border_shadow)
    {
	decor_shadow_destroy (xdisplay, frame->border_shadow);
	frame->border_shadow = NULL;
    }

    frame->border_shadow = decor_shadow_create (xdisplay,
						 screen,
						 1, 1,
						 frame->win_extents.left,
						 frame->win_extents.right,
						 frame->win_extents.top + frame->titlebar_height,
						 frame->win_extents.bottom,
						 frame->win_extents.left -
						 TRANSLUCENT_CORNER_SIZE,
						 frame->win_extents.right -
						 TRANSLUCENT_CORNER_SIZE,
						 frame->win_extents.top + frame->titlebar_height -
						 TRANSLUCENT_CORNER_SIZE,
						 frame->win_extents.bottom -
						 TRANSLUCENT_CORNER_SIZE,
						 opt_shadow,
						 &frame->window_context,
						 draw_border_shape,
						 (void *) info);
    if (frame->border_no_shadow)
    {
	decor_shadow_destroy (xdisplay, frame->border_no_shadow);
	frame->border_no_shadow = NULL;
    }

    frame->border_no_shadow = decor_shadow_create (xdisplay,
					 screen,
					 1, 1,
					 frame->win_extents.left,
					 frame->win_extents.right,
					 frame->win_extents.top + frame->titlebar_height,
					 frame->win_extents.bottom,
					 frame->win_extents.left -
					 TRANSLUCENT_CORNER_SIZE,
					 frame->win_extents.right -
					 TRANSLUCENT_CORNER_SIZE,
					 frame->win_extents.top + frame->titlebar_height -
					 TRANSLUCENT_CORNER_SIZE,
					 frame->win_extents.bottom -
					 TRANSLUCENT_CORNER_SIZE,
					 opt_no_shadow,
					 &frame->window_context_no_shadow,
					 draw_border_shape,
					 0);

    /* Maximized border shadow pixmap mode */
    if (frame->max_border_shadow)
    {
	decor_shadow_destroy (xdisplay, frame->max_border_shadow);
	frame->max_border_shadow = NULL;
    }

    info->state = (WNCK_WINDOW_STATE_MAXIMIZED_HORIZONTALLY |
		   WNCK_WINDOW_STATE_MAXIMIZED_VERTICALLY);

    frame->max_border_shadow =
	decor_shadow_create (xdisplay,
			     screen,
			     1, 1,
			     frame->max_win_extents.left,
			     frame->max_win_extents.right,
			     frame->max_win_extents.top + frame->max_titlebar_height,
			     frame->max_win_extents.bottom,
			     frame->max_win_extents.left - TRANSLUCENT_CORNER_SIZE,
			     frame->max_win_extents.right - TRANSLUCENT_CORNER_SIZE,
			     frame->max_win_extents.top + frame->max_titlebar_height -
			     TRANSLUCENT_CORNER_SIZE,
			     frame->max_win_extents.bottom - TRANSLUCENT_CORNER_SIZE,
			     opt_shadow,
			     &frame->max_window_context,
			     draw_border_shape,
			     (void *) info);

    /* Enforced maximize zero shadow reparenting mode */
    if (frame->max_border_no_shadow)
    {
	decor_shadow_destroy (xdisplay, frame->max_border_shadow);
	frame->max_border_shadow = NULL;
    }

    frame->max_border_no_shadow =
	decor_shadow_create (xdisplay,
			     screen,
			     1, 1,
			     frame->max_win_extents.left,
			     frame->max_win_extents.right,
			     frame->max_win_extents.top + frame->max_titlebar_height,
			     frame->max_win_extents.bottom,
			     frame->max_win_extents.left - TRANSLUCENT_CORNER_SIZE,
			     frame->max_win_extents.right - TRANSLUCENT_CORNER_SIZE,
			     frame->max_win_extents.top + frame->max_titlebar_height -
			     TRANSLUCENT_CORNER_SIZE,
			     frame->max_win_extents.bottom - TRANSLUCENT_CORNER_SIZE,
			     opt_no_shadow,
			     &frame->max_window_context_no_shadow,
			     draw_border_shape,
			     (void *) info);
}


typedef struct _tdtd_shadow_options
{
    decor_shadow_options_t *shadow;
    decor_shadow_options_t *no_shadow;
} tdtd_shadow_options_t;

void
frame_update_shadow (decor_frame_t	    *frame,
		     decor_shadow_info_t    *info,
		     decor_shadow_options_t *opt_shadow,
		     decor_shadow_options_t *opt_no_shadow)
{
    gwd_decor_frame_ref (frame);

    (*frame->update_shadow) (gdk_x11_get_default_xdisplay (),
			     gdk_x11_screen_get_xscreen (gdk_screen_get_default ()),
			     frame, info, opt_shadow, opt_no_shadow);

    gwd_decor_frame_unref (frame);
}

void
update_frames_shadows (gpointer key,
		       gpointer value,
		       gpointer user_data)
{
    decor_frame_t	  *frame = (decor_frame_t *) value;
    tdtd_shadow_options_t *opts =  (tdtd_shadow_options_t *) user_data;

    gwd_decor_frame_ref (frame);

    decor_shadow_info_t *info = malloc (sizeof (decor_shadow_info_t));

    if (!info)
	return;

    info->frame = frame;
    info->state = 0;

    frame_update_shadow (frame, info, opts->shadow, opts->no_shadow);

    gwd_decor_frame_unref (frame);

    free (info);
    info = NULL;

}

int
update_shadow (void)
{
    decor_shadow_options_t opt_shadow;
    decor_shadow_options_t opt_no_shadow;
    tdtd_shadow_options_t  *opts;

    opts = malloc (sizeof (tdtd_shadow_options_t));

    if (!opts)
	return 0;

    opt_shadow.shadow_radius  = settings->shadow_radius;
    opt_shadow.shadow_opacity = settings->shadow_opacity;

    memcpy (opt_shadow.shadow_color, settings->shadow_color, sizeof (settings->shadow_color));

    opt_shadow.shadow_offset_x = settings->shadow_offset_x;
    opt_shadow.shadow_offset_y = settings->shadow_offset_y;

    opt_no_shadow.shadow_radius  = 0;
    opt_no_shadow.shadow_opacity = 0;

    opt_no_shadow.shadow_offset_x = 0;
    opt_no_shadow.shadow_offset_y = 0;

    opts->shadow = &opt_shadow;
    opts->no_shadow = &opt_no_shadow;

    gwd_frames_foreach (update_frames_shadows, (gpointer ) opts);

    if (opts)
	free (opts);

    return 1;
}

/*
 * update_window_decoration
 *
 * Returns: void
 * Description: The master function to update the window decoration
 * if the pixmap needs to be redrawn
 */
void
update_window_decoration (WnckWindow *win)
{
    decor_t *d = g_object_get_data (G_OBJECT (win), "decor");

    if (d->decorated)
    {
	/* force size update */
	d->context = NULL;
	d->width = d->height = 0;

	update_window_decoration_size (win);
	update_event_windows (win);
    }
}

/*
 * update_window_decoration_state
 *
 * Returns: void
 * Description: helper function to update the state of the decor_t
 */
void
update_window_decoration_state (WnckWindow *win)
{
    decor_t *d = g_object_get_data (G_OBJECT (win), "decor");

    d->state = wnck_window_get_state (win);
}

/*
 * update_window_decoration_actions
 *
 * Returns: void
 * Description: helper function to update the actions of the decor_t
 */
void
update_window_decoration_actions (WnckWindow *win)
{
    decor_t *d = g_object_get_data (G_OBJECT (win), "decor");

    d->actions = wnck_window_get_actions (win);
}


/*
 * draw_decor_list
 *
 * Returns: bool
 * Description: function to be called on g_idle_add to draw window
 * decorations when we are not doing anything
 */
static gboolean
draw_decor_list (void *data)
{
    GSList  *list;
    decor_t *d;

    draw_idle_id = 0;

    for (list = draw_list; list; list = list->next)
    {
	d = (decor_t *) list->data;
	(*d->draw) (d);
    }

    g_slist_free (draw_list);
    draw_list = NULL;

    return FALSE;
}

/*
 * queue_decor_draw
 *
 * Description :queue a redraw request for this decoration. Since this function
 * only gets called on idle, don't redraw window decorations multiple
 * times if they are already waiting to be drawn (since the drawn copy
 * will always be the most updated one)
 */
void
queue_decor_draw (decor_t *d)
{
    if (g_slist_find (draw_list, d))
	return;

    draw_list = g_slist_append (draw_list, d);

    if (!draw_idle_id)
	draw_idle_id = g_idle_add (draw_decor_list, NULL);
}

/*
 * update_default_decorations
 *
 * Description: update the default decorations
 */
void
update_default_decorations (GdkScreen *screen)
{
    long	    data[256];
    Window	    xroot;
    GdkDisplay	    *gdkdisplay = gdk_display_get_default ();
    Display	    *xdisplay = gdk_x11_display_get_xdisplay (gdkdisplay);
    Atom	    bareAtom, normalAtom, activeAtom;
    decor_t	    d;
    gint	    nQuad;
    decor_quad_t    quads[N_QUADS_MAX];
    decor_frame_t   *frame;
    decor_frame_t   *bare_frame = gwd_get_decor_frame ("bare");
    decor_extents_t extents;

    xroot = RootWindowOfScreen (gdk_x11_screen_get_xscreen (screen));

    bareAtom   = XInternAtom (xdisplay, DECOR_BARE_ATOM_NAME, FALSE);
    normalAtom = XInternAtom (xdisplay, DECOR_NORMAL_ATOM_NAME, FALSE);
    activeAtom = XInternAtom (xdisplay, DECOR_ACTIVE_ATOM_NAME, FALSE);

    if (bare_frame->border_shadow)
    {
	decor_layout_t layout;

	decor_get_default_layout (&bare_frame->window_context, 1, 1, &layout);

	nQuad = decor_set_lSrStSbS_window_quads (quads, &bare_frame->window_context,
						 &layout);

	decor_quads_to_property (data, bare_frame->border_shadow->pixmap,
				 &bare_frame->win_extents, &bare_frame->win_extents,
				 &bare_frame->win_extents, &bare_frame->win_extents,
				 0, 0, quads, nQuad);

	XChangeProperty (xdisplay, xroot,
			 bareAtom,
			 XA_INTEGER,
			 32, PropModeReplace, (guchar *) data,
			 BASE_PROP_SIZE + QUAD_PROP_SIZE * nQuad);

	if (minimal)
	{
	    XChangeProperty (xdisplay, xroot,
			     normalAtom,
			     XA_INTEGER,
			     32, PropModeReplace, (guchar *) data,
			     BASE_PROP_SIZE + QUAD_PROP_SIZE * nQuad);
	    XChangeProperty (xdisplay, xroot,
			     activeAtom,
			     XA_INTEGER,
			     32, PropModeReplace, (guchar *) data,
			     BASE_PROP_SIZE + QUAD_PROP_SIZE * nQuad);
	}
    }
    else
    {
	XDeleteProperty (xdisplay, xroot, bareAtom);

	if (minimal)
	{
	    XDeleteProperty (xdisplay, xroot, normalAtom);
	    XDeleteProperty (xdisplay, xroot, activeAtom);
	}
    }

    if (minimal)
    {
	gwd_decor_frame_unref (bare_frame);
	return;
    }

    memset (&d, 0, sizeof (d));

    frame = gwd_get_decor_frame ("default");
    extents = frame->win_extents;

    d.context = &frame->window_context;
    d.shadow  = frame->border_shadow;
    d.layout  = pango_layout_new (frame->pango_context);

    decor_get_default_layout (d.context, 1, 1, &d.border_layout);

    d.width  = d.border_layout.width;
    d.height = d.border_layout.height;

    d.frame = frame;

    extents.top += frame->titlebar_height;

    d.draw = theme_draw_window_decoration;

    if (decor_normal_pixmap)
	g_object_unref (G_OBJECT (decor_normal_pixmap));

    nQuad = decor_set_lSrStSbS_window_quads (quads, d.context,
					     &d.border_layout);

    decor_normal_pixmap = create_pixmap (d.width, d.height, frame->style_window_rgba);

    if (decor_normal_pixmap)
    {
	d.pixmap  = decor_normal_pixmap;
	d.active  = FALSE;
	d.picture = XRenderCreatePicture (xdisplay,
					  GDK_PIXMAP_XID (d.pixmap),
					  xformat_rgba, 0, NULL);

	(*d.draw) (&d);

	XRenderFreePicture (xdisplay, d.picture);

	decor_quads_to_property (data, GDK_PIXMAP_XID (d.pixmap),
				 &extents, &extents,
				 &extents, &extents, 0, 0, quads, nQuad);

	XChangeProperty (xdisplay, xroot,
			 normalAtom,
			 XA_INTEGER,
			 32, PropModeReplace, (guchar *) data,
			 BASE_PROP_SIZE + QUAD_PROP_SIZE * nQuad);
    }

    if (decor_active_pixmap)
	g_object_unref (G_OBJECT (decor_active_pixmap));

    decor_active_pixmap = create_pixmap (d.width, d.height, frame->style_window_rgba);

    if (decor_active_pixmap)
    {
	d.pixmap  = decor_active_pixmap;
	d.active  = TRUE;
	d.picture = XRenderCreatePicture (xdisplay,
					  GDK_PIXMAP_XID (d.pixmap),
					  xformat_rgba, 0, NULL);

	(*d.draw) (&d);

	XRenderFreePicture (xdisplay, d.picture);

	decor_quads_to_property (data, GDK_PIXMAP_XID (d.pixmap),
				 &extents, &extents,
				 &extents, &extents, 0, 0, quads, nQuad);

	XChangeProperty (xdisplay, xroot,
			 activeAtom,
			 XA_INTEGER,
			 32, PropModeReplace, (guchar *) data,
			 BASE_PROP_SIZE + QUAD_PROP_SIZE * nQuad);
    }

    gwd_decor_frame_unref (frame);
    gwd_decor_frame_unref (bare_frame);

    if (d.layout)
	g_object_unref (G_OBJECT (d.layout));
}

/*
 * copy_to_front_buffer
 *
 * Description: Helper function to copy the buffer pixmap to a front buffer
 */
void
copy_to_front_buffer (decor_t *d)
{
    if (!d->buffer_pixmap)
	return;

    cairo_set_operator (d->cr, CAIRO_OPERATOR_SOURCE);
    gdk_cairo_set_source_pixmap (d->cr, d->buffer_pixmap, 0, 0);
    cairo_paint (d->cr);
}
