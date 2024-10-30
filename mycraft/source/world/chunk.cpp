#include "world/chunk.h"


Block* Chunk::get_block(const BlockIndex& block_ind)
{
	if (!block_ind.is_valid())
		return nullptr;
	return &blocks[block_ind.to_int()];
}

const Block* Chunk::get_block(const BlockIndex& block_ind) const
{
	if (!block_ind.is_valid())
		return nullptr;
	return &blocks[block_ind.to_int()];
}

void Chunk::place_block(const BlockIndex& block_ind, Block block)
{
	if (Block* self_block = get_block(block_ind)) {
		*self_block = block;
	}
}

void Chunk::remove_block(const BlockIndex& block_ind)
{
	place_block(block_ind, Block::AIR);
}

void Chunk::convert(const ChunkPosition& chunk_pos, std::vector<Quad>& out_quads, const BlockTest& block_test)
{
	std::mutex lock;
	kl::async_for(0, (int) std::size(blocks), [&](int i)
	{
		auto& block = blocks[i];
		if (is_block_gas(block))
			return;

		BlockPosition block_pos = BlockPosition::from_index(chunk_pos, BlockIndex::from_int(i));
		std::vector<Quad> quads;
		quads.reserve(6);
		if (is_block_pettle(block)) {
			pettle_to_quads(block_pos, block, quads, block_test);
		}
		else {
			block_to_quads(block_pos, block, quads, block_test);
		}

		lock.lock();
		out_quads.insert(out_quads.end(), quads.begin(), quads.end());
		lock.unlock();
	});
}

void Chunk::upload(const ChunkPosition& chunk_pos, kl::GPU& gpu, const BlockTest& block_test)
{
	std::vector<Quad> quads;
	quads.reserve(std::size(blocks));
	convert(chunk_pos, quads, block_test);
	if (quads.empty()) {
		buffer = {};
		return;
	}
	const UINT byte_size = (UINT) quads.size() * sizeof(Quad);
	buffer = gpu.create_vertex_buffer(quads.data(), byte_size);
}

ChunkGenerator::ChunkGenerator()
{
	std::filesystem::create_directories(WORLD_PATH);
	std::filesystem::create_directories(CHUNK_PATH);
}

void ChunkGenerator::generate_chunk(const ChunkPosition& chunk_pos, Chunk& out_chunk)
{
	kl::async_for(0, (int) std::size(out_chunk.blocks), [&](int i)
	{
		const BlockIndex block_ind = BlockIndex::from_int(i);
		out_chunk.blocks[i] = generate_block(chunk_pos, block_ind);
	});
}

void ChunkGenerator::generate_chunk_cached(const ChunkPosition& chunk_pos, Chunk& out_chunk)
{
	if (load_chunk(chunk_pos, out_chunk))
		return;
	generate_chunk(chunk_pos, out_chunk);
	save_chunk(chunk_pos, out_chunk);
}

Block ChunkGenerator::generate_block(const ChunkPosition& chunk_pos, const BlockIndex& block_ind)
{
	switch (block_ind.y)
	{
	case 2: return Block::GRASS;
	case 1: return Block::DIRT;
	case 0: return Block::STONE;
	default: return Block::AIR;
	}
}

bool ChunkGenerator::load_chunk(const ChunkPosition& chunk_pos, Chunk& chunk)
{
	const std::string path = make_chunk_path(chunk_pos);

	kl::File file{ path, false };
	if (!file)
		return false;

	const size_t block_count = std::size(chunk.blocks);
	return file.read<Block>(chunk.blocks, block_count) == block_count;
}

bool ChunkGenerator::save_chunk(const ChunkPosition& chunk_pos, const Chunk& chunk)
{
	const std::string path = make_chunk_path(chunk_pos);

	kl::File file{ path, true };
	if (!file)
		return false;

	const size_t block_count = std::size(chunk.blocks);
	return file.write<Block>(chunk.blocks, block_count) == block_count;
}

std::string ChunkGenerator::make_chunk_path(const ChunkPosition& pos)
{
	return kl::format(CHUNK_PATH, pos.x, '_', pos.z, ".chunk");
}
