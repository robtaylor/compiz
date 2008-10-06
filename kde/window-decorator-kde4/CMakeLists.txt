project (kde4-window-decorator)

set (USE_KDE4 1 CACHE BOOL "Build KDE 4 window decorator")

if (USE_KDE4)

    find_package(KDE4)

    if (KDE4_FOUND)

	include(KDE4Defaults)
	include(FindPkgConfig)
	include(MacroLibrary)
	include (FindPlasma)

	if (PLASMA_FOUND)

	    add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

	    qt4_add_dbus_adaptor (kwd4_SRCS org.kde.KWin.xml decorator.h KWD::Decorator kwinadaptor)
	    

	    pkg_check_modules (KWD4 REQUIRED xdamage xrender xcomposite)

	    link_directories(
		${KWD4_LIBRARY_DIRS}
	    )

	    include_directories (
		${compiz_SOURCE_DIR}/include
		${KWD4_INCLUDE_DIRS}
		${KDE4_INCLUDES}
		${KDE4_INCLUDE_DIR}
		${PLASMA_INCLUDE_DIR}
		${QT_INCLUDES}
	    )

	    set (kwd4_SRCS
		main.cpp
		utils.cpp
		decorator.cpp
		window.cpp
		options.cpp
		kdecoration_plugins.cpp
		switcher.cpp
		${kwd4_SRCS}
	    )

	    kde4_add_executable (
		kde4-window-decorator
		${kwd4_SRCS}
	    )

	    target_link_libraries (
		kde4-window-decorator
		decoration
		${KWD4_LIBRARIES}
		${PLASMA_LIBS}
		${KDE4_KDECORE_LIBS}
		${KDE4_KDEUI_LIBS}
		kdecorations
	    )

	    install (
		TARGETS kde4-window-decorator
		DESTINATION ${exec_prefix}
	    )

	else (PLASMA_FOUND)
	    set (USE_KDE4 0)
	endif (PLASMA_FOUND)

    else (KDE4_FOUND)
	set (USE_KDE4 0)
    endif (KDE4_FOUND)

endif (USE_KDE4)