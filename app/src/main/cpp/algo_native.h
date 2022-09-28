//
// Created by Imx on 2018/9/3.
//
#include "opencv2/opencv.hpp"

#ifndef TME_PRO_ALGOINF_H
#define TME_PRO_ALGOINF_H

using namespace std;


int init();

int initParams();

int detectFrame(cv::Mat &frame, string &alarmType);

void release();

int detectEnable(bool enable);

std::string getVersion();

void sendAlarm(int alarm, const char *extMsg);

void takeSnapshot(char *path, int _quality);

#endif //TME_PRO_ALGOINF_H
