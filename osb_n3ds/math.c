#include <string.h>
#include "math.h"

Vec3 Math_QuatRotate(Quat q, Vec3 v){

    Quat q1 = Math_QuatMult(q, (Quat){v.x, v.y, v.z, 0});

    q1 = Math_QuatMult(q1, (Quat){-q.x, -q.y, -q.z, q.w});

    return (Vec3){q1.x, q1.y, q1.z};
}

Quat Math_Slerp(Quat qa, Quat qb, float t){

    Quat qm;
    
    float cosHalfTheta = qa.w*qb.w + qa.x*qb.x + qa.y*qb.y + qa.z*qb.z;
    
    if(cosHalfTheta < 0){
        qb.x *= -1; qb.y *= -1; qb.z *= -1; qb.w *= -1;
        cosHalfTheta *= -1; 
    }
    
    if(fabs(cosHalfTheta) >= 1.0){
        qm.w = qa.w; 
        qm.x = qa.x; 
        qm.y = qa.y;
        qm.z = qa.z;
        return qm;
    }
    
    float halfTheta = acos(cosHalfTheta);
    float sinHalfTheta = sin(halfTheta);
    
    if(fabs(sinHalfTheta) < 0.001){
        qm.w = (qa.w * 0.5 + qb.w * 0.5);
        qm.x = (qa.x * 0.5 + qb.x * 0.5);
        qm.y = (qa.y * 0.5 + qb.y * 0.5);
        qm.z = (qa.z * 0.5 + qb.z * 0.5);
        return qm;
    }
    
    float rA = sin((1-t) * halfTheta) / sinHalfTheta;
    float rB = sin(t * halfTheta) / sinHalfTheta;

    qm.x = (qa.x * rA + qb.x * rB);
    qm.y = (qa.y * rA + qb.y * rB);
    qm.z = (qa.z * rA + qb.z * rB);
    qm.w = (qa.w * rA + qb.w * rB);
 
    return qm;
}

void Math_MatrixFromQuat(Quat q, float *matrix){
    float qx = q.x;
    float qy = q.y;
    float qz = q.z;
    float qw = q.w;
    float n = 1.0f/sqrt(qx*qx+qy*qy+qz*qz+qw*qw);

    qx *= n; qy *= n; qz *= n; qw *= n;

    matrix[0] = 1.0f - 2.0f*qy*qy - 2.0f*qz*qz;
    matrix[1] = 2.0f*qx*qy - 2.0f*qz*qw;
    matrix[2] = 2.0f*qx*qz + 2.0f*qy*qw;
    matrix[3] = 0.0f;
    matrix[4] = 2.0f*qx*qy + 2.0f*qz*qw;
    matrix[5] = 1.0f - 2.0f*qx*qx - 2.0f*qz*qz;
    matrix[6] = 2.0f*qy*qz - 2.0f*qx*qw;
    matrix[7] = 0.0f;
    matrix[8] = 2.0f*qx*qz - 2.0f*qy*qw;
    matrix[9] = 2.0f*qy*qz + 2.0f*qx*qw;
    matrix[10] = 1.0f - 2.0f*qx*qx - 2.0f*qy*qy;
    matrix[11] = 0.0f;
    matrix[12] = 0.0f;
    matrix[13] = 0.0f;
    matrix[14] = 0.0f;
    matrix[15] = 1.0f;
}

void Math_TranslateMatrix(float *matrix, Vec3 v){
    matrix[0] = 1;
    matrix[1] = 0;
    matrix[2] = 0;
    matrix[3] = v.x;
    matrix[4] = 0;
    matrix[5] = 1;
    matrix[6] = 0;
    matrix[7] = v.y;
    matrix[8] = 0;
    matrix[9] = 0;
    matrix[10] = 1;
    matrix[11] = v.z;
    matrix[12] = 0;
    matrix[13] = 0;
    matrix[14] = 0;
    matrix[15] = 1;
}

void Math_MatrixMatrixMult(float *res, float *a, float *b){
    
    float m[16];

    m[0] = (a[0]  * b[0]) + (a[1]  *  b[4]) + (a[2]  * b[8])  + (a[3]  * b[12]);
    m[1] = (a[0]  * b[1]) + (a[1]  *  b[5]) + (a[2]  * b[9])  + (a[3]  * b[13]);
    m[2] = (a[0]  * b[2]) + (a[1]  *  b[6]) + (a[2]  * b[10]) + (a[3]  * b[14]);
    m[3] = (a[0]  * b[3]) + (a[1]  *  b[7]) + (a[2]  * b[11]) + (a[3]  * b[15]);
    m[4] = (a[4]  * b[0]) + (a[5]  *  b[4]) + (a[6]  * b[8])  + (a[7]  * b[12]);
    m[5] = (a[4]  * b[1]) + (a[5]  *  b[5]) + (a[6]  * b[9])  + (a[7]  * b[13]);
    m[6] = (a[4]  * b[2]) + (a[5]  *  b[6]) + (a[6]  * b[10]) + (a[7]  * b[14]);
    m[7] = (a[4]  * b[3]) + (a[5]  *  b[7]) + (a[6]  * b[11]) + (a[7]  * b[15]);
    m[8] = (a[8]  * b[0]) + (a[9]  *  b[4]) + (a[10] * b[8])  + (a[11] * b[12]);
    m[9] = (a[8]  * b[1]) + (a[9]  *  b[5]) + (a[10] * b[9])  + (a[11] * b[13]);
    m[10] = (a[8]  * b[2]) + (a[9]  *  b[6]) + (a[10] * b[10]) + (a[11] * b[14]);
    m[11] = (a[8]  * b[3]) + (a[9]  *  b[7]) + (a[10] * b[11]) + (a[11] * b[15]);
    m[12] = (a[12] * b[0]) + (a[13] *  b[4]) + (a[14] * b[8])  + (a[15] * b[12]);
    m[13] = (a[12] * b[1]) + (a[13] *  b[5]) + (a[14] * b[9])  + (a[15] * b[13]);
    m[14] = (a[12] * b[2]) + (a[13] *  b[6]) + (a[14] * b[10]) + (a[15] * b[14]);
    m[15] = (a[12] * b[3]) + (a[13] *  b[7]) + (a[14] * b[11]) + (a[15] * b[15]);

    memcpy(res, m, sizeof(float) * 16);
}

void Math_InverseMatrix(float *m){

    float m00 = m[ 0], m01 = m[ 1], m02 = m[ 2], m03 = m[ 3];
    float m10 = m[ 4], m11 = m[ 5], m12 = m[ 6], m13 = m[ 7];
    float m20 = m[ 8], m21 = m[ 9], m22 = m[10], m23 = m[11];
    float m30 = m[12], m31 = m[13], m32 = m[14], m33 = m[15];

    m[ 0] = m12*m23*m31 - m13*m22*m31 + m13*m21*m32 - m11*m23*m32 - m12*m21*m33 + m11*m22*m33;
    m[ 1] = m03*m22*m31 - m02*m23*m31 - m03*m21*m32 + m01*m23*m32 + m02*m21*m33 - m01*m22*m33;
    m[ 2] = m02*m13*m31 - m03*m12*m31 + m03*m11*m32 - m01*m13*m32 - m02*m11*m33 + m01*m12*m33;
    m[ 3] = m03*m12*m21 - m02*m13*m21 - m03*m11*m22 + m01*m13*m22 + m02*m11*m23 - m01*m12*m23;
    m[ 4] = m13*m22*m30 - m12*m23*m30 - m13*m20*m32 + m10*m23*m32 + m12*m20*m33 - m10*m22*m33;
    m[ 5] = m02*m23*m30 - m03*m22*m30 + m03*m20*m32 - m00*m23*m32 - m02*m20*m33 + m00*m22*m33;
    m[ 6] = m03*m12*m30 - m02*m13*m30 - m03*m10*m32 + m00*m13*m32 + m02*m10*m33 - m00*m12*m33;
    m[ 7] = m02*m13*m20 - m03*m12*m20 + m03*m10*m22 - m00*m13*m22 - m02*m10*m23 + m00*m12*m23;
    m[ 8] = m11*m23*m30 - m13*m21*m30 + m13*m20*m31 - m10*m23*m31 - m11*m20*m33 + m10*m21*m33;
    m[ 9] = m03*m21*m30 - m01*m23*m30 - m03*m20*m31 + m00*m23*m31 + m01*m20*m33 - m00*m21*m33;
    m[10] = m01*m13*m30 - m03*m11*m30 + m03*m10*m31 - m00*m13*m31 - m01*m10*m33 + m00*m11*m33;
    m[11] = m03*m11*m20 - m01*m13*m20 - m03*m10*m21 + m00*m13*m21 + m01*m10*m23 - m00*m11*m23;
    m[12] = m12*m21*m30 - m11*m22*m30 - m12*m20*m31 + m10*m22*m31 + m11*m20*m32 - m10*m21*m32;
    m[13] = m01*m22*m30 - m02*m21*m30 + m02*m20*m31 - m00*m22*m31 - m01*m20*m32 + m00*m21*m32;
    m[14] = m02*m11*m30 - m01*m12*m30 - m02*m10*m31 + m00*m12*m31 + m01*m10*m32 - m00*m11*m32;
    m[15] = m01*m12*m20 - m02*m11*m20 + m02*m10*m21 - m00*m12*m21 - m01*m10*m22 + m00*m11*m22;
    float det = (m00 * m[0]) + (m01 * m[4]) + (m02 * m[8]) + (m03 * m[12]); 

    int k;
    for(k = 0; k < 16; k++) m[k] *= 1 / det;
}

void Math_ScalingMatrixXYZ(float *matrix, float x, float y, float z){
    matrix[0] = x;
    matrix[1] = 0;
    matrix[2] = 0;
    matrix[3] = 0;
    matrix[4] = 0;
    matrix[5] = y;
    matrix[6] = 0;
    matrix[7] = 0;
    matrix[8] = 0;
    matrix[9] = 0;
    matrix[10] = z;
    matrix[11] = 0;
    matrix[12] = 0;
    matrix[13] = 0;
    matrix[14] = 0;
    matrix[15] = 1;
}

void Math_Perspective(float *matrix, float fov, float a, float n, float f){

    float tanHalfFov = tan(fov * 0.5f);

    matrix[0] = 1.0 / tanHalfFov;
    matrix[1] = 0;
    matrix[2] = 0;
    matrix[3] = 0;
    matrix[4] = 0;
    matrix[5] = 1.0f / (tanHalfFov * a);
    matrix[6] = 0;
    matrix[7] = 0;
    matrix[8] = 0;
    matrix[9] = 0;
    matrix[10] = 0;
    matrix[11] = (n + f) / (n - f);
    matrix[12] = (2 * n * f) / (n - f);
    matrix[13] = 0;
    matrix[14] = -1.0;
    matrix[15] = 0;

    float mat[16];

    Math_Identity(mat);
    mat[11] = 0.5;
    mat[12] = -0.5;

    Math_MatrixMatrixMult(matrix, matrix, mat);

    Math_RotateMatrix(mat, (Vec3){0,0,-M_PI/2});
    Math_MatrixMatrixMult(matrix, matrix, mat);
}

void Math_LookAt(float *ret, Vec3 eye, Vec3 center, Vec3 up){

    Vec3 z = Math_Vec3Normalize(Math_Vec3SubVec3(eye, center));  // Forward
    Vec3 x = Math_Vec3Normalize(Math_Vec3Cross(up, z)); // Right
    Vec3 y = Math_Vec3Cross(z, x);

    ret[0] = x.x;
    ret[1] = x.y;
    ret[2] = x.z;
    ret[3] = -(Math_Vec3Dot(x, eye));
    ret[4] = y.x;
    ret[5] = y.y;
    ret[6] = y.z;
    ret[7] = -(Math_Vec3Dot(y, eye));
    ret[8] = z.x;
    ret[9] = z.y;
    ret[10] = z.z;
    ret[11] = -(Math_Vec3Dot(z, eye));
    ret[12] = 0;
    ret[13] = 0;
    ret[14] = 0;
    ret[15] = 1;
}

void Math_RotateMatrix(float *matrix, Vec3 angles){

    float sx = sin(angles.x);
    float cx = cos(angles.x);
    float sy = sin(angles.y);
    float cy = cos(angles.y);
    float sz = sin(angles.z);
    float cz = cos(angles.z);

    matrix[0] = cy*cz;
    matrix[1] = (-cy*sz*cx) + (sy*sx);
    matrix[2] = (cy*sz*sx) + (sy*cx);
    matrix[3] = 0;
    matrix[4] = sz;
    matrix[5] = cz*cx;
    matrix[6] = -cz*sx;
    matrix[7] = 0;
    matrix[8] = -sy*cz;
    matrix[9] = (sy*sz*cx) + (cy*sx);
    matrix[10] = (-sy*sz*sx) + (cy*cx);
    matrix[11] = 0;
    matrix[12] = 0;
    matrix[13] = 0;
    matrix[14] = 0;
    matrix[15] = 1;
}

void Math_Identity(float *matrix){
    matrix[0] = 1;
    matrix[1] = 0;
    matrix[2] = 0;
    matrix[3] = 0;
    matrix[4] = 0;
    matrix[5] = 1;
    matrix[6] = 0;
    matrix[7] = 0;
    matrix[8] = 0;
    matrix[9] = 0;
    matrix[10] = 1;
    matrix[11] = 0;
    matrix[12] = 0;
    matrix[13] = 0;
    matrix[14] = 0;
    matrix[15] = 1;
}

void Math_Ortho(float *matrix, float l, float r, float t, float b, float n, float f){

    matrix[0] = 2/(r-l);    matrix[1] = 0;           matrix[2] = 0;            matrix[3] = -((r+l)/(r-l));
    matrix[4] = 0;          matrix[5] = 2/(t-b);     matrix[6] = 0;            matrix[7] = -((t+b)/(t-b));
    matrix[8] = 0;          matrix[9] = 0;           matrix[10] = 1/(f-n);     matrix[11] = n/(f-n);
    matrix[12] = 0;         matrix[13] = 0;          matrix[14] = 0;           matrix[15] =  1;

    float mat[16];

    Math_Identity(mat);

    mat[0] = 0;
    mat[1] = 1;
    mat[4] = -1;
    mat[5] = 0;

    Math_MatrixMatrixMult(matrix, mat, matrix);
}