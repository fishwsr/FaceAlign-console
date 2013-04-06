// Align.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdlib.h>
#include <iostream>
#include <afxwin.h>
#include <windows.h> 
#include <gdiplus.h> 



#include "CThinPlateSpline.h"

#include "FaceAlignDll.h"
#pragma comment(lib,"FaceAlignDll.lib")

using namespace std;
using namespace Gdiplus; 
using namespace cv;

CFaceAlignDll* g_pAlign;
static CascadeClassifier cascade;
const char* face_cascade_name = "lbpcascade_frontalface.xml";
static bool isShowFace = false;
void detectAndDisplay(Mat& image, RECT* dectBox);
void showAlignedFace(Mat& img, float* ptsPos, int ptsNum);

bool ConvertToGrayBits(BYTE* pOrgBits, int width, int height, int nChannels, BYTE* pGrayBits, int nOrgStride, int nGrayStride)
{
	if (nOrgStride == 0)
		nOrgStride = width*nChannels;
	if (nGrayStride == 0)
		nGrayStride = width;


	BYTE *pOrgPtr = pOrgBits, *pGrayPtr = pGrayBits;


	if (nChannels == 1)
	{
		for (int i=0; i<height; i++, pOrgPtr+=nOrgStride, pGrayPtr+=nGrayStride)
			memcpy(pGrayPtr, pOrgPtr, width);
	}
	else
	{
		for (int i=0; i<height; i++, pOrgPtr+=nOrgStride, pGrayPtr+=nGrayStride)
			for(int j=0, k=0; j<width; j++, k+=3)
				pGrayPtr[j] = (BYTE)(((int)pOrgPtr[k+2]*77 + (int)pOrgPtr[k+1]*151 + (int)pOrgPtr[k]*28) >> 8);
	}


	return true;
}

void DestroyAlign()
{
	if( NULL != g_pAlign )
	{
		delete g_pAlign;
		g_pAlign = NULL;
	}
}

int InitAlign(const WCHAR* wzModelFile)
{
	DestroyAlign();

	g_pAlign = new CFaceAlignDll();


	if( !g_pAlign )
		return E_OUTOFMEMORY;
	else
	{
		return g_pAlign->Init( wzModelFile );
	}
}


int Align(BYTE* pImgData, int iImgW, int iImgH, int iStride, RECT& rcFace, float* pPoints)
{
	if( NULL == g_pAlign )
		return E_POINTER;


	//for color image input
	if( iStride / iImgW > 1 ) 
	{
		BYTE* pGrayData = new BYTE[ iImgW* iImgH ];


		ConvertToGrayBits( pImgData, iImgW, iImgH, iStride / iImgW, pGrayData, 0, 0 );


		HRESULT hr = g_pAlign->Align( pGrayData, iImgW, iImgH, iImgW, rcFace, pPoints );


		delete[] pGrayData;


		return hr;
	}
	else
	{
		return g_pAlign->Align( pImgData, iImgW, iImgH, iStride, rcFace, pPoints );
	}
}


int PointNum()
{
	if( NULL == g_pAlign )
		return -1;

	return g_pAlign->FacePtNum();
}


 void AvgShape( float* pPoints )
{
	if( NULL == g_pAlign || NULL == pPoints )
		return;

	g_pAlign->AvgShape(pPoints);
}


 void procPic(CString strFilePath, float* ptsPos1)
 {
	 int Pos = strFilePath.GetLength() - strFilePath.ReverseFind(TEXT('\\')) -1;
	 cout<<strFilePath.Right(Pos)<<":"<<endl;
	 string srcImageFile="";
	 string resImageFile="";
	 srcImageFile = strFilePath;
	 resImageFile = strFilePath;
	 resImageFile.insert(strFilePath.ReverseFind(TEXT('\\')),"\\res");
	 Bitmap srcImg(strFilePath.AllocSysString());
	 if(srcImg.GetLastStatus() == Ok)
	 {		

		 const WCHAR configpath[]=L"casm.bin";
		 InitAlign(configpath); //input file path of casm.bin

		 Gdiplus::Rect detRect(0, 0, srcImg.GetWidth(), srcImg.GetHeight());
		 BitmapData lkData;
		 srcImg.LockBits(&detRect,ImageLockModeWrite,srcImg.GetPixelFormat(),&lkData);

		 int ptsNum = PointNum();
		 if (ptsNum < 0)
		 {
			 printf("Initial failed");
			 exit(0);
		 }
		 ptsNum = ptsNum  <<1;
		 //ptsPos1= new float[ptsNum];

		 Mat image = imread(srcImageFile);
		 //CvSize dst_cvsize;
		 //dst_cvsize.width = 240;
		 //dst_cvsize.height = image.rows * ((float)dst_cvsize.width/image.cols); // 高同比例缩小
		 //resize(image, image, dst_cvsize);

		 RECT rcBoundBox;
		 rcBoundBox.left = 0;
		 rcBoundBox.top = 0;
		 rcBoundBox.right = srcImg.GetWidth();
		 rcBoundBox.bottom = srcImg.GetHeight();
		 if(!image.empty())
		 {
			 if( !cascade.load( face_cascade_name ) )
			 { 
				 fprintf( stderr, "ERROR: Could not load classifier cascade\n" ); 
				 return; 
			 }
			 detectAndDisplay(image, &rcBoundBox); 
		 }

		 Align(reinterpret_cast<BYTE *>(lkData.Scan0), srcImg.GetWidth(), srcImg.GetHeight(), lkData.Stride, rcBoundBox, ptsPos1);
		 showAlignedFace(image, ptsPos1, ptsNum);
		 try
		 {
			 imwrite(resImageFile,image);
		 }
		 catch (runtime_error& ex) 
		 {
			 fprintf(stderr, "Exception saving aligned image: %s\n", ex.what());
			 return;
		 }
		 image.release();
		 //delete ptsPos1;
		 srcImg.UnlockBits(&lkData); 
		 DestroyAlign();
	 }
	 srcImageFile = "";
	 resImageFile = "";
 }

 //void ProcPicDir(CString strPicDir)
 //{
	// CFileFind fileFinder;

	// if (strPicDir.Right(1) == TEXT("\\"))
	// {
	//	 int nPos  = strPicDir.ReverseFind(TEXT('\\'));
	//	 strPicDir = strPicDir.Left(nPos);
	// }

	// CString strPicFile = TEXT("");
	// strPicFile.Format(TEXT("%s\\%s"),strPicDir,TEXT("*.*"));

	// BOOL bWorking = fileFinder.FindFile(strPicFile);
	// while (bWorking)
	// {   
	//	 bWorking = fileFinder.FindNextFile();
	//	 if (fileFinder.IsDots())
	//	 {
	//		 continue;
	//	 }

	//	 CString strFilePath = fileFinder.GetFilePath();
	//	 if (!fileFinder.IsDirectory())
	//	 {      
	//		 int nPos = strFilePath.ReverseFind(TEXT('.'));
	//		 CString strExt = strFilePath.Right(strFilePath.GetLength() - nPos - 1);
	//		 if (strExt.CompareNoCase(TEXT("bmp"))  == 0)
	//		 {   
	//			 procPic(strFilePath);

	//		 }
	//	 }
	// }
	// fileFinder.Close();
 //}

void detectAndDisplay(Mat& img, RECT* detectBox) 
{ 
    double scale=1.2;
	Mat small_img(cvRound (img.rows/scale), cvRound(img.cols/scale), CV_8UC1);
	Mat gray(cvRound (img.rows/scale),cvRound(img.cols/scale),8,1);
	cvtColor(img, gray, CV_BGR2GRAY);
	resize(gray, small_img, small_img.size(), 0, 0, INTER_LINEAR);
    equalizeHist(small_img,small_img); //直方图均衡

	//Detect the biggest face
	double t = (double)cvGetTickCount(); 
	std::vector<cv::Rect> faces; 
	cascade.detectMultiScale(small_img, faces, 1.1, 2, CV_HAAR_FIND_BIGGEST_OBJECT, cvSize(20, 20)); 
	t = (double)cvGetTickCount() - t; 
	printf( "detection time = %gms\n", t/((double)cvGetTickFrequency()*1000.) );

    //found objects and draw boxes around them 
	if(faces.size() == 1)
	{
		(*detectBox).left = faces[0].x * scale;
		(*detectBox).right = (faces[0].x + faces[0].width) * scale;		
		(*detectBox).top = faces[0].y * scale + 10;
		(*detectBox).bottom = (faces[0].y + faces[0].height) * scale + 10;
		rectangle(img, cvPoint((*detectBox).left,(*detectBox).top), cvPoint((*detectBox).right,(*detectBox).bottom), Scalar( 255, 0, 0 ));
		if(isShowFace)
		{
			namedWindow("image", CV_WINDOW_AUTOSIZE); //创建窗口
			imshow("image",img);
			cvWaitKey(0); 
			destroyWindow("image");
		}
	}
	else
	{
		printf("Fail to detect faces.\n");
	}
	return;
}

void showAlignedFace(Mat& img, float* ptsPos, int ptsNum)
{
	for(int i=0;i<ptsNum;i=i+2)
	{
		cv::Point point1(ptsPos[i], ptsPos[i+1]);
		cv::Point point2(ptsPos[i+2],ptsPos[i+3]);
		if(i<172)
		{
			circle(img, point1 ,1 , CV_RGB(255,0,0),2, 8, 0 );
			switch(i)
			{
			case 14:
			case 30:
				point2.x = ptsPos[i-14];
				point2.y = ptsPos[i-13];
				break;
			case 50:
			case 70:
				point2.x = ptsPos[i-18];
				point2.y = ptsPos[i-17];
				break;
			case 94:
				point2.x = ptsPos[i-22];
				point2.y = ptsPos[i-21];
				break;
			case 134:
				point2.x = ptsPos[i-38];
				point2.y = ptsPos[i-37];
			}
			line(img, point1, point2, CV_RGB(255,0,0));
		}
		else if(i == 172)
		{
			circle(img, point1 ,1 , CV_RGB(255,0,0),2, 8, 0 );
		}
		else
		{
			circle(img, point1 ,1 , CV_RGB(0,255,0),2, 8, 0 );
		}
	} 
	if(isShowFace)
	{
		namedWindow("image", CV_WINDOW_AUTOSIZE); //创建窗口
		imshow("image",img);
		cvWaitKey(0); 
		destroyWindow("image");
	}
}

void ORBmatching(float* posPoint)
{
	Mat img_1_Orb = imread("..\\Debug\\sheldon1.bmp");
	Mat img_2_Orb = imread("..\\Debug\\sheldon2.bmp");

	ORB orb;
	vector<KeyPoint> keyPoints_1_Orb, keyPoints_2_Orb;
	Mat descriptors_1_Orb, descriptors_2_Orb;
	keyPoints_1_Orb.resize(94);
	for(int i=0;i<94;i++)
	{
		keyPoints_1_Orb[i].pt.x = posPoint[2*i];
		keyPoints_1_Orb[i].pt.y = posPoint[2*i+1];
	}

	if( !cascade.load( face_cascade_name ) )
	{ 
		fprintf( stderr, "ERROR: Could not load classifier cascade\n" ); 
		return; 
	}
	RECT rcBox;
	detectAndDisplay(img_2_Orb, &rcBox); 
	int rcNum = (rcBox.right-rcBox.left) * (rcBox.bottom-rcBox.top);
	keyPoints_2_Orb.resize(rcNum);
	for(int i=rcBox.left, k=0; i<rcBox.right; i++)
	{
		for (int j=rcBox.top; j<rcBox.bottom; j++,k++)
		{
			keyPoints_2_Orb[k].pt.x = i;
			keyPoints_2_Orb[k].pt.y = j;
		}
	}

	double t = (double)cvGetTickCount(); 
	orb(img_1_Orb, Mat(), keyPoints_1_Orb, descriptors_1_Orb, true);
	orb(img_2_Orb, Mat(), keyPoints_2_Orb, descriptors_2_Orb, true);

	BruteForceMatcher<Hamming> matcher_Orb;
	vector<DMatch> matches_Orb;
	matcher_Orb.match(descriptors_1_Orb, descriptors_2_Orb, matches_Orb);

	t = (double)cvGetTickCount() - t; 
	printf( "matching time = %gms\n", t/((double)cvGetTickFrequency()*1000.) );
	//double max_dist = 0;
	//double min_dist = 100;
	////-- Quick calculation of max and min distances between keypoints
	//for (int i=0; i<descriptors_1_Orb.rows; i++)
	//{ 
	//	double dist = matches_Orb[i].distance;
	//	if (dist < min_dist) min_dist = dist;
	//	if(dist > max_dist) max_dist = dist;
	//}
	//printf("-- Max dist : %f \n", max_dist);
	//printf("-- Min dist : %f \n", min_dist);

	//-- Draw only "good" matches (i.e. whose distance is less than 0.6*max_dist )
	// PS.- radiusMatch can also be used here.

	//vector<DMatch> good_matches_Orb;
	//for (int i=0; i<descriptors_1_Orb.rows; i++)
	//{ 
	//	//if(matches_Orb[i].distance < 0.7*max_dist)
	//	{ 
	//		good_matches_Orb.push_back(matches_Orb[i]); 
	//	}
	//}
	//printf("Good matches countnum = %d \n", good_matches_Orb.size());

	std::vector<cv::Point> kp1,kp2;
	kp1.push_back(cvPoint(0,0));
	kp2.push_back(cvPoint(0,0));
	kp1.push_back(cvPoint(img_1_Orb.cols-1,0));
	kp2.push_back(cvPoint(img_2_Orb.cols-1,0));
	kp1.push_back(cvPoint(0,img_1_Orb.rows-1));
	kp2.push_back(cvPoint(0,img_2_Orb.rows-1));
	kp1.push_back(cvPoint(img_1_Orb.cols-1,img_1_Orb.rows-1));
	kp2.push_back(cvPoint(img_2_Orb.cols-1,img_2_Orb.rows-1));
	for(int i=0; i<93; i++)
	{
		if (i == 85)
		{
			continue;
		}
		DMatch k = matches_Orb[i];
		kp1.push_back(keyPoints_1_Orb[k.queryIdx].pt);
		kp2.push_back(keyPoints_2_Orb[k.trainIdx].pt);
		//printf("%d :\n", i);
		//printf("kp1:%d %d\n", kp1[i+4].x, kp1[i+4].y);
		//printf("kp2:%d %d\n", kp2[i+4].x, kp2[i+4].y);
	}
	
	CThinPlateSpline tps(kp1, kp2);
	Mat sketch1 = imread("wholesketch.jpg");
	Mat sketch2;
	
	double t2 = (double)cvGetTickCount(); 
	tps.warpImage(sketch1,sketch2,0.01,INTER_CUBIC,BACK_WARP);
	t2 = (double)cvGetTickCount() - t2; 
	printf( "warping time = %gms\n", t2/((double)cvGetTickFrequency()*1000.) );
	//Mat img_matches_Orb;

	//vector<DMatch> sub_matches_Orb;
	//for(int i=87;i<94;i++)
	//{
	//	sub_matches_Orb.push_back(matches_Orb[i]);
	//}

	//drawMatches(img_1_Orb, keyPoints_1_Orb, img_2_Orb, keyPoints_2_Orb,
	//	sub_matches_Orb, img_matches_Orb, Scalar::all(-1), Scalar::all(-1),
	//	vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

	 //display
	//imshow("MatchORB", img_matches_Orb);
	//for(int i=0; i<94;i++)
	//{
	//	circle(sketch2,kp2[i],1,Scalar(0,0,255),-1);
	//	circle(sketch1,kp1[i],1,Scalar(0,0,255),-1);
	//}
	imshow("sketch1", sketch1);
	imshow("sketch2", sketch2);
	waitKey(0);
}

int _tmain(int argc, _TCHAR* argv[])
{
	ULONG_PTR m_gdiplusToken; 
	GdiplusStartupInput gdiplusStartupInput;     //声明
	GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);    //启动

	string imageFilePath="";
	char imgPath[MAX_PATH];
	char imgDrive[_MAX_DRIVE];
	_splitpath(_pgmptr,imgDrive,imgPath,NULL,NULL);

	imageFilePath.append(imgDrive);
	imageFilePath.append(imgPath);
	imageFilePath.append("\sheldon1.bmp");

	float* posPoint = new float[188];
	procPic(imageFilePath.c_str(),posPoint);
	ORBmatching(posPoint);
	delete posPoint;
	return 0;
}