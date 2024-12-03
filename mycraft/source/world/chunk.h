#pragma once

#include "world/block.h"


struct Chunk
{
    Block blocks[CHUNK_WIDTH * CHUNK_WIDTH * CHUNK_HEIGHT] = {};
    dx::Buffer buffer;

    Block* get_block( BlockIndex const& block_ind );
    Block const* get_block( BlockIndex const& block_ind ) const;

    void place_block( BlockIndex const& block_ind, Block block );
    void remove_block( BlockIndex const& block_ind );

    void convert( ChunkPosition const& chunk_pos, std::vector<Quad>& out_quads, BlockTest const& block_test );
    void upload( ChunkPosition const& chunk_pos, kl::GPU& gpu, BlockTest const& block_test );
};

struct ChunkGenerator
{
    static inline std::string WORLD_PATH = "_world/";
    static inline std::string CHUNK_PATH = WORLD_PATH + "chunks/";

    ChunkGenerator();

    void generate_chunk( ChunkPosition const& chunk_pos, Chunk& out_chunk );
    void generate_chunk_cached( ChunkPosition const& chunk_pos, Chunk& out_chunk );
    Block generate_block( ChunkPosition const& chunk_pos, BlockIndex const& block_ind );

    bool load_chunk( ChunkPosition const& chunk_pos, Chunk& chunk );
    bool save_chunk( ChunkPosition const& chunk_pos, Chunk const& chunk );

    static std::string make_chunk_path( ChunkPosition const& position );
};
