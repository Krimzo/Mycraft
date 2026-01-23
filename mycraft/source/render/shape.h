#pragma once

#include "world/block.h"
#include "render/renderer.h"


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

struct UIPointRenderInfo
{
    D3D_PRIMITIVE_TOPOLOGY topology;
    UINT offset = 0;
    UINT count = 0;
};

struct UITextRenderInfo
{
    kl::Text text;
};

struct UITextFormatHandler
{
    kl::GPU& gpu;

    UITextFormatHandler( kl::GPU& gpu );

    kl::TextFormat const& get( float font_size );

private:
    std::map<float, kl::TextFormat> m_formats;
};

struct UIProduct
{
    using RenderInfoStorage = std::vector<std::variant<UIPointRenderInfo, UITextRenderInfo>>;

    Renderer const& renderer;

    UIProduct( Renderer const& renderer );

    void append_point( UIPoint const& point );
    void append_line( UILine const& line );
    void append_triangle( UITriangle const& triangle );
    void append_text( std::string_view const& text, flt4 const& color, float font_height, flt2 position, flt2 rect = {}, bool hor_center = false, bool ver_center = false );

    void clear();

    UIPoint const* point_data() const;
    UINT point_count() const;

    RenderInfoStorage const& render_info() const;

private:
    UITextFormatHandler m_text_format_handler;
    std::vector<UIPoint> m_points;
    RenderInfoStorage m_render_info;

    UIPointRenderInfo& get_or_make_render_info( D3D_PRIMITIVE_TOPOLOGY topology );
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
