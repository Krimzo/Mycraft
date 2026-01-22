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

    void clear();
};

struct UIRectangle
{
    flt2 position;
    flt2 size;
    rgb color;
    flt2 uv_tl{ 0.0f };
    flt2 uv_br{ 1.0f };
    float tex_blend = 0.0f;

    void produce( UIProduct& product ) const;
};
