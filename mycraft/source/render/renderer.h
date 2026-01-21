#pragma once

#include "game/game.h"


struct Renderer
{
    Game& game;

    dx::RasterState shadow_raster;
    dx::RasterState cull_raster;
    dx::RasterState no_cull_raster;
    dx::RasterState wireframe_raster;

    dx::DepthState enabled_depth;
    dx::DepthState disabled_depth;
    dx::DepthState write_depth_stencil;
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
    void render_raster();
    void render_tracing();

    void draw_sky( kl::Camera const& camera );
    void draw_hit_block( kl::Camera const& camera );
    void draw_portals_stencil( kl::Camera const& camera );

    void raster_shadows( kl::Camera const& camera );
    void raster_chunks( kl::Camera const& camera, bool write_ds, UINT stencil_ref );
    void tracing_world( kl::Camera const& camera );

    mat4 inv_shadow_cam( kl::Camera camera ) const;
};
