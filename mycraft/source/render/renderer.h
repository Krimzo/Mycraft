#pragma once

#include "game/game.h"


struct Renderer
{
    Game& game;

    dx::RasterState shadow_raster;
    dx::RasterState cull_raster;
    dx::RasterState no_cull_raster;
    dx::RasterState wireframe_raster;
    dx::RasterState portal_stencil_raster;
    dx::RasterState ui_raster;

    dx::DepthState enabled_depth;
    dx::DepthState disabled_depth;
    dx::DepthState write_stencil;
    dx::DepthState compare_stencil;

    dx::SamplerState shadow_sampler;
    dx::SamplerState atlas_sampler;

    dx::BlendState enabled_blend;
    dx::BlendState disabled_blend;

    kl::Shaders draw_sky_shaders;
    kl::Shaders draw_hit_block_shaders;
    kl::Shaders raster_shadow_shaders;
    kl::Shaders raster_chunk_shaders;
    kl::Shaders tracing_world_shaders;
    kl::Shaders portal_stencil_shaders;

    dx::Buffer sky_mesh;
    dx::Buffer hit_block_mesh;
    dx::Buffer tracing_mesh;
    dx::Buffer portal_mesh;

    dx::Texture atlas_texture;
    dx::ShaderView atlas_shader_view;

    Renderer( Game& game );

    void render();

private:
    void reload_shaders();

    void render_raster();
    void render_tracing();

    void draw_sky( kl::Camera const& camera );
    void draw_hit_block( kl::Camera const& camera );
    void draw_portals_stencil( kl::Camera const& camera );
    void draw_portals( kl::Camera const& camera );
    void draw_raster_shadows( kl::Camera const& camera );
    void draw_raster_chunks( kl::Camera const& camera, plane const* clipping_plane, bool should_write_stencil, UINT stencil_ref );
    void draw_tracing_world( kl::Camera const& camera );

    mat4 get_inv_shadow_cam( kl::Camera camera ) const;
};
