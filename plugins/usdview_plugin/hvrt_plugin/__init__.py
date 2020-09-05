import sys

from pxr import Tf
from pxr.Usdviewq.plugin import PluginContainer

from . import blackboard
from . import controlPanel


def exitUsdview(usdviewApi):
    sys.exit()


_hvrtControlPanel = None
def startHvrt(usdviewApi):

    # I'm so sorry, Spiff...
    actions = usdviewApi._UsdviewApi__appController._ui.menuRendererPlugin.actions()

    activated = False
    for action in actions:
        if action.text() == "HydraVulkanRT":
            action.trigger()
            activated = True
            break
    if not activated:
        print("Could not locate 'HydraVulkanRT' Render Plugin.")
        return

    global _hvrtControlPanel
    if _hvrtControlPanel is None:
        _hvrtControlPanel = controlPanel.ControlPanel(
            usdviewApi.qMainWindow,
            usdviewApi,
            shortcuts = [
                ("Escape", exitUsdview),
                ("`", startHvrt),
                ("Shift+`", springFix),
            ])

    _hvrtControlPanel.setVisible(not _hvrtControlPanel.isVisible())


def springFix(usdviewApi):

    springPrim = usdviewApi.stage.GetPrimAtPath("/Spring_high")
    if springPrim:
        usdviewApi.dataModel.selection.setPrim(springPrim)
        # Sorry again, Spiff...
        usdviewApi._UsdviewApi__appController._frameSelection()

    # Set default clip distances to something more reasonable for small geometry.
    usdviewApi.dataModel.viewSettings.freeCamera.overrideNear = 0.01
    usdviewApi.dataModel.viewSettings.freeCamera.overrideFar = 100.0

    # Trigger a re-draw.
    usdviewApi.UpdateGUI()


class HVRTPluginContainer(PluginContainer):

    def registerPlugins(self, plugRegistry, usdviewApi):

        self._exitUsdview = plugRegistry.registerCommandPlugin(
            "HVRTPluginContainer.exitUsdview",
            "Exit Usdview",
            exitUsdview)
        self._startHvrt = plugRegistry.registerCommandPlugin(
            "HVRTPluginContainer.startHvrt",
            "Start HVRT",
            startHvrt)
        self._springFix = plugRegistry.registerCommandPlugin(
            "HVRTPluginContainer.springFix",
            "Spring Fix",
            springFix)

    def configureView(self, plugRegistry, plugUIBuilder):

        hvrtMenu = plugUIBuilder.findOrCreateMenu("HVRT")
        hvrtMenu.addItem(self._exitUsdview, "Escape")
        hvrtMenu.addItem(self._startHvrt, "`")
        hvrtMenu.addItem(self._springFix, "Shift+`")


Tf.Type.Define(HVRTPluginContainer)
