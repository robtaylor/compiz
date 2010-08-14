#include "gtk-window-decorator.h"

/* TODO: Trash all of this and use a window property
 * instead - much much cleaner!
 */

#ifdef USE_GCONF
static gboolean
shadow_settings_changed (GConfClient *client)
{
    double   radius, opacity;
    int      offset;
    gchar    *color;
    gboolean changed = FALSE;

    radius = gconf_client_get_float (client,
				     COMPIZ_SHADOW_RADIUS_KEY,
				     NULL);
    radius = MAX (0.0, MIN (radius, 48.0));
    if (shadow_radius != radius)
    {
	shadow_radius = radius;
	changed = TRUE;
    }

    opacity = gconf_client_get_float (client,
				      COMPIZ_SHADOW_OPACITY_KEY,
				      NULL);
    opacity = MAX (0.0, MIN (opacity, 6.0));
    if (shadow_opacity != opacity)
    {
	shadow_opacity = opacity;
	changed = TRUE;
    }

    color = gconf_client_get_string (client,
				     COMPIZ_SHADOW_COLOR_KEY,
				     NULL);
    if (color)
    {
	int c[4];

	if (sscanf (color, "#%2x%2x%2x%2x", &c[0], &c[1], &c[2], &c[3]) == 4)
	{
	    shadow_color[0] = c[0] << 8 | c[0];
	    shadow_color[1] = c[1] << 8 | c[1];
	    shadow_color[2] = c[2] << 8 | c[2];
	    changed = TRUE;
	}

	g_free (color);
    }

    offset = gconf_client_get_int (client,
				   COMPIZ_SHADOW_OFFSET_X_KEY,
				   NULL);
    offset = MAX (-16, MIN (offset, 16));
    if (shadow_offset_x != offset)
    {
	shadow_offset_x = offset;
	changed = TRUE;
    }

    offset = gconf_client_get_int (client,
				   COMPIZ_SHADOW_OFFSET_Y_KEY,
				   NULL);
    offset = MAX (-16, MIN (offset, 16));
    if (shadow_offset_y != offset)
    {
	shadow_offset_y = offset;
	changed = TRUE;
    }

    return changed;
}

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
    }
    else
    {
	theme_draw_window_decoration	= draw_window_decoration;
	theme_calc_decoration_size	= calc_decoration_size;
	theme_update_border_extents	= update_border_extents;
	theme_get_event_window_position = get_event_window_position;
	theme_get_button_position	= get_button_position;
    }

    return TRUE;
#else
    theme_draw_window_decoration    = draw_window_decoration;
    theme_calc_decoration_size	    = calc_decoration_size;
    theme_update_border_extents	    = update_border_extents;
    theme_get_event_window_position = get_event_window_position;
    theme_get_button_position	    = get_button_position;

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

    str = gconf_client_get_string (client,
				   COMPIZ_TITLEBAR_FONT_KEY,
				   NULL);
    if (!str)
	str = g_strdup ("Sans Bold 12");

    if (titlebar_font)
	pango_font_description_free (titlebar_font);

    titlebar_font = pango_font_description_from_string (str);

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
    else if (strcmp (key, COMPIZ_SHADOW_RADIUS_KEY)   == 0 ||
	     strcmp (key, COMPIZ_SHADOW_OPACITY_KEY)  == 0 ||
	     strcmp (key, COMPIZ_SHADOW_OFFSET_X_KEY) == 0 ||
	     strcmp (key, COMPIZ_SHADOW_OFFSET_Y_KEY) == 0 ||
	     strcmp (key, COMPIZ_SHADOW_COLOR_KEY) == 0)
    {
	if (shadow_settings_changed (client))
	    changed = TRUE;
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


#elif USE_DBUS_GLIB

static DBusHandlerResult
dbus_handle_message (DBusConnection *connection,
		     DBusMessage    *message,
		     void           *user_data)
{
    WnckScreen	      *screen = user_data;
    char	      **path;
    const char        *interface, *member;
    DBusHandlerResult result = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    interface = dbus_message_get_interface (message);
    member    = dbus_message_get_member (message);

    (void) connection;

    if (!interface || !member)
	return result;

    if (!dbus_message_is_signal (message, interface, member))
	return result;

    if (strcmp (member, "changed"))
	return result;

    if (!dbus_message_get_path_decomposed (message, &path))
	return result;

    if (!path[0] || !path[1] || !path[2] || !path[3] || !path[4] || !path[5])
    {
	dbus_free_string_array (path);
	return result;
    }

    if (!strcmp (path[0], "org")	 &&
	!strcmp (path[1], "freedesktop") &&
	!strcmp (path[2], "compiz")      &&
	!strcmp (path[3], "decor")  &&
	!strcmp (path[4], "allscreens"))
    {
	result = DBUS_HANDLER_RESULT_HANDLED;

	if (strcmp (path[5], "shadow_radius") == 0)
	{
	    dbus_message_get_args (message, NULL,
				   DBUS_TYPE_DOUBLE, &shadow_radius,
				   DBUS_TYPE_INVALID);
	}
	else if (strcmp (path[5], "shadow_opacity") == 0)
	{
	    dbus_message_get_args (message, NULL,
				   DBUS_TYPE_DOUBLE, &shadow_opacity,
				   DBUS_TYPE_INVALID);
	}
	else if (strcmp (path[5], "shadow_color") == 0)
	{
	    DBusError error;
	    char      *str;

	    dbus_error_init (&error);

	    dbus_message_get_args (message, &error,
				   DBUS_TYPE_STRING, &str,
				   DBUS_TYPE_INVALID);

	    if (!dbus_error_is_set (&error))
	    {
		int c[4];

		if (sscanf (str, "#%2x%2x%2x%2x",
			    &c[0], &c[1], &c[2], &c[3]) == 4)
		{
		    shadow_color[0] = c[0] << 8 | c[0];
		    shadow_color[1] = c[1] << 8 | c[1];
		    shadow_color[2] = c[2] << 8 | c[2];
		}
	    }

	    dbus_error_free (&error);
	}
	else if (strcmp (path[5], "shadow_x_offset") == 0)
	{
	    dbus_message_get_args (message, NULL,
				   DBUS_TYPE_INT32, &shadow_offset_x,
				   DBUS_TYPE_INVALID);
	}
	else if (strcmp (path[5], "shadow_y_offset") == 0)
	{
	    dbus_message_get_args (message, NULL,
				   DBUS_TYPE_INT32, &shadow_offset_y,
				   DBUS_TYPE_INVALID);
	}

	decorations_changed (screen);
    }

    dbus_free_string_array (path);

    return result;
}

static DBusMessage *
send_and_block_for_shadow_option_reply (DBusConnection *connection,
					char	       *path)
{
    DBusMessage *message;

    message = dbus_message_new_method_call (NULL,
					    path,
					    DBUS_INTERFACE,
					    DBUS_METHOD_GET);
    if (message)
    {
	DBusMessage *reply;
	DBusError   error;

	dbus_message_set_destination (message, DBUS_DEST);

	dbus_error_init (&error);
	reply = dbus_connection_send_with_reply_and_block (connection,
							   message, -1,
							   &error);
	dbus_message_unref (message);

	if (!dbus_error_is_set (&error))
	    return reply;
    }

    return NULL;
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

    gconf_client_add_dir (gconf,
			  COMPIZ_GCONF_DIR1,
			  GCONF_CLIENT_PRELOAD_ONELEVEL,
			  NULL);

    g_signal_connect (G_OBJECT (gconf),
		      "value_changed",
		      G_CALLBACK (value_changed),
		      screen);
#elif USE_DBUS_GLIB
    DBusConnection *connection;
    DBusMessage	   *reply;
    DBusError	   error;

    dbus_error_init (&error);

    connection = dbus_bus_get (DBUS_BUS_SESSION, &error);
    if (!dbus_error_is_set (&error))
    {
	dbus_bus_add_match (connection, "type='signal'", &error);

	dbus_connection_add_filter (connection,
				    dbus_handle_message,
				    screen, NULL);

	dbus_connection_setup_with_g_main (connection, NULL);
    }

    reply = send_and_block_for_shadow_option_reply (connection, DBUS_PATH
						    "/shadow_radius");
    if (reply)
    {
	dbus_message_get_args (reply, NULL,
			       DBUS_TYPE_DOUBLE, &shadow_radius,
			       DBUS_TYPE_INVALID);

	dbus_message_unref (reply);
    }

    reply = send_and_block_for_shadow_option_reply (connection, DBUS_PATH
						    "/shadow_opacity");
    if (reply)
    {
	dbus_message_get_args (reply, NULL,
			       DBUS_TYPE_DOUBLE, &shadow_opacity,
			       DBUS_TYPE_INVALID);
	dbus_message_unref (reply);
    }

    reply = send_and_block_for_shadow_option_reply (connection, DBUS_PATH
						    "/shadow_color");
    if (reply)
    {
	DBusError error;
	char      *str;

	dbus_error_init (&error);

	dbus_message_get_args (reply, &error,
			       DBUS_TYPE_STRING, &str,
			       DBUS_TYPE_INVALID);

	if (!dbus_error_is_set (&error))
	{
	    int c[4];

	    if (sscanf (str, "#%2x%2x%2x%2x", &c[0], &c[1], &c[2], &c[3]) == 4)
	    {
		shadow_color[0] = c[0] << 8 | c[0];
		shadow_color[1] = c[1] << 8 | c[1];
		shadow_color[2] = c[2] << 8 | c[2];
	    }
	}

	dbus_error_free (&error);

	dbus_message_unref (reply);
    }

    reply = send_and_block_for_shadow_option_reply (connection, DBUS_PATH
						    "/shadow_x_offset");
    if (reply)
    {
	dbus_message_get_args (reply, NULL,
			       DBUS_TYPE_INT32, &shadow_offset_x,
			       DBUS_TYPE_INVALID);
	dbus_message_unref (reply);
    }

    reply = send_and_block_for_shadow_option_reply (connection, DBUS_PATH
						    "/shadow_y_offset");
    if (reply)
    {
	dbus_message_get_args (reply, NULL,
			       DBUS_TYPE_INT32, &shadow_offset_y,
			       DBUS_TYPE_INVALID);
	dbus_message_unref (reply);
    }
#endif

    style_window_rgba = gtk_window_new (GTK_WINDOW_POPUP);

    gdkscreen = gdk_display_get_default_screen (gdk_display_get_default ());
    colormap = gdk_screen_get_rgba_colormap (gdkscreen);
    if (colormap)
	gtk_widget_set_colormap (style_window_rgba, colormap);

    gtk_widget_realize (style_window_rgba);

    switcher_label = gtk_label_new ("");
    switcher_label_obj = gtk_widget_get_accessible (switcher_label);
    atk_object_set_role (switcher_label_obj, ATK_ROLE_STATUSBAR);
    gtk_container_add (GTK_CONTAINER (style_window_rgba), switcher_label);

    gtk_widget_set_size_request (style_window_rgba, 0, 0);
    gtk_window_move (GTK_WINDOW (style_window_rgba), -100, -100);
    gtk_widget_show_all (style_window_rgba);

    g_signal_connect_object (style_window_rgba, "style-set",
			     G_CALLBACK (style_changed),
			     0, 0);

    settings = gtk_widget_get_settings (style_window_rgba);

    g_object_get (G_OBJECT (settings), "gtk-double-click-time",
		  &double_click_timeout, NULL);

    pango_context = gtk_widget_create_pango_context (style_window_rgba);

    style_window_rgb = gtk_window_new (GTK_WINDOW_POPUP);

    gdkscreen = gdk_display_get_default_screen (gdk_display_get_default ());
    colormap = gdk_screen_get_rgb_colormap (gdkscreen);
    if (colormap)
	gtk_widget_set_colormap (style_window_rgb, colormap);

    gtk_widget_realize (style_window_rgb);

    switcher_label = gtk_label_new ("");
    switcher_label_obj = gtk_widget_get_accessible (switcher_label);
    atk_object_set_role (switcher_label_obj, ATK_ROLE_STATUSBAR);
    gtk_container_add (GTK_CONTAINER (style_window_rgb), switcher_label);

    gtk_widget_set_size_request (style_window_rgb, 0, 0);
    gtk_window_move (GTK_WINDOW (style_window_rgb), -100, -100);
    gtk_widget_show_all (style_window_rgb);

    g_signal_connect_object (style_window_rgb, "style-set",
			     G_CALLBACK (style_changed),
			     0, 0);

    settings = gtk_widget_get_settings (style_window_rgb);

    g_object_get (G_OBJECT (settings), "gtk-double-click-time",
		  &double_click_timeout, NULL);

    pango_context = gtk_widget_create_pango_context (style_window_rgb);

#ifdef USE_GCONF
    use_system_font = gconf_client_get_bool (gconf,
					     COMPIZ_USE_SYSTEM_FONT_KEY,
					     NULL);
    theme_changed (gconf);
    theme_opacity_changed (gconf);
    button_layout_changed (gconf);
#endif

    update_style (style_window_rgba);
    update_style (style_window_rgb);
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
    shadow_settings_changed (gconf);
    blur_settings_changed (gconf);
#endif

    (*theme_update_border_extents) (text_height);

    update_shadow ();

    return TRUE;
}