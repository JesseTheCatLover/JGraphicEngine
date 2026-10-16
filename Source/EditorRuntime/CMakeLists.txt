# EditorRuntime/CMakeLists.txt

redleaf_add_module(
        NAME EditorRuntime

        PUBLIC_DEPS Engine
        PRIVATE_DEPS SilverwareBackend
)

redleaf_grant_private_access(
        TARGET EditorRuntime

        MODULES
        Engine
)