
#include "algo.h"

#define NV21 0x11
#define RGB  0x0
#define BGR  0x1

#define ACTION_FPS 6
#define YAWN_FPS 6
#define EYE_FPS 8
#define ATTEN_FPS 8

#define DETECT_WIDTH 600
#define DETECT_HEIGHT 600
#define DETECT_FACE_WIDTH 900
#define START_IMAGE_X 200

void restoreRect(Rect &src, int times) {
    src.x *= times;
    src.y *= times;
    src.width *= times;
    src.height *= times;
}

void AsertROI(Rect &roi, int width, int height) {
    width = width - 1;
    height = height - 1;

    if (roi.x < 0)
        roi.x = 0;
    else if (roi.x > width)
        roi.x = width;

    if (roi.y < 0)
        roi.y = 0;
    else if (roi.y > height)
        roi.y = height;

    if ((roi.x + roi.width) > width)
        roi.width = width - roi.x;

    if ((roi.y + roi.height) > height)
        roi.height = height - roi.y;
}

Algo::Algo() {
    faceDetector = MTCNN("/mnt/sdcard/TME/models/mtcnn");

    string eyeLabelFile[3] = {"CloseEye", "OpenEye", "BlackGlass"};//睁眼闭眼分类器
    classifierEye =
            Classifier(
                    "/mnt/sdcard/TME/models/cifar_eye/cifarEyeDeploy.prototxt",
                    "/mnt/sdcard/TME/models/cifar_eye/cifar10_quickEye_iter.caffemodel",
                    "/mnt/sdcard/TME/models/cifar_eye/mean.binaryproto",
                    eyeLabelFile, 3);

    string ActionLabelFile[3] = {"smokeClassify", "callClassify", "Normal"};//是否有抽烟打手机分类器
    classifierAction =
            Classifier(
                    "/mnt/sdcard/TME/models/cifar_action/cifarActionDeploy.prototxt",
                    "/mnt/sdcard/TME/models/cifar_action/cifar10_quick_action.caffemodel",
                    "/mnt/sdcard/TME/models/cifar_action/mean.binaryproto",
                    ActionLabelFile, 3);

    string attenLabelFile[3] = {"Left", "Right", "NoLR"};
    classifierAtten =
            Classifier(
                    "/mnt/sdcard/TME/models/cifar_atten/cifarLRDeploy.prototxt",
                    "/mnt/sdcard/TME/models/cifar_atten/cifar10_quickLR_iter.caffemodel",
                    "/mnt/sdcard/TME/models/cifar_atten/mean.binaryproto",
                    attenLabelFile, 3);

    string mouthLabelFile[3] = {"CloseMouth", "HalfOpen", "OpenMouth"};//是否打哈欠分类器
    classifierMouth =
            Classifier(
                    "/mnt/sdcard/TME/models/cifar_mouth/cifarOpenDeploy.prototxt",
                    "/mnt/sdcard/TME/models/cifar_mouth/cifar10_quick_iter.caffemodel",
                    "/mnt/sdcard/TME/models/cifar_mouth/mean.binaryproto",
                    mouthLabelFile, 3);

//    objectDetect = ObjectDetector("/mnt/sdcard/Adas/models/CifarDetect/detect_deploy.prototxt",
//                                  "/mnt/sdcard/Adas/models/CifarDetect/detect_deploy.caffemodel");
    //objectDetect = ObjectDetector("/mnt/sdcard/TME/models/cifar_detect/detect_deploy.prototxt",
    //                              "/mnt/sdcard/TME/models/cifar_detect/detect_deploy.caffemodel");

    tracker = KCFTracker(true, false, true, false);

}


int Algo::init(int format, int width, int height) {
    smallTimes = 4;
    smallFrame = Mat(height / smallTimes, width / smallTimes, CV_8UC3);
    classifyFrame = Mat::zeros(32, 32, CV_8UC3);

    stEyeAction.dataSize = EYE_FPS * algoParam.eyeLevel;
    stEyeAction.alarmCnt = (int) (stEyeAction.dataSize * algoParam.eyeDeqConf); //閉眼

    stYawnAction.dataSize = YAWN_FPS * algoParam.yawnLevel;
    stYawnAction.alarmCnt = (int) (stYawnAction.dataSize * algoParam.yawnDeqConf); //打哈欠

    stAttenAction.dataSize = ATTEN_FPS * algoParam.attenLevel;
    stAttenAction.alarmCnt = (int) (stAttenAction.dataSize * algoParam.attenDeqConf); //左顧右盼

    stSmokeClassify.dataSize = ACTION_FPS * algoParam.smokeClassifyLevel;//抽烟
    stSmokeClassify.alarmCnt = (int) (stSmokeClassify.dataSize * algoParam.smokeClassifyDeqConf);

    stCallClassify.dataSize = ACTION_FPS * algoParam.callClassifyLevel;
    stCallClassify.alarmCnt = (int) (stCallClassify.dataSize * algoParam.callClassifyDeqConf); //打電話

    stFaceAction.dataSize = 10 * algoParam.nopLevel;
    stFaceAction.alarmCnt = (int) (stFaceAction.dataSize - 2); //駕駛員離崗

    stBlockGlass.dataSize = EYE_FPS * algoParam.eyeLevel;
    stBlockGlass.alarmCnt = stBlockGlass.dataSize * algoParam.eyeDeqConf; //红外阻断

    stCover.dataSize = 30;//遮挡
    stCover.alarmCnt = 27;

    return 0;
}


/**
 * 对驾驶员图像进行分类
 */
int Algo::detectFrame(cv::Mat &frameIn, int &demoMode) {
    openblas_set_num_threads(4);
    if (frameIdx > 50000) {
        frameIdx = 0;
    }
    frameIdx++;

    if (frameIdx % 10 == 1) {
       // imwrite("/sdcard/frameIn.jpg", frameIn);
    }

    if (frameIn.rows > frameIn.cols) { // 竖图
        imageType = 1;
    } else {
        imageType = 2;
    }
//    if ((720 == frameIn.cols) && (1280 == frameIn.rows)) {
//        imageType = 1;
//    }
//    if ((1280 == frameIn.cols) && (720 == frameIn.rows)) {
//        imageType = 2;
//    }
//    if (0 == imageType) {
//        return IMAGE_TYPE_ALARM;
//    }

    //LOGE("imageType: %d", imageType);
    double time_t1 = 0, time_t2 = 0, time = 0;
    time_t1 = getTickCount();

    Mat frame;
    Mat coverROI;
    if (1 == imageType) //竖图
    {
        frame = frameIn.clone();
        resize(frame, smallFrame, Size(smallFrame.cols, smallFrame.rows));
        resize(smallFrame, coverROI, Size(frame.cols * 0.25, frame.rows * 0.125));
    }
    if (2 == imageType) //横图
    {
        frame = frameIn(Rect(START_IMAGE_X, 0, DETECT_FACE_WIDTH, frameIn.rows)).clone();//ROI
        resize(frame, smallFrame, Size((int) (frame.cols * 0.25), (int) (frame.rows * 0.25)));
        resize(smallFrame, coverROI, Size(frame.cols * 0.2, frame.rows * 0.25));
        //coverROI = smallFrame(Rect(50,50,100,100)).clone();
    }


    //LOGE("smallFrame---w=%d,h=%d",smallFrame.cols,smallFrame.rows);
    //LOGE("frame---w=%d,h=%d",frame.cols,frame.rows);
    //LOGE("frameIn---w=%d,h=%d",frameIn.cols,frameIn.rows);

    Mat grayFrameZD;
    cvtColor(coverROI, grayFrameZD, CV_BGR2GRAY);
    if (frameIdx % 10 == 1) {
        imwrite("/sdcard/gray.jpg", grayFrameZD);
    }

    Mat tmp_mZD, tmp_sdZD;
    double mZD = 0.0, sdZD = 0.0;
    meanStdDev(grayFrameZD, tmp_mZD, tmp_sdZD);
    mZD = tmp_mZD.at<double>(0, 0);
    sdZD = tmp_sdZD.at<double>(0, 0);

//    time_t1 = (double) getTickCount() - time_t1;
//    time_t1 = time_t1 / ((float) getTickFrequency() / 1000.);
//    LOGE("time_t1 = %f ms, smallFrame.w = %d , smallFrame.h = %d",time_t1,smallFrame.cols,smallFrame.rows);
//    LOGE("-------mZD = %f, std = %f-----",mZD, sdZD);

    if (sdZD < 15 && mZD < 30) {
        AddAction(COVER_TYPE, 1);
    } else {
        AddAction(COVER_TYPE, 0);
    }

//    cvtColor(frame, frame, CV_BGR2GRAY);

    if (CheckActionResult(COVER_TYPE) > 0) {
        ClearAllResult();
        Mat faceROIImg;
        int flagPerson = 0;
        if (1 == imageType) //竖图
        {
            faceROIImg = frame(Rect(0, 200, 715, 715)).clone();
            flagPerson = FaceCheckNcnn(faceROIImg, 0.35);
        }
        if (2 == imageType) //横图
        {
            faceROIImg = frame(Rect(100, 0, 715, 715)).clone();
            flagPerson = FaceCheckNcnn(faceROIImg, 0.35);
        }
        if (flagPerson == 1) {
            Mat saveImg;
            int imgheight = frame.rows;
            int imgwidth = frame.cols;
            int maxl = imgheight > imgwidth ? imgheight : imgwidth;
            int paddingleft = (maxl - imgwidth) >> 1;
            int paddingright = (maxl - imgwidth) >> 1;
            int paddingbottom = (maxl - imgheight) >> 1;
            int paddingtop = (maxl - imgheight) >> 1;
            copyMakeBorder(frame, saveImg, paddingtop, paddingbottom, paddingleft, paddingright,
                           cv::BORDER_CONSTANT, 0);
            faceObjects.clear();
            faceClasses.clear();
            faceConfidences.clear();
            detectFaceSlim.detectObjNcnn(saveImg, faceObjects, faceClasses, faceConfidences);
            if (faceClasses.size() < 1) {
                return COVER_ALARM;     //摄像头遮挡报警
            } else {
                return NRL_FACE;
            }
        } else {
            return NRL_FACE;
        }
    }

//    LOGE("sdZD: %f, mZD: %f", sdZD, mZD);
    if (sdZD < 15 && mZD < 30) {
        return NO_FACE; //摄像头遮挡返回
    }

    if (frameIdx % 6001 == 1) {
        Mat detectMaskImg;
        int imgheight = smallFrame.rows;
        int imgwidth = smallFrame.cols;
        int maxl = imgheight > imgwidth ? imgheight : imgwidth;

        int paddingleft = (maxl - imgwidth) >> 1;
        int paddingright = (maxl - imgwidth) >> 1;
        int paddingbottom = (maxl - imgheight) >> 1;
        int paddingtop = (maxl - imgheight) >> 1;
        copyMakeBorder(smallFrame, detectMaskImg, paddingtop, paddingbottom, paddingleft,
                       paddingright, cv::BORDER_CONSTANT, 0);
        faceObjects.clear();
        faceClasses.clear();
        faceConfidences.clear();
        detectFaceSlim.detectObjNcnn(detectMaskImg, faceObjects, faceClasses, faceConfidences);

        if (faceClasses.size() > 0) {
            errCnt = 0;
            //取出概率最大的脸
            int IdProMax = 0;
            float scoreMax = faceConfidences[0];
            for (int i = 1; i < faceClasses.size(); i++) {
                if (faceConfidences[i] > scoreMax) {
                    IdProMax = i;
                }
            }
            if (faceClasses[IdProMax] == 2) {
                faceOrMask = 1;
            } else {
                faceOrMask = 0;
            }
        }
    }

    if ((frameIdx % 25 == 1) || (trackFlag == 0)) {
        minSize = 40;
        vector<FaceInfo> faceInfo = faceDetector.Detect(smallFrame, minSize, threshold, factor, 3);

        if (faceInfo.size() > 0) {
            errCnt = 0;
            int tmpW = 0;
            int tmpFlag = 0;

            for (int i = 0; i < faceInfo.size(); i++) {
                int w = (int) (faceInfo[i].bbox.xmax - faceInfo[i].bbox.xmin + 1);
                if (w > tmpW) {
                    tmpW = w;
                    tmpFlag = i;
                }
            }
            //检测到人脸
            int x = (int) faceInfo[tmpFlag].bbox.xmin;
            int y = (int) faceInfo[tmpFlag].bbox.ymin;
            int w = (int) (faceInfo[tmpFlag].bbox.xmax - faceInfo[tmpFlag].bbox.xmin + 1);
            int h = (int) (faceInfo[tmpFlag].bbox.ymax - faceInfo[tmpFlag].bbox.ymin + 1);

            Rect trackRoi = Rect(x, y, w, h);

            AsertROI(trackRoi, smallFrame.cols, smallFrame.rows);
            kcfResult = trackRoi;
            tracker.init(trackRoi, smallFrame);

            faceRect = kcfResult;
            restoreRect(faceRect, smallTimes); //还原到输入图像上，扩大四倍

            trackFlag = 1;
        } else {
            Mat faceImg;
            int imgheight = smallFrame.rows;
            int imgwidth = smallFrame.cols;
            int maxl = imgheight > imgwidth ? imgheight : imgwidth;

            int paddingleft = (maxl - imgwidth) >> 1;
            int paddingright = (maxl - imgwidth) >> 1;
            int paddingbottom = (maxl - imgheight) >> 1;
            int paddingtop = (maxl - imgheight) >> 1;
            copyMakeBorder(smallFrame, faceImg, paddingtop, paddingbottom, paddingleft,
                           paddingright, cv::BORDER_CONSTANT, 0);
            faceObjects.clear();
            faceClasses.clear();
            faceConfidences.clear();
            detectFaceSlim.detectObjNcnn(faceImg, faceObjects, faceClasses, faceConfidences);

            if (faceClasses.size() > 0) {
                errCnt = 0;
                //取出概率最大的脸
                int IdProMax = 0;
                float scoreMax = faceConfidences[0];
                for (int i = 1; i < faceClasses.size(); i++) {
                    if (faceConfidences[i] > scoreMax) {
                        IdProMax = i;
                    }
                }
                if (faceClasses[IdProMax] == 2) {
                    faceOrMask = 1;
                } else {
                    faceOrMask = 0;
                }
                Rect faceRect1;
                faceRect1.x = faceObjects[IdProMax].x - paddingleft;
                faceRect1.y = faceObjects[IdProMax].y - paddingtop;
                faceRect1.width = faceObjects[IdProMax].width;
                faceRect1.height = faceObjects[IdProMax].height;

                Rect trackRoii = faceRect1;

                kcfResult = trackRoii;
                tracker.init(trackRoii, smallFrame);

                faceRect = kcfResult;
                restoreRect(faceRect, 4); //还原到输入图像上，扩大四倍

                trackFlag = 1;
            } else {
                errCnt++;
                trackFlag = 0;
            }
        }
    }

    //人脸跟踪
    if ((faceRect.width > 10) && (errCnt < ERR_CNT)) {
        float value = 0.0f;
        kcfResult = tracker.update(smallFrame, value);

        if (value < 0.4f || value > 1.2f) {
            LOGE("tracking 22222222222222222");
            rstFlag = NO_FACE;
            trackFlag = 0;

            AddAction(FACE_ACTION_TYPE, 1);
            AddAction(EYE_ACTION_TYPE, 0);
            AddAction(YAWN_ACTION_TYPE, 0);
            AddAction(ATTEN_ACTION_TYPE, 0);
            AddAction(SMOKE_CLASSIFY_TYPE, 0);
            AddAction(CALL_CLASSIFY_TYPE, 0);
            //AddAction(PHONE_TYPE, 0);
            //AddAction(CIGARETTE_TYPE, 0);
            //AddAction(BLINK_TYPE, 0);
            AddAction(BLOCKGLASS_TYPE, 0);
        } else {
            trackFlag = 1;
            rstFlag = NRL_FACE; //处于跟踪的脸

            AsertROI(kcfResult, smallFrame.cols, smallFrame.rows);
            faceRect = kcfResult;
            restoreRect(faceRect, smallTimes); //还原到输入图像上，扩大四倍
            AddAction(FACE_ACTION_TYPE, 0);
        }
    } else {
        faceRect = Rect(0, 0, 0, 0);
        LOGE("tracking 33333333333333333");
        rstFlag = NO_FACE;
        trackFlag = 0;

        AddAction(FACE_ACTION_TYPE, 1);
        AddAction(EYE_ACTION_TYPE, 0);
        AddAction(YAWN_ACTION_TYPE, 0);
        AddAction(ATTEN_ACTION_TYPE, 0);
        AddAction(SMOKE_CLASSIFY_TYPE, 0);
        AddAction(CALL_CLASSIFY_TYPE, 0);
        //AddAction(PHONE_TYPE, 0);
        //AddAction(CIGARETTE_TYPE, 0);
        //AddAction(BLINK_TYPE, 0);
        AddAction(BLOCKGLASS_TYPE, 0);
        errCnt = ERR_CNT;
    }

    faceRectX.pop_front();
    faceRectX.push_back(faceRect.x);
    //LOGE("faceRect: x=%d,y=%d,width=%d,height=%d",faceRect.x,faceRect.y,faceRect.width,faceRect.height);

    //驾驶员离岗报警
    if (CheckActionResult(FACE_ACTION_TYPE) > 0) {
        ClearAllResult();

        Mat faceROIImg;
        int flagPerson = 0;
        if (1 == imageType) //竖图
        {
            faceROIImg = frame(Rect(0, 200, 715, 715)).clone();
            flagPerson = FaceCheckNcnn(faceROIImg, 0.35);
        }
        if (2 == imageType) //横图
        {
            faceROIImg = frame(Rect(100, 0, 715, 715)).clone();
            flagPerson = FaceCheckNcnn(faceROIImg, 0.35);
        }
        if (flagPerson == 1) {
            Mat saveImg2;
            int imgheight = frame.rows;
            int imgwidth = frame.cols;
            int maxl = imgheight > imgwidth ? imgheight : imgwidth;
            int paddingleft = (maxl - imgwidth) >> 1;
            int paddingright = (maxl - imgwidth) >> 1;
            int paddingbottom = (maxl - imgheight) >> 1;
            int paddingtop = (maxl - imgheight) >> 1;
            copyMakeBorder(frame, saveImg2, paddingtop, paddingbottom, paddingleft, paddingright,
                           cv::BORDER_CONSTANT, 0);
            faceObjects.clear();
            faceClasses.clear();
            faceConfidences.clear();
            detectFaceSlim.detectObjNcnn(saveImg2, faceObjects, faceClasses, faceConfidences);
            if (faceClasses.size() < 1) {
                return ABNORMAL_ALARM; //未检测到人脸
            } else {
                return NRL_FACE;
            }

        } else {
            return NRL_FACE;
        }
    }

    if (rstFlag == NO_FACE) {
        LOGE("tracking 444444444444444");
        return rstFlag;
    }
    //cv::rectangle(frame, faceRect, cv::Scalar(255, 255, 0), 2); //在输入图像中画跟踪的人脸框
    //drawFrame(frame, BGR);
    //sleep(1);

    LOGE("tracking 5555555555555555555");

    if (faceRect.x < 50 || faceRect.y < 5 || faceRect.width < 5 || faceRect.height < 5) {
        return NRL_FACE;
    }
    if ((faceRect.x + faceRect.width > (frame.cols - 50)) ||
        (faceRect.y + faceRect.height > (frame.rows - 50))) {
        return NRL_FACE;
    }
    AsertROI(faceRect, frame.cols, frame.rows);

    LOGE("tracking 66666666666666");

    //抽烟、打手机
    Mat detectFace = frame(faceRect);
    Rect actionRect = faceRect; //输入图像的脸
    actionRect.x -= faceRect.width / 2;
    actionRect.width = faceRect.width + faceRect.width;
    AsertROI(actionRect, frame.cols, frame.rows);
    //cv::rectangle(frame, actionRect, cv::Scalar(255, 0, 0), 1);
    Mat actionFace = frame(actionRect);

    //第一步：检测是否有抽烟打手机
    resize(actionFace, classifyFrame, Size(32, 32));
    predictScore = -0.1f;
    std::vector<Prediction> actionPredictions = classifierAction.Classify(classifyFrame);
    for (size_t i = 0; i < actionPredictions.size(); ++i) {
        Prediction p = actionPredictions[i];
        if (p.second > predictScore) {
            predictScore = p.second;
            predictText = p.first;
        }
    }

//    LOGE("step1---predictText = %s,predictScore=%f,algoParam.callClassifyScore=%f",
//    predictText.c_str(),predictScore,algoParam.callClassifyScore);

    if (predictText == "smokeClassify" && predictScore > algoParam.smokeClassifyScore) //分类抽烟
    {
        AddAction(SMOKE_CLASSIFY_TYPE, 1);
        AddAction(CALL_CLASSIFY_TYPE, 0);
        AddAction(YAWN_ACTION_TYPE, 0);
        AddAction(ATTEN_ACTION_TYPE, 0);
        AddAction(EYE_ACTION_TYPE, 0);
        //AddAction(BLINK_TYPE, 0);
        AddAction(BLOCKGLASS_TYPE, 0);
        alarmFlag = 1;
    } else if (predictText == "callClassify" && predictScore > algoParam.callClassifyScore) {
        AddAction(CALL_CLASSIFY_TYPE, 1);
        AddAction(SMOKE_CLASSIFY_TYPE, 0);
        AddAction(YAWN_ACTION_TYPE, 0);
        AddAction(ATTEN_ACTION_TYPE, 0);
        AddAction(EYE_ACTION_TYPE, 0);
        //AddAction(BLINK_TYPE, 0);
        AddAction(BLOCKGLASS_TYPE, 0);
        alarmFlag = 1;
    } else {
        AddAction(SMOKE_CLASSIFY_TYPE, 0);
        AddAction(CALL_CLASSIFY_TYPE, 0);
        alarmFlag = 0;
    }

    //抽烟报警是否到达阈值
    if (CheckActionResult(SMOKE_CLASSIFY_TYPE) > 0) {
        ClearAllResult();
        if (demoMode == 1)//演示模式
        {
            return SMOKE_ALARM;
        }

        Mat smokeCheckImg;
        int imgheight = smallFrame.rows;
        int imgwidth = smallFrame.cols;
        int maxl = imgheight > imgwidth ? imgheight : imgwidth;

        int paddingleft = (maxl - imgwidth) >> 1;
        int paddingright = (maxl - imgwidth) >> 1;
        int paddingbottom = (maxl - imgheight) >> 1;
        int paddingtop = (maxl - imgheight) >> 1;
        copyMakeBorder(smallFrame, smokeCheckImg, paddingtop, paddingbottom, paddingleft,
                       paddingright, cv::BORDER_CONSTANT, 0);
        faceObjects.clear();
        faceClasses.clear();
        faceConfidences.clear();
        detectFaceSlim.detectObjNcnn(smokeCheckImg, faceObjects, faceClasses, faceConfidences);

        if (faceClasses.size() > 0) {
            //取出概率最大的脸
            int IdProMax = 0;
            float scoreMax = faceConfidences[0];
            for (int i = 1; i < faceClasses.size(); i++) {
                if (faceConfidences[i] > scoreMax) {
                    IdProMax = i;
                }
            }
            if (faceClasses[IdProMax] == 2) {
                return NRL_FACE;
            }
        }
        if (faceOrMask == 1) {
            return NRL_FACE;
        }

        if (1 == imageType) //竖图
        {
            //检测烟
            detectSquareRect.x = 0;
            detectSquareRect.y = faceRect.y;
            detectSquareRect.width = 719;
            tmpHeight = 1279 - faceRect.y;
            if (tmpHeight > 720) {
                detectSquareRect.height = 719;
                //frame(detectSquareRect).copyTo(detectSquareImg);
                detectSquareImg = frame(detectSquareRect).clone();
            } else {
                detectSquareRect.y = 559;
                detectSquareRect.height = 719;
                //frame(detectSquareRect).copyTo(detectSquareImg);
                detectSquareImg = frame(detectSquareRect).clone();
            }

            Rect ssdSmokeRect;
            ssdSmokeRect.x = faceRect.x - (int) (0.4 * faceRect.width);
            ssdSmokeRect.y = faceRect.y + (int) (0.3 * faceRect.height) - detectSquareRect.y;
            ssdSmokeRect.width = (int) (1.8 * faceRect.width);
            ssdSmokeRect.height = (int) (0.8 * faceRect.height);
            AsertROI(ssdSmokeRect, 719, 719);

            int smokeFlag = SmokeCheckNcnn(detectSquareImg, algoParam.smokeConfidence,
                                           ssdSmokeRect);

            if (smokeFlag == 1) {
                driverStatus.pop_front();
                driverStatus.push_back(1);
                //return SMOKE_ALARM; //亮点不开启，抽烟报警
                if (((frameIdx - tmpSmokeCnt) < 1440) && ((frameIdx - tmpSmokeCnt) > 40)) {
                    tmpSmokeCnt = frameIdx;
                    smokeNumStatus++;
                } else {
                    tmpSmokeCnt = frameIdx;
                }
                if (smokeNumStatus > 1) {
                    if (algoParam.smokeDot == 1)//亮点检测,1-开启，默认关闭
                    {
                        Rect brightRect;
                        brightRect.x = faceRect.x - (int) (0.3 * faceRect.width);
                        brightRect.y = faceRect.y + (int) (0.35 * faceRect.height);
                        brightRect.width = (int) (1.6 * faceRect.width);
                        brightRect.height = (int) (0.7 * faceRect.height);
                        AsertROI(brightRect, frame.cols, frame.rows);

                        Mat rgbBrightFace = frame(brightRect);
                        Mat grayBrightFace, binaryBrightFace, closeImage;
                        cvtColor(rgbBrightFace, grayBrightFace, CV_BGR2GRAY);
                        cv::threshold(grayBrightFace, binaryBrightFace, 220, 255, THRESH_BINARY);

                        Mat element = getStructuringElement(MORPH_RECT, Size(7, 7));
                        morphologyEx(binaryBrightFace, closeImage, MORPH_CLOSE, element);

                        contours.clear();
                        hierarchy.clear();
                        cv::findContours(closeImage, contours, hierarchy, RETR_EXTERNAL,
                                         CHAIN_APPROX_NONE,
                                         Point());

                        int flagBright = 0;
                        for (int i = 0; i < contours.size(); i++) {
                            double cntArea = cv::contourArea(contours[i]);
                            if (cntArea > 10 && cntArea < 200) {
                                flagBright = 1;
                            }
                        }
                        if (flagBright == 1) {
                            smokeNumStatus = 0;
                            return SMOKE_ALARM; //存在亮点，抽烟报警
                        } else {
                            return NRL_FACE;
                        }
                    } else {
                        smokeNumStatus = 0;
                        return SMOKE_ALARM; //亮点不开启，抽烟报警
                    }
                } else {
                    return NRL_FACE;
                }
            }
        }

        if (2 == imageType) //横图
        {
            //检测烟
            Rect ssdSmokeRect;
            Rect smokeROI(0, 0, DETECT_WIDTH, DETECT_HEIGHT);
            if (faceRect.width < 250) {
                smokeROI.x = faceRect.x - (int) (0.7 * faceRect.width);
                if (smokeROI.x < 0) {
                    smokeROI.x = 0;
                }
                smokeROI.y = faceRect.y;

                int tmpH = 715 - smokeROI.y;
                if (tmpH < DETECT_HEIGHT) {
                    smokeROI.y = 715 - DETECT_HEIGHT;
                }

                int tmpW = DETECT_FACE_WIDTH - smokeROI.x;
                if (tmpW < DETECT_WIDTH) {
                    smokeROI.x = DETECT_FACE_WIDTH - DETECT_WIDTH - 5;
                }

                detectSquareImg = frame(smokeROI).clone();

                ssdSmokeRect.x = faceRect.x - (int) (0.4 * faceRect.width) - smokeROI.x;
                if (ssdSmokeRect.x < 0) {
                    ssdSmokeRect.x = 0;
                }
                ssdSmokeRect.y = faceRect.y + (int) (0.3 * faceRect.height) - smokeROI.y;
                ssdSmokeRect.width = (int) (1.8 * faceRect.width);
                ssdSmokeRect.height = (int) (0.8 * faceRect.height);
                AsertROI(ssdSmokeRect, DETECT_WIDTH, DETECT_HEIGHT);
            } else {
                detectSquareRect.y = 0;
                detectSquareRect.height = 715;
                detectSquareRect.x = faceRect.x - (int) (0.6 * faceRect.width);
                if (detectSquareRect.x < 0) {
                    detectSquareRect.x = 0;
                }
                tmpWidth = DETECT_FACE_WIDTH - detectSquareRect.x;
                if (tmpWidth > 720) {
                    detectSquareRect.width = 715;
                    detectSquareImg = frame(detectSquareRect).clone();
                } else {
                    detectSquareRect.x = 180;
                    detectSquareRect.width = 715;
                    detectSquareImg = frame(detectSquareRect).clone();
                }

                ssdSmokeRect.x = faceRect.x - (int) (0.4 * faceRect.width) - detectSquareRect.x;
                ssdSmokeRect.y = faceRect.y + (int) (0.3 * faceRect.height) - detectSquareRect.y;
                ssdSmokeRect.width = (int) (1.8 * faceRect.width);
                ssdSmokeRect.height = (int) (0.8 * faceRect.height);
                AsertROI(ssdSmokeRect, 715, 715);
            }

            //cv::rectangle(detectSquareImg, ssdSmokeRect, cv::Scalar(255, 0, 0), 1);
            //drawFrame(detectSquareImg, BGR);
            //sleep(1);

            int smokeFlag = SmokeCheckNcnn(detectSquareImg, algoParam.smokeConfidence,
                                           ssdSmokeRect);
            if (smokeFlag == 1) {
                //cv::putText(frame, "smoke", cv::Point(20, 20), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
                driverStatus.pop_front();
                driverStatus.push_back(1);
                //return SMOKE_ALARM; //亮点不开启，抽烟报警
                if ((frameIdx - tmpSmokeCnt) < 1440 && ((frameIdx - tmpSmokeCnt) > 40)) {
                    tmpSmokeCnt = frameIdx;
                    smokeNumStatus++;
                } else {
                    tmpSmokeCnt = frameIdx;
                }
                if (smokeNumStatus > 1) {
                    if (algoParam.smokeDot == 1)//亮点检测,1-开启，默认关闭
                    {
                        Rect brightRect;
                        brightRect.x = faceRect.x - (int) (0.3 * faceRect.width);
                        brightRect.y = faceRect.y + (int) (0.35 * faceRect.height);
                        brightRect.width = (int) (1.6 * faceRect.width);
                        brightRect.height = (int) (0.7 * faceRect.height);
                        AsertROI(brightRect, frame.cols, frame.rows);

                        Mat rgbBrightFace = frame(brightRect);
                        Mat grayBrightFace, binaryBrightFace, closeImage;
                        cvtColor(rgbBrightFace, grayBrightFace, CV_BGR2GRAY);
                        cv::threshold(grayBrightFace, binaryBrightFace, 220, 255, THRESH_BINARY);

                        Mat element = getStructuringElement(MORPH_RECT, Size(7, 7));
                        morphologyEx(binaryBrightFace, closeImage, MORPH_CLOSE, element);

                        contours.clear();
                        hierarchy.clear();
                        cv::findContours(closeImage, contours, hierarchy, RETR_EXTERNAL,
                                         CHAIN_APPROX_NONE, Point());

                        int flagBright = 0;
                        for (int i = 0; i < contours.size(); i++) {
                            double cntArea = cv::contourArea(contours[i]);
                            if (cntArea > 10 && cntArea < 200) {
                                flagBright = 1;
                            }
                        }
                        if (flagBright == 1) {
                            smokeNumStatus = 0;
                            return SMOKE_ALARM; //存在亮点，抽烟报警
                        } else {
                            return NRL_FACE;
                        }
                    } else {
                        smokeNumStatus = 0;
                        return SMOKE_ALARM; //亮点不开启，抽烟报警
                    }
                } else {
                    return NRL_FACE;
                }
            } else if (smokeFlag == 2) {
                //检测到手机和手
                return NRL_FACE;
            } else if (smokeFlag == 3) {
                if (demoMode == 1)//演示模式
                {
                    return SMOKE_ALARM;
                }
                return NRL_FACE;
                //return HAND_OFF_ALARM;
            }
        }
    }

    LOGE("tracking 77777777777777777777: %d", CheckActionResult(CALL_CLASSIFY_TYPE));

    //打手机检测报警队列
    if (CheckActionResult(CALL_CLASSIFY_TYPE) > 0) {
        LOGE("tracking call-call-call-call-call");
        ClearAllResult();

        if (demoMode == 1)//演示模式
        {
            return CALL_ALARM;
        }

        if (1 == imageType) //竖图
        {
            LOGE("tracking 8888888888888888");
            //打手机检测报警队列
            if (CheckActionResult(CALL_CLASSIFY_TYPE) > 0) {
                LOGE("tracking call-call-call-call- 22222222222222");
                ClearAllResult();
                //检测打手机
                detectSquareRect.x = 0;
                detectSquareRect.y = faceRect.y;
                detectSquareRect.width = 719;
                tmpHeight = 1279 - faceRect.y;
                if (tmpHeight > 720) {
                    detectSquareRect.height = 719;
                    //frame(detectSquareRect).copyTo(detectSquareImg);
                    detectSquareImg = frame(detectSquareRect).clone();
                } else {
                    detectSquareRect.y = 559;
                    detectSquareRect.height = 719;
                    //frame(detectSquareRect).copyTo(detectSquareImg);
                    detectSquareImg = frame(detectSquareRect).clone();
                }

                Rect callRect;
                callRect.x = faceRect.x - (int) (0.7 * faceRect.width);
                callRect.y = faceRect.y + (int) (0.2 * faceRect.height) - detectSquareRect.y;
                callRect.width = (int) (2.4 * faceRect.width);
                callRect.height = faceRect.height;
                AsertROI(callRect, 719, 719);

                //float start = getTickCount();
                int callFlag = CallCheckNcnn(detectSquareImg, 0.3, callRect);
                //float end = getTickCount();
                //float useTime = 1000.0 * (end - start) / getTickFrequency();
                //LOGE("time----%f",useTime);
                LOGE("tracking callFlag callFlag callFlag callFlag = %d", callFlag);

                if (callFlag == 2) {
                    driverStatus.pop_front();
                    driverStatus.push_back(2);

                    if ((frameIdx - tmpCallCnt) < 1440) {
                        tmpCallCnt = frameIdx;
                        callNumStatus++;
                    } else {
                        tmpCallCnt = frameIdx;
                    }
                    if (callNumStatus > 1) {
                        callNumStatus = 0;
                        LOGE("tracking 999999999999999999");
                        return CALL_ALARM;
                    } else {
                        return NRL_FACE;
                    }
                }
            }
        }
        if (2 == imageType) //横图
        {
            //检测打手机
            Rect callRect;
            Rect phoneROI(0, 0, DETECT_WIDTH, DETECT_HEIGHT);
            if (faceRect.width < 250) {
                phoneROI.x = faceRect.x - (int) (0.7 * faceRect.width);
                if (phoneROI.x < 0) {
                    phoneROI.x = 0;
                }
                phoneROI.y = faceRect.y;

                int tmpH = 715 - phoneROI.y;
                if (tmpH < DETECT_HEIGHT) {
                    phoneROI.y = 715 - DETECT_HEIGHT;
                }

                int tmpW = DETECT_FACE_WIDTH - phoneROI.x;
                if (tmpW < DETECT_WIDTH) {
                    phoneROI.x = DETECT_FACE_WIDTH - DETECT_WIDTH - 5;
                }
                detectSquareImg = frame(phoneROI).clone();

                callRect.x = 0;
                callRect.y = faceRect.y + (int) (0.2 * faceRect.height) - phoneROI.y;
                callRect.width = (int) (2.4 * faceRect.width);
                callRect.height = faceRect.height;
                AsertROI(callRect, DETECT_WIDTH, DETECT_HEIGHT);
            } else {
                detectSquareRect.y = 0;
                detectSquareRect.height = 715;
                detectSquareRect.x = faceRect.x - 0.7 * faceRect.width;
                if (detectSquareRect.x < 0) {
                    detectSquareRect.x = 0;
                }
                tmpWidth = DETECT_FACE_WIDTH - detectSquareRect.x;
                if (tmpWidth > 715) {
                    detectSquareRect.width = 715;
                    detectSquareImg = frame(detectSquareRect).clone();
                } else {
                    detectSquareRect.x = 180;
                    detectSquareRect.width = 715;
                    detectSquareImg = frame(detectSquareRect).clone();
                }

                callRect.x = faceRect.x - (int) (0.7 * faceRect.width) - detectSquareRect.x;
                callRect.y = faceRect.y + (int) (0.2 * faceRect.height);
                callRect.width = (int) (2.4 * faceRect.width);
                callRect.height = faceRect.height;
                AsertROI(callRect, 715, 715);
            }

            //cv::rectangle(detectSquareImg, callRect, cv::Scalar(255, 0, 0), 1);
            //drawFrame(detectSquareImg, BGR);
            //sleep(1);

            //float start = getTickCount();
            int callFlag = CallCheckNcnn(detectSquareImg, algoParam.callConfidence, callRect);
            //float end = getTickCount();
            //float useTime = 1000.0 * (end - start) / getTickFrequency();
            //cout<<"detect time : "<<useTime<<endl;
            //LOGE("callFlag = %d",callFlag);

            if (callFlag == 1) {
                //检测到手和烟
                return NRL_FACE;
            } else if (callFlag == 2) {
                driverStatus.pop_front();
                driverStatus.push_back(2);

                if ((frameIdx - tmpCallCnt) < 1440) {
                    tmpCallCnt = frameIdx;
                    callNumStatus++;
                } else {
                    tmpCallCnt = frameIdx;
                }
                if (callNumStatus > 1) {
                    callNumStatus = 0;
                    //cv::putText(frame, "call", cv::Point(20, 20), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
                    return CALL_ALARM; //打电话报警
                } else {
                    return NRL_FACE;
                }

            } else if (callFlag == 3) {
                if (demoMode == 1)//演示模式
                {
                    return CALL_ALARM; //打电话报警
                }
                return NRL_FACE;
            }
        }
    }

    LOGE("tracking eeeeeeeeeeeeeeeeee");

    if ((predictText == "smokeClassify" && predictScore > 0.6)
        || (predictText == "callClassify" && predictScore > 0.6)) {
        return NRL_FACE;
    }


    //第二步：检测是否打哈欠
    Mat mouthImg = detectFace(
            Rect(0, (int) (0.5 * detectFace.rows), detectFace.cols, (int) (0.5 * detectFace.rows)));
    resize(mouthImg, classifyFrame, Size(32, 32));
    std::vector<Prediction> openclose_predictions = classifierMouth.Classify(classifyFrame);
    predictScore = -0.1;
    for (size_t i = 0; i < openclose_predictions.size(); ++i) {
        Prediction p = openclose_predictions[i];
        if (p.second > predictScore) {
            predictScore = p.second;
            predictText = p.first;
        }
    }

    /*if(predictText != "Normal"){
        char mouth[25];
        sprintf(mouth, "%s : %.2f", predictText.c_str(), predictScore);
        putText(frame, mouth, cv::Point(450, 120), CV_FONT_ITALIC, 0.7, Scalar(255, 0, 0), 2);
    }
    drawFrame(frame, BGR);
    sleep(1);*/

    if (predictText == "OpenMouth" && predictScore > algoParam.yawnClassifyScore) {
        AddAction(YAWN_ACTION_TYPE, 1);
        AddAction(ATTEN_ACTION_TYPE, 0);
        AddAction(EYE_ACTION_TYPE, 0);
        //AddAction(BLINK_TYPE, 0);
        AddAction(BLOCKGLASS_TYPE, 0);
    } else {
        AddAction(YAWN_ACTION_TYPE, 0);
    }

    if (CheckActionResult(YAWN_ACTION_TYPE) > 0) {
        ClearAllResult();

        Mat mouthCheckImg;
        int imgheight = smallFrame.rows;
        int imgwidth = smallFrame.cols;
        int maxl = imgheight > imgwidth ? imgheight : imgwidth;

        int paddingleft = (maxl - imgwidth) >> 1;
        int paddingright = (maxl - imgwidth) >> 1;
        int paddingbottom = (maxl - imgheight) >> 1;
        int paddingtop = (maxl - imgheight) >> 1;
        copyMakeBorder(smallFrame, mouthCheckImg, paddingtop, paddingbottom, paddingleft,
                       paddingright, cv::BORDER_CONSTANT, 0);
        faceObjects.clear();
        faceClasses.clear();
        faceConfidences.clear();
        detectFaceSlim.detectObjNcnn(mouthCheckImg, faceObjects, faceClasses, faceConfidences);

        Rect faceRectMouth;
        if (faceClasses.size() > 0) {
            //取出概率最大的脸
            int IdProMax = 0;
            float scoreMax = faceConfidences[0];
            for (int i = 1; i < faceClasses.size(); i++) {
                if (faceConfidences[i] > scoreMax) {
                    IdProMax = i;
                }
            }
            if (faceClasses[IdProMax] == 2) {
                faceOrMask = 1;
                return NRL_FACE;
            } else {
                faceOrMask = 0;
            }
            faceRectMouth.x = faceObjects[IdProMax].x - paddingleft;
            faceRectMouth.y = faceObjects[IdProMax].y - paddingtop;
            faceRectMouth.width = faceObjects[IdProMax].width;
            faceRectMouth.height = faceObjects[IdProMax].height;
        }
        float faceHtoW = 1.0 * faceRectMouth.height / faceRectMouth.width;
        if (faceHtoW > 1.45) {
            return NRL_FACE;
        }

        if (demoMode == 1)//演示模式
        {
            return YAWN_ALARM;
        }

        if (1 == imageType) //竖图
        {
//检测方法验证
            detectSquareRect.x = 0;
            detectSquareRect.y = faceRect.y;
            detectSquareRect.width = 719;
            tmpHeight = 1279 - faceRect.y;
            if (tmpHeight > 720) {
                detectSquareRect.height = 719;
                //frame(detectSquareRect).copyTo(detectSquareImg);
                detectSquareImg = frame(detectSquareRect).clone();
            } else {
                detectSquareRect.y = 559;
                detectSquareRect.height = 719;
                //frame(detectSquareRect).copyTo(detectSquareImg);
                detectSquareImg = frame(detectSquareRect).clone();
            }


            Rect ssdMouthRect;
            ssdMouthRect.x = faceRect.x - (int) (0.2 * faceRect.width);
            ssdMouthRect.y = faceRect.y + (int) (0.3 * faceRect.height) - detectSquareRect.y;
            ssdMouthRect.width = (int) (1.4 * faceRect.width);
            ssdMouthRect.height = (int) (0.75 * faceRect.height);
            AsertROI(ssdMouthRect, 719, 719);

            int mouthValue = MouthCheckNcnn(detectSquareImg, 0.3, ssdMouthRect);
            if (mouthValue == 1) {
                driverStatus.pop_front();
                driverStatus.push_back(3);
                //cout<<"normal---openMouth openMouth openMouth openMouth"<<endl;
                //cv::putText(frame, "openouth", cv::Point(20, 20), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
                return YAWN_ALARM;
            }
        }
        if (2 == imageType) //横图
        {
            //检测方法验证
            detectSquareRect.y = 0;
            detectSquareRect.height = 715;
            detectSquareRect.x = faceRect.x - (int) (0.5 * faceRect.width);
            if (detectSquareRect.x < 0) {
                detectSquareRect.x = 0;
            }
            tmpWidth = DETECT_FACE_WIDTH - detectSquareRect.x;
            if (tmpWidth > 720) {
                detectSquareRect.width = 715;
                detectSquareImg = frame(detectSquareRect).clone();
            } else {
                detectSquareRect.x = 180;
                detectSquareRect.width = 715;
                detectSquareImg = frame(detectSquareRect).clone();
            }

            Rect ssdMouthRect;
            ssdMouthRect.x = faceRect.x - detectSquareRect.x - (int) (0.2 * faceRect.width);
            if (ssdMouthRect.x < 0) {
                ssdMouthRect.x = 0;
            }
            ssdMouthRect.y = faceRect.y + (int) (0.3 * faceRect.height);
            ssdMouthRect.width = (int) (1.4 * faceRect.width);
            ssdMouthRect.height = (int) (0.8 * faceRect.height);
            AsertROI(ssdMouthRect, 715, 715);

            //cv::rectangle(detectSquareImg, ssdMouthRect, cv::Scalar(255, 0, 0), 1);
            //drawFrame(detectSquareImg, BGR);
            //sleep(1);


            int mouthValue = MouthCheckNcnn(detectSquareImg, algoParam.yawnConfidence,
                                            ssdMouthRect);
            if (mouthValue == 1) {
                driverStatus.pop_front();
                driverStatus.push_back(3);
                //cout<<"normal---openMouth openMouth openMouth openMouth"<<endl;
                //cv::putText(frame, "openouth", cv::Point(20, 20), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
                return YAWN_ALARM;
            }
        }

    }
    //存在打哈欠
    if (predictText == "OpenMouth") {
        return NRL_FACE;
    }

    //第三步：左顾右盼，抬头低头
    resize(detectFace, classifyFrame, Size(32, 32));
    std::vector<Prediction> leftRightPredictions = classifierAtten.Classify(classifyFrame);
    predictScore = -0.1;
    for (size_t i = 0; i < leftRightPredictions.size(); ++i) {
        Prediction p = leftRightPredictions[i];
        if (p.second > predictScore) {
            predictScore = p.second;
            predictText = p.first;
        }
    }

    if (predictText == "Right" && predictScore > algoParam.attenRightScore) {
        AddAction(ATTEN_ACTION_TYPE, 1);
        AddAction(BLOCKGLASS_TYPE, 0);
        AddAction(EYE_ACTION_TYPE, 0);
    } else if (predictText == "Left" && predictScore > algoParam.attenLeftScore) {
        AddAction(ATTEN_ACTION_TYPE, 1);
        AddAction(BLOCKGLASS_TYPE, 0);
        AddAction(EYE_ACTION_TYPE, 0);
    } else {
        AddAction(ATTEN_ACTION_TYPE, 0);
    }

    //上下左右
    /*std::vector<Prediction> lrudPredictions = classifierLRUD.Classify(classifyFrame);
    predictScore = -0.1;
    for (size_t i = 0; i < lrudPredictions.size(); ++i) {
        Prediction p = lrudPredictions[i];
        if (p.second > predictScore) {
            predictScore = p.second;
            predictText = p.first;
        }
    }

    if (predictText == "Up" && predictScore > algoParam.attenUpScore)
    {
        AddAction(ATTEN_ACTION_TYPE, 1);
    }
    else if (predictText == "Down" && predictScore > algoParam.attenDownScore)
    {
        AddAction(ATTEN_ACTION_TYPE, 1);
    }
    else if (predictText == "Right" && predictScore > algoParam.attenRightScore)
    {
        AddAction(ATTEN_ACTION_TYPE, 1);
    }
    else if (predictText == "Left" && predictScore > algoParam.attenLeftScore)
    {
        AddAction(ATTEN_ACTION_TYPE, 1);
    }else
    {
        AddAction(ATTEN_ACTION_TYPE, 0);
    }*/

    /*char mouth[25];
    sprintf(mouth, "%s : %.2f", predictText.c_str(), predictScore);
    putText(frame, mouth, cv::Point(450, 120), CV_FONT_ITALIC, 1, Scalar(0, 0, 255), 2);*/


    if (CheckActionResult(ATTEN_ACTION_TYPE) > 0) {
        ClearAllResult();

        if (demoMode == 1)//演示模式
        {
            return ATTEN_ALARM;
        }

        if (1 == imageType) //竖图
        {
            //检测方法验证
            detectSquareRect.x = 0;
            detectSquareRect.y = faceRect.y;
            detectSquareRect.width = 719;
            tmpHeight = 1279 - faceRect.y;
            if (tmpHeight > 720) {
                detectSquareRect.height = 719;
                //frame(detectSquareRect).copyTo(detectSquareImg);
                detectSquareImg = frame(detectSquareRect).clone();
            } else {
                detectSquareRect.y = 559;
                detectSquareRect.height = 719;
                //frame(detectSquareRect).copyTo(detectSquareImg);
                detectSquareImg = frame(detectSquareRect).clone();
            }

            Rect ssdLRRect = faceRect;
            ssdLRRect.y = faceRect.y - detectSquareRect.y;
            AsertROI(ssdLRRect, 719, 719);

            int lrFlag = LRCheckNcnn(detectSquareImg, 0.3, ssdLRRect);
            if ((lrFlag == 3) || (lrFlag == 2)) {
                Mat lrCheckImg;
                int imgheight = smallFrame.rows;
                int imgwidth = smallFrame.cols;
                int maxl = imgheight > imgwidth ? imgheight : imgwidth;

                int paddingleft = (maxl - imgwidth) >> 1;
                int paddingright = (maxl - imgwidth) >> 1;
                int paddingbottom = (maxl - imgheight) >> 1;
                int paddingtop = (maxl - imgheight) >> 1;
                copyMakeBorder(smallFrame, lrCheckImg, paddingtop, paddingbottom, paddingleft,
                               paddingright, cv::BORDER_CONSTANT, 0);
                faceObjects.clear();
                faceClasses.clear();
                faceConfidences.clear();
                detectFaceSlim.detectObjNcnn(lrCheckImg, faceObjects, faceClasses, faceConfidences);

                if (faceClasses.size() > 0) {
                    //取出概率最大的脸
                    int IdProMax = 0;
                    float scoreMax = faceConfidences[0];
                    for (int i = 1; i < faceClasses.size(); i++) {
                        if (faceConfidences[i] > scoreMax) {
                            IdProMax = i;
                        }
                    }
                    float faceRatio =
                            1.0 * faceObjects[IdProMax].height / faceObjects[IdProMax].width;
                    if (faceRatio > 1.45) {
                        driverStatus.pop_front();
                        driverStatus.push_back(4);
                        return ATTEN_ALARM; //注意力分散
                    } else {
                        return NRL_FACE;
                    }
                }
                //cv::putText(frame, "attention", cv::Point(20, 20), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
                //return ATTEN_ALARM; //注意力分散
            }
        }
        if (2 == imageType) //横图
        {
            //检测方法验证
            Rect ssdLRRect = faceRect;
            Rect lrROI(0, 0, 600, 600);
            if (faceRect.width < 300) {
                lrROI.x = faceRect.x - 150;
                if (lrROI.x < 0) {
                    lrROI.x = 0;
                }
                lrROI.y = faceRect.y;

                int tmpH = 715 - lrROI.y;
                if (tmpH < 600) {
                    lrROI.y = 115;
                }

                int tmpW = DETECT_FACE_WIDTH - lrROI.x;
                if (tmpW < 600) {
                    lrROI.x = 290;
                }

                detectSquareImg = frame(lrROI).clone();

                ssdLRRect.x = faceRect.x - lrROI.x;
                ssdLRRect.y = faceRect.y - lrROI.y;
                AsertROI(ssdLRRect, DETECT_WIDTH, DETECT_HEIGHT);
            } else {
                detectSquareRect.y = 0;
                detectSquareRect.height = 715;
                detectSquareRect.x = faceRect.x - (int) (0.5 * faceRect.width);
                if (detectSquareRect.x < 0) {
                    detectSquareRect.x = 0;
                }
                tmpWidth = DETECT_FACE_WIDTH - detectSquareRect.x;
                if (tmpWidth > 720) {
                    detectSquareRect.width = 715;
                    detectSquareImg = frame(detectSquareRect).clone();
                } else {
                    detectSquareRect.x = 180;
                    detectSquareRect.width = 715;
                    detectSquareImg = frame(detectSquareRect).clone();
                }

                ssdLRRect.x = faceRect.x - detectSquareRect.x;
                AsertROI(ssdLRRect, 715, 715);
            }

            int lrFlag = LRCheckNcnn(detectSquareImg, 0.25, ssdLRRect);
            if ((lrFlag == 3) || (lrFlag == 2)) {
                //cv::putText(frame, "attention", cv::Point(20, 20), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);

                Mat lrCheckImg;
                int imgheight = smallFrame.rows;
                int imgwidth = smallFrame.cols;
                int maxl = imgheight > imgwidth ? imgheight : imgwidth;

                int paddingleft = (maxl - imgwidth) >> 1;
                int paddingright = (maxl - imgwidth) >> 1;
                int paddingbottom = (maxl - imgheight) >> 1;
                int paddingtop = (maxl - imgheight) >> 1;
                copyMakeBorder(smallFrame, lrCheckImg, paddingtop, paddingbottom, paddingleft,
                               paddingright, cv::BORDER_CONSTANT, 0);
                faceObjects.clear();
                faceClasses.clear();
                faceConfidences.clear();
                detectFaceSlim.detectObjNcnn(lrCheckImg, faceObjects, faceClasses, faceConfidences);

                if (faceClasses.size() > 0) {
                    //取出概率最大的脸
                    int IdProMax = 0;
                    float scoreMax = faceConfidences[0];
                    for (int i = 1; i < faceClasses.size(); i++) {
                        if (faceConfidences[i] > scoreMax) {
                            IdProMax = i;
                        }
                    }
                    float faceRatio =
                            1.0 * faceObjects[IdProMax].height / faceObjects[IdProMax].width;
                    if (faceRatio > 1.45) {
                        driverStatus.pop_front();
                        driverStatus.push_back(4);
                        return ATTEN_ALARM; //注意力分散
                    }
                }
            }
        }


    }

    if (predictText == "Right" || predictText == "Left") {
        return NRL_FACE;
    }

    /*if (predictText == "Right" || predictText == "Left" ||
            predictText == "Up"|| predictText == "Down") {
        AddAction(EYE_ACTION_TYPE, 0);
        AddAction(BLINK_TYPE, 0);
        AddAction(BLOCKGLASS_TYPE, 0);
        return NRL_FACE;
    }*/

    //第四步：闭眼、眨眼、红外阻断
    //第四步： 检测是否闭眼
    Mat eyeImg = detectFace(Rect(0, (int) (0.125 * detectFace.rows), detectFace.cols,
                                 (int) (0.5 * detectFace.rows)));
    resize(eyeImg, classifyFrame, Size(32, 32));
    std::vector<Prediction> eyePredictions = classifierEye.Classify(classifyFrame);
    predictScore = -0.1;
    for (size_t i = 0; i < eyePredictions.size(); ++i) {
        Prediction p = eyePredictions[i];
        if (p.second > predictScore) {
            predictScore = p.second;
            predictText = p.first;
        }
    }

    if (predictText == "CloseEye" && predictScore > algoParam.eyeClassifyScore) //闭眼
    {
        AddAction(EYE_ACTION_TYPE, 1);
        AddAction(BLOCKGLASS_TYPE, 0);
    } else if (predictText == "BlackGlass" && predictScore > 0.7) //红外阻断
    {
        AddAction(BLOCKGLASS_TYPE, 1);
        AddAction(EYE_ACTION_TYPE, 0);
    } else {
        AddAction(EYE_ACTION_TYPE, 0);
        AddAction(BLOCKGLASS_TYPE, 0);
    }

    //红外阻断报警
    if (CheckActionResult(BLOCKGLASS_TYPE) > 0) {
        ClearAllResult();

        if (1 == imageType) //竖图
        {
            Mat faceROI = frame(faceRect).clone();
            Mat eyeROI = faceROI(Rect((int) (0.15 * faceROI.cols), (int) (0.2 * faceROI.rows),
                                      (int) (0.7 * faceROI.cols), (int) (0.28 * faceROI.rows)));
            Mat eyeROIGray;
            cvtColor(eyeROI, eyeROIGray, CV_BGR2GRAY);
            int blackCnt = 0;

            for (int i = 0; i < eyeROIGray.rows; i++) {
                uchar *data = eyeROIGray.ptr<uchar>(i);

                for (int j = 0; j < eyeROIGray.cols; j++) {
                    if (data[j] < 50) {
                        blackCnt++;
                    }
                }
            }
            int thresholdCnt = (int) (0.6 * eyeROIGray.rows * eyeROIGray.cols);

            if (blackCnt > thresholdCnt) {
                detectSquareRect.x = 0;
                detectSquareRect.y = faceRect.y;
                detectSquareRect.width = 719;
                tmpHeight = 1279 - faceRect.y;
                if (tmpHeight > 720) {
                    detectSquareRect.height = 719;
                    detectSquareImg = frame(detectSquareRect).clone();
                } else {
                    detectSquareRect.y = 559;
                    detectSquareRect.height = 719;
                    detectSquareImg = frame(detectSquareRect).clone();
                }

                Rect ssdEyeRect = faceRect;
                ssdEyeRect.y = faceRect.y - detectSquareRect.y;
                AsertROI(ssdEyeRect, 719, 719);

                int eyeNum = InfraredCheck(detectSquareImg, algoParam.eyeConfidence, ssdEyeRect);
                if (eyeNum == 1) {
                    driverStatus.pop_front();
                    driverStatus.push_back(5);
                    return INFRARED_ALARM;
                }
            }
        }
        if (2 == imageType) //横图
        {
            Mat faceROI = frame(faceRect).clone();
            Mat eyeROI = faceROI(Rect((int) (0.15 * faceROI.cols), (int) (0.2 * faceROI.rows),
                                      (int) (0.7 * faceROI.cols), (int) (0.28 * faceROI.rows)));
            Mat eyeROIGray;
            cvtColor(eyeROI, eyeROIGray, CV_BGR2GRAY);
            int blackCnt = 0;

            for (int i = 0; i < eyeROIGray.rows; i++) {
                uchar *data = eyeROIGray.ptr<uchar>(i);

                for (int j = 0; j < eyeROIGray.cols; j++) {
                    if (data[j] < 50) {
                        blackCnt++;
                    }
                }
            }
            int thresholdCnt = (int) (0.6 * eyeROIGray.rows * eyeROIGray.cols);

            if (blackCnt > thresholdCnt) {
                Rect ssdEyeRect = faceRect;

                Rect eyeROI(0, 0, 600, 600);
                if (faceRect.width < 300) {
                    eyeROI.x = faceRect.x - 150;
                    if (eyeROI.x < 0) {
                        eyeROI.x = 0;
                    }
                    eyeROI.y = faceRect.y;

                    int h = 715 - eyeROI.y;
                    if (h < 600) {
                        eyeROI.y = 115;
                    }

                    int w = DETECT_FACE_WIDTH - eyeROI.x;
                    if (w < 600) {
                        eyeROI.x = 290;
                    }

                    detectSquareImg = frame(eyeROI).clone();

                    ssdEyeRect.x = faceRect.x - eyeROI.x;
                    ssdEyeRect.y = faceRect.y - eyeROI.y;
                    AsertROI(ssdEyeRect, DETECT_WIDTH, DETECT_HEIGHT);
                } else {
                    detectSquareRect.y = 0;
                    detectSquareRect.height = 715;
                    detectSquareRect.x = faceRect.x - (int) (0.5 * faceRect.width);
                    if (detectSquareRect.x < 0) {
                        detectSquareRect.x = 0;
                    }
                    tmpWidth = DETECT_FACE_WIDTH - detectSquareRect.x;
                    if (tmpWidth > 720) {
                        detectSquareRect.width = 715;
                        detectSquareImg = frame(detectSquareRect).clone();
                    } else {
                        detectSquareRect.x = 180;
                        detectSquareRect.width = 715;
                        detectSquareImg = frame(detectSquareRect).clone();
                    }

                    ssdEyeRect.x = faceRect.x - detectSquareRect.x;
                    AsertROI(ssdEyeRect, 715, 715);
                }

                int eyeNum = InfraredCheck(detectSquareImg, algoParam.eyeConfidence, ssdEyeRect);
                if (eyeNum == 1) {
                    driverStatus.pop_front();
                    driverStatus.push_back(5);
                    return INFRARED_ALARM;
                }
            }
        }

    }

    /*if (CheckActionResult(BLINK_TYPE) > 0)
{
    detectSquareRect.x = 0;
    detectSquareRect.y = faceRect.y;
    detectSquareRect.width = 719;
    tmpHeight = 1279 - faceRect.y;
    if (tmpHeight > 720)
    {
        detectSquareRect.height = 719;
        detectSquareImg = frame(detectSquareRect).clone();
    }
    else
    {
        detectSquareRect.y = 559;
        detectSquareRect.height = 719;
        detectSquareImg = frame(detectSquareRect).clone();
    }

    Rect ssdBlinkEyeRect = faceRect;
    ssdBlinkEyeRect.y = faceRect.y - detectSquareRect.y;
    AsertROI(ssdBlinkEyeRect, 719, 719);

    int closeEyeFlag = CloseEyeCheckNcnn(detectSquareImg, algoParam.yawnConfidence, ssdBlinkEyeRect);
    if (closeEyeFlag == 1)
    {
        return BLINK_ALARM; //眨眼报警
    }
}*/





    if (CheckActionResult(EYE_ACTION_TYPE) > 0) {
        ClearAllResult();

        //cv::putText(frame, "closeEye", cv::Point(20, 20), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
        //return EYES_ALARM;

        if (demoMode == 1)//演示模式
        {
            return EYES_ALARM; //闭眼报警
        }

        if (1 == imageType) //竖图
        {
            int min_value = *min_element(faceRectX.begin(), faceRectX.end());
            int max_value = *max_element(faceRectX.begin(), faceRectX.end());
            //LOGE("max_value=%d,min_value=%d",max_value,min_value);
            if ((max_value - min_value) > 100) {
                return NRL_FACE;
            }
            //eye detect
            detectSquareRect.x = 0;
            detectSquareRect.y = faceRect.y;
            detectSquareRect.width = 719;
            tmpHeight = 1279 - faceRect.y;
            if (tmpHeight > 720) {
                detectSquareRect.height = 719;
                //frame(detectSquareRect).copyTo(detectSquareImg);
                detectSquareImg = frame(detectSquareRect).clone();
            } else {
                detectSquareRect.y = 559;
                detectSquareRect.height = 719;
                //frame(detectSquareRect).copyTo(detectSquareImg);
                detectSquareImg = frame(detectSquareRect).clone();
            }

            Rect ssdEyeRect = faceRect;
            ssdEyeRect.y = faceRect.y - detectSquareRect.y;
            AsertROI(ssdEyeRect, 719, 719);

            int closeEyeFlag = CloseEyeCheckNcnn(detectSquareImg, 0.3, ssdEyeRect);
            if (closeEyeFlag == 1) {
                driverStatus.pop_front();
                driverStatus.push_back(6);

                if (frameIdx - tmpEyeCnt < 9000) {
                    tmpEyeCnt = frameIdx;
                    eyeNumStatus++;
                } else {
                    tmpEyeCnt = frameIdx;
                }
                if (eyeNumStatus > 1) {
                    eyeNumStatus = 0;
                    int openMouthCnt = 0;
                    int closeEyeCnt = 0;
                    for (deque<int>::iterator it = driverStatus.begin();
                         it != driverStatus.end(); it++) {
                        if ((*it) == 1 || (*it) == 2) {
                            return NRL_FACE;
                        }
                        if ((*it) == 3) {
                            openMouthCnt++;
                        }
                        if ((*it) == 6) {
                            closeEyeCnt++;
                        }
                    }

                    if (openMouthCnt >= 1 && closeEyeCnt >= 2) {
                        return EYES_ALARM; //闭眼报警
                    } else {
                        return NRL_FACE;
                    }
                } else {
                    driverStatus.pop_front();
                    driverStatus.push_back(0);
                    return NRL_FACE;
                }
            }
        }
        if (2 == imageType) //横图
        {
            //eye detect
            Rect ssdEyeRect = faceRect;

            Rect eyeROI(0, 0, 600, 600);
            if (faceRect.width < 300) {
                eyeROI.x = faceRect.x - 150;
                if (eyeROI.x < 0) {
                    eyeROI.x = 0;
                }
                eyeROI.y = faceRect.y;

                int h = 715 - eyeROI.y;
                if (h < 600) {
                    eyeROI.y = 115;
                }

                int w = DETECT_FACE_WIDTH - eyeROI.x;
                if (w < 600) {
                    eyeROI.x = 290;
                }

                detectSquareImg = frame(eyeROI).clone();

                ssdEyeRect.x = faceRect.x - eyeROI.x;
                ssdEyeRect.y = faceRect.y - eyeROI.y;
                AsertROI(ssdEyeRect, DETECT_WIDTH, DETECT_HEIGHT);
            } else {
                detectSquareRect.y = 0;
                detectSquareRect.height = 715;
                detectSquareRect.x = faceRect.x - (int) (0.5 * faceRect.width);
                if (detectSquareRect.x < 0) {
                    detectSquareRect.x = 0;
                }
                tmpWidth = DETECT_FACE_WIDTH - detectSquareRect.x;
                if (tmpWidth > 720) {
                    detectSquareRect.width = 715;
                    detectSquareImg = frame(detectSquareRect).clone();
                } else {
                    detectSquareRect.x = 180;
                    detectSquareRect.width = 715;
                    detectSquareImg = frame(detectSquareRect).clone();
                }

                ssdEyeRect.x = faceRect.x - detectSquareRect.x;
                AsertROI(ssdEyeRect, 715, 715);
            }

            /*cv::rectangle(detectSquareImg, ssdEyeRect, cv::Scalar(255, 255, 0), 1);
            drawFrame(detectSquareImg, BGR);
            sleep(1);*/

            int closeEyeFlag = CloseEyeCheckNcnn(detectSquareImg, algoParam.eyeConfidence,
                                                 ssdEyeRect);
            if (closeEyeFlag == 1) {
                driverStatus.pop_front();
                driverStatus.push_back(6);

                if (frameIdx - tmpEyeCnt < 3000) {
                    tmpEyeCnt = frameIdx;
                    eyeNumStatus++;
                } else {
                    tmpEyeCnt = frameIdx;
                }
                if (eyeNumStatus > 1) {
                    eyeNumStatus = 0;
                    int openMouthCnt = 0;
                    int closeEyeCnt = 0;
                    for (deque<int>::iterator it = driverStatus.begin();
                         it != driverStatus.end(); it++) {
                        if ((*it) == 1 || (*it) == 2) {
                            return NRL_FACE;
                        }
                        if ((*it) == 3) {
                            openMouthCnt++;
                        }
                        if ((*it) == 6) {
                            closeEyeCnt++;
                        }
                    }

                    if (openMouthCnt >= 1 && closeEyeCnt >= 2) {
                        return EYES_ALARM; //闭眼报警
                    } else {
                        return NRL_FACE;
                    }
                } else {
                    driverStatus.pop_front();
                    driverStatus.push_back(0);
                    return NRL_FACE;
                }
            }
        }

    }
/*if(tmpCloseEyeCnt > stBlink.alarmCnt)
{
return BLINK_ALARM; //眨眼报警
}

if (CheckActionResult(BLOCKGLASS_TYPE) > 0)
{
ClearAllResult();
return BLOCKGLASS_ALARM;
}*/


    return rstFlag;
}


//单帧增加报警信息
void Algo::AddAction(int flag, int num) {
    LOGI("AddAction: %d, num: %d", flag, num);
    pAction = NULL;
    switch (flag) {
        case EYE_ACTION_TYPE:
            pAction = &stEyeAction;
            break;
        case YAWN_ACTION_TYPE:
            pAction = &stYawnAction;
            break;
        case ATTEN_ACTION_TYPE:
            pAction = &stAttenAction;
            break;
        case SMOKE_CLASSIFY_TYPE:
            pAction = &stSmokeClassify;
            break;
        case CALL_CLASSIFY_TYPE:
            pAction = &stCallClassify;
            break;
        case FACE_ACTION_TYPE:
            pAction = &stFaceAction;
            break;
        case PHONE_TYPE:
            pAction = &stPhone;
            break;
        case CIGARETTE_TYPE:
            pAction = &stCigarette;
            break;
        case BLOCKGLASS_TYPE:
            pAction = &stBlockGlass;
            break;
        case BLINK_TYPE:
            pAction = &stBlink;
            break;
        case COVER_TYPE:
            pAction = &stCover;
            break;
        case WHEEL_TYPE:
            pAction = &stWheel;
            break;
    }

    if (pAction == NULL)
        return;

    pAction->action.insert(pAction->action.end(), num);

    while (pAction->action.size() > (uint) pAction->dataSize) {
        pAction->action.erase(pAction->action.begin());
    }

    //LOGE("type = %d, zongshu = %d , tongji=%d    %d", flag, pAction->dataSize, pAction->action.size(),num);
}


//检查是否到达报警阈值
int Algo::CheckActionResult(int flag) {
    int actionCnt = 0;
    pAction = NULL;

    switch (flag) {
        case EYE_ACTION_TYPE:
            pAction = &stEyeAction;
            break;
        case YAWN_ACTION_TYPE:
            pAction = &stYawnAction;
            break;
        case ATTEN_ACTION_TYPE:
            pAction = &stAttenAction;
            break;
        case SMOKE_CLASSIFY_TYPE:
            pAction = &stSmokeClassify;
            break;
        case CALL_CLASSIFY_TYPE:
            pAction = &stCallClassify;
            break;
        case FACE_ACTION_TYPE:
            pAction = &stFaceAction;
            break;
        case PHONE_TYPE:
            pAction = &stPhone;
            break;
        case CIGARETTE_TYPE:
            pAction = &stCigarette;
            break;
        case BLOCKGLASS_TYPE:
            pAction = &stBlockGlass;
            break;
        case BLINK_TYPE:
            pAction = &stBlink;
            break;
        case COVER_TYPE:
            pAction = &stCover;
            break;
        case WHEEL_TYPE:
            pAction = &stWheel;
            break;
    }

    LOGE("tracking pAction actionSize: %d, dataSize: %d", pAction->action.size(), pAction->dataSize);
    if (pAction == NULL || (pAction->action.size() < (unsigned int) pAction->dataSize)) {
        LOGE("tracking pAction count: %d", actionCnt);
        return actionCnt;
    }

    for (int i = 0; i < pAction->dataSize; i++) {
        actionCnt += pAction->action[i];
    }

    //if(flag == 3)
    //LOGE("leixing = %d, zongshu = %d,size=%d, tongji = %d, alarm=%d", flag, pAction->dataSize, pAction->action.size() , actionCnt, pAction->alarmCnt);

    if (pAction->alarmCnt < 1) {
        pAction->alarmCnt = 1;
    }

    LOGE("tracking actionCount: %d, alarmCnt: %d", actionCnt, pAction->alarmCnt);
    if (actionCnt >= pAction->alarmCnt) {
        return 1;
    }
    return 0;
}


void Algo::ClearAllResult() {
    pAction = NULL;
    pAction = &stEyeAction;
    pAction->action.clear();

    pAction = &stYawnAction;
    pAction->action.clear();

    pAction = &stAttenAction;
    pAction->action.clear();

    pAction = &stSmokeClassify;
    pAction->action.clear();

    pAction = &stCallClassify;
    pAction->action.clear();

    pAction = &stFaceAction;
    pAction->action.clear();

    pAction = &stPhone;
    pAction->action.clear();

    pAction = &stCigarette;
    pAction->action.clear();

    pAction = &stBlockGlass;
    pAction->action.clear();

    pAction = &stBlink;
    pAction->action.clear();

    pAction = &stCover;
    pAction->action.clear();

    pAction = &stWheel;
    pAction->action.clear();
}

//清空报警队列
void Algo::ClearActionResult(int flag) {
    pAction = NULL;
    switch (flag) {
        case EYE_ACTION_TYPE:
            pAction = &stEyeAction;
            break;
        case YAWN_ACTION_TYPE:
            pAction = &stYawnAction;
            break;
        case ATTEN_ACTION_TYPE:
            pAction = &stAttenAction;
            break;
        case SMOKE_CLASSIFY_TYPE:
            pAction = &stSmokeClassify;
            break;
        case CALL_CLASSIFY_TYPE:
            pAction = &stCallClassify;
            break;
        case FACE_ACTION_TYPE:
            pAction = &stFaceAction;
            break;
        case PHONE_TYPE:
            pAction = &stPhone;
            break;
        case CIGARETTE_TYPE:
            pAction = &stCigarette;
            break;
        case BLOCKGLASS_TYPE:
            pAction = &stBlockGlass;
            break;
        case BLINK_TYPE:
            pAction = &stBlink;
            break;
        case COVER_TYPE:
            pAction = &stCover;
            break;
        case WHEEL_TYPE:
            pAction = &stWheel;
            break;
    }
    pAction->action.clear();
}


//ncnn
int Algo::FaceCheckNcnn(Mat frame, float confidence) {
    int existPerson = 0;
    objects.clear();
    classes.clear();
    confidences.clear();
    detectNcnn.detectObjNcnn(frame, objects, classes, confidences);

    for (int i = 0; i < classes.size(); i++) {
        if (confidences[i] > confidence) {
            if ((classes[i] == ObjectType::EYE) || (classes[i] == ObjectType::CLOSED) ||
                (classes[i] == ObjectType::CIGARETTE)) {
                existPerson = 1;
            }
        }

        if ((classes[i] == ObjectType::MOUTH) && (confidences[i] > 0.5)) {
            existPerson = 1;
        }
        if ((classes[i] == ObjectType::HAND) && (confidences[i] > 0.5)) {
            existPerson = 1;
        }

    }
    if (existPerson == 1) {
        return 0;
    } else {
        return 1;//没有人脸
    }
}

int Algo::SmokeCheckNcnn(Mat frame, float confidence, cv::Rect ROIRect) {
    int handssdFlag = 0;
    int cigarettessdFlag = 0;
    int phonessdFlag = 0;
    Rect smokeRect;
    Rect handRect;
    float tmpSmokeConf = 0.0;
    float tmpHandConf = 0.0;

    objects.clear();
    classes.clear();
    confidences.clear();
    detectNcnn.detectObjNcnn(frame, objects, classes, confidences);

    for (int i = 0; i < classes.size(); i++) {
        if (classes[i] == ObjectType::HAND) //手的概率提高
        {
            if (confidences[i] > 0.6) {
                handssdFlag = 1;
                if (confidences[i] > tmpHandConf) {
                    tmpHandConf = confidences[i];
                    handRect = objects[i];
                }
            }
        }

        if (classes[i] == ObjectType::CIGARETTE) {
            cigarettessdFlag = 1;
            if (confidences[i] > tmpSmokeConf) {
                tmpSmokeConf = confidences[i];
                smokeRect = objects[i];
            }
        }

        if ((classes[i] == ObjectType::PHONE) && (confidences[i] > 0.6)) {
            if ((ROIRect.y < objects[i].y)
                && (ROIRect.x < objects[i].x)
                && ((objects[i].x + objects[i].width) < (ROIRect.x + ROIRect.width + 10))
                && ((objects[i].y + objects[i].height) < (ROIRect.y + ROIRect.height + 10))) {
                phonessdFlag = 1;
            }
        }
    }

    if ((ROIRect.y > smokeRect.y) || (ROIRect.x > smokeRect.x)
        || ((smokeRect.x + smokeRect.width) > (ROIRect.x + ROIRect.width + 10))
        || ((smokeRect.y + smokeRect.height) > (ROIRect.y + ROIRect.height + 10))) {
        if (handssdFlag == 1) {
            return 3;
        }
        return 0;
    }

    if ((ROIRect.y > handRect.y) || (ROIRect.x > handRect.x)
        || ((handRect.x + handRect.width) > (ROIRect.x + ROIRect.width + 10))) {
        if (handssdFlag == 1) {
            return 3;
        }
        return 0;
    }

    //戒指
    if ((smokeRect.x > handRect.x) &&
        (smokeRect.x + smokeRect.width < handRect.x + handRect.width)) {
        if ((smokeRect.y > handRect.y) &&
            (smokeRect.y + smokeRect.height < handRect.y + handRect.height)) {
            return 0;
        }
    }

    //耳机线
    if ((handRect.y + (int) (0.7 * handRect.height)) < (smokeRect.y + smokeRect.height)) {
        if ((handRect.x + (int) (0.8 * handRect.width)) < smokeRect.x) {
            return 0;
        }
    }

    if ((handRect.x + (int) (1.4 * handRect.width)) < smokeRect.x) {
        return 0;
    }

    if (handRect.x > smokeRect.x + (int) (0.5 * smokeRect.width)) {
        if ((smokeRect.y > handRect.y) && (smokeRect.y < handRect.y + handRect.height)) {
            return 0;
        }
    }

    if ((handssdFlag == 1) && (cigarettessdFlag == 1) && (phonessdFlag == 0) &&
        (tmpSmokeConf > confidence)) {
        return 1;
    }
    if ((handssdFlag == 1) && (phonessdFlag == 1) && (cigarettessdFlag == 0)) {
        return 2;
    }
    if (handssdFlag == 1) {
        return 3;
    }

    return 0;
}


int Algo::CallCheckNcnn(Mat frame, float confidence, cv::Rect ROIRect) {
    int handssdFlag = 0;
    int cigarettessdFlag = 0;
    int phonessdFlag = 0;
    Rect phoneRect;
    Rect handRect;
    float tmpPhoneConf = 0.0;
    float tmpHandConf = 0.0;

    objects.clear();
    classes.clear();
    confidences.clear();
    detectNcnn.detectObjNcnn(frame, objects, classes, confidences);

    for (int i = 0; i < classes.size(); i++) {
        if (classes[i] == ObjectType::HAND) {
            if (confidences[i] > 0.6) {
                handssdFlag = 1;

                if (confidences[i] > tmpHandConf) {
                    tmpHandConf = confidences[i];
                    handRect = objects[i];
                }
            }
        }

        if (classes[i] == ObjectType::PHONE) {
            phonessdFlag = 1;
            if (confidences[i] > tmpPhoneConf) {
                tmpPhoneConf = confidences[i];
                phoneRect = objects[i];
            }
        }


        if ((classes[i] == ObjectType::CIGARETTE) && (confidences[i] > 0.6)) {
            if ((ROIRect.y < objects[i].y) && (ROIRect.x < objects[i].x)
                && ((objects[i].x + objects[i].width) < (ROIRect.x + ROIRect.width + 10))
                && ((objects[i].y + objects[i].height) < (ROIRect.y + ROIRect.height + 10))) {
                cigarettessdFlag = 1;
            }
        }
    }

    if ((ROIRect.y > phoneRect.y) || (ROIRect.x > phoneRect.x)
        || ((phoneRect.x + phoneRect.width) > (ROIRect.x + ROIRect.width + 10))
        || ((phoneRect.y + phoneRect.height) > (ROIRect.y + ROIRect.height + 10))) {
        if (handssdFlag == 1) {
            return 3;
        }
        return 0;
    }
    if ((ROIRect.y > handRect.y) || (ROIRect.x > handRect.x)
        || ((handRect.x + handRect.width) > (ROIRect.x + ROIRect.width + 10))
        || ((handRect.y + handRect.height) > (ROIRect.y + ROIRect.height + 10))) {
        if (handssdFlag == 1) {
            return 3;
        }
        return 0;
    }
    if ((handssdFlag == 1) && (cigarettessdFlag == 1) && (phonessdFlag == 0)) {
        return 1;
    }
    if ((handssdFlag == 1) && (phonessdFlag == 1) && (cigarettessdFlag == 0) &&
        (tmpPhoneConf > confidence)) {
        return 2;
    }
    if (handssdFlag == 1) {
        return 3;
    }
    return 0;
}

int Algo::MouthCheckNcnn(Mat frame, float confidence, cv::Rect ROIRect) {
    objects.clear();
    classes.clear();
    confidences.clear();
    detectNcnn.detectObjNcnn(frame, objects, classes, confidences);
    float tmpF = 0.0;
    float w = 0.0;
    float h = 0.0;
    Rect mouthRect;

    for (int i = 0; i < classes.size(); i++) {
        if (confidences[i] > 0.6) {
            //检测到手、烟、手机，直接返回，未知报警（喝水、叼烟、吃东西等）
            if (classes[i] == ObjectType::HAND || classes[i] == ObjectType::CIGARETTE) {
                //return 2;
            }
        }

        //取出概率最高的嘴，使用宽高比阈值
        if (classes[i] == ObjectType::MOUTH) {
            if (confidences[i] > tmpF) {
                tmpF = confidences[i];
                mouthRect = objects[i];
            }
        }
    }

    if ((ROIRect.y > mouthRect.y) || (ROIRect.x > mouthRect.x)
        || ((mouthRect.x + mouthRect.width) > (ROIRect.x + ROIRect.width + 10))
        || ((mouthRect.y + mouthRect.height) > (ROIRect.y + ROIRect.height + 10))) {
        // return 0;
    }

    if ((mouthRect.height < 1) || (mouthRect.width < 1))//没有检测到嘴
    {
        LOGE("no mouth");
        return 0;
    }
    if (((1.0 * mouthRect.height / mouthRect.width) > 1.2) && (tmpF > confidence)) {
        return 1;
    }

    return 0;
}


int Algo::LRCheckNcnn(Mat frame, float confidence, cv::Rect ROIRect) {
    int eyeNum = 0;
    int closedEyeNum = 0;
    int lrMouth = 0;
    int mouthFlag = 0;
    Rect eyeRect;
    Rect mouthRect;
    float tmpMouth = 0.0;
    float tmpEye = 0.0;
    int handFlag = 0;

    objects.clear();
    classes.clear();
    confidences.clear();
    detectNcnn.detectObjNcnn(frame, objects, classes, confidences);

    for (int i = 0; i < classes.size(); i++) {
        if (classes[i] == ObjectType::HAND) {
            if (confidences[i] > 0.6) {
                handFlag = 1;
                return 0;
            }
        }

        if (confidences[i] > confidence) {
            if (classes[i] == ObjectType::EYE) {
                if (confidences[i] > tmpEye) {
                    if ((ROIRect.y < objects[i].y) && (ROIRect.x < objects[i].x)
                        && ((objects[i].x + objects[i].width) < (ROIRect.x + ROIRect.width + 10))
                        &&
                        ((objects[i].y + objects[i].height) < (ROIRect.y + ROIRect.height + 10))) {
                        tmpEye = confidences[i];
                        eyeRect = objects[i];
                        eyeNum++;
                    }
                }
            }
            if (classes[i] == ObjectType::CLOSED) {
                if ((ROIRect.y < objects[i].y) && (ROIRect.x < objects[i].x)
                    && ((objects[i].x + objects[i].width) < (ROIRect.x + ROIRect.width + 10))
                    && ((objects[i].y + objects[i].height) < (ROIRect.y + ROIRect.height + 10))) {
                    closedEyeNum++;
                }
            }
            //取出概率最高的嘴，使用宽高比阈值
            if (classes[i] == ObjectType::MOUTH) {
                if (confidences[i] > tmpMouth) {
                    if ((ROIRect.y < objects[i].y) && (ROIRect.x < objects[i].x)
                        && ((objects[i].x + objects[i].width) < (ROIRect.x + ROIRect.width + 10))
                        &&
                        ((objects[i].y + objects[i].height) < (ROIRect.y + ROIRect.height + 10))) {
                        lrMouth = 1;
                        tmpMouth = confidences[i];
                        mouthRect = objects[i];
                    }
                }
            }
        }
    }

    if (handFlag == 1) //hand
    {
        return 0;
    }
    if (lrMouth == 1) {
        float htow = 1.0 * mouthRect.height / mouthRect.width;
        if (htow < 0.65) {
            return 0;
        }
    } else {
        if ((eyeNum < 2) && (closedEyeNum < 2) && ((eyeNum + closedEyeNum) < 2)) //一只眼睛
        {
            return 3;
        } else {
            return 0;
        }
    }

    if ((handFlag != 1) && (lrMouth == 1)) {
        int mouthCenterX = mouthRect.x + (0.5 * mouthRect.width);
        int mouthCenterXToFL = abs(ROIRect.x - mouthCenterX);
        int mouthCenterXToFR = abs(ROIRect.x + ROIRect.width - mouthCenterX);

        float LeftToRight = 1.0 * mouthCenterXToFL / mouthCenterXToFR;
        if ((LeftToRight < 0.37) || (LeftToRight > 4)) {
            if ((eyeNum < 2) && (closedEyeNum < 2) && ((eyeNum + closedEyeNum) < 2)) //一只眼睛
            {
                return 2;
            }
        }
    }

    return 0;
}


int Algo::CloseEyeCheckNcnn(Mat frame, float confidence, cv::Rect ROIRect) {
    int eyeNum = 0;
    int closedEye = 0;

    objects.clear();
    classes.clear();
    confidences.clear();
    detectNcnn.detectObjNcnn(frame, objects, classes, confidences);

    for (int i = 0; i < classes.size(); i++) {
        if (confidences[i] > confidence) {
            if (confidences[i] > 0.6) {
                //检测到手、烟、手机，直接返回，未知报警（喝水、叼烟、吃东西等）
                if (classes[i] == ObjectType::HAND || classes[i] == ObjectType::CIGARETTE) {
                    return 0;
                }
            }
            if (classes[i] == ObjectType::CLOSED) {
                if ((ROIRect.y < objects[i].y) && (ROIRect.x < objects[i].x)
                    && ((objects[i].x + objects[i].width) < (ROIRect.x + ROIRect.width + 10))
                    && ((objects[i].y + objects[i].height) < (ROIRect.y + ROIRect.height + 10))) {
                    closedEye++;
                }
            }
            if (classes[i] == ObjectType::EYE) {
                if ((ROIRect.y < objects[i].y) && (ROIRect.x < objects[i].x)
                    && ((objects[i].x + objects[i].width) < (ROIRect.x + ROIRect.width + 10))
                    && ((objects[i].y + objects[i].height) < (ROIRect.y + ROIRect.height + 10))) {
                    eyeNum++;
                }
            }
        }
    }

    if (eyeNum > 0) {
        return 0;
    }

    if (closedEye >= 1)//检测到闭合的眼睛，闭眼
    {
        return 1;
    }
    return 0;
}


int Algo::InfraredCheck(Mat frame, float confidence, cv::Rect ROIRect) {
    int eyeNum = 0;
    int closedEye = 0;

    objects.clear();
    classes.clear();
    confidences.clear();
    detectNcnn.detectObjNcnn(frame, objects, classes, confidences);

    for (int i = 0; i < classes.size(); i++) {
        if (confidences[i] > confidence) {
            if (confidences[i] > 0.6) {
                //检测到手、烟、手机，直接返回，未知报警（喝水、叼烟、吃东西等）
                if (classes[i] == ObjectType::HAND || classes[i] == ObjectType::CIGARETTE) {
                    return 0;
                }
            }
            if (classes[i] == ObjectType::CLOSED) {
                if ((ROIRect.y < objects[i].y) && (ROIRect.x < objects[i].x)
                    && ((objects[i].x + objects[i].width) < (ROIRect.x + ROIRect.width + 10))
                    && ((objects[i].y + objects[i].height) < (ROIRect.y + ROIRect.height + 10))) {
                    closedEye++;
                }
            }
            if (classes[i] == ObjectType::EYE) {
                if ((ROIRect.y < objects[i].y) && (ROIRect.x < objects[i].x)
                    && ((objects[i].x + objects[i].width) < (ROIRect.x + ROIRect.width + 10))
                    && ((objects[i].y + objects[i].height) < (ROIRect.y + ROIRect.height + 10))) {
                    eyeNum++;
                }
            }
        }
    }

    if ((eyeNum == 0) && (closedEye == 0)) {
        return 1;
    }

    return 0;
}

int Algo::release() {

}