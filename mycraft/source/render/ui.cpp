#include "render/ui.h"


UIMesh::UIMesh( kl::GPU& gpu )
    : gpu( gpu )
{
}

void UIMesh::upload( UIPoint const* data, UINT count )
{
    if ( count < 0 )
        return;

    point_count = count;
    if ( count == 0 )
        return;

    const UINT buffer_size = gpu.vertex_buffer_size( buffer, sizeof( UIPoint ) );
    if ( buffer_size >= count )
    {
        gpu.write_to_buffer( buffer, data, count * sizeof( UIPoint ), true );
        return;
    }

    dx::BufferDescriptor descriptor{};
    descriptor.ByteWidth = count * sizeof( UIPoint );
    descriptor.Usage = D3D11_USAGE_DYNAMIC;
    descriptor.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    descriptor.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    dx::SubresourceDescriptor subresource_data{};
    subresource_data.pSysMem = data;
    buffer = gpu.create_buffer( &descriptor, &subresource_data );

    if constexpr ( kl::IS_DEBUG )
        kl::print( "Created UI buffer: ", count, " UIPoint-s (", count * sizeof( UIPoint ), " bytes)" );
}

void UIMesh::draw( D3D_PRIMITIVE_TOPOLOGY topology ) const
{
    if ( !buffer )
        return;

    gpu.set_draw_type( topology );
    gpu.bind_vertex_buffer( buffer, 0, 0, sizeof( UIPoint ) );
    gpu.draw( point_count, 0 );
}

UI::UI( Renderer const& renderer )
    : renderer( renderer )
    , m_triangle_mesh( renderer.game.world.system.gpu )
    , m_line_mesh( renderer.game.world.system.gpu )
    , m_point_mesh( renderer.game.world.system.gpu )
{
    auto& gpu = renderer.game.world.system.gpu;
    std::vector<dx::LayoutDescriptor> ui_input_layout = {
        { "KL_Layer", 0, DXGI_FORMAT_R8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "KL_Position", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "KL_Color", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "KL_UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "KL_Blend", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    m_shaders = gpu.create_shaders( kl::read_file( "shaders/draw_ui.hlsl" ), ui_input_layout );
}

void UI::update()
{
    reload();
    draw();
}

void UI::reload()
{
    m_product.clear();
    switch ( renderer.game.game_state )
    {
    case GameState::PLAYING: reload_state_playing(); break;
    case GameState::MAIN_MENU: reload_state_main_menu(); break;
    case GameState::EXIT: reload_state_exit(); break;
    }
}

void UI::draw()
{
    auto& gpu = renderer.game.world.system.gpu;

    m_triangle_mesh.upload( (UIPoint*) m_product.triangles.data(), (UINT) m_product.triangles.size() * 3 );
    m_line_mesh.upload( (UIPoint*) m_product.lines.data(), (UINT) m_product.lines.size() * 2 );
    m_point_mesh.upload( (UIPoint*) m_product.points.data(), (UINT) m_product.points.size() * 1 );

    gpu.clear_internal_depth();

    gpu.bind_raster_state( renderer.ui_raster );
    gpu.bind_depth_state( renderer.enabled_depth );
    gpu.bind_blend_state( renderer.enabled_blend );

    gpu.bind_sampler_state_for_pixel_shader( renderer.atlas_sampler, 0 );
    gpu.bind_shader_view_for_pixel_shader( renderer.atlas_shader_view, 0 );

    struct alignas( 16 ) CB
    {
        float AR;
    } cb = {};

    cb.AR = renderer.game.world.system.window.aspect_ratio();
    m_shaders.upload( cb );

    gpu.bind_shaders( m_shaders );
    m_triangle_mesh.draw( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    m_line_mesh.draw( D3D_PRIMITIVE_TOPOLOGY_LINELIST );
    m_point_mesh.draw( D3D_PRIMITIVE_TOPOLOGY_POINTLIST );
    gpu.draw_text();

    gpu.unbind_blend_state();
}

void UI::reload_state_playing()
{
    reload_crosshair();
    reload_toolbar();
}

void UI::reload_state_main_menu()
{
    reload_state_playing();
    reload_main_menu();
}

void UI::reload_state_exit()
{
}

void UI::reload_crosshair()
{
    static constexpr float size = 0.02f;
    static const rgb color{ 255, 255, 255, 190 };
    m_product.lines.push_back( {
        { 1, { 0.0f, -size }, color },
        { 1, { 0.0f, size }, color },
        } );
    m_product.lines.push_back( {
        { 1, { -size, 0.0f }, color },
        { 1, { size, 0.0f }, color },
        } );
}

void UI::reload_toolbar()
{
    static constexpr flt2 item_size{ 0.08f };
    static constexpr flt2 toolbar_padding{ 0.01f };
    static constexpr flt2 toolbar_size = {
        toolbar_padding.x + Inventory::HORIZONTAL_COUNT * ( item_size.x + toolbar_padding.x ),
        toolbar_padding.y + item_size.y + toolbar_padding.y,
    };
    static constexpr flt2 toolbar_position = { -toolbar_size.x * 0.5f, -0.95f };

    auto& inventory = renderer.game.player.inventory;

    UIRectangle toolbar_background{ 3, toolbar_position, toolbar_size, rgb( 0, 0, 0, 190 ) };
    toolbar_background.produce( m_product );

    for ( int i = 0; i < Inventory::HORIZONTAL_COUNT; i++ )
    {
        flt2 item_position = toolbar_position + toolbar_padding + flt2{ i * ( item_size.x + toolbar_padding.x ), 0.0f };
        if ( i == inventory.selected_slot )
        {
            static constexpr flt2 extension = toolbar_padding * 0.5f;
            UIRectangle selected{ 2, item_position - extension, item_size + extension * 2.0f, rgb( 220, 220, 220, 190 ) };
            selected.produce( m_product );
        }
        const Block block = inventory.toolbar[i].value_or( Block::AIR );
        UIRectangle item{ 1, item_position, item_size, {},
            atlas_uv( block, flt2{ 0.0f } ), atlas_uv( block, flt2{ 1.0f } ), 1.0f };
        item.produce( m_product );
    }
}

void UI::reload_main_menu()
{
    UIRectangle main_menu_background;
    main_menu_background.layer = 0;
    main_menu_background.center_align( { 0.0f, 0.0f }, { 1.0f, 1.25f } );
    main_menu_background.color = { 0, 0, 0, 220 };
    main_menu_background.produce( m_product );
}
