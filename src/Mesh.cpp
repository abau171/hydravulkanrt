#include <Common.h>

#include <VulkanUtils.h>

#include <Mesh.h>


HVRTMesh::HVRTMesh(
    pxr::SdfPath const& id,
    pxr::SdfPath const& instancerId,
    const VulkanBasicInfo& vkbi,
    const std::unordered_map<std::string, HVRTMaterial*>& materials)
    : HdMesh(id, instancerId),
      _vkbi(vkbi),
      _materials(materials),
      _vertexBuffer(_vkbi),
      _indexBuffer(_vkbi),
      _normalBuffer(_vkbi),
      _as(_vkbi)
{
}

HVRTMesh::~HVRTMesh() {
}

void HVRTMesh::Sync(
    pxr::HdSceneDelegate* sceneDelegate,
    pxr::HdRenderParam* renderParam,
    pxr::HdDirtyBits* dirtyBits,
    pxr::TfToken const &reprToken)
{
    (void) renderParam;
    (void) reprToken;

    const pxr::SdfPath& id = GetId();

    if (*dirtyBits & pxr::HdChangeTracker::DirtyMaterialId) {
        auto it = _materials.find(sceneDelegate->GetMaterialId(id).GetString());
        _material = nullptr;
        if (it != _materials.end()) {
            _material = it->second;
        }
        _hasColor = false;
    }

    if (!_hasColor) {

        if (_material && _material->hasColor()) {

            _color = _material->getColor();

        } else {

            _color = pxr::GfVec3f(0.7f); // Default color if no constant displayColor exists.

            pxr::HdPrimvarDescriptorVector primvars =
                GetPrimvarDescriptors(sceneDelegate, pxr::HdInterpolationConstant);
            for (const pxr::HdPrimvarDescriptor& primvar: primvars) {
                if (primvar.name == pxr::HdTokens->displayColor) {

                    // It would be nice to just GetPrimvar(sceneDelegate, pxr::HdTokens->displayColor),
                    // but we have to check that it's in the primvars vector or errors may occur.
                    pxr::VtValue displayColorValue = GetPrimvar(sceneDelegate, primvar.name);
                    if (displayColorValue.IsHolding<pxr::VtVec3fArray>()) {
                        pxr::VtVec3fArray displayColorVector =
                            displayColorValue.Get<pxr::VtVec3fArray>();
                        if (displayColorVector.size() == 1) {
                            _color = displayColorVector[0];
                        }
                    }

                    break;
                }
            }
        }

        _hasColor = true;
    }

    bool recomputeNormals = false;

    if (*dirtyBits & pxr::HdChangeTracker::DirtyPoints) {

        pxr::VtValue value = sceneDelegate->Get(id, pxr::HdTokens->points);
        _vertices = value.Get<pxr::VtVec3fArray>();
        _verticesChanged = true;

        recomputeNormals = true;
    }

    if (*dirtyBits & pxr::HdChangeTracker::DirtyTopology) {

        _topology = GetMeshTopology(sceneDelegate);
        _adjacencyTable.BuildAdjacencyTable(&_topology);

        pxr::HdMeshUtil meshUtil(&_topology, id);
        pxr::VtIntArray triangleIndexToFaceIndex;
        meshUtil.ComputeTriangleIndices(&_indices, &triangleIndexToFaceIndex);
        _indicesChanged = true;

        recomputeNormals = true;
    }

    // TODO: Use authored surface normals if available.
    // if (*dirtyBits & pxr::HdChangeTracker::DirtyNormals) { }
    if (recomputeNormals) {
        _normals = pxr::Hd_SmoothNormals::ComputeSmoothNormals(
            &_adjacencyTable,
            _vertices.size(),
            _vertices.cdata());
        _normalsChanged = true;
    }

    if (*dirtyBits & pxr::HdChangeTracker::DirtyTransform) {
        _modelToWorld = pxr::GfMatrix4f(sceneDelegate->GetTransform(id));
        _normalModelToWorld = pxr::GfMatrix4f(
            _modelToWorld.ExtractRotationMatrix().GetInverse().GetTranspose(),
            pxr::GfVec3f(0.0f));
        _transformChanged = true;
    }

    *dirtyBits &= ~pxr::HdChangeTracker::AllSceneDirtyBits;
}

void HVRTMesh::commitResources() {

    // TODO: Vulkan can handle multithreaded uploads, so move these back into Sync().

    if (_verticesChanged) {
        _verticesChanged = false;

        size_t newVertexBufferSize = _vertices.size() * sizeof(pxr::GfVec3f);
        if (_vertexBuffer.size() != newVertexBufferSize) {
            _vertexBuffer.allocate(
                newVertexBufferSize,
                true,
                vk::BufferUsageFlagBits::eVertexBuffer
                | vk::BufferUsageFlagBits::eRayTracingKHR
                | vk::BufferUsageFlagBits::eShaderDeviceAddress);
        }

        std::memcpy(
            _vertexBuffer.data(),
            _vertices.data(),
            _vertexBuffer.size());

        _needsRefit = true;
    }

    if (_indicesChanged) {
        _indicesChanged = false;

        size_t newIndexBufferSize = _indices.size() * sizeof(pxr::GfVec3i);
        if (_indexBuffer.size() != newIndexBufferSize) {
            _indexBuffer.allocate(
                newIndexBufferSize,
                true,
                vk::BufferUsageFlagBits::eIndexBuffer
                | vk::BufferUsageFlagBits::eRayTracingKHR
                | vk::BufferUsageFlagBits::eShaderDeviceAddress);
        }

        std::memcpy(
            _indexBuffer.data(),
            _indices.data(),
            _indexBuffer.size());

        _needsRebuild = true;
    }

    if (_normalsChanged) {
        _normalsChanged = false;

        size_t newNormalBufferSize = _normals.size() * sizeof(pxr::GfVec3f);
        if (_normalBuffer.size() != newNormalBufferSize) {
            _normalBuffer.allocate(newNormalBufferSize, true, vk::BufferUsageFlagBits::eVertexBuffer);
        }

        std::memcpy(
            _normalBuffer.data(),
            _normals.data(),
            _normalBuffer.size());
    }

    if (_maxAsVertices < _vertices.size() || _maxAsTriangles < _indices.size()) {
        _maxAsVertices = _vertices.size();
        _maxAsTriangles = _indices.size();

        _as.allocateBottomLevel(_maxAsVertices, _maxAsTriangles);

        _needsRebuild = true;
    }
}

void HVRTMesh::getASBuildInfo(
    bool* instanceChanged,
    vk::AccelerationStructureInstanceKHR* instance,
    uint64_t* scratchMemorySize)
{
    *instanceChanged = (_needsRebuild || _transformChanged);
    _transformChanged = false;

    *instance = {
        .transform = {},
        .instanceCustomIndex = 0,
        .mask = 0xff,
        .instanceShaderBindingTableRecordOffset = 0,
        .flags = vk::GeometryInstanceFlagsKHR(),
        .accelerationStructureReference = _vkbi.device.getAccelerationStructureAddressKHR({
                .accelerationStructure = _as.getAccelerationStructure(),
            },
            _vkbi.dispatchLoader),
    };
    auto modelToWorldT = _modelToWorld.GetTranspose();
    std::memcpy(&instance->transform.matrix[0][0], modelToWorldT.data(), 12 * sizeof(float));

    *scratchMemorySize = _as.getScratchMemorySize();
}

void HVRTMesh::buildAS(vk::CommandBuffer& commandBuffer, VulkanBuffer& scratchBuffer)
{
    if (_needsRebuild || _needsRefit) {
        _as.buildBottomLevel(
            commandBuffer,
            scratchBuffer,
            _indices.size(),
            _vertexBuffer,
            _indexBuffer,
            _needsRefit && !_needsRebuild);
        _needsRebuild = false;
        _needsRefit = false;
    }
}

void HVRTMesh::draw(
    vk::UniqueCommandBuffer& commandBuffer,
    vk::UniquePipelineLayout& pipelineLayout,
    pxr::GfMatrix4f& worldToNdc)
{
    HVRTMesh::PushConstants pushConstants = {
        _modelToWorld * worldToNdc,
        _modelToWorld,
        _normalModelToWorld.GetRow(0),
        _normalModelToWorld.GetRow(1),
        _normalModelToWorld.GetRow(2),
        _color
    };
    commandBuffer->pushConstants(
        pipelineLayout.get(),
        vk::ShaderStageFlagBits::eVertex,
        0,
        sizeof(HVRTMesh::PushConstants),
        reinterpret_cast<void*>(&pushConstants));

    vk::Buffer vertexBuffers[] = { _vertexBuffer.getBuffer(), _normalBuffer.getBuffer() };
    vk::DeviceSize vertexBufferOffsets[] = { 0, 0 };
    commandBuffer->bindVertexBuffers(0, 2, vertexBuffers, vertexBufferOffsets);

    commandBuffer->bindIndexBuffer(_indexBuffer.getBuffer(), 0, vk::IndexType::eUint32);

    commandBuffer->drawIndexed(3 * _indices.size(), 1, 0, 0, 0);
}
