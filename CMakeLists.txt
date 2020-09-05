cmake_minimum_required(VERSION 3.16)
project(HydraVulkanRT CXX)

find_package(Vulkan REQUIRED)
find_package(PythonLibs REQUIRED)
find_package(pxr CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS}\
    -Wall\
    -Wextra\
    -Werror\
    -Wno-error=unused-variable\
    -Wno-error=unused-function\
    -Wno-error=unused-parameter\
    -Wno-error=unused-but-set-variable\
    -Wno-deprecated")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

add_library(blackboard SHARED src/Blackboard.cpp)
target_include_directories(blackboard PRIVATE src/)

set(SOURCES
    src/VulkanUtils.cpp
    src/VulkanBuffer.cpp
    src/VulkanImage.cpp
    src/VulkanAccelerationStructure.cpp
    src/HVRTGL.cpp
    src/RenderPlugin.cpp
    src/RenderDelegate.cpp
    src/RenderPass.cpp
    src/Blitter.cpp
    src/Mesh.cpp
)
add_library(HydraVulkanRT SHARED ${SOURCES})
target_compile_definitions(HydraVulkanRT PRIVATE
    __TBB_show_deprecation_message_atomic_H # Silence annoying TBB messages from pxr includes.
    VK_ENABLE_BETA_EXTENSIONS
    GLM_ENABLE_EXPERIMENTAL
    GLM_FORCE_RADIANS
    GLM_FORCE_DEPTH_ZERO_TO_ONE)
target_include_directories(HydraVulkanRT PRIVATE
    src/
    build/pch/
    ${Vulkan_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIRS}
    ${PXR_INCLUDE_DIRS})
target_precompile_headers(HydraVulkanRT PRIVATE
    src/Common.h)
target_link_libraries(HydraVulkanRT stdc++fs Vulkan::Vulkan ${PXR_LIBRARIES} blackboard)

set(GLSL_COMPILER glslc)
set(SHADER_SOURCES
    shaders/main.vert
    shaders/main.frag
    shaders/main.rgen
    shaders/main.rchit
    shaders/main.rmiss
)
foreach(SHADER_SOURCE ${SHADER_SOURCES})
    get_filename_component(FILENAME ${SHADER_SOURCE} NAME)
    set(SHADER_SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILENAME}.spv")
    add_custom_command(
        OUTPUT ${SHADER_SPIRV}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
        COMMAND ${GLSL_COMPILER} "${PROJECT_SOURCE_DIR}/${SHADER_SOURCE}" -o ${SHADER_SPIRV} --target-env=vulkan1.2
        DEPENDS ${SHADER_SOURCE})
    list(APPEND SHADER_SPIRV_BINARIES ${SHADER_SPIRV})
endforeach(SHADER_SOURCE)
add_custom_target(Shaders DEPENDS ${SHADER_SPIRV_BINARIES})
add_dependencies(HydraVulkanRT Shaders)

configure_file(
    "${PROJECT_SOURCE_DIR}/plugins/plugInfo.json"
    "${PROJECT_BINARY_DIR}/plugInfo.json"
    COPYONLY)
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/usdview_plugin")
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/usdview_plugin/hvrt_plugin")
configure_file(
    "${PROJECT_SOURCE_DIR}/plugins/usdview_plugin/plugInfo.json"
    "${PROJECT_BINARY_DIR}/usdview_plugin/plugInfo.json"
    COPYONLY)
configure_file(
    "${PROJECT_SOURCE_DIR}/plugins/usdview_plugin/hvrt_plugin/__init__.py"
    "${PROJECT_BINARY_DIR}/usdview_plugin/hvrt_plugin/__init__.py"
    COPYONLY)
configure_file(
    "${PROJECT_SOURCE_DIR}/plugins/usdview_plugin/hvrt_plugin/blackboard.py"
    "${PROJECT_BINARY_DIR}/usdview_plugin/hvrt_plugin/blackboard.py"
    COPYONLY)
configure_file(
    "${PROJECT_SOURCE_DIR}/plugins/usdview_plugin/hvrt_plugin/controlPanel.py"
    "${PROJECT_BINARY_DIR}/usdview_plugin/hvrt_plugin/controlPanel.py"
    COPYONLY)
configure_file(
    "${PROJECT_SOURCE_DIR}/plugins/usdview_plugin/hvrt_plugin/baseControlPanel.py"
    "${PROJECT_BINARY_DIR}/usdview_plugin/hvrt_plugin/baseControlPanel.py"
    COPYONLY)
