package com.dsm;

import android.Manifest;
import android.app.Activity;
import android.content.pm.PackageManager;
import android.hardware.Camera;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.widget.FrameLayout;

import com.dsm.bean.AlarmState;
import com.media.SoundPlayer;
import com.sample.ncnndsm.R;

public class MainActivity extends Activity {
    private static final String TAG = "DSM_LOG";

    private final static int FRAME_WIDTH = 720, FRAME_HEIGHT = 1280;
    private Camera mCamera;
    private CameraPreview mPreview;
    private AlgoInterface algoInstance;

    private SoundPlayer soundPlayer;
    private boolean nrlEnable = true;
    private int faceFlag = 1;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        soundPlayer = SoundPlayer.getPlayer(this);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && checkSelfPermission(
                Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[]{
                    Manifest.permission.READ_EXTERNAL_STORAGE,
                    Manifest.permission.WRITE_EXTERNAL_STORAGE,
                    Manifest.permission.CAMERA
            }, 1);
        }

        algoInstance = AlgoInterface.getInstance();
        algoInstance.init();
        algoInstance.setAlarmListener(alarmListener);

        new InitCameraThread().start();
        getCameraInfo();

        //startService(new Intent(this, AlgoService.class));
    }


    private AlgoInterface.OnAlarmListener alarmListener = new AlgoInterface.OnAlarmListener() {

        @Override
        public void onAlarm(int alarm, String msg) {
            Log.i(TAG, "alarm: " + alarm + " msg: " + msg);

            if (alarm < 0) {
                return;
            } else if (alarm == 0) {
                Log.e(TAG, "NRL_FACE");

            } else if (alarm == 1) {
                Log.e(TAG, "NO_FACE");
            } else {
                Log.e(TAG, "发生报警 : 0x" + Integer.toHexString(alarm));
                handleAlarmInfo(alarm);
            }

        }
    };

    private synchronized void handleAlarmInfo(int alarmId) {
        if (alarmId == AlarmState.DSM_DRIVER_ABNORMAL) {
            if (nrlEnable) {
                nrlEnable = false;
            } else {
                return;
            }
        }

        Log.e(TAG, "Dsm alarm 0x" + Integer.toHexString(alarmId));
        if (alarmId == AlarmState.DSM_DRIVER_ABNORMAL || alarmId == AlarmState.DSM_COVER_ALARM) {
            if (faceFlag == 0) {
                return;
            }
            faceFlag = 0;
        }

        soundPlayer.playVoice(alarmId, 1);
    }



    private int mBackCameraId, mFrontCameraId;
    private Camera.CameraInfo mBackCameraInfo, mFrontCameraInfo;

    private void getCameraInfo() {
        int numberOfCameras = Camera.getNumberOfCameras();
        for (int cameraId = 0; cameraId < numberOfCameras; cameraId++) {
            Camera.CameraInfo cameraInfo = new Camera.CameraInfo();
            Camera.getCameraInfo(cameraId, cameraInfo);
            if (cameraInfo.facing == Camera.CameraInfo.CAMERA_FACING_BACK) {
                mBackCameraId = cameraId;
                mBackCameraInfo = cameraInfo;
            } else if (cameraInfo.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
                mFrontCameraId = cameraId;
                mFrontCameraInfo = cameraInfo;
            }
        }

        Log.e(TAG, "mBackCameraId: " + mBackCameraId  + ", mFrontCameraId: " + mFrontCameraId);
    }

    private boolean safeCameraOpen() {
        boolean qOpened = false;
        try {
            releaseCamera();
            mCamera = Camera.open(1);
            qOpened = (mCamera != null);
        } catch (Exception e) {
            Log.e(TAG, "failed to open Camera");
            e.printStackTrace();
        }
        return qOpened;
    }

    private void releaseCamera() {
        if (mCamera != null) {
            mCamera.setPreviewCallback(null);
            mCamera.release();        // release the camera for other applications
            mCamera = null;
        }
    }

    private class InitCameraThread extends Thread {
        @Override
        public void run() {
            super.run();
            if (safeCameraOpen()) {
                Log.i(TAG, "Open camera");
                Log.i(TAG, "InitCameraThread current thread: " + Thread.currentThread().getName());
                mPreview = new CameraPreview(MainActivity.this, mCamera);

                runOnUiThread(() -> {
                    mPreview.setSurfaceTextureListener(mPreview);
                    FrameLayout preview = findViewById(R.id.camera_preview);
                    preview.addView(mPreview);
                });

            }
        }
    }



    @Override
    protected void onPause() {
        super.onPause();
        releaseCamera();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        releaseCamera();
        algoInstance.releaseInstance();
    }

}
