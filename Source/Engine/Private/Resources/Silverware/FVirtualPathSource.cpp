// Copyright 2026 JesseTheCatLover. All Rights Reserved.

#include "FVirtualPathSource.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string_view>
#include <utility>

#include "Core/Project/VirtualPathMounter.h"
#include "Silverware/Core/Resources/ResourcePath.h"
#include "Utilities/UPath.h"

namespace Silverware::Adapters::RedleafEngine
{
    namespace
    {
        class FFileResourceStream final : public IResourceStream
        {
        private:
            std::ifstream m_File;
            std::uint64_t m_Size = 0;
            std::uint64_t m_Position = 0;

        public:
            FFileResourceStream(
                std::ifstream file,
                std::uint64_t size)
                : m_File(std::move(file))
                , m_Size(size)
            {
            }

            [[nodiscard]] bool Read(std::span<std::byte> destination, std::size_t& outBytesRead) override
            {
                outBytesRead = 0;

                if (!m_File.is_open())
                    return false;

                if (destination.empty() || m_Position == m_Size)
                    return true;

                const std::uint64_t remaining = m_Size - m_Position;
                const auto maxRead = static_cast<std::uint64_t>(
                    std::numeric_limits<std::streamsize>::max());

                const std::uint64_t requested64 = std::min({
                    remaining,
                    static_cast<std::uint64_t>(destination.size()),
                    maxRead
                });

                const auto requested =
                    static_cast<std::size_t>(requested64);

                m_File.read(reinterpret_cast<char*>(destination.data()),
                    static_cast<std::streamsize>(requested));

                const std::streamsize bytesRead = m_File.gcount();

                if (bytesRead < 0)
                    return false;

                outBytesRead = static_cast<std::size_t>(bytesRead);
                m_Position += outBytesRead;

                return !m_File.bad() && outBytesRead == requested;
            }

            [[nodiscard]] bool Seek(
                std::uint64_t absoluteOffset) override
            {
                if (!m_File.is_open() ||
                    absoluteOffset > m_Size ||
                    absoluteOffset > static_cast<std::uint64_t>(
                        std::numeric_limits<std::streamoff>::max()))
                {
                    return false;
                }

                m_File.clear();
                m_File.seekg(
                    static_cast<std::streamoff>(absoluteOffset),
                    std::ios::beg);

                if (!m_File)
                    return false;

                m_Position = absoluteOffset;
                return true;
            }

            [[nodiscard]] std::uint64_t Tell() const noexcept override
            {
                return m_Position;
            }

            [[nodiscard]] std::uint64_t GetSize() const noexcept override
            {
                return m_Size;
            }
        };
    }

    FVirtualPathSource::FVirtualPathSource(const ::VirtualPathMounter& mounter, std::string virtualRoot)
        : m_Mounter(mounter)
        , m_VirtualRoot(UPath::NormalizeVirtual(virtualRoot))
    {
    }

    bool FVirtualPathSource::IsValid() const noexcept
    {
        return m_VirtualRoot.size() > 1 &&
               m_VirtualRoot.front() == '/' &&
               m_VirtualRoot.find("//") == std::string::npos;
    }

    EResourceOpenResult FVirtualPathSource::Open(
        std::string_view relativePath,
        TUniquePtr<IResourceStream>& outStream)
    {
        outStream.reset();

        if (!IsValid())
            return EResourceOpenResult::Error;

        // Sources receive normalized, namespace-relative paths.
        FResourcePath validatedPath;

        if (!FResourcePath::TryCreate(
                "validation",
                relativePath,
                validatedPath) ||
            std::string_view(validatedPath.GetRelativePath()) != relativePath)
        {
            return EResourceOpenResult::Error;
        }

        std::string virtualPath = m_VirtualRoot;
        virtualPath += '/';
        virtualPath.append(relativePath.data(), relativePath.size());
        virtualPath = UPath::NormalizeVirtual(virtualPath);

        std::string physicalPath;

        if (!m_Mounter.ResolveVirtualToPhysical(
                virtualPath,
                physicalPath))
        {
            // The configured virtual root isn't resolvable.
            return EResourceOpenResult::Error;
        }

        std::error_code error;

        const bool exists = std::filesystem::exists(physicalPath, error);

        if (error)
            return EResourceOpenResult::Error;

        if (!exists)
            return EResourceOpenResult::NotFound;

        const bool isRegularFile = std::filesystem::is_regular_file(physicalPath, error);

        if (error)
            return EResourceOpenResult::Error;

        if (!isRegularFile)
            return EResourceOpenResult::NotFound;

        std::ifstream file(
            physicalPath,
            std::ios::binary | std::ios::ate);

        if (!file.is_open())
            return EResourceOpenResult::Error;

        const std::streampos end = file.tellg();

        if (end == std::streampos(-1))
            return EResourceOpenResult::Error;

        const std::streamoff size = static_cast<std::streamoff>(end);

        if (size < 0)
            return EResourceOpenResult::Error;

        file.seekg(0, std::ios::beg);

        if (!file)
            return EResourceOpenResult::Error;

        outStream = TUniquePtr<IResourceStream>(
            new FFileResourceStream(
                std::move(file),
                static_cast<std::uint64_t>(size)));

        return EResourceOpenResult::Opened;
    }
}