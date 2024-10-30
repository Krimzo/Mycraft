#include "world/block.h"


static constexpr flt2 DEFINED_TEXTURES[4] = {
	{ 0.0f, 1.0f },
	{ 0.0f, 0.0f },
	{ 1.0f, 0.0f },
	{ 1.0f, 1.0f },
};

static constexpr byte find_closest_texture(const flt2& texture)
{
	float dist = std::numeric_limits<float>::infinity();
	byte index = 0;
	for (byte i = 0; i < (byte) std::size(DEFINED_TEXTURES); i++) {
		float d = (texture - DEFINED_TEXTURES[i]).length();
		if (d < dist) {
			dist = d;
			index = i;
		}
	}
	return index;
}

static constexpr Vertex convert_vertex(const vertex& vertex)
{
	Vertex result;
	result.position = vertex.position;
	result.texture = find_closest_texture(vertex.uv);
	return result;
}

static constexpr Triangle convert_triangle(const triangle& triangle)
{
	Triangle result;
	result.vertices[0] = convert_vertex(triangle.a);
	result.vertices[1] = convert_vertex(triangle.b);
	result.vertices[2] = convert_vertex(triangle.c);
	return result;
}

static const std::vector<Quad> BLOCK_QUADS = []() -> std::vector<Quad>
{
	std::vector triangles = kl::GPU::generate_cube_mesh(1.0f);
	for (auto& triangle : triangles) {
		triangle.a.position += flt3{ 0.5f };
		triangle.b.position += flt3{ 0.5f };
		triangle.c.position += flt3{ 0.5f };
	}
	std::vector<Quad> quads;
	quads.resize(triangles.size() / 2);
	for (int i = 0; i < (int) quads.size(); i++) {
		quads[i].triangles[0] = convert_triangle(triangles[i + 0ull]);
		quads[i].triangles[1] = convert_triangle(triangles[i + 6ull]);
	}
	return quads;
}();

static const std::vector<Quad> PETTLE_QUADS = []() -> std::vector<Quad>
{
	const float near_offset = (sqrt(2.0f) - 1.0f) / (2.0f * sqrt(2.0f));
	const float far_offset = 1.0f - near_offset;

	triangle triangle1;
	triangle1.a.position = { near_offset, 0.0f, near_offset }; 
	triangle1.b.position = { near_offset, 1.0f, near_offset }; 
	triangle1.c.position = {  far_offset, 1.0f,  far_offset }; 
	triangle1.a.uv = { 0.0f, 1.0f };
	triangle1.b.uv = { 0.0f, 0.0f };
	triangle1.c.uv = { 1.0f, 0.0f };

	triangle triangle2;
	triangle2.a.position = { near_offset, 0.0f, near_offset }; 
	triangle2.b.position = {  far_offset, 1.0f,  far_offset }; 
	triangle2.c.position = {  far_offset, 0.0f,  far_offset }; 
	triangle2.a.uv = { 0.0f, 1.0f };
	triangle2.b.uv = { 1.0f, 0.0f };
	triangle2.c.uv = { 1.0f, 1.0f };

	std::vector<Quad> quads;
	Quad quad;

	quad.triangles[0] = convert_triangle(triangle1);
	quad.triangles[1] = convert_triangle(triangle2);
	quads.push_back(quad);

	triangle1.a.z = 1.0f - triangle1.a.z;
	triangle1.b.z = 1.0f - triangle1.b.z;
	triangle1.c.z = 1.0f - triangle1.c.z;
	triangle2.a.z = 1.0f - triangle2.a.z;
	triangle2.b.z = 1.0f - triangle2.b.z;
	triangle2.c.z = 1.0f - triangle2.c.z;
	quad.triangles[0] = convert_triangle(triangle1);
	quad.triangles[1] = convert_triangle(triangle2);
	quads.push_back(quad);

	std::swap(triangle1.a, triangle1.c);
	std::swap(triangle2.a, triangle2.c);

	quad.triangles[0] = convert_triangle(triangle1);
	quad.triangles[1] = convert_triangle(triangle2);
	quads.push_back(quad);

	triangle1.a.z = 1.0f - triangle1.a.z;
	triangle1.b.z = 1.0f - triangle1.b.z;
	triangle1.c.z = 1.0f - triangle1.c.z;
	triangle2.a.z = 1.0f - triangle2.a.z;
	triangle2.b.z = 1.0f - triangle2.b.z;
	triangle2.c.z = 1.0f - triangle2.c.z;
	quad.triangles[0] = convert_triangle(triangle1);
	quad.triangles[1] = convert_triangle(triangle2);
	quads.push_back(quad);

	return quads;
}();

static byte calc_ambient(const BlockPosition& block_pos, const BlockTest& block_test)
{
	static constexpr BlockPosition adjacents[6] = {
		{ 1, 0, 0 }, { -1,  0,  0 },
		{ 0, 1, 0 }, {  0, -1,  0 },
		{ 0, 0, 1 }, {  0,  0, -1 },
	};
	int counter = 0;
	for (auto& adj : adjacents) {
		if (!block_test(block_pos + adj))
			counter += 1;
	}
	float perc = float(counter) / std::size(adjacents);
	return byte(perc * 255);
}

void block_to_quads(const BlockPosition& block_pos, const Block block, std::vector<Quad>& out_quads, const BlockTest& block_test)
{
	for (Quad quad : BLOCK_QUADS) {
		triangle triangle;
		triangle.a.position = quad.triangles[0].vertices[0].position;
		triangle.b.position = quad.triangles[0].vertices[1].position;
		triangle.c.position = quad.triangles[0].vertices[2].position;
		flt3 triangle_normal = triangle.normal();

		BlockPosition forward_pos = BlockPosition::from_flt3(block_pos.to_flt3() + flt3{ 0.1f } + triangle_normal);
		if (block_test(forward_pos))
			continue;
		
		for (auto& triangle : quad.triangles) {
			for (auto& vertex : triangle.vertices) {
				vertex.position += block_pos.to_flt3();
				vertex.ambient = calc_ambient(BlockPosition::from_flt3(vertex.position + flt3{ 0.1f }), block_test);
				vertex.block = block;
			}
		}
		out_quads.push_back(quad);
	}
}

void pettle_to_quads(const BlockPosition& block_pos, const Block block, std::vector<Quad>& out_quads, const BlockTest& block_test)
{
	static constexpr BlockPosition adjacents[6] = {
		{ 1, 0, 0 }, { -1,  0,  0 },
		{ 0, 1, 0 }, {  0, -1,  0 },
		{ 0, 0, 1 }, {  0,  0, -1 },
	};

	bool is_visible = false;
	for (auto& adj : adjacents) {
		if (!block_test(block_pos + adj)) {
			is_visible = true;
			break;
		}
	}
	if (!is_visible)
		return;

	for (Quad quad : PETTLE_QUADS) {
		for (auto& triangle : quad.triangles) {
			for (auto& vertex : triangle.vertices) {
				vertex.position += block_pos.to_flt3();
				vertex.ambient = calc_ambient(BlockPosition::from_flt3(vertex.position + flt3{ 0.1f }), block_test);
				vertex.block = block;
			}
		}
		out_quads.push_back(quad);
	}
}
