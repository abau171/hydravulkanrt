import contextlib

from PySide2 import *
from PySide2.QtCore import Qt

from . import blackboard


class BaseControlPanel(QtWidgets.QWidget):

    def __init__(self, mainWindow, usdviewApi, title, shortcuts = []):

        super().__init__(mainWindow, Qt.Window | Qt.Dialog)

        self._usdviewApi = usdviewApi

        self.setWindowTitle(title)
        self.setMinimumSize(240, 0)

        self._mainLayout = QtWidgets.QFormLayout()
        self.setLayout(self._mainLayout)

        self._currentLayout = self._mainLayout

        self._shortcuts = shortcuts
        for shortcut, callback in shortcuts:
            self._registerShortcut(shortcut, callback)

        # with self.addGroup("Test Group"):
        #     self.addText("Test")
        #     self.addSeparator()
        #     self.addButton("Test", lambda: print("TEST"))
        # self.addCheckbox("Test", lambda checked: print(checked))
        # self.addIntInput("Test", lambda value: print(value), low = 0, high = 100)
        # self.addFloatInput("Test", lambda value: print(value), low = 0, high = 100)

    def _registerShortcut(self, shortcut, callback):
        action = QtWidgets.QAction(self)
        action.setShortcut(shortcut)
        action.triggered.connect(lambda checked: callback(self._usdviewApi))
        self.addAction(action)

    def bbBool(self, key):
        def setBlackboardBool(value):
            blackboard.setInt(key, 1 if value else 0)
            self._usdviewApi.UpdateGUI()
        return setBlackboardBool

    def bbInt(self, key):
        def setBlackboardInt(value):
            blackboard.setInt(key, value)
            self._usdviewApi.UpdateGUI()
        return setBlackboardInt

    def bbFloat(self, key):
        def setBlackboardFloat(value):
            blackboard.setFloat(key, value)
            self._usdviewApi.UpdateGUI()
        return setBlackboardFloat

    def bbVec3(self, key):
        def setBlackboardVec3(value):
            blackboard.setVec3(key, value)
            self._usdviewApi.UpdateGUI()
        return setBlackboardVec3

    @contextlib.contextmanager
    def addGroup(self, label):

        box = QtWidgets.QGroupBox(label)
        boxLayout = QtWidgets.QFormLayout()
        box.setLayout(boxLayout)

        oldLayout = self._currentLayout
        self._currentLayout = boxLayout
        yield box
        self._currentLayout = oldLayout

        self._currentLayout.addRow(box)

    def addText(self, label):
        text = QtWidgets.QLabel()
        self._currentLayout.addRow(QtWidgets.QLabel("{}:".format(label)), text)
        return text

    def addSeparator(self):
        separator = QtWidgets.QFrame()
        separator.setFrameShape(QtWidgets.QFrame.HLine)
        separator.setFrameShadow(QtWidgets.QFrame.Sunken)
        self._currentLayout.addRow(separator)

    def addButton(self, label, clickCallback):
        button = QtWidgets.QPushButton(label)
        button.clicked.connect(clickCallback)
        self._currentLayout.addRow(button)
        return button

    def addCheckbox(self, label, changeCallback, *, initial = False):
        checkbox = QtWidgets.QCheckBox(label)
        checkbox.setChecked(initial)
        checkbox.stateChanged.connect(lambda state: changeCallback(state == Qt.CheckState.Checked))
        self._currentLayout.addRow(checkbox)
        return checkbox

    def addIntInput(self, label, changeCallback, *, low, high, initial = None, step = 1):

        if initial is None:
            initial = low

        spinBox = QtWidgets.QSpinBox()
        spinBox.setRange(low, high)
        spinBox.setSingleStep(step)
        spinBox.setValue(initial)
        spinBox.valueChanged.connect(changeCallback)

        self._currentLayout.addRow(QtWidgets.QLabel("{}:".format(label)), spinBox)
        return spinBox

    def addFloatInput(
            self,
            label,
            changeCallback,
            *,
            low,
            high,
            initial = None,
            step = 1,
            decimals = 2):

        if initial is None:
            initial = low

        spinBox = QtWidgets.QDoubleSpinBox()
        spinBox.setRange(low, high)
        spinBox.setSingleStep(step)
        spinBox.setDecimals(decimals)
        spinBox.setValue(initial)
        spinBox.valueChanged.connect(changeCallback)

        self._currentLayout.addRow(QtWidgets.QLabel("{}:".format(label)), spinBox)
        return spinBox

    def addFloatSlider(
            self,
            label,
            changeCallback,
            *,
            low,
            high,
            initial = None):

        if initial is None:
            initial = low

        slider = QtWidgets.QSlider()
        slider.setOrientation(Qt.Horizontal)
        slider.setRange(0, 1000)
        slider.setSliderPosition(1000 * (initial - low) / (high - low))
        slider.valueChanged.connect(lambda value: changeCallback(value * (high - low) / 1000 + low))
        slider.BCP_getValue = lambda: changeCallback(slider.getValue() * (high - low) / 1000 + low)

        self._currentLayout.addRow(QtWidgets.QLabel("{}:".format(label)), slider)
        return slider
