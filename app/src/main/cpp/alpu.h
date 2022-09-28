/*
 * alpu.h
 *
 *  Created on: 2017年12月9日
 *      Author: Imx
 */
#include <jni.h>
#include <iostream>

#include "Log.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include<string.h>


#ifndef ALPU_H_
#define ALPU_H_

#ifdef __cplusplus
extern "C" {
#endif

class Alpu {
public:
	unsigned char _alpuc_process();
};

#ifdef __cplusplus
}
#endif
#endif /* ALPU_H_ */
