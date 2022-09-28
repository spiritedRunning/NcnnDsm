/*
 * Log.h
 *
 *  Created on: 2017年7月25日
 *      Author: Administrator
 */

#ifndef LOG_H_
#define LOG_H_

#include <android/log.h>

#define LOG_TAG "TME/Native"
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))

#endif /* LOG_H_ */
