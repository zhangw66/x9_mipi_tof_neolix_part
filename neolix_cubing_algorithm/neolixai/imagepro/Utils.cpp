#include"Utils.h"
#include <opencv2/imgproc/imgproc.hpp>
#include<opencv2/core/core.hpp>
#include <opencv2//opencv.hpp>
#include <vector>
#include "../imagepro/CalDepth.h"
#include "../driverdecorate/base.h"
#include"../pointcloud/pointcloud.hpp"
#include<string>
#include <sstream>
#include <fstream>

#include<cmath>
namespace neolix{

    extern cv::Mat g_plane_para_least;
    extern cv::Mat g_plane_ransca;
	extern cv::Mat g_rect_para;
	extern cv::Mat g_plane_rect;

    template <class Type>
    Type stringToNum(const string& str)
    {
        istringstream iss(str);
        Type num;
        iss >> num;
        return num;
    }
   static std::vector<std::string> split(const std::string &s, const std::string &seperator){
      std::vector<std::string> result;
      typedef std::string::size_type string_size;
      string_size i = 0;

      while(i != s.size()){
        //�ҵ��ַ������׸������ڷָ�������ĸ��
        int flag = 0;
        while(i != s.size() && flag == 0){
          flag = 1;
          for(string_size x = 0; x < seperator.size(); ++x)
          if(s[i] == seperator[x]){
            ++i;
            flag = 0;
            break;
          }
        }

        //�ҵ���һ���ָ������������ָ���֮����ַ���ȡ����
        flag = 0;
        string_size j = i;
        while(j != s.size() && flag == 0){
          for(string_size x = 0; x < seperator.size(); ++x)
          if(s[j] == seperator[x]){
            flag = 1;
            break;
          }
          if(flag == 0)
          ++j;
        }
        if(i != j){
          result.push_back(s.substr(i, j-i));
          i = j;
        }
      }
      return result;
    }

 bool readTunePara(std::string path, double para[])
 {
     std::ifstream ifile;
     ifile.open(path);
     if(!ifile.is_open()) return false;
     //���ļ��ж�ȡ��������������Ϻ���������ÿ�����̵�ϵ��������
     std::vector<std::string> result;
     std::string str;
     for(int i = 0; i < 3 ; i++)
     {
         if(!std::getline(ifile,str)) return false;
         result.clear();
         result = split(str," ");
         if(result.size() != 3) return false;
         para[3*i + 0] = stringToNum<double>(result[0]);
         para[3*i + 1] = stringToNum<double>(result[1]);
         para[3*i + 2] = stringToNum<double>(result[2]);
     }
     return true;
 }

void LaplasSharp(const cv::Mat &src, cv::Mat &dest)
{
	cv::Mat tmp = src.clone();
	dest.create(src.size(),src.type());
	for (int row = 1; row < src.rows-1; row++)
	{
		const uchar* previous  = tmp.ptr<const uchar>(row-1);
		const uchar* current   = tmp.ptr<const uchar>(row);
		const uchar* next      = tmp.ptr<const uchar>(row+1);

		uchar *result = dest.ptr<uchar>(row);
		for (int col = 1; col < src.cols-1; col++)
		{
			*result++ = cv::saturate_cast<uchar>(
				5*current[col]-current[col-1]
				-current[col+1]-previous[col]-next[col]
				);
		}
	}
	dest.row(0).setTo(cv::Scalar(0));
	dest.row(dest.rows - 1).setTo(cv::Scalar(0));
	dest.col(0).setTo(cv::Scalar(0));
	dest.col(dest.cols -1).setTo(cv::Scalar(0));
}


void GammaConver(const cv::Mat &src, cv::Mat &dest, double gamma)
{
	cv::Mat X;
	src.convertTo(X,CV_32FC1);
	cv::pow(X,gamma,dest);
	cv::normalize(dest,dest,255,0,cv::NORM_MINMAX,CV_8UC1);
}

void ExtractObject(const cv::Mat &depthImage, cv::Mat &BinaryImage)
{
	cv::Mat temp = cv::Mat::ones(depthImage.size(),CV_8UC1);
	temp = 255*temp;
	BinaryImage = depthImage.clone();
	//��һ��[0,255]
	cv::normalize(BinaryImage,BinaryImage,255,0,cv::NORM_MINMAX,CV_16SC1);

	//ת������
	BinaryImage.convertTo(BinaryImage,CV_8UC1);
	//���⻯
	cv::equalizeHist(BinaryImage,BinaryImage);
	//���ط�ת
	cv::subtract(temp,BinaryImage,BinaryImage);
	//��߶Աȶ�
	BinaryImage.convertTo(BinaryImage,-1,0.8,-25);
	//��ֵ�˲�
	cv::medianBlur(BinaryImage,BinaryImage,5);
	//��
	LaplasSharp(BinaryImage,BinaryImage);
	GammaConver(BinaryImage,BinaryImage,1/0.4);
	//��̬ѧ����
	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT,cv::Size(5,5));
	cv::morphologyEx(BinaryImage,BinaryImage,cv::MORPH_OPEN,element);
	//��ֵ��
	cv::threshold(BinaryImage,BinaryImage,250,255,cv::THRESH_OTSU);
}
void readDepthData(cv::Mat &dest, const size_t rows, const size_t cols,const string RawPath)
{
	//��raw�������ļ�������
	dest.create(rows,cols,CV_16UC1);
	std::ifstream inFile(RawPath, std::ios::in|std::ios::binary);
	unsigned char upBit;
	unsigned char downBit;
	char *buf = new char[rows*2];
	for (size_t col = 0; col < cols; col++)
	{
		//auto rowPtr = dest.ptr<float>(col);
		//cout<<sizeof(float)<<endl;
		inFile.read(buf,rows*2);

		for (size_t row = 0; row < rows*2; row += 2)
		{
			unsigned short dept = 0;
			downBit = buf[row];
			upBit = buf[row+1];
			dept = (int)downBit;
			dept += (int)upBit*256;
			dest.at<short>(row/2,col) = dept;
			//cout<<dest.at<float>(row/2,col)<<"== "<<dept<<"  ";
		}
		cout<<endl;
	}
	inFile.close();
	delete[] buf;

}

void onMouse(int event, int x ,int y, void* img)
{
	cv::Mat *image = (cv::Mat*)img;
	if(event == CV_EVENT_LBUTTONDOWN)
	{
		std::cout<<"x = "<<x
			<<" y = "<<y<<endl<<
			"depth="<<image->at<ushort>(y,x)<<endl;
	}

}

unsigned short calculateDepthFromDepthImagInRangeCountour(cv::Mat &depthIamge, std::vector<cv::Point> &contour,double &confidence,unsigned short &mindepth)
{
	cv::Mat shortIamge;
	std::vector<short> depthPoints;
	if (depthIamge.depth() != CV_16UC1)
	{
		depthIamge.convertTo(shortIamge,CV_16UC1);
	}else
	{
		shortIamge = depthIamge.clone();
	}

	for (int row = 0; row < shortIamge.rows; row++)
	{
		auto rowPtr = shortIamge.ptr<short>(row);
		for (int col =0; col < shortIamge.cols; col++)
		{
			if(cv::pointPolygonTest(contour, cv::Point(col,row),false) == 1) depthPoints.push_back(rowPtr[col]);
		}
	}

	caldepth depthHist(&depthPoints);
	depthHist.calDepthHist();
    unsigned short d =    depthHist.getdepth(confidence);
  //  unsigned short d2 = depthHist.getMa
    mindepth = depthHist.getMinDepth();
	///std::cout << "max depth" << depthHist.getMaxDepth() << std::endl;
	return d;
}

unsigned short calculateDepthFromDepthImagOutRangeCountour(cv::Mat &depthIamge, std::vector<cv::Point> &contour,double &confidence)
{
	cv::Mat shortIamge;
	std::vector<short> depthPoints;
	if (depthIamge.type() != CV_16UC1)
	{
		depthIamge.convertTo(shortIamge,CV_16UC1);
	}else
	{
		shortIamge = depthIamge.clone();
	}

	for (int row = 0; row < shortIamge.rows; row++)
	{
		auto rowPtr = shortIamge.ptr<short>(row);
		for (int col =0; col < shortIamge.cols; col++)
		{
			if(cv::pointPolygonTest(contour, cv::Point(col,row),false) != 1) depthPoints.push_back(rowPtr[col]);
		}
	}

	caldepth depthHist(&depthPoints);
	depthHist.calDepthHist();
	return depthHist.getdepth(confidence);

}

bool adjustSystem( cv::Mat depthIamgeRoi,  cv::Mat mask, unsigned short &distance )
{
	std::vector<short> vaildDepth;
	cv::Mat shortImage;
	if (depthIamgeRoi.type() != CV_16UC1)
	{
		depthIamgeRoi.convertTo(shortImage, CV_16UC1);
	}else
	{
		shortImage = depthIamgeRoi.clone();
	}

    short* ptr = shortImage.ptr <short>();

	for (int index = 0; index < shortImage.size().area(); index++)
	{
		if (0 != ptr[index])
		{
			vaildDepth.push_back(ptr[index]);
		}
	}
	cv::Mat vailDepthMatix(vaildDepth);
	cv::Mat mean, stddev;
	cv::meanStdDev(vailDepthMatix,mean,stddev);
	//�������ƽ̨��������ľ����ı�׼�����һ����ֵ����Ϊ�ǲ���ƽ̨������ͷ���Ǵ�ֱ�������ǲ���ƽ̨�������壬���У��ʧ�ܣ����ء���������ϵͳ����У��
	if(MEADSURING_TABLE_STDDEV < stddev.at<double>(0,0)  )  return false;

    if(mask.rows == shortImage.rows && mask.cols == shortImage.cols && mask.channels() == shortImage.channels())
    {
        double confidence;
        std::vector<short> histData;
        short* depth_ptr = shortImage.ptr<short>();
        uchar* mask_ptr  = mask.ptr<uchar>();
        for(int index =0; index < shortImage.size().area(); index++) if(mask_ptr[index] != 0) histData.push_back(depth_ptr[index]);
        caldepth depthHist(&histData);
        depthHist.calDepthHist();
        distance = depthHist.getdepth(confidence);
    }else return false;
	return true;
}


//ͨ�������ɫ���жϲ���ƽ̨��ƽ�棬��ȡ����
void padDepthMask(const cv::Mat colorDepthImage, cv::Mat &mask)
{
    cv::Mat hsv_img;
    cv::cvtColor(colorDepthImage, hsv_img, CV_RGB2HSV);
    int h_min=52, s_min=11, v_min=28;
	int h_max=109, s_max=255, v_max=255;
	cv::Scalar hsv_min(h_min, s_min, v_min);
	cv::Scalar hsv_max(h_max, s_max, v_max);
	mask = Mat::zeros(colorDepthImage.rows, colorDepthImage.cols, CV_8UC3);
	inRange(hsv_img, hsv_min, hsv_max, mask);
}

void distancesFromCamToPad(std::vector<short> &distances,std::vector<index_value<int, cv::Point2i>> &centerPoints,  cv::Mat PadDepthImage)
{
    //���в�ֳ����Σ�
    if(PadDepthImage.rows <3 || PadDepthImage.cols < 3) throw "the padDepthImage cols and rows must more than 3!";
    cv::Mat shortImage;
    if(PadDepthImage.type() != CV_16UC1)
    {
        PadDepthImage.convertTo(shortImage,CV_16UC1);
    }else
    {
        shortImage = PadDepthImage;

    }

    /*
    *   0  1  2  3  4  5  6  7  8
    * 0 *  *  *  *  *  *  *  *  * rs0
    * 1 *  @  *  *  @  *  *  @  *
    * 2 *  *  *  *  *  *  *  *  * re0
    * 3 *  *  *  *  *  *  *  *  * cs1
    * 4 *  @  *  *  @  *  *  @  *
    * 5 *  *  *  *  *  *  *  *  * re1
    * 6 *  *  *  *  *  *  *  *  * rs2
    * 7 *  @  *  *  @  *  *  @  *
    * 8 *  *  *  *  *  *  *  *  * re2
    *  cs0   ce0,cs1  ce1,cs2  ce2
    *
    *  rs0 = 0; re0 = (rows+1)/3-1; rs1 = re0+1;re1 = (rows+1)*2/3 -1;rs2 = re1+1; re2 = rows;
    *  cs0 = 0; ce0 = (cols+1)/3-1; cs1 = ce0+1;ce1 = (cols+1)*2/3 -1;cs2 = ce1+1; ce2 = cols;
    */
    std::vector<int> rowPoints;
    std::vector<int> colPoints;
    std::vector<short> depthPoints;
    double confidence;

    int rectWidth,rectHeight;
    int index = 0;
    cv::Mat roi;
    rowPoints.resize(6);
    colPoints.resize(6);
    distances.clear();
    centerPoints.clear();

    rowPoints[0] = 0;
    rowPoints[5] = PadDepthImage.rows-1;
    rowPoints[1] = (PadDepthImage.rows+1)/3-1;
    rowPoints[2] = rowPoints[1]+1;
    rowPoints[3] = (PadDepthImage.rows+2)*2/3-1;
    rowPoints[4] = rowPoints[3]+1;

    colPoints[0] = 0;
    colPoints[5] = PadDepthImage.cols-1;
    colPoints[1] = (PadDepthImage.cols+1)/3-1;
    colPoints[2] = colPoints[1]+1;
    colPoints[3] = (PadDepthImage.cols+1)*2/3-1;
    colPoints[4] = colPoints[3]+1;

    for(int colPoint = 0; colPoint < 6; colPoint+=2)
    {
        for(int rowPoint = 0; rowPoint < 6; rowPoint+=2)
        {
            rectWidth  = colPoints[colPoint+1] - colPoints[colPoint]+1;
            rectHeight = rowPoints[rowPoint+1] - rowPoints[rowPoint]+1;
            cv::Rect box(colPoints[colPoint],rowPoints[rowPoint],
                         rectWidth,rectHeight);
            //������������
            cv::Point2i center(colPoints[colPoint]+ rectWidth/2, rowPoints[rowPoint] + rectHeight/2);
            index_value<int, cv::Point2i> iv;
            iv.index = index++;
            iv.value = center;
            centerPoints.push_back(iv);
            //��������ͷ��������ľ���
            roi = shortImage(box);
            depthPoints.clear();
            short* ptr = roi.ptr < short>();
            for (int index = 0; index < roi.size().area(); index++)
            {
                depthPoints.push_back(ptr[index]);
            }
            caldepth depthHist(&depthPoints);
            depthHist.calDepthHist();
            unsigned short dis = depthHist.getdepth(confidence);
            distances.push_back(dis);
        }
    }
}

//��������֮��ľ���
float distancebet2Point2i(cv::Point2i point1, cv::Point2f point2)
{
    float diffx   = static_cast<float>(point1.x - point2.x);
    float diffy   = static_cast<float>(point1.y - point2.y);
    float diffpow = std::pow(diffx,2)+std::pow(diffy,2);
    return sqrt(diffpow);

}

float calDisCam2Pad(std::vector<float> &distances, std::vector<index_value<int,cv::Point2i>> &centerPoints, cv::Point2f point)
{
    float mindis = distancebet2Point2i(centerPoints[0].value,point);
    int minindex = centerPoints[0].index;
    for(size_t i = 1; i < centerPoints.size(); i++)
    {
        if(mindis > distancebet2Point2i(centerPoints[i].value,point))
        {
            mindis = distancebet2Point2i(centerPoints[i].value,point);
            minindex = centerPoints[i].index;
        }
    }
    return distances[minindex-1];
}

//ͨ��Ѱ���������������Բ����ȡ����������
void calCoutousCenter(std::vector<cv::Point> &contour, cv::Point2f &center)
{
    float radius = 0.0;
    cv::minEnclosingCircle(contour,center,radius);
}

//int leastSquareEquationFordepthCloud(pointcloud<double> *depthPoints, cv::Mat &dest)
//{
//    int left_x = g_plane_rect.at<int>(0, 0);//
//    int left_y = g_plane_rect.at<int>(1, 0);

//    double x_square = 0;
//    double x_y      = 0;
//    double y_square = 0;
//    double x_sum    = 0;
//    double y_sum    = 0;
//    double x_z      = 0;
//    double y_z      = 0;
//    double z_sum    = 0;
//    double n        = 0;

//    double x        = 0;
//    double y        = 0;
//    double z        = 0;

//    cv::Mat EquaMat_L,EquaMat_R;
//    int size_ = depthPoints->getSize();
//    if(0 >= size_) return -1;
//    for(int i = 0; i < size_; i++)
//    {
//        x = (*depthPoints)[i][0];
//        y = (*depthPoints)[i][1];
//        z = (*depthPoints)[i][2];

//        x_square += ((x + left_x) * (x + left_x));
//        y_square += ((y + left_y) * (y + left_y));
//        n++;
//        x_y += ((x + left_x) * (y + left_y));
//        x_z += ((x + left_x) * z);
//        y_z += ((y + left_y) * z);
//        x_sum += x + left_x;
//        y_sum += y + left_y;
//        z_sum += z;
//    }
//    EquaMat_L = (cv::Mat_<double>(3,3) << x_square,x_y,x_sum,x_y,y_square,y_sum,x_sum,y_sum,n);
//    EquaMat_R = (cv::Mat_<double>(3,1) << x_z,y_z,z_sum);
//    cv::solve(EquaMat_L,EquaMat_R,dest,DECOMP_SVD);
//    return 1;
//}

int leastSquareEquationForPointCloud(pointcloud<double> *point, cv::Mat &dest)
{
    double x_square = 0;
    double x_y      = 0;
    double y_square = 0;
    double x_sum    = 0;
    double y_sum    = 0;
    double x_z      = 0;
    double y_z      = 0;
    double z_sum    = 0;
    double n        = 0;

    double x        = 0;
    double y        = 0;
    double z        = 0;

    cv::Mat EquaMat_L,EquaMat_R;
    int size_ = point->getSize();
    if(0 >= size_) return -1;
    for(int i = 0; i < size_; i++)
    {
        x = (*point)[i][0];
        y = (*point)[i][1];
        z = (*point)[i][2];

        x_square += ((x ) * (x ));
        y_square += ((y ) * (y ));
        n++;
        x_y += ((x ) * (y ));
        x_z += ((x ) * z);
        y_z += ((y ) * z);
        x_sum += x ;
        y_sum += y ;
        z_sum += z;
    }
    EquaMat_L = (cv::Mat_<double>(3,3) << x_square,x_y,x_sum,x_y,y_square,y_sum,x_sum,y_sum,n);
    EquaMat_R = (cv::Mat_<double>(3,1) << x_z,y_z,z_sum);
    if(!cv::solve(EquaMat_L,EquaMat_R,dest,DECOMP_SVD)) return -1;
    return 1;
}

int leastSquareEquation(const cv::Mat depthImage, cv::Mat &dest)
{
   // std::vector<short> vaildDepth;
	cv::Mat shortImage;
	if (depthImage.type() != CV_16SC1)
	{
		depthImage.convertTo(shortImage, CV_16SC1);
	}else
	{
		shortImage = depthImage.clone();
	}

    int left_x = g_plane_rect.at<int>(0, 0);//
    int left_y = g_plane_rect.at<int>(1, 0);
    double x_square = 0;
    double x_y      = 0;
    double y_square = 0;
    double x_sum    = 0;
    double y_sum    = 0;
    double x_z      = 0;
    double y_z      = 0;
    double z_sum    = 0;
    double n        = 0;
    cv::Mat EquaMat_L,EquaMat_R;
    for (int row = 0; row < shortImage.rows; row++)
	{
		auto rowPtr = shortImage.ptr<short>(row);
		for (int col =0; col < shortImage.cols; col++)
		{
            if(rowPtr[col] != 0)
            {
                 x_square += ( (col + left_x) * ( col + left_x));
				 y_square += ( (row + left_y) * ( row + left_y));
				 n++;
				 x_y      += ( (col + left_x) * (row + left_y));
				 x_z      += ( (col + left_x) * rowPtr[col]);
				 y_z      += ( (row + left_y) * rowPtr[col]);
				 x_sum    += col + left_x;
				 y_sum    += row + left_y;
                 z_sum    += rowPtr[col];
                 /*
				x_square += ((col ) * (col ));
				y_square += ((row ) * (row ));
				n++;
				x_y += ((col ) * (row ));
				x_z += ((col ) * rowPtr[col]);
				y_z += ((row ) * rowPtr[col]);
				x_sum += col ;
				y_sum += row ;
                z_sum += rowPtr[col];*/
            }

		}
	}
	if(n < 3)
    {
       // std::cout<<"��С����������ϣ�������Ҫ���������㣬��������������3�����˳�";
		return -1;
    }
    EquaMat_L = (cv::Mat_<double>(3,3) << x_square,x_y,x_sum,x_y,y_square,y_sum,x_sum,y_sum,n);
    EquaMat_R = (cv::Mat_<double>(3,1) << x_z,y_z,z_sum);
    cv::solve(EquaMat_L,EquaMat_R,dest,DECOMP_SVD);
    return 1;

}

void Getavepaddis(std::vector<cv::Point2f> point ,const double a[],double &paddis)
{
	/**
	*ƽ�淽�� z = x*a[0] + y *a[1] + a[2]
	*/
	/*
	*2017.12.18.debugu:��ƽ�������ϵͳһ�����ƽ��������ͷX-Y�治ƽ�е�����
	*/
    int left_x = g_rect_para.at<int>(0, 0);
    int left_y = g_rect_para.at<int>(1, 0);
	paddis = 0;
	double high;
	for(size_t i=0 ; i<point.size();i++)
	{
        high = a[0]*(point[i].x + left_x )+a[1]*(point[i].y + left_y)+a[2];
        //high = a[0] * (point[i].x ) + a[1] * (point[i].y ) + a[2];
		paddis+=high;
	}
	paddis = paddis/point.size();
}

void LUT( cv::Mat depth1, ushort table[], cv::Mat &depth2)
{
	if( CV_16U != depth1.type()) throw "neolix LUT required ushort or short";
	size_t pixs = depth1.size().area();
	depth2.create(depth1.rows, depth1.cols, CV_16U);
	ushort *src = depth1.ptr<ushort>();
	ushort *dst = depth2.ptr<ushort>();
	for(size_t i = 0 ; i < pixs; i++)
	{
		dst[i] = table[src[i]];
	}
}

int selectedDepthPointFromDepthImage(pointcloud<double> &depthPoints, cv::Mat &depthImage, cv::Rect maxRect, cv::Rect minRect)
{
    cv::Mat shortImage;

    if(maxRect.area() < minRect.area()) return -1;


    if (depthImage.type() != CV_16SC1)
    {
        depthImage.convertTo(shortImage, CV_16SC1);
    }else
    {
        shortImage = depthImage.clone();
    }

    cv::Point2i point;
    depthPoints.resize(maxRect.area(),maxRect.area());
    unsigned int pointcloudIndex = 0;

    for(int row = 0; row < shortImage.rows; row++)
    {
        auto rowPtr = shortImage.ptr<short>(row);
        for(int col = 0; col < shortImage.cols; col++)
        {
            point.x = col;
            point.y = row;
            if(maxRect.contains(point) && (!minRect.contains(point)) && rowPtr[col] > 0)
            {
                depthPoints.data[pointcloudIndex][0] = col;
                depthPoints.data[pointcloudIndex][1] = row;
                depthPoints.data[pointcloudIndex][2] = rowPtr[col];
               // std::cout<<rowPtr[col]<<std::endl;
                pointcloudIndex++;

            }
        }
    }

    depthPoints.onlyModifySzie(pointcloudIndex);
    return 1;

}

}