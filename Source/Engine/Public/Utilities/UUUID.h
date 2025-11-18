//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>

namespace UUUID
{
    /**
     * @brief Generate a RFC-4122 version 4 UUID as a string
     * @note e.g. "550e8400-e29b-41d4-a716-446655440000"
     */
    std::string GenerateUUID();

    /**
     * @brief Checks if an UUID string is valid
     * @param uuid UUID string
     * @return True if valid
     */
    [[nodiscard]] bool IsValid(std::string uuid);
}
