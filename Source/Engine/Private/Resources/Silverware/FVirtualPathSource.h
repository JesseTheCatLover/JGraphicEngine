// Copyright 2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "Core/Memory/SmartPointers.h"
#include "Silverware/Core/Resources/IResourceSource.h"

class VirtualPathMounter;

namespace Silverware::Adapters::RedleafEngine
{
    class FVirtualPathSource final : public IResourceSource
    {
    private:
        const ::VirtualPathMounter& m_Mounter;
        std::string m_VirtualRoot;

    public:
        FVirtualPathSource(const ::VirtualPathMounter& mounter, std::string virtualRoot);

        [[nodiscard]] bool IsValid() const noexcept;

        [[nodiscard]] EResourceOpenResult Open(
            std::string_view relativePath,
            TUniquePtr<IResourceStream>& outStream) override;

        [[nodiscard]] EResourceListResult EnumerateFiles(
            std::string_view relativeDirectory,
            bool recursive,
            std::vector<std::string>& outRelativePaths) override;
    };
}