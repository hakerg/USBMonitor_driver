#pragma once
#include "DrawingRegion.h"
class Bitmap
{
private:
	RGBQUAD* data;

public:
	const int width, height, size;

	Bitmap(int Width, int Height);
	~Bitmap();
	RGBQUAD& at(const int& x, const int& y);
	void clear(RGBQUAD color);
	void fillRectangle(DrawingRegion& region);
	RGBQUAD* getData() { return data; }
	void swapDataPointers(RGBQUAD*& newData);
};

