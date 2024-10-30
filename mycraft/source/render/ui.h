#pragma once

#include "render/shape.h"
#include "render/renderer.h"


struct UI
{
	Renderer& renderer;

	UI(Renderer& renderer);

	void update();
	void render();

private:
	kl::Shaders m_shaders;
	UIProduct m_product;
	dx::Buffer m_triangle_mesh;
	dx::Buffer m_line_mesh;
	dx::Buffer m_point_mesh;

	dx::Buffer create_mesh(const UIPoint* data, UINT count) const;

	void reload_shapes();
	void reload_meshes();

	void make_crosshair();
	void make_toolbar();
};
