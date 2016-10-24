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
HANDLE hCom;  //ȫ�ֱ��������ھ��
//����
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
		//AfxMessageBox("��COMʧ��!");
		return FALSE;
	}
	SetupComm(hCom, 1024, 1024); //���뻺����������������Ĵ�С����1024


	////�趨����ʱ
	//TimeOuts.ReadIntervalTimeout = 1000;
	//TimeOuts.ReadTotalTimeoutMultiplier = 500;
	//TimeOuts.ReadTotalTimeoutConstant = 5000;
	////�趨д��ʱ
	//TimeOuts.WriteTotalTimeoutMultiplier = 500;
	//TimeOuts.WriteTotalTimeoutConstant = 2000;
	//SetCommTimeouts(hCom, &TimeOuts); //���ó�ʱ

	DCB dcb;
	GetCommState(hCom, &dcb);
	dcb.BaudRate = 9600; //������Ϊ9600
	dcb.ByteSize = 8; //ÿ���ֽ���8λ
	dcb.Parity = NOPARITY; //����żУ��λ
	dcb.StopBits = TWOSTOPBITS; //����ֹͣλ
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
	memset(lpOutBuffer, '\0', 4); //ǰ7���ֽ�������
	if (dat>255)
		dat = 255;
	lpOutBuffer[0] = dir;  //���ͻ������ĵ�1���ֽ�ΪDC1
	lpOutBuffer[1] = dat;  //��2���ֽ�Ϊ�ַ�0(30H)
	lpOutBuffer[2] = '\r'; //��3���ֽ�Ϊ�ַ�0(30H)
	lpOutBuffer[3] = '\n'; // ��4���ֽ�Ϊ�ַ�1(31H)
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

	CvCapture* pCap = cvCreateCameraCapture(0);//����-1Ҳ���ԣ������ҵĵ���װ����CyberLink YouCam�����
	//OpenCV��Ĭ�ϵ��ø�����ͷ����������ϵͳ������
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
	// CvCapture ��һ���ṹ�壬��������ͼ�񲶻�����Ҫ����Ϣ��
	// opencv�ṩ���ַ�ʽ���ⲿ����ͼ��һ���Ǵ�����ͷ�У�һ��
	// ��ͨ��������Ƶ�õ�ͼ�����ַ�ʽ������ӵ�һ֡��ʼһ֡һ֡
	// �İ�˳���ȡ�����ÿ��ȡһ֡��Ҫ������Ӧ��״̬�Ͳ�����
	// �������Ƶ�ļ��л�ȡ����Ҫ������Ƶ�ļ����ļ�������Ӧ��******
	// ���ͣ���һ�����Ҫ��ȡ����Ҫ������һ֡�ȡ� ��Щ��Ϣ��������
	// CvCapture�ṹ�У�ÿ��ȡһ֡����Щ��Ϣ���������£���ȡ��һ֡
	// ��Ҫ������Ϣ������ȡ��api�ӿ�
	//=======================================================
	//CvCapture* capture = 0;
	//===========================================================
	// IplImage �ǽṹ�����ͣ���������һ֡ͼ�����Ϣ��Ҳ����һ֡
	// ͼ�����������ֵ���ɵ�һ������
	//===========================================================
	IplImage *frame, *frame_copy = 0;

	// ����һ�����ڣ��á�result����Ϊ���ڵı�ʶ��
	cvNamedWindow("Camera", 1);

	// ==========================================
	// ��ʼ��һ����Ƶ���������
	// ���ߵײ�Ĳ���api����� Capture1.avi�в���ͼƬ��
	// �ײ�api����Ⲣѡ����Ӧ��******������׼������
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
		// ��������˼��̣����˳����򣬷������������һ֡

		GetKey = cvWaitKey(10);
		if ((char)GetKey == 'q')//�س����˳�
			break;
	}
	cvReleaseCapture(&pCap);
	cvDestroyWindow("Camera");

	return 0;
	//  // ��� ��ʼ��ʧ�ܣ���ôcaptureΪ��ָ�룬����ֹͣ��
	//  // ������벶��ѭ��
	//  if( capture )
	//  {
	//      if( cvGrabFrame( capture ))
	//            frame = cvRetrieveFrame( capture );
	//// �����ȡ�����ת��ʧ�ܣ����˳�ѭ��
	//if( frame )
	//{
	//	CenterPoint=frame->width/2;
	//}
	//	

	//// ����ѭ��
	//        for(;;)
	//        {
	//            // ����cvGrabFrame,�õײ�api����һ֡ͼ��
	//            // �������ʧ�ܣ����˳�ѭ��
	//            // ����ɹ��������ͼ�񱣴��ڵײ�api�Ļ�����
	//            if( !cvGrabFrame( capture ))
	//                break;
	//            
	//				// ������õ�ͼ����Ϣ�ӻ�����ת����IplImage��ʽ����frame��
	//				frame = cvRetrieveFrame( capture );
	//
	//				// �����ȡ�����ת��ʧ�ܣ����˳�ѭ��
	//				if( !frame )
	//					 break;
	//				detect_and_draw( frame ); 
	//
	//				// ��frame�е�ͼ����Ϣ�ڴ���result����ʾ
	//				// detect_and_draw( frame ); 
	//				cvShowImage( "source", frame );
	//
	//				// ��ͣһ��������㿴һ��ͼ��
	//				//Sleep(10);
	//            
	//            // ��������˼��̣����˳����򣬷������������һ֡
	//            if( cvWaitKey( 10 ) >= 0 )
	//                break;
	//        }
	//
	//        // �˳�����֮ǰҪ����һ�¶�ջ�е��ڴ棬����ڴ�й¶
	//        //cvReleaseImage( &frame );ע�ⲻ��Ҫ��䣬��Ϊframe�Ǵ���Ƶ�в���ģ�û�е��������ڴ棬�����ͷţ���capture �ͷŵ�ʱ��frame��Ȼ���ͷ��ˡ�
	//        
	//        // �˳�֮ǰ�����ײ�api�Ĳ���������������ռ��é�Ӳ���ʺ
	//        // �����ʹ�ñ�ĳ����޷������Ѿ������Ǵ򿪵��ļ�
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

	cvEqualizeHist(small_img, small_img); //ֱ��ͼ����

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
		//��ȡ���ĵ�
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
