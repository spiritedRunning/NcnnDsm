package com.dsm.bean;

import android.content.ComponentName;

/**
 * Created by Imx on 2018/9/6.
 */

public interface ActionTable {
    int TYPE_ADAS = 0;

    int TYPE_DSM = 1;

    int TYPE_MONITOR = 2;

    int TYPE_BSD = 3;

    int TYPE_APC = 2;

    int TYPE_ACT = 4;

    ComponentName ALARM_SERVICE = new ComponentName("com.tme.alarm", "com.tme.alarm.MediaService");
    String CPTURE_ENCODER = "com.tme.alarm.capture_encoder";
    String CPTURE_ENCODER_TAG = "CPTURE_ENCODER_TAG";

    String ALARM_TYPE = "ALARM_TYPE";

    /**
     * 报警标识 苏标中使用
     */
    String ALARM_FLAG = "ALARM_FLAG";

    String ALARM_TEST = "com.tme.alarm.TEST";

    /**
     * 系统参数变化
     */
    String SYSTEM_CONFIG = "com.tme.system.CONFIG";
    /**
     * 灯光控制广播
     */
    String SYSTEM_LIGHT = "com.tme.system.LIGHT";
    String LIGTH_INDEX = "LIGTH_INDEX";
    // 灯光控制 -1关闭 0打开 >0 闪烁
    String LIGTH_STATE = "LIGTH_STATE";
    int LIGTH_ON = 0, LIGHT_OFF = -1;

    String SYSTEM_TIME = "com.tme.system.time";
    String TIME_VALUE = "TIME_VALUE";

    /**
     * DSM证据广播
     */
    String DSM_ALARM = "com.tme.dsm.Alarm";
    /*
     ALGO证据广播
     */
    String ADAS_ALARM = "com.tme.adas.Alarm";

    String BSD_ALARM = "com.tme.bsd.Alarm";

    String MONITOR_ALARM = "com.tme.monitor.Alarm";

    String RECORD_TYPE = "RECORD_TYPE";

    String RECORD_SERVICE = "RECORD_SERVICE";
    String RECORD_STATE = "RECORD_STATE";
    int RECORD_START = 0, RECORD_STOP = 1, RECORD_RESTART = 2;

    String ALARM_INFO = "ALARM_INFO";

    /**
     * 处理过后的报警广播
     */
    String Alarm_Broadcast = "com.tme.alarm.broadcast";

    String APC_Broadcast = "com.tme.apc.broadcast";

    /**
     * 人脸部分
     */
    String DSM_FACE_DOWNLOAD = "com.tme.dsm.face.download";

    String DSM_FACE_BACK = "com.tme.dsm.face.back";

    /**
     * Stream类型 0 ali视频推送 1本地RTSP
     */
    String STREAM_TYPE = "STREAM_TYPE";
    int STREAM_PUSH = 0, STREAM_UDP = 1;
    /**
     * 视频推流的url
     */
    String PUSH_URL = "PUSH_URL";
    String PUSH_STATE = "PUSH_STATE";

    String CAMERA_ID = "CAMERA_ID";
    int PUSH_START = 0, PUSH_STOP = 1, PUSH_STATE_INFO = 2;

    /**
     * 应用安装/卸载广播  以下PACKAGE相关命名，必须与Brain Install中保持一致
     */
    String PACKAGE_INSTALL = "com.tme.install.INSTALL";
    String PACKAGE_UNINSTALL = "com.tme.install.UNINSTALL";
    String PACKAGE_PATH = "PACKAGE_PATH";
    /**
     * 安装失败的文件路径
     */
    String PACKAGE_PATH_FAILED = "PACKAGE_PATH_FAILED";
    String PACKAGE_PATH_ERR = "PACKAGE_PATH_ERR";
    String PACKAGE_NAME = "PACKAGE_NAME";
    /**
     * 压缩包文件路径
     */
    String PACKAGE_FILE = "PACKAGE_FILE";
    String PACKAGE_FILES = "PACKAGE_FILES";

    String PACKAGE_INSTALLING = "com.tme.install.INSTALLING";
    String PACKAGE_INSTALLED = "com.tme.install.PACKAGE_INSTALLED";
    String PACKAGE_UNINSTALLING = "com.tme.install.PACKAGE_UNINSTALLING";
    String PACKAGE_UNINSTALLED = "com.tme.install.PACKAGE_UNINSTALLED";

    /**
     * 工厂测试广播
     */
    String FACTORY_TEST = "com.tme.factory.TEST";
    String TEST_STATE = "TEST_STATE";
    int TEST_START = 1, TEST_STOP = 0;

    String TOOL_CONFIG = "com.tme.tool.CONFIG";

    String WIRELESS_USB_ACTION = "com.tme.brain.WIRELESS_USB";
    String USB_ACTION = "com.tme.brain.USB";
    String WIFIAP_ACTION = "com.tme.brain.WIFIAP";
    String WIFIAP_ENABLE = "WIFIAP_ENABLE";
    String USB_ENABLE = "USB_ENABLE";
    String RECOVERY = "com.tme.install.RECOVERY";

    String SHUTDOWN = "com.tme.brain.SHUTDOWN";

    String GYROSCOPE_VALUE = "GYROSCOPE_VALUE";
    String ACCELEROMETER_VALUE = "ACCELEROMETER_VALUE";

    String ETHERNET_SETTING = "com.opt.net.ethernet.EthernetSetting";

    String ETHERNET_DHCP_ENABLE = "ETHERNET_DHCP_ENABLE";
    String ETHERNET_IP = "ETHERNET_IP";
    String ETHERNET_GATEWAY = "ETHERNET_GATEWAY";
    String ETHERNET_MASK = "ETHERNET_MASK";
    String ETHERNET_DNS1 = "ETHERNET_DNS1";
    String ETHERNET_DNS2 = "ETHERNET_DNS2";

    String DSM_STATE = "com.tme.dsm.State";
    String FACE_FLAG = "FACE_FLAG";

    String WIFI_ENABLE = "WIFI_ENABLE";
    String WIFI_SSID = "WIFI_SSID";
    String WIFI_KEY = "WIFI_KEY";
    String WIFI_TYPE = "WIFI_TYPE";

    /**
     * apc开关门消息
     */
    String APC_OPEN_DOOR = "APC_OPEN_DOOR";
    String APC_CLOSE_DOOR = "APC_CLOSE_DOOR";

    String ALARM_START_CONFIG = "ALARM_START_CONFIG";
    //ADAS/DSM/MONITOR/BSD算法服务开启标记位
    int FLAG_ADAS_STATUS    = 0x01;
    int FLAG_DSM_STATUS   = 0x02;
    int FLAG_MONITOR_STATUS    = 0x04;
    int FLAG_BSD_STATUS    = 0x08;
}
