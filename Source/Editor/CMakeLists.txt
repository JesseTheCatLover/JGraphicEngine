# Editor/CMakeLists.txt

redleaf_add_module(
        NAME Editor

        PUBLIC_DEPS
        EditorRuntime
        imgui
)

redleaf_set_folder(Editor "EditorLibs")