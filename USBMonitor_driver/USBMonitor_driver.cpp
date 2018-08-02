// USBMonitor_driver.cpp: Okreœla punkt wejœcia dla aplikacji konsoli.
//

#include "stdafx.h"
#include "SerialPort.h"
#include "ScreenSender.h"
#include "ScreenCapturer.h"
#include "Bitmap.h"
#include <iostream>
#include <SFML\Graphics.hpp>

#define ARDUINO_PORT L"COM4"

int main()
{
	const float fps = 30;
	SerialPort arduino(ARDUINO_PORT, 500000);
	std::cout << arduino.IsConnected() << std::endl;
	if (arduino.IsConnected())
	{
		Bitmap screen(320, 240);
		ScreenSender screenSender(screen, arduino, 1000);
		ScreenCapturer screenCapturer(screen, fps, ScreenSender::screenCaptured, &screenSender);
		
		sf::RenderWindow window(sf::VideoMode(screen.width * 2, screen.height * 2), "Screen / Arduino");
		bool active = true;
		window.setFramerateLimit((int)fps);
		sf::Texture texture[4];
		sf::Sprite sprite[4];
		sf::Uint8* pixelData[4];
		for (int i = 0; i < 4; i++)
		{
			texture[i].create(screen.width, screen.height);
			sprite[i].setTexture(texture[i]);
			pixelData[i] = new sf::Uint8[screen.size * 4];
		}
		sprite[1].setPosition(sf::Vector2f((float)screen.width, 0.f));
		sprite[2].setPosition(sf::Vector2f(0.f, (float)screen.height));
		sprite[3].setPosition(sf::Vector2f((float)screen.width, (float)screen.height));

		for (int textureIndex = 0; textureIndex < 4; textureIndex++)
		{
			for (register int i = 0; i < screen.size; i++)
			{
				sf::Uint8* t = pixelData[textureIndex] + i * 4;
				t[3] = 255;
			}
		}

		while (window.isOpen())
		{
			sf::Event event;
			while (window.pollEvent(event))
			{
				switch (event.type)
				{
				case sf::Event::Closed:
					window.close();
					break;
				case sf::Event::LostFocus:
					active = false;
					break;
				case sf::Event::GainedFocus:
					active = true;
					break;
				}
			}

			if (active)
			{
				for (register int i = 0; i < screen.size; i++)
				{
					const RGBQUAD* s1 = screen.getData() + i;
					const RGBQUAD* s2 = screenSender.getArduinoPixelData() + i;

					sf::Uint8* t = pixelData[0] + i * 4;
					t[0] = s1->rgbRed;
					t[1] = s1->rgbGreen;
					t[2] = s1->rgbBlue;

					t = pixelData[1] + i * 4;
					t[0] = s2->rgbRed;
					t[1] = s2->rgbGreen;
					t[2] = s2->rgbBlue;

					t = pixelData[2] + i * 4;
					t[0] = abs(int(s2->rgbRed) - s1->rgbRed);
					t[1] = abs(int(s2->rgbGreen) - s1->rgbGreen);
					t[2] = abs(int(s2->rgbBlue) - s1->rgbBlue);

					sf::Uint8* tf = pixelData[3] + i * 4;
					tf[0] = (int(t[0]) + t[1] + t[2]) / 3;
					tf[1] = tf[0];
					tf[2] = tf[0];
				}

				for (int i = 0; i < 4; i++)
				{
					texture[i].update(pixelData[i]);
					window.draw(sprite[i]);
				}
				window.display();
			}
			else
			{
				sf::sleep(sf::milliseconds(100));
			}
		}

		for (int i = 0; i < 4; i++)
		{
			delete[] pixelData[i];
		}
	}
    return 0;
}

