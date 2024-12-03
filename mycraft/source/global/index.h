#pragma once

#include "global/defines.h"


inline constexpr int CHUNK_WIDTH = 16;
inline constexpr int CHUNK_HEIGHT = 64;
inline constexpr flt3 CHUNK_SIZE = flt3{ float( CHUNK_WIDTH ), float( CHUNK_HEIGHT ), float( CHUNK_WIDTH ) };
inline constexpr flt3 CHUNK_HALF = CHUNK_SIZE * 0.5f;

inline constexpr int ATLAS_SIZE = 16;
inline constexpr float ATLAS_DIVIDER = 1.0f / ATLAS_SIZE;
#define ATLAS_POS(x, y) (((x) << 4) | (y))

struct ChunkIndex
{
    int x, z;

    constexpr ChunkIndex()
        : ChunkIndex( 0, 0 )
    {}

    constexpr ChunkIndex( int x, int z )
        : x( x ), z( z )
    {}

    constexpr bool is_valid( int width_chunks ) const
    {
        return x >= 0 && z >= 0 && x < width_chunks && z < width_chunks;
    }

    constexpr int to_int( int width_chunks ) const
    {
        return x + (z * width_chunks);
    }

    static constexpr ChunkIndex from_int( int index, int width_chunks )
    {
        return { index % width_chunks, index / width_chunks };
    }
};

constexpr bool operator==( ChunkIndex const& first, ChunkIndex const& second )
{
    return first.x == second.x && first.z == second.z;
}

constexpr bool operator!=( ChunkIndex const& first, ChunkIndex const& second )
{
    return !(first == second);
}

constexpr ChunkIndex operator+( ChunkIndex const& first, ChunkIndex const& second )
{
    return { first.x + second.x, first.z + second.z };
}

constexpr ChunkIndex operator-( ChunkIndex const& first, ChunkIndex const& second )
{
    return { first.x - second.x, first.z - second.z };
}

struct ChunkPosition
{
    int x, z;

    constexpr ChunkPosition()
        : ChunkPosition( 0, 0 )
    {}

    constexpr ChunkPosition( int x, int z )
        : x( x ), z( z )
    {}

    constexpr ChunkIndex to_index() const
    {
        return { x / CHUNK_WIDTH, z / CHUNK_WIDTH };
    }

    static constexpr ChunkPosition from_index( ChunkIndex index )
    {
        return { index.x * CHUNK_WIDTH, index.z * CHUNK_WIDTH };
    }

    constexpr flt3 to_flt3() const
    {
        return { float( x ), 0.0f, float( z ) };
    }

    static constexpr ChunkPosition from_flt3( flt3 const& world )
    {
        return {
            (int) floor( world.x / CHUNK_WIDTH ) * CHUNK_WIDTH,
            (int) floor( world.z / CHUNK_WIDTH ) * CHUNK_WIDTH,
        };
    }
};

constexpr bool operator==( ChunkPosition const& first, ChunkPosition const& second )
{
    return first.x == second.x && first.z == second.z;
}

constexpr bool operator!=( ChunkPosition const& first, ChunkPosition const& second )
{
    return !(first == second);
}

constexpr ChunkPosition operator+( ChunkPosition const& first, ChunkPosition const& second )
{
    return { first.x + second.x, first.z + second.z };
}

constexpr ChunkPosition operator-( ChunkPosition const& first, ChunkPosition const& second )
{
    return { first.x - second.x, first.z - second.z };
}

struct BlockIndex
{
    int x, y, z;

    constexpr BlockIndex()
        : BlockIndex( 0, 0, 0 )
    {}

    constexpr BlockIndex( int x, int y, int z )
        : x( x ), y( y ), z( z )
    {}

    constexpr bool is_valid() const
    {
        return x >= 0 && y >= 0 && z >= 0 && x < CHUNK_WIDTH && y < CHUNK_HEIGHT && z < CHUNK_WIDTH;
    }

    constexpr int to_int() const
    {
        return x + (z * CHUNK_WIDTH) + (y * CHUNK_WIDTH * CHUNK_WIDTH);
    }

    static constexpr BlockIndex from_int( int index )
    {
        return {
            index % CHUNK_WIDTH,
            index / (CHUNK_WIDTH * CHUNK_WIDTH),
            (index / CHUNK_WIDTH) % CHUNK_WIDTH,
        };
    }

    constexpr flt3 to_flt3() const
    {
        return { float( x ), float( y ), float( z ) };
    }

    static constexpr BlockIndex from_flt3( flt3 const& vec )
    {
        return {
            (int) floor( vec.x ),
            (int) floor( vec.y ),
            (int) floor( vec.z ),
        };
    }
};

constexpr bool operator==( BlockIndex const& first, BlockIndex const& second )
{
    return first.x == second.x && first.y == second.y && first.z == second.z;
}

constexpr bool operator!=( BlockIndex const& first, BlockIndex const& second )
{
    return !(first == second);
}

constexpr BlockIndex operator+( BlockIndex const& first, BlockIndex const& second )
{
    return { first.x + second.x, first.y + second.y, first.z + second.z };
}

constexpr BlockIndex operator-( BlockIndex const& first, BlockIndex const& second )
{
    return { first.x - second.x, first.y - second.y, first.z - second.z };
}

struct BlockPosition
{
    int x, y, z;

    constexpr BlockPosition()
        : BlockPosition( 0, 0, 0 )
    {}

    constexpr BlockPosition( int x, int y, int z )
        : x( x ), y( y ), z( z )
    {}

    constexpr BlockIndex to_index( ChunkPosition chunk_pos ) const
    {
        return BlockIndex::from_flt3( to_flt3() - chunk_pos.to_flt3() );
    }

    static constexpr BlockPosition from_index( ChunkPosition chunk_pos, BlockIndex const& index )
    {
        return {
            chunk_pos.x + index.x,
            index.y,
            chunk_pos.z + index.z,
        };
    }

    constexpr flt3 to_flt3() const
    {
        return { float( x ), float( y ), float( z ) };
    }

    static constexpr BlockPosition from_flt3( flt3 const& vec )
    {
        return {
            (int) floor( vec.x ),
            (int) floor( vec.y ),
            (int) floor( vec.z ),
        };
    }
};

constexpr bool operator==( BlockPosition const& first, BlockPosition const& second )
{
    return first.x == second.x && first.y == second.y && first.z == second.z;
}

constexpr bool operator!=( BlockPosition const& first, BlockPosition const& second )
{
    return !(first == second);
}

constexpr BlockPosition operator+( BlockPosition const& first, BlockPosition const& second )
{
    return { first.x + second.x, first.y + second.y, first.z + second.z };
}

constexpr BlockPosition operator-( BlockPosition const& first, BlockPosition const& second )
{
    return { first.x - second.x, first.y - second.y, first.z - second.z };
}
