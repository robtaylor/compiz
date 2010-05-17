find_package (Compiz REQUIRED)

include (CompizPlugin)
include (CompizCommon)

message ("build rpath is " ${COMPIZ_BUILD_WITH_RPATH})

compiz_plugin(decor PLUGINDEPS composite opengl LIBRARIES decoration)

if (COMPIZ_BUILD_WITH_RPATH)
    message ("set rpath")

    set_target_properties (
	decor PROPERTIES
	INSTALL_RPATH "${COMPIZ_LIBDIR}"
    )
endif (COMPIZ_BUILD_WITH_RPATH)
