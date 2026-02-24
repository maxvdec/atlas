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

/**
 * @brief Enumeration of supported resource types in the workspace.
 *
 */
enum class ResourceType { File, Image, SpecularMap, Audio, Font, Model };

/**
 * @brief Structure representing a single resource in the workspace.
 *
 */
struct Resource {
    /**
     * @brief Filesystem path pointing to the asset on disk.
     */
    fs::path path;
    /**
     * @brief Human readable name used to query the resource later.
     */
    std::string name;
    /**
     * @brief Type tag describing how the resource should be interpreted.
     */
    ResourceType type = ResourceType::File;
};

/**
 * @brief Structure representing a collection of related resources.
 *
 */
struct ResourceGroup {
    /**
     * @brief Name that identifies the group for lookup purposes.
     */
    std::string groupName;
    /**
     * @brief Resources that belong to this group.
     */
    std::vector<Resource> resources;

    /**
     * @brief Finds a resource by name within this group.
     *
     * @param name The name of the resource to find.
     * @return (Resource) The found resource.
     */
    Resource findResource(const std::string& name);
};

/**
 * @brief Singleton class that manages all resources and resource groups in the
 * application. Provides centralized access to assets like images, audio files,
 * and other resources.
 *
 * \subsection workspace-example Example
 * ```cpp
 * // Get the workspace instance
 * Workspace &workspace = Workspace::get();
 * // Set the root path for resources
 * workspace.setRootPath("assets/");
 * // Create a resource
 * Resource texture = workspace.createResource("textures/brick.png",
 * "BrickTexture", ResourceType::Image);
 * // Create a resource group
 * std::vector<Resource> skyboxResources = {...};
 * ResourceGroup skybox = workspace.createResourceGroup("Skybox",
 * skyboxResources);
 * ```
 *
 */
class Workspace {
public:
    static Workspace& get() {
        static Workspace instance;
        return instance;
    }

    Workspace(const Workspace&) = delete;
    Workspace& operator=(const Workspace&) = delete;

    /**
     * @brief Creates a new resource from a file path.
     *
     * @param path The filesystem path to the resource.
     * @param name The name to assign to the resource.
     * @param type The type of resource to create.
     * @return (Resource) The created resource.
     */
    Resource createResource(const fs::path& path, const std::string& name,
                            ResourceType type = ResourceType::File);
    /**
     * @brief Creates a new resource group containing multiple resources.
     *
     * @param groupName The name for the resource group.
     * @param initResources The vector of resources to include in the group.
     * @return (ResourceGroup) The created resource group.
     */
    ResourceGroup createResourceGroup(const std::string& groupName,
                                      const std::vector<Resource>& initResources);
    /**
     * @brief Retrieves a resource by its name.
     *
     * @param name The name of the resource to retrieve.
     * @return (Resource) The found resource.
     */
    Resource getResource(const std::string& name);
    /**
     * @brief Gets all resources registered in the workspace.
     *
     * @return (std::vector<Resource>) Vector containing all resources.
     */
    std::vector<Resource> getAllResources();
    /**
     * @brief Gets all resources of a specific type.
     *
     * @param type The type of resources to retrieve.
     * @return (std::vector<Resource>) Vector containing resources of the
     * specified type.
     */
    std::vector<Resource> getResourcesByType(ResourceType type);

    /**
     * @brief Retrieves a resource group by its name.
     *
     * @param groupName The name of the resource group to retrieve.
     * @return (ResourceGroup) The found resource group.
     */
    ResourceGroup getResourceGroup(const std::string& groupName);
    /**
     * @brief Gets all resource groups registered in the workspace.
     *
     * @return (std::vector<ResourceGroup>) Vector containing all resource
     * groups.
     */
    std::vector<ResourceGroup> getAllResourceGroups();

    /**
     * @brief Sets the root path for the workspace.
     *
     * @param path The root filesystem path to set.
     */
    inline void setRootPath(const fs::path& path) { rootPath = path; }

private:
    std::vector<Resource> resources;
    std::vector<ResourceGroup> resourceGroups;
    std::optional<fs::path> rootPath;

    Workspace() : resources({}), resourceGroups({}), rootPath(std::nullopt) {
    }

    ~Workspace() = default;
};

#endif // WORKSPACE_H