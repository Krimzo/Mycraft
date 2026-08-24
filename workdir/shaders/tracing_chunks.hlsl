#define ATLAS_POS(x, y) (((x) << 4) | (y))

struct VS_OUT
{
    float2 ndc : VS_NDC;
    float4 position : SV_Position;
};

struct AABB
{
    float3 position;
    float3 size;
    
    float3 min_point()
    {
		return position - size;
    }

    float3 max_point()
    {
		return position + size;
    }
    
    bool contains(float3 pnt)
    {
        float3 minp = min_point();
		float3 maxp = max_point();
        return
            (pnt.x >= minp.x && pnt.x <= maxp.x) &&
            (pnt.y >= minp.y && pnt.y <= maxp.y) &&
            (pnt.z >= minp.z && pnt.z <= maxp.z);
    }
};

struct Ray
{
    float3 origin;
    float3 direction;
    
    bool intersect_aabb(AABB aabb, inout float3 out_inter)
    {
        if (aabb.contains(origin))
        {
            out_inter = origin;
            return true;
        }

        float3 inv_ray = 1.0f / direction;
        float3 t1 = (aabb.min_point() - origin) * inv_ray;
        float3 t2 = (aabb.max_point() - origin) * inv_ray;
        float3 t_min = min(t1, t2);
        float3 t_max = max(t1, t2);
        float t_min_max = max(max(t_min.x, t_min.y), t_min.z);
        float t_max_min = min(min(t_max.x, t_max.y), t_max.z);
        if (t_max_min < 0.0f || t_min_max > t_max_min)
            return false;

        out_inter = origin + direction * t_min_max;
        return true;
    }
};

static const float CHUNK_WIDTH = 16.0f;
static const float CHUNK_HEIGHT = 64.0f;
static const float3 CHUNK_SIZE = float3(CHUNK_WIDTH, CHUNK_HEIGHT, CHUNK_WIDTH);
static const float3 CHUNK_HALF = CHUNK_SIZE * 0.5f;
static const uint AIR = ATLAS_POS(0, 15);
static const uint GRASS = ATLAS_POS(3, 0);
static const uint DIRT = ATLAS_POS(2, 0);
static const uint STONE = ATLAS_POS(1, 0);
static const uint COBBLE = ATLAS_POS(0, 1);
static const uint WOOD = ATLAS_POS(4, 1);
static const uint PLANKS = ATLAS_POS(4, 0);
static const uint COBWEB = ATLAS_POS(11, 0);
static const uint ROSE = ATLAS_POS(12, 0);
static const uint DANDELION = ATLAS_POS(13, 0);
static const uint SAPLING = ATLAS_POS(15, 0);

float4x4 INV_CAM;
float3 CAMERA_POSITION;
float RENDER_DISTANCE;
float3 SUN_DIRECTION;

Buffer<uint> BLOCKS_TEXTURE : register(t0);
Texture2D<float4> ATLAS_TEXTURE : register(t1);

SamplerState ATLAS_SAMPLER : register(s0);

float sqr(float x)
{
    return x * x;
}

float3 to_chunk_pos(float3 pos)
{
    return floor(float3(pos.x, 0.0f, pos.z) / CHUNK_SIZE) * CHUNK_SIZE;
}

bool in_world_bounds(float3 pos)
{
    float3 bottom_left = to_chunk_pos(CAMERA_POSITION - RENDER_DISTANCE * CHUNK_SIZE);
    float3 top_right = to_chunk_pos(CAMERA_POSITION + RENDER_DISTANCE * CHUNK_SIZE) + CHUNK_SIZE;
    return all(pos >= bottom_left && pos <= top_right);
}

uint get_block(float3 pos)
{
    static const int chunk_blocks = CHUNK_WIDTH * CHUNK_WIDTH * CHUNK_HEIGHT;
    const int width_chunks = RENDER_DISTANCE * 2 + 1;
    
    int3 first_chunk = to_chunk_pos(CAMERA_POSITION - RENDER_DISTANCE * CHUNK_SIZE);
    int3 chunk_ind = to_chunk_pos(pos - first_chunk) / CHUNK_SIZE;    
    int3 block_ind = floor(pos - to_chunk_pos(pos));    
    
    int ind = (chunk_ind.x + chunk_ind.z * width_chunks) * chunk_blocks
        + (block_ind.x + (block_ind.z * CHUNK_WIDTH) + (block_ind.y * CHUNK_WIDTH * CHUNK_WIDTH));
    return BLOCKS_TEXTURE[ind];
}

bool trace(Ray ray, inout float3 out_pos, inout uint out_type)
{
    float3 block = floor(ray.origin);
    float3 t_delta = 1 / abs(ray.direction);
    float3 t_max =
    {
        (block.x + (ray.direction.x > 0 ? 1 : 0) - ray.origin.x) / ray.direction.x,
        (block.y + (ray.direction.y > 0 ? 1 : 0) - ray.origin.y) / ray.direction.y,
        (block.z + (ray.direction.z > 0 ? 1 : 0) - ray.origin.z) / ray.direction.z,
    };
    while (in_world_bounds(block + 0.5f))
    {
        uint block_type = get_block(block);
        if (block_type != AIR)
        {
            out_pos = floor(block);
            out_type = block_type;
            return true;
        }
        
        if (t_max.x < t_max.y)
        {
            if (t_max.x < t_max.z)
            {
                block.x += ray.direction.x > 0 ? 1 : -1;
                t_max.x += t_delta.x;
            }
            else
            {
                block.z += ray.direction.z > 0 ? 1 : -1;
                t_max.z += t_delta.z;
            }
        }
        else
        {
            if (t_max.y < t_max.z)
            {
                block.y += ray.direction.y > 0 ? 1 : -1;
                t_max.y += t_delta.y;
            }
            else
            {
                block.z += ray.direction.z > 0 ? 1 : -1;
                t_max.z += t_delta.z;
            }
        }
    }
    return false;
}

float3 closest_axis_vec(float3 dir)
{
    float3 abs_dir = abs(dir);
    if (abs_dir.x > abs_dir.y && abs_dir.x > abs_dir.z)
        return float3(sign(dir.x), 0.0f, 0.0f);
    if (abs_dir.y > abs_dir.z)
        return float3(0.0f, sign(dir.y), 0.0f);
    return float3(0.0f, 0.0f, sign(dir.z));
}

float3 get_color(Ray ray, float3 block_pos, uint block_type)
{
    static const float ambient = 0.1f;
    
    AABB aabb;
    aabb.position = block_pos + 0.5f;
    aabb.size = 0.5f;
    
    float3 inter = 0.0f;
    if (!ray.intersect_aabb(aabb, inter)) 
        return 0.0f;
    
#if 0
    float3 normal = normalize(inter - aabb.position);
    normal = normalize(round(normal));
#else
    float3 a = inter;
    float3 b = a + ddx(a);
    float3 c = a + ddy(a);
    float3 normal = normalize(cross(b - a, c - a));
#endif
    
    Ray shadow_ray;
    shadow_ray.origin = inter + normal * 0.001f;
    shadow_ray.direction = -SUN_DIRECTION;
    
    float3 light = 0.0f;
    float3 _ign1 = 0.0f;
    uint _ign2 = 0;
    if (trace(shadow_ray, _ign1, _ign2))
    {
        light = ambient;
    }
    else
    {
        float diffuse = saturate(dot(-SUN_DIRECTION, normal));
        light = max(diffuse, ambient);
    }
    
    float u = ((block_type >> 4) & 0xF) / 16.0f;
    float v = ((block_type >> 0) & 0xF) / 16.0f;
    
    float3 color = ATLAS_TEXTURE.Sample(ATLAS_SAMPLER, float2(u, v)).rgb;
    return color * light;
}

VS_OUT v_shader(float3 position : KL_Position)
{
    VS_OUT data;
    data.ndc = position.xy;
    data.position = float4(position, 1.0f);
    return data;
}

float4 p_shader(VS_OUT data) : SV_Target0
{
    float4 pixel_world = mul(float4(data.ndc, 1.0f, 1.0f), INV_CAM);
    pixel_world /= pixel_world.w;
    
    Ray ray;
    ray.origin = CAMERA_POSITION;
    ray.direction = normalize(pixel_world.xyz - ray.origin);
    
    float3 block_pos = 0.0f;
    uint block_type = 0;
    if (!trace(ray, block_pos, block_type))
        discard;
    
    float3 color = get_color(ray, block_pos, block_type);
    return float4(color, 1.0f);
}
