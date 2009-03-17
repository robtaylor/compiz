find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin(decor PLUGINDEPS composite opengl LIBRARIES decoration)

set_target_properties (
    decor PROPERTIES
    INSTALL_RPATH "${COMPIZ_LIBDIR}"
)
