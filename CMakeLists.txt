cmake_minimum_required(VERSION 3.13)
project(calib LANGUAGES C CXX)

option(CALIB_ENABLE_OPENXR "Enable the optional OpenXR/OpenGL backend" OFF)

# Configure C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add preprocessor definitions
add_compile_definitions(
    PROJECT_NAME="calib"
)

# --- Automated Dependency Downloads at Configure Time (No Python) ---
set(LIBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs")
set(IMGUI_DIR "${LIBS_DIR}/ImGui")
set(INCLUDE_DIR "${LIBS_DIR}/include")
set(GL_DIR "${INCLUDE_DIR}/GL")
set(KHR_DIR "${INCLUDE_DIR}/KHR")

# Ensure base directories exist
file(MAKE_DIRECTORY "${LIBS_DIR}")
file(MAKE_DIRECTORY "${INCLUDE_DIR}")
file(MAKE_DIRECTORY "${GL_DIR}")
file(MAKE_DIRECTORY "${KHR_DIR}")

# 1. Dear ImGui
if(NOT EXISTS "${IMGUI_DIR}/imgui.cpp")
    message(STATUS "Downloading Dear ImGui...")
    file(DOWNLOAD "https://github.com/ocornut/imgui/archive/refs/tags/v1.92.8.zip" "${LIBS_DIR}/imgui.zip" SHOW_PROGRESS)
    message(STATUS "Extracting Dear ImGui...")
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar -xf imgui.zip WORKING_DIRECTORY "${LIBS_DIR}")
    file(RENAME "${LIBS_DIR}/imgui-1.92.8" "${IMGUI_DIR}")
    file(REMOVE "${LIBS_DIR}/imgui.zip")
endif()

# 2. Native File Dialog Extended
if(NOT EXISTS "${IMGUI_DIR}/nfd.h")
    message(STATUS "Downloading Native File Dialog Extended...")
    file(DOWNLOAD "https://github.com/btzy/nativefiledialog-extended/archive/refs/tags/v1.3.0.zip" "${LIBS_DIR}/nfd.zip" SHOW_PROGRESS)
    message(STATUS "Extracting Native File Dialog Extended...")
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar -xf nfd.zip WORKING_DIRECTORY "${LIBS_DIR}")
    set(NFD_SRC "${LIBS_DIR}/nativefiledialog-extended-1.3.0/src")
    file(COPY "${NFD_SRC}/include/nfd.h" DESTINATION "${IMGUI_DIR}")
    file(COPY "${NFD_SRC}/include/nfd.hpp" DESTINATION "${IMGUI_DIR}")
    file(COPY "${NFD_SRC}/nfd_win.cpp" DESTINATION "${IMGUI_DIR}")
    file(COPY "${NFD_SRC}/nfd_cocoa.m" DESTINATION "${IMGUI_DIR}")
    file(COPY "${NFD_SRC}/nfd_gtk.cpp" DESTINATION "${IMGUI_DIR}")
    file(REMOVE_RECURSE "${LIBS_DIR}/nativefiledialog-extended-1.3.0")
    file(REMOVE "${LIBS_DIR}/nfd.zip")
endif()

# 3. picojson
if(NOT EXISTS "${INCLUDE_DIR}/picojson.h")
    message(STATUS "Downloading picojson...")
    file(DOWNLOAD "https://github.com/kazuho/picojson/archive/refs/tags/v1.3.0.zip" "${LIBS_DIR}/picojson.zip" SHOW_PROGRESS)
    message(STATUS "Extracting picojson...")
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar -xf picojson.zip WORKING_DIRECTORY "${LIBS_DIR}")
    file(COPY "${LIBS_DIR}/picojson-1.3.0/picojson.h" DESTINATION "${INCLUDE_DIR}")
    file(REMOVE_RECURSE "${LIBS_DIR}/picojson-1.3.0")
    file(REMOVE "${LIBS_DIR}/picojson.zip")
endif()

# 4. OpenGL & KHR headers
if(NOT EXISTS "${GL_DIR}/glcorearb.h")
    message(STATUS "Downloading OpenGL & KHR headers...")
    set(UA_HEADER "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36")
    file(DOWNLOAD "https://registry.khronos.org/OpenGL/api/GL/glcorearb.h" "${GL_DIR}/glcorearb.h" HTTPHEADER "${UA_HEADER}")
    file(DOWNLOAD "https://registry.khronos.org/OpenGL/api/GL/glext.h" "${GL_DIR}/glext.h" HTTPHEADER "${UA_HEADER}")
    file(DOWNLOAD "https://registry.khronos.org/OpenGL/api/GL/wglext.h" "${GL_DIR}/wglext.h" HTTPHEADER "${UA_HEADER}")
    file(DOWNLOAD "https://registry.khronos.org/OpenGL/api/GL/glxext.h" "${GL_DIR}/glxext.h" HTTPHEADER "${UA_HEADER}")
    file(DOWNLOAD "https://registry.khronos.org/EGL/api/KHR/khrplatform.h" "${KHR_DIR}/khrplatform.h" HTTPHEADER "${UA_HEADER}")
endif()

# 5. GLFW & OpenCV (Windows)
if(WIN32)
    if(NOT EXISTS "${LIBS_DIR}/glfw-3.4.bin.WIN64/lib-vc2022/glfw3.lib")
        message(STATUS "Downloading GLFW for Windows...")
        file(DOWNLOAD "https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.bin.WIN64.zip" "${LIBS_DIR}/glfw.zip" SHOW_PROGRESS)
        message(STATUS "Extracting GLFW...")
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar -xf glfw.zip WORKING_DIRECTORY "${LIBS_DIR}")
        file(REMOVE "${LIBS_DIR}/glfw.zip")
    endif()

    if(NOT EXISTS "${LIBS_DIR}/opencv/build/include/opencv2/opencv.hpp")
        message(STATUS "Downloading OpenCV for Windows...")
        file(DOWNLOAD "https://github.com/opencv/opencv/releases/download/4.13.0/opencv-4.13.0-windows.exe" "${LIBS_DIR}/opencv.exe" SHOW_PROGRESS)
        message(STATUS "Extracting OpenCV...")
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar -xf opencv.exe WORKING_DIRECTORY "${LIBS_DIR}")
        file(REMOVE "${LIBS_DIR}/opencv.exe")
    endif()
endif()

# 6. GLFW & OpenCV (macOS)
if(APPLE)
    if(NOT EXISTS "${LIBS_DIR}/glfw-3.4.bin.MACOS/lib-arm64/libglfw3.a" AND NOT EXISTS "${LIBS_DIR}/glfw-3.4.bin.MACOS/lib-x86_64/libglfw3.a")
        message(STATUS "Downloading GLFW for macOS...")
        file(DOWNLOAD "https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.bin.MACOS.zip" "${LIBS_DIR}/glfw_macos.zip" SHOW_PROGRESS)
        message(STATUS "Extracting GLFW for macOS...")
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar -xf glfw_macos.zip WORKING_DIRECTORY "${LIBS_DIR}")
        file(REMOVE "${LIBS_DIR}/glfw_macos.zip")
    endif()

    if(NOT EXISTS "${LIBS_DIR}/opencv-4.13.0/CMakeLists.txt")
        message(STATUS "Downloading OpenCV source for macOS...")
        file(DOWNLOAD "https://github.com/opencv/opencv/archive/refs/tags/4.13.0.zip" "${LIBS_DIR}/opencv_src.zip" SHOW_PROGRESS)
        message(STATUS "Extracting OpenCV source...")
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar -xf opencv_src.zip WORKING_DIRECTORY "${LIBS_DIR}")
        file(REMOVE "${LIBS_DIR}/opencv_src.zip")
    endif()
endif()

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
    OpenXrGl.cpp
    OpenXrGl.h
    Preference.cpp
    Preference.h
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

if(CALIB_ENABLE_OPENXR)
    if(NOT WIN32)
        message(FATAL_ERROR "The current OpenXR/OpenGL binding is implemented for Windows only.")
    endif()

    set(OPENXR_SDK_DIR "${LIBS_DIR}/OpenXR-SDK-release-1.1.61")
    if(NOT EXISTS "${OPENXR_SDK_DIR}/CMakeLists.txt")
        message(STATUS "Downloading OpenXR SDK v1.1.61...")
        file(DOWNLOAD
            "https://github.com/KhronosGroup/OpenXR-SDK/archive/refs/tags/release-1.1.61.zip"
            "${LIBS_DIR}/openxr.zip"
            SHOW_PROGRESS
        )
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar -xf openxr.zip WORKING_DIRECTORY "${LIBS_DIR}")
        file(REMOVE "${LIBS_DIR}/openxr.zip")
    endif()

    set(BUILD_API_LAYERS OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_CONFORMANCE_TESTS OFF CACHE BOOL "" FORCE)
    set(DYNAMIC_LOADER OFF CACHE BOOL "" FORCE)
    set(BUILD_WITH_SYSTEM_JSONCPP OFF CACHE BOOL "" FORCE)
    add_subdirectory("${OPENXR_SDK_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/openxr_sdk" EXCLUDE_FROM_ALL)
endif()

# --- Source Groups for IDEs (Visual Studio & Xcode) ---

# 1. Header Files (.h)
set(HEADER_FILES_LIST
    Buffer.h
    Calibration.h
    CamCv.h
    CamImage.h
    Camera.h
    CamMf.h
    opencv_link.h
    Compute.h
    Config.h
    Expand.h
    Framebuffer.h
    GgApp.h
    Intrinsics.h
    Menu.h
    Mesh.h
    Preference.h
    Texture.h
    gg.h
    resource.h
    parseconfig.h
)
foreach(h_file ${HEADER_FILES_LIST})
    list(APPEND HEADER_FILES_LIST "${CMAKE_CURRENT_SOURCE_DIR}/${h_file}")
endforeach()
source_group("Header Files" FILES ${HEADER_FILES_LIST})

# 2. Shader Files (.vert .frag .geom .comp)
file(GLOB SHADER_FILES_GLOB_ABS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.geom"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.comp"
)
set(SHADER_FILES_LIST ${SHADER_FILES_GLOB_ABS})
foreach(sh_file ${SHADER_FILES_GLOB_ABS})
    get_filename_component(sh_name ${sh_file} NAME)
    list(APPEND SHADER_FILES_LIST ${sh_name})
endforeach()
source_group("Shader Files" FILES ${SHADER_FILES_LIST})

# 3. Resource Files (images, 3D models, config, fonts, icons, resource script)
set(RESOURCE_FILES_LIST
    calib_config.json
    Mplus1-Regular.ttf
    castle.jpg
    initial.jpg
    sky.jpg
    gg.icns
    wakayama-univ.ico
    wakayama-univ.png
    wakayama.gif
    calib.rc
)
foreach(r_file ${RESOURCE_FILES_LIST})
    list(APPEND RESOURCE_FILES_LIST "${CMAKE_CURRENT_SOURCE_DIR}/${r_file}")
endforeach()
source_group("Resource Files" FILES ${RESOURCE_FILES_LIST})

# 4. ImGui & Native File Dialog
set(IMGUI_FILES_LIST
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
    libs/ImGui/nfd_win.cpp
    libs/ImGui/nfd_cocoa.m
    libs/ImGui/nfd_gtk.cpp
)
foreach(i_file ${IMGUI_FILES_LIST})
    list(APPEND IMGUI_FILES_LIST "${CMAKE_CURRENT_SOURCE_DIR}/${i_file}")
endforeach()
source_group("ImGui" FILES ${IMGUI_FILES_LIST})

# Prevent compilation and linking of non-code resources (like OBJ models, shaders, images)
set(NON_COMPILE_RESOURCE_FILES
    calib_config.json
    Mplus1-Regular.ttf
    castle.jpg
    initial.jpg
    sky.jpg
    gg.icns
    wakayama-univ.ico
    wakayama-univ.png
    wakayama.gif
)
foreach(r_file ${NON_COMPILE_RESOURCE_FILES})
    list(APPEND NON_COMPILE_RESOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${r_file}")
endforeach()

set_source_files_properties(
    ${HEADER_FILES_LIST}
    ${SHADER_FILES_LIST}
    ${NON_COMPILE_RESOURCE_FILES}
    PROPERTIES HEADER_FILE_ONLY TRUE
)

# Platforms configuration
if(WIN32)
    # Windows platform settings
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS UNICODE _UNICODE)
    
    # Platform sources
    file(GLOB SHADER_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/*.vert"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.frag"
    )
    
    set(ASSET_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/calib_config.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/Mplus1-Regular.ttf"
        "${CMAKE_CURRENT_SOURCE_DIR}/castle.jpg"
        "${CMAKE_CURRENT_SOURCE_DIR}/initial.jpg"
        "${CMAKE_CURRENT_SOURCE_DIR}/sky.jpg"
        "${CMAKE_CURRENT_SOURCE_DIR}/wakayama-univ.ico"
        "${CMAKE_CURRENT_SOURCE_DIR}/wakayama-univ.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/wakayama.gif"
    )
    
    set(PLATFORM_SOURCES
        libs/ImGui/nfd_win.cpp
        calib.rc
        resource.h
        CamMf.cpp
        CamMf.h
        ${SHADER_SOURCES}
        ${ASSET_FILES}
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

    if(CALIB_ENABLE_OPENXR)
        target_compile_definitions(calib PRIVATE CALIB_ENABLE_OPENXR)
        target_link_libraries(calib PRIVATE openxr_loader)
    endif()
    
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
        mfuuid
        mfreadwrite
    )
    
    # Copy Assets and DLLs to Build Directory
    file(GLOB SHADER_FILES 
        "${CMAKE_CURRENT_SOURCE_DIR}/*.vert"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.frag"
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
    include_external_msproject(INSTALL_MSI "${CMAKE_CURRENT_SOURCE_DIR}/INSTALL/INSTALL.vdproj"
        TYPE "54435603-DBB4-11D2-8724-00A0C9A8B90C"
        GUID "27EA2BED-E087-3708-B1D1-34290B2E3F34"
    )
    set_target_properties(INSTALL_MSI PROPERTIES
        EXCLUDE_FROM_ALL TRUE
    )
    add_dependencies(INSTALL_MSI calib)

elseif(APPLE)
    # macOS platform settings
    set(PLATFORM_SOURCES
        libs/ImGui/nfd_cocoa.m
    )
    
    # Configure GLFW pre-compiled binary path for macOS
    set(GLFW_MACOS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/glfw-3.4.bin.MACOS")
    set(GLFW_MACOS_INCLUDE_DIR "${GLFW_MACOS_DIR}/include")
    include_directories(${GLFW_MACOS_INCLUDE_DIR})
    
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
        set(GLFW_MACOS_LIB "${GLFW_MACOS_DIR}/lib-arm64/libglfw3.a")
    else()
        set(GLFW_MACOS_LIB "${GLFW_MACOS_DIR}/lib-x86_64/libglfw3.a")
    endif()
    
    # Fallback to universal if specific arch doesn't exist
    if(NOT EXISTS "${GLFW_MACOS_LIB}")
        set(GLFW_MACOS_LIB "${GLFW_MACOS_DIR}/lib-universal/libglfw3.a")
    endif()
    
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
        castle.jpg
        initial.jpg
        sky.jpg
        gg.icns
        wakayama-univ.ico
        wakayama-univ.png
        wakayama.gif
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
        "${GLFW_MACOS_LIB}"
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
        COMMAND install_name_tool -id @executable_path/../Frameworks/$<TARGET_FILE_NAME:opencv_world> $<TARGET_FILE_DIR:calib>/../Frameworks/$<TARGET_FILE_NAME:opencv_world>
        COMMAND install_name_tool -change $<TARGET_FILE:opencv_world> @executable_path/../Frameworks/$<TARGET_FILE_NAME:opencv_world> $<TARGET_FILE:calib>
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
