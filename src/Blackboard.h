#pragma once

#include <pxr/base/gf/vec3f.h>


int getInt(const std::string& key, int defaultValue = 0);

void setInt(const std::string& key, int value);


float getFloat(const std::string& key, float defaultValue = 0.0f);

void setFloat(const std::string& key, float value);


pxr::GfVec3f getVec3(const std::string& key, pxr::GfVec3f defaultValue = pxr::GfVec3f(0.0f));

void setVec3(const std::string& key, pxr::GfVec3f value);


extern "C" {


int bbGetInt(const char* key, int defaultValue = 0);

void bbSetInt(const char* key, int value);


float bbGetFloat(const char* key, float defaultValue = 0.0f);

void bbSetFloat(const char* key, float value);


void bbGetVec3(const char* key, float* x, float* y, float* z);

void bbSetVec3(const char* key, float x, float y, float z);


}
