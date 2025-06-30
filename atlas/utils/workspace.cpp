/*
 workspace.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Workspace functions implementation
 Copyright (c) 2025 maxvdec
*/

#include "atlas/workspace.hpp"
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

Workspace::Workspace(const std::string &path) : path(path) {
    if (!fs::exists(path)) {
        throw std::runtime_error("Workspace path does not exist: " + path);
    }
}

Resource Workspace::loadResource(const std::string &resourceName) const {
    fs::path resourcePath = getResourcePath(resourceName);
    if (!fs::exists(resourcePath)) {
        throw std::runtime_error("Resource does not exist: " +
                                 resourcePath.string());
    }
    std::ifstream file(resourcePath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open resource file: " +
                                 resourcePath.string());
    }
    std::vector<char> data((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
    return Resource{resourcePath.string(), std::move(data)};
}

fs::path Workspace::getResourcePath(const std::string &resourceName) const {
    return fs::path(path) / resourceName;
}

Resource::Resource(std::string path) : path(std::move(path)) {
    if (!fs::exists(this->path)) {
        throw std::runtime_error("Resource path does not exist: " + this->path);
    }

    std::ifstream file(this->path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open resource file: " + this->path);
    }
    data = std::vector<char>((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
}
