#include "render/shape.h"


void UIRectangle::generate( UIProduct& product ) const
{
    flt2 w_lb = { position.x, position.y };
    flt2 w_lt = { position.x, position.y + size.y };
    flt2 w_rt = { position.x + size.x, position.y + size.y };
    flt2 w_rb = { position.x + size.x, position.y };

    flt2 t_lb = atlas_uv( block, { 0.0f, 1.0f } );
    flt2 t_lt = atlas_uv( block, { 0.0f, 0.0f } );
    flt2 t_rt = atlas_uv( block, { 1.0f, 0.0f } );
    flt2 t_rb = atlas_uv( block, { 1.0f, 1.0f } );

    product.triangles.push_back( {
        { w_lb, color, t_lb, blend },
        { w_lt, color, t_lt, blend },
        { w_rt, color, t_rt, blend },
        } );
    product.triangles.push_back( {
        { w_lb, color, t_lb, blend },
        { w_rt, color, t_rt, blend },
        { w_rb, color, t_rb, blend },
        } );
}
