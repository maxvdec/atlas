//
// memory.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Memory Tracing implementation functions
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/tracer/data.h"
#include "atlas/tracer/log.h"
#include <json.hpp>

using json = nlohmann::json;

void FrameMemoryPacket::send() {
    if (!TracerServices::getInstance().isOk()) {
        return;
    }

    json object;
    object["frame_number"] = frameNumber;
    object["total_allocated_mb"] = totalAllocatedMb;
    object["total_gpu_mb"] = totalGPUMb;
    object["total_cpu_mb"] = totalCPUMb;
    object["allocation_count"] = allocationCount;
    object["deallocation_count"] = deallocationCount;
    object["type"] = "frame_memory_info";
    TracerServices::getInstance().tracerPipe->send(object.dump() + "\n");
}

void AllocationPacket::send() {
    if (!TracerServices::getInstance().isOk()) {
        return;
    }

    json object;
    object["description"] = description;
    object["owner"] = owner;
    object["domain"] = static_cast<int>(domain);
    object["kind"] = static_cast<int>(kind);
    object["size_mb"] = sizeMb;
    object["frame_number"] = frameNumber;
    object["type"] = "allocation_event";
    TracerServices::getInstance().tracerPipe->send(object.dump() + "\n");
}