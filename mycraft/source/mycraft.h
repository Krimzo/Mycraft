#pragma once

#include "system/system.h"
#include "world/world.h"
#include "game/game.h"
#include "render/renderer.h"
#include "render/ui.h"


struct Mycraft
{
    System system{ "Mycraft" };
    World world{ system, 12 };
    Game game{ world };
    Renderer renderer{ game };
    UI ui{ renderer };

    Mycraft();
    ~Mycraft();

    bool update();
};
