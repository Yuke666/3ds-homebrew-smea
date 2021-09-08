#ifndef MATH_DEF
#define MATH_DEF

#include <math.h>

#define SWAP(x, y, T) do { T SWAP = x; x = y; y = SWAP; } while(0)
#define MIN(x,y) ( (x) < (y) ? (x) : (y) )
#define MAX(x,y) ((x)>(y)?(x):(y))

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
static inline Vec3 Math_Vec3MultFloat(Vec3 v, float s){ return (Vec3){v.x * s, v.y * s, v.z * s}; }
static inline float Math_Vec3Magnitude(Vec3 v) { return sqrt(v.x*v.x + v.y*v.y + v.z* v.z); }
static inline float Math_Vec3Dot(Vec3 v1, Vec3 v2){ return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z); }
static inline Vec3 Math_Vec3Cross(Vec3 v1, Vec3 v) { return (Vec3){(v1.y * v.z) - (v1.z * v.y), (v1.z * v.x) - (v1.x * v.z), (v1.x * v.y) - (v1.y * v.x) }; }

static inline Vec3 Math_LerpVec3(Vec3 a1, Vec3 a2, float t){
    return Math_Vec3AddVec3(Math_Vec3MultFloat(a1,(1.0-t)), Math_Vec3MultFloat(a2, t));
}

static inline Vec3 Math_Vec3Normalize(Vec3 v){
    float mag = Math_Vec3Magnitude(v);
    v.x /= mag;
    v.y /= mag;
    v.z /= mag;
    return v;
}

static inline Quat Math_QuatConj(Quat q){ return (Quat){ -q.x, -q.y, -q.z, q.w }; }
static inline float Math_QuatMag(Quat q){ return sqrtf(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w); }

static inline Quat Math_QuatInv(Quat q){
    Quat conjugate = Math_QuatConj(q);
    double mag = pow(Math_QuatMag(q), 2);
    conjugate.x /= mag;
    conjugate.y /= mag;
    conjugate.z /= mag;
    conjugate.w /= mag;
    return conjugate;
}

static inline Quat Math_QuatNormalize(Quat q){
    float mag = Math_QuatMag(q);
    return (Quat){
        q.x / mag,
        q.y / mag,
        q.z / mag,
        q.w / mag,
    };
}

static inline Quat Math_QuatMult(Quat q1, Quat q2){
    return (Quat){
        q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y,
        q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x,
        q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w,
        q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z,
    };
}

Vec3 Math_QuatRotate(Quat q, Vec3 v);
Quat Math_Slerp(Quat qa, Quat qb, float t);
void Math_InverseMatrix(float *m);
void Math_MatrixMatrixMult(float *res, float *a, float *b);
void Math_TranslateMatrix(float *matrix, Vec3 vector);
void Math_MatrixFromQuat(Quat q, float *matrix);
void Math_ScalingMatrixXYZ(float *matrix, float x, float y, float z);
void Math_Perspective( float *matrix, float fov, float a, float n, float f);
void Math_LookAt(float *ret, Vec3 eye, Vec3 center, Vec3 up );
void Math_RotateMatrix(float *matrix, Vec3 angles);
void Math_Identity(float *matrix);
void Math_Ortho(float *matrix, float l, float r, float t, float b, float n, float f);

#endif