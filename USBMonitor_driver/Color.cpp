#include "stdafx.h"
#include "Color.h"
#include <stdlib.h>


uint16_t getU16(RGBQUAD c)
{
	return (c.rgbBlue >> 3) + ((c.rgbGreen >> 2) << 5) + ((c.rgbRed >> 3) << 11);
}


int contrast(RGBQUAD c1, RGBQUAD c2)
{
	int result = abs(c1.rgbRed - c2.rgbRed) + abs(c1.rgbGreen - c2.rgbGreen) + abs(c1.rgbBlue - c2.rgbBlue);
	//return abs(c1.rgbRed - c2.rgbRed) + abs(c1.rgbGreen - c2.rgbGreen) + abs(c1.rgbBlue - c2.rgbBlue);
	return result * result;
}