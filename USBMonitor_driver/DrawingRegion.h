#pragma once
#include "SerialPort.h"
class DrawingRegion
{
public:
	int x, y, size, mode, x2, y2, sizeX, sizeY;
	RGBQUAD color;

	DrawingRegion();
	~DrawingRegion();
};