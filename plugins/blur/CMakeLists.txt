find_package (Compiz REQUIRED)

include (CompizPlugin)

find_package (OpenGL)
if (OPENGL_GLU_FOUND)
    compiz_plugin(blur PLUGINDEPS composite opengl LIBRARIES ${OPENGL_glu_LIBRARY} INCDIRS ${OPENGL_INCLUDE_DIR})
endif ()