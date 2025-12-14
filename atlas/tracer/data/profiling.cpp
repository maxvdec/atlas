//
// profiling.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Profiling data for the Atlas runtime
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/tracer/data.h"
#include "atlas/tracer/log.h"
#include <json.hpp>

using json = nlohmann::json;

void FrameTimingPacket::send() {
    if (!TracerServices::getInstance().isOk()) {
        return;
    }

    json object;
    object["type"] = "frame_timing_info";
    object["frame_number"] = frameNumber;
    object["cpu_frame_time_ms"] = cpuFrameTimeMs;
    object["gpu_frame_time_ms"] = gpuFrameTimeMs;
    object["main_thread_time_ms"] = mainThreadTimeMs;
    object["worker_thread_time_ms"] = workerThreadTimeMs;
    object["memory_mb"] = memoryMb;
    object["cpu_usage_percent"] = cpuUsagePercent;
    object["gpu_usage_percent"] = gpuUsagePercent;
    TracerServices::getInstance().tracerPipe->send(object.dump() + "\n");
}

void TimingEventPacket::send() {
    if (!TracerServices::getInstance().isOk()) {
        return;
    }

    json object;
    object["type"] = "timing_event";
    object["name"] = name;
    object["subsystem"] = static_cast<int>(subsystem);
    object["duration_ms"] = durationMs;
    object["frame_number"] = frameNumber;
    TracerServices::getInstance().tracerPipe->send(object.dump() + "\n");
}