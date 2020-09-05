#pragma once

#include <Common.h>


PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (surface)
    (diffuseColor)
);

PXR_NAMESPACE_CLOSE_SCOPE


class HVRTMaterial : public pxr::HdMaterial {

public:

    HVRTMaterial(pxr::SdfPath const& id) : pxr::HdMaterial(id), _hasColor(false) {
    }

    virtual void Sync(
        pxr::HdSceneDelegate* sceneDelegate,
        pxr::HdRenderParam* renderParam,
        pxr::HdDirtyBits* dirtyBits) override
    {
        (void) renderParam;

        if (*dirtyBits & DirtyResource) {
            pxr::VtValue vtMat = sceneDelegate->GetMaterialResource(GetId());
            if (vtMat.IsHolding<pxr::HdMaterialNetworkMap>()) {
                const pxr::HdMaterialNetworkMap& network = vtMat.Get<pxr::HdMaterialNetworkMap>();

                auto it = network.map.find(pxr::_tokens->surface);
                if (it != network.map.end()) {
                    for (const pxr::HdMaterialNode& node : it->second.nodes) {
                        for (auto it2 : node.parameters) {
                            if (it2.first != pxr::_tokens->diffuseColor) continue;
                            if (!it2.second.IsHolding<pxr::GfVec3f>()) continue;
                            _hasColor = true;
                            _color = it2.second.Get<pxr::GfVec3f>();
                        }
                    }
                }
            }
        }

        *dirtyBits = Clean;
    }

    virtual pxr::HdDirtyBits GetInitialDirtyBitsMask() const override {
        return AllDirty;
    }

    bool hasColor() const {
        return _hasColor;
    }

    pxr::GfVec3f getColor() const {
        return _color;
    }

private:

    bool _hasColor;
    pxr::GfVec3f _color;

};
