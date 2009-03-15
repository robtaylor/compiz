set (_headers
    compiz.h
    decoration.h
)

install (
    FILES ${_headers}
    DESTINATION ${includedir}/compiz
)

add_subdirectory (core)
