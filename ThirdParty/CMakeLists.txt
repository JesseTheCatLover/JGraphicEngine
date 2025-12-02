# ThirdParty/CMakeLists.txt

# --- GLAD ---
file(GLOB GLAD_SRC "${CMAKE_CURRENT_SOURCE_DIR}/glad/src/gl.c")
add_library(glad STATIC ${GLAD_SRC})
target_include_directories(glad PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/glad/include")

# --- STB ---
file(GLOB STB_SRC "${CMAKE_CURRENT_SOURCE_DIR}/stb/src/*.cpp")
add_library(stb STATIC ${STB_SRC})
target_include_directories(stb PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/stb/include")

# --- ImGui ---
set(IMGUI_SRC
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.cpp"
)
add_library(imgui STATIC ${IMGUI_SRC})
target_include_directories(imgui
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/imgui"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends"
)
target_link_libraries(imgui PUBLIC glfw)

# Third party folder
set_target_properties(glfw stb imgui glm assimp PROPERTIES
        EXCLUDE_FROM_ALL TRUE
        FOLDER "ThirdParty"
)