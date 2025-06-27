/*
 units.hpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Units and utility structs
 Copyright (c) 2025 maxvdec
*/

#ifndef ATLAS_UNITS_HPP
#define ATLAS_UNITS_HPP

struct Position2d {
    int x;
    int y;

    Position2d(int x = 0, int y = 0) : x(x), y(y) {}
};

struct Size2d {
    int width;
    int height;

    Size2d(int width = 0, int height = 0) : width(width), height(height) {}
};

typedef Size2d Frame;

#endif // ATLAS_UNITS_HPP
