//
// Created by F8LEFT on 2016/11/6.
//
#include "Check.h"
#include "string"
#include<stdio.h>
#include<pthread.h>
#include<sys/prctl.h>
#include <sys/types.h>
#include<sys/wait.h>
#include <unistd.h>
#include "errno.h"
#include <sys/ptrace.h>

//#define DEBUG
#ifdef DEBUG
#include <android/log.h>
#define FLOG_TAG "F8LEFT"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, FLOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, FLOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, FLOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, FLOG_TAG, __VA_ARGS__)
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, FLOG_TAG, __VA_ARGS__)
#else
#include <android/log.h>
#define FLOG_TAG "F8LEFT"
#define LOGE(...)
#define LOGD(...)
#define LOGW(...)
#define LOGI(...)
#define LOGV(...)
#endif


//char sflag[0x10+1] = {
//        0x4A, 0x75, 0x73, 0x74, 0x48, 0x61, 0x76, 0x65,
//        0x41, 0x54, 0x72, 0x79, 0x21, 0x21, 0x21, 0x21,
//        0x00
//};

char sflag[0x10+1] = {
        0x46, 0x38, 0x4C, 0x45, 0x46, 0x54, 0x00, 0x4A,
        0x4D, 0x50, 0x4F, 0x45, 0x50, 0x34, 0x53, 0x54,
        0x00
};

int gpipe[2];

void* parent_read_thread(void *param) {
    LOGD("wait for child process to write decode data");
    auto readPipe = gpipe[0];
    read(readPipe, sflag, 0x10);
    close(readPipe);
    return 0;
}

void* child_attach_thread(void *param) {
    int pid = *(int*)param;
    LOGD("check child status %d", pid);
    safe_attach(pid);
    handle_events();
    LOGE("watch thread exit");

    kill(getpid(), 9);
    return nullptr;
}


int checkDebugger(JNIEnv *env, jobject obj) {
// use Multi process to protect itself
    int forktime = 0;

    FORKLABEL:
    forktime++;
    if (forktime > 5) {
        return 0;
    }

    if (pipe(gpipe)) {
        return 0;
    }
    auto pid = fork();
    prctl(PR_SET_DUMPABLE, 1);

    if (pid != 0) {
        // parent
        close(gpipe[1]);
        LOGD("start new thread to read decode data");
        pthread_t ntid;
        pthread_create(&ntid, nullptr, parent_read_thread, &pid);

        do {
            int childstatus;
            auto childpid = waitpid(pid, &childstatus, WNOHANG);
            bool succ = childpid == 0;
            if(childpid > 0) {
                succ = childstatus == 1;
                LOGD("Child process end!");
            }
            if(!succ) {
                kill(pid, 9);
                goto FORKLABEL;
            }
        } while (!sflag[6]);
        LOGD("read decoded data success!!");
        pthread_create(&ntid, nullptr, child_attach_thread, &pid);
    } else {
        // child
        // Write key to pipe
        auto cpid = getppid();
        safe_attach(cpid);
        LOGD("child process Attach success, try to write data");

        close(gpipe[0]);
        auto writepipe = gpipe[1];

        char tflag[0x10+1] = {
                0x4A, 0x75, 0x73, 0x74, 0x48, 0x61, 0x76, 0x65,
                0x41, 0x54, 0x72, 0x79, 0x21, 0x21, 0x21, 0x21
        };

        write(writepipe, tflag, 0x10);
        close(writepipe);
        handle_events();
        exit(EXIT_FAILURE);
//        return execl("f8left.cm2", "f8left.cm2");
    }
    return 0;
}

bool may_cause_group_stop(int signo)
{
    switch(signo)
    {
        case SIGSTOP:
        case SIGTSTP:
        case SIGTTIN:
        case SIGTTOU:
            return true;
            break;
        default:
            break;
    }

    return false;
}

void handle_events()
{
    int status = 0;
    pid_t pid = 0;

    do
    {
        pid = TEMP_FAILURE_RETRY(waitpid(-1, &status, __WALL));
        if (pid < 0)
        {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }

        if (WIFEXITED(status))
        {
            LOGE("%d exited, status=%d\n", pid,  WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status))
        {
            LOGE("%d killed by signal %d\n", pid, WTERMSIG(status));
        }
        else if (WIFSTOPPED(status))
        {
            int signo = WSTOPSIG(status);
            LOGE("%d stopped by signal %d\n", pid, signo);

            if (may_cause_group_stop(signo))
            {
                signo = 0;
            }

            long err = ptrace(PTRACE_CONT, pid, NULL, signo);
            if (err < 0)
            {
                perror("PTRACE_CONT");
                exit(EXIT_FAILURE);
            }
        }

    } while (!WIFEXITED(status) && !WIFSIGNALED(status));

}

void safe_attach(pid_t pid)
{
    long err = ptrace(PTRACE_ATTACH, pid, NULL, NULL);
    if (err < 0)
    {
        LOGE("PTRACE_ATTACH");
        exit(EXIT_FAILURE);
    }

    int status = 0;
    err = TEMP_FAILURE_RETRY(waitpid(pid, &status, __WALL));
    if (err < 0)
    {
        LOGE("waitpid");
        exit(EXIT_FAILURE);
    }

    if (WIFEXITED(status))
    {
        LOGE("%d exited, status=%d\n", pid,  WEXITSTATUS(status));
        exit(EXIT_SUCCESS);
    }
    else if (WIFSIGNALED(status))
    {
        LOGE("%d killed by signal %d\n", pid, WTERMSIG(status));
        exit(EXIT_SUCCESS);
    }
    else if (WIFSTOPPED(status))
    {
        int signo = WSTOPSIG(status);
        LOGE("%d stopped by signal %d\n", pid, signo);

        if (may_cause_group_stop(signo))
        {
            signo = 0;
        }

        err = ptrace(PTRACE_CONT, pid, NULL, signo);
        if (err < 0)
        {
            LOGE("PTRACE_CONT");
            exit(EXIT_FAILURE);
        }
    }

    LOGD("Debugger: attached to process %d\n", pid);
}

// The flag is : "ThatIsEnd,Thanks"
bool check(JNIEnv *env, jobject obj, jstring flag) {
    LOGD("check flag: current encoded flag is %s", sflag);

    auto pflaglen = env->GetStringLength(flag);
    if (pflaglen != 0x10) {
        return false;
    }

    bool ok = false;
    auto pflag = env->GetStringUTFChars(flag, nullptr);

    char tflag[0x10+1] = {
            0x1e, 0x1d, 0x12, 0x00, 0x01, 0x12, 0x33, 0x0b,
            0x25, 0x78, 0x26, 0x11, 0x40, 0x4f, 0x4a, 0x52
    };


    for(auto i = 0; i < 0x10; i++) {
        tflag[i] ^= sflag[i];
    }

    LOGD("check flag: decoded flag is %s", tflag);
    if (memcmp(pflag, tflag, 0x10) == 0) {
        ok = true;
    }

    env->ReleaseStringUTFChars(flag, pflag);
    return ok;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = nullptr;
    jint result = -1;

    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }

    {
        auto clazz = env->FindClass("f8left/cm2/App");
        if (clazz == nullptr) {
            return -1;
        }
        JNINativeMethod method[] = {
                {"checkDebugger", "()I", (void *) checkDebugger}
        };
        if (env->RegisterNatives(clazz, method, 1) < 0) {
            return -1;
        }
        env->DeleteLocalRef(clazz);
    }

    {
        auto clazz = env->FindClass("f8left/cm2/MainActivity");
        if (clazz == nullptr) {
            return -1;
        }
        JNINativeMethod method[] = {
                {"check", "(Ljava/lang/String;)Z", (void *) check}
        };
        if (env->RegisterNatives(clazz, method, 1) < 0) {
            return -1;
        }
        env->DeleteLocalRef(clazz);
    }

    return JNI_VERSION_1_6;
}
