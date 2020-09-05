#pragma once

#include <Common.h>

#include <VulkanUtils.h>
#include <VulkanBuffer.h>
#include <VulkanAccelerationStructure.h>
#include <Material.h>


class HVRTMesh : public pxr::HdMesh {

public:

    struct PushConstants {
        pxr::GfMatrix4f modelToNdc;
        pxr::GfMatrix4f modelToWorld;
        pxr::GfVec4f normalModelToWorld0;
        pxr::GfVec4f normalModelToWorld1;
        pxr::GfVec4f normalModelToWorld2;
        pxr::GfVec3f color;
    };

    HVRTMesh(
        pxr::SdfPath const& id,
        pxr::SdfPath const& instancerId,
        const VulkanBasicInfo& vkbi,
        const std::unordered_map<std::string, HVRTMaterial*>& materials);

    virtual ~HVRTMesh();

    pxr::HdDirtyBits GetInitialDirtyBitsMask() const override {
        return
            pxr::HdChangeTracker::DirtyPoints
            | pxr::HdChangeTracker::DirtyTopology
            | pxr::HdChangeTracker::DirtyTransform
            | pxr::HdChangeTracker::DirtyNormals
            | pxr::HdChangeTracker::DirtyMaterialId;
    }

    void Sync(
        pxr::HdSceneDelegate* sceneDelegate,
        pxr::HdRenderParam* renderParam,
        pxr::HdDirtyBits* dirtyBits,
        pxr::TfToken const &reprToken) override;

    void commitResources();

    void getASBuildInfo(
        bool* instanceChanged,
        vk::AccelerationStructureInstanceKHR* instance,
        uint64_t* scratchMemorySize);

    void buildAS(vk::CommandBuffer& commandBuffer, VulkanBuffer& scratchBuffer);

    void draw(
        vk::UniqueCommandBuffer& commandBuffer,
        vk::UniquePipelineLayout& pipelineLayout,
        pxr::GfMatrix4f& worldToNdc);

protected:

    virtual pxr::HdDirtyBits _PropagateDirtyBits(pxr::HdDirtyBits bits) const override {
        return bits;
    }

    virtual void _InitRepr(
        pxr::TfToken const& reprToken,
        pxr::HdDirtyBits* dirtyBits) override
    {
        (void) reprToken;
        (void) dirtyBits;
    }

private:

    const VulkanBasicInfo& _vkbi;

    const std::unordered_map<std::string, HVRTMaterial*>& _materials;
    HVRTMaterial* _material = nullptr;
    bool _hasColor = false;
    pxr::GfVec3f _color;

    bool _verticesChanged = false;
    pxr::VtVec3fArray _vertices;

    bool _indicesChanged = false;
    pxr::VtVec3iArray _indices;

    bool _normalsChanged = false;
    pxr::VtVec3fArray _normals;

    pxr::HdMeshTopology _topology;
    pxr::Hd_VertexAdjacency _adjacencyTable;

    bool _transformChanged = false;
    pxr::GfMatrix4f _modelToWorld;
    pxr::GfMatrix4f _normalModelToWorld;

    VulkanBuffer _vertexBuffer;
    VulkanBuffer _indexBuffer;
    VulkanBuffer _normalBuffer;

    bool _needsRebuild = false;
    bool _needsRefit = false;
    size_t _maxAsVertices = 0;
    size_t _maxAsTriangles = 0;
    VulkanAccelerationStructure _as;

};
