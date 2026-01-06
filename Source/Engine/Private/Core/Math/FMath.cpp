//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/Math/FMath.h"
#include "Core/Math/FMatrix4.h"
#include "Core/Math/FRotator.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

// Matrix functions
FMatrix4 FMath::Identity() { return FMatrix4::Identity(); }

FMatrix4 FMath::Inverse(const FMatrix4& M) { return M.Inverse(); }

FMatrix4 FMath::Transpose(const FMatrix4& M) { return M.Transpose(); }

FMatrix4 FMath::Translate(const FVector3& T) { return FMatrix4::Translate(T); }

FMatrix4 FMath::Scale(const FVector3& S) { return FMatrix4::Scale(S); }

FMatrix4 FMath::Rotate(const FQuat& Q) { return FMatrix4::Rotate(Q); }

FMatrix4 FMath::Ortho(float Left, float Right, float Bottom, float Top, float Near, float Far)
{
    // Avoid divide-by-zero
    const float rl = Right - Left;
    const float tb = Top - Bottom;
    const float fn = Far - Near;

    if (std::fabs(rl) < 1e-6f || std::fabs(tb) < 1e-6f || std::fabs(fn) < 1e-6f)
        return FMatrix4(glm::mat4(1.0f));

    // We want:
    // ndcX from y, ndcY from z, ndcZ from x, all in [-1,1]
    // ndcX = 2*(y - (l+r)/2)/(r-l)
    // ndcY = 2*(z - (b+t)/2)/(t-b)
    // ndcZ = 2*(x - (n+f)/2)/(f-n)

    const float sx =  2.0f / rl;
    const float sy =  2.0f / tb;
    const float sz =  2.0f / fn;

    const float tx = -(Right + Left)   / rl;
    const float ty = -(Top + Bottom)   / tb;
    const float tz = -(Far + Near)   / fn;

    glm::mat4 M(1.0f);

    // clip.x from y
    M[1][0] = sx;
    M[3][0] = tx;

    // clip.y from z
    M[2][1] = sy;
    M[3][1] = ty;

    // clip.z from x
    M[0][2] = sz;
    M[3][2] = tz;

    return FMatrix4(M);
}

FMatrix4 FMath::Perspective(float FOVInDegree, float Aspect, float Near, float Far)
{
    Near = std::max(Near, 1e-6f);
    Far  = std::max(Far, Near + 1e-6f);
    Aspect = std::max(Aspect, 1e-6f);

    const float f = 1.0f / std::tan(glm::radians(FOVInDegree) * 0.5f);

    // We want:
    // clip.x = (f/aspect) * y
    // clip.y = f * z
    // clip.w = x
    //
    // And NDC z in [-1,1] with near->-1, far->+1:
    // ndcZ = (A*x + B) / x = A + B/x
    // A = (far+near)/(far-near)
    // B = -2*far*near/(far-near)

    const float A = (Far + Near) / (Far - Near);
    const float B = (-2.0f * Far * Near) / (Far - Near);

    glm::mat4 P(0.0f);

    // GLM is column-major: M[col][row]
    P[1][0] = f / Aspect;   // row0 col1  -> clip.x from y
    P[2][1] = f;            // row1 col2  -> clip.y from z

    P[0][2] = A;            // row2 col0  -> clip.z from x
    P[3][2] = B;            // row2 col3  -> clip.z constant term

    P[0][3] = 1.0f;         // row3 col0  -> clip.w = x

    return FMatrix4(P);
}

FMatrix4 FMath::LookAt(const FVector3& Eye, const FVector3& Target, const FVector3& Up)
{
    return FMatrix4(glm::lookAt(glm::vec3(Eye.x, Eye.y, Eye.z),
                               glm::vec3(Target.x, Target.y, Target.z),
                               glm::vec3(Up.x, Up.y, Up.z)));
}
