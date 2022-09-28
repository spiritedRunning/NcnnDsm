#include <caffe/caffe.hpp>
#include <fstream>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <omp.h>
#include <boost/shared_ptr.hpp>
#include <cblas.h>
using namespace caffe;
using namespace std;
using namespace cv;

//#define CAFFENET_MODEL
#define GLASS_LENET_MODEL
//#define OPENCLOSE_LENET_MODEL

//omp
const int threads_num = 2;

//pnet config
const float pnet_stride = 2;
const float pnet_cell_size = 12;
const int pnet_max_detect_num = 5000;

//mean & std
const float mean_val = 127.5f;
const float std_val = 0.0078125f;

//minibatch size
const int step_size = 128;
const float step_size_float = 0.0078125f;

typedef struct FaceBox
{
	float xmin;
	float ymin;
	float xmax;
	float ymax;
	float score;
} FaceBox;

typedef struct FaceInfo
{
	float bbox_reg[4];
	float landmark_reg[10];
	float landmark[10];
	FaceBox bbox;
} FaceInfo;

class MTCNN
{
public:
	MTCNN();
	MTCNN(const string& proto_model_dir);
	vector<FaceInfo> Detect(const cv::Mat& img, const int min_size, const float* threshold, const float factor, const int stage);
	vector<FaceInfo> FindPoint(const cv::Mat& img, Rect roi, const int min_size, const float* threshold, const float factor, const int stage);
	void FindEyeMouth(const cv::Mat& img, vector<FaceInfo>, Rect leftEye, Rect rightEye, Rect mouth);

protected:
	vector<FaceInfo> ProposalNet(const cv::Mat& img, int min_size, float threshold, float factor);
	vector<FaceInfo> NextStage(const cv::Mat& image, vector<FaceInfo> &pre_stage_res, int input_w, int input_h,
		int stage_num, const float threshold);

	void BBoxRegression(vector<FaceInfo>& bboxes);
	void BBoxPadSquare(vector<FaceInfo>& bboxes, int width, int height);
	void BBoxPad(vector<FaceInfo>& bboxes, int width, int height);
	void GenerateBBox(Blob<float>* confidence, Blob<float>* reg_box, float scale, float thresh);
	std::vector<FaceInfo> NMS(std::vector<FaceInfo>& bboxes, float thresh, char methodType);
	float IoU(float xmin, float ymin, float xmax, float ymax, float xmin_, float ymin_, float xmax_, float ymax_, bool is_iom = false);

private:
	boost::shared_ptr<Net<float>> PNet_;
	boost::shared_ptr<Net<float>> RNet_;
	boost::shared_ptr<Net<float>> ONet_;

	std::vector<FaceInfo> candidate_boxes_;
	std::vector<FaceInfo> total_boxes_;
};
