#include "render/shape.h"


UITextFormatHandler::UITextFormatHandler( kl::GPU& gpu )
    : gpu( gpu )
{
}

kl::TextFormat const& UITextFormatHandler::get( float font_size )
{
    auto it = m_formats.find( font_size );
    if ( it != m_formats.end() )
        return it->second;
    else
        return m_formats.emplace( font_size, gpu.create_text_format( L"Roboto", DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, font_size ) ).first->second;
}

UIProduct::UIProduct( Renderer const& renderer )
    : renderer( renderer )
    , m_text_format_handler( renderer.game.world.system.gpu )
{
}

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

void UIProduct::append_text( std::string_view const& text, flt4 const& color, float font_height, flt2 position, flt2 rect, bool center )
{
    auto& window = renderer.game.world.system.window;
    const float ar = window.aspect_ratio();
    position.x /= ar;
    rect.x /= ar;

    const int2 window_size = window.size();
    const float dip_font_height = ( font_height * 0.5f * window_size.y ) * 96.0f / window.dpi();
    const flt2 pixel_position = ( flt2{ position.x, -position.y } + flt2{ 1.0f } ) * 0.5f * window_size;
    const flt2 pixel_rect = rect * 0.5f * window_size;

    auto& info = std::get<UITextRenderInfo>( m_render_info.emplace_back( UITextRenderInfo{} ) );
    info.text.format = m_text_format_handler.get( dip_font_height );
    info.text.color = color;
    info.text.position = pixel_position;
    info.text.rect_size = pixel_rect;
    info.text.center = center;
    info.text.data.resize( text.size() );
    for ( size_t i = 0; i < text.size(); i++ )
        info.text.data[i] = (wchar_t) text[i];
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

UIProduct::RenderInfoStorage const& UIProduct::render_info() const
{
    return m_render_info;
}

UIPointRenderInfo& UIProduct::get_or_make_render_info( D3D_PRIMITIVE_TOPOLOGY topology )
{
    if ( !m_render_info.empty() && std::holds_alternative<UIPointRenderInfo>( m_render_info.back() ) && std::get<UIPointRenderInfo>( m_render_info.back() ).topology == topology )
        return std::get<UIPointRenderInfo>( m_render_info.back() );
    else
        return std::get<UIPointRenderInfo>( m_render_info.emplace_back( UIPointRenderInfo{ .topology = topology, .offset = (UINT) m_points.size(), .count = 0 } ) );
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
