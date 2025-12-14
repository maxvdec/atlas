//
// resources.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Resource Debugging sending and functions
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/tracer/data.h"
#include "atlas/tracer/log.h"
#include <json.hpp>

using json = nlohmann::json;

void ResourceEventInfo::send() {
    if (!TracerServices::getInstance().isOk()) {
        return;
    }

    json object;
    object["caller_object"] = callerObject;
    object["resource_type"] = static_cast<int>(resourceType);
    object["operation"] = static_cast<int>(operation);
    object["frame_number"] = frameNumber;
    object["size_mb"] = sizeMb;
    object["type"] = "resource_event";

    TracerServices::getInstance().tracerPipe->send(object.dump() + "\n");
}

void FrameResourcesInfo::send() {
    if (!TracerServices::getInstance().isOk()) {
        return;
    }

    json object;
    object["frame_number"] = frameNumber;
    object["resources_created"] = resourcesCreated;
    object["resources_loaded"] = resourcesLoaded;
    object["resources_unloaded"] = resourcesUnloaded;
    object["total_memory_mb"] = totalMemoryMb;
    object["type"] = "frame_resources_info";
    TracerServices::getInstance().tracerPipe->send(object.dump() + "\n");
}

void ObjectResourcesInfo::send() {
    if (!TracerServices::getInstance().isOk()) {
        return;
    }

    json object;
    object["caller_object"] = callerObject;
    object["resource_type"] = resouceType;
    object["size_mb"] = sizeMb;

    std::vector<json> breakdownArray;
    for (const auto &entry : breakdown) {
        json entryJson;
        entryJson["individual_type"] = entry.resourceType;
        entryJson["count"] = entry.count;
        breakdownArray.push_back(entryJson);
    }

    object["breakdown"] = breakdownArray;

    object["type"] = "object_resources_info";

    TracerServices::getInstance().tracerPipe->send(object.dump() + "\n");
}