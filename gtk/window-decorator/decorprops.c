#include "gtk-window-decorator.h"

void
decor_update_window_property (decor_t *d)
{
    long	    data[256];
    Display	    *xdisplay =
	GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
    WnckWindowType  win_type = wnck_window_get_window_type (d->win);
    decor_extents_t extents = gwd_get_decor_frame (get_frame_type (win_type))->win_extents;
    gint	    nQuad;
    decor_quad_t    quads[N_QUADS_MAX];
    int		    w, h;
    gint	    stretch_offset;
    REGION	    top, bottom, left, right;

    w = d->border_layout.top.x2 - d->border_layout.top.x1 -
	d->context->left_space - d->context->right_space;

    if (d->border_layout.rotation)
	h = d->border_layout.left.x2 - d->border_layout.left.x1;
    else
	h = d->border_layout.left.y2 - d->border_layout.left.y1;

    stretch_offset = w - d->button_width - 1;

    nQuad = decor_set_lSrStXbS_window_quads (quads, d->context,
					     &d->border_layout,
					     stretch_offset);

    extents.top += gwd_get_decor_frame (get_frame_type (win_type))->titlebar_height;

    if (d->frame_window)
    {
	decor_gen_window_property (data, &extents, &extents, 20, 20);
    }
    else
    {
	decor_quads_to_property (data, GDK_PIXMAP_XID (d->pixmap),
			     &extents, &extents,
			     &extents, &extents,
			     ICON_SPACE + d->button_width,
			     0,
			     quads, nQuad);
    }

    gdk_error_trap_push ();
    XChangeProperty (xdisplay, d->prop_xid,
		     win_decor_atom,
		     XA_INTEGER,
		     32, PropModeReplace, (guchar *) data,
		     BASE_PROP_SIZE + QUAD_PROP_SIZE * nQuad);
    gdk_display_sync (gdk_display_get_default ());
    gdk_error_trap_pop ();

    top.rects = &top.extents;
    top.numRects = top.size = 1;

    top.extents.x1 = -extents.left;
    top.extents.y1 = -extents.top;
    top.extents.x2 = w + extents.right;
    top.extents.y2 = 0;

    bottom.rects = &bottom.extents;
    bottom.numRects = bottom.size = 1;

    bottom.extents.x1 = -extents.left;
    bottom.extents.y1 = 0;
    bottom.extents.x2 = w + extents.right;
    bottom.extents.y2 = extents.bottom;

    left.rects = &left.extents;
    left.numRects = left.size = 1;

    left.extents.x1 = -extents.left;
    left.extents.y1 = 0;
    left.extents.x2 = 0;
    left.extents.y2 = h;

    right.rects = &right.extents;
    right.numRects = right.size = 1;

    right.extents.x1 = 0;
    right.extents.y1 = 0;
    right.extents.x2 = extents.right;
    right.extents.y2 = h;

    decor_update_blur_property (d,
				w, h,
				&top, stretch_offset,
				&bottom, w / 2,
				&left, h / 2,
				&right, h / 2);
}

void
decor_update_switcher_property (decor_t *d)
{
    decor_extents_t _switcher_extents    = { 6, 6, 6, 6 + SWITCHER_SPACE };
    decor_context_t switcher_context = {
    	{ 0, 0, 0, 0 },
    	6, 6, 6, 6 + SWITCHER_SPACE,
    	0, 0, 0, 0
	};
    long	 data[256];
    Display	 *xdisplay = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
    gint	 nQuad;
    decor_quad_t quads[N_QUADS_MAX];
    GtkStyle     *style;
    long         fgColor[4];
    
    nQuad = decor_set_lSrStSbX_window_quads (quads, &d->frame->window_context,
					     &d->border_layout,
					     d->border_layout.top.x2 -
					     d->border_layout.top.x1 -
					     d->frame->window_context.extents.left -
						 d->frame->window_context.extents.right -
						     32);
    
    decor_quads_to_property (data, GDK_PIXMAP_XID (d->pixmap),
			     &d->frame->win_extents, &d->frame->win_extents,
			     &d->frame->win_extents, &d->frame->win_extents,
			     0, 0, quads, nQuad);
    
    style = gtk_widget_get_style (d->frame->style_window_rgba);
    
    fgColor[0] = style->fg[GTK_STATE_NORMAL].red;
    fgColor[1] = style->fg[GTK_STATE_NORMAL].green;
    fgColor[2] = style->fg[GTK_STATE_NORMAL].blue;
    fgColor[3] = SWITCHER_ALPHA;
    
    gdk_error_trap_push ();
    XChangeProperty (xdisplay, d->prop_xid,
		     win_decor_atom,
		     XA_INTEGER,
		     32, PropModeReplace, (guchar *) data,
		     BASE_PROP_SIZE + QUAD_PROP_SIZE * nQuad);
    XChangeProperty (xdisplay, d->prop_xid, switcher_fg_atom,
		     XA_INTEGER, 32, PropModeReplace, (guchar *) fgColor, 4);
    gdk_display_sync (gdk_display_get_default ());
    gdk_error_trap_pop ();
    
}
