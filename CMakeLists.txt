

cmake_minimum_required(VERSION 3.10)

project(VideoRenderer LANGUAGES C CXX)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


# 优化编译大小
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os")

# 移除未使用的段
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")

# 强制使用静态链接运行时库（针对 MSVC）
if (MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /utf-8")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_options(/bigobj)
endif()

# 添加到你的CMakeLists.txt中，在创建可执行文件之前
if(MINGW)
    # 对于Debug构建，使用最小调试信息
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g1")
    set(CMAKE_C_FLAGS_DEBUG "-O0 -g1")

    # 对于包含大量模板的文件，完全禁用调试信息
    set_source_files_properties(
            cpp/src/CachedExprtk.cpp
            PROPERTIES
            COMPILE_FLAGS "-g0"
    )
endif()

# 查找Node.js
find_program(NODE_EXECUTABLE node)
if(NOT NODE_EXECUTABLE)
    message(FATAL_ERROR "Node.js not found. Please install Node.js to proceed.")
endif()

# 设置资源目录和生成的源文件
set(RESOURCE_DIR "${CMAKE_SOURCE_DIR}/shader")
set(GENERATED_CPP "${CMAKE_BINARY_DIR}/embedded_files.cpp")
set(GENERATE_SCRIPT "${CMAKE_SOURCE_DIR}/generate.js")

# 查找所有资源文件
file(GLOB RESOURCE_FILES "${RESOURCE_DIR}/*")

# 添加自定义命令来生成 embedded_files.cpp
add_custom_command(
    OUTPUT ${GENERATED_CPP}
    COMMAND ${NODE_EXECUTABLE} ${GENERATE_SCRIPT} ${RESOURCE_DIR} ${GENERATED_CPP}
    DEPENDS ${GENERATE_SCRIPT} ${RESOURCE_FILES}
    COMMENT "Generating embedded_files.cpp from shader using Node.js"
)


# 添加源文件


# 添加源文件
set(SOURCES
        thirdPart/glad/src/glad.c
        cpp/tinyexpr/tinyexpr.c          # 修正路径
        cpp/nanovg/nanovg.c              # 修正路径
        cpp/nanovg/nanovg_impl.c         # 修正路径
        cpp/src/ExpressTool.cpp          # 修正路径
        cpp/src/FFmpegWriter.cpp         # 修正路径
        cpp/src/RendererResource.cpp     # 修正路径
        cpp/src/TextResource.cpp         # 修正路径
        cpp/src/ImageResource.cpp        # 修正路径
        cpp/src/Camera.cpp               # 修正路径
        cpp/src/Materials.cpp            # 修正路径
        cpp/src/RenderPass.cpp           # 修正路径
        cpp/src/RenderTargetPool.cpp     # 修正路径
        cpp/src/ShaderManager.cpp        # 修正路径
        cpp/src/VideoRenderer.cpp        # 修正路径
        cpp/src/TransitionRenderer.cpp   # 修正路径
        cpp/src/PluginRenderer.cpp       # 修正路径
        cpp/src/VideoResource.cpp        # 修正路径
        cpp/src/ScopedProfiler.cpp       # 修正路径
        # cpp/VideoResourceGpu.cpp       # 修正路径
        cpp/Keyframe.cpp                 # 修正路径
        cpp/TrackUtils.cpp               # 修正路径
        cpp/CoreUtils.cpp                # 修正路径
        cpp/Engine.cpp                   # 修正路径
        cpp/Main.cpp                     # 修正路径
        cpp/src/ExpressionCache.cpp
        cpp/src/ExpressionCache.h
        cpp/src/CachedExprtk.cpp
        cpp/src/CachedExprtk.h
)

# 创建可执行文件
add_executable(VideoRenderer ${SOURCES}  ${GENERATED_CPP})

set(THIRD_PARTY_DIR "${CMAKE_CURRENT_LIST_DIR}")

# 定义宏 PROJECT_ROOT_DIR，值为项目的根目录
target_compile_definitions(VideoRenderer PRIVATE PROJECT_ROOT_DIR=\"${CMAKE_SOURCE_DIR}\")

include_directories("${THIRD_PARTY_DIR}/thirdPart/glad/include")

include_directories("${THIRD_PARTY_DIR}/thirdPart/glm")

add_subdirectory("${THIRD_PARTY_DIR}/thirdPart/freetype")
target_link_libraries(VideoRenderer PRIVATE freetype)



# 添加GLFW子项目
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory("${THIRD_PARTY_DIR}/thirdPart/glfw-3.4")
target_link_libraries(VideoRenderer PRIVATE glfw)

if(WIN32)
    include_directories("${THIRD_PARTY_DIR}/thirdPart/ffmpeg/include")
    target_link_libraries(VideoRenderer PRIVATE "${THIRD_PARTY_DIR}/thirdPart/ffmpeg/lib/avcodec.lib")
    target_link_libraries(VideoRenderer PRIVATE "${THIRD_PARTY_DIR}/thirdPart/ffmpeg/lib/avformat.lib")
    target_link_libraries(VideoRenderer PRIVATE "${THIRD_PARTY_DIR}/thirdPart/ffmpeg/lib/avutil.lib")
    target_link_libraries(VideoRenderer PRIVATE "${THIRD_PARTY_DIR}/thirdPart/ffmpeg/lib/swscale.lib")

    # 复制FFmpeg DLL文件到输出目录
    set(FFMPEG_DLL_DIR "${THIRD_PARTY_DIR}/thirdPart/ffmpeg/bin")

    if(EXISTS "${FFMPEG_DLL_DIR}")
        file(GLOB FFMPEG_DLLS "${FFMPEG_DLL_DIR}/*.dll")
        foreach(dll ${FFMPEG_DLLS})
            add_custom_command(TARGET VideoRenderer POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${dll}"
                    $<TARGET_FILE_DIR:VideoRenderer>
                    COMMENT "Copying FFmpeg DLL: ${dll}"
            )
        endforeach()
    else()
        message(WARNING "FFmpeg DLL directory not found: ${FFMPEG_DLL_DIR}")
    endif()
elseif(UNIX AND NOT APPLE)
    # 使用pkg-config查找FFmpeg
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(AVCODEC REQUIRED libavcodec)
    pkg_check_modules(AVFORMAT REQUIRED libavformat)
    pkg_check_modules(AVUTIL REQUIRED libavutil)
    pkg_check_modules(SWSCALE REQUIRED libswscale)

    # 包含目录
    target_include_directories(VideoRenderer PRIVATE
            ${AVCODEC_INCLUDE_DIRS}
            ${AVFORMAT_INCLUDE_DIRS}
            ${AVUTIL_INCLUDE_DIRS}
            ${SWSCALE_INCLUDE_DIRS}
    )

    # 链接库
    target_link_libraries(VideoRenderer PRIVATE
            ${AVCODEC_LIBRARIES}
            ${AVFORMAT_LIBRARIES}
            ${AVUTIL_LIBRARIES}
            ${SWSCALE_LIBRARIES}
    )

    # 或者更简单的方式
    pkg_check_modules(FFMPEG REQUIRED
            libavcodec
            libavformat
            libavutil
            libswscale
    )

    target_include_directories(VideoRenderer PRIVATE ${FFMPEG_INCLUDE_DIRS})
    target_link_libraries(VideoRenderer PRIVATE ${FFMPEG_LIBRARIES})
endif()