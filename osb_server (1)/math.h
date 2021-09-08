#ifndef MATH_DEF
#define MATH_DEF

#include <math.h>

typedef struct {
    float x;
    float y;
    float w;
    float h;
} Rect2D;

typedef struct {
    float x;
    float y;
} Vec2;

typedef struct {
    float x;
    float y;
    float z;
} Vec3;

typedef struct {
    float x;
    float y;
    float z;
    float w;
} Vec4;

typedef struct {
    float x;
    float y;
    float z;
    float w;
} Quat;

static inline Vec3 Math_Vec3AddVec3(Vec3 v1, Vec3 v2){ return (Vec3){v1.x + v2.x, v1.y + v2.y, v1.z + v2.z}; }
static inline Vec3 Math_Vec3SubVec3(Vec3 v1, Vec3 v2){ return (Vec3){v1.x - v2.x, v1.y - v2.y, v1.z - v2.z}; }
static inline float Math_Vec3Magnitude(Vec3 v) { return sqrt(v.x*v.x + v.y*v.y + v.z* v.z); }
static inline float Math_Vec3Dot(Vec3 v1, Vec3 v2){ return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z); }
static inline Vec3 Math_Vec3Cross(Vec3 v1, Vec3 v) { return (Vec3){(v1.y * v.z) - (v1.z * v.y), (v1.z * v.x) - (v1.x * v.z), (v1.x * v.y) - (v1.y * v.x) }; }

static inline Vec3 Math_Vec3Normalize(Vec3 v){
    float mag = Math_Vec3Magnitude(v);
    v.x /= mag;
    v.y /= mag;
    v.z /= mag;
    return v;
}

#endif