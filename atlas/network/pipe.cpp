//
// pipe.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Pipe implementation for C++
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/network/pipe.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <thread>
#include <mutex>
#include <vector>
#include <string>

NetworkPipe::NetworkPipe() = default;

NetworkPipe::~NetworkPipe() { stop(); }

void NetworkPipe::setPort(int port) { this->port = port; }

void NetworkPipe::onRecieve(const PipeCallback &callback) {
    this->dispatcher = callback;
}

void NetworkPipe::start() {
    if (port == 0) {
        std::cerr << "Port not set. Cannot start NetworkPipe." << std::endl;
        return;
    }
    running = true;

    listenerThread = std::thread(&NetworkPipe::listenLoop, this);
}

void NetworkPipe::stop() {
    running = false;
    if (clientSocket != -1) {
        close(clientSocket);
        clientSocket = -1;
    }
    if (serverSocket != -1) {
        close(serverSocket);
        serverSocket = -1;
    }
    if (listenerThread.joinable()) {
        listenerThread.join();
    }
}

void NetworkPipe::listenLoop() {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("socket");
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(serverSocket, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) <
        0) {
        perror("bind");
        return;
    }

    if (listen(serverSocket, 1) < 0) {
        perror("listen");
        return;
    }

    std::cout << "Listening on port " << port << "...\n";

    sockaddr_in clientAddr{};
    socklen_t clientLen = sizeof(clientAddr);
    clientSocket = accept(
        serverSocket, reinterpret_cast<sockaddr *>(&clientAddr), &clientLen);
    if (clientSocket < 0) {
        perror("accept");
        return;
    }

    char buffer[4096];
    while (running) {
        std::memset(buffer, 0, sizeof(buffer));
        ssize_t received = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (received > 0) {
            std::string msg(buffer, received);

            {
                std::lock_guard<std::mutex> lock(messagesMutex);
                messages.push_back(msg);
            }

            if (dispatcher) {
                dispatcher(msg);
            }
        } else if (received == 0) {
            std::cout << "Client disconnected\n";
            close(clientSocket);
            clientSocket = -1;
            break;
        } else {
            perror("recv");
            break;
        }
    }
}

void NetworkPipe::send(const std::string &message) const {
    if (clientSocket != -1) {
        ssize_t sent = ::send(clientSocket, message.c_str(), message.size(), 0);
        if (sent < 0) {
            perror("send");
        }
    }
}

std::vector<std::string> NetworkPipe::getMessages() {
    std::lock_guard<std::mutex> lock(messagesMutex);
    return messages;
}
