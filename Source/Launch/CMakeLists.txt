# Launch/CMakeLists.txt

# ---------------- Application Module ----------------

redleaf_add_module(
        NAME JApplication

        PUBLIC_DEPS
        Engine
        Editor
        imgui
)

# ---------------- Executables ----------------

redleaf_add_executable(
        NAME JEditor
        ENTRY EditorEntry.cpp
        DEPS JApplication
)

redleaf_add_executable(
        NAME JGame
        ENTRY GameEntry.cpp
        DEPS JApplication
)

redleaf_add_executable(
        NAME RedleafEngine
        ENTRY ApplicationEntry.cpp
        DEPS JApplication
)

# ---------------- Force Engine Registration ----------------

redleaf_force_link(JEditor Engine)
redleaf_force_link(JGame Engine)
redleaf_force_link(RedleafEngine Engine)

# ---------------- IDE Organization ----------------

redleaf_set_folder(JEditor "Executables")
redleaf_set_folder(JGame "Executables")
redleaf_set_folder(RedleafEngine "Executables")