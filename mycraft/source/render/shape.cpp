#include "render/shape.h"


void UIProduct::clear()
{
    triangles.clear();
    lines.clear();
    points.clear();
}

void UIRectangle::produce( UIProduct& product ) const
{
    const flt2 w_tl = { position.x, position.y + size.y };
    const flt2 w_tr = { position.x + size.x, position.y + size.y };
    const flt2 w_bl = { position.x, position.y };
    const flt2 w_br = { position.x + size.x, position.y };

    const flt2 uv_tr = { uv_br.x, uv_tl.y };
    const flt2 uv_bl = { uv_tl.x, uv_br.y };

    product.triangles.push_back( {
        { layer, w_bl, color, uv_bl, tex_blend },
        { layer, w_tl, color, uv_tl, tex_blend },
        { layer, w_tr, color, uv_tr, tex_blend },
        } );
    product.triangles.push_back( {
        { layer, w_bl, color, uv_bl, tex_blend },
        { layer, w_tr, color, uv_tr, tex_blend },
        { layer, w_br, color, uv_br, tex_blend },
        } );
}

void UIRectangle::center_align( flt2 center, flt2 scale )
{
    position = center - scale * 0.5f;
    size = scale;
}
