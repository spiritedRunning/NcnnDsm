//
// Created by admin on 2019/3/1.
//

#ifndef GUOBIAO_DETECTNCNN_H
#define GUOBIAO_DETECTNCNN_H
#include <string>
#include <ncnn/net.h>
#include "opencv2/opencv.hpp"

using namespace std;
using namespace cv;



class DetectNcnn {
public:
    DetectNcnn();
	~DetectNcnn()
	{}
	DetectNcnn(const string& model_file, const string& weights_file);

    void detectObjNcnn(const Mat& img, vector<Rect>& objects, vector<int>& classes, vector<float>& confidences);

private:
	ncnn::Net squeezenet;
};

class DetectNcnnFace {
public:
    DetectNcnnFace();

    ~DetectNcnnFace()
    {}

    DetectNcnnFace(const string& model_file, const string& weights_file);

    void detectObjNcnn(const Mat& img, vector<Rect>& objects, vector<int>& classes, vector<float>& confidences);

private:
    ncnn::Net squeezenet;
};
#endif //GUOBIAO_DETECTNCNN_H
