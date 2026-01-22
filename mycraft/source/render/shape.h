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

struct UIRenderInfo
{
    D3D_PRIMITIVE_TOPOLOGY topology;
    UINT offset = 0;
    UINT count = 0;
};

struct UIProduct
{
    void append_point( UIPoint const& point );
    void append_line( UILine const& line );
    void append_triangle( UITriangle const& triangle );

    void clear();

    UIPoint const* point_data() const;
    UINT point_count() const;

    std::vector<UIRenderInfo> const& render_info() const;

private:
    std::vector<UIPoint> m_points;
    std::vector<UIRenderInfo> m_render_info;

    UIRenderInfo& get_or_make_render_info( D3D_PRIMITIVE_TOPOLOGY topology );
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
    void center_align( flt2 center, flt2 scale );
};
