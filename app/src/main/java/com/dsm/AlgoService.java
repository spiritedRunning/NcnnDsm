package com.dsm;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Handler;
import android.os.IBinder;
import android.util.Log;

import com.dsm.bean.ActionTable;
import com.dsm.bean.AlarmState;
import com.media.MediaPlayor;
import com.media.SoundPlayer;


public class AlgoService extends Service {
    private final static String TAG = "DSM_LOG";

    private SoundPlayer soundPlayer;
    private Handler handler;

    private MediaPlayor mediaPlayor;
    private boolean nrlEnable;

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onRebind(Intent intent) {
        super.onRebind(intent);
    }

    @Override
    public boolean onUnbind(Intent intent) {
        return true;
    }

    @Override
    public void onCreate() {
        super.onCreate();

        AlgoInterface.getInstance().setAlarmListener(alarmListener);

        soundPlayer = SoundPlayer.getPlayer(this);
        handler = new Handler();


        IntentFilter filter = new IntentFilter(ActionTable.SYSTEM_CONFIG);
        filter.addAction(ActionTable.ALARM_TEST);
        filter.addAction(ActionTable.DSM_FACE_DOWNLOAD);
        registerReceiver(algoReceiver, filter);

        nrlEnable = true;
        mediaPlayor = MediaPlayor.getPlayer(this);

    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        unregisterReceiver(algoReceiver);

        super.onDestroy();
    }


    private int faceFlag = 1;

    private AlgoInterface.OnAlarmListener alarmListener = new AlgoInterface.OnAlarmListener() {
        private long lastStateTimeTick = 0;

        @Override
        public void onAlarm(int alarm, String msg) {
            Log.i(TAG, "alarm: " + alarm + " msg: " + msg);

            if (alarm < 0) {
                return;
            } else if (alarm == 0) {
                Log.e(TAG, "NRL_FACE");
                faceFlag = 1;
            } else if (alarm == 1) {
                Log.e(TAG, "NO_FACE");
            } else {
                Log.e(TAG, "发生报警 : 0x" + Integer.toHexString(alarm));
                handleAlarmInfo(alarm);
            }
            if (System.currentTimeMillis() - lastStateTimeTick > 500) {
                lastStateTimeTick = System.currentTimeMillis();
                Intent intent = new Intent(ActionTable.DSM_STATE);
                intent.putExtra(ActionTable.FACE_FLAG, ((alarm == 1 || faceFlag == 0) ? 0 : 1));
                sendBroadcast(intent);
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



    private BroadcastReceiver algoReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
        }
    };



}
