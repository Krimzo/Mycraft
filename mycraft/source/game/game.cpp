#include "game/game.h"


Environment::Environment( kl::GPU& gpu )
    : sun( gpu, 4096 )
{
}

std::optional<Block> Inventory::selected_item() const
{
    return toolbar[selected_slot];
}

flt3 Player::position() const
{
    return camera.position - flt3{ 0.0f, PLAYER_HEIGHT, 0.0f };
}

void Player::set_position( flt3 const& position )
{
    camera.position = position + flt3{ 0.0f, PLAYER_HEIGHT, 0.0f };
}

mat4 Portal::matrix() const
{
    static constexpr mat4 HALF_BOX_TRANSLATION = mat4::translation( flt3{ .5f } );
    return mat4::translation( position ) * mat4::rotation( rotation ) * mat4::scaling( scale ) * HALF_BOX_TRANSLATION;
}

Game::Game( World& world )
    : world( world )
    , environment( world.system.gpu )
{
    player.camera.position = { 5.0f, 5.0f, 5.0f };
    player.camera.set_forward( { 0.0f, 0.0f, 1.0f } );
    player.camera.field_of_view = 105.0f;
    player.prev_cam_pos = player.camera.position;

    int counter = 0;
    for ( auto value : { Block::GRASS, Block::DIRT, Block::STONE, Block::COBBLE, Block::WOOD, Block::PLANKS, Block::COBWEB, Block::ROSE, Block::DANDELION } )
    {
        player.inventory.toolbar[counter] = value;
        counter += 1;
    }

    Portal& portal_a = portals.emplace_back();
    Portal& portal_b = portals.emplace_back();

    portal_a.rotation = { 0.0f, -90.0f, 0.0f };
    portal_a.position = { 10.0f, 10.0f, 1.0f };
    portal_a.friend_portal = &portal_b;

    portal_b.rotation = { 0.0f, 180.0f, 0.0f };
    portal_b.position = { -9.0f, 3.0f, 23.0f };
    portal_b.friend_portal = &portal_a;
}

void Game::update()
{
    switch ( game_state )
    {
    case GameState::PLAYING: update_state_playing(); break;
    case GameState::MAIN_MENU: update_state_main_menu(); break;
    case GameState::EXIT: update_state_exit(); break;
    }
}

float Game::current_daytime() const
{
    float time = environment.day_duration * environment.day_offset + world.system.timer.elapsed();
    return fmod( time, environment.day_duration );
}

float Game::max_view_distance() const
{
    static constexpr flt3 single_chunk{ CHUNK_WIDTH, 0.0f, CHUNK_WIDTH };
    flt3 center_point = world.center_chunk_pos().to_flt3() + single_chunk;
    flt3 first_point = world.first_chunk_pos().to_flt3();
    return ( first_point - center_point ).length();
}

void Game::pause()
{
    auto& window = world.system.window;
    auto& mouse = window.mouse;
    auto& timer = world.system.timer;

    game_state = GameState::MAIN_MENU;
    timer.stop();
    mouse.set_hidden( false );
}

void Game::resume()
{
    auto& window = world.system.window;
    auto& mouse = window.mouse;
    auto& timer = world.system.timer;

    game_state = GameState::PLAYING;
    mouse.set_hidden( true );
    mouse.set_position( window.frame_center() );
    timer.start();
}

void Game::update_state_playing()
{
    float delta_t = world.system.timer.delta();
    state_playing_update_mouse_input();
    state_playing_update_keyboard_input();
    if ( player.gamemode == GameMode::CREATIVE )
    {
        state_playing_update_creative_movement( delta_t );
    }
    else
    {
        state_playing_update_survival_movement( delta_t );
        state_playing_update_survival_velocity( delta_t );
        state_playing_update_survival_collisions( delta_t );
    }
    state_playing_update_portals();
    state_playing_update_time();
    state_playing_update_world();
}

void Game::state_playing_update_mouse_input()
{
    auto& window = world.system.window;

    player.inventory.selected_slot = abs( Inventory::HORIZONTAL_COUNT + player.inventory.selected_slot - window.mouse.scroll() ) % Inventory::HORIZONTAL_COUNT;

    if ( window.focused() )
    {
        int2 frame_center = window.frame_center();
        player.camera.rotate( window.mouse.position(), frame_center );
        window.mouse.set_position( frame_center );
        window.mouse.set_hidden( true );
    }

    if ( window.mouse.left.pressed() )
    {
        std::optional<HitPayload> opt_payload = hit_block;
        if ( !opt_payload )
            return;

        auto& payload = opt_payload.value();
        auto& chunk = world.get_chunk( payload.chunk_ind );
        chunk.remove_block( payload.block_ind );
        world.upload_tracing();
        world.upload_save( payload.chunk_ind );
    }
    if ( window.mouse.middle.pressed() )
    {
        std::optional<HitPayload> opt_payload = hit_block;
        if ( !opt_payload )
            return;

        auto& payload = opt_payload.value();
        auto& chunk = world.get_chunk( payload.chunk_ind );
        if ( Block* block = chunk.get_block( payload.block_ind ) )
        {
            for ( int i = 0; i < Inventory::HORIZONTAL_COUNT; i++ )
            {
                if ( player.inventory.toolbar[i] == *block )
                {
                    player.inventory.selected_slot = i;
                    break;
                }
            }
        }
    }
    if ( window.mouse.right.pressed() )
    {
        std::optional<HitPayload> opt_payload = hit_block;
        if ( !opt_payload )
            return;

        auto& payload = opt_payload.value();
        world.adjust_by_normal( payload );

        auto& chunk = world.get_chunk( payload.chunk_ind );
        chunk.place_block( payload.block_ind, player.inventory.selected_item().value_or( Block::AIR ) );
        world.upload_tracing();
        world.upload_save( payload.chunk_ind );
    }
}

void Game::state_playing_update_keyboard_input()
{
    auto& window = world.system.window;
    auto& keyboard = window.keyboard;
    auto& mouse = window.mouse;
    auto& gpu = world.system.gpu;
    auto& timer = world.system.timer;

    if ( keyboard.esc.pressed() )
    {
        pause();
    }
    if ( keyboard.f11.pressed() )
    {
        gpu.set_fullscreen( !gpu.fullscreened() );
    }

    if ( keyboard.one.pressed() )
    {
        player.inventory.selected_slot = 0;
    }
    if ( keyboard.two.pressed() )
    {
        player.inventory.selected_slot = 1;
    }
    if ( keyboard.three.pressed() )
    {
        player.inventory.selected_slot = 2;
    }
    if ( keyboard.four.pressed() )
    {
        player.inventory.selected_slot = 3;
    }
    if ( keyboard.five.pressed() )
    {
        player.inventory.selected_slot = 4;
    }
    if ( keyboard.six.pressed() )
    {
        player.inventory.selected_slot = 5;
    }
    if ( keyboard.seven.pressed() )
    {
        player.inventory.selected_slot = 6;
    }
    if ( keyboard.eight.pressed() )
    {
        player.inventory.selected_slot = 7;
    }
    if ( keyboard.nine.pressed() )
    {
        player.inventory.selected_slot = 8;
    }
}

void Game::state_playing_update_creative_movement( float delta_t )
{
    auto& keyboard = world.system.window.keyboard;
    if ( keyboard.shift )
    {
        player.camera.speed = Player::CAMERA_SPEED * 2.0f;
    }
    else
    {
        player.camera.speed = Player::CAMERA_SPEED;
    }
    if ( keyboard.w )
    {
        player.camera.move_forward( delta_t );
    }
    if ( keyboard.s )
    {
        player.camera.move_back( delta_t );
    }
    if ( keyboard.d )
    {
        player.camera.move_right( delta_t );
    }
    if ( keyboard.a )
    {
        player.camera.move_left( delta_t );
    }
    if ( keyboard.q )
    {
        player.camera.move_down( delta_t );
    }
    if ( keyboard.e )
    {
        player.camera.move_up( delta_t );
    }
}

void Game::state_playing_update_survival_movement( float delta_t )
{
    auto& keyboard = world.system.window.keyboard;

    flt3 forward = player.camera.forward();
    flt3 right = player.camera.right();
    float speed = player.walk_speed;
    if ( keyboard.shift )
    {
        speed *= 2.0f;
    }

    if ( keyboard.w )
    {
        player.camera.position.x += forward.x * speed * delta_t;
        player.camera.position.z += forward.z * speed * delta_t;
    }
    if ( keyboard.s )
    {
        player.camera.position.x -= forward.x * speed * delta_t;
        player.camera.position.z -= forward.z * speed * delta_t;
    }
    if ( keyboard.d )
    {
        player.camera.position.x += right.x * speed * delta_t;
        player.camera.position.z += right.z * speed * delta_t;
    }
    if ( keyboard.a )
    {
        player.camera.position.x -= right.x * speed * delta_t;
        player.camera.position.z -= right.z * speed * delta_t;
    }
    if ( keyboard.space.pressed() )
    {
        player.velocity.y = player.jump_speed;
    }
}

void Game::state_playing_update_survival_velocity( float delta_t )
{
    player.velocity.y += environment.gravity * delta_t;
    player.camera.position += player.velocity * delta_t;
}

void Game::state_playing_update_survival_collisions( float delta_t )
{
    flt3 player_pos = player.position();
    Block* block = world.get_world_block( BlockPosition::from_flt3( player_pos ) );
    if ( block && is_block_solid( *block ) )
    {
        player_pos.y = floor( player_pos.y + 1.0f );
        player.set_position( player_pos );
        player.velocity = {};
    }
}

void Game::state_playing_update_portals()
{
    static constexpr aabb PORTAL_OBJ_AABB = { flt3{ 0.0f }, flt3{ 0.5f } };
    static constexpr float MOVE_BIAS = 0.01f;
    if ( player.camera.position == player.prev_cam_pos )
        return;
    for ( auto& portal : portals )
    {
        const mat4 inv_obj_mat = kl::inverse( portal.matrix() );
        const flt3 obj_player_pos = ( inv_obj_mat * flt4( player.camera.position, 1.0f ) ).xyz();
        const flt3 obj_prev_cam_pos = ( inv_obj_mat * flt4( player.prev_cam_pos, 1.0f ) ).xyz();
        ray obj_ray = { obj_prev_cam_pos, obj_player_pos - obj_prev_cam_pos };
        flt3 intersection;
        if ( !obj_ray.intersect_aabb( PORTAL_OBJ_AABB, &intersection ) )
            continue;
        if ( ( intersection - obj_prev_cam_pos ).length() > ( obj_player_pos - obj_prev_cam_pos ).length() )
            continue;
        obj_ray.origin += obj_ray.direction() * ( ( intersection - obj_ray.origin ).length() + kl::sqrt( 3.0f ) );
        obj_ray.set_direction( -obj_ray.direction() );
        if ( !obj_ray.intersect_aabb( PORTAL_OBJ_AABB, &intersection ) )
            continue;
        intersection -= obj_ray.direction() * MOVE_BIAS;
        const mat4 friend_mat = portal.friend_portal->matrix();
        const flt3 inv_player_forward = ( inv_obj_mat * flt4( player.camera.forward(), 0.0f ) ).xyz();
        player.camera.position = ( friend_mat * flt4( intersection, 1.0f ) ).xyz();
        player.camera.set_forward( ( friend_mat * flt4( inv_player_forward, 0.0f ) ).xyz() );
        break;
    }
    player.prev_cam_pos = player.camera.position;
}

void Game::state_playing_update_time()
{
    static constexpr flt3 sun_start{ -0.666f, -0.666f, 0.333f };
    float time = current_daytime();
    flt3 direction;
    direction.x = sin( time * 2.0f * kl::pi() / environment.day_duration );
    direction.y = cos( time * 2.0f * kl::pi() / environment.day_duration );
    direction.z = -( 1.0f / 2.5f ) * sin( time * 2.0f * kl::pi() / environment.day_duration );
    environment.sun.set_direction( direction );
}

void Game::state_playing_update_world()
{
    player.camera.far_plane = max_view_distance();
    world.set_world_center( player.camera.position );
    hit_block = world.cast_ray( player.camera.ray() );
}

void Game::update_state_main_menu()
{
    state_main_menu_update_keyboard_input();
}

void Game::state_main_menu_update_keyboard_input()
{
    auto& window = world.system.window;
    auto& keyboard = window.keyboard;
    auto& mouse = window.mouse;
    auto& gpu = world.system.gpu;
    auto& timer = world.system.timer;

    if ( keyboard.esc.pressed() )
    {
        resume();
    }
    if ( keyboard.f11.pressed() )
    {
        gpu.set_fullscreen( !gpu.fullscreened() );
    }
}

void Game::update_state_exit()
{
    world.system.window.close();
}
