#include "gtk-window-decorator.h"

GdkPixmap *
pixmap_new_from_pixbuf (GdkPixbuf *pixbuf, int depth)
{
    GdkPixmap *pixmap;
    guint     width, height;
    cairo_t   *cr;

    width  = gdk_pixbuf_get_width (pixbuf);
    height = gdk_pixbuf_get_height (pixbuf);

    pixmap = create_pixmap (width, height, depth);
    if (!pixmap)
	return NULL;

    cr = (cairo_t *) gdk_cairo_create (GDK_DRAWABLE (pixmap));
    gdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);
    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint (cr);
    cairo_destroy (cr);

    return pixmap;
}


void
gdk_cairo_set_source_color_alpha (cairo_t  *cr,
				  GdkColor *color,
				  double   alpha)
{
    cairo_set_source_rgba (cr,
			   color->red   / 65535.0,
			   color->green / 65535.0,
			   color->blue  / 65535.0,
			   alpha);
}

inline GdkWindow *
create_gdk_window (Window xframe)
{
    GdkDisplay  *display = gdk_display_get_default ();
    GdkScreen   *screen  = gdk_display_get_default_screen (display);
    GdkWindow   *window  = gdk_window_foreign_new (xframe);
    GdkColormap *cmap    = gdk_screen_get_rgb_colormap (screen);

    gdk_drawable_set_colormap (GDK_DRAWABLE (window), cmap);

    return window;
}

GdkColormap *
get_colormap_for_drawable (GdkDrawable *d)
{
    GdkDisplay *display = gdk_display_get_default ();
    GdkScreen  *screen  = gdk_display_get_default_screen (display);

    if (gdk_drawable_get_depth (d) == 32)
	return gdk_screen_get_rgba_colormap (screen);

    return gdk_screen_get_rgb_colormap (screen);
}

XRenderPictFormat *
get_format_for_drawable (decor_t *d, GdkDrawable *drawable)
{
    if (!d->frame_window || gdk_drawable_get_depth (drawable) == 32)
	return xformat_rgba;

    return xformat_rgb;
}

GdkPixmap *
create_pixmap (int w,
	       int h,
	       int depth)
{
    GtkWidget *widget;
    GdkWindow *window;

    if (w == 0 || h == 0)
	abort ();

    widget = (depth > 24) ? style_window_rgba : style_window_rgb;
    window = gtk_widget_get_window (widget);
    return gdk_pixmap_new (GDK_DRAWABLE (window), w, h, depth);
}