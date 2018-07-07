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
		sendTimes[0][0][i] = sendTimeConstant + i * i * timeFactor[0];
		sendTimes[0][1][i] = sendTimeConstant + i * i * timeFactor[1];
		sendTimes[0][2][i] = sendTimes[0][1][i];
		sendTimes[0][3][i] = sendTimeConstant + i * timeFactor[1];
		sendTimes[0][4][i] = sendTimes[0][3][i];
		sendTimes[1][0][i] = sendTimes[0][0][i];
		sendTimes[1][1][i] = sendTimes[0][1][i];
		sendTimes[1][2][i] = sendTimes[0][1][i];
		sendTimes[1][3][i] = sendTimes[0][3][i];
		sendTimes[1][4][i] = sendTimes[0][3][i];
	}

	bestRegions[0].priority = 0;
	bestRegions[1].priority = 0;

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
	char* data = new char[6 + screen.size * 2];
	while (running)
	{
		if (clock() >= nextTouchCheck)
		{
			touchSupport();
			nextTouchCheck += 100;
		}

		if (bestRegions[0].priority > 0)
		{
			timeToStartSleep = clock() + millisUntilSlowMode;
			auto& t = bestRegions[0];
			int datacount = 0;
			if (portrait)
			{
				if (t.mode == 1 || t.mode == 4)
				{
					portrait = false;
					data[0] = 'L';
					datacount = 1;
				}
			}
			else
			{
				if (t.mode == 2 || t.mode == 3)
				{
					portrait = true;
					data[0] = 'P';
					datacount = 1;
				}
			}
			UINT16* colorPtr = (UINT16*)(&data[datacount + 5]);
			switch (t.mode)
			{
			case 0:
				arduinoScreen.fillRectangle(t);
				*colorPtr = getU16(t.color);
				break;
			case 1:
				for (int y = t.y; y < t.y2; y++)
				{
					for (int x = t.x; x < t.x2; x += 4)
					{
						RGBQUAD& colorRef = screen.at(x, y);
						arduinoScreen.at(x, y) = colorRef;
						arduinoScreen.at(x + 1, y) = colorRef;
						arduinoScreen.at(x + 2, y) = colorRef;
						arduinoScreen.at(x + 3, y) = colorRef;
						*colorPtr = getU16(colorRef);
						++colorPtr;
					}
				}
				break;
			case 2:
				for (int x = t.x; x < t.x2; x++)
				{
					for (int y = t.y2 - 4; y >= t.y; y -= 4)
					{
						RGBQUAD& colorRef = screen.at(x, y);
						arduinoScreen.at(x, y) = colorRef;
						arduinoScreen.at(x, y + 1) = colorRef;
						arduinoScreen.at(x, y + 2) = colorRef;
						arduinoScreen.at(x, y + 3) = colorRef;
						*colorPtr = getU16(colorRef);
						++colorPtr;
					}
				}
				break;
			case 3:
				for (int x = t.x; x < t.x2; x += 2)
				{
					RGBQUAD& colorRef = screen.at(x, t.y);
					arduinoScreen.at(x, t.y) = colorRef;
					arduinoScreen.at(x + 1, t.y) = colorRef;
					arduinoScreen.at(x, t.y + 1) = colorRef;
					arduinoScreen.at(x + 1, t.y + 1) = colorRef;
					*colorPtr = getU16(colorRef);
					++colorPtr;
				}
				break;
			case 4:
				for (int y = t.y; y < t.y2; y += 2)
				{
					RGBQUAD& colorRef = screen.at(t.x, y);
					arduinoScreen.at(t.x, y) = colorRef;
					arduinoScreen.at(t.x + 1, y) = colorRef;
					arduinoScreen.at(t.x, y + 1) = colorRef;
					arduinoScreen.at(t.x + 1, y + 1) = colorRef;
					*colorPtr = getU16(colorRef);
					++colorPtr;
				}
				break;
			}

			int firstCoord = t.x;
			int secondCoord;
			if (portrait)
			{
				secondCoord = screen.height - t.y2;
			}
			else
			{
				secondCoord = t.y;
			}
			data[datacount] = t.mode + '0';
			data[datacount + 1] = (firstCoord & 255);
			data[datacount + 2] = firstCoord >> 8;
			data[datacount + 3] = secondCoord;
			data[datacount + 4] = t.size - 1;
			if (t.mode == 0)
			{
				target.WriteData(data, 7 + datacount);
				wait += sendTimes[portrait][0][t.size] - sendDelay;
				if (wait < 0) wait = 0;
				int millis = (int)(wait - 0.5);
				if (millis > 0)
				{
					Sleep(millis);
					wait -= millis + 0.5;
				}
			}
			else if (t.mode >= 3)
			{
				target.WriteData(data, 5 + t.size * 2 + datacount);
			}
			else
			{
				target.WriteData(data, 5 + t.size * t.size * 2 + datacount);
			}

			//std::cout << "m" << t.mode << " s" << t.size << ", " << t.x << "x" << t.y << std::endl;

			t = bestRegions[1];
			bestRegions[1].priority = 0;
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
		n.mode = rand() % 5;
		n.x = rand() % screen.width;
		n.y = rand() % screen.height;
		/*if (bestRegions[0].priority > 0)
		{
			if (n.x >= bestRegions[0].x && n.x < bestRegions[0].x2 && n.y >= bestRegions[0].y && n.y < bestRegions[0].y2) continue;
			if (bestRegions[1].priority > 0 && bestRegions[1].mode == n.mode)
			{
				if (n.x >= bestRegions[1].x && n.x < bestRegions[1].x2 && n.y >= bestRegions[1].y && n.y < bestRegions[1].y2) continue;
			}
		}*/
		const int& sizeXMultiplier = regionSizeXMultiplier[n.mode];
		const int& sizeYMultiplier = regionSizeYMultiplier[n.mode];
		n.x = n.x / sizeXMultiplier * sizeXMultiplier;
		n.y = n.y / sizeYMultiplier * sizeYMultiplier;
		if (n.mode == 0) n.color = screen.at(n.x, n.y);
		n.contrast = calculateUnitContrast(n, n.x, n.y);
		if (n.contrast > 0)
		{
			n.size = 1;
			n.sizeX = sizeXMultiplier;
			n.sizeY = sizeYMultiplier;
			n.x2 = n.x + n.sizeX;
			n.y2 = n.y + n.sizeY;
			calculatePriority(n);
			DrawingRegionWithPriority t;
			t.mode = n.mode;
			if (t.mode == 0) t.color = n.color;
			do
			{
				t.size = n.size * 2;
				t.sizeX = (t.mode == 4 ? n.sizeX : n.sizeX * 2);
				t.sizeY = (t.mode == 3 ? n.sizeY : n.sizeY * 2);
				t.x = n.x / t.sizeX * t.sizeX;
				t.y = n.y / t.sizeY * t.sizeY;
				t.x2 = t.x + t.sizeX;
				t.y2 = t.y + t.sizeY;
				if (t.x2 > screen.width || t.y2 > screen.height)
				{
					break;
				}
				t.contrast = n.contrast;
				for (register int y = t.y; y < t.y2; y += sizeYMultiplier)
				{
					int xStart, xEnd;
					if (y >= n.y && y < n.y2)
					{
						if (t.x == n.x)
						{
							xStart = n.x2;
							xEnd = t.x2;
						}
						else
						{
							xStart = t.x;
							xEnd = n.x;
						}
					}
					else
					{
						xStart = t.x;
						xEnd = t.x2;
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
				}
				else
				{
					break;
				}
			} while (n.size < 256);

			if (n.priority > bestRegions[1].priority)
			{
				if (n.priority > bestRegions[0].priority && bestRegions[0].priority <= 0)
				{
					bestRegions[1] = bestRegions[0];
					bestRegions[0] = n;
				}
				else
				{
					bestRegions[1] = n;
				}
			}
		}
		else if (clock() > timeToStartSleep) Sleep(1);
	}
}


/*unsigned char ScreenSender::reverse(unsigned char b)
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
}*/


int ScreenSender::calculateUnitContrast(const DrawingRegionWithPriority& region, const int& x, const int& y)
{
	RGBQUAD& colorRef = screen.at(x, y);
	if (region.mode == 0)
	{
		return contrast(arduinoScreen.at(x, y), region.color) - 2 * contrast(colorRef, region.color);
	}
	else if (region.mode == 1)
	{
		return contrast(colorRef, arduinoScreen.at(x, y)) +
			contrast(arduinoScreen.at(x + 1, y), colorRef) +
			contrast(arduinoScreen.at(x + 2, y), colorRef) +
			contrast(arduinoScreen.at(x + 3, y), colorRef) - 2 * (
				contrast(screen.at(x + 1, y), colorRef) +
				contrast(screen.at(x + 2, y), colorRef) +
				contrast(screen.at(x + 3, y), colorRef));
	}
	else if (region.mode >= 3)
	{
		return contrast(colorRef, arduinoScreen.at(x, y)) +
			contrast(arduinoScreen.at(x + 1, y), colorRef) +
			contrast(arduinoScreen.at(x, y + 1), colorRef) +
			contrast(arduinoScreen.at(x + 1, y + 1), colorRef) - 2 * (
				contrast(screen.at(x + 1, y), colorRef) +
				contrast(screen.at(x, y + 1), colorRef) +
				contrast(screen.at(x + 1, y + 1), colorRef));
	}
	else
	{
		return contrast(colorRef, arduinoScreen.at(x, y)) +
			contrast(arduinoScreen.at(x, y + 1), colorRef) +
			contrast(arduinoScreen.at(x, y + 2), colorRef) +
			contrast(arduinoScreen.at(x, y + 3), colorRef) - 2 * (
				contrast(screen.at(x, y + 1), colorRef) +
				contrast(screen.at(x, y + 2), colorRef) +
				contrast(screen.at(x, y + 3), colorRef));
	}
}


void ScreenSender::touchSupport()
{
	unsigned char mode = 'T';
	target.WriteData((char*)&mode, 1);
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