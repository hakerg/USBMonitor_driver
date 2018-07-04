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
			ref.rgbRed = dither3(ref.rgbRed, x, y);
			ref.rgbGreen = dither2(ref.rgbGreen, x + 1, y);
			ref.rgbBlue = dither3(ref.rgbBlue, x, y + 1);
		}
	}
}


BYTE ScreenCapturer::dither2(BYTE value, int posX, int posY)
{
	static int ditherThreshold[2][2] = { 0, 2, 3, 1 };

	if ((value & 3) > ditherThreshold[posY & 1][posX & 1])
	{
		BYTE ret = ((value >> 2) + 1) << 2;
		if (ret == 0) return value >> 2 << 2;
		else return ret;
	}
	else
	{
		return value >> 2 << 2;
	}
}


BYTE ScreenCapturer::dither3(BYTE value, int posX, int posY)
{
	static int ditherThreshold[2][4] = { 0, 4, 1, 5, 6, 2, 7, 3 };

	if ((value & 7) > ditherThreshold[posY & 1][posX & 3])
	{
		BYTE ret = ((value >> 3) + 1) << 3;
		if (ret == 0) return value >> 3 << 3;
		else return ret;
	}
	else
	{
		return value >> 3 << 3;
	}
}