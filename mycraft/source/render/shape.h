#pragma once

#include "world/block.h"


struct UIPoint
{
    flt2 position;
    flt4 color;
    flt2 texture;
    float tex_blend = 0.0f;
};

struct UILine
{
    UIPoint a;
    UIPoint b;
};

struct UITriangle
{
    UIPoint a;
    UIPoint b;
    UIPoint c;
};

struct UIProduct
{
    std::vector<UIPoint> points;
    std::vector<UILine> lines;
    std::vector<UITriangle> triangles;
};

struct UIRectangle
{
    flt2 position;
    flt2 size;
    rgb color;
    Block block = Block::AIR;
    float blend = 0.0f;

    void generate( UIProduct& product ) const;
};
