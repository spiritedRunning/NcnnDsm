package com.dsm;

import android.content.Context;
import android.content.res.Configuration;
import android.graphics.ImageFormat;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.util.Log;

import android.view.TextureView;
import android.widget.Toast;


import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class CameraPreview extends TextureView implements TextureView.SurfaceTextureListener {
    private static final String TAG = "CameraPreview";
    private Camera mCamera;
    private Context mContext;
    private Size cameraSize = new Size(-1, -1);

    private ExecutorService executorService = Executors.newFixedThreadPool(1);

    public CameraPreview(Context context) {
        super(context);
        mContext = context;
    }

    public CameraPreview(Context context, Camera camera) {
        super(context);
        mCamera = camera;
        mContext = context;
    }

    public void setCamera(Camera camera) {
        this.mCamera = camera;
    }

    @Override
    public void onSurfaceTextureAvailable(SurfaceTexture surfaceTexture, int width, int height) {
        Log.e(TAG, "TextureView onSurfaceTextureAvailable, width: " + width + ", height: " + height);

        if (this.getResources().getConfiguration().orientation != Configuration.ORIENTATION_LANDSCAPE) {
            mCamera.setDisplayOrientation(90);
        } else {
            mCamera.setDisplayOrientation(0);
        }
        try {
            mCamera.setPreviewCallback(mCameraPreviewCallback);
            mCamera.setPreviewTexture(surfaceTexture);

            Camera.Parameters parameters = mCamera.getParameters();
            if (cameraSize.getWidth() <= 0 || cameraSize.getHeight() <= 0) {
                Camera.Size size = getOptimalSize(
                        mContext.getResources().getDisplayMetrics().widthPixels,
                        mContext.getResources().getDisplayMetrics().heightPixels);
                cameraSize.setWidth(size.width);
                cameraSize.setHeight(size.height);
            }

            // 设置预览尺寸
            Log.e(TAG, "optimal preview width: " + cameraSize.getWidth() + ", height: " + cameraSize.getHeight());
            parameters.setPreviewSize(cameraSize.getWidth(), cameraSize.getHeight());

            mCamera.setParameters(parameters);
            mCamera.startPreview();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private Camera.Size getOptimalSize(int w, int h) {
        Camera.Parameters cameraParameter = mCamera.getParameters();
        List<Camera.Size> sizes = cameraParameter.getSupportedPreviewSizes();
        final double ASPECT_TOLERANCE = 0.1;
        // 竖屏是 h/w, 横屏是 w/h
        double targetRatio = (double) h / w;
        Camera.Size optimalSize = null;
        double minDiff = Double.MAX_VALUE;

        int targetHeight = h;

        for (Camera.Size size : sizes) {
            double ratio = (double) size.width / size.height;
            if (Math.abs(ratio - targetRatio) > ASPECT_TOLERANCE) continue;
            if (Math.abs(size.height - targetHeight) < minDiff) {
                optimalSize = size;
                minDiff = Math.abs(size.height - targetHeight);
            }
        }

        if (optimalSize == null) {
            minDiff = Double.MAX_VALUE;
            for (Camera.Size size : sizes) {
                if (Math.abs(size.height - targetHeight) < minDiff) {
                    optimalSize = size;
                    minDiff = Math.abs(size.height - targetHeight);
                }
            }
        }

        return optimalSize;
    }


    @Override
    public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {
        Log.i(TAG, "TextureView onSurfaceTextureSizeChanged"); // Ignored, Camera does all the work for us
    }

    @Override
    public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
        Log.i(TAG, "TextureView onSurfaceTextureDestroyed");
        mCamera.stopPreview();
        mCamera.release();
        return true;
    }

    @Override
    public void onSurfaceTextureUpdated(SurfaceTexture surface) {
        // Invoked every time there's a new Camera preview frame
    }


    private Camera.PreviewCallback mCameraPreviewCallback = new Camera.PreviewCallback() {
        @Override
        public void onPreviewFrame(byte[] data, Camera camera) {
//            Log.d(TAG, "onPreviewFrame: data.length=" + data.length);
            Camera.Size previewSize = mCamera.getParameters().getPreviewSize();
            //Log.i(TAG, "previewSize: " + previewSize.width + " " + previewSize.height);

            //int format = mCamera.getParameters().getPreviewFormat();
            //int bitsPerPixel = ImageFormat.getBitsPerPixel(format);
            //Log.i(TAG, "format: " + format + ", bitsPerPixel: " + bitsPerPixel);

            AlgoInterface.getInstance().runningDsmAlgo(data, previewSize.width, previewSize.height);
        }

    };

}
