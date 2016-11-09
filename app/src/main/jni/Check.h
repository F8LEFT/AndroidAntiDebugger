//
// Created by F8LEFT on 2016/11/6.
//

#ifndef CM2_CHECK_H
#define CM2_CHECK_H

#include <jni.h>
#include <sys/types.h>

int checkDebugger(JNIEnv* env, jobject obj);
bool check(JNIEnv* env, jobject obj, jstring flag);

void safe_attach(pid_t pid);
void handle_events();


#endif //CM2_CHECK_H
