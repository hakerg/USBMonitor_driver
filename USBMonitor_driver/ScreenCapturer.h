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

	void captureFunc(void(*onScreenCaptured)(void*), void* data);
	void lossDataTo16Bit();

public:
	ScreenCapturer(Bitmap& Target, float MaxFps, void(*onScreenCaptured)(void*) = NULL, void* onScreenCapturedData = NULL);
	~ScreenCapturer();
	static BYTE dither2(BYTE value, int posX, int posY);
	static BYTE dither3(BYTE value, int posX, int posY);
};