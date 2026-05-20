#include "math.h"
#include <math.h>

Mat4 Mat4FromEulerYXZ(f32 yaw, f32 pitch, f32 roll) {
    Mat4 m = {0};
    f32 cy = cosf(yaw); f32 sy = sinf(yaw);
    f32 cp = cosf(pitch); f32 sp = sinf(pitch);
    f32 cr = cosf(roll); f32 sr = sinf(roll);

    m.m[0][0] = cy*cr + sy*sp*sr;
    m.m[0][1] = sr*cp;
    m.m[0][2] = -sy*cr + cy*sp*sr;
    m.m[0][3] = 0;

    m.m[1][0] = -cy*sr + sy*sp*cr;
    m.m[1][1] = cr*cp;
    m.m[1][2] = sr*sy + cy*sp*cr;
    m.m[1][3] = 0;

    m.m[2][0] = sy*cp;
    m.m[2][1] = -sp;
    m.m[2][2] = cy*cp;
    m.m[2][3] = 0;

    m.m[3][3] = 1;

    return m;
}

Mat4 Mat4Perspective(f32 fov_rad, f32 aspect, f32 near_z, f32 far_z) {
    Mat4 m = {0};
    f32 f = 1.0f / tanf(fov_rad / 2.0f);
    
    m.m[0][0] = f / aspect;
    m.m[1][1] = f;
    m.m[2][2] = (far_z + near_z) / (near_z - far_z);
    m.m[2][3] = -1.0f;
    m.m[3][2] = (2.0f * far_z * near_z) / (near_z - far_z);
    m.m[3][3] = 0.0f;
    
    return m;
}

Mat4 Mat4ViewFPS(Vec3 pos, f32 yaw, f32 pitch) {
    /* 
     * View Matrix = Rotation^-1 * Translation^-1
     * We apply Rotation then Translation to keep it relative.
     */
    Mat4 ry = Mat4FromEulerYXZ(-yaw, 0, 0);
    Mat4 rx = Mat4FromEulerYXZ(0, -pitch, 0);
    Mat4 m = Mat4Multiply(rx, ry);
    
    Vec3 invPos = { -pos.x, -pos.y, -pos.z };
    Mat4 t = Mat4Translation(invPos);
    
    return Mat4Multiply(m, t);
}

Mat4 Mat4Multiply(Mat4 a, Mat4 b) {
    Mat4 res = {0};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                res.m[i][j] += a.m[i][k] * b.m[k][j];
            }
        }
    }
    return res;
}

Mat4 Mat4Translation(Vec3 t) {
    Mat4 m = {0};
    m.m[0][0] = 1;
    m.m[1][1] = 1;
    m.m[2][2] = 1;
    m.m[3][3] = 1;
    m.m[3][0] = t.x;
    m.m[3][1] = t.y;
    m.m[3][2] = t.z;
    return m;
}

Vec3 Vec3Normalize(Vec3 v) {
    f32 mag = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    if (mag < 0.0001f) return (Vec3){0,0,0};
    return (Vec3){v.x/mag, v.y/mag, v.z/mag};
}
