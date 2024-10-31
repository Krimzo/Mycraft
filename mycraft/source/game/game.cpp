#include "game/game.h"


Environment::Environment(kl::GPU& gpu)
	: sun(gpu, 4096)
{}

std::optional<Block> Inventory::selected_item() const
{
	return toolbar[selected_slot];
}

flt3 Player::position() const
{
	return camera.position - flt3{ 0.0f, PLAYER_HEIGHT, 0.0f };
}

void Player::set_position(const flt3& position)
{
	camera.position = position + flt3{ 0.0f, PLAYER_HEIGHT, 0.0f };
}

Game::Game(World& world)
	: world(world)
	, environment(world.system.gpu)
{
	auto& window = world.system.window;
	window.mouse.set_position(window.frame_center());
	player.camera.position = { 5.0f, 5.0f, 5.0f };
	player.camera.set_forward({ 1.0f, -1.0f, 1.0f });

	int counter = 0;
	for (auto value : { Block::GRASS, Block::DIRT, Block::STONE, Block::COBBLE, Block::WOOD, Block::PLANKS, Block::COBWEB, Block::ROSE, Block::DANDELION }) {
		player.inventory.toolbar[counter] = value;
		counter += 1;
	}
}

void Game::update()
{
	float delta_t = world.system.timer.delta();
	handle_mouse_input();
	handle_keyboard_input();
	if (player.gamemode == GameMode::CREATIVE) {
		update_creative_movement(delta_t);
	}
	else {
		update_survival_movement(delta_t);
		update_velocity(delta_t);
		update_collisions(delta_t);
	}
	update_time();
	update_world();
}

float Game::current_daytime() const
{
	float time = environment.day_duration * environment.day_offset + world.system.timer.elapsed();
	return fmod(time, environment.day_duration);
}

float Game::max_view_distance() const
{
	static constexpr flt3 single_chunk{ CHUNK_WIDTH, 0.0f, CHUNK_WIDTH };
	flt3 center_point = world.center_chunk_pos().to_flt3() + single_chunk;
	flt3 first_point = world.first_chunk_pos().to_flt3();
	return (first_point - center_point).length();
}

void Game::handle_mouse_input()
{
	auto& window = world.system.window;

	player.inventory.selected_slot = abs(Inventory::HORIZONTAL_COUNT + player.inventory.selected_slot - window.mouse.scroll()) % Inventory::HORIZONTAL_COUNT;
	
	if (window.is_focused()) {
		int2 frame_center = window.frame_center();
		player.camera.rotate(window.mouse.position(), frame_center);
		window.mouse.set_position(frame_center);
		window.mouse.set_hidden(true);
	}

	if (window.mouse.left.pressed()) {
		std::optional<HitPayload> opt_payload = hit_block;
		if (!opt_payload)
			return;

		HitPayload& payload = opt_payload.value();
		Chunk& chunk = world.get_chunk(payload.chunk_ind);
		chunk.remove_block(payload.block_ind);
		world.upload_tracing();
		world.upload_save(payload.chunk_ind);
	}
	if (window.mouse.middle.pressed()) {
		std::optional<HitPayload> opt_payload = hit_block;
		if (!opt_payload)
			return;

		HitPayload& payload = opt_payload.value();
		Chunk& chunk = world.get_chunk(payload.chunk_ind);
		if (Block* block = chunk.get_block(payload.block_ind)) {
			for (int i = 0; i < Inventory::HORIZONTAL_COUNT; i++) {
				if (player.inventory.toolbar[i] == *block) {
					player.inventory.selected_slot = i;
					break;
				}
			}
		}
	}
	if (window.mouse.right.pressed()) {
		std::optional<HitPayload> opt_payload = hit_block;
		if (!opt_payload)
			return;

		HitPayload& payload = opt_payload.value();
		world.adjust_by_normal(payload);
		Chunk& chunk = world.get_chunk(payload.chunk_ind);
		chunk.place_block(payload.block_ind, player.inventory.selected_item().value_or(Block::AIR));
		world.upload_tracing();
		world.upload_save(payload.chunk_ind);
	}
}

void Game::handle_keyboard_input()
{
	auto& window = world.system.window;
	auto& gpu = world.system.gpu;

	if (window.keyboard.esc.pressed()) {
		window.close();
	}
	if (window.keyboard.f11.pressed()) {
		bool new_state = !window.in_fullscreen();
		window.set_fullscreen(new_state);
		gpu.set_fullscreen(new_state);
	}
	if (window.keyboard.comma.pressed()) {
		world.system.vsync = !world.system.vsync;
	}
	if (window.keyboard.period.pressed()) {
		world.system.wireframe = !world.system.wireframe;
	}
	if (window.keyboard.divide.pressed()) {
		render_mode = render_mode == RenderMode::RASTER ? RenderMode::TRACING : RenderMode::RASTER;
	}
	if (window.keyboard.plus.pressed()) {
		int ren_dist = world.render_distance() + 1;
		ren_dist = kl::clamp(ren_dist, 1, 16);
		world.set_render_distance(ren_dist);
	}
	if (window.keyboard.minus.pressed()) {
		int ren_dist = world.render_distance() - 1;
		ren_dist = kl::clamp(ren_dist, 1, 16);
		world.set_render_distance(ren_dist);
	}

	if (window.keyboard.one.pressed()) {
		player.inventory.selected_slot = 0;
	}
	if (window.keyboard.two.pressed()) {
		player.inventory.selected_slot = 1;
	}
	if (window.keyboard.three.pressed()) {
		player.inventory.selected_slot = 2;
	}
	if (window.keyboard.four.pressed()) {
		player.inventory.selected_slot = 3;
	}
	if (window.keyboard.five.pressed()) {
		player.inventory.selected_slot = 4;
	}
	if (window.keyboard.six.pressed()) {
		player.inventory.selected_slot = 5;
	}
	if (window.keyboard.seven.pressed()) {
		player.inventory.selected_slot = 6;
	}
	if (window.keyboard.eight.pressed()) {
		player.inventory.selected_slot = 7;
	}
	if (window.keyboard.nine.pressed()) {
		player.inventory.selected_slot = 8;
	}
}

void Game::update_time()
{
	static constexpr flt3 sun_start{ -0.666f, -0.666f, 0.333f };
	float time = current_daytime();
	flt3 direction;
	direction.x = sin(time * 2.0f * kl::pi() / environment.day_duration);
	direction.y = cos(time * 2.0f * kl::pi() / environment.day_duration);
	direction.z = -(1.0f / 2.5f) * sin(time * 2.0f * kl::pi() / environment.day_duration);
	environment.sun.set_direction(direction);
}

void Game::update_world()
{
	player.camera.far_plane = max_view_distance();
	world.set_world_center(player.camera.position);
	hit_block = world.cast_ray(player.camera.ray());
}

void Game::update_creative_movement(float delta_t)
{
	auto& window = world.system.window;
	if (window.keyboard.shift) {
		player.camera.speed = Player::CAMERA_SPEED * 2.0f;
	}
	else {
		player.camera.speed = Player::CAMERA_SPEED;
	}
	if (window.keyboard.w) {
		player.camera.move_forward(delta_t);
	}
	if (window.keyboard.s) {
		player.camera.move_back(delta_t);
	}
	if (window.keyboard.d) {
		player.camera.move_right(delta_t);
	}
	if (window.keyboard.a) {
		player.camera.move_left(delta_t);
	}
	if (window.keyboard.c) {
		player.camera.move_down(delta_t);
	}
	if (window.keyboard.space) {
		player.camera.move_up(delta_t);
	}
}

void Game::update_survival_movement(float delta_t)
{
	auto& window = world.system.window;

	const flt3 forward = player.camera.forward();
	const flt3 right = player.camera.right();
	float speed = player.walk_speed;
	if (window.keyboard.shift) {
		speed *= 2.0f;
	}

	if (window.keyboard.w) {
		player.camera.position.x += forward.x * speed * delta_t;
		player.camera.position.z += forward.z * speed * delta_t;
	}
	if (window.keyboard.s) {
		player.camera.position.x -= forward.x * speed * delta_t;
		player.camera.position.z -= forward.z * speed * delta_t;
	}
	if (window.keyboard.d) {
		player.camera.position.x += right.x * speed * delta_t;
		player.camera.position.z += right.z * speed * delta_t;
	}
	if (window.keyboard.a) {
		player.camera.position.x -= right.x * speed * delta_t;
		player.camera.position.z -= right.z * speed * delta_t;
	}
	if (window.keyboard.space.pressed()) {
		player.velocity.y = player.jump_speed;
	}
}

void Game::update_velocity(float delta_t)
{
	player.velocity.y += environment.gravity * delta_t;
	player.camera.position += player.velocity * delta_t;
}

void Game::update_collisions(float delta_t)
{
	flt3 player_pos = player.position();
	Block* block = world.get_world_block(BlockPosition::from_flt3(player_pos));
	if (block && is_block_solid(*block)) {
		player_pos.y = floor(player_pos.y + 1.0f);
		player.set_position(player_pos);
		player.velocity = {};
	}
}
