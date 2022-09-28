
#include "ObjectDetect.hpp"
#include "Log.h"

ObjectDetector::ObjectDetector() {

}

ObjectDetector::ObjectDetector(const string &model_file, const string &weights_file) {

#ifdef CPU_ONLY
    Caffe::set_mode(Caffe::CPU);
#else
    Caffe::set_mode(Caffe::GPU);
#endif

    /* Load the network. */
    net_.reset(new Net<float>(model_file, TEST));
    net_->CopyTrainedLayersFrom(weights_file);

    CHECK_EQ(net_->num_inputs(), 1) << "Network should have exactly one input.";
    CHECK_EQ(net_->num_outputs(), 1) << "Network should have exactly one output.";

    Blob<float> *input_layer = net_->input_blobs()[0];
    num_channels_ = input_layer->channels();
    CHECK(num_channels_ == 3 || num_channels_ == 1) << "Input layer should have 1 or 3 channels.";

    input_geometry_ = Size(input_layer->width(), input_layer->height());

    //mean_= Scalar(102.9801, 115.9465, 122.7717);
    //mean_= Scalar(127.5, 127.5, 127.5);
    mean_ = Scalar(104, 117, 123);
}

void ObjectDetector::Detect(const Mat &img, vector<Rect> &objects, vector<int> &classes,
                            vector<float> &confidences) {
    Blob<float> *input_layer = net_->input_blobs()[0];
    input_layer->Reshape(1, num_channels_, input_geometry_.height, input_geometry_.width);

    /* Forward dimension change to all layers. */
    net_->Reshape();

    vector<Mat> input_channels;
    WrapInputLayer(&input_channels);
    Preprocess(img, &input_channels);

    net_->Forward();

    /* Copy the output layer to a std::vector */
    Blob<float> *result_blob = net_->output_blobs()[0];
    const float *result = result_blob->cpu_data();
    const int num_det = result_blob->height();

    int label;
    float score;
    Rect object;
    for (int k = 0; k < num_det; ++k) {
        if (result[0] == -1) {
            // Skip invalid detection.
            result += 7;
            continue;
        }

        vector<float> detection(result, result + 7);
        result += 7;

        label = detection[1];
        score = detection[2];

        object.x = detection[3] * img.cols;
        object.y = detection[4] * img.rows;
        object.width = (detection[5] - detection[3]) * img.cols + 1;
        object.height = (detection[6] - detection[4]) * img.rows + 1;

        objects.push_back(object);
        classes.push_back(label);
        confidences.push_back(score);

    }

}

/* Wrap the input layer of the network in separate cv::Mat objects
 * (one per channel). This way we save one memcpy operation and we
 * don't need to rely on cudaMemcpy2D. The last preprocessing
 * operation will write the separate channels directly to the input
 * layer. */
void ObjectDetector::WrapInputLayer(vector<cv::Mat> *input_channels) {
    Blob<float> *input_layer = net_->input_blobs()[0];

    int width = input_layer->width();
    int height = input_layer->height();
    float *input_data = input_layer->mutable_cpu_data();
    for (int i = 0; i < input_layer->channels(); ++i) {
        Mat channel(height, width, CV_32FC1, input_data);
        input_channels->push_back(channel);
        input_data += width * height;
    }
}

void ObjectDetector::Preprocess(const Mat &img, vector<Mat> *input_channels) {
    /* Convert the input image to the input image format of the network. */
    Mat sample;
    if (img.channels() == 3 && num_channels_ == 1)
        cvtColor(img, sample, COLOR_BGR2GRAY);
    else if (img.channels() == 4 && num_channels_ == 1)
        cvtColor(img, sample, COLOR_BGRA2GRAY);
    else if (img.channels() == 4 && num_channels_ == 3)
        cvtColor(img, sample, COLOR_BGRA2BGR);
    else if (img.channels() == 1 && num_channels_ == 3)
        cvtColor(img, sample, COLOR_GRAY2BGR);
    else
        sample = img;

    Mat sample_resized;
    if (sample.size() != input_geometry_)
        resize(sample, sample_resized, input_geometry_);
    else
        sample_resized = sample;

    Mat sample_float;
    if (num_channels_ == 3)
        sample_resized.convertTo(sample_float, CV_32FC3);
    else
        sample_resized.convertTo(sample_float, CV_32FC1);

    Mat sample_normalized;
    subtract(sample_float, mean_, sample_normalized);

    //sample_normalized=sample_normalized * 0.017;
    //sample_normalized = sample_normalized * 0.007843;
    split(sample_normalized, *input_channels);

    CHECK(reinterpret_cast<float *>(input_channels->at(0).data) ==
          net_->input_blobs()[0]->cpu_data())
    << "Input channels are not wrapping the input layer of the network.";
}
