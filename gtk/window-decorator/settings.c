#include "gtk-window-decorator.h"

/* TODO: Trash all of this and use a window property
 * instead - much much cleaner!
 */

void
shadow_property_changed (WnckScreen *s)
{
    GdkDisplay *display = gdk_display_get_default ();
    Display    *xdisplay = GDK_DISPLAY_XDISPLAY (display);
    GdkScreen  *screen = gdk_display_get_default_screen (display);
    Window     root = GDK_WINDOW_XWINDOW (gdk_screen_get_root_window (screen));
    Atom actual;
    int  result, format;
    unsigned long n, left;
    unsigned char *prop_data;
    gboolean	  changed = FALSE;
    XTextProperty shadow_color_xtp;

    result = XGetWindowProperty (xdisplay, root, compiz_shadow_info_atom,
				 0, 32768, 0, XA_INTEGER, &actual,
				 &format, &n, &left, &prop_data);

    if (result != Success)
	return;

    if (n == 4)
    {
	long *data      = (long *) prop_data;
	gdouble radius  = data[0];
	gdouble opacity = data[1];
	gint x_off      = data[2];
	gint y_off      = data[3];

	/* Radius and Opacity are multiplied by 1000 to keep precision,
	 * divide by that much to get our real radius and opacity
	 */
	radius /= 1000;
	opacity /= 1000;

	changed = radius != shadow_radius   ||
		  opacity != shadow_opacity ||
		  x_off != shadow_offset_x  ||
		  y_off != shadow_offset_y;

	shadow_radius = (gdouble) MAX (0.0, MIN (radius, 48.0));
	shadow_opacity = (gdouble) MAX (0.0, MIN (opacity, 6.0));
	shadow_offset_x = (gint) MAX (-16, MIN (x_off, 16));
	shadow_offset_y = (gint) MAX (-16, MIN (y_off, 16));
    }

    XFree (prop_data);

    result = XGetTextProperty (xdisplay, root, &shadow_color_xtp,
			       compiz_shadow_color_atom);

    if (shadow_color_xtp.value)
    {
	int  ret_count = 0;
	char **t_data = NULL;
	
	XTextPropertyToStringList (&shadow_color_xtp, &t_data, &ret_count);
	
	if (ret_count == 1)
	{
	    int c[4];

	    if (sscanf (t_data[0], "#%2x%2x%2x%2x",
			&c[0], &c[1], &c[2], &c[3]) == 4)
	    {
		shadow_color[0] = c[0] << 8 | c[0];
		shadow_color[1] = c[1] << 8 | c[1];
		shadow_color[2] = c[2] << 8 | c[2];
		changed = TRUE;
	    }
	}

	XFree (shadow_color_xtp.value);
	if (t_data)
	    XFreeStringList (t_data);
    }

    if (changed)
	decorations_changed (s);
}

#ifdef USE_GCONF
static gboolean
blur_settings_changed (GConfClient *client)
{
    gchar *type;
    int   new_type = blur_type;

    if (cmdline_options & CMDLINE_BLUR)
	return FALSE;

    type = gconf_client_get_string (client,
				    BLUR_TYPE_KEY,
				    NULL);

    if (type)
    {
	if (strcmp (type, "titlebar") == 0)
	    new_type = BLUR_TYPE_TITLEBAR;
	else if (strcmp (type, "all") == 0)
	    new_type = BLUR_TYPE_ALL;
	else if (strcmp (type, "none") == 0)
	    new_type = BLUR_TYPE_NONE;

	g_free (type);
    }

    if (new_type != blur_type)
    {
	blur_type = new_type;
	return TRUE;
    }

    return FALSE;
}

static gboolean
theme_changed (GConfClient *client)
{

#ifdef USE_METACITY
    gboolean use_meta_theme;

    if (cmdline_options & CMDLINE_THEME)
	return FALSE;

    use_meta_theme = gconf_client_get_bool (client,
					    USE_META_THEME_KEY,
					    NULL);

    if (use_meta_theme)
    {
	gchar *theme;

	theme = gconf_client_get_string (client,
					 META_THEME_KEY,
					 NULL);

	if (theme)
	{
	    meta_theme_set_current (theme, TRUE);
	    if (!meta_theme_get_current ())
		use_meta_theme = FALSE;

	    g_free (theme);
	}
	else
	{
	    use_meta_theme = FALSE;
	}
    }

    if (use_meta_theme)
    {
	theme_draw_window_decoration	= meta_draw_window_decoration;
	theme_calc_decoration_size	= meta_calc_decoration_size;
	theme_update_border_extents	= meta_update_border_extents;
	theme_get_event_window_position = meta_get_event_window_position;
	theme_get_button_position	= meta_get_button_position;
	theme_get_title_scale	    	= meta_get_title_scale;
    }
    else
    {
	theme_draw_window_decoration	= draw_window_decoration;
	theme_calc_decoration_size	= calc_decoration_size;
	theme_update_border_extents	= update_border_extents;
	theme_get_event_window_position = get_event_window_position;
	theme_get_button_position	= get_button_position;
	theme_get_title_scale	    	= get_title_scale;
    }

    return TRUE;
#else
    theme_draw_window_decoration    = draw_window_decoration;
    theme_calc_decoration_size	    = calc_decoration_size;
    theme_update_border_extents	    = update_border_extents;
    theme_get_event_window_position = get_event_window_position;
    theme_get_button_position	    = get_button_position;
    theme_get_title_scale	    = get_title_scale;

    return FALSE;
#endif

}

static gboolean
theme_opacity_changed (GConfClient *client)
{

#ifdef USE_METACITY
    gboolean shade_opacity, changed = FALSE;
    gdouble  opacity;

    opacity = gconf_client_get_float (client,
				      META_THEME_OPACITY_KEY,
				      NULL);

    if (!(cmdline_options & CMDLINE_OPACITY) &&
	opacity != meta_opacity)
    {
	meta_opacity = opacity;
	changed = TRUE;
    }

    if (opacity < 1.0)
    {
	shade_opacity = gconf_client_get_bool (client,
					       META_THEME_SHADE_OPACITY_KEY,
					       NULL);

	if (!(cmdline_options & CMDLINE_OPACITY_SHADE) &&
	    shade_opacity != meta_shade_opacity)
	{
	    meta_shade_opacity = shade_opacity;
	    changed = TRUE;
	}
    }

    opacity = gconf_client_get_float (client,
				      META_THEME_ACTIVE_OPACITY_KEY,
				      NULL);

    if (!(cmdline_options & CMDLINE_ACTIVE_OPACITY) &&
	opacity != meta_active_opacity)
    {
	meta_active_opacity = opacity;
	changed = TRUE;
    }

    if (opacity < 1.0)
    {
	shade_opacity =
	    gconf_client_get_bool (client,
				   META_THEME_ACTIVE_SHADE_OPACITY_KEY,
				   NULL);

	if (!(cmdline_options & CMDLINE_ACTIVE_OPACITY_SHADE) &&
	    shade_opacity != meta_active_shade_opacity)
	{
	    meta_active_shade_opacity = shade_opacity;
	    changed = TRUE;
	}
    }

    return changed;
#else
    return FALSE;
#endif

}

static gboolean
button_layout_changed (GConfClient *client)
{

#ifdef USE_METACITY
    gchar *button_layout;

    button_layout = gconf_client_get_string (client,
					     META_BUTTON_LAYOUT_KEY,
					     NULL);

    if (button_layout)
    {
	meta_update_button_layout (button_layout);

	meta_button_layout_set = TRUE;

	g_free (button_layout);

	return TRUE;
    }

    if (meta_button_layout_set)
    {
	meta_button_layout_set = FALSE;
	return TRUE;
    }
#endif

    return FALSE;
}

static void
titlebar_font_changed (GConfClient *client)
{
    gchar *str;
    gint  i;

    str = gconf_client_get_string (client,
				   COMPIZ_TITLEBAR_FONT_KEY,
				   NULL);
    if (!str)
	str = g_strdup ("Sans Bold 12");

    for (i = 0; i < 5; i++)
    {
	decor_frame_t *frame = &decor_frames[i];
	gfloat	      scale = 1.0f;
	if (frame->titlebar_font)
	    pango_font_description_free (frame->titlebar_font);

	frame->titlebar_font = pango_font_description_from_string (str);

	scale = (*theme_get_title_scale) (frame);

	pango_font_description_set_size (frame->titlebar_font,
					 MAX (pango_font_description_get_size (frame->titlebar_font) * scale, 1));
    }

    g_free (str);
}

static void
titlebar_click_action_changed (GConfClient *client,
			       const gchar *key,
			       int         *action_value,
			       int          default_value)
{
    gchar *action;

    *action_value = default_value;

    action = gconf_client_get_string (client, key, NULL);
    if (action)
    {
	if (strcmp (action, "toggle_shade") == 0)
	    *action_value = CLICK_ACTION_SHADE;
	else if (strcmp (action, "toggle_maximize") == 0)
	    *action_value = CLICK_ACTION_MAXIMIZE;
	else if (strcmp (action, "minimize") == 0)
	    *action_value = CLICK_ACTION_MINIMIZE;
	else if (strcmp (action, "raise") == 0)
	    *action_value = CLICK_ACTION_RAISE;
	else if (strcmp (action, "lower") == 0)
	    *action_value = CLICK_ACTION_LOWER;
	else if (strcmp (action, "menu") == 0)
	    *action_value = CLICK_ACTION_MENU;
	else if (strcmp (action, "none") == 0)
	    *action_value = CLICK_ACTION_NONE;

	g_free (action);
    }
}

static void
wheel_action_changed (GConfClient *client)
{
    gchar *action;

    wheel_action = WHEEL_ACTION_DEFAULT;

    action = gconf_client_get_string (client, WHEEL_ACTION_KEY, NULL);
    if (action)
    {
	if (strcmp (action, "shade") == 0)
	    wheel_action = WHEEL_ACTION_SHADE;
	else if (strcmp (action, "none") == 0)
	    wheel_action = WHEEL_ACTION_NONE;

	g_free (action);
    }
}

static void
value_changed (GConfClient *client,
	       const gchar *key,
	       GConfValue  *value,
	       void        *data)
{
    gboolean changed = FALSE;

    if (strcmp (key, COMPIZ_USE_SYSTEM_FONT_KEY) == 0)
    {
	if (gconf_client_get_bool (client,
				   COMPIZ_USE_SYSTEM_FONT_KEY,
				   NULL) != use_system_font)
	{
	    use_system_font = !use_system_font;
	    changed = TRUE;
	}
    }
    else if (strcmp (key, COMPIZ_TITLEBAR_FONT_KEY) == 0)
    {
	titlebar_font_changed (client);
	changed = !use_system_font;
    }
    else if (strcmp (key, COMPIZ_DOUBLE_CLICK_TITLEBAR_KEY) == 0)
    {
	titlebar_click_action_changed (client, key,
				       &double_click_action,
				       DOUBLE_CLICK_ACTION_DEFAULT);
    }
    else if (strcmp (key, COMPIZ_MIDDLE_CLICK_TITLEBAR_KEY) == 0)
    {
	titlebar_click_action_changed (client, key,
				       &middle_click_action,
				       MIDDLE_CLICK_ACTION_DEFAULT);
    }
    else if (strcmp (key, COMPIZ_RIGHT_CLICK_TITLEBAR_KEY) == 0)
    {
	titlebar_click_action_changed (client, key,
				       &right_click_action,
				       RIGHT_CLICK_ACTION_DEFAULT);
    }
    else if (strcmp (key, WHEEL_ACTION_KEY) == 0)
    {
	wheel_action_changed (client);
    }
    else if (strcmp (key, BLUR_TYPE_KEY) == 0)
    {
	if (blur_settings_changed (client))
	    changed = TRUE;
    }
    else if (strcmp (key, USE_META_THEME_KEY) == 0 ||
	     strcmp (key, META_THEME_KEY) == 0)
    {
	if (theme_changed (client))
	    changed = TRUE;
    }
    else if (strcmp (key, META_BUTTON_LAYOUT_KEY) == 0)
    {
	if (button_layout_changed (client))
	    changed = TRUE;
    }
    else if (strcmp (key, META_THEME_OPACITY_KEY)	       == 0 ||
	     strcmp (key, META_THEME_SHADE_OPACITY_KEY)	       == 0 ||
	     strcmp (key, META_THEME_ACTIVE_OPACITY_KEY)       == 0 ||
	     strcmp (key, META_THEME_ACTIVE_SHADE_OPACITY_KEY) == 0)
    {
	if (theme_opacity_changed (client))
	    changed = TRUE;
    }

    if (changed)
	decorations_changed (data);
}
#endif

gboolean
init_settings (WnckScreen *screen)
{
    GtkSettings	   *settings;
    GdkScreen	   *gdkscreen;
    GdkColormap	   *colormap;
    AtkObject	   *switcher_label_obj;

#ifdef USE_GCONF
    GConfClient	   *gconf;

    gconf = gconf_client_get_default ();

    gconf_client_add_dir (gconf,
			  GCONF_DIR,
			  GCONF_CLIENT_PRELOAD_ONELEVEL,
			  NULL);

    gconf_client_add_dir (gconf,
			  METACITY_GCONF_DIR,
			  GCONF_CLIENT_PRELOAD_ONELEVEL,
			  NULL);

    g_signal_connect (G_OBJECT (gconf),
		      "value_changed",
		      G_CALLBACK (value_changed),
		      screen);
#endif

    switcher_style_window_rgba = gtk_window_new (GTK_WINDOW_POPUP);

    gdkscreen = gdk_display_get_default_screen (gdk_display_get_default ());
    colormap = gdk_screen_get_rgba_colormap (gdkscreen);
    if (colormap)
	gtk_widget_set_colormap (switcher_style_window_rgba, colormap);

    gtk_widget_realize (switcher_style_window_rgba);

    switcher_label = gtk_label_new ("");
    switcher_label_obj = gtk_widget_get_accessible (switcher_label);
    atk_object_set_role (switcher_label_obj, ATK_ROLE_STATUSBAR);
    gtk_container_add (GTK_CONTAINER (switcher_style_window_rgba), switcher_label);

    gtk_widget_set_size_request (switcher_style_window_rgba, 0, 0);
    gtk_window_move (GTK_WINDOW (switcher_style_window_rgba), -100, -100);
    gtk_widget_show_all (switcher_style_window_rgba);

    g_signal_connect_object (switcher_style_window_rgba, "style-set",
			     G_CALLBACK (style_changed),
			     0, 0);

    settings = gtk_widget_get_settings (switcher_style_window_rgba);

    g_object_get (G_OBJECT (settings), "gtk-double-click-time",
		  &double_click_timeout, NULL);

    switcher_pango_context = gtk_widget_create_pango_context (switcher_style_window_rgba);

#ifdef USE_GCONF
    use_system_font = gconf_client_get_bool (gconf,
					     COMPIZ_USE_SYSTEM_FONT_KEY,
					     NULL);
    theme_changed (gconf);
    theme_opacity_changed (gconf);
    button_layout_changed (gconf);
#endif

    update_style (switcher_style_window_rgba);
#ifdef USE_GCONF
    titlebar_font_changed (gconf);
#endif

    update_titlebar_font ();

#ifdef USE_GCONF
    titlebar_click_action_changed (gconf,
				   COMPIZ_DOUBLE_CLICK_TITLEBAR_KEY,
				   &double_click_action,
				   DOUBLE_CLICK_ACTION_DEFAULT);
    titlebar_click_action_changed (gconf,
				   COMPIZ_MIDDLE_CLICK_TITLEBAR_KEY,
				   &middle_click_action,
				   MIDDLE_CLICK_ACTION_DEFAULT);
    titlebar_click_action_changed (gconf,
				   COMPIZ_RIGHT_CLICK_TITLEBAR_KEY,
				   &right_click_action,
				   RIGHT_CLICK_ACTION_DEFAULT);
    wheel_action_changed (gconf);
    blur_settings_changed (gconf);
#endif

    (*theme_update_border_extents) ();
    
    shadow_property_changed (screen);

    update_shadow ();

    return TRUE;
}
