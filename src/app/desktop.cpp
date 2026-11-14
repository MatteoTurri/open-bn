/*
 * Desktop methods and the singleton pointer.
 */

#include <vector>

#include "desktop.h"

Desktop *Desktop::desktop = 0;

Desktop::Desktop(int, int)
{
	unk_00 = 0;
	unk_04 = 600;
	desktop = this;
}

Desktop::~Desktop()
{
	desktop = 0;
}

Desktop *Desktop::GetDesktop()
{
	return desktop;
}

void Desktop::DispatchMouseMessage(int, int, int, int) const
{}

void Desktop::Draw() const
{}
