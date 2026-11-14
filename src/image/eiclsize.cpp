/*
 * EiclSize: setters and the aspect-ratio fit.
 */

#pragma implementation "eiclsize.h"
#include "eiclsize.h"

EiclSize::EiclSize(int w, int h)
{
	SetSize_(w, h);
}

EiclSize::EiclSize(const EiclSize *other)
{
	SetSize(*other);
}

void EiclSize::SetSize(int w, int h)
{
	SetSize_(w, h);
}

void EiclSize::SetSize(EiclSize sz)
{
	SetSize(sz.GetWidth(), sz.GetHeight());
}

void EiclSize::SetWidth(int w)
{
	SetSize_(w, GetHeight());
}

void EiclSize::SetHeight(int h)
{
	SetSize_(GetWidth(), h);
}

int EiclSize::GetWidth() const
{
	return w;
}

int EiclSize::GetHeight() const
{
	return h;
}

EiclSize EiclSize::operator+(EiclSize &other)
{
	return EiclSize(other.GetWidth() + GetWidth(), other.GetHeight() + GetHeight());
}

void EiclSize::SetSize_(int w, int h)
{
	this->w = w;
	this->h = h;
}

int EiclSize::FitTo(int w, int h)
{
	if ((double)w / (double)this->w > (double)h / (double)this->h) {
		this->w = this->w * h / this->h;
		this->h = this->h * h / this->h;
	} else {
		this->h = this->h * w / this->w;
		this->w = this->w * w / this->w;
	}

	if (this->w <= 0)
		this->w = 1;
	if (this->h <= 0)
		this->h = 1;

	return 1;
}

void EiclSize::FitTo(EiclSize sz)
{
	FitTo(sz.GetWidth(), sz.GetHeight());
}
