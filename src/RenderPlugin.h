#pragma once

#include <Common.h>


class HVRTRenderPlugin : public pxr::HdRendererPlugin {

public:

    HVRTRenderPlugin();

    virtual ~HVRTRenderPlugin() = default;

    virtual pxr::HdRenderDelegate* CreateRenderDelegate() override;

    virtual pxr::HdRenderDelegate* CreateRenderDelegate(
        pxr::HdRenderSettingsMap const& settingsMap) override;

    virtual void DeleteRenderDelegate(
        pxr::HdRenderDelegate* renderDelegate) override;

    virtual bool IsSupported() const override;

private:

    HVRTRenderPlugin(const HVRTRenderPlugin&) = delete;
    HVRTRenderPlugin& operator=(const HVRTRenderPlugin&) = delete;

};
