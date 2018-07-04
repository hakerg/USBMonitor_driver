#pragma once
#include "Bitmap.h"
#include "SerialPort.h"
#include "DrawingRegion.h"
#include <thread>
#include <map>


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
	DrawingRegionWithPriority bestRegion[2];
	clock_t millisUntilSlowMode;
	clock_t nextTouchCheck;
	volatile clock_t timeToStartSleep;
	const double sendTimeConstant = 0.14;
	const double timeFactor[2] = { 0.003, 0.042 };
	const double sendDelay = 0.15;
	double sendTimes[2][3][257];
	SerialPort& target;
	bool mouseClicked;
	INPUT clickInput, moveInput, releaseInput;
	int touchXMin = 0, touchXMax = 1, touchYMin = 0, touchYMax = 1;
	volatile bool portrait = false;
	const int rangeSizeXMultiplier[3] = { 1, 4, 1 };
	const int rangeSizeYMultiplier[3] = { 1, 1, 4 };

	void findingFunc();
	void sendingFunc();
	unsigned char reverse(unsigned char b);
	uint32_t reverse4(uint32_t i);
	void calculatePriority(DrawingRegionWithPriority& r)
	{
		r.priority = r.contrast / sendTimes[portrait][r.mode][r.size];
	}
	int calculateUnitContrast(const DrawingRegionWithPriority& region, const int& x, const int& y);
	void touchSupport();
	void changeRotation();

public:
	ScreenSender(Bitmap& sourceScreen, SerialPort& Target, clock_t MillisUntilSlowMode);
	~ScreenSender();
	const RGBQUAD* getArduinoPixelData() { return arduinoScreen.getData(); }
};

