# HuanFace Camera 1.0.0 — Windows Real-Time Beauty Camera
# Fixed build: no macro conflicts, SDK as subdir, NOMINMAX/NOGDI, Qt6 optional

cmake_minimum_required(VERSION 3.20)
project(HuanFaceCamera VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Critical for Windows: avoid min/max macro, OPAQUE macro, etc.
add_definitions(-DNOMINMAX -DWIN32_LEAN_AND_MEAN -DNOGDI -D_CRT_SECURE_NO_WARNINGS -DUNICODE -D_UNICODE)

# Qt6 optional
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Widgets Core Gui QUIET)
if(Qt6_FOUND)
    message(STATUS "Qt6 found: ${Qt6_VERSION}")
    set(HUANFACE_QT ON)
else()
    message(WARNING "Qt6 not found, UI will be stubbed for CI")
    set(HUANFACE_QT OFF)
endif()

# SDK as subdirectory — builds huanface static lib with its own defines
# SDK CMakeLists already has NOMINMAX etc
set(HUANFACE_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../sdk")
if(EXISTS "${HUANFACE_SDK_DIR}/CMakeLists.txt")
    # Avoid double miniz, set options
    set(HUANFACE_BACKEND_D3D11 ON CACHE BOOL "" FORCE)
    set(HUANFACE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(HUANFACE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory(${HUANFACE_SDK_DIR} ${CMAKE_BINARY_DIR}/huanface_sdk)
    set(HUANFACE_LIB huanface)
else()
    message(WARNING "SDK not found at ${HUANFACE_SDK_DIR}, using sources directly")
    file(GLOB HUANFACE_SDK_SOURCES
        ${HUANFACE_SDK_DIR}/src/core/*.cpp
        ${HUANFACE_SDK_DIR}/src/frame/*.cpp
        ${HUANFACE_SDK_DIR}/src/bundle/*.cpp
        ${HUANFACE_SDK_DIR}/src/rendering/*.cpp
        ${HUANFACE_SDK_DIR}/src/platform/*.cpp
        ${HUANFACE_SDK_DIR}/src/image/*.cpp
        ${HUANFACE_SDK_DIR}/src/face/*.cpp
        ${HUANFACE_SDK_DIR}/src/makeup/*.cpp
        ${HUANFACE_SDK_DIR}/src/beauty/*.cpp
        ${HUANFACE_SDK_DIR}/third_party/miniz/miniz.c
    )
    if(WIN32)
        list(APPEND HUANFACE_SDK_SOURCES
            ${HUANFACE_SDK_DIR}/src/rendering/d3d11/d3d11_backend.cpp
            ${HUANFACE_SDK_DIR}/src/rendering/d3d11/d3d11_full_pipeline.cpp
        )
    endif()
    add_library(huanface STATIC ${HUANFACE_SDK_SOURCES})
    target_include_directories(huanface PUBLIC ${HUANFACE_SDK_DIR}/include ${HUANFACE_SDK_DIR}/src)
    set(HUANFACE_LIB huanface)
endif()

# Camera App sources
set(CAMERA_SOURCES
    src/main.cpp
    src/CameraManager.cpp
    src/CameraDevice.cpp
    src/HuanFaceAdapter.cpp
    src/CameraRenderer.cpp
    src/CaptureManager.cpp
)

if(HUANFACE_QT)
    list(APPEND CAMERA_SOURCES
        src/MainWindow.cpp
    )
endif()

add_executable(HuanFaceCamera ${CAMERA_SOURCES})

target_include_directories(HuanFaceCamera PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${HUANFACE_SDK_DIR}/include
    ${HUANFACE_SDK_DIR}/src
)

target_link_libraries(HuanFaceCamera PRIVATE
    ${HUANFACE_LIB}
)

if(WIN32)
    target_link_libraries(HuanFaceCamera PRIVATE
        mf mfplat mfreadwrite mfuuid
        d3d11 dxgi d3dcompiler
    )
endif()

if(HUANFACE_QT)
    target_link_libraries(HuanFaceCamera PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui)
endif()

# Output bin
set_target_properties(HuanFaceCamera PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

message(STATUS "HuanFace Camera 1.0.0")
message(STATUS "SDK Dir: ${HUANFACE_SDK_DIR}")
message(STATUS "Qt6: ${HUANFACE_QT}")
