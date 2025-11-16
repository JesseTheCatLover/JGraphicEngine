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
    return FMatrix4(glm::ortho(Left, Right, Bottom, Top, Near, Far));
}

FMatrix4 FMath::Perspective(float FOVInDegree, float Aspect, float Near, float Far)
{
    return FMatrix4(glm::perspective(glm::radians(FOVInDegree), Aspect, Near, Far));
}

FMatrix4 FMath::LookAt(const FVector3& Eye, const FVector3& Target, const FVector3& Up)
{
    return FMatrix4(glm::lookAt(glm::vec3(Eye.x, Eye.y, Eye.z),
                               glm::vec3(Target.x, Target.y, Target.z),
                               glm::vec3(Up.x, Up.y, Up.z)));
}
