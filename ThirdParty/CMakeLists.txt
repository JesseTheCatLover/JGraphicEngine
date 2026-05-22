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
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/misc/cpp/imgui_stdlib.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.cpp"
)
add_library(imgui STATIC ${IMGUI_SRC})
target_include_directories(imgui
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/imgui"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends"
)
target_link_libraries(imgui PUBLIC glfw)

# --- Native File Dialog Extended ---
add_library(nfd STATIC)

target_include_directories(nfd PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/nfd-extended/include"
)

if(WIN32)
    target_sources(nfd PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/nfd-extended/src/nfd_win.cpp"
    )

elseif(APPLE)
    target_sources(nfd PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/nfd-extended/src/nfd_cocoa.m"
    )

    # Required frameworks
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(UTI_LIBRARY UniformTypeIdentifiers REQUIRED)

    # Optional, older SDKs / NFD revisions might use it; ignore if not found
    find_library(MOBILE_CORE_SERVICES_LIBRARY MobileCoreServices)

    target_link_libraries(nfd PRIVATE
            ${COCOA_LIBRARY}
            ${UTI_LIBRARY}
    )

    if(MOBILE_CORE_SERVICES_LIBRARY)
        target_link_libraries(nfd PRIVATE ${MOBILE_CORE_SERVICES_LIBRARY})
    endif()

elseif(UNIX)
    target_sources(nfd PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/nfd-extended/src/nfd_gtk.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/nfd-extended/src/nfd_portal.cpp"
    )

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK REQUIRED gtk+-3.0)

    target_include_directories(nfd PRIVATE ${GTK_INCLUDE_DIRS})
    target_link_libraries(nfd PRIVATE ${GTK_LIBRARIES})

endif()


# Third party folder
set_target_properties(glfw stb imgui glm assimp nfd PROPERTIES
        EXCLUDE_FROM_ALL TRUE
        FOLDER "ThirdParty"
)