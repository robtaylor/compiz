#include "gtk-window-decorator.h"

static const PangoFontDescription *
get_titlebar_font (void)
{
    if (use_system_font)
    {
	return NULL;
    }
    else
	return titlebar_font;
}

void
update_titlebar_font (void)
{
    const PangoFontDescription *font_desc;
    PangoFontMetrics	       *metrics;
    PangoLanguage	       *lang;

    font_desc = get_titlebar_font ();
    if (!font_desc)
    {
	GtkStyle *default_style;

	default_style = gtk_widget_get_default_style ();
	font_desc = default_style->font_desc;
    }

    pango_context_set_font_description (pango_context, font_desc);

    lang    = pango_context_get_language (pango_context);
    metrics = pango_context_get_metrics (pango_context, font_desc, lang);

    text_height = PANGO_PIXELS (pango_font_metrics_get_ascent (metrics) +
				pango_font_metrics_get_descent (metrics));

    pango_font_metrics_unref (metrics);
}

void
update_event_windows (WnckWindow *win)
{
    Display *xdisplay;
    decor_t *d = g_object_get_data (G_OBJECT (win), "decor");
    gint    x0, y0, width, height, x, y, w, h;
    gint    i, j, k, l;
    gint    actions = d->actions;

    xdisplay = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());

    wnck_window_get_client_window_geometry (win, &x0, &y0, &width, &height);

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

	    if (d->frame_window)
	    {
		BoxPtr box = &d->event_windows[i][j].pos;
		box->x1  = x;
		box->x2 = x + w;
		box->y1 = y;
		box->y2 = y + h;
	    }
	    else if (!d->frame_window && w != 0 && h != 0)
	    {
		XMapWindow (xdisplay, d->event_windows[i][j].window);
		XMoveResizeWindow (xdisplay, d->event_windows[i][j].window,
				   x, y, w, h);
	    }
	    else if (!d->frame_window)
	    {
		XUnmapWindow (xdisplay, d->event_windows[i][j].window);
	    }
	}
    }

    /* no button event windows if width is less than minimum width */
    if (width < ICON_SPACE + d->button_width)
	actions = 0;

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

	if (d->frame_window &&
	    button_actions[i] && !(actions & button_actions[i]))
	{
	    memset (&d->button_windows[i].pos, 0, sizeof (Box));
	}
	else if (!d->frame_window &&
		 button_actions[i] && !(actions & button_actions[i]))
	{
	    XUnmapWindow (xdisplay, d->button_windows[i].window);
	    continue;
	}

	if (d->frame_window &&
	    (*theme_get_button_position) (d, i, width, height, &x, &y, &w, &h))
	{
	    BoxPtr box = &d->button_windows[i].pos;
	    box->x1 = x;
	    box->y1 = y;
	    box->x2 = x + w;
	    box->y2 = y + h;
	}
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

#ifdef HAVE_WNCK_WINDOW_HAS_NAME
static const char *
wnck_window_get_real_name (WnckWindow *win)
{
    return wnck_window_has_name (win) ? wnck_window_get_name (win) : NULL;
}
#define wnck_window_get_name wnck_window_get_real_name
#endif

gint
max_window_name_width (WnckWindow *win)
{
    decor_t     *d = g_object_get_data (G_OBJECT (win), "decor");
    const gchar *name;
    gint	w;

    if (!d->layout)
    {
	d->layout = pango_layout_new (pango_context);
	if (!d->layout)
	    return 0;

	pango_layout_set_wrap (d->layout, PANGO_WRAP_CHAR);
    }

    name = wnck_window_get_name (win);
    if (!name)
	return 0;

    pango_layout_set_auto_dir (d->layout, FALSE);
    pango_layout_set_width (d->layout, -1);
    pango_layout_set_text (d->layout, name, strlen (name));
    pango_layout_get_pixel_size (d->layout, &w, NULL);

    if (d->name)
	pango_layout_set_text (d->layout, d->name, strlen (d->name));

    return w + 6;
}

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

    name = wnck_window_get_name (win);
    if (name && (name_length = strlen (name)))
    {
	gint w;

	if (theme_draw_window_decoration != draw_window_decoration)
	{
	    w = SHRT_MAX;
	}
	else
	{
	    gint width;

	    wnck_window_get_client_window_geometry (win, NULL, NULL,
						    &width, NULL);

	    w = width - ICON_SPACE - 2 - d->button_width;
	    if (w < 1)
		w = 1;
	}

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

	pango_layout_set_text (d->layout, d->name, name_length);
    }
}

void
update_window_decoration_icon (WnckWindow *win)
{
    decor_t *d = g_object_get_data (G_OBJECT (win), "decor");

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

    d->icon_pixbuf = wnck_window_get_mini_icon (win);
    if (d->icon_pixbuf)
    {
	cairo_t	*cr;

	g_object_ref (G_OBJECT (d->icon_pixbuf));

	if (d->frame_window)
	    d->icon_pixmap = pixmap_new_from_pixbuf (d->icon_pixbuf,
						     24);
	else
	    d->icon_pixmap = pixmap_new_from_pixbuf (d->icon_pixbuf,
						     32);
	cr = gdk_cairo_create (GDK_DRAWABLE (d->icon_pixmap));
	d->icon = cairo_pattern_create_for_surface (cairo_get_target (cr));
	cairo_destroy (cr);
    }
}



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
    int               depth;

    xdisplay = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());

    wnck_window_get_client_window_geometry (win, &x, &y, &w, &h);

    name_width = max_window_name_width (win);

    if (!(*theme_calc_decoration_size) (d, w, h, name_width, &width, &height))
    {
	update_window_decoration_name (win);
	return FALSE;
    }

    gdk_error_trap_push ();

    if (d->frame_window)
	depth = gdk_drawable_get_depth (GDK_DRAWABLE (d->frame_window));
    else
	depth = 32;

    pixmap = create_pixmap (width, height, depth);

    gdk_flush ();

    if (!pixmap || gdk_error_trap_pop ())
    {
	memset (pixmap, 0, sizeof (pixmap));
	return FALSE;
    }

    gdk_error_trap_push ();

    buffer_pixmap = create_pixmap (width, height, depth);

    gdk_flush ();

    if (!buffer_pixmap || gdk_error_trap_pop ())
    {
	memset (buffer_pixmap, 0, sizeof (buffer_pixmap));
	g_object_unref (G_OBJECT (pixmap));
	return FALSE;
    }

    format = get_format_for_drawable (d, GDK_DRAWABLE (buffer_pixmap));
    picture = XRenderCreatePicture (xdisplay, GDK_PIXMAP_XID (buffer_pixmap),
				    format, 0, NULL);

    if (d->pixmap)
	g_object_unref (G_OBJECT (d->pixmap));

    if (d->buffer_pixmap)
	g_object_unref (G_OBJECT (d->buffer_pixmap));

    if (d->picture)
	XRenderFreePicture (xdisplay, d->picture);

    if (d->cr)
	cairo_destroy (d->cr);

    d->pixmap	     = pixmap;
    d->buffer_pixmap = buffer_pixmap;
    d->cr	     = gdk_cairo_create (pixmap);

    d->picture = picture;

    d->width  = width;
    d->height = height;

    d->prop_xid = wnck_window_get_xid (win);

    update_window_decoration_name (win);

    queue_decor_draw (d);

    return TRUE;
}

/* to save some memory, value is specific to current decorations */
#define TRANSLUCENT_CORNER_SIZE 3

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
    double		save_decoration_alpha;

    memset (&d, 0, sizeof (d));

    d.pixmap  = gdk_pixmap_foreign_new_for_display (gdk_display_get_default (),
						    pixmap);
    d.width   = width;
    d.height  = height;
    d.active  = TRUE;
    d.draw    = theme_draw_window_decoration;
    d.picture = picture;
    d.context = c;

    /* we use closure argument if maximized */
    if (closure)
	d.state |=
	    WNCK_WINDOW_STATE_MAXIMIZED_HORIZONTALLY |
	    WNCK_WINDOW_STATE_MAXIMIZED_VERTICALLY;

    decor_get_default_layout (c, 1, 1, &d.border_layout);

    colormap = get_colormap_for_drawable (GDK_DRAWABLE (d.pixmap));
    gdk_drawable_set_colormap (d.pixmap, colormap);

    /* create shadow from opaque decoration */
    save_decoration_alpha = decoration_alpha;
    decoration_alpha = 1.0;

    (*d.draw) (&d);

    decoration_alpha = save_decoration_alpha;

    XRenderFillRectangle (xdisplay, PictOpSrc, picture, &white,
			  c->left_space,
			  c->top_space,
			  width - c->left_space - c->right_space,
			  height - c->top_space - c->bottom_space);

    g_object_unref (G_OBJECT (d.pixmap));
}

int
update_shadow (void)
{
    decor_shadow_options_t opt_shadow;
    decor_shadow_options_t opt_no_shadow;
    Display		   *xdisplay = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
    GdkDisplay		   *display = gdk_display_get_default ();
    GdkScreen		   *screen = gdk_display_get_default_screen (display);

    opt_shadow.shadow_radius  = shadow_radius;
    opt_shadow.shadow_opacity = shadow_opacity;

    memcpy (opt_shadow.shadow_color, shadow_color, sizeof (shadow_color));

    opt_shadow.shadow_offset_x = shadow_offset_x;
    opt_shadow.shadow_offset_y = shadow_offset_y;

    opt_no_shadow.shadow_radius  = 0;
    opt_no_shadow.shadow_opacity = 0;

    opt_no_shadow.shadow_offset_x = 0;
    opt_no_shadow.shadow_offset_y = 0;

    if (no_border_shadow)
    {
	decor_shadow_destroy (xdisplay, no_border_shadow);
	no_border_shadow = NULL;
    }

    no_border_shadow = decor_shadow_create (xdisplay,
					    gdk_x11_screen_get_xscreen (screen),
					    1, 1,
					    0,
					    0,
					    0,
					    0,
					    0, 0, 0, 0,
					    &opt_shadow,
					    &shadow_context,
					    decor_draw_simple,
					    0);

    if (border_shadow)
    {
	decor_shadow_destroy (xdisplay, border_shadow);
	border_shadow = NULL;
    }

    border_shadow = decor_shadow_create (xdisplay,
					 gdk_x11_screen_get_xscreen (screen),
					 1, 1,
					 _win_extents.left,
					 _win_extents.right,
					 _win_extents.top + titlebar_height,
					 _win_extents.bottom,
					 _win_extents.left -
					 TRANSLUCENT_CORNER_SIZE,
					 _win_extents.right -
					 TRANSLUCENT_CORNER_SIZE,
					 _win_extents.top + titlebar_height -
					 TRANSLUCENT_CORNER_SIZE,
					 _win_extents.bottom -
					 TRANSLUCENT_CORNER_SIZE,
					 &opt_shadow,
					 &window_context,
					 draw_border_shape,
					 0);

    if (border_no_shadow)
    {
	decor_shadow_destroy (xdisplay, border_no_shadow);
	border_no_shadow = NULL;
    }

    border_no_shadow = decor_shadow_create (xdisplay,
					 gdk_x11_screen_get_xscreen (screen),
					 1, 1,
					 _win_extents.left,
					 _win_extents.right,
					 _win_extents.top + titlebar_height,
					 _win_extents.bottom,
					 _win_extents.left -
					 TRANSLUCENT_CORNER_SIZE,
					 _win_extents.right -
					 TRANSLUCENT_CORNER_SIZE,
					 _win_extents.top + titlebar_height -
					 TRANSLUCENT_CORNER_SIZE,
					 _win_extents.bottom -
					 TRANSLUCENT_CORNER_SIZE,
					 &opt_no_shadow,
					 &window_context_no_shadow,
					 draw_border_shape,
					 0);

    decor_context_t *context = &window_context_no_shadow;


    if (max_border_shadow)
    {
	decor_shadow_destroy (xdisplay, max_border_shadow);
	max_border_shadow = NULL;
    }

    max_border_shadow =
	decor_shadow_create (xdisplay,
			     gdk_x11_screen_get_xscreen (screen),
			     1, 1,
			     _max_win_extents.left,
			     _max_win_extents.right,
			     _max_win_extents.top + max_titlebar_height,
			     _max_win_extents.bottom,
			     _max_win_extents.left - TRANSLUCENT_CORNER_SIZE,
			     _max_win_extents.right - TRANSLUCENT_CORNER_SIZE,
			     _max_win_extents.top + max_titlebar_height -
			     TRANSLUCENT_CORNER_SIZE,
			     _max_win_extents.bottom - TRANSLUCENT_CORNER_SIZE,
			     &opt_shadow,
			     &max_window_context,
			     draw_border_shape,
			     (void *) 1);

    if (max_border_no_shadow)
    {
	decor_shadow_destroy (xdisplay, max_border_shadow);
	max_border_shadow = NULL;
    }

    max_border_no_shadow =
	decor_shadow_create (xdisplay,
			     gdk_x11_screen_get_xscreen (screen),
			     1, 1,
			     _max_win_extents.left,
			     _max_win_extents.right,
			     _max_win_extents.top + max_titlebar_height,
			     _max_win_extents.bottom,
			     _max_win_extents.left - TRANSLUCENT_CORNER_SIZE,
			     _max_win_extents.right - TRANSLUCENT_CORNER_SIZE,
			     _max_win_extents.top + max_titlebar_height -
			     TRANSLUCENT_CORNER_SIZE,
			     _max_win_extents.bottom - TRANSLUCENT_CORNER_SIZE,
			     &opt_no_shadow,
			     &max_window_context_no_shadow,
			     draw_border_shape,
			     (void *) 1);

    if (switcher_shadow)
    {
	decor_shadow_destroy (xdisplay, switcher_shadow);
	switcher_shadow = NULL;
    }

    switcher_shadow = decor_shadow_create (xdisplay,
					   gdk_x11_screen_get_xscreen (screen),
					   1, 1,
					   _switcher_extents.left,
					   _switcher_extents.right,
					   _switcher_extents.top,
					   _switcher_extents.bottom,
					   _switcher_extents.left -
					   TRANSLUCENT_CORNER_SIZE,
					   _switcher_extents.right -
					   TRANSLUCENT_CORNER_SIZE,
					   _switcher_extents.top -
					   TRANSLUCENT_CORNER_SIZE,
					   _switcher_extents.bottom -
					   TRANSLUCENT_CORNER_SIZE,
					   &opt_shadow,
					   &switcher_context,
					   decor_draw_simple,
					   0);

    return 1;
}

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
    else
    {
	Window xid = wnck_window_get_xid (win);
	Window select;

	if (get_window_prop (xid, select_window_atom, &select))
	{
	    /* force size update */
	    d->context = NULL;
	    d->width = d->height = 0;
	    switcher_width = switcher_height = 0;

	    update_switcher_window (win, select);
	}
    }
}

void
update_window_decoration_state (WnckWindow *win)
{
    decor_t *d = g_object_get_data (G_OBJECT (win), "decor");

    d->state = wnck_window_get_state (win);
}

void
update_window_decoration_actions (WnckWindow *win)
{
    decor_t *d = g_object_get_data (G_OBJECT (win), "decor");

    d->actions = wnck_window_get_actions (win);
}

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

void
queue_decor_draw (decor_t *d)
{
    if (g_slist_find (draw_list, d))
	return;

    draw_list = g_slist_append (draw_list, d);

    if (!draw_idle_id)
	draw_idle_id = g_idle_add (draw_decor_list, NULL);
}

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
    decor_extents_t extents = _win_extents;

    xroot = RootWindowOfScreen (gdk_x11_screen_get_xscreen (screen));

    bareAtom   = XInternAtom (xdisplay, DECOR_BARE_ATOM_NAME, FALSE);
    normalAtom = XInternAtom (xdisplay, DECOR_NORMAL_ATOM_NAME, FALSE);
    activeAtom = XInternAtom (xdisplay, DECOR_ACTIVE_ATOM_NAME, FALSE);

    if (no_border_shadow)
    {
	decor_layout_t layout;

	decor_get_default_layout (&shadow_context, 1, 1, &layout);

	nQuad = decor_set_lSrStSbS_window_quads (quads, &shadow_context,
						 &layout);

	decor_quads_to_property (data, no_border_shadow->pixmap,
				 &_shadow_extents, &_shadow_extents,
				 &_shadow_extents, &_shadow_extents,
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
	return;

    memset (&d, 0, sizeof (d));

    d.context = &window_context;
    d.shadow  = border_shadow;
    d.layout  = pango_layout_new (pango_context);

    decor_get_default_layout (d.context, 1, 1, &d.border_layout);

    d.width  = d.border_layout.width;
    d.height = d.border_layout.height;

    extents.top += titlebar_height;

    d.draw = theme_draw_window_decoration;

    if (decor_normal_pixmap)
	g_object_unref (G_OBJECT (decor_normal_pixmap));

    nQuad = decor_set_lSrStSbS_window_quads (quads, d.context,
					     &d.border_layout);

    decor_normal_pixmap = create_pixmap (d.width, d.height, 32);

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

    decor_active_pixmap = create_pixmap (d.width, d.height, 32);

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

    if (d.layout)
	g_object_unref (G_OBJECT (d.layout));
}

void
copy_to_front_buffer (decor_t *d)
{
    if (!d->buffer_pixmap)
	return;

    cairo_set_operator (d->cr, CAIRO_OPERATOR_SOURCE);
    gdk_cairo_set_source_pixmap (d->cr, d->buffer_pixmap, 0, 0);
    cairo_paint (d->cr);
}
