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

bool OpenPort();  //打开串口  
bool SetupDCB(int rate_arg);  //设置DCB  
bool SetupTimeout(DWORD ReadInterval, DWORD ReadTotalMultiplier, DWORD
	ReadTotalConstant, DWORD WriteTotalMultiplier, DWORD WriteTotalConstant);   //设置超时  
void ReciveChar();   //接收字符  
bool WriteChar(char* szWriteBuffer, DWORD dwSend);  //发送字符  

bool OpenPort()
{
	hComm = CreateFile(L"COM8",//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!此处更改com口！！！！！！！！！！！！！！！
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
	if (!GetCommState(hComm, &dcb))//获取当前DCB配置  
	{
		return FALSE;
	}
	dcb.DCBlength = sizeof(dcb);
	/* ---------- Serial Port Config ------- */
	dcb.BaudRate = rate_arg; //波特率
	dcb.Parity = NOPARITY;  //奇偶校验位（Parity），在数据存储和传输中，字节中额外增加一个比特位，用来检验错误。它常常是从两个或更多的原始数据中产生一个冗余数据，冗余数据可以从一个原始数据中进行重建。不过，奇偶校验数据并不是对原始数据的完全复制。
	dcb.fParity = 0;         //关闭奇偶校验，1是开
	dcb.StopBits = ONESTOPBIT;//停止位
	dcb.ByteSize = 8;
	dcb.fOutxCtsFlow = 0;     //输出cts
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
	timeouts.ReadIntervalTimeout = ReadInterval;  //////读取字符之间的最大时间
	timeouts.ReadTotalTimeoutConstant = ReadTotalConstant;//以毫秒为单位。
	timeouts.ReadTotalTimeoutMultiplier = ReadTotalMultiplier;//字符乘数???
	timeouts.WriteTotalTimeoutConstant = WriteTotalConstant;//以毫秒为单位。
	timeouts.WriteTotalTimeoutMultiplier = WriteTotalMultiplier;//字符乘数
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
			bResult = ReadFile(hComm,  //通信设备（此处为串口）句柄，由CreateFile()返回值得到  
				&RXBuff,               //指向接收缓冲区  
				1,                     //指明要从串口中读取的字节数  
				&BytesRead,            //  
				&OverLapped);          //OVERLAPPED结构  
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
		bResult = WriteFile(hComm,  //通信设备句柄，CreateFile()返回值得到  
			szWriteBuffer,  //指向写入数据缓冲区  
			dwSend,  //设置要写的字节数  
			&BytesSent,  //  
			&OverLapped);  //指向异步I/O数据  
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

string number;//存储二维码的字符

int main(int argc, char** argv)
{
	if (OpenPort())
		cout << "Open port success" << endl;
	if (SetupDCB(9600))//!!!!!!!!!!!!!!!!!!!此处更改波特率！！！！！！！！！！！！！！！！！！！！！！
		cout << "Set DCB success" << endl;
	if (SetupTimeout(0, 0, 0, 0, 0))
		cout << "Set timeout success" << endl;
	PurgeComm(hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	while (1)
	{
		/*///////这里从串口得到第一条指令///////////////////*/
		reconginazeQR();//这里单片机给它输入一个值。

		/*///////这里从串口得到第二条指令（夹取第一个物料）///////////////////*/
		informationQR(0,number);//读取字符串的第一个数字
		RecognitionColor(minH, maxH, minS, maxS, minV, maxV);
		circleCenter(minH, maxH, minS, maxS, minV, maxV);

		/*///////这里从串口得到第三条指令（夹取第二个物料）///////////////////*/
		informationQR(1, number);//读取字符串的第一个数字
		RecognitionColor(minH, maxH, minS, maxS, minV, maxV);
		circleCenter(minH, maxH, minS, maxS, minV, maxV);

		/*///////这里从串口得到第四条指令(夹取第三个物料)///////////////////*/
		informationQR(2, number);//读取字符串的第一个数字
		RecognitionColor(minH, maxH, minS, maxS, minV, maxV);
		circleCenter(minH, maxH, minS, maxS, minV, maxV);
		//WriteChar("123", 3);//可以自行设置分部发送

		/*///////回到原点///////////////////*/
	}
	return 0;
}

/*定义一个函数，用来读取二维码*/
string reconginazeQR()//输入指令开始此段函数
{
	VideoCapture video(0);
	//if (!video.isOpened())
	//{
	//	printf("输入视频错误...");
	//}

	Mat frame;
	bool stop(false);
	while (video.read(frame))
	{
		ImageScanner scanner;//从二维图像读取条形码
		scanner.set_config(ZBAR_NONE, ZBAR_CFG_ENABLE, 1);
		Mat grayimage;
		cvtColor(frame, grayimage, COLOR_BGR2GRAY);
		int widthe = grayimage.cols;
		int heighte = grayimage.rows;
		uchar *raw = (uchar *)grayimage.data;
		Image imageZbar(widthe, heighte, "Y800", raw, widthe * heighte);  //Y800是指灰度图像  			

		scanner.scan(imageZbar); //扫描条码  	
		SymbolIterator symbol = imageZbar.symbol_begin();//Image支持常见的YUV以及RGB图像
		///imshow("123", frame);

		if (symbol == imageZbar.symbol_end())
		{
			//cout << "查询条码失败，请检查图片！" << endl;
			continue;
		}

		if (symbol != imageZbar.symbol_end())
		{
			//cout << "类型：" << symbol->get_type_name() << endl;
			//cout << "条码：" << symbol->get_data() << endl;
			++symbol;
			number = symbol->get_data();
			imageZbar.set_data(NULL, 0);
			break;
		}
	}
	return number;//返回二维码的字符

	video.release();
	waitKey(0);
}

int informationQR(int time,string numbers)
{	
	int color = numbers[time];
	/*判断读入的数据*/
	//color = '3';////////////////
	switch (color)
	{
	case '1': //这里1代表红色
		minH = 0;// || 156;/////////////
		maxH = 10;// || 180;///////////////////////////
		minS = 100;
		maxS = 255;
		minV = 100;
		maxV = 255;
		return minH, maxH, minS, maxS, minV, maxV;
		break;
	case'2'://2代表绿色
		minH = 30;
		maxH = 89;
		minS = 100;
		maxS = 255;
		minV = 100;
		maxV = 255;
		return minH, maxH, minS, maxS, minV, maxV;
		break;
	case '3'://3代表红色
		minH = 90;
		maxH = 124;
		minS = 100;
		maxS = 255;
		minV = 100;
		maxV = 255;
		return minH, maxH, minS, maxS, minV, maxV;
		break;
	default:
		//cout << "输出错误" << endl;
		break;
	}
}

	/*****************************************识别颜色**************************************/
void RecognitionColor(int &minH, int &maxH,int &minS,int &maxS, int &minV,int &maxV)
{
	Mat frame, gray, hsv;
	double sum_x = 0;

	VideoCapture video;

	video.open(0);

	for (int j = 0; j < 100; )//循环多次，有接近100次大小范围在一定值的时候，找出平均值，返回平均值
	{
		video >> frame;
		if (frame.empty())
		{
			//printf("视频读取出错...");
			continue;//跳出这个循环，继续读取下一帧
		}
		//imshow("sourse", frame);
		cvtColor(frame, hsv, COLOR_BGR2HSV);

		/*分离图像*/
		vector<Mat>hsvSplit;
		split(hsv, hsvSplit);
		//均衡化
		equalizeHist(hsvSplit[2], hsvSplit[2]);//均衡化的是第三通道吗？
		merge(hsvSplit, hsv);
		//二值化
		Mat threshImg;
		inRange(hsv, Scalar(minH, minS, minV), Scalar(maxH, maxS, maxV), threshImg);
		//imshow("threshImg", threshImg);

		//形态学操作
		Mat morImg;
		Mat kernel = getStructuringElement(MORPH_RECT, Size(11, 11), Point(-1, -1));/////////////////////////////////////////////////////////////////
		morphologyEx(threshImg, morImg, MORPH_OPEN, kernel, Point(), 3);
		//imshow("morImg", morImg);

		//找到轮廓
		vector<vector<Point>>contours;
		vector<Vec4i>hierachy;
		findContours(morImg, contours, hierachy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point());

		//计算轮廓矩       
		vector<Moments> mu(contours.size());
		vector<vector<double>> center_x(contours.size()), center_y(contours.size());
		vector<Point2f> mc(contours.size());
		for (int i = 0; i < contours.size(); i++)
		{
			mu[i] = moments(contours[i], false);
		}
		//计算轮廓的质心
		int i = 0; //这个i好像只能在这里定义
		for ( i; i < contours.size(); i++)//正常情况下，这里contours.size()=1
		{
			mc[i] = Point2d(mu[i].m10 / mu[i].m00, mu[i].m01 / mu[i].m00);
			center_x[j][i] = mc[i].x;//确认是否可以这样读取数组，为什么我在找下一个函数的时候不能这样用
			//center_y[j][i] = mc[i].y;
		}
		if (center_x[j][i] > 100 && center_x[j][i] < 400)//x的坐标值，这个也看现场测试//////////////////////////////////////////////////////////////////////
		{
			sum_x += center_x[j][i];
			j++;
		}
	}
		printf("center_x=%f\n",sum_x/100.0);//这个要传给串口
		video.release();//释放储存空间
}


/***********************************圆圈中心识别**************************************/
void circleCenter(int &minH, int &maxH, int &minS, int &maxS, int &minV, int &maxV)
{
	Mat frame, gray, hsv;
	double sum_circle_x=0;
	//Mat frame = imread("C:/Users/Administrator/Desktop/photo/53.png");
	//resize(src, frame, src.size() / 4);

	VideoCapture video;

	video.open(0);

	for (int j = 0; j < 100; )//循环多次，有接近100次大小范围在一定值的时候，找出平均值，返回平均值
	{
		video >> frame;
		if (frame.empty())
		{
			//printf("视频读取出错...");
			continue;//跳出这个循环，继续读取下一帧
		}
		//imshow("sourse", frame);
		cvtColor(frame, hsv, COLOR_BGR2HSV);

		/*分离图像*/
		vector<Mat>hsvSplit;
		split(hsv, hsvSplit);
		//均衡化
		equalizeHist(hsvSplit[2], hsvSplit[2]);//均衡化的是第三通道吗？
		merge(hsvSplit, hsv);

		//二值化
		Mat threshImg;
		inRange(hsv, Scalar(minH, minS, minV), Scalar(maxH, maxS, maxV), threshImg);
		//imshow("threshImg", threshImg);

		//形态学操作
		Mat morImg;
		Mat kernel_close = getStructuringElement(MORPH_RECT, Size(20, 20), Point(-1, -1));/////////////////////////////////////////////////////////////////
		Mat kernel_open = getStructuringElement(MORPH_RECT, Size(7, 7), Point(-1, -1));////////////////////////////////////////////////////////////////////
		morphologyEx(threshImg, morImg, MORPH_OPEN, kernel_open, Point(), 3);
		morphologyEx(threshImg, morImg, MORPH_CLOSE, kernel_close, Point(), 7);
		//imshow("morImg", morImg);

		//找到轮廓
		vector<vector<Point>>contours;
		vector<Vec4i>hierachy;
		findContours(morImg, contours, hierachy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point());

		double lenth;//获取轮廓的周长，用来去除过小的轮廓
		for (int i = 0; i < contours.size(); i++)
		{
			lenth = arcLength(contours[i], true);
			//printf("lenth=%f", lenth);////////////////////////////////////////////////现场测试的时候开///////////////////////////////////////////////////////
		}

		if (lenth > 700)///////////////////////////////到时候调整这些参数，看什么值合适一点，记得现场测试和拍照/////////////////////////////////////////////////
		{
			//计算轮廓矩       
			vector<Moments> mu(contours.size());
			for (int i = 0; i < contours.size(); i++)
			{
				mu[i] = moments(contours[i], false);
			}
			//计算轮廓的质心     
			vector<vector<double>> center_x(contours.size()), center_y(contours.size());
			vector<Point2f> mc(contours.size());
			int i = 0;
			for (i; i < contours.size(); i++)
			{
				mc[i] = Point2d(mu[i].m10 / mu[i].m00, mu[i].m01 / mu[i].m00);
				center_x[j][i] = mc[i].x;
				//center_y[i] = mc[i].y;//////////////////////y坐标需不需要
				//printf("center_y[%d]=%f\n", i, center_y[i]);//这个要传给串口
				//return center_x[i];
			}
			if (center_x[j][i] > 10 && center_x[j][i] < 1000)/////////////////////////x的坐标值，这个也看现场测试//////////////////////////////////////////////////////////////////////
			{
				sum_circle_x += center_x[j][i];
				j++;
			}
		}
	}
	printf("center_x=%f\n", sum_circle_x/100);//应该是通过这个传递给串口的吧
	video.release();
}
