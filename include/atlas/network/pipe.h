//
// pipe.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Pipe implementation for networking
// Copyright (c) 2025 Max Van den Eynde
//

#ifndef NETWORK_PIPE_H
#define NETWORK_PIPE_H

// TCP Pipe
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using PipeCallback = std::function<void(const std::string &)>;

class NetworkPipe {
  private:
    int port = 0;
    int serverSocket = -1;
    int clientSocket = -1;

    std::thread listenerThread;
    bool running = false;

    std::vector<std::string> messages;
    std::mutex messagesMutex;

    PipeCallback dispatcher;

    void listenLoop();

  public:
    NetworkPipe();
    ~NetworkPipe();

    void setPort(int port);
    void start();
    void stop();
    void onRecieve(const PipeCallback &callback);
    void send(const std::string &message) const;

    std::vector<std::string> getMessages();
};

#endif // NETWORK_PIPE_H