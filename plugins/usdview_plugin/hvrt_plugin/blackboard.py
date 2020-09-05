import ctypes
import pathlib


__all__ = ["getInt", "setInt", "getVec3", "setVec3"]


C_FLOAT_P = ctypes.POINTER(ctypes.c_float)


SO_PATH = pathlib.Path(__file__).parent.parent.parent / "libblackboard.so"
_blackboard = ctypes.cdll.LoadLibrary(SO_PATH)


_blackboard.bbGetInt.restype = ctypes.c_int
_blackboard.bbGetInt.argtypes = [ctypes.c_char_p, ctypes.c_int]
def getInt(key, defaultValue = 0):
    return _blackboard.bbGetInt(key.encode("utf-8"), defaultValue)

_blackboard.bbSetInt.restype = None
_blackboard.bbGetInt.argtypes = [ctypes.c_char_p, ctypes.c_int]
def setInt(key, value):
    _blackboard.bbSetInt(key.encode("utf-8"), value)


_blackboard.bbGetFloat.restype = ctypes.c_float
_blackboard.bbGetFloat.argtypes = [ctypes.c_char_p, ctypes.c_float]
def getFloat(key, defaultValue = 0):
    return _blackboard.bbGetFloat(key.encode("utf-8"), defaultValue)

_blackboard.bbSetFloat.restype = None
_blackboard.bbSetFloat.argtypes = [ctypes.c_char_p, ctypes.c_float]
def setFloat(key, value):
    _blackboard.bbSetFloat(key.encode("utf-8"), value)


_blackboard.bbGetVec3.restype = None
_blackboard.bbGetVec3.argtypes = [ctypes.c_char_p, C_FLOAT_P, C_FLOAT_P, C_FLOAT_P]
def getVec3(key, defaultValue = (0, 0, 1)):
    ctypesValue = tuple(ctypes.c_float(v) for v in defaultValue)
    _blackboard.bbGetVec3(key.encode("utf-8"), *ctypesValue)
    return tuple(v.value for v in ctypesValue)

_blackboard.bbSetVec3.restype = None
_blackboard.bbSetVec3.argtypes = [ctypes.c_char_p, ctypes.c_float, ctypes.c_float, ctypes.c_float]
def setVec3(key, value):
    _blackboard.bbSetVec3(key.encode("utf-8"), *value)
