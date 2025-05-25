

cmake_minimum_required(VERSION 3.10)

project(VideoRenderer LANGUAGES C CXX)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


# set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")



# 检查编译器类型并设置正确的 UTF-8 参数
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # 对于 Clang，使用 -finput-charset 和 -fexec-charset
    add_compile_options(-finput-charset=utf-8)
    add_compile_options(-fexec-charset=utf-8)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # 对于 MSVC，使用 /utf-8
    add_compile_options(/utf-8)
endif()


# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /utf-8")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
# # 强制使用静态链接运行时库（针对 MSVC）
# if (MSVC)
#     set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
#     add_compile_options(/bigobj)
# endif()

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
set(SOURCES
    ../cpp/nanovg/nanovg.c
    ../cpp/nanovg/nanovg_impl.c
    ../cpp/src/ExpressionCache.cpp
    ../cpp/src/ExpressTool.cpp
    ../cpp/src/FFmpegWriter.cpp
    ../cpp/src/RendererResource.cpp
    ../cpp/src/TextResource.cpp
    ../cpp/src/ImageResource.cpp
    ../cpp/src/Camera.cpp
    ../cpp/src/Materials.cpp
    ../cpp/src/RenderPass.cpp
    ../cpp/src/RenderTargetPool.cpp
    ../cpp/src/ShaderManager.cpp
    ../cpp/src/VideoRenderer.cpp
    ../cpp/src/TransitionRenderer.cpp
    ../cpp/src/PluginRenderer.cpp
    ../cpp/src/VideoResource.cpp
    ../cpp/src/ScopedProfiler.cpp
    # ../cpp/VideoResourceGpu.cpp
    ../cpp/Keyframe.cpp
    ../cpp/TrackUtils.cpp
    ../cpp/CoreUtils.cpp
    ../cpp/Engine.cpp
    ../cpp/Main.cpp
)


# 创建可执行文件
add_executable(VideoRenderer ${SOURCES}  ${GENERATED_CPP})

set(THIRD_PARTY_DIR "${CMAKE_CURRENT_LIST_DIR}")

# 定义宏 PROJECT_ROOT_DIR，值为项目的根目录
target_compile_definitions(VideoRenderer PRIVATE PROJECT_ROOT_DIR=\"${CMAKE_SOURCE_DIR}\")











# cmake_minimum_required(VERSION 3.11) # FetchContent is new in version 3.11.

# include(FetchContent)

# FetchContent_Declare(
# 	glm
# 	GIT_REPOSITORY	https://github.com/g-truc/glm.git
# 	GIT_TAG 	bf71a834948186f4097caa076cd2663c69a10e1e #refs/tags/1.0.1
# )

# FetchContent_MakeAvailable(glm)

# target_link_libraries(main PRIVATE glm::glm)



# include_directories("${THIRD_PARTY_DIR}/thirdPart/glm")







include_directories("${THIRD_PARTY_DIR}/thirdPart/ffmpeg/include")


# 使用相对路径添加包含目录
include_directories("${THIRD_PARTY_DIR}/thirdPart/glew-2.2.0/include")
include_directories("${THIRD_PARTY_DIR}/thirdPart/glfw-3.4.bin.WIN64/include")
include_directories("${THIRD_PARTY_DIR}/thirdPart/glm/include")
include_directories("${THIRD_PARTY_DIR}/thirdPart/freetype/include")


# # 使用相对路径添加库目录
target_link_libraries(VideoRenderer PRIVATE "${THIRD_PARTY_DIR}/thirdPart/glew-2.2.0/lib/Release/x64/glew32.lib")
target_link_libraries(VideoRenderer PRIVATE "${THIRD_PARTY_DIR}/thirdPart/glfw-3.4.bin.WIN64/lib-vc2022/glfw3.lib")
target_link_libraries(VideoRenderer PRIVATE "${THIRD_PARTY_DIR}/thirdPart/glm/glm.lib")
target_link_libraries(VideoRenderer PRIVATE "${THIRD_PARTY_DIR}/thirdPart/freetype/freetype.lib")




target_link_libraries(VideoRenderer PRIVATE "${THIRD_PARTY_DIR}/thirdPart/ffmpeg/lib/avcodec.lib")
target_link_libraries(VideoRenderer PRIVATE "${THIRD_PARTY_DIR}/thirdPart/ffmpeg/lib/avformat.lib")
target_link_libraries(VideoRenderer PRIVATE "${THIRD_PARTY_DIR}/thirdPart/ffmpeg/lib/avutil.lib")
target_link_libraries(VideoRenderer PRIVATE "${THIRD_PARTY_DIR}/thirdPart/ffmpeg/lib/swscale.lib")



target_link_libraries(VideoRenderer PRIVATE opengl32.lib)



# 添加自定义命令，复制 glew32.dll 到可执行文件目录
add_custom_command(TARGET VideoRenderer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PROJECT_SOURCE_DIR}/thirdPart/glew-2.2.0/bin/Release/x64/glew32.dll"
    $<TARGET_FILE_DIR:VideoRenderer>
)



