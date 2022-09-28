package com.dsm.bean;

/**
 * Created by Imx on 2018/9/11.
 */

public interface AlarmState {
    /**
     * 前向DVR
     */
    int TYPE_DVR = 0x00;

    /**
     * 录音
     */
    int SOUND_RECORD = 0x05;

    int ALARM_INX = 0x09;

    /**
     * 闭眼报警
     */
    int DSM_EYE_CLOSE = 0x10;
    /**
     * 打哈欠报警
     */
    int DSM_YWAN = 0x11;
    /**
     * 手机报警
     */
    int DSM_PHONE_CALL = 0x12;
    /**
     * 抽烟报警
     */
    int DSM_SMOKING = 0x13;
    /**
     * 分神驾驶
     */
    int DSM_ATTEN = 0x14;
    /**
     * 驾驶员异常
     */
    int DSM_DRIVER_ABNORMAL = 0x15;

    /**
     * 红外阻断
     */
    int DSM_INFRARED_ALARM = 0x16;

    int DSM_HAND_OFF_ALARM = 0x20;

    int DSM_DRIVER_RECOGNISE = 0x1a;

    int DSM_COVER_ALARM = 0x1d;

    /**
     * 驾驶员摄像头抓拍
     */
    int DSM_AUTO_CAPTURE = 0x1b;
    /**
     * 苏标抓拍
     */
    int DSM_CAPTURE = 0x1c;

    int DSM_RECOGNISE_TIPS = 0xaf;

    int DSM_RECOGNISE_INIT = 0xa0;

    int DSM_RECOGNISE_PASS = 0xa1;

    int DSM_RECOGNISE_ERROR = 0xa2;

    int DSM_DRIVER_CHANGED = 0xa3;

    /**
     * 双手脱离方向盘
     */
    int MONITOR_WHEEL_OFF = 0x21;

    int MONITOR_CAPTURE = 0x2a;

    int MONITOR_AUTO_CAPTURE = 0x2b;

    /**
     * 车距过近
     */
    int ADAS_APPROACH = 0x30;
    /**
     * 前向碰撞
     */
    int ADAS_COLLISION = 0x31;
    /**
     * 车道左偏移
     */
    int ADAS_LANE_LEFT = 0x32;
    /**
     * 车道右偏移
     */
    int ADAS_LANE_RIGHT = 0x33;
    /**
     * 行人碰撞
     */
    int ADAS_PERSON = 0x34;
    /**
     * 频繁变道
     */
    int ADAS_LANE_OFTEN = 0x35;
    /**
     * 道路标识超限报警
     */
    int ADAS_SIGN_ALARM = 0x36;
    /**
     * 障碍物
     */
    int ADAS_OBSTACLE = 0x37;
    /**
     * 道路标识识别事件
     */
    int ADAS_TRAFFIC_SIGN = 0x38;


    /**
     * 前向摄像头抓拍
     */
    int ADAS_AUTO_CAPTURE = 0x3a;
    /**
     * 苏标抓拍
     */
    int ADAS_CAPTURE = 0x3b;

    /**
     * 盲区报警
     */
    int BSD_ALARM = 0x50;
    /**
     * 盲区摄像头抓拍
     */
    int BSD_AUTO_CAPTURE = 0x5a;
    /**
     * 苏标抓拍
     */
    int BSD_CAPTURE = 0x5b;


    int ACTION_DECEL = 0x60;
    /* 急加速 状态 0x8000000 */
    int ACTION_ACCEL = 0x61;
    /* 急转弯 状态 0x20000000 */
    int ACTION_SWERVE = 0x62;
}
