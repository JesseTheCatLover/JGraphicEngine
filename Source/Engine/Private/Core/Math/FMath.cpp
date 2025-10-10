//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/Math/FMath.h"

#include "Core/Math/FMatrix.h"
#include "Core/Math/FRotator.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

// Matrix functions
FMatrix FMath::Identity() { return FMatrix::Identity(); }

FMatrix FMath::Inverse(const FMatrix& M) { return M.Inverse(); }

FMatrix FMath::Transpose(const FMatrix& M) { return M.Transpose(); }

FMatrix FMath::Translate(const FVector3& T) { return FMatrix::Translate(T); }

FMatrix FMath::Scale(const FVector3& S) { return FMatrix::Scale(S); }

FMatrix FMath::Rotate(const FQuat& Q) { return FMatrix::Rotate(Q); }

FMatrix FMath::Ortho(float Left, float Right, float Bottom, float Top, float Near, float Far)
{
    return FMatrix(glm::ortho(Left, Right, Bottom, Top, Near, Far));
}

FMatrix FMath::Perspective(float FOV, float Aspect, float Near, float Far)
{
    return FMatrix(glm::perspective(glm::radians(FOV), Aspect, Near, Far));
}

FMatrix FMath::LookAt(const FVector3& Eye, const FVector3& Target, const FVector3& Up)
{
    return FMatrix(glm::lookAt(glm::vec3(Eye.x, Eye.y, Eye.z),
                               glm::vec3(Target.x, Target.y, Target.z),
                               glm::vec3(Up.x, Up.y, Up.z)));
}
