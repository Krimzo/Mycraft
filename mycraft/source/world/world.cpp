#include "world/world.h"


World::World(System& system, int render_distance)
	: system(system)
{
	set_render_distance(render_distance);
}

int World::render_distance() const
{
	return m_render_distance;
}

void World::set_render_distance(int render_distance)
{
	m_render_distance = render_distance;
	m_chunks.resize(chunk_count());
	regenerate_all();
}

flt3 World::world_center() const
{
	return m_world_center;
}

void World::set_world_center(flt3 world_center)
{
	ChunkPosition old_chunk_pos = center_chunk_pos();
	m_world_center = world_center;
	ChunkPosition new_chunk_pos = center_chunk_pos();
	if (new_chunk_pos != old_chunk_pos) {
		ChunkIndex index_delta = (new_chunk_pos - old_chunk_pos).to_index();
		regenerate_with_swap(index_delta);
	}
}

int World::width_chunks() const
{
	return m_render_distance * 2 + 1;
}

int World::chunk_count() const
{
	return width_chunks() * width_chunks();
}

Chunk& World::get_chunk(int index)
{
	return m_chunks[index];
}

Chunk& World::get_chunk(ChunkIndex chunk_ind)
{
	return get_chunk(chunk_ind.to_int(width_chunks()));
}

BlockPosition World::get_block_world(ChunkIndex chunk_ind, BlockIndex block_ind) const
{
	return BlockPosition::from_flt3(first_chunk_pos().to_flt3()) + BlockPosition::from_index(ChunkPosition::from_index(chunk_ind), block_ind);
}

Block* World::get_world_block(const BlockPosition& block_pos)
{
	ChunkPosition chunk_pos = ChunkPosition::from_flt3(block_pos.to_flt3());
	ChunkIndex chunk_ind = (chunk_pos - first_chunk_pos()).to_index();
	if (!chunk_ind.is_valid(width_chunks()))
		return nullptr;

	Chunk& chunk = get_chunk(chunk_ind);
	BlockIndex block_ind = block_pos.to_index(chunk_pos);
	return chunk.get_block(block_ind);
}

Chunk& World::center_chunk()
{
	return get_chunk(ChunkIndex{ m_render_distance, m_render_distance });
}

Chunk& World::first_chunk()
{
	return get_chunk(ChunkIndex{ 0, 0 });
}

ChunkPosition World::center_chunk_pos() const
{
	return ChunkPosition::from_flt3(m_world_center);
}

ChunkPosition World::first_chunk_pos() const
{
	const flt3 render_dist_blocks{ float(m_render_distance * CHUNK_WIDTH) };
	return ChunkPosition::from_flt3(m_world_center - render_dist_blocks);
}

void World::upload_save(int index)
{
	ChunkIndex chunk_ind = ChunkIndex::from_int(index, width_chunks());
	ChunkPosition chunk_pos = first_chunk_pos() + ChunkPosition::from_index(chunk_ind);
	Chunk& chunk = get_chunk(index);
	chunk.upload(chunk_pos, system.gpu, get_block_test());
	generator.save_chunk(chunk_pos, chunk);
}

void World::upload_save(ChunkIndex chunk_ind)
{
	upload_save(chunk_ind.to_int(width_chunks()));
}

void World::upload_tracing()
{
	std::vector<Block> blocks;
	blocks.reserve(std::size(m_chunks.front().blocks) * m_chunks.size());
	for (auto& chunk : m_chunks) {
		blocks.insert(blocks.end(), chunk.blocks, chunk.blocks + std::size(chunk.blocks));
	}

	dx::BufferDescriptor descriptor{};
	descriptor.ByteWidth = (UINT) blocks.size();
	descriptor.Usage = D3D11_USAGE_IMMUTABLE;
	descriptor.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	dx::SubresourceDescriptor subresource_data{};
	subresource_data.pSysMem = blocks.data();
	dx::Buffer buffer = system.gpu.create_buffer(&descriptor, &subresource_data);

	dx::ShaderViewDescriptor view_descriptor{};
	view_descriptor.Format = DXGI_FORMAT_R8_UINT;
	view_descriptor.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	view_descriptor.Buffer.FirstElement = 0;
	view_descriptor.Buffer.NumElements = (UINT) blocks.size();
	m_tracing_view = system.gpu.create_shader_view(buffer, &view_descriptor);
}

dx::ShaderView World::get_tracing_view() const
{
	return m_tracing_view;
}

std::optional<HitPayload> World::cast_ray(const ray& ray, float reach)
{
	struct HitChunk
	{
		ChunkIndex chunk_ind;
		ChunkPosition chunk_pos;
		Chunk& chunk;
	};
	
	std::vector<HitChunk> hit_chunks;
	for (int i = 0; i < (int) m_chunks.size(); i++) {
		ChunkIndex chunk_ind = ChunkIndex::from_int(i, width_chunks());
		ChunkPosition chunk_pos = first_chunk_pos() + ChunkPosition::from_index(chunk_ind);

		aabb box;
		box.size = CHUNK_HALF;
		box.position = chunk_pos.to_flt3() + box.size;

		if (ray.intersect_aabb(box, nullptr)) {
			hit_chunks.emplace_back(chunk_ind, chunk_pos, get_chunk(i));
		}
	}
	
	float min_hit_dist = reach;
	std::optional<HitPayload> result;
	for (auto& hit_chunk : hit_chunks) {
		for (int i = 0; i < (int) std::size(hit_chunk.chunk.blocks); i++) {
			BlockIndex block_ind = BlockIndex::from_int(i);
			Block& block = *hit_chunk.chunk.get_block(block_ind);
			if (is_block_gas(block))
				continue;
	
			aabb box;
			box.size = flt3{ 0.5f };
			box.position = hit_chunk.chunk_pos.to_flt3() + block_ind.to_flt3() + box.size;
	
			flt3 inters;
			if (ray.intersect_aabb(box, &inters)) {
				float hit_distance = (inters - ray.origin).length();
				if (hit_distance < min_hit_dist) {
					min_hit_dist = hit_distance;

					HitPayload payload{};
					payload.chunk_ind = hit_chunk.chunk_ind;
					payload.block_ind = block_ind;
					payload.normal = kl::normalize(inters - box.position);
					result = payload;
				}
			}
		}
	}
	return result;
}

void World::adjust_by_normal(HitPayload& payload) const
{
	static constexpr BlockPosition adjacents[6] = {
		{ 1, 0, 0 }, { -1,  0,  0 },
		{ 0, 1, 0 }, {  0, -1,  0 },
		{ 0, 0, 1 }, {  0,  0, -1 },
	};

	BlockPosition dir;
	float max_dot = -std::numeric_limits<float>::infinity();
	for (auto& adj : adjacents) {
		float dot_val = kl::dot(adj.to_flt3(), payload.normal);
		if (dot_val > max_dot) {
			max_dot = dot_val;
			dir = adj;
		}
	}

	BlockPosition block_pos = get_block_world(payload.chunk_ind, payload.block_ind) + dir;
	ChunkPosition chunk_pos = ChunkPosition::from_flt3(block_pos.to_flt3());
	payload.chunk_ind = (chunk_pos - first_chunk_pos()).to_index();
	payload.block_ind = block_pos.to_index(chunk_pos);
}

bool World::chunk_visible(const plane& plane, int i) const
{
	ChunkIndex chunk_ind = ChunkIndex::from_int(i, width_chunks());
	ChunkPosition chunk_pos = first_chunk_pos() + ChunkPosition::from_index(chunk_ind);
	flt3 extremes[2] = {
		chunk_pos.to_flt3(),
		chunk_pos.to_flt3() + CHUNK_SIZE,
	};
	for (int x = 0; x < 2; x++) {
		for (int y = 0; y < 2; y++) {
			for (int z = 0; z < 2; z++) {
				flt3 pos = { extremes[x][0], extremes[y][1], extremes[z][2] };
				if (plane.in_front(pos))
					return true;
			}
		}
	}
	return false;
}

void World::regenerate_all()
{
	const auto get_chunk_pos = [this](int i)
	{
		ChunkIndex chunk_ind = ChunkIndex::from_int(i, width_chunks());
		return first_chunk_pos() + ChunkPosition::from_index(chunk_ind);
	};
	kl::async_for(0, chunk_count(), [&](int i)
	{
		generator.generate_chunk_cached(get_chunk_pos(i), get_chunk(i));
	});
	kl::async_for(0, chunk_count(), [&](int i)
	{
		get_chunk(i).upload(get_chunk_pos(i), system.gpu, get_block_test());
	});
	upload_tracing();
}

void World::regenerate_with_swap(const ChunkIndex index_delta)
{
	m_copy_chunks = m_chunks;
	kl::async_for(0, chunk_count(), [&](int i)
	{
		ChunkIndex chunk_ind = ChunkIndex::from_int(i, width_chunks());
		ChunkIndex src_chunk_ind = chunk_ind + index_delta;
		if (src_chunk_ind.is_valid(width_chunks())) {
			int src_i = src_chunk_ind.to_int(width_chunks());
			m_chunks[i] = m_copy_chunks[src_i];
		}
		else {
			ChunkPosition chunk_pos = first_chunk_pos() + ChunkPosition::from_index(chunk_ind);
			Chunk& chunk = get_chunk(i);
			generator.generate_chunk_cached(chunk_pos, chunk);
			chunk.upload(chunk_pos, system.gpu, get_block_test());
		}
	});
	upload_tracing();
}

BlockTest World::get_block_test()
{
	return [this](const BlockPosition& block_pos) -> bool
	{
		Block* block = get_world_block(block_pos);
		return block && is_block_solid(*block);
	};
}
