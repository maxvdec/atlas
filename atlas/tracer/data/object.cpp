//
// object.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Object sending functions
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/tracer/data.h"
#include "atlas/tracer/log.h"
#include <json.hpp>
#include <string>

using json = nlohmann::json;

void DebugObjectPacket::send() {
    if (!TracerServices::getInstance().isOk()) {
        return;
    }

    json object;
    object["type"] = "debug_object";
    object["id"] = std::to_string(objectId);
    object["draw_calls"] = drawCallsForObject;
    object["object_type"] = static_cast<int>(objectType);
    object["triangle_count"] = triangleCount;
    object["material_count"] = materialCount;
    object["vertex_buffer_mb"] = vertexBufferSizeMb;
    object["index_buffer_mb"] = indexBufferSizeMb;
    object["texture_count"] = textureCount;
    object["frame_count"] = frameCount;

    TracerServices::getInstance().tracerPipe->send(object.dump() + "\n");
}