#include "Mycraft.h"

Mycraft::Mycraft()
{
    kl::console::set_enabled(kl::IS_DEBUG);
}

Mycraft::~Mycraft()
{
    kl::console::set_enabled(true);
}

bool Mycraft::update()
{
    game.update();
    renderer.render();
    ui.update();
    return system.update();
}
