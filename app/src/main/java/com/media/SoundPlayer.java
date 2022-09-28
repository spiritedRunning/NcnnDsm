package com.media;

import android.content.Context;
import android.media.AudioManager;
import android.media.SoundPool;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import com.dsm.bean.AlarmState;

import java.io.File;
import java.util.HashMap;
import java.util.Map;

public class SoundPlayer {
    private static final String TAG = "SoundPlayer";

    private static final String SOUND_ROOT_PATH = "TME/voice/";

    private static SoundPlayer player;

    private int currentIdx;
    private SoundPool soundPool;
    private Map<Integer, Integer> soundMap;

    private AudioManager audioManager;
    private static final int AUDIO_FOCUS_TYPE = AudioManager.AUDIOFOCUS_GAIN_TRANSIENT;
    private static final int STREAM_TYPE = AudioManager.STREAM_MUSIC;

    private Handler handler;
    private float volume;

    public synchronized static SoundPlayer getPlayer(Context context) {
        if (player == null) {
            player = new SoundPlayer(context);
        }
        return player;
    }

    private SoundPlayer(Context context) {
        audioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);

        handler = new Handler(Looper.getMainLooper());

        soundPool = new SoundPool(16, STREAM_TYPE, 0);
        soundMap = new HashMap<Integer, Integer>();

        loadSound(soundPool, soundMap, AlarmState.DSM_RECOGNISE_TIPS);

        new Thread() {
            @Override
            public void run() {
                super.run();
                //加载资源
                loadSound(soundPool, soundMap, AlarmState.DSM_EYE_CLOSE);
                loadSound(soundPool, soundMap, AlarmState.DSM_YWAN);
                loadSound(soundPool, soundMap, AlarmState.DSM_PHONE_CALL);
                loadSound(soundPool, soundMap, AlarmState.DSM_SMOKING);
                loadSound(soundPool, soundMap, AlarmState.DSM_ATTEN);
                loadSound(soundPool, soundMap, AlarmState.DSM_DRIVER_ABNORMAL);
                loadSound(soundPool, soundMap, AlarmState.DSM_INFRARED_ALARM);
                loadSound(soundPool, soundMap, AlarmState.DSM_COVER_ALARM);
                loadSound(soundPool, soundMap, AlarmState.DSM_DRIVER_RECOGNISE);

                loadSound(soundPool, soundMap, AlarmState.DSM_RECOGNISE_ERROR);
                loadSound(soundPool, soundMap, AlarmState.DSM_RECOGNISE_PASS);
                loadSound(soundPool, soundMap,  AlarmState.DSM_DRIVER_CHANGED);
            }
        }.start();

    }

    private void loadSound(SoundPool pool, Map<Integer, Integer> map, int soundId) {
        String soundName = SoundTable.getSoundName(soundId);
        if (soundName != null) {
            File soundFile = new File(Environment.getExternalStorageDirectory(),
                    SOUND_ROOT_PATH + soundName);
            if (soundFile.exists()) {
                map.put(soundId, pool.load(soundFile.getAbsolutePath(), 1));
            }
        }
    }

    public synchronized void playVoice(int state) {
        playVoice(state, 1.0f);
    }

    /**
     * 播放音频  是否播放根据audiofocus来判断
     */
    public synchronized void playVoice(final int state, float volume) {
        try {
            //暂停当前播放的音频
            //soundPool.autoPause();
            soundPool.setVolume(currentIdx, this.volume * 0.25f, this.volume * 0.25f);
        } catch (Exception e) {
        }
        try {
            if (volume < 0) {
                volume = 0;
            }
            if (volume > 1) {
                volume = 1;
            }
            this.volume = volume;
            Integer idx = soundMap.get(state);
            if (idx != null) {
                //音频处理 焦点
                audioManager.requestAudioFocus(audioFocusChangeListener, STREAM_TYPE, AUDIO_FOCUS_TYPE);

                currentIdx = soundPool.play(idx, volume, volume, 1, 0, 1);
            }
        } catch (Exception e) {
            audioManager.abandonAudioFocus(audioFocusChangeListener);
        } finally {
            handler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    if (audioManager != null)
                        audioManager.abandonAudioFocus(audioFocusChangeListener);
                }
            }, 2000);
        }
    }


    public synchronized void pause() {
        try {
            //暂停当前播放的音频
            soundPool.autoPause();
        } catch (Exception e) {
        }
    }

    private AudioManager.OnAudioFocusChangeListener audioFocusChangeListener = new AudioManager.OnAudioFocusChangeListener() {
        @Override
        public void onAudioFocusChange(int focusChange) {
            if (focusChange > 0) {
                return;
            }
            int lossFocus = -1 * AUDIO_FOCUS_TYPE;

            if (focusChange > lossFocus) {
                //丢失掉了焦点，停止播放
                Log.e(TAG, "Audio被抢占，放弃播放  : " + focusChange);
                try {
                    //soundPool.autoPause();
                    soundPool.setVolume(currentIdx, volume * 0.25f, volume * 0.25f);
                } catch (Exception e) {

                }
            } else if (focusChange == lossFocus) {
                Log.e(TAG, "Audio被抢占，降低音量  : " + focusChange);
                soundPool.setVolume(currentIdx, 0.3f, 0.3f);
            } else if (focusChange < lossFocus) {
                Log.e(TAG, "Audio被抢占，重新抢回  : " + focusChange);
                audioManager.requestAudioFocus(audioFocusChangeListener, STREAM_TYPE, AUDIO_FOCUS_TYPE);
            }
        }
    };
}
