set (USE_GTK 1 CACHE BOOL "Build gtk-window-decorator")
set (USE_METACITY 1 CACHE BOOL "Gtk-window-decorator metacity theme support")
set (USE_GNOME 1 CACHE BOOL "Gnome settings module")
set (USE_GNOME_KEYBINDINGS 1 CACHE BOOL "Gnome keybindings")
set (USE_GCONF 1 CACHE BOOL "Install GConf schemas")

if (USE_GCONF)
    pkg_check_modules (GCONF gconf-2.0)

    find_program (GCONFTOOL_EXECUTABLE gconftool-2)
    mark_as_advanced (FORCE GCONFTOOL_EXECUTABLE)

    if (NOT GCONF_FOUND OR NOT GCONFTOOL_EXECUTABLE)
	set (USE_GCONF 0)
    endif (NOT GCONF_FOUND OR NOT GCONFTOOL_EXECUTABLE)
endif (USE_GCONF)

if (USE_GTK)
    pkg_check_modules (GTK_WINDOW_DECORATOR
	xrender>=0.8.4
	gtk+-2.0>=2.8.0
	libwnck-1.0
	pangocairo
    )
    if (GTK_WINDOW_DECORATOR_FOUND)
        set (CMAKE_REQUIRED_FLAGS ${GTK_WINDOW_DECORATOR_CFLAGS})
        set (CMAKE_REQUIRED_LIBRARIES ${GTK_WINDOW_DECORATOR_LIBRARIES})
        check_function_exists (wnck_window_has_name HAVE_WNCK_WINDOW_HAS_NAME)
        set (CMAKE_REQUIRED_FLAGS "")
        set (CMAKE_REQUIRED_LIBRARIES "")

        check_pkg_module (libwnck-1.0>=2.18.1 HAVE_LIBWNCK_2_18_1)
        check_pkg_module (libwnck-1.0>=2.19.4 HAVE_LIBWNCK_2_19_4)

        if (USE_METACITY)
            pkg_check_modules (METACITY libmetacity-private)
            if (METACITY_FOUND)
		check_pkg_module (libmetacity-private>=2.15.21 HAVE_METACITY_2_15_21)
		check_pkg_module (libmetacity-private>=2.17.0 HAVE_METACITY_2_17_0)
		check_pkg_module (libmetacity-private>=2.23.2 HAVE_METACITY_2_23_2)
            else (METACITY_FOUND)
		set (USE_METACITY 0)
            endif (METACITY_FOUND)
        endif (USE_METACITY)

	if (USE_GNOME)
	    pkg_check_modules (GNOME_WINDOW_SETTINGS
		gnome-window-settings-2.0
		gnome-desktop-2.0
	    )

	    if (GNOME_WINDOW_SETTINGS_FOUND)
	        exec_program (${PKG_CONFIG_EXECUTABLE} 
		  ARGS "--variable=prefix gnome-window-settings-2.0" 
		  OUTPUT_VARIABLE windowsettingsdatadir
		)
		set (windowsettingsdatadir ${windowsettingsdatadir}/share)
		exec_program (${PKG_CONFIG_EXECUTABLE} 
		  ARGS "--variable=libdir gnome-window-settings-2.0"
		  OUTPUT_VARIABLE windowsettingslibdir
		)
	    else (GNOME_WINDOW_SETTINGS_FOUND)
		set (USE_GNOME 0)
	    endif (GNOME_WINDOW_SETTINGS_FOUND)
	endif (USE_GNOME)

	if (USE_GNOME_KEYBINDINGS)
	    pkg_check_modules (GNOME_KEY_BINDINGS gnome-keybindings)
	    if (GNOME_KEY_BINDINGS_FOUND)
		exec_program (${PKG_CONFIG_EXECUTABLE}
		  ARGS "--variable=keysdir gnome-keybindings"
		  OUTPUT_VARIABLE keybindingsdir
		)
	    else (GNOME_KEY_BINDINGS_FOUND)
		set (USE_GNOME_KEYBINDINGS 0)
	    endif (GNOME_KEY_BINDINGS_FOUND)
        endif (USE_GNOME_KEYBINDINGS)
    else (GTK_WINDOW_DECORATOR_FOUND)
	set (USE_GTK 0)
    endif (GTK_WINDOW_DECORATOR_FOUND)
endif (USE_GTK)

if (!USE_GTK)
    set (USE_METACITY 0)
    set (USE_GNOME 0)
    set (USE_GNOME_KEYBINDINGS 0)
endif (!USE_GTK)

configure_file (
    ${CMAKE_CURRENT_SOURCE_DIR}/config.h.gtk.in
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
)

add_subdirectory (window-decorator)
add_subdirectory (gnome)
