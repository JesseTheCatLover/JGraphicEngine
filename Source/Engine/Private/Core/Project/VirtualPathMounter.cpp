// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Core/Project/VirtualPathMounter.h"

#include <algorithm>

#include "Utilities/UFileSystem.h"
#include "Utilities/UPath.h"

namespace
{
    static std::string NormalizeSlashes(std::string s)
    {
        for (char& c : s)
        {
            if (c == '\\')
                c = '/';
        }
        return s;
    }

    static std::string TrimTrailingSlashes(std::string s)
    {
        while (s.size() > 1 && !s.empty() && s.back() == '/')
            s.pop_back();
        return s;
    }
}

bool VirtualPathMounter::Mount(const std::string& virtualRoot, const std::string& physicalRoot)
{
    const std::string normalizedVirtualRoot = NormalizeVirtualPath(virtualRoot);
    const std::string normalizedPhysicalRoot = UPath::Normalize(physicalRoot);

    if (!IsValidVirtualRoot(normalizedVirtualRoot))
        return false;

    if (!UFileSystem::DirectoryExists(normalizedPhysicalRoot))
        return false;

    for (FVirtualMountPoint& existing : m_Mounts)
    {
        if (existing.virtualRoot == normalizedVirtualRoot)
        {
            existing.physicalRoot = normalizedPhysicalRoot;
            return true;
        }
    }

    m_Mounts.push_back(FVirtualMountPoint{
        normalizedVirtualRoot,
        normalizedPhysicalRoot
    });

    return true;
}

bool VirtualPathMounter::Unmount(const std::string& virtualRoot)
{
    const std::string normalizedVirtualRoot = NormalizeVirtualPath(virtualRoot);

    auto it = std::remove_if(
        m_Mounts.begin(),
        m_Mounts.end(),
        [&](const FVirtualMountPoint& mount)
        {
            return mount.virtualRoot == normalizedVirtualRoot;
        });

    const bool bRemoved = (it != m_Mounts.end());
    m_Mounts.erase(it, m_Mounts.end());
    return bRemoved;
}

void VirtualPathMounter::Clear()
{
    m_Mounts.clear();
}

bool VirtualPathMounter::IsMounted(const std::string& virtualRoot) const
{
    return FindMount(virtualRoot) != nullptr;
}

const FVirtualMountPoint* VirtualPathMounter::FindMount(const std::string& virtualRoot) const
{
    const std::string normalizedVirtualRoot = NormalizeVirtualPath(virtualRoot);

    for (const FVirtualMountPoint& mount : m_Mounts)
    {
        if (mount.virtualRoot == normalizedVirtualRoot)
            return &mount;
    }

    return nullptr;
}

bool VirtualPathMounter::ResolveVirtualToPhysical(const std::string& virtualPath, std::string& outPhysicalPath) const
{
    outPhysicalPath.clear();

    const std::string normalizedVirtualPath = NormalizeVirtualPath(virtualPath);
    if (normalizedVirtualPath.empty() || normalizedVirtualPath[0] != '/')
        return false;

    for (const FVirtualMountPoint& mount : m_Mounts)
    {
        if (!StartsWithPathSegment(normalizedVirtualPath, mount.virtualRoot))
            continue;

        std::string relative = normalizedVirtualPath.substr(mount.virtualRoot.size());
        if (!relative.empty() && relative[0] == '/')
            relative.erase(relative.begin());

        outPhysicalPath = relative.empty()
            ? mount.physicalRoot
            : UPath::Normalize(UPath::Join(mount.physicalRoot, relative));

        return true;
    }

    return false;
}

bool VirtualPathMounter::ResolvePhysicalToVirtual(const std::string& physicalPath, std::string& outVirtualPath) const
{
    outVirtualPath.clear();

    const std::string normalizedPhysicalPath = UPath::Normalize(physicalPath);

    // Prefer longest matching root first in case nested mounts ever exist later.
    const FVirtualMountPoint* bestMount = nullptr;
    size_t bestLength = 0;

    for (const FVirtualMountPoint& mount : m_Mounts)
    {
        if (!IsPathUnderPhysicalRoot(normalizedPhysicalPath, mount.physicalRoot))
            continue;

        if (mount.physicalRoot.size() > bestLength)
        {
            bestMount = &mount;
            bestLength = mount.physicalRoot.size();
        }
    }

    if (!bestMount)
        return false;

    std::string suffix = normalizedPhysicalPath.substr(bestMount->physicalRoot.size());
    suffix = NormalizeSlashes(suffix);

    if (!suffix.empty() && suffix[0] != '/')
        suffix.insert(suffix.begin(), '/');

    outVirtualPath = bestMount->virtualRoot + suffix;
    outVirtualPath = NormalizeVirtualPath(outVirtualPath);
    return true;
}

std::string VirtualPathMounter::NormalizeVirtualPath(const std::string& path)
{
    if (path.empty())
        return {};

    std::string result = NormalizeSlashes(path);

    if (result.front() != '/')
        result.insert(result.begin(), '/');

    while (result.find("//") != std::string::npos)
        result.replace(result.find("//"), 2, "/");

    result = TrimTrailingSlashes(result);
    return result;
}

bool VirtualPathMounter::IsValidVirtualRoot(const std::string& virtualRoot)
{
    if (virtualRoot.size() < 2)
        return false;

    if (virtualRoot[0] != '/')
        return false;

    if (virtualRoot.find("//") != std::string::npos)
        return false;

    return true;
}

bool VirtualPathMounter::IsPathUnderPhysicalRoot(const std::string& physicalPath, const std::string& rootPath)
{
    const std::string normalizedPath = NormalizeSlashes(UPath::Normalize(physicalPath));
    const std::string normalizedRoot = NormalizeSlashes(UPath::Normalize(rootPath));

    if (normalizedPath == normalizedRoot)
        return true;

    if (normalizedPath.size() <= normalizedRoot.size())
        return false;

    if (normalizedPath.compare(0, normalizedRoot.size(), normalizedRoot) != 0)
        return false;

    return normalizedPath[normalizedRoot.size()] == '/';
}

bool VirtualPathMounter::StartsWithPathSegment(const std::string& text, const std::string& prefix)
{
    if (text.size() < prefix.size())
        return false;

    if (text.compare(0, prefix.size(), prefix) != 0)
        return false;

    if (text.size() == prefix.size())
        return true;

    return text[prefix.size()] == '/';
}