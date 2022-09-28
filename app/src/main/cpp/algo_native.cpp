
#include <sys/stat.h>
#include <jni.h>
#include "algo_native.h"
#include "algo/algo.h"
//#include "algo/mobilefacenet/mtcnnNcnn.h"
//#include "algo/mobilefacenet/base.h"
//#include "algo/mobilefacenet/arcface.h"
#include "sys/system_properties.h"
#include "native_inf.h"

using namespace std;
using namespace cv;

#define ALGO_VERSION "V1.1.0"

#define SCALAR_UYVY_WHITE Scalar(127, 255, 0)

#define UYVY 0x27

#define CALL_CLASSIFY_SCORE "CALL_CLASSIFY_SCORE"
#define CALL_CLASSIFY_LEVEL "CALL_CLASSIFY_LEVEL"

#define SMOKE_CLASSIFY_SCORE "SMOKE_CLASSIFY_SCORE"
#define SMOKE_CLASSIFY_LEVEL "SMOKE_CLASSIFY_LEVEL"

#define    YAWN_SCORE  "YAWN_SCORE"
#define    YAWN_LEVEL  "YAWN_LEVEL"

#define    ATTEN_SCORE  "ATTEN_SCORE"
#define    ATTEN_LEVEL  "ATTEN_LEVEL"

#define    EYE_SCORE  "EYE_SCORE"
#define    EYE_LEVEL  "EYE_LEVEL"

#define NOP_LEVEL "NOP_LEVEL"

//报警队列阈值
#define SMOKE_CLASSIFY_DEQ_CONF "SMOKE_CLASSIFY_DEQ_CONF"
#define CALL_CLASSIFY_DEQ_CONF "CALL_CLASSIFY_DEQ_CONF"
#define YAWN_DEQ_CONF "YAWN_DEQ_CONF"
#define ATTEN_DEQ_CONF "ATTEN_DEQ_CONF"
#define EYE_DEQ_CONF "EYE_DEQ_CONF"

//检测阈值
#define SMOKE_CONFIDENCE "SMOKE_CONFIDENCE"
#define CALL_CONFIDENCE "CALL_CONFIDENCE"
#define YAWN_CONFIDENCE "YAWN_CONFIDENCE"
#define EYE_CONFIDENCE "EYE_CONFIDENCE"
#define SMOKE_DOT "SMOKE_DOT"

bool enable;

Algo *algo;
Mat algoFrame;

bool extraReady = false;
int extraIdx = 0;
Mat extraFrame[2];

int frameWidth = 0, frameHeight = 0;

float currentSpeed;
bool stopFlag = true;

int faceDetectFlag = 1;
int demoMode;//模式选择
int demo = 0;


pthread_mutex_t algo_mutex;


/**
 * 获取当前时间
 */
long getCurrentTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

jstring stringTojstring(JNIEnv *env, const char *pat) {
    jclass strClass = (env)->FindClass("java/lang/String");
    jmethodID ctorID = (env)->GetMethodID(strClass, "<init>", "([BLjava/lang/String;)V");
    jbyteArray bytes = (env)->NewByteArray(strlen(pat));
    (env)->SetByteArrayRegion(bytes, 0, strlen(pat), (jbyte *) pat);
    jstring encoding = (env)->NewStringUTF("GB2312");
    return (jstring) (env)->NewObject(strClass, ctorID, bytes, encoding);
}

void takeSnapshot(char *path, int quality) {
    pthread_mutex_lock(&algo_mutex);

    time_t t = time(NULL);

    char speedStr[25];
    sprintf(speedStr, "%.0f km/h", currentSpeed);
    cv::putText(algoFrame, speedStr, cv::Point(20, frameHeight - 20), CV_FONT_HERSHEY_COMPLEX_SMALL,
                0.9, Scalar(255, 255, 255), 2);

    char ch[64] = {0};
    strftime(ch, sizeof(ch) - 1, "%Y-%m-%d %H:%M:%S", localtime(&t));
    cv::putText(algoFrame, ch, cv::Point(frameWidth - 240, frameHeight - 20),
                CV_FONT_HERSHEY_COMPLEX_SMALL,
                0.9, Scalar(255, 255, 255), 2);

    cv::putText(algoFrame, ALGO_VERSION, cv::Point(frameWidth - 76, 32),
                CV_FONT_HERSHEY_COMPLEX_SMALL,
                0.9, Scalar(255, 255, 255), 2);

    vector<int> compression_params;
    compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);  //选择jpeg
    compression_params.push_back(quality); //在这个填入你要的图片质量
    imwrite(path, algoFrame, compression_params);

    pthread_mutex_unlock(&algo_mutex);
}


int readIntPara(string line, string name) {
    int pos = -1;
    if ((pos = line.find("=")) != -1) {
        return atoi(line.substr(pos + 1).c_str());
    }
    return -1;
}

int readFloatPara(string line, string name) {
    int pos = -1;
    if ((pos = line.find("=")) != -1) {
        return atof(line.substr(pos + 1).c_str());
    }
    return -1;
}

int readParams(string line, string name) {
    int pos = -1;
    if ((pos = line.find("=")) != -1) {
        return atoi(line.substr(pos + 1).c_str());
    }
    return 0;
}

extern "C"
JNIEXPORT void JNICALL JNI_METHOD(initNative)(JNIEnv *env, jclass thiz) {
    init();
    initParams();
}


int init() {
    LOGE("------init---------");
    algo = new Algo();
    frameWidth = 720;
    frameHeight = 1280;
    algo->init(UYVY, frameWidth, frameHeight);

    enable = true;
    return 0;
}

int initParams() {
    LOGE("------initParams---------");
    //read_feature_txt("/mnt/sdcard/TME/face_features/feature_list.txt", faceLib);

    ifstream conf;
    conf.open("/mnt/sdcard/TME/config/demo.cfg"); //报警参数文件
    if (conf) {
        string line;
        while (getline(conf, line)) {
            if (line.find("MODE") == 0) {
                demoMode = readIntPara(line, "MODE");
                LOGE("dsm init algo mode = %d ", demoMode);


                //0工作模式 1测试模式
                if (demoMode == 0) {
                    ifstream infile;
                    infile.open("/mnt/sdcard/TME/config/dsm.cfg"); //报警参数文件
                    if (infile) {
                        LOGE("DSM read Para");
                        bool findIndex = false;
                        string line;

                        while (getline(infile, line)) {
                            //READ PARAMS //读取报警时间
                            if (line.find(CALL_CLASSIFY_LEVEL) == 0) {
                                int value = readParams(line, CALL_CLASSIFY_LEVEL);
                                algo->algoParam.callClassifyLevel = value;
                            } else if (line.find(SMOKE_CLASSIFY_LEVEL) == 0) {
                                int value = readParams(line, SMOKE_CLASSIFY_LEVEL);
                                algo->algoParam.smokeClassifyLevel = value;
                            } else if (line.find(YAWN_LEVEL) == 0) {
                                int value = readParams(line, YAWN_LEVEL);
                                algo->algoParam.yawnLevel = value;
                            } else if (line.find(ATTEN_LEVEL) == 0) {
                                int value = readParams(line, ATTEN_LEVEL);
                                algo->algoParam.attenLevel = value;
                            } else if (line.find(EYE_LEVEL) == 0) {
                                int value = readParams(line, EYE_LEVEL);
                                algo->algoParam.eyeLevel = value;
                            } else if (line.find(NOP_LEVEL) == 0) {
                                int value = readParams(line, NOP_LEVEL);
                                algo->algoParam.nopLevel = value;
                            } else if (line.find(SMOKE_CLASSIFY_SCORE) == 0) {
                                int value = readParams(line, SMOKE_CLASSIFY_SCORE);
                                algo->algoParam.smokeClassifyScore = value / 100.0f;
                            } else if (line.find(CALL_CLASSIFY_SCORE) == 0) {
                                int value = readParams(line, CALL_CLASSIFY_SCORE);
                                algo->algoParam.callClassifyScore = value / 100.0f;
                            } else if (line.find(YAWN_SCORE) == 0) {
                                int value = readParams(line, YAWN_SCORE);
                                algo->algoParam.yawnClassifyScore = value / 100.0f;
                            } else if (line.find(ATTEN_SCORE) == 0) {
                                int value = readParams(line, ATTEN_SCORE);
                                algo->algoParam.attenLeftScore = value / 100.0f;
                                algo->algoParam.attenRightScore = value / 100.0f;
                                algo->algoParam.attenUpScore = value / 100.0f;
                                algo->algoParam.attenDownScore = value / 100.0f;
                            } else if (line.find(EYE_SCORE) == 0) {
                                int value = readParams(line, EYE_SCORE);
                                algo->algoParam.eyeClassifyScore = value / 100.0f;
                            } else if (line.find(CALL_CONFIDENCE) == 0) {
                                int value = readParams(line, CALL_CONFIDENCE);
                                algo->algoParam.callConfidence = value / 100.f;
                            } else if (line.find(SMOKE_CONFIDENCE) == 0) {
                                int value = readParams(line, SMOKE_CONFIDENCE);
                                algo->algoParam.smokeConfidence = value / 100.0f;
                            } else if (line.find(YAWN_CONFIDENCE) == 0) {
                                int value = readParams(line, YAWN_CONFIDENCE);
                                algo->algoParam.yawnConfidence = value / 100.0f;
                            } else if (line.find(EYE_CONFIDENCE) == 0) {
                                int value = readParams(line, EYE_CONFIDENCE);
                                algo->algoParam.eyeConfidence = value / 100.0f;
                            } else if (line.find(SMOKE_CLASSIFY_DEQ_CONF) == 0) {
                                int value = readParams(line, SMOKE_CLASSIFY_DEQ_CONF);
                                algo->algoParam.smokeClassifyDeqConf = value / 100.0f;
                            } else if (line.find(CALL_CLASSIFY_DEQ_CONF) == 0) {
                                int value = readParams(line, CALL_CLASSIFY_DEQ_CONF);
                                algo->algoParam.callClassifyDeqConf = value / 100.0f;
                            } else if (line.find(YAWN_DEQ_CONF) == 0) {
                                int value = readParams(line, YAWN_DEQ_CONF);
                                algo->algoParam.yawnDeqConf = value / 100.0f;
                            } else if (line.find(ATTEN_DEQ_CONF) == 0) {
                                int value = readParams(line, ATTEN_DEQ_CONF);
                                algo->algoParam.attenDeqConf = value / 100.0f;
                            } else if (line.find(EYE_DEQ_CONF) == 0) {
                                int value = readParams(line, EYE_DEQ_CONF);
                                algo->algoParam.eyeDeqConf = value / 100.0f;
                            } else if (line.find(SMOKE_DOT) == 0) {
                                int value = readParams(line, SMOKE_DOT);
                                algo->algoParam.smokeDot = value;
                            } else {
                                LOGE("UNKOWN %s", line.c_str());
                            }
                        }
                    }
                } else {
                    //每帧图像的分类概率
                    algo->algoParam.callClassifyScore = 0.95f;
                    algo->algoParam.smokeClassifyScore = 0.95f;
                    algo->algoParam.yawnClassifyScore = 0.95f;
                    algo->algoParam.attenRightScore = 0.9f;
                    algo->algoParam.attenLeftScore = 0.9f;
                    algo->algoParam.attenUpScore = 0.9f;
                    algo->algoParam.attenDownScore = 0.9f;
                    algo->algoParam.eyeClassifyScore = 0.7f;
                    algo->algoParam.normalScore = 0.9f;

                    //每一帧图像检测阈值
                    algo->algoParam.callConfidence = 0.3f;
                    algo->algoParam.smokeConfidence = 0.3f;
                    algo->algoParam.yawnConfidence = 0.3f;
                    algo->algoParam.eyeConfidence = 0.3f;
                    algo->algoParam.smokeDot = 0;

                    //报警队列时长  时间 （单位：秒）
                    algo->algoParam.smokeClassifyLevel = 2; // 2s
                    algo->algoParam.callClassifyLevel = 2;
                    algo->algoParam.attenLevel = 2;
                    algo->algoParam.yawnLevel = 2;
                    algo->algoParam.eyeLevel = 2;
                    algo->algoParam.nopLevel = 10;

                    //报警队列报警阈值
                    algo->algoParam.smokeClassifyDeqConf = 0.6;
                    algo->algoParam.callClassifyDeqConf = 0.6;
                    algo->algoParam.attenDeqConf = 0.6;
                    algo->algoParam.yawnDeqConf = 0.6;
                    algo->algoParam.eyeDeqConf = 0.6;
                }
            } else {
                LOGE("%s", line.c_str());
            }
        }
    }
    conf.close(); //关闭文件输入流
    return 0;
}


extern "C"
JNIEXPORT void JNICALL JNI_METHOD(setAlgoEnable)(JNIEnv *env, jobject thiz, jboolean _enable) {
    enable = _enable;
}

extern "C"
JNIEXPORT void JNICALL
JNI_METHOD(runningDsmAlgo)(JNIEnv *env, jobject thiz, jbyteArray _frame, jint _width,
                           jint _height) {

    jbyte *buf = env->GetByteArrayElements(_frame, 0);

    //LOGE("runningDsmAlgo width: %d, height: %d", _width, _height);
    Mat img_yuv(_height * 3 / 2, _width, CV_8UC1);
    img_yuv.data = (unsigned char *) buf;
    Mat img_nv21;
    cvtColor(img_yuv, img_nv21, CV_YUV2BGR_NV21);

    string type;
    int ret = detectFrame(img_nv21, type);
    LOGE("detectFrame result:  %d", ret);

    jclass jObjClass = env->GetObjectClass(thiz);
    if (jObjClass != nullptr) {
        jmethodID sendAlarmCallBack = env->GetMethodID(jObjClass, "sendAlarm",
                                                       "(ILjava/lang/String;)V");
        jstring msg = env->NewStringUTF(type.c_str());
        env->CallVoidMethod(thiz, sendAlarmCallBack, ret, msg);
    }

    img_nv21.release();
    img_yuv.release();

    env->ReleaseByteArrayElements(_frame, buf, 0);
}

int index = 0;


int detectFrame(cv::Mat &frame, string &type) {
    int rst = 0;
    double time_t1 = 0, time_t2 = 0, time = 0;
    time_t1 = getTickCount();

    //疲劳检测
    if (enable) {
        //pthread_mutex_lock(&algo_mutex);

        //第一步 分类获取结果
        time_t1 = getTickCount();

        Mat copyFrame = Mat(frame.rows, frame.cols, frame.depth());
        cv::transpose(frame, copyFrame);
        flip(copyFrame, copyFrame, 0);

        rst = algo->detectFrame(copyFrame, demoMode);

        if (rst == NRL_FACE) {
            if (!extraReady) {
                extraFrame[extraIdx] = copyFrame.clone();
                extraReady = true;
            }
        }

        //pthread_mutex_unlock(&algo_mutex);

        time_t1 = (double) getTickCount() - time_t1;
        time_t1 = time_t1 / ((float) getTickFrequency() / 1000.);
        LOGE("total algorithm time cost = %f ms,", time_t1);

        char alarm[25];
        if (rst == EYES_ALARM) {
            sprintf(alarm, "EYES_ALARM");
        } else if (rst == YAWN_ALARM) {
            sprintf(alarm, "YAWN_ALARM");
        } else if (rst == CALL_ALARM) {
            sprintf(alarm, "CALL_ALARM");
        } else if (rst == SMOKE_ALARM) {
            sprintf(alarm, "SMOKE_ALARM");
        } else if (rst == ATTEN_ALARM) {
            sprintf(alarm, "ATTEN_ALARM");
        } else if (rst == ABNORMAL_ALARM) {
            sprintf(alarm, "ABNORMAL_ALARM");
        } else if (rst == HAND_OFF_ALARM) {
            sprintf(alarm, "HAND_OFF_ALARM");
        } else if (rst == INFRARED_ALARM) {
            sprintf(alarm, "INFRARED_ALARM");
        } else if (rst == NO_FACE) {
            sprintf(alarm, "NO_DRIVER");
        } else if (rst == NRL_FACE) {
            sprintf(alarm, "NORMAL");
        } else if (rst == COVER_ALARM) {
            sprintf(alarm, "COVER_ALARM");
        }
        type = alarm;

//        int baseline;
//        Size textSize = getTextSize(alarm, FONT_HERSHEY_COMPLEX, 2, 2, &baseline);
//        Point origin;
//        origin.x = frame.cols / 2 - textSize.width / 2;
//        origin.y = frame.rows / 2 + textSize.height / 2;
        cv::putText(copyFrame, alarm, cv::Point(15, 64), CV_FONT_HERSHEY_COMPLEX,
                    2, SCALAR_UYVY_WHITE, 2);
//        cv::putText(algoFrame, alarm, cv::Point(15, 64), CV_FONT_HERSHEY_COMPLEX_SMALL,
//                    0.9, Scalar(255, 255, 255), 2);

        if (index++ % 10 == 1) {
            imwrite("/sdcard/text.jpg", copyFrame);
        }
        if (rst > 0) {
            if (rst == ABNORMAL_ALARM) {
                faceDetectFlag = 1;
            } else if (rst == COVER_ALARM) {
                faceDetectFlag = 1;
            }
        }

        //       drawFrame(frame);
    }

    time_t2 = getTickCount();
    time = 1000 * (time_t2 - time_t1) / cv::getTickFrequency();
    //LOGE("ALGO TOTAL TIME : %lf", time);
    return rst;
}


string getVersion() {
    return ALGO_VERSION;
}

void release() {
    stopFlag = true;
}


extern "C"
JNIEXPORT void JNICALL JNI_METHOD(release)(JNIEnv *env, jclass clazz) {
}


