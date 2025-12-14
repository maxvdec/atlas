//
// graphics.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Graphics Data Interpretation
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/tracer/data.h"
#include "atlas/tracer/log.h"
#include <json.hpp>

using json = nlohmann::json;

void DrawCallInfo::send() {
    if (!TracerServices::getInstance().isOk()) {
        return;
    }

    json data;
    data["type"] = "draw_call";
    data["caller_object"] = callerObject;
    data["draw_call_type"] = static_cast<int>(type);
    data["frame_number"] = frameNumber;

    TracerServices::getInstance().tracerPipe->send(data.dump() + "\n");
}

void FrameDrawInfo::send() {
    if (!TracerServices::getInstance().isOk()) {
        return;
    }

    json data;
    data["type"] = "frame_draw_info";
    data["frame_number"] = frameNumber;
    data["draw_call_count"] = drawCallCount;
    data["frame_time_ms"] = frameTimeMs;
    data["fps"] = fps;

    TracerServices::getInstance().tracerPipe->send(data.dump() + "\n");
}