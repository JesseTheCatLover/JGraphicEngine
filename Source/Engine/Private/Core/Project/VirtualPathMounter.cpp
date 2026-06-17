// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Core/Project/VirtualPathMounter.h"

#include <algorithm>

#include "Utilities/UFileSystem.h"
#include "Utilities/UPath.h"

namespace
{
    static std::string CanonicalSlashes(std::string s)
    {
        for (char& c : s)
            if (c == '\\') c = '/';
        return s;
    }

    static std::string TrimTrailingSlashes(std::string s)
    {
        while (s.size() > 1 && !s.empty() && s.back() == '/')
            s.pop_back();
        return s;
    }

    static std::string JoinPhysical(const std::string& a, const std::string& b)
    {
        std::filesystem::path p(a);
        p /= b;

        return CanonicalSlashes(p.lexically_normal().generic_string());
    }
}

bool VirtualPathMounter::Mount(const std::string& virtualRoot, const std::string& physicalRoot)
{
    const std::string normalizedVirtualRoot = UPath::NormalizeVirtual(virtualRoot);
    const std::string normalizedPhysicalRoot = UPath::NormalizePhysical(physicalRoot);

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
    const std::string normalizedVirtualRoot = UPath::NormalizeVirtual(virtualRoot);

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
    const std::string normalizedVirtualRoot = UPath::NormalizeVirtual(virtualRoot);

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

    const std::string normalizedVirtualPath = UPath::NormalizeVirtual(virtualPath);
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
            : UPath::NormalizePhysical(JoinPhysical(mount.physicalRoot, relative));

        return true;
    }

    return false;
}

bool VirtualPathMounter::ResolvePhysicalToVirtual(const std::string& physicalPath, std::string& outVirtualPath) const
{
    outVirtualPath.clear();

    const std::string normalizedPhysicalPath = CanonicalSlashes(UPath::NormalizePhysical(physicalPath));

    const FVirtualMountPoint* bestMount = nullptr;
    size_t bestLength = 0;

    for (const FVirtualMountPoint& mount : m_Mounts)
    {
        const std::string mountRoot = CanonicalSlashes(mount.physicalRoot);

        if (!IsPathUnderPhysicalRoot(normalizedPhysicalPath, mountRoot))
            continue;

        if (mountRoot.size() > bestLength)
        {
            bestMount = &mount;
            bestLength = mountRoot.size();
        }
    }

    if (!bestMount)
        return false;

    const std::string bestRoot = CanonicalSlashes(bestMount->physicalRoot);

    std::string suffix = normalizedPhysicalPath.substr(bestRoot.size());

    suffix = CanonicalSlashes(suffix);

    if (!suffix.empty() && suffix[0] != '/')
        suffix.insert(suffix.begin(), '/');

    outVirtualPath = bestMount->virtualRoot + suffix;
    outVirtualPath = UPath::NormalizeVirtual(outVirtualPath);

    return true;
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
    const std::string normalizedPath = CanonicalSlashes(UPath::NormalizePhysical(physicalPath));
    const std::string normalizedRoot = CanonicalSlashes(UPath::NormalizePhysical(rootPath));

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