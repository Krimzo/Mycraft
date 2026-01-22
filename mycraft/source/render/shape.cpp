#include "render/shape.h"


void UIProduct::append_point( UIPoint const& point )
{
    get_or_make_render_info( D3D_PRIMITIVE_TOPOLOGY_POINTLIST ).count += 1;
    m_points.push_back( point );
}

void UIProduct::append_line( UILine const& line )
{
    get_or_make_render_info( D3D_PRIMITIVE_TOPOLOGY_LINELIST ).count += 2;
    m_points.push_back( line.a );
    m_points.push_back( line.b );
}

void UIProduct::append_triangle( UITriangle const& triangle )
{
    get_or_make_render_info( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST ).count += 3;
    m_points.push_back( triangle.a );
    m_points.push_back( triangle.b );
    m_points.push_back( triangle.c );
}

void UIProduct::clear()
{
    m_points.clear();
    m_render_info.clear();
}

UIPoint const* UIProduct::point_data() const
{
    return m_points.data();
}

UINT UIProduct::point_count() const
{
    return (UINT) m_points.size();
}

std::vector<UIRenderInfo> const& UIProduct::render_info() const
{
    return m_render_info;
}

UIRenderInfo& UIProduct::get_or_make_render_info( D3D_PRIMITIVE_TOPOLOGY topology )
{
    return ( !m_render_info.empty() && m_render_info.back().topology == topology ) ? m_render_info.back() : m_render_info.emplace_back( UIRenderInfo{
        .topology = topology,
        .offset = (UINT) m_points.size(),
        .count = 0,
        } );
}

void UIRectangle::produce( UIProduct& product ) const
{
    const flt2 w_tl = { position.x, position.y + size.y };
    const flt2 w_tr = { position.x + size.x, position.y + size.y };
    const flt2 w_bl = { position.x, position.y };
    const flt2 w_br = { position.x + size.x, position.y };

    const flt2 uv_tr = { uv_br.x, uv_tl.y };
    const flt2 uv_bl = { uv_tl.x, uv_br.y };

    product.append_triangle( {
        { w_bl, color, uv_bl, tex_blend },
        { w_tl, color, uv_tl, tex_blend },
        { w_tr, color, uv_tr, tex_blend },
        } );
    product.append_triangle( {
        { w_bl, color, uv_bl, tex_blend },
        { w_tr, color, uv_tr, tex_blend },
        { w_br, color, uv_br, tex_blend },
        } );
}

void UIRectangle::center_align( flt2 center, flt2 scale )
{
    position = center - scale * 0.5f;
    size = scale;
}
