package com.dsm;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;

import java.io.IOException;
import java.nio.ByteBuffer;

public class AvcEncoder {

    //private static final String MIME_TYPE = MediaFormat.MIMETYPE_VIDEO_HEVC;
    private static final String MIME_TYPE = MediaFormat.MIMETYPE_VIDEO_AVC;
    // parameters for recording
    private static final int FRAME_RATE = 20; //25
    private static final float BPP = 0.15f; //0.25

    private MediaCodec mediaCodec;
    byte[] mInfo = null;

    private int mWidth, mHeight;

    public AvcEncoder(int width, int height) throws IOException {
        mWidth = width;
        mHeight = height;

        mediaCodec = MediaCodec.createEncoderByType(MIME_TYPE);
        MediaFormat mediaFormat = MediaFormat.createVideoFormat(MIME_TYPE, width, height);
        mediaFormat.setInteger(MediaFormat.KEY_BIT_RATE, calcBitRate());
        mediaFormat.setInteger(MediaFormat.KEY_FRAME_RATE, FRAME_RATE);
        mediaFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar);
        mediaFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 1); // 关键帧间隔时间

        mediaCodec.configure(mediaFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
        mediaCodec.start();
    }

    public void close() {
        try {
            mediaCodec.stop();
            mediaCodec.release();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public int offerEncoder(byte[] yuv420, byte[] output) {
        int pos = 0;
        try {
            ByteBuffer[] inputBuffers = mediaCodec.getInputBuffers();
            ByteBuffer[] outputBuffers = mediaCodec.getOutputBuffers();
            int inputBufferIndex = mediaCodec.dequeueInputBuffer(-1);
            if (inputBufferIndex >= 0) {
                ByteBuffer inputBuffer = inputBuffers[inputBufferIndex];
                inputBuffer.clear();
                inputBuffer.put(yuv420);
                mediaCodec.queueInputBuffer(inputBufferIndex, 0, yuv420.length, getPTSUs(), 0);
            }

            MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
            int outputBufferIndex = mediaCodec.dequeueOutputBuffer(bufferInfo, 0);

            while (outputBufferIndex >= 0) {
                ByteBuffer outputBuffer = outputBuffers[outputBufferIndex];
                byte[] outData = new byte[bufferInfo.size];
                outputBuffer.get(outData);

                if (mInfo != null) {
                    System.arraycopy(outData, 0, output, pos, outData.length);
                    pos += outData.length;

                } else // 保存pps sps 只有开始时 第一个帧里有， 保存起来后面用
                {
                    ByteBuffer spsPpsBuffer = ByteBuffer.wrap(outData);
                    if (spsPpsBuffer.getInt() == 0x00000001) {
                        mInfo = new byte[outData.length];
                        System.arraycopy(outData, 0, mInfo, 0, outData.length);
                    } else {
                        return -1;
                    }
                }

                if (output[4] == 0x65) // key frame 编码器生成关键帧时只有 00 00 00 01 65 没有pps
                // sps， 要加上
                {
                    System.arraycopy(output, 0, yuv420, 0, pos);
                    System.arraycopy(mInfo, 0, output, 0, mInfo.length);
                    System.arraycopy(yuv420, 0, output, mInfo.length, pos);
                    pos += mInfo.length;
                }

                mediaCodec.releaseOutputBuffer(outputBufferIndex, false);
                outputBufferIndex = mediaCodec.dequeueOutputBuffer(bufferInfo, 0);
            }

        } catch (Throwable t) {
            t.printStackTrace();
        }

        return pos;
    }


    private int calcBitRate() {
        final int bitrate = (int) (BPP * FRAME_RATE * mWidth * mHeight);
        return bitrate;
    }

    /**
     * previous presentationTimeUs for writing
     */
    private long prevOutputPTSUs = 0;

    /**
     * get next encoding presentationTimeUs
     *
     * @return
     */
    protected long getPTSUs() {
        long result = System.nanoTime() / 1000L;
        // presentationTimeUs should be monotonic
        // otherwise muxer fail to write
        if (result < prevOutputPTSUs)
            result = (prevOutputPTSUs - result) + result;
        return result;
    }
}