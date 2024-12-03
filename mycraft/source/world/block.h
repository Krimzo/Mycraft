#pragma once

#include "global/index.h"


using BlockTest = std::function<bool( BlockPosition const& )>;

enum Block : byte
{
    AIR = ATLAS_POS( 0, 15 ),
    GRASS = ATLAS_POS( 3, 0 ),
    DIRT = ATLAS_POS( 2, 0 ),
    STONE = ATLAS_POS( 1, 0 ),
    COBBLE = ATLAS_POS( 0, 1 ),
    WOOD = ATLAS_POS( 4, 1 ),
    PLANKS = ATLAS_POS( 4, 0 ),
    COBWEB = ATLAS_POS( 11, 0 ),
    ROSE = ATLAS_POS( 12, 0 ),
    DANDELION = ATLAS_POS( 13, 0 ),
    SAPLING = ATLAS_POS( 15, 0 ),
};

struct Vertex
{
    flt3 position;
    byte texture = 0;
    byte ambient = 0;
    Block block = Block::AIR;
};

struct Triangle
{
    Vertex vertices[3] = {};
};

struct Quad
{
    Triangle triangles[2] = {};
};

constexpr bool is_block_gas( Block block )
{
    return block == Block::AIR;
}

constexpr bool is_block_solid( Block block )
{
    switch ( block )
    {
    case Block::AIR:
    case Block::COBWEB:
    case Block::ROSE:
    case Block::SAPLING:
    case Block::DANDELION:
        return false;

    default:
        return true;
    }
}

constexpr bool is_block_pettle( Block block )
{
    switch ( block )
    {
    case Block::COBWEB:
    case Block::ROSE:
    case Block::SAPLING:
    case Block::DANDELION:
        return true;

    default:
        return false;
    }
}

constexpr int2 atlas_pos( Block block )
{
    return {
        (block >> 4) & 0x0F,
        (block >> 0) & 0x0F,
    };
}

constexpr flt2 atlas_uv( Block block, flt2 uv )
{
    return (uv + atlas_pos( block )) * ATLAS_DIVIDER;
}

void block_to_quads( BlockPosition const& block_pos, Block block, std::vector<Quad>& out_quads, BlockTest const& block_test );
void pettle_to_quads( BlockPosition const& block_pos, Block block, std::vector<Quad>& out_quads, BlockTest const& block_test );
