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
    void draw( D3D_PRIMITIVE_TOPOLOGY topology ) const;
};

struct UI
{
    Renderer const& renderer;

    UI( Renderer const& renderer );

    void update();
    void render();

private:
    kl::Shaders m_shaders;
    UIProduct m_product;
    UIMesh m_triangle_mesh;
    UIMesh m_line_mesh;
    UIMesh m_point_mesh;

    void reload_shapes();
    void reload_meshes();

    void make_crosshair();
    void make_toolbar();
};
