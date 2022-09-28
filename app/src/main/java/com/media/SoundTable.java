package com.media;

import com.dsm.bean.AlarmState;


public class SoundTable {
    public static String getSoundName(int state) {
        switch (state) {
            case AlarmState.DSM_EYE_CLOSE:
                return "voi_dsm_eye_close.wav";
            case AlarmState.DSM_YWAN:
                return "voi_dsm_ywan.wav";
            case AlarmState.DSM_PHONE_CALL:
                return "voi_dsm_phone_call.wav";
            case AlarmState.DSM_SMOKING:
                return "voi_dsm_smoking.wav";
            case AlarmState.DSM_ATTEN:
                return "voi_dsm_atten.wav";
            case AlarmState.DSM_DRIVER_ABNORMAL:
                return "voi_dsm_abnormal.wav";
            case AlarmState.DSM_INFRARED_ALARM:
                return "voi_dsm_infrared.wav";
            case AlarmState.DSM_COVER_ALARM:
                return "voi_dsm_cover.wav";
            case AlarmState.DSM_DRIVER_RECOGNISE:
                return "voi_dsm_driver_changed.wav";
            case AlarmState.DSM_RECOGNISE_TIPS:
                return "voi_dsm_recognise_begin.wav";
            case AlarmState.DSM_RECOGNISE_PASS:
                return "voi_dsm_recognise_pass.wav";
            case AlarmState.DSM_RECOGNISE_ERROR:
                return "voi_dsm_recognise_error.wav";
            case AlarmState.DSM_DRIVER_CHANGED:
                return "voi_dsm_driver_changed.wav";
        }
        return null;
    }
}
