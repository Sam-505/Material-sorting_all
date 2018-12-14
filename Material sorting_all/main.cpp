#include <iostream>  
#include <cstdlib>  
#include <windows.h>  
#include <stdlib.h>
#include <opencv2/opencv.hpp>
#include <math.h>
#include <zbar.h>

using namespace std;
using namespace cv;
using namespace zbar;

HANDLE hComm;
OVERLAPPED OverLapped;
COMSTAT Comstat;
DWORD dwCommEvents;

bool OpenPort();  //�򿪴���  
bool SetupDCB(int rate_arg);  //����DCB  
bool SetupTimeout(DWORD ReadInterval, DWORD ReadTotalMultiplier, DWORD
	ReadTotalConstant, DWORD WriteTotalMultiplier, DWORD WriteTotalConstant);   //���ó�ʱ  
void ReciveChar();   //�����ַ�  
bool WriteChar(char* szWriteBuffer, DWORD dwSend);  //�����ַ�  

bool OpenPort()
{
	hComm = CreateFile(L"COM8",//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!�˴�����com�ڣ�����������������������������
		GENERIC_READ | GENERIC_WRITE,
		0,
		0,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED,
		0);
	if (hComm == INVALID_HANDLE_VALUE)
		return FALSE;
	else
		return true;
}

bool SetupDCB(int rate_arg)
{
	DCB dcb;
	memset(&dcb, 0, sizeof(dcb));
	if (!GetCommState(hComm, &dcb))//��ȡ��ǰDCB����  
	{
		return FALSE;
	}
	dcb.DCBlength = sizeof(dcb);
	/* ---------- Serial Port Config ------- */
	dcb.BaudRate = rate_arg; //������
	dcb.Parity = NOPARITY;  //��żУ��λ��Parity���������ݴ洢�ʹ����У��ֽ��ж�������һ������λ��������������������Ǵ�����������ԭʼ�����в���һ���������ݣ��������ݿ��Դ�һ��ԭʼ�����н����ؽ�����������żУ�����ݲ����Ƕ�ԭʼ���ݵ���ȫ���ơ�
	dcb.fParity = 0;         //�ر���żУ�飬1�ǿ�
	dcb.StopBits = ONESTOPBIT;//ֹͣλ
	dcb.ByteSize = 8;
	dcb.fOutxCtsFlow = 0;     //���cts
	dcb.fOutxDsrFlow = 0;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;
	dcb.fDsrSensitivity = 0;
	dcb.fRtsControl = RTS_CONTROL_DISABLE;
	dcb.fOutX = 0;
	dcb.fInX = 0;
	dcb.fErrorChar = 0;
	dcb.fBinary = 1;
	dcb.fNull = 0;
	dcb.fAbortOnError = 0;
	dcb.wReserved = 0;
	dcb.XonLim = 2;
	dcb.XoffLim = 4;
	dcb.XonChar = 0x13;
	dcb.XoffChar = 0x19;
	dcb.EvtChar = 0;
	if (!SetCommState(hComm, &dcb))
	{
		return false;
	}
	else
		return true;
}

bool SetupTimeout(DWORD ReadInterval, DWORD ReadTotalMultiplier, DWORD
	ReadTotalConstant, DWORD WriteTotalMultiplier, DWORD WriteTotalConstant)
{
	COMMTIMEOUTS timeouts;
	timeouts.ReadIntervalTimeout = ReadInterval;  //////��ȡ�ַ�֮������ʱ��
	timeouts.ReadTotalTimeoutConstant = ReadTotalConstant;//�Ժ���Ϊ��λ��
	timeouts.ReadTotalTimeoutMultiplier = ReadTotalMultiplier;//�ַ�����???
	timeouts.WriteTotalTimeoutConstant = WriteTotalConstant;//�Ժ���Ϊ��λ��
	timeouts.WriteTotalTimeoutMultiplier = WriteTotalMultiplier;//�ַ�����
	if (!SetCommTimeouts(hComm, &timeouts))
	{
		return false;
	}
	else
		return true;
}

void ReciveChar()
{
	bool bRead = TRUE;
	bool bResult = TRUE;
	DWORD dwError = 0;
	DWORD BytesRead = 0;
	char RXBuff;
	for (;;)
	{
		bResult = ClearCommError(hComm, &dwError, &Comstat);
		if (Comstat.cbInQue == 0)
			continue;
		if (bRead)
		{
			bResult = ReadFile(hComm,  //ͨ���豸���˴�Ϊ���ڣ��������CreateFile()����ֵ�õ�  
				&RXBuff,               //ָ����ջ�����  
				1,                     //ָ��Ҫ�Ӵ����ж�ȡ���ֽ���  
				&BytesRead,            //  
				&OverLapped);          //OVERLAPPED�ṹ  
			cout << RXBuff << endl;
			if (!bResult)
			{
				switch (dwError == GetLastError())
				{
				case ERROR_IO_PENDING:
					bRead = FALSE;
					break;
				default:
					break;
				}
			}
		}
		else
		{
			bRead = TRUE;
		}
	}
	if (!bRead)
	{
		bRead = TRUE;
		bResult = GetOverlappedResult(hComm,
			&OverLapped,
			&BytesRead,
			TRUE);
	}
}

bool WriteChar(char* szWriteBuffer, DWORD dwSend)
{
	bool bWrite = TRUE;
	bool bResult = TRUE;
	DWORD BytesSent = 0;
	HANDLE hWriteEvent = NULL;
	ResetEvent(hWriteEvent);
	if (bWrite)
	{
		OverLapped.Offset = 0;
		OverLapped.OffsetHigh = 0;
		bResult = WriteFile(hComm,  //ͨ���豸�����CreateFile()����ֵ�õ�  
			szWriteBuffer,  //ָ��д�����ݻ�����  
			dwSend,  //����Ҫд���ֽ���  
			&BytesSent,  //  
			&OverLapped);  //ָ���첽I/O����  
		if (!bResult)
		{
			DWORD dwError = GetLastError();
			switch (dwError)
			{
			case ERROR_IO_PENDING:
				BytesSent = 0;
				bWrite = FALSE;
				break;
			default:
				break;
			}
		}
	}
	if (!bWrite)
	{
		bWrite = TRUE;
		bResult = GetOverlappedResult(hComm,
			&OverLapped,
			&BytesSent,
			TRUE);
		if (!bResult)
		{
			std::cout << "GetOverlappedResults() in WriteFile()" << std::endl;
		}
	}
	if (BytesSent != dwSend)
	{
		std::cout << "WARNING: WriteFile() error.. Bytes Sent:" << BytesSent << "; Message Length: " << strlen((char*)szWriteBuffer) << std::endl;
	}
	return TRUE;
}


/***********************************************************************************************************************************************/
int minH, maxH, minS, maxS, minV, maxV;
string reconginazeQR();
int informationQR(int time, string number);
void RecognitionColor(int &minH, int &maxH, int &minS, int &maxS, int &minV, int &maxV);
void circleCenter(int &minH, int &maxH, int &minS, int &maxS, int &minV, int &maxV);

string number;//�洢��ά����ַ�

int main(int argc, char** argv)
{
	if (OpenPort())
		cout << "Open port success" << endl;
	if (SetupDCB(9600))//!!!!!!!!!!!!!!!!!!!�˴����Ĳ����ʣ�������������������������������������������
		cout << "Set DCB success" << endl;
	if (SetupTimeout(0, 0, 0, 0, 0))
		cout << "Set timeout success" << endl;
	PurgeComm(hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	while (1)
	{
		/*///////����Ӵ��ڵõ���һ��ָ��///////////////////*/
		reconginazeQR();//���ﵥƬ����������һ��ֵ��

		/*///////����Ӵ��ڵõ��ڶ���ָ���ȡ��һ�����ϣ�///////////////////*/
		informationQR(0,number);//��ȡ�ַ����ĵ�һ������
		RecognitionColor(minH, maxH, minS, maxS, minV, maxV);
		circleCenter(minH, maxH, minS, maxS, minV, maxV);

		/*///////����Ӵ��ڵõ�������ָ���ȡ�ڶ������ϣ�///////////////////*/
		informationQR(1, number);//��ȡ�ַ����ĵ�һ������
		RecognitionColor(minH, maxH, minS, maxS, minV, maxV);
		circleCenter(minH, maxH, minS, maxS, minV, maxV);

		/*///////����Ӵ��ڵõ�������ָ��(��ȡ����������)///////////////////*/
		informationQR(2, number);//��ȡ�ַ����ĵ�һ������
		RecognitionColor(minH, maxH, minS, maxS, minV, maxV);
		circleCenter(minH, maxH, minS, maxS, minV, maxV);
		//WriteChar("123", 3);//�����������÷ֲ�����

		/*///////�ص�ԭ��///////////////////*/
	}
	return 0;
}

/*����һ��������������ȡ��ά��*/
string reconginazeQR()//����ָ�ʼ�˶κ���
{
	VideoCapture video(0);
	//if (!video.isOpened())
	//{
	//	printf("������Ƶ����...");
	//}

	Mat frame;
	bool stop(false);
	while (video.read(frame))
	{
		ImageScanner scanner;//�Ӷ�άͼ���ȡ������
		scanner.set_config(ZBAR_NONE, ZBAR_CFG_ENABLE, 1);
		Mat grayimage;
		cvtColor(frame, grayimage, COLOR_BGR2GRAY);
		int widthe = grayimage.cols;
		int heighte = grayimage.rows;
		uchar *raw = (uchar *)grayimage.data;
		Image imageZbar(widthe, heighte, "Y800", raw, widthe * heighte);  //Y800��ָ�Ҷ�ͼ��  			

		scanner.scan(imageZbar); //ɨ������  	
		SymbolIterator symbol = imageZbar.symbol_begin();//Image֧�ֳ�����YUV�Լ�RGBͼ��
		///imshow("123", frame);

		if (symbol == imageZbar.symbol_end())
		{
			//cout << "��ѯ����ʧ�ܣ�����ͼƬ��" << endl;
			continue;
		}

		if (symbol != imageZbar.symbol_end())
		{
			//cout << "���ͣ�" << symbol->get_type_name() << endl;
			//cout << "���룺" << symbol->get_data() << endl;
			++symbol;
			number = symbol->get_data();
			imageZbar.set_data(NULL, 0);
			break;
		}
	}
	return number;//���ض�ά����ַ�

	video.release();
	waitKey(0);
}

int informationQR(int time,string numbers)
{	
	int color = numbers[time];
	/*�ж϶��������*/
	//color = '3';////////////////
	switch (color)
	{
	case '1': //����1�����ɫ
		minH = 0;// || 156;/////////////
		maxH = 10;// || 180;///////////////////////////
		minS = 100;
		maxS = 255;
		minV = 100;
		maxV = 255;
		return minH, maxH, minS, maxS, minV, maxV;
		break;
	case'2'://2������ɫ
		minH = 30;
		maxH = 89;
		minS = 100;
		maxS = 255;
		minV = 100;
		maxV = 255;
		return minH, maxH, minS, maxS, minV, maxV;
		break;
	case '3'://3�����ɫ
		minH = 90;
		maxH = 124;
		minS = 100;
		maxS = 255;
		minV = 100;
		maxV = 255;
		return minH, maxH, minS, maxS, minV, maxV;
		break;
	default:
		//cout << "�������" << endl;
		break;
	}
}

	/*****************************************ʶ����ɫ**************************************/
void RecognitionColor(int &minH, int &maxH,int &minS,int &maxS, int &minV,int &maxV)
{
	Mat frame, gray, hsv;
	double sum_x = 0;

	VideoCapture video;

	video.open(0);

	for (int j = 0; j < 100; )//ѭ����Σ��нӽ�100�δ�С��Χ��һ��ֵ��ʱ���ҳ�ƽ��ֵ������ƽ��ֵ
	{
		video >> frame;
		if (frame.empty())
		{
			//printf("��Ƶ��ȡ����...");
			continue;//�������ѭ����������ȡ��һ֡
		}
		//imshow("sourse", frame);
		cvtColor(frame, hsv, COLOR_BGR2HSV);

		/*����ͼ��*/
		vector<Mat>hsvSplit;
		split(hsv, hsvSplit);
		//���⻯
		equalizeHist(hsvSplit[2], hsvSplit[2]);//���⻯���ǵ���ͨ����
		merge(hsvSplit, hsv);
		//��ֵ��
		Mat threshImg;
		inRange(hsv, Scalar(minH, minS, minV), Scalar(maxH, maxS, maxV), threshImg);
		//imshow("threshImg", threshImg);

		//��̬ѧ����
		Mat morImg;
		Mat kernel = getStructuringElement(MORPH_RECT, Size(11, 11), Point(-1, -1));/////////////////////////////////////////////////////////////////
		morphologyEx(threshImg, morImg, MORPH_OPEN, kernel, Point(), 3);
		//imshow("morImg", morImg);

		//�ҵ�����
		vector<vector<Point>>contours;
		vector<Vec4i>hierachy;
		findContours(morImg, contours, hierachy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point());

		//����������       
		vector<Moments> mu(contours.size());
		vector<vector<double>> center_x(contours.size()), center_y(contours.size());
		vector<Point2f> mc(contours.size());
		for (int i = 0; i < contours.size(); i++)
		{
			mu[i] = moments(contours[i], false);
		}
		//��������������
		int i = 0; //���i����ֻ�������ﶨ��
		for ( i; i < contours.size(); i++)//��������£�����contours.size()=1
		{
			mc[i] = Point2d(mu[i].m10 / mu[i].m00, mu[i].m01 / mu[i].m00);
			center_x[j][i] = mc[i].x;//ȷ���Ƿ����������ȡ���飬Ϊʲô��������һ��������ʱ����������
			//center_y[j][i] = mc[i].y;
		}
		if (center_x[j][i] > 100 && center_x[j][i] < 400)//x������ֵ�����Ҳ���ֳ�����//////////////////////////////////////////////////////////////////////
		{
			sum_x += center_x[j][i];
			j++;
		}
	}
		printf("center_x=%f\n",sum_x/100.0);//���Ҫ��������
		video.release();//�ͷŴ���ռ�
}


/***********************************ԲȦ����ʶ��**************************************/
void circleCenter(int &minH, int &maxH, int &minS, int &maxS, int &minV, int &maxV)
{
	Mat frame, gray, hsv;
	double sum_circle_x=0;
	//Mat frame = imread("C:/Users/Administrator/Desktop/photo/53.png");
	//resize(src, frame, src.size() / 4);

	VideoCapture video;

	video.open(0);

	for (int j = 0; j < 100; )//ѭ����Σ��нӽ�100�δ�С��Χ��һ��ֵ��ʱ���ҳ�ƽ��ֵ������ƽ��ֵ
	{
		video >> frame;
		if (frame.empty())
		{
			//printf("��Ƶ��ȡ����...");
			continue;//�������ѭ����������ȡ��һ֡
		}
		//imshow("sourse", frame);
		cvtColor(frame, hsv, COLOR_BGR2HSV);

		/*����ͼ��*/
		vector<Mat>hsvSplit;
		split(hsv, hsvSplit);
		//���⻯
		equalizeHist(hsvSplit[2], hsvSplit[2]);//���⻯���ǵ���ͨ����
		merge(hsvSplit, hsv);

		//��ֵ��
		Mat threshImg;
		inRange(hsv, Scalar(minH, minS, minV), Scalar(maxH, maxS, maxV), threshImg);
		//imshow("threshImg", threshImg);

		//��̬ѧ����
		Mat morImg;
		Mat kernel_close = getStructuringElement(MORPH_RECT, Size(20, 20), Point(-1, -1));/////////////////////////////////////////////////////////////////
		Mat kernel_open = getStructuringElement(MORPH_RECT, Size(7, 7), Point(-1, -1));////////////////////////////////////////////////////////////////////
		morphologyEx(threshImg, morImg, MORPH_OPEN, kernel_open, Point(), 3);
		morphologyEx(threshImg, morImg, MORPH_CLOSE, kernel_close, Point(), 7);
		//imshow("morImg", morImg);

		//�ҵ�����
		vector<vector<Point>>contours;
		vector<Vec4i>hierachy;
		findContours(morImg, contours, hierachy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point());

		double lenth;//��ȡ�������ܳ�������ȥ����С������
		for (int i = 0; i < contours.size(); i++)
		{
			lenth = arcLength(contours[i], true);
			//printf("lenth=%f", lenth);////////////////////////////////////////////////�ֳ����Ե�ʱ��///////////////////////////////////////////////////////
		}

		if (lenth > 700)///////////////////////////////��ʱ�������Щ��������ʲôֵ����һ�㣬�ǵ��ֳ����Ժ�����/////////////////////////////////////////////////
		{
			//����������       
			vector<Moments> mu(contours.size());
			for (int i = 0; i < contours.size(); i++)
			{
				mu[i] = moments(contours[i], false);
			}
			//��������������     
			vector<vector<double>> center_x(contours.size()), center_y(contours.size());
			vector<Point2f> mc(contours.size());
			int i = 0;
			for (i; i < contours.size(); i++)
			{
				mc[i] = Point2d(mu[i].m10 / mu[i].m00, mu[i].m01 / mu[i].m00);
				center_x[j][i] = mc[i].x;
				//center_y[i] = mc[i].y;//////////////////////y�����費��Ҫ
				//printf("center_y[%d]=%f\n", i, center_y[i]);//���Ҫ��������
				//return center_x[i];
			}
			if (center_x[j][i] > 10 && center_x[j][i] < 1000)/////////////////////////x������ֵ�����Ҳ���ֳ�����//////////////////////////////////////////////////////////////////////
			{
				sum_circle_x += center_x[j][i];
				j++;
			}
		}
	}
	printf("center_x=%f\n", sum_circle_x/100);//Ӧ����ͨ��������ݸ����ڵİ�
	video.release();
}
