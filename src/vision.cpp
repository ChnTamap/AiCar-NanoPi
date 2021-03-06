#include <iostream>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "time.h"
#include "unistd.h"
//Test WiringPi
#include "wiringSerial.h"

using namespace cv;
using namespace std;

// #define IS_DISPLAY
#define IS_SEND
// #define IS_COLOR
// #define IS_CHANGE_FRAME
#define IS_DEBUG
#define IS_NEGATIVE
#define TICK_MS 1000000

//Param
#define LH_RED 0
#define HH_RED 24
#define LH_BLUE 106
#define HH_BLUE 135
#define LH_GREEN 37
#define HH_GREEN 77
#define LH_BALL LH_BLUE
#define HH_BALL HH_BLUE
#define LH_RECT LH_GREEN
#define HH_RECT HH_GREEN
#define MISS_WIDTH
//SV
#define LS_BALL 110 //60
#define LV_BALL 43  //100
#define LS_RECT 100
#define LV_RECT 45
//FRAME
#define FRAME_HIGH_HEIGHT 120
#define FRAME_LOW_HEIGHT 100
#define FRAME_CHANGE_TIMES 8
int iLowH = LH_BALL;  //69
int iHighH = HH_BALL; //96
int iLowS = LS_BALL;
int iHighS = 255;
int iLowV = LV_BALL;
int iHighV = 255;
int iColor = 0;
//Image
Mat frame;
Mat src;
//Cam
VideoCapture cap;
int changeCount = FRAME_CHANGE_TIMES;
int highFlag = 1; //高清标志

void selectColor(int v)
{
	if (v == 0)
	{
		iLowH = LH_RED;
		iHighH = HH_RED;
		iLowS = LS_BALL;
		iLowV = LV_BALL;
	}
	else if (v == 1)
	{
		iLowH = LH_BLUE;
		iHighH = HH_BLUE;
		iLowS = LS_BALL;
		iLowV = LV_BALL;
	}
	else if (v == 2)
	{
		iLowH = LH_GREEN;
		iHighH = HH_GREEN;
		iLowS = LS_RECT;
		iLowV = LV_RECT;
	}
	cout << "(" << iLowH << "," << iHighH << "),(" << iLowS << "," << iHighS << "),(" << iLowV << "," << iHighV << ")" << endl;
}

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

	cvCreateTrackbar("Color", "Ctrl", &iColor, 2, selectColor); //Color
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
	uint16_t w;
	uint16_t h;
} VectorRect;

int detectBall(Mat img, VectorRect *maxRect)
{
	unsigned int cols = img.cols;  //Width
	unsigned int rows = img.rows;  //Height
	unsigned int unit = rows / 32; //Unit
	unsigned int x, y, xP, yP;
	unsigned int xMin, yMin, xMax, yMax;
	unsigned int y_realMin =
#ifdef IS_NEGATIVE
		0
#else
		rows
#endif // IS_NEGATIVE
		;
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
				if (
#ifdef IS_NEGATIVE
					yMax >
#else
					yMin <
#endif // IS_NEGATIVE
						y_realMin &&
					(yMax - yMin > 10))
				{
					maxRect->w = xMax - xMin;
					maxRect->h = yMax - yMin;
					maxRect->x = (xMax + xMin) / 2;
					maxRect->y = (yMax + yMin) / 2;
#ifdef IS_NEGATIVE
					y_realMin = yMax;
#else
					y_realMin = yMin;
#endif // IS_NEGATIVE
#ifdef IS_NEGATIVE
					maxRect->y = rows - maxRect->y;
#endif // IS_NEGATIVE
				}
			}
		}
	}

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
	int i;
	tickSec = (double)getTickCount();
	count = 0;
	//Start command
	VectorRect maxRect = {0, 0, 0, 0};
#ifdef IS_SEND
	for (i = 0; i < 4 * 2; i++)
	{
		serialPutchar(fd, ((unsigned char *)&maxRect)[i]);
	}
#endif
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
#ifdef IS_DEBUG
		cout << "\r";
#endif
		//RGB To HSV
		cvtColor(src, src, CV_BGR2HSV);
		//Find Color
		inRange(src, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), frame);
		//Test
		detectBall(frame, &maxRect);

#ifdef IS_DISPLAY
#ifndef IS_COLOR
		//Show range frame
		imshow("Video", frame);
		waitKey(1);
#endif // IS_COLOR
#endif // IS_DISPLAY

		//Count
		//Receive Command
		if (serialDataAvail(fd))
		{
			key = serialGetchar(fd);
			if (key == 0)
			{
				iLowH = LH_BALL; //Ball
				iHighH = HH_BALL;
				iLowS = LS_BALL;
				iLowV = LV_BALL;
			}
			else if (key == 2)
			{
				iLowH = LH_RECT; //Rect
				iHighH = HH_RECT;
				iLowS = LS_RECT;
				iLowV = LV_RECT;
			}
		}
		if (key == 1 || key == 3)
		{
			maxRect.x = 0;
			maxRect.h = 0;
		}
#ifdef IS_CHANGE_FRAME
		if (!highFlag)
		{
			//低分辨率
			maxRect.x *= 2;
			maxRect.y *= 2;
			maxRect.w *= 2;
			maxRect.h *= 2;
		}
		if (maxRect.h > FRAME_HIGH_HEIGHT)
		{
			if (changeCount > 0)
				changeCount--;
			else if (highFlag)
			{
				cap.set(CV_CAP_PROP_FRAME_WIDTH, 320);
				cap.set(CV_CAP_PROP_FRAME_HEIGHT, 240);
				highFlag = 0;
			}
		}
		else if (maxRect.h < FRAME_LOW_HEIGHT)
		{
			if (changeCount < FRAME_CHANGE_TIMES * 2)
				changeCount++;
			else if (!highFlag)
			{
				cap.set(CV_CAP_PROP_FRAME_WIDTH, 640);
				cap.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
				highFlag = 1;
			}
		}
#endif
#ifdef IS_SEND
		//Send Rect (Test WiringPi)
		if (maxRect.x)
		{
			for (i = 0; i < 4 * 2; i++)
			{
				serialPutchar(fd, ((unsigned char *)&maxRect)[i]);
			}
		}
#endif // IS_SEND
#ifdef IS_DEBUG
		count++;
		if ((double)getTickCount() - tickSec > (TICK_MS * 1000))
		{
			cout << "(" << maxRect.x << "," << maxRect.y << "," << maxRect.w << "," << maxRect.h << ")";
			cout << "    " << changeCount << "    ";
			cout << "Real FPS:" << count << "|" << key << "||" << flush;
			count = 0;
			tickSec = (double)getTickCount();
		}
		//End tick
		tick = (double)getTickCount() - tick;
//关闭帧率限制
// if (tick < (TICK_MS * 25))
// {
// 	usleep(((TICK_MS * 25) - tick) / 1000);
// }
#endif
	}
	return 0;
}
