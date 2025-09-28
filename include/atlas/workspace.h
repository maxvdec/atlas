/*
 workspace.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Workspace utilties and functions
 Copyright (c) 2025 maxvdec
*/

#ifndef WORKSPACE_H
#define WORKSPACE_H

#include <optional>
#include <string>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

enum class ResourceType { File, Image, SpecularMap, Audio };

struct Resource {
    fs::path path;
    std::string name;
    ResourceType type = ResourceType::File;
};

struct ResourceGroup {
    std::string groupName;
    std::vector<Resource> resources;

    Resource findResource(std::string name);
};

class Workspace {
  public:
    static Workspace &get() {
        static Workspace instance;
        return instance;
    }

    Workspace(const Workspace &) = delete;
    Workspace &operator=(const Workspace &) = delete;

    Resource createResource(const fs::path &path, std::string name,
                            ResourceType type = ResourceType::File);
    ResourceGroup createResourceGroup(std::string groupName,
                                      const std::vector<Resource> &resources);
    Resource getResource(std::string name);
    std::vector<Resource> getAllResources();
    std::vector<Resource> getResourcesByType(ResourceType type);

    ResourceGroup getResourceGroup(std::string groupName);
    std::vector<ResourceGroup> getAllResourceGroups();

    inline void setRootPath(const fs::path &path) { rootPath = path; }

  private:
    std::vector<Resource> resources;
    std::vector<ResourceGroup> resourceGroups;
    std::optional<fs::path> rootPath;

    Workspace() : resources({}), resourceGroups({}), rootPath(std::nullopt) {}
    ~Workspace() = default;
};

#endif // WORKSPACE_H
