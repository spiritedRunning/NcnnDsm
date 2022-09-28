package com.dsm;

public class AlgoInterface {
    private static final String TAG = "DSM_LOG";

    static {
        System.loadLibrary("dsm");
        System.loadLibrary("opencv_java3");
        System.loadLibrary("caffe");

    }

    private static class InstanceHolder {
        public static AlgoInterface instance = new AlgoInterface();
    }

    private AlgoInterface() {

    }

    public static AlgoInterface getInstance() {
        return InstanceHolder.instance;
    }

    public void init() {
        initNative();
    }
    public void releaseInstance() {
        release();
    }



    private static native void initNative();
    private static native void release();


    public native void runningDsmAlgo(byte[] data, int width, int height);
    public native void setAlgoEnable(boolean enable);

    public void sendAlarm(int alarm, String msg) {
        //Log.i(TAG, "sendAlarm callback, listener: " + alarmListener);
        if (alarmListener != null) {
            alarmListener.onAlarm(alarm, msg);
        }
    }


    private OnAlarmListener alarmListener;

    public void setAlarmListener(OnAlarmListener alarmListener) {
        this.alarmListener = alarmListener;
    }

    public interface OnAlarmListener {
        void onAlarm(int alarm, String msg);
    }

}
