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
        kl::print( "Created UIMesh buffer: ", count, " UIPoint-s (", count * sizeof( UIPoint ), " bytes)" );
}

UI::UI( Renderer const& renderer )
    : renderer( renderer )
    , m_product( renderer )
    , m_mesh( renderer.game.world.system.gpu )
{
    auto& gpu = renderer.game.world.system.gpu;
    std::vector<dx::LayoutDescriptor> ui_input_layout = {
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

    m_mesh.upload( m_product.point_data(), m_product.point_count() );
    if ( !m_mesh.buffer || m_mesh.point_count == 0 )
        return;

    gpu.bind_raster_state( renderer.ui_raster );
    gpu.bind_depth_state( renderer.disabled_depth );
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
    gpu.bind_vertex_buffer( m_mesh.buffer, 0, 0, sizeof( UIPoint ) );
    for ( auto& info_variant : m_product.render_info() )
    {
        if ( auto* point_info = std::get_if<UIPointRenderInfo>( &info_variant ) )
        {
            gpu.set_draw_type( point_info->topology );
            gpu.draw( point_info->count, point_info->offset );
        }
        else if ( auto* text_info = std::get_if<UITextRenderInfo>( &info_variant ) )
        {
            gpu.draw_text_direct( text_info->text );
        }
    }
    gpu.draw_text_batch();

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
    m_product.append_line( {
        { { 0.0f, -size }, color },
        { { 0.0f, size }, color },
        } );
    m_product.append_line( {
        { { -size, 0.0f }, color },
        { { size, 0.0f }, color },
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

    UIRectangle toolbar_background{ toolbar_position, toolbar_size, rgb( 0, 0, 0, 190 ) };
    toolbar_background.produce( m_product );

    for ( int i = 0; i < Inventory::HORIZONTAL_COUNT; i++ )
    {
        flt2 item_position = toolbar_position + toolbar_padding + flt2{ i * ( item_size.x + toolbar_padding.x ), 0.0f };
        if ( i == inventory.selected_slot )
        {
            static constexpr flt2 extension = toolbar_padding * 0.5f;
            UIRectangle selected{ item_position - extension, item_size + extension * 2.0f, rgb( 220, 220, 220, 190 ) };
            selected.produce( m_product );
        }
        const Block block = inventory.toolbar[i].value_or( Block::AIR );
        UIRectangle item{ item_position, item_size, {},
            atlas_uv( block, flt2{ 0.0f } ), atlas_uv( block, flt2{ 1.0f } ), 1.0f };
        item.produce( m_product );
    }
}

void UI::reload_main_menu()
{
    UIRectangle main_menu_background;
    main_menu_background.center_align( { 0.0f, 0.0f }, { 1.0f, 1.25f } );
    main_menu_background.color = { 15, 15, 15, 250 };
    main_menu_background.produce( m_product );

    auto& game = renderer.game;
    auto& window = game.world.system.window;

    static const flt4 DEFAULT_COLOR = rgb{ 30, 30, 30 };
    static const flt4 ENABLED_COLOR = rgb{ 150, 230, 80 };
    static const flt4 DISABLED_COLOR = rgb{ 225, 125, 80 };
    static const flt4 LIGHT_TEXT_COLOR = rgb{ 240, 240, 240 };
    static const flt4 DARK_TEXT_COLOR = rgb{ 30, 30, 30 };

    if ( ui_button( "RESUME", { 0.0f, 0.35f }, { 0.5f, 0.1f }, DEFAULT_COLOR, LIGHT_TEXT_COLOR ) )
    {
        game.resume();
        return; // because mouse.set_position(window.frame_center()) inside game.resume() will cause unintended button press
    }

    if ( ui_button( "V-SYNC", { 0.0f, 0.2f }, { 0.5f, 0.1f }, game.world.system.vsync ? ENABLED_COLOR : DISABLED_COLOR, DARK_TEXT_COLOR ) )
        game.world.system.vsync = !game.world.system.vsync;

    if ( ui_button( "WIREFRAME", { 0.0f, 0.05f }, { 0.5f, 0.1f }, game.world.system.wireframe ? ENABLED_COLOR : DISABLED_COLOR, DARK_TEXT_COLOR ) )
        game.world.system.wireframe = !game.world.system.wireframe;

    if ( ui_button( "RAYTRACING", { 0.0f, -0.1f }, { 0.5f, 0.1f }, ( game.render_mode == RenderMode::TRACING ) ? ENABLED_COLOR : DISABLED_COLOR, DARK_TEXT_COLOR ) )
        game.render_mode = ( game.render_mode == RenderMode::TRACING ) ? RenderMode::RASTER : RenderMode::TRACING;

    if ( ui_button( "EXIT", { 0.0f, -0.25f }, { 0.5f, 0.1f }, DEFAULT_COLOR, LIGHT_TEXT_COLOR ) )
        window.close();
}
