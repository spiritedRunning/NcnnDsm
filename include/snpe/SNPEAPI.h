#ifndef SNPEAPI_H
#define SNPEAPI_H

#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "snpe/SNPE.hpp"

struct DetObjInf
{
    int label;
    float conf;
    cv::Rect_<int> rect;
};

class SNPEAPI
{
public:
    SNPEAPI(std::string dlcFile,int _resizeWidth, int _resizeHeight, int _channels);
    std::vector<DetObjInf> SNPEDetAPI(const cv::Mat& img);
    ~SNPEAPI();

private:
    std::unique_ptr<zdl::SNPE::SNPE> snpe;
    cv::Scalar meanScalar = cv::Scalar(102.9801, 115.9465, 122.7717);
    float norm = 0.017;
    int resizeWidth;
    int resizeHeight;
    int channels;
};

#endif //SNPEAPI_H