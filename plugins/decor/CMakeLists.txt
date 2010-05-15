find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin(decor PLUGINDEPS composite opengl LDFLAGSADD -ldecoration)
