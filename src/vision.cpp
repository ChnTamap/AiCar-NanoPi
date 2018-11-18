#include <iostream>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "time.h"
#include "unistd.h"
//Test WiringPi
#include "wiringSerial.h"

using namespace cv;
using namespace std;

#define IS_DISPLAY
// #define IS_SEND
#define IS_COLOR
#define IS_NEGATIVE
#define TICK_MS 1000000

//Param
#define LH_BLUE 106
#define HH_BLUE 135
#define LH_GREEN 40
#define HH_GREEN 70
#define LH_BALL LH_BLUE
#define HH_BALL HH_BLUE
#define LH_RECT LH_GREEN
#define HH_RECT HH_GREEN
int iLowH = LH_BALL;  //69
int iHighH = HH_BALL; //96
int iLowS = 120;
int iHighS = 255;
int iLowV = 70;
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

typedef struct
{
	uint16_t x;
	uint16_t y;
	uint16_t h;
	uint16_t w;
} VectorRect;

int detectBall(Mat img, VectorRect *maxRect)
{
	unsigned int cols = img.cols;  //Width
	unsigned int rows = img.rows;  //Height
	unsigned int unit = rows / 32; //Unit
	unsigned int x, y, xP, yP;
	unsigned int xMin, yMin, xMax, yMax;
	//Max
	maxRect->x = 0;
	maxRect->y = 0;
	maxRect->h = 0;
	maxRect->w = 0;
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
				//Find max size
				if (xMax - xMin > maxRect->w)
				{
					maxRect->w = xMax - xMin;
					maxRect->h = yMax - yMin;
					maxRect->x = (xMax + xMin) / 2;
					maxRect->y = (yMax + yMin) / 2;
#ifdef IS_NEGATIVE
					maxRect->y = rows - maxRect->y;
#endif // IS_NEGATIVE
				}
			}
		}
	}

	cout << "(" << maxRect->x << "," << maxRect->y << "," << maxRect->w << "," << maxRect->h << ")";
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
	cout << "Open Cam success:(" << cap.get(CV_CAP_PROP_FRAME_WIDTH) << "," << cap.get(CV_CAP_PROP_FRAME_HEIGHT) << ")" << endl;
	cap >> src;
	cout << "Read Img success:(" << src.rows << "," << src.cols << ")" << endl;

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

	/* Test Wiring Pi */
	int fd;
	if ((fd = serialOpen("/dev/ttyS1", 115200)) < 0)
	{
		cout << "Error of Serial\n";
		return 1;
	}

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

#ifdef IS_DISPLAY
#ifdef IS_COLOR
		//Show Src
		imshow("Video", src);
		waitKey(1);
#endif // IS_COLOR
#endif // IS_DISPLAY

		Vector<Mat> hsvPanels;
		cout << "\r";
		//RGB To HSV
		cvtColor(src, src, CV_BGR2HSV);
		//Find Color
		inRange(src, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), frame);
		//Test
		VectorRect maxRect;
		detectBall(frame, &maxRect);

#ifdef IS_DISPLAY
#ifndef IS_COLOR
		//Show range frame
		imshow("Video", frame);
		waitKey(1);
#endif // IS_COLOR
#endif // IS_DISPLAY

		//Count
		count++;
		if ((double)getTickCount() - tickSec > (TICK_MS * 1000))
		{
			cout << "Real FPS:" << count << "|" << key << "||" << flush;
			count = 0;
			tickSec = (double)getTickCount();
		}
		//End tick
		tick = (double)getTickCount() - tick;
		if (tick < (TICK_MS * 25))
		{
			usleep(((TICK_MS * 25) - tick) / 1000);
		}
#ifdef IS_SEND
		//Receive Command
		if (serialDataAvail(fd))
		{
			key = serialGetchar(fd);
			if (key == 0)
			{
				iLowH = LH_BALL; //Ball
				iHighH = HH_BALL;
			}
			else if (key == 2)
			{
				iLowH = LH_RECT; //Rect
				iHighH = HH_RECT;
			}
		}
		if (key == 1 || key == 3)
		{
			maxRect.x = 0;
		}
//Send Rect (Test WiringPi)
		if (maxRect.x)
		{
			for (int i = 0; i < 4 * 2; i++)
			{
				serialPutchar(fd, ((unsigned char *)&maxRect)[i]);
			}
		}
#endif // IS_SEND
	}
	return 0;
}