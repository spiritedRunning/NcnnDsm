package com.media;

import android.content.Context;
import android.content.res.Resources;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnCompletionListener;
import android.media.MediaPlayer.OnPreparedListener;
import android.os.Handler;
import android.util.Log;

public class MediaPlayor {
    private final static String TAG = "MediaPlayor";

    private static MediaPlayor player;

    public synchronized static MediaPlayor getPlayer(Context context) {
        if (player == null) {
            player = new MediaPlayor(context);
        }
        return player;
    }

    private Context context;
    private Resources resources;
    private String packageName;
    private MediaPlayer mediaPlayer;

    private boolean isPlaying;

    private Object object = new Object();

    private Handler handler;

    private MediaPlayor(Context context) {
        this.context = context;
        resources = context.getResources();
        packageName = context.getPackageName();

        handler = new Handler();

        initMediaPlayor();
    }

    public void initMediaPlayor() {
        if (mediaPlayer != null) {
            return;
        }
        mediaPlayer = new MediaPlayer();
        mediaPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
        mediaPlayer.setOnPreparedListener(new OnPreparedListener() {

            @Override
            public void onPrepared(MediaPlayer mp) {
                try {
                    isPlaying = true;
                    mp.start();

                } catch (Exception e) {
                    Log.e(TAG, "onPrepared", e);
                }
            }
        });
        mediaPlayer.setOnErrorListener(new MediaPlayer.OnErrorListener() {

            @Override
            public boolean onError(MediaPlayer mp, int what, int extra) {
                isPlaying = false;
                return false;
            }
        });
        mediaPlayer.setOnCompletionListener(new OnCompletionListener() {

            @Override
            public void onCompletion(MediaPlayer mp) {
                isPlaying = false;
            }
        });
        mediaPlayer.setLooping(false);
    }

    public synchronized void play(final String url) {
        new Thread() {
            public void run() {
                while (isPlaying) {
                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
                isPlaying = true;
                try {
                    mediaPlayer.reset();
                } catch (Exception e) {
                    Log.e(TAG, "reset", e);
                }

                try {
                    mediaPlayer.setDataSource(url);
                    mediaPlayer.prepareAsync();
                } catch (Exception e) {
                    Log.e(TAG, "setDataSource", e);
                    try {
                        mediaPlayer = null;
                        initMediaPlayor();
                        mediaPlayer.reset();

                        mediaPlayer.setDataSource(url);
                        mediaPlayer.prepareAsync();
                    } catch (Exception exe) {
                    }
                }
            }

            ;
        }.start();
    }

    public void release() {
        if (mediaPlayer != null) {
            mediaPlayer.stop();
            mediaPlayer.release();
            mediaPlayer = null;
        }
    }
}
