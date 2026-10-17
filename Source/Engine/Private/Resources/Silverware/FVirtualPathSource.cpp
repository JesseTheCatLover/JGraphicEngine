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

        bool IsUnderRoot(
    const std::filesystem::path& root,
    const std::filesystem::path& candidate)
        {
            auto rootIt = root.begin();
            auto candidateIt = candidate.begin();

            for (; rootIt != root.end(); ++rootIt, ++candidateIt)
            {
                if (candidateIt == candidate.end() || *rootIt != *candidateIt)
                    return false;
            }

            return true;
        }

        bool IsSafeRelativePath(const std::filesystem::path& path, bool allowEmpty = false)
        {
            if (path.is_absolute() || path.has_root_name() || path.has_root_directory())
                return false;

            if (path.empty())
                return allowEmpty;

            for (const auto& component : path)
            {
                if (component == ".." || component == ".")
                    return false;
            }

            return true;
        }

        bool ResolveCanonicalRoot(
            const VirtualPathMounter& mounter,
            const std::string& virtualRoot,
            std::filesystem::path& outRoot)
        {
            std::string physicalRoot;
            if (!mounter.ResolveVirtualToPhysical(virtualRoot, physicalRoot))
                return false;

            std::error_code error;
            const auto canonicalRoot = std::filesystem::canonical(physicalRoot, error);
            if (error)
                return false;

            if (!std::filesystem::is_directory(canonicalRoot, error) || error)
                return false;

            outRoot = canonicalRoot;
            return true;
        }
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

    EResourceListResult FVirtualPathSource::EnumerateFiles(
    std::string_view relativeDirectory,
    bool recursive,
    std::vector<std::string>& outRelativePaths)
{
    outRelativePaths.clear();

    if (!IsValid())
        return EResourceListResult::Error;

    const std::filesystem::path relativePath{std::string(relativeDirectory)};
    if (!IsSafeRelativePath(relativePath, true))
        return EResourceListResult::Error;

    if (!relativeDirectory.empty())
    {
        FResourcePath validatedPath;

        if (!FResourcePath::TryCreate("validation", relativeDirectory, validatedPath) ||
            std::string_view(validatedPath.GetRelativePath()) != relativeDirectory)
        {
            return EResourceListResult::Error;
        }
    }

    std::filesystem::path canonicalRoot;
    if (!ResolveCanonicalRoot(m_Mounter, m_VirtualRoot, canonicalRoot))
        return EResourceListResult::Error;

    std::error_code error;
    std::filesystem::path target = canonicalRoot;

    if (!relativePath.empty())
    {
        target = std::filesystem::canonical(canonicalRoot / relativePath, error);

        if (error)
        {
            if (error == std::errc::no_such_file_or_directory ||
                error == std::errc::not_a_directory)
            {
                return EResourceListResult::NotFound;
            }

            return EResourceListResult::Error;
        }
    }

    if (!IsUnderRoot(canonicalRoot, target))
        return EResourceListResult::Error;

    if (!std::filesystem::is_directory(target, error))
        return error ? EResourceListResult::Error : EResourceListResult::NotFound;

    if (error)
        return EResourceListResult::Error;

    auto addFile = [&](const std::filesystem::directory_entry& entry)
    {
        const bool isSymlink = entry.is_symlink(error);
        if (error)
            return false;

        // Don't discover font files through symbolic links.
        if (isSymlink)
            return true;

        if (!entry.is_regular_file(error))
            return !error;

        const auto canonicalFile = std::filesystem::canonical(entry.path(), error);
        if (error || !IsUnderRoot(canonicalRoot, canonicalFile))
            return false;

        const auto relativeFile = canonicalFile.lexically_relative(canonicalRoot);
        if (!IsSafeRelativePath(relativeFile))
            return false;

        outRelativePaths.push_back(relativeFile.generic_string());
        return true;
    };

    if (recursive)
    {
        std::filesystem::recursive_directory_iterator iterator(target, error);
        const std::filesystem::recursive_directory_iterator end;

        if (error)
            return EResourceListResult::Error;

        while (iterator != end)
        {
            if (!addFile(*iterator))
            {
                outRelativePaths.clear();
                return EResourceListResult::Error;
            }

            iterator.increment(error);
            if (error)
            {
                outRelativePaths.clear();
                return EResourceListResult::Error;
            }
        }
    }
    else
    {
        std::filesystem::directory_iterator iterator(target, error);
        const std::filesystem::directory_iterator end;

        if (error)
            return EResourceListResult::Error;

        while (iterator != end)
        {
            if (!addFile(*iterator))
            {
                outRelativePaths.clear();
                return EResourceListResult::Error;
            }

            iterator.increment(error);
            if (error)
            {
                outRelativePaths.clear();
                return EResourceListResult::Error;
            }
        }
    }

    std::sort(outRelativePaths.begin(), outRelativePaths.end());
    return EResourceListResult::Enumerated;
}
}
