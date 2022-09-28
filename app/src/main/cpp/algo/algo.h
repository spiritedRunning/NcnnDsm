//
// Created by admin on 2018/3/19.
//

#ifndef ADAS_DWSAPP_ALGO_H_H
#define ADAS_DWSAPP_ALGO_H_H

#include "opencv2/opencv.hpp"
#include "mtcnn/mtcnn.h"
#include "kcf/kcftracker.hpp"
#include "../algo_native.h"
#include "../Log.h"
#include "classifier.h"
#include "objectDetect.hpp"
#include "detectNcnn.h"
#include <deque>

using namespace cv;
using namespace std;

//报警类型
enum ALARM_TYPE {
    NRL_FACE = 0x00, //检测到人脸
    NO_FACE = 0x01, //未检测到脸
    IMAGE_TYPE_ALARM = 0x02, //输入图像类型错误
    EYES_ALARM = 0x10,//闭眼报警
    YAWN_ALARM = 0x11,//打哈欠报警
    CALL_ALARM = 0x12,//打电话报警
    SMOKE_ALARM = 0x13,//抽烟报警
    ATTEN_ALARM = 0x14, //分神
    ABNORMAL_ALARM = 0x15,//驾驶员异常
    INFRARED_ALARM = 0x16,//红外阻断
    HAND_OFF_ALARM = 0x20, //脱离方向盘
    COVER_ALARM = 0x1d,//遮挡报警
};

//报警类型，用于报警队列
enum {
    EYE_ACTION_TYPE = 0x01, //闭眼
    YAWN_ACTION_TYPE = 0x02, // 打哈欠
    ATTEN_ACTION_TYPE = 0x03, // 注意力分散
    SMOKE_CLASSIFY_TYPE = 0x04, // 脱离方向盘
    CALL_CLASSIFY_TYPE = 0x05, // 打手机
    FACE_ACTION_TYPE = 0x06,
    PHONE_TYPE = 0x07,         //手+手机
    CIGARETTE_TYPE = 0x08,     //手+烟
    BLINK_TYPE = 0x09,         //眨眼
    BLOCKGLASS_TYPE = 0x10,    //红外阻断
    COVER_TYPE = 0x11,
    WHEEL_TYPE=0x12   //方向盘
};

enum ObjectType {
    EYE = 1,
    CLOSED = 2,
    HAND = 3,
    PHONE = 4,
    CIGARETTE = 5,
    MOUTH = 6,
    BELT = 7
};

enum FaceMask{
    FACE = 1,
    MASK = 2
};

//报警阈值设置
typedef struct {
    //单帧图像分类阈值
    float callClassifyScore=0.9;
    float smokeClassifyScore=0.9;
    float yawnClassifyScore=0.9;
    float attenRightScore=0.9;
    float attenLeftScore=0.9;
    float attenUpScore=0.9;
    float attenDownScore=0.9;
    float eyeClassifyScore=0.7;
    float normalScore=0.9;

    //检测阈值
    float callConfidence=0.3;
    float smokeConfidence=0.3;
    float yawnConfidence=0.4;
    float eyeConfidence=0.4;
    int smokeDot=1;

    //报警队列统计时长（单位：秒）
    int smokeClassifyLevel=3;
    int callClassifyLevel=4;
    int attenLevel=4;
    int yawnLevel=3;
    int eyeLevel=3;
    int nopLevel=5;//没有脸

    //报警队列报警阈值
    float smokeClassifyDeqConf=0.8;
    float callClassifyDeqConf=0.8;
    float attenDeqConf=0.95;
    float yawnDeqConf=0.9;
    float eyeDeqConf=0.8;
} stAlgoParam;

//int demoMode = 0;//演示模式(1),正常模式(0)

class Algo {
public:
    Algo();

    int alarmFlag = 0;

    int init(int fomat, int width, int height);
    int initActionPool();

    int detectFrame(cv::Mat &algoMat, int &demoMode);
    int release();

    int FaceCheckNcnn(Mat frame, float confidence);
    int SmokeCheckNcnn(Mat frame, float confidence, cv::Rect ROIRect);
    int CallCheckNcnn(Mat frame, float confidence, cv::Rect ROIRect);
    int MouthCheckNcnn(Mat frame, float confidence, cv::Rect ROIRect);
    int LRCheckNcnn(Mat frame, float confidence, cv::Rect ROIRect);
    int CloseEyeCheckNcnn(Mat frame, float confidence, cv::Rect ROIRect);
    int InfraredCheck(Mat frame, float confidence, cv::Rect ROIRect);

    void AddAction(int flag, int num);
    int CheckActionResult(int flag);
    void ClearActionResult(int flag);
    void ClearAllResult();

    void setAlgoParam(stAlgoParam param);

    stAlgoParam algoParam;   //param

//private:
    int imageType = 0;//1-720(width)x1280(height)  2-1280(width)x720(height)
    int frameIdx = 0;
    int ERR_CNT = 25;
    int errCnt = 0;

    int tmpWidth = 0;
    int tmpHeight = 0;

    int frameWidth;
    int frameHeight;

    Mat mainframe;
    Mat smallFrame;     //小图，算法处理
    int smallTimes = 4; //大图缩小倍数
    int rstFlag = 0;

    Mat detectSquareImg;//检测图片
    Rect detectSquareRect;//检测图片框

    MTCNN faceDetector;
    int minSize = 40; //MTCNN最小变化尺度
    float factor = 0.709f;
    float threshold[3] = {0.7f, 0.6f, 0.6f};
    Rect faceRect; //检测人脸目标

    KCFTracker tracker;
    int trackFlag = 0;
    Rect kcfResult; //跟踪结果


    typedef struct {
        int dataSize; //队列大小
        int alarmCnt; //报警阈值
        vector<int> action; // 动作标记
    } stActionCnt;

    //报警队列
    stActionCnt *pAction;
    stActionCnt stEyeAction;
    stActionCnt stYawnAction;
    stActionCnt stAttenAction;
    stActionCnt stSmokeClassify;
    stActionCnt stCallClassify;
    stActionCnt stFaceAction;
    stActionCnt stPhone; //手+手机
    stActionCnt stCigarette;//手+烟
    stActionCnt stBlink; //眨眼
    stActionCnt stBlockGlass;//红外阻断
    stActionCnt stCover;
    stActionCnt stWheel;

    Mat classifyFrame;
    float predictScore;
    string predictText;
    Classifier classifierEye;//睁闭眼分类器
    Classifier classifierAction; //抽烟打电话分类器
    Classifier classifierMouth; //嘴巴分类器
    Classifier classifierAtten;

    int faceOrMask=0;//0-face, 1-mask
    //检测算法结果
    vector<Rect> objects;
    vector<int> classes;
    vector<float> confidences;



    //face results
    vector<Rect> faceObjects;
    vector<int> faceClasses;
    vector<float> faceConfidences;

//ncnn
    DetectNcnn detectNcnn;
    DetectNcnnFace detectFaceSlim; //face detector

    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;
    deque<int>faceRectX={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};
    deque<int>driverStatus={0,0,0,0,0,0};

    int tmpSmokeCnt = 0;
    int smokeNumStatus = 0;

    int tmpCallCnt = 0;
    int callNumStatus = 0;

    int tmpEyeCnt = 0;
    int eyeNumStatus = 0;
};
#endif //ADAS_DWSAPP_ALGO_H_H