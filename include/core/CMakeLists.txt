set (_headers
    action.h
    atoms.h
    core.h
    countedlist.h
    icon.h
    match.h
    modifierhandler.h
    option.h
    output.h
    plugin.h
    point.h
    pluginclasshandler.h
    pluginclasses.h
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
