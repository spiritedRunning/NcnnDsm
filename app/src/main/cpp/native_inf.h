

#ifndef TME_PRO_ALGO_NATIVE_INF_H
#define TME_PRO_ALGO_NATIVE_INF_H

//定义包名+类名
#define CLASS com_dsm_AlgoInterface
#define CLASS_METHOD "com/dsm/AlgoInterface"

#define JNI_METHOD_CLZ_(_CLASS, FUNC) Java_##_CLASS##_##FUNC
#define JNI_METHOD_CLZ(_CLASS, FUNC) JNI_METHOD_CLZ_(_CLASS, FUNC)
#define JNI_METHOD(FUNC) JNI_METHOD_CLZ(CLASS, FUNC)

#endif //TME_PRO_ALGO_NATIVE_INF_H
