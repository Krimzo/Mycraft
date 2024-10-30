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

	dx::SamplerState shadow_sampler;
	dx::SamplerState atlas_sampler;

	dx::BlendState enabled_blend;
	dx::BlendState disabled_blend;

	kl::Shaders draw_sky_shaders;
	kl::Shaders draw_hit_block_shaders;
	kl::Shaders raster_shadow_shaders;
	kl::Shaders raster_chunk_shaders;
	kl::Shaders tracing_world_shaders;

	dx::Buffer sky_mesh;
	dx::Buffer hit_block_mesh;
	dx::Buffer tracing_mesh;

	dx::Texture atlas_texture;
	dx::ShaderView atlas_shader_view;

	Renderer(Game& game);

	void render();

private:
	void render_raster();
	void render_tracing();

	void draw_sky();
	void draw_hit_block();

	void raster_shadows();
	void raster_chunks();
	void tracing_world();

	mat4 inv_shadow_cam() const;
};
