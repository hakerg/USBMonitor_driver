#pragma once
#include "Bitmap.h"
#include <thread>
class ScreenCapturer
{
private:
	volatile bool running = true;
	Bitmap& target;
	std::thread capturingThread;
	clock_t frameTime;
	HDC hDesktopDC;
	HDC hMyDC;
	HBITMAP hBitmap;
	BITMAPINFO bi;
	RGBQUAD* screenData;

	void captureFunc();
	void lossDataTo16Bit();

public:
	ScreenCapturer(Bitmap& Target, float MaxFps);
	~ScreenCapturer();
	static BYTE dither(BYTE value, int lowBits, int posX, int posY);
};