#pragma once
#include "SerialPort.h"
class DrawingRegion
{
public:
	int x, y, size, mode;
	RGBQUAD color;

	DrawingRegion();
	~DrawingRegion();
};