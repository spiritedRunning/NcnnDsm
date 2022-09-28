//
// Created by imx on 1/9/18.
//

#ifndef ALGO_CLASSIFIER_H
#define ALGO_CLASSIFIER_H

#define USE_OPENCV

#include <caffe/caffe.hpp>
#ifdef USE_OPENCV
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#endif  // USE_OPENCV
#include <algorithm>
#include <iosfwd>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace caffe;  // NOLINT(build/namespaces)
using std::string;

typedef std::pair<string, float> Prediction;

class Classifier {
public:
    Classifier();
	Classifier(const string& model_file, const string& trained_file,
			const string& mean_file, const string label_file[], int n);

	std::vector<Prediction> Classify(const cv::Mat& img, int N = 5);

private:
	void SetMean(const string& mean_file);

	std::vector<float> Predict(const cv::Mat& img);

	void WrapInputLayer(std::vector<cv::Mat>* input_channels);

	void Preprocess(const cv::Mat& img, std::vector<cv::Mat>* input_channels);

private:
	boost::shared_ptr<Net<float> > net_;
	cv::Size input_geometry_;
	int num_channels_;
	cv::Mat mean_;
	std::vector<string> labels_;
};

#endif //ALGO_CLASSIFIER_H
