#include "gtk-window-decorator.h"

static void
draw_switcher_background (decor_t *d)
{
    Display	  *xdisplay = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
    cairo_t	  *cr;
    GtkStyle	  *style;
    decor_color_t color;
    double	  alpha = SWITCHER_ALPHA / 65535.0;
    double	  x1, y1, x2, y2, h;
    int		  top;
    unsigned long pixel;
    ushort	  a = SWITCHER_ALPHA;

    if (!d->buffer_pixmap)
	return;

    style = gtk_widget_get_style (style_window_rgba);

    color.r = style->bg[GTK_STATE_NORMAL].red   / 65535.0;
    color.g = style->bg[GTK_STATE_NORMAL].green / 65535.0;
    color.b = style->bg[GTK_STATE_NORMAL].blue  / 65535.0;

    cr = gdk_cairo_create (GDK_DRAWABLE (d->buffer_pixmap));

    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

    top = _switcher_extents.top;

    x1 = switcher_context.left_space - _switcher_extents.left;
    y1 = switcher_context.top_space - _switcher_extents.top;
    x2 = d->width - switcher_context.right_space + _switcher_extents.right;
    y2 = d->height - switcher_context.bottom_space + _switcher_extents.bottom;

    h = y2 - y1 - _switcher_extents.top - _switcher_extents.top;

    cairo_set_line_width (cr, 1.0);

    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

    draw_shadow_background (d, cr, switcher_shadow, &switcher_context);

    fill_rounded_rectangle (cr,
			    x1 + 0.5,
			    y1 + 0.5,
			    _switcher_extents.left - 0.5,
			    top - 0.5,
			    5.0, CORNER_TOPLEFT,
			    &color, alpha, &color, alpha * 0.75,
			    SHADE_TOP | SHADE_LEFT);

    fill_rounded_rectangle (cr,
			    x1 + _switcher_extents.left,
			    y1 + 0.5,
			    x2 - x1 - _switcher_extents.left -
			    _switcher_extents.right,
			    top - 0.5,
			    5.0, 0,
			    &color, alpha, &color, alpha * 0.75,
			    SHADE_TOP);

    fill_rounded_rectangle (cr,
			    x2 - _switcher_extents.right,
			    y1 + 0.5,
			    _switcher_extents.right - 0.5,
			    top - 0.5,
			    5.0, CORNER_TOPRIGHT,
			    &color, alpha, &color, alpha * 0.75,
			    SHADE_TOP | SHADE_RIGHT);

    fill_rounded_rectangle (cr,
			    x1 + 0.5,
			    y1 + top,
			    _switcher_extents.left - 0.5,
			    h,
			    5.0, 0,
			    &color, alpha, &color, alpha * 0.75,
			    SHADE_LEFT);

    fill_rounded_rectangle (cr,
			    x2 - _switcher_extents.right,
			    y1 + top,
			    _switcher_extents.right - 0.5,
			    h,
			    5.0, 0,
			    &color, alpha, &color, alpha * 0.75,
			    SHADE_RIGHT);

    fill_rounded_rectangle (cr,
			    x1 + 0.5,
			    y2 - _switcher_extents.top,
			    _switcher_extents.left - 0.5,
			    _switcher_extents.top - 0.5,
			    5.0, CORNER_BOTTOMLEFT,
			    &color, alpha, &color, alpha * 0.75,
			    SHADE_BOTTOM | SHADE_LEFT);

    fill_rounded_rectangle (cr,
			    x1 + _switcher_extents.left,
			    y2 - _switcher_extents.top,
			    x2 - x1 - _switcher_extents.left -
			    _switcher_extents.right,
			    _switcher_extents.top - 0.5,
			    5.0, 0,
			    &color, alpha, &color, alpha * 0.75,
			    SHADE_BOTTOM);

    fill_rounded_rectangle (cr,
			    x2 - _switcher_extents.right,
			    y2 - _switcher_extents.top,
			    _switcher_extents.right - 0.5,
			    _switcher_extents.top - 0.5,
			    5.0, CORNER_BOTTOMRIGHT,
			    &color, alpha, &color, alpha * 0.75,
			    SHADE_BOTTOM | SHADE_RIGHT);

    cairo_rectangle (cr, x1 + _switcher_extents.left,
		     y1 + top,
		     x2 - x1 - _switcher_extents.left - _switcher_extents.right,
		     h);
    gdk_cairo_set_source_color_alpha (cr,
				      &style->bg[GTK_STATE_NORMAL],
				      alpha);
    cairo_fill (cr);

    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

    rounded_rectangle (cr,
		       x1 + 0.5, y1 + 0.5,
		       x2 - x1 - 1.0, y2 - y1 - 1.0,
		       5.0,
		       CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
		       CORNER_BOTTOMRIGHT);

    cairo_clip (cr);

    cairo_translate (cr, 1.0, 1.0);

    rounded_rectangle (cr,
		       x1 + 0.5, y1 + 0.5,
		       x2 - x1 - 1.0, y2 - y1 - 1.0,
		       5.0,
		       CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
		       CORNER_BOTTOMRIGHT);

    cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.4);
    cairo_stroke (cr);

    cairo_translate (cr, -2.0, -2.0);

    rounded_rectangle (cr,
		       x1 + 0.5, y1 + 0.5,
		       x2 - x1 - 1.0, y2 - y1 - 1.0,
		       5.0,
		       CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
		       CORNER_BOTTOMRIGHT);

    cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.1);
    cairo_stroke (cr);

    cairo_translate (cr, 1.0, 1.0);

    cairo_reset_clip (cr);

    rounded_rectangle (cr,
		       x1 + 0.5, y1 + 0.5,
		       x2 - x1 - 1.0, y2 - y1 - 1.0,
		       5.0,
		       CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
		       CORNER_BOTTOMRIGHT);

    gdk_cairo_set_source_color_alpha (cr,
				      &style->fg[GTK_STATE_NORMAL],
				      alpha);

    cairo_stroke (cr);

    cairo_destroy (cr);

    gdk_draw_drawable (d->pixmap,
		       d->gc,
		       d->buffer_pixmap,
		       0,
		       0,
		       0,
		       0,
		       d->width,
		       d->height);

    pixel = ((((a * style->bg[GTK_STATE_NORMAL].blue ) >> 24) & 0x0000ff) |
	     (((a * style->bg[GTK_STATE_NORMAL].green) >> 16) & 0x00ff00) |
	     (((a * style->bg[GTK_STATE_NORMAL].red  ) >>  8) & 0xff0000) |
	     (((a & 0xff00) << 16)));

    decor_update_switcher_property (d);

    gdk_error_trap_push ();
    XSetWindowBackground (xdisplay, d->prop_xid, pixel);
    XClearWindow (xdisplay, d->prop_xid);

    gdk_display_sync (gdk_display_get_default ());
    gdk_error_trap_pop ();

    d->prop_xid = 0;
}

static void
draw_switcher_foreground (decor_t *d)
{
    cairo_t	  *cr;
    GtkStyle	  *style;
    double	  alpha = SWITCHER_ALPHA / 65535.0;

    if (!d->pixmap || !d->buffer_pixmap)
	return;

    style = gtk_widget_get_style (style_window_rgba);

    cr = gdk_cairo_create (GDK_DRAWABLE (d->buffer_pixmap));

    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

    cairo_rectangle (cr, switcher_context.left_space,
		     d->height - switcher_context.bottom_space,
		     d->width - switcher_context.left_space -
		     switcher_context.right_space,
		     SWITCHER_SPACE);

    gdk_cairo_set_source_color_alpha (cr,
				      &style->bg[GTK_STATE_NORMAL],
				      alpha);
    cairo_fill (cr);

    if (d->layout)
    {
	int w;

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	gdk_cairo_set_source_color_alpha (cr,
					  &style->fg[GTK_STATE_NORMAL],
					  1.0);

	pango_layout_get_pixel_size (d->layout, &w, NULL);

	cairo_move_to (cr, d->width / 2 - w / 2,
		       d->height - switcher_context.bottom_space +
		       SWITCHER_SPACE / 2 - text_height / 2);

	pango_cairo_show_layout (cr, d->layout);
    }

    cairo_destroy (cr);

    gdk_draw_drawable  (d->pixmap,
			d->gc,
			d->buffer_pixmap,
			0,
			0,
			0,
			0,
			d->width,
			d->height);
}

void
draw_switcher_decoration (decor_t *d)
{
    if (d->prop_xid)
	draw_switcher_background (d);

    draw_switcher_foreground (d);
}

gboolean
update_switcher_window (WnckWindow *win,
			Window     selected)
{
    decor_t           *d = g_object_get_data (G_OBJECT (win), "decor");
    GdkPixmap         *pixmap, *buffer_pixmap = NULL;
    gint              height, width = 0;
    WnckWindow        *selected_win;
    Display           *xdisplay;
    XRenderPictFormat *format;

    xdisplay = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());

    wnck_window_get_client_window_geometry (win, NULL, NULL, &width, NULL);

    decor_get_default_layout (&switcher_context, width, 1, &d->border_layout);

    width  = d->border_layout.width;
    height = d->border_layout.height;

    d->decorated = FALSE;
    d->draw	 = draw_switcher_decoration;

    if (!d->pixmap && switcher_pixmap)
    {
	g_object_ref (G_OBJECT (switcher_pixmap));

	d->pixmap = switcher_pixmap;
    }

    if (!d->buffer_pixmap && switcher_buffer_pixmap)
    {
	g_object_ref (G_OBJECT (switcher_buffer_pixmap));
	d->buffer_pixmap = switcher_buffer_pixmap;
    }

    if (!d->width)
	d->width = switcher_width;

    if (!d->height)
	d->height = switcher_height;

    selected_win = wnck_window_get (selected);
    if (selected_win)
    {
	glong		name_length;
	PangoLayoutLine *line;
	const gchar	*name;

	if (d->name)
	{
	    g_free (d->name);
	    d->name = NULL;
	}

	name = wnck_window_get_name (selected_win);
	if (name && (name_length = strlen (name)))
	{
	    if (!d->layout)
	    {
		d->layout = pango_layout_new (pango_context);
		if (d->layout)
		    pango_layout_set_wrap (d->layout, PANGO_WRAP_CHAR);
	    }

	    if (d->layout)
	    {
		int tw;

		tw = width - switcher_context.left_space -
		    switcher_context.right_space - 64;
		pango_layout_set_auto_dir (d->layout, FALSE);
		pango_layout_set_width (d->layout, tw * PANGO_SCALE);
		pango_layout_set_text (d->layout, name, name_length);

		line = pango_layout_get_line (d->layout, 0);

		name_length = line->length;
		if (pango_layout_get_line_count (d->layout) > 1)
		{
		    if (name_length < 4)
		    {
			g_object_unref (G_OBJECT (d->layout));
			d->layout = NULL;
		    }
		    else
		    {
			d->name = g_strndup (name, name_length);
			strcpy (d->name + name_length - 3, "...");
		    }
		}
		else
		    d->name = g_strndup (name, name_length);

		if (d->layout)
		    pango_layout_set_text (d->layout, d->name, name_length);
	    }
	}
	else if (d->layout)
	{
	    g_object_unref (G_OBJECT (d->layout));
	    d->layout = NULL;
	}
    }

    if (selected != switcher_selected_window)
    {
	gtk_label_set_text (GTK_LABEL (switcher_label), "");
	if (selected_win && d->name)
	    gtk_label_set_text (GTK_LABEL (switcher_label), d->name);
	switcher_selected_window = selected;
    }

    if (width == d->width && height == d->height)
    {
	if (!d->gc)
	    d->gc = gdk_gc_new (d->pixmap);

	if (!d->picture)
	{
	    XRenderPictFormat *format;

	    format = get_format_for_drawable (d,
					      GDK_DRAWABLE (d->buffer_pixmap));
	    d->picture = XRenderCreatePicture (xdisplay,
					       GDK_PIXMAP_XID (buffer_pixmap),
					       format, 0, NULL);
	}
	queue_decor_draw (d);
	return FALSE;
    }

    pixmap = create_pixmap (width, height, 32);
    if (!pixmap)
	return FALSE;

    buffer_pixmap = create_pixmap (width, height, 32);
    if (!buffer_pixmap)
    {
	g_object_unref (G_OBJECT (pixmap));
	return FALSE;
    }

    if (switcher_pixmap)
	g_object_unref (G_OBJECT (switcher_pixmap));

    if (switcher_buffer_pixmap)
	g_object_unref (G_OBJECT (switcher_buffer_pixmap));

    if (d->pixmap)
	g_object_unref (G_OBJECT (d->pixmap));

    if (d->buffer_pixmap)
	g_object_unref (G_OBJECT (d->buffer_pixmap));

    if (d->gc)
	g_object_unref (G_OBJECT (d->gc));

    if (d->picture)
	XRenderFreePicture (xdisplay, d->picture);

    switcher_pixmap	   = pixmap;
    switcher_buffer_pixmap = buffer_pixmap;

    switcher_width  = width;
    switcher_height = height;

    g_object_ref (G_OBJECT (pixmap));
    g_object_ref (G_OBJECT (buffer_pixmap));

    d->pixmap	     = pixmap;
    d->buffer_pixmap = buffer_pixmap;
    d->gc	     = gdk_gc_new (pixmap);

    format = get_format_for_drawable (d, GDK_DRAWABLE (d->buffer_pixmap));
    d->picture = XRenderCreatePicture (xdisplay, GDK_PIXMAP_XID (buffer_pixmap),
				       format, 0, NULL);

    d->width  = width;
    d->height = height;

    d->prop_xid = wnck_window_get_xid (win);

    queue_decor_draw (d);

    return TRUE;
}