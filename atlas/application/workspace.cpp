/*
 workspace.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Resource implementation and functions
 Copyright (c) 2025 maxvdec
*/

#include "atlas/workspace.h"
#include "atlas/tracer/log.h"

Resource Workspace::createResource(const fs::path &path, std::string name,
                                   ResourceType type) {
    for (const auto &res : resources) {
        if (res.name == name) {
            atlas_log("Resource already exists: " + name);
            return res;
        }
    }
    atlas_log("Creating resource: " + name);
    Resource res;
    if (rootPath && path.is_relative()) {
        res.path = rootPath.value() / path;
    } else {
        res.path = path;
    }
    res.name = name;
    res.type = type;
    resources.push_back(res);
    return res;
}

ResourceGroup
Workspace::createResourceGroup(std::string groupName,
                               const std::vector<Resource> &resources) {
    atlas_log("Creating resource group: " + groupName);
    ResourceGroup group;
    group.groupName = groupName;
    group.resources = resources;
    resourceGroups.push_back(group);
    return group;
}

Resource Workspace::getResource(std::string name) {
    for (const auto &res : resources) {
        if (res.name == name) {
            return res;
        }
    }
    atlas_warning("Resource not found: " + name);
    throw std::runtime_error("Resource not found: " + name);
}

std::vector<Resource> Workspace::getAllResources() { return resources; }

std::vector<Resource> Workspace::getResourcesByType(ResourceType type) {
    std::vector<Resource> filtered;
    for (const auto &res : resources) {
        if (res.type == type) {
            filtered.push_back(res);
        }
    }
    return filtered;
}

std::vector<ResourceGroup> Workspace::getAllResourceGroups() {
    return resourceGroups;
}

ResourceGroup Workspace::getResourceGroup(std::string groupName) {
    for (const auto &group : resourceGroups) {
        if (group.groupName == groupName) {
            return group;
        }
    }
    atlas_warning("Resource group not found: " + groupName);
    throw std::runtime_error("Resource group not found: " + groupName);
}

Resource ResourceGroup::findResource(std::string name) {
    for (const auto &res : resources) {
        if (res.name == name) {
            return res;
        }
    }
    throw std::runtime_error("Resource not found in group: " + name);
}
