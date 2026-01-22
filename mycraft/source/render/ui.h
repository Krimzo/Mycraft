#pragma once

#include "render/shape.h"
#include "render/renderer.h"


struct UIMesh
{
    kl::GPU& gpu;
    dx::Buffer buffer;
    UINT point_count = 0;

    UIMesh( kl::GPU& gpu );

    void upload( UIPoint const* data, UINT count );
};

struct UI
{
    Renderer const& renderer;

    UI( Renderer const& renderer );

    void update();

private:
    kl::Shaders m_shaders;
    UIProduct m_product;
    UIMesh m_mesh;

    void reload();
    void draw();

    void reload_state_playing();
    void reload_state_main_menu();
    void reload_state_exit();

    void reload_crosshair();
    void reload_toolbar();
    void reload_main_menu();

    template<int ID = 0>
    void ui_button( std::string_view const& text, flt2 center, flt2 scale, flt4 color, auto&& callback )
    {
        // logic
        auto& window = renderer.game.world.system.window;
        const flt2 mouse_ndc = window.mouse_ndc_ar();
        if ( mouse_ndc.in_bounds( center - scale * 0.5f, center + scale * 0.5f ) )
        {
            color.xyz() *= 1.5f;
            if ( window.mouse.left.pressed() )
                callback();
        }

        // draw
        UIRectangle background;
        background.center_align( center, scale );
        background.color = color;
        background.produce( m_product );
        m_product.append_text( text,
            { flt3{ 1.0f } - color.xyz(), color.w },
            scale.y * 0.7f,
            flt2{ center.x - scale.x * 0.5f, center.y + scale.y * 0.5f },
            scale,
            true );
    }
};
