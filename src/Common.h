#pragma once

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <memory>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <cmath>
#include <optional>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <limits>

#include <experimental/filesystem>
namespace std::filesystem {
    using namespace std::experimental::filesystem;
}

// Anything from vulkan/ or pxr/ is expensive to compile.
// Pre-compile them here for much faster incremental builds.

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_beta.h>
#include <vulkan/vulkan.hpp>

#include <pxr/base/gf/matrix3f.h>
#include <pxr/base/gf/matrix4f.h>
#include <pxr/imaging/hd/mesh.h>
#include <pxr/imaging/hd/vertexAdjacency.h>
#include <pxr/imaging/hd/meshUtil.h>
#include <pxr/imaging/hd/smoothNormals.h>
#include <pxr/imaging/hd/material.h>
#include <pxr/imaging/hd/camera.h>
#include <pxr/imaging/hd/extComputation.h>
#include <pxr/imaging/hd/resourceRegistry.h>
#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/imaging/hd/renderPass.h>
#include <pxr/imaging/hd/renderPassState.h>
#include <pxr/imaging/hd/rendererPlugin.h>
#include <pxr/imaging/hd/rendererPluginRegistry.h>
#include <pxr/usd/sdr/shaderNode.h>

const float PI_OVER_FOUR = 0.78539816339f;
const float PI_OVER_TWO = 1.57079632679f;
const float PI = 3.14159265359f;
const float TWO_PI = 6.28318530718f;
const float FOUR_PI = 12.5663706144f;
const float INV_PI = 0.31830988618f;
const float INV_TWO_PI = 0.15915494309f;
const float INV_FOUR_PI = 0.07957747154f;
