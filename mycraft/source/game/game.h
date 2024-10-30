#pragma once

#include "world/world.h"


enum RenderMode : uint8_t
{
	RASTER = 0,
	TRACING,
};

enum GameMode : uint8_t
{
	SURVIVAL = 0,
	CREATIVE,
};

struct Environment
{
	kl::DirectionalLight sun;
	float gravity = -10.0f;
	float day_duration = 24.0f * 60.0f;
	float day_offset = 0.4f;

	Environment(kl::GPU& gpu);
};

struct Inventory
{
	static constexpr int HORIZONTAL_COUNT = 9;
	static constexpr int VERTICAL_COUNT = 3;

	std::optional<Block> items[HORIZONTAL_COUNT * VERTICAL_COUNT] = {};
	std::optional<Block> toolbar[HORIZONTAL_COUNT] = {};
	int selected_slot = 0;

	std::optional<Block> selected_item() const;
};

struct Player
{
	static constexpr float PLAYER_HEIGHT = 1.65f;
	static constexpr float CAMERA_SPEED = 5.0f;

	kl::Camera camera;
	flt3 velocity;

	float jump_speed = 5.0f;
	float walk_speed = 2.5f;

	GameMode gamemode = GameMode::CREATIVE;
	Inventory inventory;

	flt3 position() const;
	void set_position(const flt3& position);
};

struct Game
{
	static constexpr float TRANSITION_DURATION = 2.0f;
	static constexpr float RETURN_DURATION = 2.0f;

	RenderMode render_mode = RenderMode::RASTER;

	World& world;
	Environment environment;
	Player player;

	std::optional<HitPayload> hit_block;

	Game(World& world);

	void update();

	float current_daytime() const;
	float max_view_distance() const;

private:
	void handle_mouse_input();
	void handle_keyboard_input();
	
	void update_time();
	void update_world();

	void update_creative_movement(float delta_t);
	void update_survival_movement(float delta_t);
	void update_velocity(float delta_t);
	void update_collisions(float delta_t);
};
