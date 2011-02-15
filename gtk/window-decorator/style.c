#include "gtk-window-decorator.h"

void
update_style (GtkWidget *widget)
{
    GtkStyle      *style;
    decor_color_t spot_color;
    
    style = gtk_widget_get_style (widget);
    g_object_ref (G_OBJECT (style));
    
    style = gtk_style_attach (style, widget->window);
    
    spot_color.r = style->bg[GTK_STATE_SELECTED].red   / 65535.0;
    spot_color.g = style->bg[GTK_STATE_SELECTED].green / 65535.0;
    spot_color.b = style->bg[GTK_STATE_SELECTED].blue  / 65535.0;
    
    g_object_unref (G_OBJECT (style));
    
    shade (&spot_color, &_title_color[0], 1.05);
    shade (&_title_color[0], &_title_color[1], 0.85);
    
}

void
style_changed (PangoContext *context,
	       GtkWidget *widget)
{
    GdkDisplay *gdkdisplay;
    GdkScreen  *gdkscreen;
    WnckScreen *screen;

    gdkdisplay = gdk_display_get_default ();
    gdkscreen  = gdk_display_get_default_screen (gdkdisplay);
    screen     = wnck_screen_get_default ();

    update_style (widget);

    pango_cairo_context_set_resolution (context,
					gdk_screen_get_resolution (gdkscreen));

    decorations_changed (screen);
}
