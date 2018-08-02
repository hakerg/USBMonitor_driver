#pragma once
#include "Bitmap.h"
#include "SerialPort.h"
#include "DrawingRegion.h"
#include <thread>
#include <map>
#include <array>


class DrawingRegionWithPriority : public DrawingRegion
{
public:
	double priority;
	int contrast;
};

class ScreenSender
{
private:
	volatile bool running = true;
	std::thread findingThread, sendingThread;
	Bitmap arduinoScreen;
	Bitmap& screen;
	DrawingRegionWithPriority bestRegions[2];
	clock_t millisUntilSlowMode;
	clock_t nextTouchCheck;
	volatile clock_t timeToStartSleep;
	const double sendTimeConstant = 0.14;
	const double timeFactor[2] = { 0.003, 0.042 };
	const double sendDelay = 0.15;
	double sendTimes[2][5][257];
	SerialPort& target;
	bool mouseClicked;
	INPUT clickInput, moveInput, releaseInput;
	int touchXMin = 0, touchXMax = 1, touchYMin = 0, touchYMax = 1;
	volatile bool portrait = false;
	const int regionSizeXMultiplier[5] = { 1, 4, 1, 2, 2 };
	const int regionSizeYMultiplier[5] = { 1, 1, 4, 2, 2 };
	const std::array<const int, 1> modes = { 0 };

	void findingFunc();
	void sendingFunc();
	//unsigned char reverse(unsigned char b);
	//uint32_t reverse4(uint32_t i);
	void calculatePriority(DrawingRegionWithPriority& r)
	{
		//auto& sendTime = sendTimes[portrait][r.mode][r.size];
		//r.priority = r.contrast / sendTime * (50 - sendTime);
		r.priority = r.contrast / sendTimes[portrait][r.mode][r.size];
	}
	int calculateUnitContrast(const DrawingRegionWithPriority& region, const int& x, const int& y);
	void touchSupport();

public:
	int* contrastData;

	ScreenSender(Bitmap& sourceScreen, SerialPort& Target, clock_t MillisUntilSlowMode);
	~ScreenSender();
	const RGBQUAD* getArduinoPixelData() { return arduinoScreen.getData(); }
	static void screenCaptured(void* obj);
};

