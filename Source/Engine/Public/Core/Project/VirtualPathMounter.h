// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>

struct FVirtualMountPoint
{
    std::string virtualRoot;   // "/Engine", "/Project"
    std::string physicalRoot;  // absolute normalized path
};

class VirtualPathMounter
{
public:
    VirtualPathMounter() = default;
    ~VirtualPathMounter() = default;

    VirtualPathMounter(const VirtualPathMounter&) = delete;
    VirtualPathMounter& operator=(const VirtualPathMounter&) = delete;
    VirtualPathMounter(VirtualPathMounter&&) = delete;
    VirtualPathMounter& operator=(VirtualPathMounter&&) = delete;

public:
    bool Mount(const std::string& virtualRoot, const std::string& physicalRoot);
    bool Unmount(const std::string& virtualRoot);
    void Clear();

    [[nodiscard]] bool IsMounted(const std::string& virtualRoot) const;
    [[nodiscard]] const FVirtualMountPoint* FindMount(const std::string& virtualRoot) const;

    bool ResolveVirtualToPhysical(const std::string& virtualPath, std::string& outPhysicalPath) const;
    bool ResolvePhysicalToVirtual(const std::string& physicalPath, std::string& outVirtualPath) const;

    [[nodiscard]] const std::vector<FVirtualMountPoint>& GetMounts() const { return m_Mounts; }

private:
    [[nodiscard]] static bool IsValidVirtualRoot(const std::string& virtualRoot);
    [[nodiscard]] static bool IsPathUnderPhysicalRoot(const std::string& physicalPath, const std::string& rootPath);
    [[nodiscard]] static bool StartsWithPathSegment(const std::string& text, const std::string& prefix);

private:
    std::vector<FVirtualMountPoint> m_Mounts;
};