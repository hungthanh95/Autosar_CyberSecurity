/********************************************************************************************************/
/************************************************INCULDES************************************************/
/********************************************************************************************************/
#include "Scheduler/scheduler.h"
#include "SecOC.h"
#include "SecOC_Debug.h"
#include "Com.h"
#include "CanTP.h"
#include "Ethernet/ethernet.h"
#include "SoAd.h"

#ifdef _WIN32
    #include <windows.h>
#else
    #include <stdio.h>
    #include <stdlib.h>
    #include <time.h>
    #include <unistd.h>
    #include <signal.h>
    #include <string.h>
    #include <sys/time.h>
#endif

/********************************************************************************************************/
/********************************************GlobalVaribles**********************************************/
/********************************************************************************************************/

#ifdef _WIN32
    static HANDLE threads[NUM_FUNCTIONS];
    static HANDLE timerHandle;
    static boolean once = FALSE;
#else
    static char stacks[NUM_FUNCTIONS][STACK_SIZE];
    static struct task_state {
        void (*function)(void);
        char *stack;
        int state;
        ucontext_t context;
    } tasks[NUM_FUNCTIONS];
    static boolean once = FALSE;
    static pthread_t t;
#endif

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

void EthernetRecieveFn(void) {
#ifdef _WIN32
    while (1) {
        if (!once) {
            ethernet_ReceiveMainFunction();
            once = TRUE;
        }
        Sleep(1);
    }
#else
    while (1) {
        tasks[0].state++;
        if (!once) {
            if (pthread_create(&t, NULL, (void*)&ethernet_ReceiveMainFunction, NULL) != 0) {
                #ifdef SCHEDULER_DEBUG
                printf("error create thread");
                #endif
                return;
            }
            once = TRUE;
        }
        swapcontext(&tasks[0].context, &tasks[1].context);
    }
#endif
}

void RecieveMainFunctions(void) {
#ifdef _WIN32
    while (1) {
        SoAd_MainFunctionRx();
        SecOC_MainFunctionRx();
        CanTp_MainFunctionRx();
        Sleep(1);
    }
#else
    while (1) {
        tasks[1].state++;
        SoAd_MainFunctionRx();
        SecOC_MainFunctionRx();
        CanTp_MainFunctionRx();
        swapcontext(&tasks[1].context, &tasks[2].context);
    }
#endif
}

void TxMainFunctions(void) {
#ifdef _WIN32
    while (1) {
        Com_MainTx();
        SecOC_MainFunctionTx();
        SoAd_MainFunctionTx();
        CanTp_MainFunctionTx();
        Sleep(1);
    }
#else
    while (1) {
        tasks[2].state++;
        Com_MainTx();
        SecOC_MainFunctionTx();
        SoAd_MainFunctionTx();
        CanTp_MainFunctionTx();
        swapcontext(&tasks[2].context, &tasks[0].context);
    }
#endif
}

#ifdef _WIN32
VOID CALLBACK TimerCallback(PVOID lpParam, BOOLEAN TimerOrWaitFired) {
    // Timer callback - nothing to do here as threads run continuously
}
#else
void scheduler_handler(int signum) {
    // Not used in Linux implementation
}
#endif

void Scheduler_Start(void) {
#ifdef _WIN32
    // Create Windows threads
    threads[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)EthernetRecieveFn, NULL, 0, NULL);
    threads[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RecieveMainFunctions, NULL, 0, NULL);
    threads[2] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)TxMainFunctions, NULL, 0, NULL);

    // Create a timer
    CreateTimerQueueTimer(&timerHandle, NULL, TimerCallback, 
        NULL, 0, 900, WT_EXECUTEDEFAULT);

    // Wait for all threads
    WaitForMultipleObjects(NUM_FUNCTIONS, threads, TRUE, INFINITE);

    // Cleanup
    for (int i = 0; i < NUM_FUNCTIONS; i++) {
        CloseHandle(threads[i]);
    }
    DeleteTimerQueueTimer(NULL, timerHandle, NULL);
#else
    int i;
    struct sigaction sa;
    struct itimerval timer;
    void (*functions[NUM_FUNCTIONS])(void) = {EthernetRecieveFn, RecieveMainFunctions, TxMainFunctions};

    // Initialize task states
    for (i = 0; i < NUM_FUNCTIONS; i++) {
        tasks[i].function = functions[i];
        tasks[i].stack = stacks[i] + STACK_SIZE;
        tasks[i].state = 0;
        getcontext(&tasks[i].context);
        tasks[i].context.uc_stack.ss_sp = tasks[i].stack;
        tasks[i].context.uc_stack.ss_size = STACK_SIZE;
        tasks[i].context.uc_link = NULL;
        makecontext(&tasks[i].context, tasks[i].function, 0);
    }

    // Set up timer signal
    sa.sa_handler = scheduler_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, NULL);

    // Configure timer
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 900000;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 900000;

    setitimer(ITIMER_REAL, &timer, NULL);
    tasks[0].state = 1;

    swapcontext(&tasks[0].context, &tasks[0].context);

    while (1) {
        pause();
    }
#endif
}