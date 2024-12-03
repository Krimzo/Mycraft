#include "render/ui.h"


UI::UI( Renderer const& renderer )
    : renderer( renderer )
{
    auto& gpu = renderer.game.world.system.gpu;
    std::vector<dx::LayoutDescriptor> ui_input_layout = {
        { "KL_Position", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "KL_Color", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "KL_Texture", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "KL_Blend", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    m_shaders = gpu.create_shaders( kl::read_file( "shaders/draw_ui.hlsl" ), ui_input_layout );
}

void UI::update()
{
    reload_shapes();
    reload_meshes();
}

void UI::render()
{
    auto& gpu = renderer.game.world.system.gpu;

    gpu.bind_raster_state( renderer.no_cull_raster );
    gpu.bind_depth_state( renderer.disabled_depth );
    gpu.bind_blend_state( renderer.enabled_blend );

    gpu.bind_sampler_state_for_pixel_shader( renderer.atlas_sampler, 0 );
    gpu.bind_shader_view_for_pixel_shader( renderer.atlas_shader_view, 0 );

    struct alignas(16) CB
    {
        float AR;
    } cb = {};

    cb.AR = renderer.game.world.system.window.aspect_ratio();
    m_shaders.upload( cb );

    gpu.bind_shaders( m_shaders );
    gpu.draw( m_triangle_mesh, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, sizeof( UIPoint ) );
    gpu.draw( m_line_mesh, D3D_PRIMITIVE_TOPOLOGY_LINELIST, sizeof( UIPoint ) );
    gpu.draw( m_point_mesh, D3D_PRIMITIVE_TOPOLOGY_POINTLIST, sizeof( UIPoint ) );
    gpu.draw_text();

    gpu.unbind_blend_state();
}

dx::Buffer UI::create_mesh( UIPoint* data, UINT count ) const
{
    if ( count <= 0 )
        return {};
    auto& gpu = renderer.game.world.system.gpu;
    return gpu.create_vertex_buffer( data, count * sizeof( UIPoint ) );
}

void UI::reload_shapes()
{
    m_product = {};
    make_crosshair();
    make_toolbar();
}

void UI::reload_meshes()
{
    m_point_mesh = create_mesh( (UIPoint*) m_product.points.data(), (UINT) m_product.points.size() * 1 );
    m_line_mesh = create_mesh( (UIPoint*) m_product.lines.data(), (UINT) m_product.lines.size() * 2 );
    m_triangle_mesh = create_mesh( (UIPoint*) m_product.triangles.data(), (UINT) m_product.triangles.size() * 3 );
}

void UI::make_crosshair()
{
    static constexpr float size = 0.02f;
    static const rgb color{ 255, 255, 255, 190 };
    m_product.lines.push_back( {
        { flt2{ 0.0f, -size }, color },
        { flt2{ 0.0f, size }, color },
        } );
    m_product.lines.push_back( {
        { flt2{ -size, 0.0f }, color },
        { flt2{ size, 0.0f }, color },
        } );
}

void UI::make_toolbar()
{
    static constexpr flt2 item_size = { 0.08f, 0.08f };
    static constexpr flt2 toolbar_padding = { 0.01f, 0.01f };
    static constexpr flt2 toolbar_size = {
        toolbar_padding.x + Inventory::HORIZONTAL_COUNT * (item_size.x + toolbar_padding.x),
        toolbar_padding.y + item_size.y + toolbar_padding.y,
    };
    static constexpr flt2 toolbar_position = { -toolbar_size.x * 0.5f, -0.95f };

    auto& inventory = renderer.game.player.inventory;

    UIRectangle toolbar{ toolbar_position, toolbar_size, rgb( 0, 0, 0, 190 ) };
    toolbar.generate( m_product );

    for ( int i = 0; i < Inventory::HORIZONTAL_COUNT; i++ )
    {
        flt2 item_position = toolbar_position + toolbar_padding + flt2{ i * (item_size.x + toolbar_padding.x), 0.0f };
        if ( i == inventory.selected_slot )
        {
            static constexpr flt2 extension = toolbar_padding * 0.5f;
            UIRectangle selected{ item_position - extension, item_size + extension * 2.0f, rgb( 220, 220, 220, 190 ) };
            selected.generate( m_product );
        }
        UIRectangle item{ item_position, item_size, {}, inventory.toolbar[i].value_or( Block::AIR ), 1.0f };
        item.generate( m_product );
    }
}
