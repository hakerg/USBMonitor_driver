#include "stdafx.h"
#include "Bitmap.h"
#include <algorithm>


Bitmap::Bitmap(int Width, int Height) : width(Width), height(Height), size(Width * Height)
{
	data = new RGBQUAD[size];
}


Bitmap::~Bitmap()
{
	delete[] data;
}


RGBQUAD& Bitmap::at(const int& x, const int& y)
{
	return data[y * width + x];
}


void Bitmap::clear(RGBQUAD color)
{
	std::fill_n(data, size, color);
}


void Bitmap::fillRectangle(DrawingRegion& region)
{
	int y2 = region.y + region.size;
	for (int y = region.y; y < y2; y++)
	{
		std::fill_n(&at(region.x, y), region.size, region.color);
	}
}


void Bitmap::swapDataPointers(RGBQUAD*& newData)
{
	std::swap(data, newData);
}