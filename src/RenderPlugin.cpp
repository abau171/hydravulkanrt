#include <Common.h>

#include <RenderDelegate.h>

#include <RenderPlugin.h>


PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType) {
    HdRendererPluginRegistry::Define<HVRTRenderPlugin>();
}

PXR_NAMESPACE_CLOSE_SCOPE


HVRTRenderPlugin::HVRTRenderPlugin() {
}

pxr::HdRenderDelegate* HVRTRenderPlugin::CreateRenderDelegate() {
    return new HVRTRenderDelegate();
}

pxr::HdRenderDelegate* HVRTRenderPlugin::CreateRenderDelegate(
    pxr::HdRenderSettingsMap const& settingsMap)
{
    (void) settingsMap;
    return new HVRTRenderDelegate(settingsMap);
}

void HVRTRenderPlugin::DeleteRenderDelegate(pxr::HdRenderDelegate* renderDelegate) {
    delete renderDelegate;
}

bool HVRTRenderPlugin::IsSupported() const {
    return true;
}
