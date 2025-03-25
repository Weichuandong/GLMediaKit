# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)

# Declares the project name. The project name can be accessed via ${ PROJECT_NAME},
# Since this is the top level CMakeLists.txt, the project name is also accessible
# with ${CMAKE_PROJECT_NAME} (both CMake variables are in-sync within the top level
# build script scope).
project("GLMediaKit")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
#
# In this top level CMakeLists.txt, ${CMAKE_PROJECT_NAME} is used to define
# the target library name; in the sub-module's CMakeLists.txt, ${PROJECT_NAME}
# is preferred for the same purpose.
#
# 导入FFmpeg库
set(FFMPEG_LIBS avcodec avfilter avformat avutil swscale swresample postproc)
message("ANDROID_ABI = ${ANDROID_ABI}")

foreach(LIB ${FFMPEG_LIBS})
    add_library(${LIB} SHARED IMPORTED)
    set_target_properties(${LIB} PROPERTIES
            IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ffmpeg/lib/${ANDROID_ABI}/lib${LIB}.so)
endforeach()

find_library(GLESV3-lib GLESv3)
find_library(EGL-lib EGL)
find_library(android-lib android)
find_library(log-lib log)
#find_library(SLES-lib )

# In order to load a library into your app from Java/Kotlin, you must call
# System.loadLibrary() and pass the name of the library defined here;
# for GameActivity/NativeActivity derived applications, the same library name must be
# used in the AndroidManifest.xml file.

add_library(${CMAKE_PROJECT_NAME} SHARED
        # List C/C++ source files with relative paths to this CMakeLists.txt.
        JNIPlayer.cpp

        src/RenderThread.cpp
        src/TextureManger.cpp
        src/Player.cpp
        src/SLAudioPlayer.cpp

        src/Renderer/GLRenderer.cpp
        src/Renderer/ShaderManager.cpp
        src/Renderer/ImageRenderer.cpp
        src/Renderer/OffscreenRenderer.cpp
        src/Renderer/VideoRenderer.cpp
        src/Renderer/Geometry/Geometry.cpp
        src/Renderer/Geometry/Triangle.cpp
        src/Renderer/Geometry/Square.cpp
        src/Renderer/Geometry/MovingTriangle.cpp
        src/Renderer/Geometry/RotatingTriangle.cpp

        src/EGL/EGLCore.cpp

        src/Decoder/FFmpegVideoDecoder.cpp
        src/Decoder/FFMpegAudioDecoder.cpp
        src/Demuxer/FFmpegDemuxer.cpp
        )

## include
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/3rdparty
        ${CMAKE_SOURCE_DIR}/3rdparty/ffmpeg/include
)

# Specifies libraries CMake should link to your target library. You
# can link libraries from various origins, such as libraries defined in this
# build script, prebuilt third-party libraries, or Android system libraries.
target_link_libraries(${CMAKE_PROJECT_NAME}
        # List libraries link to the target library
        jnigraphics
        OpenSLES
        ${GLESV3-lib}
        ${EGL-lib}
        ${android-lib}
        ${log-lib}
        ${FFMPEG_LIBS}
)