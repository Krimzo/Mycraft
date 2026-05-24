#pragma once

#include "game/game.h"


struct Renderer
{
    Game& game;

    dx::RasterState sky_raster;
    dx::RasterState raster_shadows_raster;
    dx::RasterState raster_chunks_raster;
    dx::RasterState hit_block_raster;
    dx::RasterState portals_raster;
    dx::RasterState tracing_chunks_raster;
    dx::RasterState particles_raster;

    dx::DepthState sky_depth;
    dx::DepthState raster_shadows_depth;
    dx::DepthState raster_chunks_depth;
    dx::DepthState hit_block_depth;
    dx::DepthState portals_write_stencil_depth;
    dx::DepthState portals_write_depth_depth;
    dx::DepthState tracing_chunks_depth;
    dx::DepthState particles_depth;

    dx::SamplerState shadow_sampler;
    dx::SamplerState atlas_sampler;

    dx::Buffer sky_mesh;
    dx::Buffer hit_block_mesh;
    dx::Buffer tracing_mesh;
    dx::Buffer portal_mesh;
    dx::Buffer particles_mesh;

    dx::Texture atlas_texture;
    dx::ShaderView atlas_shader_view;

    kl::Shaders sky_shaders;
    kl::Shaders raster_shadows_shaders;
    kl::Shaders raster_chunks_shaders;
    kl::Shaders hit_block_shaders;
    kl::Shaders portals_write_stencil_shaders;
    kl::Shaders portals_write_depth_shaders;
    kl::Shaders tracing_chunks_shaders;
    kl::Shaders particles_shaders;

    Renderer( Game& game );

    void reload_raster_states();
    void reload_depth_states();
    void reload_sampler_states();
    void reload_meshes();
    void reload_textures();
    void reload_shaders();

    bool is_wireframe() const;
    void set_wireframe( bool enabled );

    void render();

private:
    RenderMode m_render_mode_before_particles{};

    void render_raster( kl::Camera const& camera );
    void draw_sky( kl::Camera const& camera, UINT allowed_stencil );
    void draw_raster_shadows( kl::Camera const& camera );
    void draw_raster_chunks( kl::Camera const& camera, plane const* clipping_plane, UINT allowed_stencil );
    void draw_hit_block( kl::Camera const& camera, UINT allowed_stencil );
    void draw_portals( kl::Camera const& camera );

    void render_tracing( kl::Camera const& camera );
    void draw_tracing_chunks( kl::Camera const& camera );

    void render_particles( kl::Camera const& camera );
    void handle_particles( kl::Camera const& camera );

    mat4 get_inv_shadow_cam( kl::Camera camera ) const;
};
