#include"stdafx.h"
#include<opencv/cvaux.h>
#include<opencv/highgui.h>
#include<opencv/cxcore.h>
#include <opencv\cv.h>
#include<Windows.h>//serial port communication

#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <assert.h> 
#include <math.h> 
#include <float.h> 
#include <limits.h> 
#include <time.h> 
#include <ctype.h>



#ifdef _EiC 
#define WIN32 
#endif

static CvMemStorage* storage = 0;
static CvHaarClassifierCascade* cascade = 0;

void detect_and_draw(IplImage* image);

const char* cascade_name =
"haarcascade_frontalface_alt.xml";
/*    "haarcascade_profileface.xml";*/
HANDLE hCom;  //全局变量，串口句柄
//中心
int CenterPoint = 0;
int DivDat;
bool ComOpen = false;
int Angle = 0;
int DevCnt = 0;
int GetKey;
bool OpenCom()
{

	COMMTIMEOUTS TimeOuts;
	hCom = CreateFile(TEXT("COM3"), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hCom == INVALID_HANDLE_VALUE)
	{
		//AfxMessageBox("打开COM失败!");
		return FALSE;
	}
	SetupComm(hCom, 1024, 1024); //输入缓冲区和输出缓冲区的大小都是1024


	////设定读超时
	//TimeOuts.ReadIntervalTimeout = 1000;
	//TimeOuts.ReadTotalTimeoutMultiplier = 500;
	//TimeOuts.ReadTotalTimeoutConstant = 5000;
	////设定写超时
	//TimeOuts.WriteTotalTimeoutMultiplier = 500;
	//TimeOuts.WriteTotalTimeoutConstant = 2000;
	//SetCommTimeouts(hCom, &TimeOuts); //设置超时

	DCB dcb;
	GetCommState(hCom, &dcb);
	dcb.BaudRate = 9600; //波特率为9600
	dcb.ByteSize = 8; //每个字节有8位
	dcb.Parity = NOPARITY; //无奇偶校验位
	dcb.StopBits = TWOSTOPBITS; //两个停止位
	SetCommState(hCom, &dcb);

	PurgeComm(hCom, PURGE_TXCLEAR | PURGE_RXCLEAR);
	printf("Com Open OK...");
	return TRUE;

}
void CloseCom()
{
	CloseHandle(hCom);
}
void ComSend(unsigned char dir, unsigned char dat)
{
	unsigned char lpOutBuffer[4];
	memset(lpOutBuffer, '\0', 4); //前7个字节先清零
	if (dat>255)
		dat = 255;
	lpOutBuffer[0] = dir;  //发送缓冲区的第1个字节为DC1
	lpOutBuffer[1] = dat;  //第2个字节为字符0(30H)
	lpOutBuffer[2] = '\r'; //第3个字节为字符0(30H)
	lpOutBuffer[3] = '\n'; // 第4个字节为字符1(31H)
	DWORD dwBytesWrite = 4;
	COMSTAT ComStat;
	DWORD dwErrorFlags;
	BOOL bWriteStat;
	ClearCommError(hCom, &dwErrorFlags, &ComStat);
	bWriteStat = WriteFile(hCom, lpOutBuffer, dwBytesWrite, &dwBytesWrite, NULL);

	PurgeComm(hCom, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
	if (!bWriteStat)
	{
		printf("Send Fail...\r\n");
		return;
	}

}
void StepMotor(int step)
{


	if (step>128)
		step = 128;
	else if (step<-128)
		step = -128;
	step = step / 10;
	if (ComOpen)
	{
		Angle = Angle + step;
		if (Angle>128)
		{
			ComSend(0, 128);
			Angle = 0;
			printf("Send  Angle>128\r\n");
			return;
		}
		else if (Angle<-128)
		{
			ComSend(1, 128);
			Angle = 0;
			printf("Send  Angle<-128\r\n");
			return;
		}

		if (step>0)
		{
			ComSend(1, step);
		}
		else
		{
			ComSend(0, 0 - step);
		}
		printf("Send  DivDat= %d\n", step);
	}
}
int main(int argc, char** argv)
{
	//	HWND MyWin;
	ComOpen = OpenCom();
	cascade_name = "haarcascade_frontalface_alt2.xml";
	cascade = (CvHaarClassifierCascade*)cvLoad(cascade_name, 0, 0, 0);

	CvCapture* pCap = cvCreateCameraCapture(0);//这里-1也可以，不过我的电脑装的有CyberLink YouCam软件，
	//OpenCV会默认调用该摄像头，而不调用系统的驱动
	//IplImage *camframe = NULL;

	if (cvCreateCameraCapture == NULL)
	{
		return(0);
	}

	//cvNamedWindow("Camera",CV_WINDOW_FULLSCREEN);

	/*while ((camframe = cvQueryFrame(pCap)) != 0 &&  cvWaitKey(20) != 27)
	{
	camframe = cvQueryFrame(pCap);

	cvShowImage("Camera", camframe);
	}
	*/
	//cvReleaseCapture(&pCap);  
	//cvDestroyWindow("Camera");  



	if (!cascade)
	{
		fprintf(stderr, "ERROR: Could not load classifier cascade\n");
		return -1;
	}
	storage = cvCreateMemStorage(0);
	//   cvNamedWindow( "result", 1 ); 
	//    
	//   const char* filename = "lena.jpg"; 
	//   IplImage* image = cvLoadImage( filename, 1 );

	//   if( image ) 
	//   { 
	//       detect_and_draw( image ); 
	//       cvWaitKey(0); 
	//       cvReleaseImage( &image );   
	//   }

	//   cvDestroyWindow("result"); 
	//return 0;

	//========================================================
	// CvCapture 是一个结构体，用来保存图像捕获所需要的信息。
	// opencv提供两种方式从外部捕获图像，一种是从摄像头中，一种
	// 是通过解码视频得到图像。两种方式都必须从第一帧开始一帧一帧
	// 的按顺序获取，因此每获取一帧后都要保存相应的状态和参数。
	// 比如从视频文件中获取，需要保存视频文件的文件名，相应的******
	// 类型，下一次如果要获取将需要解码哪一帧等。 这些信息都保存在
	// CvCapture结构中，每获取一帧后，这些信息都将被更新，获取下一帧
	// 需要将新信息传给获取的api接口
	//=======================================================
	//CvCapture* capture = 0;
	//===========================================================
	// IplImage 是结构体类型，用来保存一帧图像的信息，也就是一帧
	// 图像的所有像素值构成的一个矩阵
	//===========================================================
	IplImage *frame, *frame_copy = 0;

	// 创建一个窗口，用“result”作为窗口的标识符
	cvNamedWindow("Camera", 1);

	// ==========================================
	// 初始化一个视频捕获操作。
	// 告诉底层的捕获api我想从 Capture1.avi中捕获图片，
	// 底层api将检测并选择相应的******并做好准备工作
	//==============================================
	//capture = cvCaptureFromFile("hello.avi");

	frame = cvQueryFrame(pCap);
	if (frame != 0)
	{
		CenterPoint = frame->width / 2;
	}
	else
		return 0;
	while ((frame = cvQueryFrame(pCap)) != 0)
	{
		frame = cvQueryFrame(pCap);

		detect_and_draw(frame);
		//cvShowImage("Camera", frame); 
		// 如果你敲了键盘，就退出程序，否则继续捕获下一帧

		GetKey = cvWaitKey(10);
		if ((char)GetKey == 'q')//回车键退出
			break;
	}
	cvReleaseCapture(&pCap);
	cvDestroyWindow("Camera");

	return 0;
	//  // 如果 初始化失败，那么capture为空指针，程序停止，
	//  // 否则进入捕获循环
	//  if( capture )
	//  {
	//      if( cvGrabFrame( capture ))
	//            frame = cvRetrieveFrame( capture );
	//// 如果获取缓存或转换失败，则退出循环
	//if( frame )
	//{
	//	CenterPoint=frame->width/2;
	//}
	//	

	//// 捕获循环
	//        for(;;)
	//        {
	//            // 调用cvGrabFrame,让底层api解码一帧图像
	//            // 如果解码失败，就退出循环
	//            // 如果成功，解码的图像保存在底层api的缓存中
	//            if( !cvGrabFrame( capture ))
	//                break;
	//            
	//				// 将解码得到图像信息从缓存中转换成IplImage格式放在frame中
	//				frame = cvRetrieveFrame( capture );
	//
	//				// 如果获取缓存或转换失败，则退出循环
	//				if( !frame )
	//					 break;
	//				detect_and_draw( frame ); 
	//
	//				// 将frame中的图像信息在窗口result中显示
	//				// detect_and_draw( frame ); 
	//				cvShowImage( "source", frame );
	//
	//				// 暂停一会儿，让你看一下图像
	//				//Sleep(10);
	//            
	//            // 如果你敲了键盘，就退出程序，否则继续捕获下一帧
	//            if( cvWaitKey( 10 ) >= 0 )
	//                break;
	//        }
	//
	//        // 退出程序之前要清理一下堆栈中的内存，免得内存泄露
	//        //cvReleaseImage( &frame );注意不需要这句，因为frame是从视频中捕获的，没有单独分配内存，无需释放，当capture 释放的时候frame自然就释放了。
	//        
	//        // 退出之前结束底层api的捕获操作，免得它们占着茅坑不拉屎
	//        // 比如会使得别的程序无法访问已经被它们打开的文件
	//        cvReleaseCapture( &capture );
	//    
	//}
	//    cvDestroyWindow("source");
	//	//cvDestroyWindow("result");
	//    return 0; 
}


void detect_and_draw(IplImage* img)
{
	double scale = 1.2;
	static CvScalar colors[] = {
			{ { 0, 0, 255 } }, { { 0, 128, 255 } }, { { 0, 255, 255 } }, { { 0, 255, 0 } },
			{ { 255, 128, 0 } }, { { 255, 255, 0 } }, { { 255, 0, 0 } }, { { 255, 0, 255 } }
	};//Just some pretty colors to draw with
	DevCnt++;
	if (DevCnt>5)
		DevCnt = 0;

	//Image Preparation 
	// 
	IplImage* gray = cvCreateImage(cvSize(img->width, img->height), 8, 1);
	IplImage* small_img = cvCreateImage(cvSize(cvRound(img->width / scale), cvRound(img->height / scale)), 8, 1);
	cvCvtColor(img, gray, CV_BGR2GRAY);
	cvResize(gray, small_img, CV_INTER_LINEAR);

	cvEqualizeHist(small_img, small_img); //直方图均衡

	//Detect objects if any 
	// 
	cvClearMemStorage(storage);
	double t = (double)cvGetTickCount();
	CvSeq* objects = cvHaarDetectObjects(small_img, cascade, storage, 1.1, 2, 0, cvSize(30, 30));

	t = (double)cvGetTickCount() - t;
	printf("detection time = %gms\n", t / ((double)cvGetTickFrequency()*1000.));

	//Loop through found objects and draw boxes around them 
	for (int i = 0; i<(objects ? objects->total : 0); ++i)
	{
		CvRect* r = (CvRect*)cvGetSeqElem(objects, i);
		cvRectangle(img, cvPoint(r->x*scale, r->y*scale), cvPoint((r->x + r->width)*scale, (r->y + r->height)*scale), colors[i % 8]);
		//获取中心点
		DivDat = (r->x*scale + r->width*scale / 2 - CenterPoint) * 128 / 320;
		if ((i == 0))
			StepMotor(DivDat);

	}
	for (int i = 0; i < (objects ? objects->total : 0); i++)
	{
		CvRect* r = (CvRect*)cvGetSeqElem(objects, i);
		CvPoint center;
		int radius;
		center.x = cvRound((r->x + r->width*0.5)*scale);
		center.y = cvRound((r->y + r->height*0.5)*scale);
		radius = cvRound((r->width + r->height)*0.25*scale);
		cvCircle(img, center, radius, colors[i % 8], 3, 8, 0);
	}

	cvShowImage("Camera", img);
	cvReleaseImage(&gray);
	cvReleaseImage(&small_img);
}
