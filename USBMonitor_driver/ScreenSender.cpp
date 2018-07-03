#include "stdafx.h"
#include "ScreenSender.h"
#include "Color.h"
#include <iostream>
#include <algorithm>
#define _USE_MATH_DEFINES
#include <math.h>
#include <map>
#include <vector>


ScreenSender::ScreenSender(Bitmap& sourceScreen, SerialPort& Target, clock_t MillisUntilSlowMode) :
	screen(sourceScreen),
	target(Target),
	arduinoScreen(sourceScreen.width, sourceScreen.height),
	millisUntilSlowMode(MillisUntilSlowMode)
{
	timeToStartSleep = clock() + MillisUntilSlowMode;
	RGBQUAD clearColor;
	clearColor.rgbRed = 127;
	clearColor.rgbGreen = 127;
	clearColor.rgbBlue = 127;
	clearColor.rgbReserved = 0;
	arduinoScreen.clear(clearColor);

	for (int i = 1; i <= 256; i++)
	{
		sendTimes[0][i] = sendTimeConstant + i * i * timeFactor[0];
		sendTimes[1][i] = sendTimeConstant + i * i * timeFactor[1];
		sendTimes[2][i] = sendTimeConstant + i * timeFactor[1];
		sendTimes[3][i] = sendTimes[2][i];
	}

	/*sendTimes[0][1] = 0.143;
	sendTimes[0][2] = 0.143;
	sendTimes[0][4] = 0.154;
	sendTimes[0][8] = 0.3;
	sendTimes[0][16] = 0.8;
	sendTimes[0][32] = 2.7;
	sendTimes[0][64] = 11;

	sendTimes[1][1] = 0.14;
	sendTimes[1][2] = 0.26;
	sendTimes[1][4] = 0.75;
	sendTimes[1][8] = 2.77;
	sendTimes[1][16] = 10.8;
	sendTimes[1][32] = 42.8;
	sendTimes[1][64] = 171.2;*/

	mouseClicked = false;
	memset(&moveInput, 0, sizeof(INPUT));
	memset(&clickInput, 0, sizeof(INPUT));
	memset(&releaseInput, 0, sizeof(INPUT));
	moveInput.type = INPUT_MOUSE;
	moveInput.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
	clickInput.type = INPUT_MOUSE;
	clickInput.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
	releaseInput.type = INPUT_MOUSE;
	releaseInput.mi.dwFlags = MOUSEEVENTF_LEFTUP;
	nextTouchCheck = clock();

	findingThread = std::thread(&ScreenSender::findingFunc, this);
	sendingThread = std::thread(&ScreenSender::sendingFunc, this);
}


ScreenSender::~ScreenSender()
{
	running = false;
	findingThread.join();
	sendingThread.join();
}


void ScreenSender::sendingFunc()
{
	double wait = 0;
	char* data = new char[5 + screen.size * 2];
	UINT16* colorPtr = (UINT16*)(&data[5]);
	while (running)
	{
		if (clock() >= nextTouchCheck)
		{
			touchSupport();
			nextTouchCheck += 100;
		}

		if (bestRegion.priority > 0)
		{
			DrawingRegion t = (DrawingRegion)bestRegion;

			bestRegion.priority = 0;
			data[0] = t.mode;
			data[1] = (t.x & 255);
			data[2] = t.x >> 8;
			data[3] = t.y;
			data[4] = t.size - 1;
			int millis;
			UINT16* tPtr;
			switch (t.mode)
			{
			case 0:
				arduinoScreen.fillRectangle(t);
				*colorPtr = getU16(t.color);
				target.WriteData(data, 7);
				wait += sendTimes[0][t.size] - sendDelay;
				if (wait < 0) wait = 0;
				millis = (int)(wait - 0.5);
				if (millis > 0)
				{
					Sleep(millis);
					wait -= millis + 0.5;
				}
				break;
			case 1:
				tPtr = colorPtr;
				for (int y = t.y; y < t.y + t.size; y++)
				{
					for (int x = t.x; x < t.x + t.size * 4; x += 4)
					{
						RGBQUAD& colorRef = screen.at(x, y);
						arduinoScreen.at(x, y) = colorRef;
						arduinoScreen.at(x + 1, y) = colorRef;
						arduinoScreen.at(x + 2, y) = colorRef;
						arduinoScreen.at(x + 3, y) = colorRef;
						*tPtr = getU16(colorRef);
						++tPtr;
					}
				}
				target.WriteData(data, 5 + t.size * t.size * 2);
				break;
			case 2:
				tPtr = colorPtr;
				for (int x = t.x; x < t.x + t.size * 4; x += 4)
				{
					RGBQUAD& colorRef = screen.at(x, t.y);
					arduinoScreen.at(x, t.y) = colorRef;
					arduinoScreen.at(x + 1, t.y) = colorRef;
					arduinoScreen.at(x + 2, t.y) = colorRef;
					arduinoScreen.at(x + 3, t.y) = colorRef;
					*tPtr = getU16(colorRef);
					++tPtr;
				}
				target.WriteData(data, 5 + t.size * 2);
				break;
			case 3:
				tPtr = colorPtr;
				for (int y = t.y; y < t.y + t.size * 4; y += 4)
				{
					RGBQUAD& colorRef = screen.at(t.x, y);
					arduinoScreen.at(t.x, y) = colorRef;
					arduinoScreen.at(t.x, y + 1) = colorRef;
					arduinoScreen.at(t.x, y + 2) = colorRef;
					arduinoScreen.at(t.x, y + 2) = colorRef;
					*tPtr = getU16(colorRef);
					++tPtr;
				}
				target.WriteData(data, 5 + t.size * 2);
				break;
			}

			//std::cout << t.size << std::endl;
		}
		else if (clock() > timeToStartSleep)
		{
			Sleep(1);
		}
	}
	delete[] data;
}


void ScreenSender::findingFunc()
{
	while (running)
	{
		DrawingRegionWithPriority n;
		int sizeXMultiplier;
		int sizeYMultiplier;
		n.mode = rand() & 1;
		switch (n.mode)
		{
		case 0:
			n.x = rand() % screen.width;
			n.y = rand() % screen.height;
			sizeXMultiplier = 1;
			sizeYMultiplier = 1;
			n.color = screen.at(n.x, n.y);
			break;
		case 1:
		case 2:
			n.x = rand() % (screen.width - 3);
			n.y = rand() % screen.height;
			sizeXMultiplier = 4;
			sizeYMultiplier = 1;
			break;
		case 3:
			n.x = rand() % screen.width;
			n.y = rand() % (screen.height - 3);
			sizeXMultiplier = 1;
			sizeYMultiplier = 4;
			break;
		}
		n.contrast = calculateUnitContrast(n, n.x, n.y);
		if (n.contrast > 0)
		{
			n.size = 1;
			int nxSize = sizeXMultiplier;
			int nySize = sizeYMultiplier;
			calculatePriority(n);
			DrawingRegionWithPriority t;
			t.mode = n.mode;
			if (t.mode == 0) t.color = n.color;
			do
			{
				t.size = n.size * 2;
				int txSize = (t.mode == 3 ? nxSize : nxSize * 2);
				int tySize = (t.mode == 2 ? nySize : nySize * 2);
				t.x = n.x / txSize * txSize;
				t.y = n.y / tySize * tySize;
				int x2 = t.x + txSize;
				int y2 = t.y + tySize;
				if (x2 > screen.width || y2 > screen.height)
				{
					break;
				}
				t.contrast = n.contrast;
				for (register int y = t.y; y < y2; y += sizeYMultiplier)
				{
					int xStart, xEnd;
					if (y >= n.y && y < n.y + nySize)
					{
						if (t.x == n.x)
						{
							xStart = t.x + nxSize;
							xEnd = x2;
						}
						else
						{
							xStart = t.x;
							xEnd = x2 - nxSize;
						}
					}
					else
					{
						xStart = t.x;
						xEnd = x2;
					}
					for (register int x = xStart; x < xEnd; x += sizeXMultiplier)
					{
						t.contrast += calculateUnitContrast(t, x, y);
					}
				}
				calculatePriority(t);
				if (t.priority > n.priority)
				{
					n = t;
					nxSize = txSize;
					nySize = tySize;
				}
				else
				{
					break;
				}
			} while (n.size < 256);

			if (n.priority > bestRegion.priority)
			{
				bestRegion = n;
				timeToStartSleep = clock() + millisUntilSlowMode;
			}
		}
		else if (clock() > timeToStartSleep) Sleep(1);
	}
}


unsigned char ScreenSender::reverse(unsigned char b)
{
	b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
	b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
	b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
	return b;
}


uint32_t ScreenSender::reverse4(uint32_t i)
{
	return
		uint32_t(reverse(i)) << 24 |
		uint32_t(reverse(i >> 8)) << 16 |
		uint32_t(reverse(i >> 16)) << 8 |
		uint32_t(reverse(i >> 24));
}


int ScreenSender::calculateUnitContrast(const DrawingRegionWithPriority& region, const int& x, const int& y)
{
	RGBQUAD& colorRef = screen.at(x, y);
	if (region.mode == 0)
	{
		return contrast(arduinoScreen.at(x, y), region.color) - 2 * contrast(colorRef, region.color);
	}
	else if (region.mode == 3)
	{
		return contrast(colorRef, arduinoScreen.at(x, y)) +
			contrast(arduinoScreen.at(x, y + 1), colorRef) +
			contrast(arduinoScreen.at(x, y + 2), colorRef) +
			contrast(arduinoScreen.at(x, y + 3), colorRef) - 2 * (
				contrast(screen.at(x, y + 1), colorRef) +
				contrast(screen.at(x, y + 2), colorRef) +
				contrast(screen.at(x, y + 3), colorRef));
	}
	else
	{
		return contrast(colorRef, arduinoScreen.at(x, y)) +
			contrast(arduinoScreen.at(x + 1, y), colorRef) +
			contrast(arduinoScreen.at(x + 2, y), colorRef) +
			contrast(arduinoScreen.at(x + 3, y), colorRef) - 2 * (
				contrast(screen.at(x + 1, y), colorRef) +
				contrast(screen.at(x + 2, y), colorRef) +
				contrast(screen.at(x + 3, y), colorRef));
	}
}


void ScreenSender::touchSupport()
{
	while (target.Available() >= 4)
	{
		int16_t data[2];
		target.Read((char*)data, 4);
		if (data[0] == -1)
		{
			if (mouseClicked)
			{
				SendInput(1, &releaseInput, sizeof(INPUT));
				mouseClicked = false;
			}
		}
		else
		{
			int x = -data[0];
			int y = data[1];
			if (x < touchXMin) touchXMin = x;
			else if (x > touchXMax) touchXMax = x;
			if (y < touchYMin) touchYMin = y;
			else if (y > touchYMax) touchYMax = y;
			moveInput.mi.dx = (x - touchXMin) * 65536 / (touchXMax - touchXMin);
			moveInput.mi.dy = (y - touchYMin) * 65536 / (touchYMax - touchYMin);
			SendInput(1, &moveInput, sizeof(INPUT));
			if (!mouseClicked)
			{
				SendInput(1, &clickInput, sizeof(INPUT));
				mouseClicked = true;
			}
		}
	}
}