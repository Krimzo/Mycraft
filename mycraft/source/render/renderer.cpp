#include "render/renderer.h"


Renderer::Renderer(Game& game)
	: game(game)
{
	auto& window = game.world.system.window;
	auto& gpu = game.world.system.gpu;

	dx::RasterStateDescriptor shadow_raster_descriptor{};
	shadow_raster_descriptor.FillMode = D3D11_FILL_SOLID;
	shadow_raster_descriptor.CullMode = D3D11_CULL_BACK;
	shadow_raster_descriptor.SlopeScaledDepthBias = 2.5f;

	shadow_raster = gpu.create_raster_state(&shadow_raster_descriptor);
	cull_raster = gpu.create_raster_state(false, true);
	no_cull_raster = gpu.create_raster_state(false, false);
	wireframe_raster = gpu.create_raster_state(true, true);

	enabled_depth = gpu.create_depth_state(true);
	disabled_depth = gpu.create_depth_state(false);

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

	enabled_blend = gpu.create_blend_state(true);
	disabled_blend = gpu.create_blend_state(false);

	const std::vector<dx::LayoutDescriptor> chunk_input_layout = {
		{ "KL_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "KL_Texture", 0, DXGI_FORMAT_R8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "KL_Ambient", 0, DXGI_FORMAT_R8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "KL_Block", 0, DXGI_FORMAT_R8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	
	draw_sky_shaders = gpu.create_shaders(kl::read_file("shaders/draw_sky.hlsl"));
	draw_hit_block_shaders = gpu.create_shaders(kl::read_file("shaders/draw_hit_block.hlsl"));
	raster_shadow_shaders = gpu.create_shaders(kl::read_file("shaders/raster_shadows.hlsl"), chunk_input_layout);
	raster_chunk_shaders = gpu.create_shaders(kl::read_file("shaders/raster_chunks.hlsl"), chunk_input_layout);
	tracing_world_shaders = gpu.create_shaders(kl::read_file("shaders/tracing_world.hlsl"));

	sky_mesh = gpu.create_cube_mesh(1.0f);
	hit_block_mesh = gpu.create_vertex_buffer({
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
	});
	tracing_mesh = gpu.create_screen_mesh();

	atlas_texture = gpu.create_texture(kl::Image("textures/blocks.png"));
	atlas_shader_view = gpu.create_shader_view(atlas_texture, nullptr);

	window.on_resize.emplace_back([this](int2 size)
	{
		if (size.x > 0 && size.y > 0) {
			this->game.world.system.gpu.resize_internal(size);
			this->game.world.system.gpu.set_viewport_size(size);
			this->game.player.camera.update_aspect_ratio(size);
		}
	});
	window.set_fullscreen(true);
	gpu.set_fullscreen(true);
}

void Renderer::render()
{
	if (game.render_mode == RenderMode::TRACING) {
		render_tracing();
	}
	else {
		render_raster();
	}
}

void Renderer::render_raster()
{
	draw_sky();
	raster_shadows();
	raster_chunks();
	draw_hit_block();
}

void Renderer::render_tracing()
{
	draw_sky();
	tracing_world();
	draw_hit_block();
}

void Renderer::draw_sky()
{
	auto& gpu = game.world.system.gpu;

	gpu.bind_raster_state(no_cull_raster);
	gpu.bind_depth_state(disabled_depth);

	struct alignas(16) CB
	{
		mat4 VP;
		flt3 SUN_DIRECTION;
	} cb = {};

	cb.VP = game.player.camera.matrix();
	cb.SUN_DIRECTION = game.environment.sun.direction();
	draw_sky_shaders.upload(cb);

	gpu.bind_shaders(draw_sky_shaders);
	gpu.draw(sky_mesh);
}

void Renderer::draw_hit_block()
{
	auto& gpu = game.world.system.gpu;

	auto opt_payload = game.hit_block;
	if (!opt_payload)
		return;

	HitPayload& payload = opt_payload.value();
	BlockPosition block_pos = game.world.get_block_world(payload.chunk_ind, payload.block_ind);

	gpu.bind_raster_state(cull_raster);
	gpu.bind_depth_state(enabled_depth);

	struct alignas(16) CB
	{
		mat4 WVP;
	} cb = {};

	cb.WVP = game.player.camera.matrix()
		* mat4::translation(block_pos.to_flt3() - flt3{ 0.001f })
		* mat4::scaling(flt3{ 1.002f });
	draw_hit_block_shaders.upload(cb);

	gpu.bind_shaders(draw_hit_block_shaders);
	gpu.draw(hit_block_mesh, D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
}

void Renderer::raster_shadows()
{
	auto& gpu = game.world.system.gpu;

	gpu.bind_raster_state(shadow_raster);
	gpu.bind_depth_state(enabled_depth);

	gpu.bind_sampler_state_for_pixel_shader(atlas_sampler, 0);
	gpu.bind_shader_view_for_pixel_shader(atlas_shader_view, 0);

	gpu.set_viewport_size(int2{ game.environment.sun.resolution() });
	gpu.bind_target_depth_views({}, game.environment.sun.depth_view(0));
	gpu.clear_depth_view(game.environment.sun.depth_view(0));

	struct alignas(16) CB
	{
		mat4 VP;
		flt3 CAMERA_ORIGIN;
		float ELAPSED_TIME;
		float RENDER_DISTANCE;
	} cb = {};

	cb.VP = game.environment.sun.matrix(inv_shadow_cam());
	cb.CAMERA_ORIGIN = game.player.camera.position;
	cb.ELAPSED_TIME = game.world.system.timer.elapsed();
	cb.RENDER_DISTANCE = (float) game.world.render_distance();
	raster_shadow_shaders.upload(cb);

	mat4 inv_sun_mat = kl::inverse(cb.VP);
	flt4 sun_pos = inv_sun_mat * flt4(0.0f, 0.0f, -1.0f, 1.0f);
	sun_pos /= sun_pos.w;

	plane sun_plane;
	sun_plane.position = sun_pos.xyz();
	sun_plane.set_normal(game.environment.sun.direction());

	gpu.bind_shaders(raster_shadow_shaders);
	for (int i = 0; i < game.world.chunk_count(); i++) {
		if (game.world.chunk_visible(sun_plane, i)) {
			gpu.draw(game.world.get_chunk(i).buffer, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, sizeof(Vertex));
		}
	}

	gpu.bind_internal_views();
	gpu.set_viewport_size(game.world.system.window.size());
}

void Renderer::raster_chunks()
{
	auto& gpu = game.world.system.gpu;

	gpu.bind_raster_state(game.world.system.wireframe ? wireframe_raster : cull_raster);
	gpu.bind_depth_state(enabled_depth);

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
	} cb = {};

	cb.VP = game.player.camera.matrix();
	cb.SUN_VP = game.environment.sun.matrix(inv_shadow_cam());
	cb.CAMERA_POSITION = game.player.camera.position;
	cb.ELAPSED_TIME = game.world.system.timer.elapsed();
	cb.SUN_DIRECTION = game.environment.sun.direction();
	cb.RENDER_DISTANCE = (float) game.world.render_distance();
	cb.SHADOW_TEXEL_SIZE = flt2{ 1.0f / game.environment.sun.resolution() };
	raster_chunk_shaders.upload(cb);

	plane camera_plane;
	camera_plane.position = game.player.camera.position;
	camera_plane.set_normal(game.player.camera.forward());

	gpu.bind_shaders(raster_chunk_shaders);
	for (int i = 0; i < game.world.chunk_count(); i++) {
		if (game.world.chunk_visible(camera_plane, i)) {
			gpu.draw(game.world.get_chunk(i).buffer, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, sizeof(Vertex));
		}
	}

	gpu.unbind_shader_view_for_pixel_shader(1);
	gpu.unbind_shader_view_for_pixel_shader(0);
}

void Renderer::tracing_world()
{
	auto& gpu = game.world.system.gpu;

	gpu.bind_raster_state(no_cull_raster);
	gpu.bind_depth_state(disabled_depth);

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

	cb.INV_CAM = kl::inverse(game.player.camera.matrix());
	cb.CAMERA_POSITION = game.player.camera.position;
	cb.RENDER_DISTANCE = (float) game.world.render_distance();
	cb.SUN_DIRECTION = game.environment.sun.direction();
	tracing_world_shaders.upload(cb);

	gpu.bind_shaders(tracing_world_shaders);
	gpu.draw(tracing_mesh);

	gpu.unbind_shader_view_for_pixel_shader(1);
	gpu.unbind_shader_view_for_pixel_shader(0);
}

mat4 Renderer::inv_shadow_cam() const
{
	kl::Camera camera = game.player.camera;
	camera.near_plane = 0.01f;
	camera.far_plane = flt2(CHUNK_WIDTH).length();
	return kl::inverse(camera.matrix());
}
