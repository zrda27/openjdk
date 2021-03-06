/*
 * Copyright (c) 2003, 2018, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

#include <stdio.h>
#include <string.h>
#include "jvmti.h"
#include "agent_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef JNI_ENV_ARG

#ifdef __cplusplus
#define JNI_ENV_ARG(x, y) y
#define JNI_ENV_ARG1(x)
#define JNI_ENV_PTR(x) x
#else
#define JNI_ENV_ARG(x,y) x, y
#define JNI_ENV_ARG1(x) x
#define JNI_ENV_PTR(x) (*x)
#endif

#endif

#define JVMTI_ENV_ARG JNI_ENV_ARG
#define JVMTI_ENV_ARG1 JNI_ENV_ARG1
#define JVMTI_ENV_PTR JNI_ENV_PTR

#define JVMTI_ERROR_CHECK(str,res) if ( res != JVMTI_ERROR_NONE) { printf(str); printf("%d\n",res); return res;}
#define JVMTI_ERROR_CHECK_EXPECTED_ERROR(str,res,err) if ( res != err) { printf(str); printf("unexpected error %d\n",res); return res;}

#define JVMTI_ERROR_CHECK_VOID(str,res) if ( res != JVMTI_ERROR_NONE) { printf(str); printf("%d\n",res); iGlobalStatus = 2; }

#define JVMTI_ERROR_CHECK_EXPECTED_ERROR_VOID(str,res,err) if ( res != err) { printf(str); printf("unexpected error %d\n",res); iGlobalStatus = 2; }

#define THREADS_LIMIT 2000


jvmtiEnv *jvmti;
jint iGlobalStatus = 0;
jthread susp_thrd[THREADS_LIMIT];
static jvmtiEventCallbacks callbacks;
static jvmtiCapabilities jvmti_caps;
jrawMonitorID jraw_monitor[20];

int process_once = 0;



int printdump = 0;


void debug_printf(const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    if (printdump) {
        vprintf(fmt, args);
    }
    va_end(args);
}


void JNICALL vmInit(jvmtiEnv *jvmti_env, JNIEnv *env, jthread thread) {

    debug_printf("VMInit event  done\n");

}

void JNICALL vmExit(jvmtiEnv *jvmti_env, JNIEnv *env) {
    debug_printf("------------ JVMTI_EVENT_VM_DEATH ------------\n");
}

void JNICALL classFileLoadEvent(jvmtiEnv *jvmti_env, JNIEnv *env,
                        jclass class_being_redifined,
                        jobject loader, const char* name,
                        jobject protection_domain,
                        jint class_data_len,
                        const unsigned char* class_data,
                        jint* new_class_data_len,
                        unsigned char** new_class_data) {

}



void init_callbacks() {
    memset((void *)&callbacks, 0, sizeof(jvmtiEventCallbacks));
    callbacks.VMInit = vmInit;
    callbacks.VMDeath = vmExit;
    callbacks.ClassFileLoadHook = classFileLoadEvent;
}


#ifdef STATIC_BUILD
JNIEXPORT jint JNICALL Agent_OnLoad_JvmtiTest(JavaVM *jvm, char *options, void *reserved) {
    return Agent_Initialize(jvm, options, reserved);
}
JNIEXPORT jint JNICALL Agent_OnAttach_JvmtiTest(JavaVM *jvm, char *options, void *reserved) {
    return Agent_Initialize(jvm, options, reserved);
}
JNIEXPORT jint JNI_OnLoad_JvmtiTest(JavaVM *jvm, char *options, void *reserved) {
    return JNI_VERSION_1_8;
}
#endif
jint Agent_Initialize(JavaVM * jvm, char *options, void *reserved) {
    jint res;

    if (options && strlen(options) > 0) {
        if (strstr(options, "printdump")) {
            printdump = 1;
        }
    }

    res = JNI_ENV_PTR(jvm)->
        GetEnv(JNI_ENV_ARG(jvm, (void **) &jvmti), JVMTI_VERSION_1_1);
    if (res < 0) {
        debug_printf("Wrong result of a valid call to GetEnv!\n");
        return JNI_ERR;
    }


    /* Add capabilities */
    res = JVMTI_ENV_PTR(jvmti)->GetPotentialCapabilities(JVMTI_ENV_ARG(jvmti, &jvmti_caps));
    JVMTI_ERROR_CHECK("GetPotentialCapabilities returned error", res);

    res = JVMTI_ENV_PTR(jvmti)->AddCapabilities(JVMTI_ENV_ARG(jvmti, &jvmti_caps));
    JVMTI_ERROR_CHECK("GetPotentialCapabilities returned error", res);

    /* Enable events */
    init_callbacks();
    res = JVMTI_ENV_PTR(jvmti)->SetEventCallbacks(JVMTI_ENV_ARG(jvmti, &callbacks), sizeof(callbacks));
    JVMTI_ERROR_CHECK("SetEventCallbacks returned error", res);

    res = JVMTI_ENV_PTR(jvmti)->SetEventNotificationMode(JVMTI_ENV_ARG(jvmti,JVMTI_ENABLE),JVMTI_EVENT_VM_INIT,NULL);
    JVMTI_ERROR_CHECK("SetEventNotificationMode for VM_INIT returned error", res);

    res = JVMTI_ENV_PTR(jvmti)->SetEventNotificationMode(JVMTI_ENV_ARG(jvmti,JVMTI_ENABLE),JVMTI_EVENT_VM_DEATH,NULL);
    JVMTI_ERROR_CHECK("SetEventNotificationMode for vm death event returned error", res);

    return JNI_OK;
}


JNIEXPORT jint JNICALL
Java_nsk_jvmti_unit_functions_nosuspendMonitorInfo_JvmtiTest_GetResult(JNIEnv * env, jclass cls) {
    return iGlobalStatus;
}

JNIEXPORT void JNICALL
Java_nsk_jvmti_unit_functions_nosuspendMonitorInfo_JvmtiTest_CheckMonitorInfo(JNIEnv * env, jclass cls, jthread thr, jobject oobj, jint expected_count) {

  jvmtiError ret;
  jint count;
  jobject *owned_monitor;

  debug_printf(" jvmti GetMonitorInfo \n");


    ret = JVMTI_ENV_PTR(jvmti)->GetOwnedMonitorInfo(JVMTI_ENV_ARG(jvmti, thr), &count , &owned_monitor);
    if (ret != JVMTI_ERROR_NONE) {
        printf("Error: GetMonitorInfo %d \n", ret);
        iGlobalStatus = 2;
    }

    if (expected_count != 0 && expected_count != count) {
        printf("Error: GetMonitorInfo expected count %d but got %d \n", expected_count, count);
        iGlobalStatus = 2;
    }

    if (expected_count !=0 ) {

        ret = JVMTI_ENV_PTR(jvmti)->GetCurrentContendedMonitor(JVMTI_ENV_ARG(jvmti, thr), owned_monitor);
        if (ret != JVMTI_ERROR_NONE) {
            printf("Error: GetContendedMonitorInfo %d \n", ret);
            iGlobalStatus = 2;
        }
    }

}

#ifdef __cplusplus
}
#endif
