#include <stdio.h>
#include <jni.h>

extern "C" {

extern bool MRT_CheckHeapObj(void* ptr);
extern uint32_t MRT_RefCount(void* ptr);

JNIEXPORT jboolean JNICALL Java_RCPermanentTest3_isHeapObject__Ljava_lang_Object_2
(JNIEnv *env, jclass cls, jobject obj) {
  return MRT_CheckHeapObj((void*)obj);
}

JNIEXPORT jint JNICALL Java_RCPermanentTest3_refCount__Ljava_lang_Object_2
(JNIEnv *env, jclass cls, jobject obj) {
  return MRT_RefCount((void*)obj);
}

} /* extern "C" */
