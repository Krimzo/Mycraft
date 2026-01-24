#include "render/renderer.h"


Renderer::Renderer( Game& game )
    : game( game )
{
    auto& window = game.world.system.window;
    auto& gpu = game.world.system.gpu;

    dx::RasterStateDescriptor shadow_raster_descriptor{};
    shadow_raster_descriptor.FillMode = D3D11_FILL_SOLID;
    shadow_raster_descriptor.CullMode = D3D11_CULL_BACK;
    shadow_raster_descriptor.SlopeScaledDepthBias = 2.5f;

    dx::RasterStateDescriptor ui_raster_descriptor{};
    ui_raster_descriptor.FillMode = D3D11_FILL_SOLID;
    ui_raster_descriptor.CullMode = D3D11_CULL_NONE;
    ui_raster_descriptor.AntialiasedLineEnable = true;
    ui_raster_descriptor.MultisampleEnable = true;

    shadow_raster = gpu.create_raster_state( &shadow_raster_descriptor );
    cull_raster = gpu.create_raster_state( false, true );
    no_cull_raster = gpu.create_raster_state( false, false );
    wireframe_raster = gpu.create_raster_state( true, true );
    ui_raster = gpu.create_raster_state( &ui_raster_descriptor );

    dx::DepthStateDescriptor write_depth_stencil_descriptor{};
    write_depth_stencil_descriptor.DepthEnable = true;
    write_depth_stencil_descriptor.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    write_depth_stencil_descriptor.DepthFunc = D3D11_COMPARISON_LESS;
    write_depth_stencil_descriptor.StencilEnable = true;
    write_depth_stencil_descriptor.StencilReadMask = 0xFF;
    write_depth_stencil_descriptor.StencilWriteMask = 0xFF;
    write_depth_stencil_descriptor.FrontFace.StencilFailOp = D3D11_STENCIL_OP_REPLACE;
    write_depth_stencil_descriptor.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    write_depth_stencil_descriptor.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
    write_depth_stencil_descriptor.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    write_depth_stencil_descriptor.BackFace.StencilFailOp = D3D11_STENCIL_OP_REPLACE;
    write_depth_stencil_descriptor.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    write_depth_stencil_descriptor.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
    write_depth_stencil_descriptor.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    dx::DepthStateDescriptor compare_stencil_descriptor{};
    compare_stencil_descriptor.DepthEnable = true;
    compare_stencil_descriptor.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    compare_stencil_descriptor.DepthFunc = D3D11_COMPARISON_LESS;
    compare_stencil_descriptor.StencilEnable = true;
    compare_stencil_descriptor.StencilReadMask = 0xFF;
    compare_stencil_descriptor.StencilWriteMask = 0xFF;
    compare_stencil_descriptor.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    compare_stencil_descriptor.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    compare_stencil_descriptor.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    compare_stencil_descriptor.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
    compare_stencil_descriptor.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    compare_stencil_descriptor.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    compare_stencil_descriptor.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    compare_stencil_descriptor.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;

    enabled_depth = gpu.create_depth_state( true );
    disabled_depth = gpu.create_depth_state( false );
    write_stencil = gpu.create_depth_state( &write_depth_stencil_descriptor );
    compare_stencil = gpu.create_depth_state( &compare_stencil_descriptor );

    dx::SamplerStateDescriptor shadow_sampler_descriptor{};
    shadow_sampler_descriptor.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    shadow_sampler_descriptor.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    shadow_sampler_descriptor.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    shadow_sampler_descriptor.BorderColor[0] = 1.0f;
    shadow_sampler_descriptor.BorderColor[1] = 1.0f;
    shadow_sampler_descriptor.BorderColor[2] = 1.0f;
    shadow_sampler_descriptor.BorderColor[3] = 1.0f;
    shadow_sampler_descriptor.ComparisonFunc = D3D11_COMPARISON_LESS;

    shadow_sampler = gpu.create_sampler_state( &shadow_sampler_descriptor );
    atlas_sampler = gpu.create_sampler_state( false, false );

    enabled_blend = gpu.create_blend_state( true );
    disabled_blend = gpu.create_blend_state( false );

    std::vector<dx::LayoutDescriptor> chunk_input_layout = {
        { "KL_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "KL_Texture", 0, DXGI_FORMAT_R8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "KL_Ambient", 0, DXGI_FORMAT_R8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "KL_Block", 0, DXGI_FORMAT_R8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    draw_sky_shaders = gpu.create_shaders( kl::read_file_string( "shaders/draw_sky.hlsl" ) );
    draw_hit_block_shaders = gpu.create_shaders( kl::read_file_string( "shaders/draw_hit_block.hlsl" ) );
    raster_shadow_shaders = gpu.create_shaders( kl::read_file_string( "shaders/raster_shadows.hlsl" ), chunk_input_layout );
    raster_chunk_shaders = gpu.create_shaders( kl::read_file_string( "shaders/raster_chunks.hlsl" ), chunk_input_layout );
    tracing_world_shaders = gpu.create_shaders( kl::read_file_string( "shaders/tracing_world.hlsl" ) );
    portal_stencil_shaders = gpu.create_shaders( kl::read_file_string( "shaders/portal_stencil.hlsl" ) );

    sky_mesh = gpu.create_cube_mesh( 1.0f );
    hit_block_mesh = gpu.create_vertex_buffer( {
        { { 0.0f, 0.0f, 0.0f } },
        { { 1.0f, 0.0f, 0.0f } },
        { { 1.0f, 0.0f, 1.0f } },
        { { 0.0f, 0.0f, 1.0f } },
        { { 0.0f, 0.0f, 0.0f } },
        { { 0.0f, 1.0f, 0.0f } },
        { { 1.0f, 1.0f, 0.0f } },
        { { 1.0f, 1.0f, 1.0f } },
        { { 0.0f, 1.0f, 1.0f } },
        { { 0.0f, 1.0f, 0.0f } },
        { { 1.0f, 1.0f, 0.0f } },
        { { 1.0f, 0.0f, 0.0f } },
        { { 1.0f, 1.0f, 0.0f } },
        { { 1.0f, 1.0f, 1.0f } },
        { { 1.0f, 0.0f, 1.0f } },
        { { 1.0f, 1.0f, 1.0f } },
        { { 0.0f, 1.0f, 1.0f } },
        { { 0.0f, 0.0f, 1.0f } },
        } );
    tracing_mesh = gpu.create_screen_mesh();
    portal_mesh = gpu.create_cube_mesh( { 1.0f } );

    atlas_texture = gpu.create_texture( kl::Image( "textures/blocks.png" ) );
    atlas_shader_view = gpu.create_shader_view( atlas_texture, nullptr );

    window.on_resize.emplace_back( [this]( int2 size )
        {
            this->game.world.system.gpu.resize_internal( size, DXGI_FORMAT_D24_UNORM_S8_UINT );
            this->game.world.system.gpu.set_viewport_size( size );
            this->game.player.camera.update_aspect_ratio( size );
        } );
    gpu.set_fullscreen( true );
    window.mouse.set_position( window.frame_center() );
}

void Renderer::render()
{
    if ( game.render_mode == RenderMode::TRACING )
    {
        render_tracing();
    }
    else
    {
        render_raster();
    }
}

void Renderer::render_raster()
{
    auto& main_camera = game.player.camera;
    draw_portals_stencil( main_camera );
    draw_sky( main_camera );
    draw_raster_shadows( main_camera );
    draw_raster_chunks( main_camera, nullptr, true, 0xFF );
    draw_hit_block( main_camera );
    draw_portals( main_camera );
}

void Renderer::render_tracing()
{
    auto& main_camera = game.player.camera;
    draw_sky( main_camera );
    draw_tracing_world( main_camera );
    draw_hit_block( main_camera );
}

void Renderer::draw_sky( kl::Camera const& camera )
{
    auto& gpu = game.world.system.gpu;

    gpu.bind_raster_state( no_cull_raster );
    gpu.bind_depth_state( disabled_depth );

    struct alignas( 16 ) CB
    {
        mat4 VP;
        flt3 SUN_DIRECTION;
    } cb = {};

    cb.VP = camera.matrix();
    cb.SUN_DIRECTION = game.environment.sun.direction();
    draw_sky_shaders.upload( cb );

    gpu.bind_shaders( draw_sky_shaders );
    gpu.draw( sky_mesh );
}

void Renderer::draw_hit_block( kl::Camera const& camera )
{
    auto& gpu = game.world.system.gpu;

    auto opt_payload = game.hit_block;
    if ( !opt_payload )
        return;

    HitPayload const& payload = opt_payload.value();
    BlockPosition block_pos = game.world.get_block_world( payload.chunk_ind, payload.block_ind );

    gpu.bind_raster_state( cull_raster );
    gpu.bind_depth_state( enabled_depth );

    struct alignas( 16 ) CB
    {
        mat4 WVP;
    } cb = {};

    cb.WVP = camera.matrix()
        * mat4::translation( block_pos.to_flt3() - flt3{ 0.001f } )
        * mat4::scaling( flt3{ 1.002f } );
    draw_hit_block_shaders.upload( cb );

    gpu.bind_shaders( draw_hit_block_shaders );
    gpu.draw( hit_block_mesh, D3D_PRIMITIVE_TOPOLOGY_LINESTRIP );
}

void Renderer::draw_portals_stencil( kl::Camera const& camera )
{
    auto& gpu = game.world.system.gpu;

    gpu.bind_raster_state( cull_raster );

    struct alignas( 16 ) CB
    {
        mat4 WVP;
    } cb = {};

    int portal_index = 0;
    gpu.bind_shaders( portal_stencil_shaders );
    for ( auto& portal : game.portals )
    {
        gpu.bind_depth_state( write_stencil, portal_index++ );

        cb.WVP = camera.matrix() * portal.matrix();
        portal_stencil_shaders.upload( cb );

        gpu.draw( portal_mesh );
    }
}

void Renderer::draw_portals( kl::Camera const& camera )
{
    auto& gpu = game.world.system.gpu;

    gpu.context()->ClearDepthStencilView( gpu.back_depth_view().get(), D3D11_CLEAR_DEPTH, 1.0f, 0xFF );

    int portal_index = 0;
    for ( auto& portal : game.portals )
    {
        const flt3 friend_position = portal.friend_portal->position;
        const mat4 friend_matrix = portal.friend_portal->matrix();
        const mat4 relative_matrix = friend_matrix * kl::inverse( portal.matrix() );

        kl::Camera portal_camera = camera;
        portal_camera.position = ( relative_matrix * flt4( camera.position, 1.0f ) ).xyz();
        portal_camera.set_forward( ( relative_matrix * flt4( camera.forward(), 0.0f ) ).xyz() );
        draw_raster_shadows( portal_camera );

        plane clipping_plane;
        clipping_plane.position = friend_position;
        clipping_plane.set_normal( ( friend_matrix * flt4( 0.0f, 0.0f, 1.0f, 0.0f ) ).xyz() );
        if ( clipping_plane.in_front( portal_camera.position ) )
            clipping_plane.set_normal( -clipping_plane.normal() );
        draw_raster_chunks( portal_camera, &clipping_plane, false, portal_index++ );
    }
}

void Renderer::draw_raster_shadows( kl::Camera const& camera )
{
    auto& gpu = game.world.system.gpu;

    gpu.bind_raster_state( shadow_raster );
    gpu.bind_depth_state( enabled_depth );

    gpu.bind_sampler_state_for_pixel_shader( atlas_sampler, 0 );
    gpu.bind_shader_view_for_pixel_shader( atlas_shader_view, 0 );

    gpu.set_viewport_size( int2{ game.environment.sun.resolution() } );
    gpu.bind_target_depth_views( {}, game.environment.sun.depth_view( 0 ) );
    gpu.clear_depth_view( game.environment.sun.depth_view( 0 ) );

    struct alignas( 16 ) CB
    {
        mat4 VP;
    } cb = {};

    cb.VP = game.environment.sun.matrix( get_inv_shadow_cam( camera ) );
    raster_shadow_shaders.upload( cb );

    mat4 inv_sun_mat = kl::inverse( cb.VP );
    flt4 sun_pos = inv_sun_mat * flt4( 0.0f, 0.0f, -1.0f, 1.0f );
    sun_pos /= sun_pos.w;

    plane sun_plane;
    sun_plane.position = sun_pos.xyz();
    sun_plane.set_normal( game.environment.sun.direction() );

    gpu.bind_shaders( raster_shadow_shaders );
    for ( int i = 0; i < game.world.chunk_count(); i++ )
    {
        if ( game.world.chunk_visible( sun_plane, i ) )
            gpu.draw( game.world.get_chunk( i ).buffer, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, sizeof( Vertex ) );
    }

    gpu.bind_internal_views();
    gpu.set_viewport_size( game.world.system.window.size() );
}

void Renderer::draw_raster_chunks( kl::Camera const& camera, plane const* clipping_plane, bool should_write_stencil, UINT stencil_ref )
{
    auto& gpu = game.world.system.gpu;

    gpu.bind_raster_state( game.world.system.wireframe ? wireframe_raster : cull_raster );
    gpu.bind_depth_state( should_write_stencil ? write_stencil : compare_stencil, stencil_ref );

    gpu.bind_shader_view_for_pixel_shader( game.environment.sun.shader_view( 0 ), 0 );
    gpu.bind_shader_view_for_pixel_shader( atlas_shader_view, 1 );

    gpu.bind_sampler_state_for_pixel_shader( shadow_sampler, 0 );
    gpu.bind_sampler_state_for_pixel_shader( atlas_sampler, 1 );

    struct alignas( 16 ) CB
    {
        mat4 VP;
        mat4 SUN_VP;
        flt3 CAMERA_POSITION;
        float ELAPSED_TIME;
        flt3 SUN_DIRECTION;
        float RENDER_DISTANCE;
        flt2 SHADOW_TEXEL_SIZE;
        float ENABLE_CLIPPING_PLANE;
        alignas( 16 ) flt3 CLIPPING_PLANE_POSITION;
        alignas( 16 ) flt3 CLIPPING_PLANE_NORMAL;
    } cb = {};

    cb.VP = camera.matrix();
    cb.SUN_VP = game.environment.sun.matrix( get_inv_shadow_cam( camera ) );
    cb.CAMERA_POSITION = camera.position;
    cb.ELAPSED_TIME = game.world.system.timer.elapsed();
    cb.SUN_DIRECTION = game.environment.sun.direction();
    cb.RENDER_DISTANCE = (float) game.world.render_distance();
    cb.SHADOW_TEXEL_SIZE = flt2{ 1.0f / game.environment.sun.resolution() };
    cb.ENABLE_CLIPPING_PLANE = (float) (bool) clipping_plane;
    cb.CLIPPING_PLANE_POSITION = clipping_plane ? clipping_plane->position : flt3{};
    cb.CLIPPING_PLANE_NORMAL = clipping_plane ? clipping_plane->normal() : flt3{};
    raster_chunk_shaders.upload( cb );

    plane camera_plane;
    camera_plane.position = camera.position;
    camera_plane.set_normal( camera.forward() );

    gpu.bind_shaders( raster_chunk_shaders );
    for ( int i = 0; i < game.world.chunk_count(); i++ )
    {
        if ( game.world.chunk_visible( camera_plane, i ) )
            gpu.draw( game.world.get_chunk( i ).buffer, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, sizeof( Vertex ) );
    }

    gpu.unbind_shader_view_for_pixel_shader( 1 );
    gpu.unbind_shader_view_for_pixel_shader( 0 );
}

void Renderer::draw_tracing_world( kl::Camera const& camera )
{
    auto& gpu = game.world.system.gpu;

    gpu.bind_raster_state( no_cull_raster );
    gpu.bind_depth_state( disabled_depth );

    gpu.bind_shader_view_for_pixel_shader( game.world.get_tracing_view(), 0 );
    gpu.bind_shader_view_for_pixel_shader( atlas_shader_view, 1 );

    gpu.bind_sampler_state_for_pixel_shader( atlas_sampler, 0 );

    struct alignas( 16 ) CB
    {
        mat4 INV_CAM;
        flt3 CAMERA_POSITION;
        float RENDER_DISTANCE;
        flt3 SUN_DIRECTION;
    } cb = {};

    cb.INV_CAM = kl::inverse( camera.matrix() );
    cb.CAMERA_POSITION = camera.position;
    cb.RENDER_DISTANCE = (float) game.world.render_distance();
    cb.SUN_DIRECTION = game.environment.sun.direction();
    tracing_world_shaders.upload( cb );

    gpu.bind_shaders( tracing_world_shaders );
    gpu.draw( tracing_mesh );

    gpu.unbind_shader_view_for_pixel_shader( 1 );
    gpu.unbind_shader_view_for_pixel_shader( 0 );
}

mat4 Renderer::get_inv_shadow_cam( kl::Camera camera ) const
{
    camera.near_plane = 0.01f;
    camera.far_plane = flt2( CHUNK_WIDTH ).length();
    return kl::inverse( camera.matrix() );
}
