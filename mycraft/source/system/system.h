#pragma once

#include "klibrary.h"


struct System
{
    kl::Window window;
    kl::GPU gpu;
    kl::Timer timer;

    bool vsync = false;
    bool wireframe = false;

    System( std::string_view const& title );

    bool update();
};
