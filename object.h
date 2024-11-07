#pragma once
#include "ray.h"
#include "color.h"
#include <float.h>
#include <string>
#include <memory>

class Object;

//------------------------------------------------------------------------------
/**
*/
struct HitResult
{
    // hit point
    vec3 p;
    // normal
    vec3 normal;
    // hit object, or nullptr
    Object* object = nullptr;
    // intersection distance
    float t = FLT_MAX;
};

//------------------------------------------------------------------------------
/**
*/
class Object
{
public:
    Object() 
    {
    }

    virtual ~Object()
    {
    }

    virtual HitResult Intersect(Ray ray, float maxDist) { return {}; };
    virtual Color GetColor() = 0;
    virtual Ray ScatterRay(Ray ray, vec3 point, vec3 normal) { return Ray({ 0,0,0 }, {1,1,1}); };
};