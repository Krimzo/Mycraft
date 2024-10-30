#pragma once

#include "world/block.h"


struct Chunk
{
	Block blocks[CHUNK_WIDTH * CHUNK_WIDTH * CHUNK_HEIGHT] = {};
	dx::Buffer buffer;

	Block* get_block(const BlockIndex& block_ind);
	const Block* get_block(const BlockIndex& block_ind) const;

	void place_block(const BlockIndex& block_ind, Block block);
	void remove_block(const BlockIndex& block_ind);

	void convert(const ChunkPosition& chunk_pos, std::vector<Quad>& out_quads, const BlockTest& block_test);
	void upload(const ChunkPosition& chunk_pos, kl::GPU& gpu, const BlockTest& block_test);
};

struct ChunkGenerator
{
	static inline const std::string WORLD_PATH = "_world/";
	static inline const std::string CHUNK_PATH = WORLD_PATH + "chunks/";

	ChunkGenerator();

	void generate_chunk(const ChunkPosition& chunk_pos, Chunk& out_chunk);
	void generate_chunk_cached(const ChunkPosition& chunk_pos, Chunk& out_chunk);
	Block generate_block(const ChunkPosition& chunk_pos, const BlockIndex& block_ind);

	bool load_chunk(const ChunkPosition& chunk_pos, Chunk& chunk);
	bool save_chunk(const ChunkPosition& chunk_pos, const Chunk& chunk);

	static std::string make_chunk_path(const ChunkPosition& position);
};
