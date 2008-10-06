set (_headers
    action.h
    atoms.h
    core.h
    icon.h
    match.h
    metadata.h
    option.h
    output.h
    plugin.h
    point.h
    privatehandler.h
    privates.h
    rect.h
    region.h
    screen.h
    session.h
    size.h
    timer.h
    window.h
    wrapsystem.h
)

install (
    FILES ${_headers}
    DESTINATION ${includedir}/compiz/core
)