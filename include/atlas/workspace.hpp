/*
 workspace.hpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Workspace related functions to speed the loading of resources
 Copyright (c) 2025 maxvdec
*/

#ifndef ATLAS_WORKSPACE_HPP
#define ATLAS_WORKSPACE_HPP

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct Resource {
    std::string path;
    std::vector<char> data;

    inline std::string getExtension() const {
        return fs::path(path).extension().string();
    }

    inline std::string getName() const {
        return fs::path(path).filename().string();
    }
};

class Workspace {
  public:
    std::string path;

    Workspace(const std::string &path);

    Resource loadResource(const std::string &resourceName) const;
    fs::path getResourcePath(const std::string &resourceName) const;
};

#endif // ATLAS_WORKSPACE_HPP
