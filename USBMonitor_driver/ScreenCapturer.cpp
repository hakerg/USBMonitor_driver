#include "stdafx.h"
#include "ScreenCapturer.h"
#include <iostream>


ScreenCapturer::ScreenCapturer(Bitmap& Target, float MaxFps) : target(Target)
{
	frameTime = (clock_t)ceil(1000.0 / MaxFps);
	hDesktopDC = GetDC(NULL);
	hMyDC = CreateCompatibleDC(hDesktopDC);
	memset(&bi, 0, sizeof(&bi));
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biWidth = target.width;
	bi.bmiHeader.biHeight = -target.height;
	hBitmap = CreateCompatibleBitmap(hDesktopDC, target.width, target.height);
	SelectObject(hMyDC, hBitmap);
	SetStretchBltMode(hMyDC, HALFTONE);
	screenData = new RGBQUAD[target.size];
	capturingThread = std::thread(&ScreenCapturer::captureFunc, this);
}


ScreenCapturer::~ScreenCapturer()
{
	running = false;
	capturingThread.join();
	DeleteObject(hBitmap);
	ReleaseDC(NULL, hDesktopDC);
	DeleteDC(hMyDC);
	delete[] screenData;
}


void ScreenCapturer::captureFunc()
{
	while (running)
	{
		clock_t start = clock();

		StretchBlt(hMyDC, 0, 0, target.width, target.height, hDesktopDC, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SRCCOPY);
		GetDIBits(hMyDC, hBitmap, 0, target.height, screenData, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
		lossDataTo16Bit();
		target.swapDataPointers(screenData);

		clock_t finish = clock();
		clock_t workingTime = finish - start;
		if (workingTime < frameTime)
		{
			Sleep(frameTime - workingTime);
		}
	}
}


void ScreenCapturer::lossDataTo16Bit()
{
	for (register int x = 0; x < target.width; x++)
	{
		for (register int y = 0; y < target.height; y++)
		{
			RGBQUAD& ref = screenData[y * target.width + x];
			ref.rgbRed = dither(ref.rgbRed, 3, x, y);
			ref.rgbGreen = dither(ref.rgbGreen, 2, x + 1, y);
			ref.rgbBlue = dither(ref.rgbBlue, 3, x, y + 1);
		}
	}
}


BYTE ScreenCapturer::dither(BYTE value, int lowBits, int posX, int posY)
{
	static bool ditherThreshold[4][2][2] = { { 0, 0, 0, 0 },{ 1, 0, 0, 0 },{ 1, 0, 0, 1 },{ 1, 1, 0, 1 } };

	if (ditherThreshold[(value >> (lowBits - 2)) & 3][posX & 1][posY & 1])
	{
		BYTE ret = ((value >> lowBits) + 1) << lowBits;
		if (ret == 0) return value >> lowBits << lowBits;
		else return ret;
	}
	else
	{
		return value >> lowBits << lowBits;
	}
}