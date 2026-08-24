#include "render/renderer.h"

static constexpr UINT8 DEFAULT_STENCIL_VALUE = 0xFF;
static constexpr int PARTICLE_GENERATE_COUNT = 5'000'000;
static constexpr float PARTICLE_GENERATE_HALF_BOX_SIZE = 35.0f;

Renderer::Renderer(Game& game) : game(game)
{
    auto& window = game.world.system.window;
    auto& gpu = game.world.system.gpu;

    reload_raster_states();
    reload_depth_states();
    reload_sampler_states();
    reload_meshes();
    reload_textures();
    reload_shaders();

    window.on_resize.emplace_back([this](int2 size) {
        this->game.world.system.gpu.resize_internal(size, DXGI_FORMAT_D24_UNORM_S8_UINT);
        this->game.world.system.gpu.set_viewport_size(size);
        this->game.player.camera.update_aspect_ratio(size);
    });
    if constexpr (kl::IS_DEBUG)
        window.maximize();
    else
        gpu.set_fullscreen(true);
    window.mouse.set_position(window.frame_center());
}

void Renderer::reload_raster_states()
{
    auto& gpu = game.world.system.gpu;

    dx::RasterStateDescriptor raster_shadows_raster_descriptor{};
    raster_shadows_raster_descriptor.FillMode = D3D11_FILL_SOLID;
    raster_shadows_raster_descriptor.CullMode = D3D11_CULL_BACK;
    raster_shadows_raster_descriptor.SlopeScaledDepthBias = 2.5f;

    dx::RasterStateDescriptor portals_raster_descriptor{};
    portals_raster_descriptor.FillMode = D3D11_FILL_SOLID;
    portals_raster_descriptor.CullMode = D3D11_CULL_NONE;
    portals_raster_descriptor.AntialiasedLineEnable = true;
    portals_raster_descriptor.MultisampleEnable = true;
    portals_raster_descriptor.DepthClipEnable = false;

    sky_raster = gpu.create_raster_state(false, false);
    raster_shadows_raster = gpu.create_raster_state(&raster_shadows_raster_descriptor);
    raster_chunks_raster = gpu.create_raster_state(false, true);
    hit_block_raster = gpu.create_raster_state(false, true);
    portals_raster = gpu.create_raster_state(&portals_raster_descriptor);
    tracing_chunks_raster = gpu.create_raster_state(false, false);
    particles_raster = gpu.create_raster_state(false, false);
}

void Renderer::reload_depth_states()
{
    auto& gpu = game.world.system.gpu;

    dx::DepthStateDescriptor sky_depth_descriptor{};
    sky_depth_descriptor.DepthEnable = false;
    sky_depth_descriptor.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    sky_depth_descriptor.DepthFunc = D3D11_COMPARISON_LESS;
    sky_depth_descriptor.StencilEnable = true;
    sky_depth_descriptor.StencilReadMask = 0xFF;
    sky_depth_descriptor.StencilWriteMask = 0xFF;
    sky_depth_descriptor.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    sky_depth_descriptor.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
    sky_depth_descriptor.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    sky_depth_descriptor.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    sky_depth_descriptor.BackFace = sky_depth_descriptor.FrontFace;

    dx::DepthStateDescriptor raster_chunks_depth_descriptor = sky_depth_descriptor;
    raster_chunks_depth_descriptor.DepthEnable = true;

    dx::DepthStateDescriptor hit_block_depth_descriptor = sky_depth_descriptor;
    hit_block_depth_descriptor.DepthEnable = true;

    dx::DepthStateDescriptor portals_write_stencil_depth_descriptor{};
    portals_write_stencil_depth_descriptor.DepthEnable = true;
    portals_write_stencil_depth_descriptor.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    portals_write_stencil_depth_descriptor.DepthFunc = D3D11_COMPARISON_LESS;
    portals_write_stencil_depth_descriptor.StencilEnable = true;
    portals_write_stencil_depth_descriptor.StencilReadMask = 0xFF;
    portals_write_stencil_depth_descriptor.StencilWriteMask = 0xFF;
    portals_write_stencil_depth_descriptor.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    portals_write_stencil_depth_descriptor.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    portals_write_stencil_depth_descriptor.FrontFace.StencilFailOp = D3D11_STENCIL_OP_REPLACE;
    portals_write_stencil_depth_descriptor.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
    portals_write_stencil_depth_descriptor.BackFace = portals_write_stencil_depth_descriptor.FrontFace;

    dx::DepthStateDescriptor portals_write_depth_depth_descriptor{};
    portals_write_depth_depth_descriptor.DepthEnable = true;
    portals_write_depth_depth_descriptor.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    portals_write_depth_depth_descriptor.DepthFunc = D3D11_COMPARISON_ALWAYS;
    portals_write_depth_depth_descriptor.StencilEnable = true;
    portals_write_depth_depth_descriptor.StencilReadMask = 0xFF;
    portals_write_depth_depth_descriptor.StencilWriteMask = 0xFF;
    portals_write_depth_depth_descriptor.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    portals_write_depth_depth_descriptor.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
    portals_write_depth_depth_descriptor.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    portals_write_depth_depth_descriptor.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    portals_write_depth_depth_descriptor.BackFace = portals_write_depth_depth_descriptor.FrontFace;

    dx::DepthStateDescriptor particles_depth_descriptor = portals_write_stencil_depth_descriptor;
    particles_depth_descriptor.DepthEnable = true;

    sky_depth = gpu.create_depth_state(&sky_depth_descriptor);
    raster_shadows_depth = gpu.create_depth_state(true);
    raster_chunks_depth = gpu.create_depth_state(&raster_chunks_depth_descriptor);
    hit_block_depth = gpu.create_depth_state(&hit_block_depth_descriptor);
    portals_write_stencil_depth = gpu.create_depth_state(&portals_write_stencil_depth_descriptor);
    portals_write_depth_depth = gpu.create_depth_state(&portals_write_depth_depth_descriptor);
    tracing_chunks_depth = gpu.create_depth_state(false);
    particles_depth = gpu.create_depth_state(&particles_depth_descriptor);
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

    shadow_sampler = gpu.create_sampler_state(&shadow_sampler_descriptor);
    atlas_sampler = gpu.create_sampler_state(false, false);
}

void Renderer::reload_meshes()
{
    auto& gpu = game.world.system.gpu;

    sky_mesh = gpu.create_cube_mesh(1.0f);
    hit_block_mesh = gpu.create_vertex_buffer({
        {{0.0f, 0.0f, 0.0f}},
        {{1.0f, 0.0f, 0.0f}},
        {{1.0f, 0.0f, 1.0f}},
        {{0.0f, 0.0f, 1.0f}},
        {{0.0f, 0.0f, 0.0f}},
        {{0.0f, 1.0f, 0.0f}},
        {{1.0f, 1.0f, 0.0f}},
        {{1.0f, 1.0f, 1.0f}},
        {{0.0f, 1.0f, 1.0f}},
        {{0.0f, 1.0f, 0.0f}},
        {{1.0f, 1.0f, 0.0f}},
        {{1.0f, 0.0f, 0.0f}},
        {{1.0f, 1.0f, 0.0f}},
        {{1.0f, 1.0f, 1.0f}},
        {{1.0f, 0.0f, 1.0f}},
        {{1.0f, 1.0f, 1.0f}},
        {{0.0f, 1.0f, 1.0f}},
        {{0.0f, 0.0f, 1.0f}},
    });
    tracing_mesh = gpu.create_screen_mesh();
    portal_mesh = gpu.create_cube_mesh({1.0f});
    particles_mesh = {};
}

void Renderer::reload_textures()
{
    auto& gpu = game.world.system.gpu;

    atlas_texture = gpu.create_texture(kl::Image("textures/blocks.png"));
    atlas_shader_view = gpu.create_shader_view(atlas_texture, nullptr);
}

void Renderer::reload_shaders()
{
    auto& gpu = game.world.system.gpu;

    const std::initializer_list<dx::LayoutDescriptor> chunk_input_layout = {
        {"KL_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,
         0},
        {"KL_Texture", 0, DXGI_FORMAT_R8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"KL_Ambient", 0, DXGI_FORMAT_R8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"KL_Block", 0, DXGI_FORMAT_R8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    const std::initializer_list<dx::LayoutDescriptor> particle_input_layout = {
        {"KL_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,
         0},
        {"KL_Color", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    sky_shaders = gpu.create_shaders(kl::read_file_string("shaders/sky.hlsl"));
    raster_shadows_shaders =
        gpu.create_shaders(kl::read_file_string("shaders/raster_shadows.hlsl"), chunk_input_layout);
    raster_chunks_shaders = gpu.create_shaders(kl::read_file_string("shaders/raster_chunks.hlsl"), chunk_input_layout);
    hit_block_shaders = gpu.create_shaders(kl::read_file_string("shaders/hit_block.hlsl"));
    portals_write_stencil_shaders = gpu.create_shaders(kl::read_file_string("shaders/portals_write_stencil.hlsl"));
    portals_write_depth_shaders = gpu.create_shaders(kl::read_file_string("shaders/portals_write_depth.hlsl"));
    tracing_chunks_shaders = gpu.create_shaders(kl::read_file_string("shaders/tracing_chunks.hlsl"));
    particles_shaders = gpu.create_shaders(kl::read_file_string("shaders/particles.hlsl"), particle_input_layout);

    if constexpr (kl::IS_DEBUG)
        kl::print("Shaders reloaded.");
}

bool Renderer::is_wireframe() const
{
    dx::RasterStateDescriptor desc{};
    raster_chunks_raster->GetDesc(&desc);
    return desc.FillMode == D3D11_FILL_WIREFRAME;
}

void Renderer::set_wireframe(bool enabled)
{
    auto& gpu = game.world.system.gpu;
    dx::RasterStateDescriptor desc{};
    raster_chunks_raster->GetDesc(&desc);
    desc.FillMode = enabled ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
    raster_chunks_raster = gpu.create_raster_state(&desc);
}

void Renderer::render()
{
    auto& window = game.world.system.window;
    if (window.keyboard.f5.pressed())
        reload_shaders();

    auto& player_camera = game.player.camera;
    switch (game.render_mode)
    {
    case RenderMode::RASTER:
        render_raster(player_camera);
        break;
    case RenderMode::TRACING:
        render_tracing(player_camera);
        break;
    case RenderMode::PARTICLES:
        render_particles(player_camera);
        break;
    }

    handle_particles(player_camera);
}

void Renderer::render_raster(kl::Camera const& camera)
{
    draw_sky(camera, DEFAULT_STENCIL_VALUE);
    draw_raster_shadows(camera);
    draw_raster_chunks(camera, nullptr, DEFAULT_STENCIL_VALUE);
    draw_hit_block(camera, DEFAULT_STENCIL_VALUE);
    draw_portals(camera);
}

void Renderer::draw_sky(kl::Camera const& camera, UINT allowed_stencil)
{
    auto& gpu = game.world.system.gpu;

    gpu.bind_raster_state(sky_raster);
    gpu.bind_depth_state(sky_depth, allowed_stencil);

    struct alignas(16) CB
    {
        mat4 VP;
        flt3 SUN_DIRECTION;
    } cb = {};

    cb.VP = camera.matrix();
    cb.SUN_DIRECTION = game.environment.sun.direction();
    sky_shaders.upload(cb);

    gpu.bind_shaders(sky_shaders);
    gpu.draw(sky_mesh);
}

void Renderer::draw_raster_shadows(kl::Camera const& camera)
{
    auto& gpu = game.world.system.gpu;

    gpu.bind_raster_state(raster_shadows_raster);
    gpu.bind_depth_state(raster_shadows_depth);

    gpu.bind_sampler_state_for_pixel_shader(atlas_sampler, 0);
    gpu.bind_shader_view_for_pixel_shader(atlas_shader_view, 0);

    gpu.set_viewport_size(int2{game.environment.sun.resolution()});
    gpu.bind_target_depth_views({}, game.environment.sun.depth_view(0));
    gpu.clear_depth_view(game.environment.sun.depth_view(0));

    struct alignas(16) CB
    {
        mat4 VP;
    } cb = {};

    cb.VP = game.environment.sun.matrix(get_inv_shadow_cam(camera));
    raster_shadows_shaders.upload(cb);

    mat4 inv_sun_mat = kl::inverse(cb.VP);
    flt4 sun_pos = inv_sun_mat * flt4(0.0f, 0.0f, 0.0f, 1.0f);
    sun_pos /= sun_pos.w;

    plane sun_plane;
    sun_plane.position = sun_pos.xyz();
    sun_plane.set_normal(game.environment.sun.direction());

    gpu.bind_shaders(raster_shadows_shaders);
    for (int i = 0; i < game.world.chunk_count(); i++)
        if (game.world.chunk_visible(sun_plane, i))
            gpu.draw(game.world.get_chunk(i).buffer, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, sizeof(Vertex));

    gpu.bind_internal_views();
    gpu.set_viewport_size(game.world.system.window.size());
}

void Renderer::draw_raster_chunks(kl::Camera const& camera, plane const* clipping_plane, UINT allowed_stencil)
{
    auto& gpu = game.world.system.gpu;

    gpu.bind_raster_state(raster_chunks_raster);
    gpu.bind_depth_state(raster_chunks_depth, allowed_stencil);

    gpu.bind_shader_view_for_pixel_shader(game.environment.sun.shader_view(0), 0);
    gpu.bind_shader_view_for_pixel_shader(atlas_shader_view, 1);

    gpu.bind_sampler_state_for_pixel_shader(shadow_sampler, 0);
    gpu.bind_sampler_state_for_pixel_shader(atlas_sampler, 1);

    struct alignas(16) CB
    {
        mat4 VP;
        mat4 SUN_VP;
        flt3 CAMERA_POSITION;
        float ELAPSED_TIME;
        flt3 SUN_DIRECTION;
        float RENDER_DISTANCE;
        flt2 SHADOW_TEXEL_SIZE;
        float ENABLE_CLIPPING_PLANE;
        alignas(16) flt3 CLIPPING_PLANE_POSITION;
        alignas(16) flt3 CLIPPING_PLANE_NORMAL;
    } cb = {};

    cb.VP = camera.matrix();
    cb.SUN_VP = game.environment.sun.matrix(get_inv_shadow_cam(camera));
    cb.CAMERA_POSITION = camera.position;
    cb.ELAPSED_TIME = game.world.system.timer.elapsed();
    cb.SUN_DIRECTION = game.environment.sun.direction();
    cb.RENDER_DISTANCE = (float)game.world.render_distance();
    cb.SHADOW_TEXEL_SIZE = flt2{1.0f / game.environment.sun.resolution()};
    cb.ENABLE_CLIPPING_PLANE = (float)(bool)clipping_plane;
    cb.CLIPPING_PLANE_POSITION = clipping_plane ? clipping_plane->position : flt3{};
    cb.CLIPPING_PLANE_NORMAL = clipping_plane ? clipping_plane->normal() : flt3{};
    raster_chunks_shaders.upload(cb);

    plane camera_plane;
    camera_plane.position = camera.position;
    camera_plane.set_normal(camera.forward());

    gpu.bind_shaders(raster_chunks_shaders);
    for (int i = 0; i < game.world.chunk_count(); i++)
        if (game.world.chunk_visible(camera_plane, i))
            gpu.draw(game.world.get_chunk(i).buffer, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, sizeof(Vertex));

    gpu.unbind_shader_view_for_pixel_shader(1);
    gpu.unbind_shader_view_for_pixel_shader(0);
}

void Renderer::draw_hit_block(kl::Camera const& camera, UINT allowed_stencil)
{
    auto& gpu = game.world.system.gpu;

    auto opt_payload = game.hit_block;
    if (!opt_payload)
        return;

    HitPayload const& payload = opt_payload.value();
    BlockPosition block_pos = game.world.get_block_world(payload.chunk_ind, payload.block_ind);

    gpu.bind_raster_state(hit_block_raster);
    gpu.bind_depth_state(hit_block_depth, allowed_stencil);

    struct alignas(16) CB
    {
        mat4 WVP;
    } cb = {};

    cb.WVP = camera.matrix() * mat4::translation(block_pos.to_flt3() - flt3{0.001f}) * mat4::scaling(flt3{1.002f});
    hit_block_shaders.upload(cb);

    gpu.bind_shaders(hit_block_shaders);
    gpu.draw(hit_block_mesh, D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
}

void Renderer::draw_portals(kl::Camera const& camera)
{
    auto& gpu = game.world.system.gpu;

    struct alignas(16) CB
    {
        mat4 WVP;
    } cb = {};

    int portal_index = 0;
    for (auto& portal : game.portals)
    {
        // prepare
        const flt3 friend_position = portal.friend_portal->position;
        const mat4 friend_matrix = portal.friend_portal->matrix();
        const mat4 relative_matrix = friend_matrix * kl::inverse(portal.matrix());

        kl::Camera portal_camera = camera;
        portal_camera.position = (relative_matrix * flt4(camera.position, 1.0f)).xyz();
        portal_camera.set_forward((relative_matrix * flt4(camera.forward(), 0.0f)).xyz());

        plane clipping_plane;
        clipping_plane.position = friend_position;
        clipping_plane.set_normal((friend_matrix * flt4(0.0f, 0.0f, 1.0f, 0.0f)).xyz());
        if (clipping_plane.in_front(portal_camera.position))
            clipping_plane.set_normal(-clipping_plane.normal());

        gpu.bind_raster_state(portals_raster);
        cb.WVP = camera.matrix() * portal.matrix();

        // write stencil
        gpu.bind_depth_state(portals_write_stencil_depth, portal_index);
        portals_write_stencil_shaders.upload(cb);
        gpu.bind_shaders(portals_write_stencil_shaders);
        gpu.draw(portal_mesh);

        // write depth
        gpu.bind_depth_state(portals_write_depth_depth, portal_index);
        portals_write_depth_shaders.upload(cb);
        gpu.bind_shaders(portals_write_depth_shaders);
        gpu.draw(portal_mesh);

        // draw
        draw_sky(portal_camera, portal_index);
        draw_raster_shadows(portal_camera);
        draw_raster_chunks(portal_camera, &clipping_plane, portal_index);

        portal_index += 1;
    }
}

void Renderer::render_tracing(kl::Camera const& camera)
{
    draw_sky(camera, DEFAULT_STENCIL_VALUE);
    draw_tracing_chunks(camera);
    draw_hit_block(camera, DEFAULT_STENCIL_VALUE);
}

void Renderer::draw_tracing_chunks(kl::Camera const& camera)
{
    auto& gpu = game.world.system.gpu;

    gpu.bind_raster_state(tracing_chunks_raster);
    gpu.bind_depth_state(tracing_chunks_depth);

    gpu.bind_shader_view_for_pixel_shader(game.world.get_tracing_view(), 0);
    gpu.bind_shader_view_for_pixel_shader(atlas_shader_view, 1);

    gpu.bind_sampler_state_for_pixel_shader(atlas_sampler, 0);

    struct alignas(16) CB
    {
        mat4 INV_CAM;
        flt3 CAMERA_POSITION;
        float RENDER_DISTANCE;
        flt3 SUN_DIRECTION;
    } cb = {};

    cb.INV_CAM = kl::inverse(camera.matrix());
    cb.CAMERA_POSITION = camera.position;
    cb.RENDER_DISTANCE = (float)game.world.render_distance();
    cb.SUN_DIRECTION = game.environment.sun.direction();
    tracing_chunks_shaders.upload(cb);

    gpu.bind_shaders(tracing_chunks_shaders);
    gpu.draw(tracing_mesh);

    gpu.unbind_shader_view_for_pixel_shader(1);
    gpu.unbind_shader_view_for_pixel_shader(0);
}

void Renderer::render_particles(kl::Camera const& camera)
{
    auto& gpu = game.world.system.gpu;

    gpu.bind_raster_state(particles_raster);
    gpu.bind_depth_state(particles_depth);

    gpu.context()->ClearDepthStencilView(gpu.back_depth_view().get(), D3D11_CLEAR_STENCIL, 0.0f, 254);

    struct CB
    {
        mat4 WVP;
    };

    CB cb = {};
    cb.WVP = camera.matrix();
    particles_shaders.upload(cb);

    gpu.bind_shaders(particles_shaders);
    gpu.draw(particles_mesh, D3D_PRIMITIVE_TOPOLOGY_POINTLIST, sizeof(flt3));

    render_raster(camera);
}

void Renderer::handle_particles(kl::Camera const& camera)
{
    if (!game.world.system.window.keyboard.p.pressed())
        return;

    if (game.render_mode == RenderMode::PARTICLES)
    {
        game.render_mode = m_render_mode_before_particles;
        return;
    }

    m_render_mode_before_particles = game.render_mode;
    game.render_mode = RenderMode::PARTICLES;

    std::vector<flt3> particles;
    particles.resize(PARTICLE_GENERATE_COUNT);
    std::for_each(std::execution::par, particles.begin(), particles.end(), [&](flt3& p) {
        p = camera.position + kl::random::gen_float3(-PARTICLE_GENERATE_HALF_BOX_SIZE, PARTICLE_GENERATE_HALF_BOX_SIZE);
    });

    auto& gpu = game.world.system.gpu;
    particles_mesh = gpu.create_vertex_buffer(particles.data(), (UINT)particles.size() * sizeof(flt3));
}

mat4 Renderer::get_inv_shadow_cam(kl::Camera camera) const
{
    camera.far_plane *= .25f;
    return kl::inverse(camera.matrix());
}
