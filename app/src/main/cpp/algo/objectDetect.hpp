/*
 * BasicAlg.hpp
 *
 *  Created on: March 6, 2018
 *      Author: sgl
 */

#ifndef _OBJECT_DETECT_HPP_
#define _OBJECT_DETECT_HPP_

#include <caffe/caffe.hpp>

#include <algorithm>
#include <iomanip>
#include <iosfwd>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <fstream>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv/cv.h>

using namespace cv;
using namespace std;
using namespace caffe;


class ObjectDetector
{
 public:

	ObjectDetector();
	ObjectDetector(const string& model_file, const string& weights_file);
	void Detect(const Mat& img, vector<Rect>& objects, vector<int>& classes, vector<float>& confidences);

 private:

    void WrapInputLayer(vector<Mat>* input_channels);

    void Preprocess(const Mat& img, vector<Mat>* input_channels);

 private:

    boost::shared_ptr<Net<float> > net_;
    Size input_geometry_;
    int num_channels_;
    Scalar mean_;
};




#endif
