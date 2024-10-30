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
	ui.update();
	renderer.render();
	ui.render();
	return system.update();
}
