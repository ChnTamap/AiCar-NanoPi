#include <iostream>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "time.h"
#include "unistd.h"

using namespace cv;
using namespace std;

// #define IS_DISPLAY
#define TICK_MS 1000000

//Param
int iLowH = 107;
int iHighH = 136;
int iLowS = 128;
int iHighS = 255;
int iLowV = 81;
int iHighV = 255;
//Image
Mat frame;
Mat src;
//Cam
VideoCapture cap;

void initCtrlWindow(void)
{
	//Window
	namedWindow("Ctrl", CV_WINDOW_AUTOSIZE);

	//Ctrl
	cvCreateTrackbar("LowH", "Ctrl", &iLowH, 179); //Hue (0 - 179)
	cvCreateTrackbar("HighH", "Ctrl", &iHighH, 179);

	cvCreateTrackbar("LowS", "Ctrl", &iLowS, 255); //Saturation (0 - 255)
	cvCreateTrackbar("HighS", "Ctrl", &iHighS, 255);

	cvCreateTrackbar("LowV", "Ctrl", &iLowV, 255); //Value (0 - 255)
	cvCreateTrackbar("HighV", "Ctrl", &iHighV, 255);
}

typedef struct
{
	uint8_t H;
	uint8_t S;
	uint8_t V;
} HSV_Typedef;

typedef struct
{
	uint16_t x;
	uint16_t y;
} Vector2D;

int detectBall(Mat img)
{
	unsigned int cols = img.cols;  //Width
	unsigned int rows = img.rows;  //Height
	unsigned int unit = rows / 32; //Unit
	unsigned int x, y, xP, yP;
	unsigned int xMin, yMin, xMax, yMax;
	//Color point
	uchar *color;
	uchar *colorLine;
	//Stack
	Vector2D checkList[rows * rows];
	Vector2D *pCheckPoint = checkList;
	Vector2D *endCheckPoint = checkList + rows * rows - 5;

	//Draw border
	for (y = 0; y < rows; y++)
	{
		colorLine = img.ptr(y);
		*colorLine = 0;
		*(colorLine + cols - 1) = 0;
	}
	color = img.ptr(0);
	colorLine = img.ptr(rows - 1);
	for (x = 0; x < cols; x++)
	{
		*(color + x) = 0;
		*(colorLine + x) = 0;
	}

	//Check by 'Water'
	for (y = 0; y < rows; y += unit)
	{
		colorLine = img.ptr(y);
		for (x = 0; x < cols; x += unit)
		{
			color = colorLine + x;
			if (*color)
			{
				*color = 0;
				xMin = xMax = x;
				yMin = yMax = y;
				pCheckPoint = checkList;
				pCheckPoint->x = x;
				(pCheckPoint++)->y = y;
				while ((pCheckPoint--) > checkList)
				{
					xP = pCheckPoint->x;
					yP = pCheckPoint->y;
					if (xP > xMax)
						xMax = xP;
					if (yP > yMax)
						yMax = yP;
					if (xP < xMin)
						xMin = xP;
					if (yP < yMin)
						yMin = yP;
					color = img.ptr(yP) + xP - 1;
					//Left
					if (*(color) > 0x80)
					{
						pCheckPoint->x = xP - 1;
						(pCheckPoint++)->y = yP;
						*color = 0;
					}
					else
						*color = 0x80;
					//Right
					color += 2;
					if (*(color) > 0x80)
					{
						pCheckPoint->x = xP + 1;
						(pCheckPoint++)->y = yP;
						*color = 0;
					}
					else
						*color = 0x80;
					//Up
					color = img.ptr(yP - 1) + xP;
					if (*(color) > 0x80)
					{
						pCheckPoint->x = xP;
						(pCheckPoint++)->y = yP - 1;
						*color = 0;
					}
					else
						*color = 0x80;
					//Down
					color = img.ptr(yP + 1) + xP;
					if (*(color) > 0x80)
					{
						pCheckPoint->x = xP;
						(pCheckPoint++)->y = yP + 1;
						*color = 0;
					}
					else
						*color = 0x80;
					if (pCheckPoint > endCheckPoint)
					{
						cout << "Error" << endl;
						return -1;
					}
				}
				if (xMax - xMin > unit * 8)
				{
					cout << "(" << xMax + xMin / 2 << "," << yMax + yMin / 2 << ")";
				}
			}
		}
	}

	cout << "        " << flush;

	return 0;
}

int cvMain(void)
{
	/* Init Camrea */
	cap = VideoCapture(0);
	if (!cap.isOpened())
	{
		cout << "Cannot open the cam" << endl;
		return -1;
	}

	/* Init Display */
#ifdef IS_DISPLAY
	namedWindow("Video", CV_WINDOW_AUTOSIZE);
	initCtrlWindow();
#endif // IS_DISPLAY

	/* Init Timeer */
	double tick;
	double tickSec;
	int count = 0;
	//Test
	tick = (double)getTickCount();
	usleep(TICK_MS * 1000 / 1000);
	tickSec = (double)getTickCount() - tick;
	cout << tickSec << "Tick per sec" << endl;

	/* Loop */
	//Loop init
	int key;
	tickSec = (double)getTickCount();
	count = 0;
	//Loop
	while (true)
	{
		//Start tick
		tick = (double)getTickCount();

		//Get frame
		cap >> src;
		Vector<Mat> hsvPanels;
		cout << "\r";
		//RGB To HSV
		cvtColor(src, src, CV_BGR2HSV);
		//Find Color
		inRange(src, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), frame);
		//Test
		detectBall(frame);

#ifdef IS_DISPLAY
		//Show frame
		imshow("Video", frame);
		waitKey(1);
#endif // IS_DISPLAY

		//Count
		count++;
		if ((double)getTickCount() - tickSec > (TICK_MS * 1000))
		{
			cout << "Real FPS:" << count << "|" << flush;
			count = 0;
			tickSec = (double)getTickCount();
		}
		//End tick
		tick = (double)getTickCount() - tick;
		if (tick < (TICK_MS * 25))
		{
			usleep(((TICK_MS * 25) - tick) / 1000);
		}
	}

	return 0;
}