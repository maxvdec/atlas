/*
 window.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Window creation and customization functions
 Copyright (c) 2025 maxvdec
*/

#ifndef WINDOW_H
#define WINDOW_H

#include <string>

typedef void *CoreWindowReference;

class Window {
  public:
    std::string title;
    int width;
    int height;

    Window(const std::string &title, int width, int height);
    ~Window();

    void run();

  private:
    CoreWindowReference windowRef;
};

#endif // WINDOW_H
