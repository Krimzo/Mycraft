#include "render/renderer.h"


Renderer::Renderer( Game& game )
    : game( game )
{
    auto& window = game.world.system.window;
    auto& gpu = game.world.system.gpu;

    reload_raster_states();
    reload_depth_states();
    reload_sampler_states();
    reload_meshes();
    reload_textures();
    reload_shaders();

    window.on_resize.emplace_back( [this]( int2 size )
        {
            this->game.world.system.gpu.resize_internal( size, DXGI_FORMAT_D24_UNORM_S8_UINT );
            this->game.world.system.gpu.set_viewport_size( size );
            this->game.player.camera.update_aspect_ratio( size );
        } );
    if constexpr ( kl::IS_DEBUG )
        window.maximize();
    else
        gpu.set_fullscreen( true );
    window.mouse.set_position( window.frame_center() );
}

void Renderer::reload_raster_states()
{
    auto& gpu = game.world.system.gpu;

    dx::RasterStateDescriptor portal_indices_raster_descriptor{};
    portal_indices_raster_descriptor.FillMode = D3D11_FILL_SOLID;
    portal_indices_raster_descriptor.CullMode = D3D11_CULL_NONE;
    portal_indices_raster_descriptor.AntialiasedLineEnable = true;
    portal_indices_raster_descriptor.MultisampleEnable = true;
    portal_indices_raster_descriptor.DepthClipEnable = false;

    dx::RasterStateDescriptor raster_shadows_raster_descriptor{};
    raster_shadows_raster_descriptor.FillMode = D3D11_FILL_SOLID;
    raster_shadows_raster_descriptor.CullMode = D3D11_CULL_BACK;
    raster_shadows_raster_descriptor.SlopeScaledDepthBias = 2.5f;

    portal_indices_raster = gpu.create_raster_state( &portal_indices_raster_descriptor );
    sky_raster = gpu.create_raster_state( false, false );
    raster_shadows_raster = gpu.create_raster_state( &raster_shadows_raster_descriptor );
    raster_chunks_raster = gpu.create_raster_state( false, true );
    hit_block_raster = gpu.create_raster_state( false, true );
    tracing_chunks_raster = gpu.create_raster_state( false, false );
}

void Renderer::reload_depth_states()
{
    auto& gpu = game.world.system.gpu;

    dx::DepthStateDescriptor portal_indices_depth_descriptor{};
    portal_indices_depth_descriptor.DepthEnable = true;
    portal_indices_depth_descriptor.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    portal_indices_depth_descriptor.DepthFunc = D3D11_COMPARISON_LESS;
    portal_indices_depth_descriptor.StencilEnable = true;
    portal_indices_depth_descriptor.StencilReadMask = 0xFF;
    portal_indices_depth_descriptor.StencilWriteMask = 0xFF;
    portal_indices_depth_descriptor.FrontFace.StencilFailOp = D3D11_STENCIL_OP_REPLACE;
    portal_indices_depth_descriptor.BackFace.StencilFailOp = D3D11_STENCIL_OP_REPLACE;
    portal_indices_depth_descriptor.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    portal_indices_depth_descriptor.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    portal_indices_depth_descriptor.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
    portal_indices_depth_descriptor.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
    portal_indices_depth_descriptor.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    portal_indices_depth_descriptor.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    dx::DepthStateDescriptor raster_chunks_depth_descriptor{};
    raster_chunks_depth_descriptor.DepthEnable = true;
    raster_chunks_depth_descriptor.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    raster_chunks_depth_descriptor.DepthFunc = D3D11_COMPARISON_LESS;
    raster_chunks_depth_descriptor.StencilEnable = true;
    raster_chunks_depth_descriptor.StencilReadMask = 0xFF;
    raster_chunks_depth_descriptor.StencilWriteMask = 0xFF;
    raster_chunks_depth_descriptor.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    raster_chunks_depth_descriptor.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    raster_chunks_depth_descriptor.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    raster_chunks_depth_descriptor.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    raster_chunks_depth_descriptor.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    raster_chunks_depth_descriptor.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    raster_chunks_depth_descriptor.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
    raster_chunks_depth_descriptor.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;

    portal_indices_depth = gpu.create_depth_state( &portal_indices_depth_descriptor );
    sky_depth = gpu.create_depth_state( false );
    raster_shadows_depth = gpu.create_depth_state( true );
    raster_chunks_depth = gpu.create_depth_state( &raster_chunks_depth_descriptor );
    hit_block_depth = gpu.create_depth_state( true );
    tracing_chunks_depth = gpu.create_depth_state( false );
}

void Renderer::reload_sampler_states()
{
    auto& gpu = game.world.system.gpu;

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
}

void Renderer::reload_meshes()
{
    auto& gpu = game.world.system.gpu;

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
}

void Renderer::reload_textures()
{
    auto& gpu = game.world.system.gpu;

    atlas_texture = gpu.create_texture( kl::Image( "textures/blocks.png" ) );
    atlas_shader_view = gpu.create_shader_view( atlas_texture, nullptr );
}

void Renderer::reload_shaders()
{
    auto& gpu = game.world.system.gpu;

    const std::vector<dx::LayoutDescriptor> chunk_input_layout = {
        { "KL_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "KL_Texture", 0, DXGI_FORMAT_R8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "KL_Ambient", 0, DXGI_FORMAT_R8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "KL_Block", 0, DXGI_FORMAT_R8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    portal_indices_shaders = gpu.create_shaders( kl::read_file_string( "shaders/portal_indices.hlsl" ) );
    sky_shaders = gpu.create_shaders( kl::read_file_string( "shaders/sky.hlsl" ) );
    raster_shadows_shaders = gpu.create_shaders( kl::read_file_string( "shaders/raster_shadows.hlsl" ), chunk_input_layout );
    raster_chunks_shaders = gpu.create_shaders( kl::read_file_string( "shaders/raster_chunks.hlsl" ), chunk_input_layout );
    hit_block_shaders = gpu.create_shaders( kl::read_file_string( "shaders/hit_block.hlsl" ) );
    tracing_chunks_shaders = gpu.create_shaders( kl::read_file_string( "shaders/tracing_chunks.hlsl" ) );

    if constexpr ( kl::IS_DEBUG )
        kl::print( "Shaders reloaded." );
}

bool Renderer::is_wireframe() const
{
    dx::RasterStateDescriptor desc{};
    raster_chunks_raster->GetDesc( &desc );
    return desc.FillMode == D3D11_FILL_WIREFRAME;
}

void Renderer::set_wireframe( bool enabled )
{
    auto& gpu = game.world.system.gpu;
    dx::RasterStateDescriptor desc{};
    raster_chunks_raster->GetDesc( &desc );
    desc.FillMode = enabled ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
    raster_chunks_raster = gpu.create_raster_state( &desc );
}

void Renderer::render()
{
    auto& window = game.world.system.window;
    if ( window.keyboard.f5.pressed() )
        reload_shaders();

    auto& player_camera = game.player.camera;
    switch ( game.render_mode )
    {
    case RenderMode::RASTER: render_raster( player_camera ); break;
    case RenderMode::TRACING: render_tracing( player_camera ); break;
    }
}

void Renderer::render_raster( kl::Camera const& camera )
{
    draw_portal_indices( camera );
    draw_sky( camera );
    draw_raster_shadows( camera );
    draw_raster_chunks( camera, nullptr, true, 0xFF );
    draw_hit_block( camera );
    draw_portals( camera );
}

void Renderer::draw_portal_indices( kl::Camera const& camera )
{
    auto& gpu = game.world.system.gpu;

    gpu.bind_raster_state( portal_indices_raster );

    struct alignas( 16 ) CB
    {
        mat4 WVP;
    } cb = {};

    int portal_index = 0;
    gpu.bind_shaders( portal_indices_shaders );
    for ( auto& portal : game.portals )
    {
        gpu.bind_depth_state( portal_indices_depth, portal_index++ );

        cb.WVP = camera.matrix() * portal.matrix();
        portal_indices_shaders.upload( cb );

        gpu.draw( portal_mesh );
    }
}

void Renderer::draw_sky( kl::Camera const& camera )
{
    auto& gpu = game.world.system.gpu;

    gpu.bind_raster_state( sky_raster );
    gpu.bind_depth_state( sky_depth );

    struct alignas( 16 ) CB
    {
        mat4 VP;
        flt3 SUN_DIRECTION;
    } cb = {};

    cb.VP = camera.matrix();
    cb.SUN_DIRECTION = game.environment.sun.direction();
    sky_shaders.upload( cb );

    gpu.bind_shaders( sky_shaders );
    gpu.draw( sky_mesh );
}

void Renderer::draw_raster_shadows( kl::Camera const& camera )
{
    auto& gpu = game.world.system.gpu;

    gpu.bind_raster_state( raster_shadows_raster );
    gpu.bind_depth_state( raster_shadows_depth );

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
    raster_shadows_shaders.upload( cb );

    mat4 inv_sun_mat = kl::inverse( cb.VP );
    flt4 sun_pos = inv_sun_mat * flt4( 0.0f, 0.0f, 0.0f, 1.0f );
    sun_pos /= sun_pos.w;

    plane sun_plane;
    sun_plane.position = sun_pos.xyz();
    sun_plane.set_normal( game.environment.sun.direction() );

    gpu.bind_shaders( raster_shadows_shaders );
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

    gpu.bind_raster_state( raster_chunks_raster );
    gpu.bind_depth_state( raster_chunks_depth, stencil_ref );

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
    raster_chunks_shaders.upload( cb );

    plane camera_plane;
    camera_plane.position = camera.position;
    camera_plane.set_normal( camera.forward() );

    gpu.bind_shaders( raster_chunks_shaders );
    for ( int i = 0; i < game.world.chunk_count(); i++ )
    {
        if ( game.world.chunk_visible( camera_plane, i ) )
            gpu.draw( game.world.get_chunk( i ).buffer, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, sizeof( Vertex ) );
    }

    gpu.unbind_shader_view_for_pixel_shader( 1 );
    gpu.unbind_shader_view_for_pixel_shader( 0 );
}

void Renderer::draw_hit_block( kl::Camera const& camera )
{
    auto& gpu = game.world.system.gpu;

    auto opt_payload = game.hit_block;
    if ( !opt_payload )
        return;

    HitPayload const& payload = opt_payload.value();
    BlockPosition block_pos = game.world.get_block_world( payload.chunk_ind, payload.block_ind );

    gpu.bind_raster_state( hit_block_raster );
    gpu.bind_depth_state( hit_block_depth );

    struct alignas( 16 ) CB
    {
        mat4 WVP;
    } cb = {};

    cb.WVP = camera.matrix()
        * mat4::translation( block_pos.to_flt3() - flt3{ 0.001f } )
        * mat4::scaling( flt3{ 1.002f } );
    hit_block_shaders.upload( cb );

    gpu.bind_shaders( hit_block_shaders );
    gpu.draw( hit_block_mesh, D3D_PRIMITIVE_TOPOLOGY_LINESTRIP );
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

void Renderer::render_tracing( kl::Camera const& camera )
{
    draw_sky( camera );
    draw_tracing_chunks( camera );
    draw_hit_block( camera );
}

void Renderer::draw_tracing_chunks( kl::Camera const& camera )
{
    auto& gpu = game.world.system.gpu;

    gpu.bind_raster_state( tracing_chunks_raster );
    gpu.bind_depth_state( tracing_chunks_depth );

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
    tracing_chunks_shaders.upload( cb );

    gpu.bind_shaders( tracing_chunks_shaders );
    gpu.draw( tracing_mesh );

    gpu.unbind_shader_view_for_pixel_shader( 1 );
    gpu.unbind_shader_view_for_pixel_shader( 0 );
}

mat4 Renderer::get_inv_shadow_cam( kl::Camera camera ) const
{
    camera.far_plane *= .25f;
    return kl::inverse( camera.matrix() );
}
