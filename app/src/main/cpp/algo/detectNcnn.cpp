//
// Created by admin on 2019/3/1.
//
#include "detectNcnn.h"
#include "Log.h"

DetectNcnn::DetectNcnn() {
    this->squeezenet.load_param("/mnt/sdcard/TME/models/dsm_ncnn/detect_deploy.param");
    this->squeezenet.load_model("/mnt/sdcard/TME/models/dsm_ncnn/detect_deploy.bin");
}

DetectNcnn::DetectNcnn(const string &model_file, const string &weights_file) {
    int a = this->squeezenet.load_param(model_file.c_str());
    int b = this->squeezenet.load_model(weights_file.c_str());
}

void DetectNcnn::detectObjNcnn(const Mat &img, vector<Rect> &objects, vector<int> &classes,
                               vector<float> &confidences) {
    //LOGE("........in detect.......");
    //ncnn::Net squeezenet;
    //squeezenet.load_param("/mnt/sdcard/Adas/models/CifarDetect/detect_deploy.param");
    //squeezenet.load_model("/mnt/sdcard/Adas/models/CifarDetect/detect_deploy.bin");

    const int target_size = 300;

    int img_w = img.cols;
    int img_h = img.rows;

    ncnn::Mat in = ncnn::Mat::from_pixels_resize(img.data, ncnn::Mat::PIXEL_BGR, img.cols, img.rows,
                                                 target_size, target_size);

    const float mean_vals[3] = {104.f, 117.f, 123.f};
    const float norm_vals[3] = {0.007843, 0.007843, 0.007843};//尺度
    in.substract_mean_normalize(mean_vals, norm_vals);

    ncnn::Extractor ex = this->squeezenet.create_extractor();

    ex.set_num_threads(4);

    ex.input("data", in);

    ncnn::Mat out;
    ex.extract("detection_out", out);

    int label;
    float score;
    Rect object;

    for (int i = 0; i < out.h; i++) {
        const float *values = out.row(i);

        label = (int) (values[0]);
        score = values[1];
        object.x = (int) (values[2] * img_w);
        object.y = (int) (values[3] * img_h);
        object.width = (int) (values[4] * img_w - object.x);
        object.height = (int) (values[5] * img_h - object.y);

        if (score > 0.28) {
            objects.push_back(object);
            classes.push_back(label);
            confidences.push_back(score);
        }
    }
}


DetectNcnnFace::DetectNcnnFace() {
    this->squeezenet.load_param("/mnt/sdcard/TME/models/face_slim/face.param");
    this->squeezenet.load_model("/mnt/sdcard/TME/models/face_slim/face.bin");
}

DetectNcnnFace::DetectNcnnFace(const string &model_file, const string &weights_file) {
    int a = this->squeezenet.load_param(model_file.c_str());
    int b = this->squeezenet.load_model(weights_file.c_str());
}

void DetectNcnnFace::detectObjNcnn(const Mat &img, vector<Rect> &objects, vector<int> &classes,
                                   vector<float> &confidences) {
    //ncnn::Net squeezenet;
    //squeezenet.load_param("/mnt/sdcard/Adas/models/CifarDetect/detect_deploy.param");
    //squeezenet.load_model("/mnt/sdcard/Adas/models/CifarDetect/detect_deploy.bin");

    const int target_size = 300;

    int img_w = img.cols;
    int img_h = img.rows;

    ncnn::Mat in = ncnn::Mat::from_pixels_resize(img.data, ncnn::Mat::PIXEL_BGR, img.cols, img.rows,
                                                 target_size, target_size);

    const float mean_vals[3] = {104.f, 117.f, 123.f};//均值
    const float norm_vals[3] = {0.007843, 0.007843, 0.007843};//尺度
    in.substract_mean_normalize(mean_vals, norm_vals);

    ncnn::Extractor ex = this->squeezenet.create_extractor();

    ex.set_num_threads(4);

    ex.input("data", in);

    ncnn::Mat out;
    ex.extract("detection_out", out);

    int label;
    float score;
    Rect object;

    for (int i = 0; i < out.h; i++) {
        const float *values = out.row(i);

        label = (int) (values[0]);
        score = values[1];
        object.x = (int) (values[2] * img_w);
        object.y = (int) (values[3] * img_h);
        object.width = (int) (values[4] * img_w - object.x);
        object.height = (int) (values[5] * img_h - object.y);

        if (score > 0.7) {
            objects.push_back(object);
            classes.push_back(label);
            confidences.push_back(score);
        }
    }
}