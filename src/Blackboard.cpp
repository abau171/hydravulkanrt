#include <unordered_map>

#include <Blackboard.h>


static std::unordered_map<std::string, int> _intMap;

int getInt(const std::string& key, int defaultValue) {
    auto it = _intMap.find(key);
    if (it == _intMap.end()) return defaultValue;
    return it->second;
}

void setInt(const std::string& key, int value) {
    _intMap[key] = value;
}


static std::unordered_map<std::string, float> _floatMap;

float getFloat(const std::string& key, float defaultValue) {
    auto it = _floatMap.find(key);
    if (it == _floatMap.end()) return defaultValue;
    return it->second;
}

void setFloat(const std::string& key, float value) {
    _floatMap[key] = value;
}


static std::unordered_map<std::string, pxr::GfVec3f> _vec3Map;

pxr::GfVec3f getVec3(const std::string& key, pxr::GfVec3f defaultValue) {
    auto it = _vec3Map.find(key);
    if (it == _vec3Map.end()) return defaultValue;
    return it->second;
}

void setVec3(const std::string& key, pxr::GfVec3f value) {
    _vec3Map[key] = value;
}


extern "C" {


int bbGetInt(const char* key, int defaultValue) {
    return getInt(std::string(key), defaultValue);
}

void bbSetInt(const char* key, int value) {
    setInt(std::string(key), value);
}


float bbGetFloat(const char* key, float defaultValue) {
    return getFloat(std::string(key), defaultValue);
}

void bbSetFloat(const char* key, float value) {
    setFloat(std::string(key), value);
}


void bbGetVec3(const char* key, float* x, float* y, float* z) {
    pxr::GfVec3f value = getVec3(std::string(key), pxr::GfVec3f(*x, *y, *z));
    *x = value[0];
    *y = value[1];
    *z = value[2];
}

void bbSetVec3(const char* key, float x, float y, float z) {
    setVec3(std::string(key), pxr::GfVec3f(x, y, z));
}


}
