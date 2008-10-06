
set (_headers
    opengl.h
    fragment.h
    matrix.h
    texture.h
    vector.h
)

install (
    FILES ${_headers}
    DESTINATION ${includedir}/compiz/opengl
)