#include "system/system.h"


System::System( std::string_view const& title )
    : window( "Mycraft" )
    , gpu( window )
{}

bool System::update()
{
    timer.update();
    gpu.swap_buffers( vsync );
    gpu.clear_internal();
    return window.process();
}
