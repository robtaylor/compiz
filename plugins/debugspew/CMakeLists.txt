find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin (debugspew PLUGINDEPS composite opengl)
