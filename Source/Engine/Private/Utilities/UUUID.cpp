//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Utilities/UUUID.h"
#include <array>
#include <random>
#include <sstream>
#include <iomanip>
#include <cctype>

namespace UUUID
{
    std::string GenerateUUID()
    {
        // 16 random bytes = 128 bits
        std::array<unsigned char, 16> bytes{};

        // Use std::random_device for best quality randomness.
        // It's usually backed by /dev/urandom or similar on real systems.
        std::random_device rd;
        for (auto& b : bytes)
        {
            b = static_cast<unsigned char>(rd());
        }

        // Set version to 4 (random UUID)
        // byte index 6: high nibble = 0100 (0x4)
        bytes[6] = static_cast<unsigned char>((bytes[6] & 0x0F) | 0x40);

        // Set variant to 10xx (RFC 4122)
        // byte index 8: high two bits = 10
        bytes[8] = static_cast<unsigned char>((bytes[8] & 0x3F) | 0x80);

        // Format as 8-4-4-4-12 hex digits
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');

        for (int i = 0; i < 16; ++i)
        {
            oss << std::setw(2) << static_cast<int>(bytes[i]);
            if (i == 3 || i == 5 || i == 7 || i == 9)
                oss << '-';
        }

        return oss.str();
    }

    bool IsValid(std::string uuid)
    {
        if (uuid.size() != 36)
            return false;

        for (std::size_t i = 0; i < uuid.size(); ++i)
        {
            if (i == 8 || i == 13 || i == 18 || i == 23)
            {
                if (uuid[i] != '-')
                    return false;
            }
            else
            {
                if (!std::isxdigit(static_cast<unsigned char>(uuid[i])))
                    return false;
            }
        }

        return true;
    }
}
