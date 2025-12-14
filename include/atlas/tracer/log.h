//
// log.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Logging functions through the Tracer Protocol
// Copyright (c) 2025 Max Van den Eynde
//

#ifndef TRACER_LOG_H
#define TRACER_LOG_H

#include <atlas/network/pipe.h>
#include <chrono>
#include <ctime>
#include <memory>
#include <string>

class TracerServices {
  private:
    TracerServices();

  public:
    static TracerServices &getInstance() {
        static TracerServices instance;
        return instance;
    }

    std::shared_ptr<NetworkPipe> tracerPipe;

    bool isOk() const { return tracerPipe != nullptr; }

    void startTracing(int port);
};

class Logger {
  private:
    Logger();

  public:
    static Logger &getInstance() {
        static Logger instance;
        return instance;
    }

    void log(const std::string &message, const std::string &file, int line);
    void warning(const std::string &message, const std::string &file, int line);
    void error(const std::string &message, const std::string &file, int line);
};

#define atlas_log(msg) Logger::getInstance().log(msg, __FILE__, __LINE__);
#define atlas_warning(msg)                                                     \
    Logger::getInstance().warning(msg, __FILE__, __LINE__);
#define atlas_error(msg) Logger::getInstance().error(msg, __FILE__, __LINE__);

#define TRACER_PORT 5123

class DebugTimer {
  public:
    DebugTimer(const std::string &name);
    ~DebugTimer();

    uint64_t stop();

  private:
    std::string name;
    std::chrono::high_resolution_clock::time_point startTime;
};

#endif // TRACER_LOG_H