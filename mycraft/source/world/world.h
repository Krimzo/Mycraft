#pragma once

#include "system/system.h"
#include "world/chunk.h"


struct HitPayload
{
    ChunkIndex chunk_ind;
    BlockIndex block_ind;
    flt3 normal;
};

struct World
{
    System& system;
    ChunkGenerator generator{};

    World( System& system, int render_distance );

    int render_distance() const;
    void set_render_distance( int render_distance );

    flt3 world_center() const;
    void set_world_center( flt3 world_center );

    int width_chunks() const;
    int chunk_count() const;

    Chunk& get_chunk( int index );
    Chunk& get_chunk( ChunkIndex chunk_ind );

    BlockPosition get_block_world( ChunkIndex chunk_ind, BlockIndex block_ind ) const;
    Block* get_world_block( BlockPosition const& block_pos );

    Chunk& center_chunk();
    Chunk& first_chunk();

    ChunkPosition center_chunk_pos() const;
    ChunkPosition first_chunk_pos() const;

    void upload_save( int index );
    void upload_save( ChunkIndex chunk_ind );

    void upload_tracing();
    dx::ShaderView get_tracing_view() const;

    std::optional<HitPayload> cast_ray( ray const& ray, float reach = 5.0f );
    void adjust_by_normal( HitPayload& payload ) const;

    bool chunk_visible( plane const& plane, int i ) const;

private:
    int m_render_distance;
    flt3 m_world_center;
    std::vector<Chunk> m_chunks;
    std::vector<Chunk> m_copy_chunks;
    dx::ShaderView m_tracing_view;

    void regenerate_all();
    void regenerate_with_swap( ChunkIndex index_delta );

    BlockTest get_block_test();
};
