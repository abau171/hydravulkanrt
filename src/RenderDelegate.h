#pragma once

#include <Common.h>

#include <VulkanUtils.h>
#include <Mesh.h>


class HVRTRenderDelegate : public pxr::HdRenderDelegate {

public:

    HVRTRenderDelegate();

    HVRTRenderDelegate(pxr::HdRenderSettingsMap const&);

    virtual const pxr::TfTokenVector& GetSupportedRprimTypes() const;

    virtual const pxr::TfTokenVector& GetSupportedSprimTypes() const;

    virtual const pxr::TfTokenVector& GetSupportedBprimTypes() const;

    virtual pxr::HdResourceRegistrySharedPtr GetResourceRegistry() const;

    virtual pxr::HdRenderPassSharedPtr CreateRenderPass(
        pxr::HdRenderIndex* index,
        pxr::HdRprimCollection const& collection);

    virtual pxr::HdInstancer* CreateInstancer(
        pxr::HdSceneDelegate* delegate,
        pxr::SdfPath const& id,
        pxr::SdfPath const& instancerId);

    virtual void DestroyInstancer(pxr::HdInstancer* instancer);

    virtual pxr::HdRprim* CreateRprim(
        pxr::TfToken const& typeId,
        pxr::SdfPath const& rprimId,
        pxr::SdfPath const& instancerId);

    virtual void DestroyRprim(pxr::HdRprim* rPrim);

    virtual pxr::HdSprim* CreateSprim(pxr::TfToken const& typeId, pxr::SdfPath const& sprimId);

    virtual pxr::HdSprim* CreateFallbackSprim(pxr::TfToken const& typeId);

    virtual void DestroySprim(pxr::HdSprim* sprim);

    virtual pxr::HdBprim* CreateBprim(pxr::TfToken const& typeId, pxr::SdfPath const& bprimId);

    virtual pxr::HdBprim* CreateFallbackBprim(pxr::TfToken const& typeId);

    virtual void DestroyBprim(pxr::HdBprim* bprim);

    virtual void CommitResources(pxr::HdChangeTracker* tracker);

private:

    static const pxr::TfTokenVector SUPPORTED_RPRIM_TYPES;
    static const pxr::TfTokenVector SUPPORTED_SPRIM_TYPES;
    static const pxr::TfTokenVector SUPPORTED_BPRIM_TYPES;

    void init();

    void vulkanInit();

    pxr::HdResourceRegistrySharedPtr _resourceRegistry;

    vk::UniqueInstance _instance;
    vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> _debugMessenger;
    vk::UniqueDevice _device;

    VulkanBasicInfo _vkbi;

    std::unordered_set<HVRTMesh*> _meshes;
    std::unordered_map<std::string, HVRTMaterial*> _materials;

};
