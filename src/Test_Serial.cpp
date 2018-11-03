/*
 * serial.c:
 *	Example program to read bytes from the Serial line
 *
 * Copyright (c) 2012-2013 Gordon Henderson. <projects@drogon.net>
 ***********************************************************************
 * This file is part of wiringPi:
 *	https://projects.drogon.net/raspberry-pi/wiringpi/
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with wiringPi.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */

/*±‡“Î√¸¡Ó g++ ./src/Test_Serial.cpp -o ./build/Test_serial -lwiringPi -lpthread */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "unistd.h"

#include <wiringSerial.h>

#define USART_DATA_LEN 3

int main()
{
	int fd;

	if ((fd = serialOpen("/dev/ttyS1", 115200)) < 0)
	{
		fprintf(stderr, "Unable to open serial device: %s\n", strerror(errno));
		return 1;
	}

	// Loop, getting and printing characters

	for (;;)
	{
		// putchar(serialGetchar(fd));
		// fflush(stdout);
		short speed[USART_DATA_LEN];
		int i;
		scanf("%hd,%hd,%hd", speed, speed + 1, speed + 2);
		for (i = 0; i < USART_DATA_LEN * 2; i++)
		{
			serialPutchar(fd, ((unsigned char *)speed)[i]);
			printf("%d ", ((unsigned char *)speed)[i]);
		}
		printf("\n");
	}
}
