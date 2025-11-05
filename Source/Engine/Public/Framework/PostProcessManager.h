//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <variant>

struct FPassParam {
    std::unordered_map<std::string, float> floats;
    std::unordered_map<std::string, int> ints;
};

struct FPostPassDesc {
    std::string name;
    bool enabled = true;
    FPassParam params;
};

class PostProcessManager
{
public:
    const std::vector<FPostPassDesc>& GetChain() const { return Chain; }
    std::vector<FPostPassDesc>& EditChain() { Dirty = true; return Chain; }
    bool IsDirtyAndClear() { bool d = Dirty; Dirty = false; return d; }

    void AddPass(FPostPassDesc pass) { Chain.push_back(std::move(pass)); Dirty = true; }
    void RemovePass(size_t i) { if (i<Chain.size()) { Chain.erase(Chain.begin()+i); Dirty = true; } }
    void MovePass(size_t from, size_t to) { if (from<Chain.size() && to<Chain.size()) { auto p=Chain[from]; Chain.erase(Chain.begin()+from); Chain.insert(Chain.begin()+to,p); Dirty=true; } }

private:
    std::vector<FPostPassDesc> Chain;
    bool Dirty = true; // force initial build
};