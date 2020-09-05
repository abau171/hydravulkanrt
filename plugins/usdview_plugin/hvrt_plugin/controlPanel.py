import sys
import math

from PySide2 import *
from PySide2.QtCore import Qt

from . import baseControlPanel


class ControlPanel(baseControlPanel.BaseControlPanel):

    def __init__(self, mainWindow, usdviewApi, shortcuts = []):

        super().__init__(mainWindow, usdviewApi, "HVRT Control Panel", shortcuts)

        self._usdviewApi = usdviewApi

        self.addCheckbox("Converge", self.bbBool("converge"), initial = False)

        self.addIntInput(
            "AO Rays per Frame",
            self.bbInt("aoRaysPerFrame"),
            low = 0,
            high = 100,
            initial = 1)

        self.addFloatInput(
            "AO Max Distance",
            self.bbFloat("ao_maxdist"),
            low = 0,
            high = 100,
            initial = 100)

        self.addIntInput(
            "Lights",
            self._numLightsChanged,
            low = 0,
            high = 10,
            initial = 0)

        self._lightGroups = [None] * 10
        self._lightPhiThetas = [(0, 0)] * 10
        self._lightColorBrightnesses = [[1, 1, 1, 1]] * 10
        for i in range(10):
            self._addLightGroup(i)
        self._numLightsChanged(0)

    def _addLightColorChannel(self, channelName, lightI, channelI):
        self.addFloatInput(
            channelName,
            lambda value: self._intensityChanged(lightI, channelI, value),
            low = 0,
            high = 10 if (channelI == 3) else 1,
            initial = 1)

    def _addLightGroup(self, i):
        with self.addGroup("Light {}".format(i)) as group:
            phi = self.addFloatSlider(
                "ϕ",
                lambda value: self._phiChanged(i, value),
                low = -2 * math.pi,
                high = 2 * math.pi,
                initial = 0)
            theta = self.addFloatSlider(
                "θ",
                lambda value: self._thetaChanged(i, value),
                low = 0,
                high = math.pi,
                initial = 0)
            self.addCheckbox("Raytraced", self.bbBool("light_raytraced_{}".format(i)), initial = True)
            for j, channelName in enumerate(["R", "G", "B", "☀"]):
                self._addLightColorChannel(channelName, i, j)
        self._lightGroups[i] = group

    def _numLightsChanged(self, numLights):

        self.bbInt("numLights")(numLights)

        for i, group in enumerate(self._lightGroups):
            group.setVisible(i < numLights)

    def _writeLightV(self, i):
        phi, theta = self._lightPhiThetas[i]
        v = (
            math.cos(phi) * math.sin(theta),
            math.sin(phi) * math.sin(theta),
            math.cos(theta))
        self.bbVec3("light_v_{}".format(i))(v)

    def _phiChanged(self, i, phi):
        self._lightPhiThetas[i] = (phi, self._lightPhiThetas[i][1])
        self._writeLightV(i)

    def _thetaChanged(self, i, theta):
        self._lightPhiThetas[i] = (self._lightPhiThetas[i][0], theta)
        self._writeLightV(i)

    def _intensityChanged(self, lightI, channelI, value):
        self._lightColorBrightnesses[lightI][channelI] = value
        cb = self._lightColorBrightnesses[lightI]
        intensity = (cb[0] * cb[3], cb[1] * cb[3], cb[2] * cb[3])
        writer = self.bbVec3("light_intensity_{}".format(lightI))(intensity)
