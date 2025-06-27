/*
 main.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: The main entry point for the Atlas Test
 Copyright (c) 2025 maxvdec
*/

#include <atlas/window.hpp>
#include <iostream>

int main() {
    Window mywin = Window("Atlas Test", Frame(800, 600));
    mywin.run();
    return 0;
}
