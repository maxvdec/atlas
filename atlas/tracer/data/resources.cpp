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
    object["callerObject"] = callerObject;
    object["resourceType"] = static_cast<int>(resourceType);
    object["operation"] = static_cast<int>(operation);
    object["frameNumber"] = frameNumber;
    object["sizeMb"] = sizeMb;

    TracerServices::getInstance().tracerPipe->send(object.dump() + "\n");
}

void FrameResourcesInfo::send() {
    if (!TracerServices::getInstance().isOk()) {
        return;
    }

    json object;
    object["frameNumber"] = frameNumber;
    object["resourcesCreated"] = resourcesCreated;
    object["resourcesLoaded"] = resourcesLoaded;
    object["resourcesUnloaded"] = resourcesUnloaded;
    object["totalMemoryMb"] = totalMemoryMb;

    TracerServices::getInstance().tracerPipe->send(object.dump() + "\n");
}

void ObjectResourcesInfo::send() {
    if (!TracerServices::getInstance().isOk()) {
        return;
    }

    json object;
    object["callerObject"] = callerObject;
    object["resouceType"] = resouceType;
    object["sizeMb"] = sizeMb;

    std::vector<json> breakdownArray;
    for (const auto &entry : breakdown) {
        json entryJson;
        entryJson["individualType"] = entry.resourceType;
        entryJson["count"] = entry.count;
        breakdownArray.push_back(entryJson);
    }
    object["breakdown"] = breakdownArray;

    TracerServices::getInstance().tracerPipe->send(object.dump() + "\n");
}