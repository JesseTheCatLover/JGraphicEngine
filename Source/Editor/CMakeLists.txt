# Editor/CMakeLists.txt

redleaf_add_module(
        NAME Editor

        PUBLIC_DEPS
        EditorRuntime
        imgui

        PRIVATE_DEPS SilverwareBackend
)

redleaf_set_folder(Editor "EditorLibs")