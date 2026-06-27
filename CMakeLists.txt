cmake_minimum_required(VERSION 3.13)
project(calib LANGUAGES C CXX)

# Configure C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add preprocessor definitions
add_compile_definitions(
    PROJECT_NAME="calib"
    GG_USE_IMGUI
)

# Common include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/ImGui
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/ImGui/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/include
)

# Common source files
set(SOURCES
    Buffer.cpp
    Buffer.h
    Calibration.cpp
    Calibration.h
    CamCv.h
    CamImage.h
    Camera.h
    Capture.cpp
    Capture.h
    Compute.h
    Config.cpp
    Config.h
    Expand.cpp
    Expand.h
    Framebuffer.cpp
    Framebuffer.h
    GgApp.cpp
    GgApp.h
    Intrinsics.cpp
    Intrinsics.h
    Menu.cpp
    Menu.h
    Mesh.h
    Preference.cpp
    Preference.h
    Scene.cpp
    Scene.h
    Texture.cpp
    Texture.h
    calib.cpp
    gg.cpp
    gg.h
    main.cpp
    
    # ImGui sources
    libs/ImGui/imgui.cpp
    libs/ImGui/imgui.h
    libs/ImGui/imgui_draw.cpp
    libs/ImGui/imgui_widgets.cpp
    libs/ImGui/imgui_tables.cpp
    libs/ImGui/imgui_demo.cpp
    libs/ImGui/backends/imgui_impl_glfw.cpp
    libs/ImGui/backends/imgui_impl_glfw.h
    libs/ImGui/backends/imgui_impl_opengl3.cpp
    libs/ImGui/backends/imgui_impl_opengl3.h
    libs/ImGui/backends/imgui_impl_opengl3_loader.h
)

# Platforms configuration
if(WIN32)
    # Windows platform settings
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    
    # Platform sources
    file(GLOB SHADER_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/*.vert"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.frag"
    )
    source_group("Shader Files" FILES ${SHADER_SOURCES})
    
    set(PLATFORM_SOURCES
        libs/ImGui/nfd_win.cpp
        calib.rc
        resource.h
        ${SHADER_SOURCES}
    )
    
    # Setup OpenCV paths (pre-built)
    set(OPENCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/opencv")
    set(OPENCV_INCLUDE_DIR "${OPENCV_DIR}/build/include")
    set(OPENCV_LIB_DIR "${OPENCV_DIR}/build/x64/vc16/lib")
    set(OPENCV_BIN_DIR "${OPENCV_DIR}/build/x64/vc16/bin")
    
    include_directories(${OPENCV_INCLUDE_DIR})
    link_directories(${OPENCV_LIB_DIR})
    
    # Setup GLFW paths (pre-built)
    set(GLFW_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/glfw-3.4.bin.WIN64")
    set(GLFW_INCLUDE_DIR "${GLFW_DIR}/include")
    set(GLFW_LIB_DIR "${GLFW_DIR}/lib-vc2022")
    
    include_directories(${GLFW_INCLUDE_DIR})
    link_directories(${GLFW_LIB_DIR})
    
    add_executable(calib ${SOURCES} ${PLATFORM_SOURCES})
    
    # Link libraries
    target_link_libraries(calib PRIVATE
        debug "${OPENCV_LIB_DIR}/opencv_world4130d.lib"
        optimized "${OPENCV_LIB_DIR}/opencv_world4130.lib"
        "${GLFW_LIB_DIR}/glfw3.lib"
        opengl32
        comctl32
        ole32
        uuid
        shell32
        user32
        gdi32
        winmm
        shlwapi
        strmiids
        mf
        mfplat
    )
    
    # Copy Assets and DLLs to Build Directory
    file(GLOB SHADER_FILES 
        "${CMAKE_CURRENT_SOURCE_DIR}/*.vert"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.frag"
    )
    set(ASSET_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/calib_config.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/Mplus1-Regular.ttf"
        "${CMAKE_CURRENT_SOURCE_DIR}/axis.obj"
        "${CMAKE_CURRENT_SOURCE_DIR}/axis.mtl"
        "${CMAKE_CURRENT_SOURCE_DIR}/bunny.obj"
        "${CMAKE_CURRENT_SOURCE_DIR}/bunny.mtl"
        "${CMAKE_CURRENT_SOURCE_DIR}/castle.jpg"
        "${CMAKE_CURRENT_SOURCE_DIR}/initial.jpg"
        "${CMAKE_CURRENT_SOURCE_DIR}/sky.jpg"
        "${CMAKE_CURRENT_SOURCE_DIR}/wakayama-univ.ico"
        "${CMAKE_CURRENT_SOURCE_DIR}/wakayama-univ.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/wakayama.gif"
    )
    
    add_custom_command(TARGET calib POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SHADER_FILES}
            ${ASSET_FILES}
            $<TARGET_FILE_DIR:calib>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<IF:$<CONFIG:Debug>,${OPENCV_BIN_DIR}/opencv_world4130d.dll,${OPENCV_BIN_DIR}/opencv_world4130.dll>"
            $<TARGET_FILE_DIR:calib>
        COMMENT "Copying assets and OpenCV DLL to build folder..."
    )
    
    # Configure Visual Studio user environment settings for debugging
    set_target_properties(calib PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:calib>"
        VS_DEBUGGER_ENVIRONMENT "PATH=$<TARGET_FILE_DIR:calib>;%PATH%"
    )
    
    # Set calib as the Visual Studio startup project
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT calib)
    
    # Include Visual Studio Installer Project
    include_external_msproject(INSTALL_MSI "${CMAKE_CURRENT_SOURCE_DIR}/INSTALL/INSTALL.vdproj")

elseif(APPLE)
    # macOS platform settings
    set(PLATFORM_SOURCES
        libs/ImGui/nfd_cocoa.m
    )
    
    # Build GLFW from source
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(libs/glfw-3.4 EXCLUDE_FROM_ALL)
    
    # Build OpenCV from source (optimized modules to compile quickly)
    set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
    set(BUILD_opencv_world ON CACHE BOOL "" FORCE)
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_PERF_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_apps OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_python2 OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_python3 OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_java OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_calib3d ON CACHE BOOL "" FORCE)
    set(BUILD_opencv_features2d ON CACHE BOOL "" FORCE)
    set(BUILD_opencv_flann ON CACHE BOOL "" FORCE)
    set(BUILD_opencv_highgui OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_imgcodecs ON CACHE BOOL "" FORCE)
    set(BUILD_opencv_imgproc ON CACHE BOOL "" FORCE)
    set(BUILD_opencv_ml OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_objdetect ON CACHE BOOL "" FORCE)
    set(BUILD_opencv_photo OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_video ON CACHE BOOL "" FORCE)
    set(BUILD_opencv_videoio ON CACHE BOOL "" FORCE)
    set(OPENCV_SKIP_VISIBILITY_HIDDEN ON CACHE BOOL "" FORCE)
    add_subdirectory(libs/opencv-4.13.0 EXCLUDE_FROM_ALL)
    
    # Target Resources (to be bundled in App Bundle)
    file(GLOB SHADER_RESOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/*.vert"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.frag"
    )
    set(APP_RESOURCES
        calib_config.json
        Mplus1-Regular.ttf
        axis.obj
        axis.mtl
        bunny.obj
        bunny.mtl
        castle.jpg
        initial.jpg
        sky.jpg
        gg.icns
        ${SHADER_RESOURCES}
    )
    
    # Set resources property so they are included in Xcode's compile phases
    set_source_files_properties(${APP_RESOURCES} PROPERTIES
        MACOSX_BUNDLE_LIMIT_TO_OUTPUT TRUE
        HEADER_FILE_ONLY TRUE
    )
    
    # Create the macOS App Bundle
    add_executable(calib MACOSX_BUNDLE ${SOURCES} ${PLATFORM_SOURCES} ${APP_RESOURCES})
    
    # Set target frameworks and link libraries
    target_link_libraries(calib PRIVATE
        opencv_world
        glfw
        "-framework CoreFoundation"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework OpenGL"
        "-framework AppKit"
    )
    
    # Configure macOS App Bundle properties
    set_target_properties(calib PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
        MACOSX_BUNDLE_BUNDLE_NAME "calib"
        RESOURCE "${APP_RESOURCES}"
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "@executable_path/../Frameworks"
    )
    
    # Copy shared libraries (dylibs) to Frameworks inside the bundle and fix load paths
    add_custom_command(TARGET calib POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:calib>/../Frameworks
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:opencv_world> $<TARGET_FILE_DIR:calib>/../Frameworks/
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:glfw> $<TARGET_FILE_DIR:calib>/../Frameworks/
        COMMAND install_name_tool -id @executable_path/../Frameworks/$<TARGET_FILE_NAME:opencv_world> $<TARGET_FILE_DIR:calib>/../Frameworks/$<TARGET_FILE_NAME:opencv_world>
        COMMAND install_name_tool -id @executable_path/../Frameworks/$<TARGET_FILE_NAME:glfw> $<TARGET_FILE_DIR:calib>/../Frameworks/$<TARGET_FILE_NAME:glfw>
        COMMAND install_name_tool -change $<TARGET_FILE:opencv_world> @executable_path/../Frameworks/$<TARGET_FILE_NAME:opencv_world> $<TARGET_FILE:calib>
        COMMAND install_name_tool -change $<TARGET_FILE:glfw> @executable_path/../Frameworks/$<TARGET_FILE_NAME:glfw> $<TARGET_FILE:calib>
        COMMENT "Packaging dynamic libraries inside the macOS App Bundle..."
    )

else()
    # Ubuntu Linux platform settings
    set(PLATFORM_SOURCES
        libs/ImGui/nfd_gtk.cpp
    )
    
    # Find system libraries
    find_package(OpenCV REQUIRED)
    
    find_package(PkgConfig REQUIRED)
    pkg_search_module(GLFW REQUIRED glfw3)
    pkg_search_module(GTK3 REQUIRED gtk+-3.0)
    
    add_executable(calib ${SOURCES} ${PLATFORM_SOURCES})
    
    # Include paths for system packages
    target_include_directories(calib PRIVATE
        ${OpenCV_INCLUDE_DIRS}
        ${GLFW_INCLUDE_DIRS}
        ${GTK3_INCLUDE_DIRS}
    )
    
    # Link libraries
    target_link_libraries(calib PRIVATE
        ${OpenCV_LIBS}
        ${GLFW_LIBRARIES}
        ${GTK3_LIBRARIES}
        GL
        dl
        pthread
    )
endif()
