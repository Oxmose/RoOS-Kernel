#include <Syscall.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

int consoleFd;
int nestedTest = 0;

void testSignalSyscall(void* pThread);
void testSignalInterrupt(void);

void* threadRoutine(void* pParam);

void handlerSyscallTest(int signum, void* uContext);
void HandlerInt0(int signum, void* uContext);
void HandlerInt1(int signum, void* uContext);

void testPingPong(void);
void* PingPongRoutine(void* pParam);
void handlerPong(int signum, void* uContext);
void handlerPing(int signum, void* uContext);

void handlerSyscallTest(int signum, void* uContext)
{
  void* pThread;
  Syscall(SYSCALL_ID_THREAD_GET_SELF, &pThread, (void*)0, (void*)0, (void*)0, (void*)0);

  if (signum == 1)
  {
    printf("-> %d (%d) ", signum, nestedTest);
    if (signum == 1 && nestedTest < 3)
    {
      nestedTest++;
      Syscall(SYSCALL_ID_SIGNAL, (void*)1, pThread, (void*)0, (void*)0, (void*)0);
      --nestedTest;
    }
    printf("-> %d (%d) ", signum, nestedTest);

    if (nestedTest == 0)
    {
      printf("\n", signum, nestedTest);
    }
  }
  else
  {
    printf("Signal %d Received\n", signum);
  }


  Syscall(SYSCALL_ID_SIGNAL_RETURN, uContext, (void*)0, (void*)0, (void*)0, (void*)0);

  printf("[ERROR] Signal %d returned\n", signum);
}

volatile int sigRecv = 0;
void* pThreadMain;

void HandlerInt0(int signum, void* uContext)
{
  void* pThread;
  Syscall(SYSCALL_ID_THREAD_GET_SELF, &pThread, (void*)0, (void*)0, (void*)0, (void*)0);

  printf("-> %d (%d) ", signum, nestedTest);

  if (signum == 2 && nestedTest < 3)
  {
    nestedTest++;
    Syscall(SYSCALL_ID_SIGNAL, (void*)2, pThread, (void*)0, (void*)0, (void*)0);
    --nestedTest;
  }

  printf("-> %d (%d) ", signum, nestedTest);

  if (nestedTest == 0)
  {
    printf("\n", signum, nestedTest);
    Syscall(SYSCALL_ID_SIGNAL, (void*)1, pThreadMain, (void*)0, (void*)0, (void*)0);
    sigRecv = 1;
  }

  Syscall(SYSCALL_ID_SIGNAL_RETURN, uContext, (void*)0, (void*)0, (void*)0, (void*)0);

  printf("[ERROR] Signal %d returned\n", signum);
}

void HandlerInt1(int signum, void* uContext)
{
  printf("Signal %dH3\n", signum);
  Syscall(SYSCALL_ID_SIGNAL_RETURN, uContext, (void*)0, (void*)0, (void*)0, (void*)0);
  printf("[ERROR] Signal %d returned\n", signum);
}

void testSignalSyscall(void* pThread)
{
  int    retVal;

  printf("Registering signal handler 1: ");
  retVal = (int)(unsigned long long) Syscall(SYSCALL_ID_SIGNAL_REGISTER, (void*)1, (void*)handlerSyscallTest, (void*)0, (void*)0, (void*)0);
  printf("%d (ERRNO: %d)\n", retVal, errno);
  printf("Registering signal handler 2: ");
  retVal = (int)(unsigned long long) Syscall(SYSCALL_ID_SIGNAL_REGISTER, (void*)2, (void*)handlerSyscallTest, (void*)0, (void*)0, (void*)0);
  printf("%d (ERRNO: %d)\n", retVal, errno);

  printf("Sending signal 1 to thread %p\n", pThread);
  retVal = (int)(unsigned long long) Syscall(SYSCALL_ID_SIGNAL, (void*)1, pThread, (void*)0, (void*)0, (void*)0);
  printf("%d (ERRNO: %d)\n", retVal, errno);
  printf("Sending signal 2 to thread %p\n", pThread);
  retVal = (int)(unsigned long long) Syscall(SYSCALL_ID_SIGNAL, (void*)2, pThread, (void*)0, (void*)0, (void*)0);
  printf("%d (ERRNO: %d)\n", retVal, errno);
  printf("Sending signal 2 to thread %p\n", pThread);
  retVal = (int)(unsigned long long) Syscall(SYSCALL_ID_SIGNAL, (void*)2, pThread, (void*)0, (void*)0, (void*)0);
  printf("%d (ERRNO: %d)\n", retVal, errno);
}

void* threadRoutine(void* pParam)
{
  int   value;

  (void)pParam;
  value = (int)(unsigned long long)pParam;

  printf("Thread started with arg: %d\n", value);

  while (sigRecv != 1) {}

  printf("Thread exiting with arg: %d\n", value);

  Syscall(SYSCALL_ID_THREAD_EXIT, (void*)28, (void*)0, (void*)0, (void*)0, (void*)0);

  printf("Thread returned: %d\n", value);

  return NULL;
}

void testSignalInterrupt(void)
{
  int   retVal;
  void* pThread;
  int threadRetVal;
  S_ThreadAttr threadAttr;

  Syscall(SYSCALL_ID_THREAD_GET_SELF, &pThreadMain, (void*)0, (void*)0, (void*)0, (void*)0);

  threadAttr.mappedCPUs.mask[0] = 0xF;
  threadAttr.stackSize = 0x1000;
  threadAttr.priority = 20;
  memcpy(threadAttr.name, "TestSignal", 11);

  printf("Registering signal handler 1: ");
  retVal = (int)(unsigned long long) Syscall(SYSCALL_ID_SIGNAL_REGISTER, (void*)1, (void*)HandlerInt1, (void*)0, (void*)0, (void*)0);
  printf("%d (ERRNO: %d)\n", retVal, errno);
  printf("Registering signal handler 2: ");
  retVal = (int)(unsigned long long) Syscall(SYSCALL_ID_SIGNAL_REGISTER, (void*)2, (void*)HandlerInt0, (void*)0, (void*)0, (void*)0);
  printf("%d (ERRNO: %d)\n", retVal, errno);

  /* Create the thread */
  printf("Creating thread for signal test: ");
  retVal = (int)(unsigned long long) Syscall(SYSCALL_ID_THREAD_CREATE, (void*)&pThread, (void*)&threadAttr, threadRoutine, (void*)53, (void*)0);
  printf("%d (ERRNO: %d)\n", retVal, errno);


  /* Signal the thread */
  printf("Sending signal 2 to thread %p\n", pThread);
  retVal = (int)(unsigned long long) Syscall(SYSCALL_ID_SIGNAL, (void*)2, pThread, (void*)0, (void*)0, (void*)0);
  printf("%d (ERRNO: %d)\n", retVal, errno);

  /* Join the thread */
  printf("Joining thread for signal test: ");
  retVal = (int)(unsigned long long) Syscall(SYSCALL_ID_THREAD_JOIN, (void*)pThread, (void*)&threadRetVal, (void*)0, (void*)0, (void*)0);
  printf("%d (ERRNO: %d) | Return: %d\n", retVal, errno, threadRetVal);
}

void handlerPing(int signum, void* uContext)
{
  (void)signum;
  printf(" -> Pi");
  Syscall(SYSCALL_ID_SIGNAL_RETURN, uContext, (void*)0, (void*)0, (void*)0, (void*)0);
}

void handlerPong(int signum, void* uContext)
{
  (void)signum;
  printf(" -> Po");
  Syscall(SYSCALL_ID_SIGNAL_RETURN, uContext, (void*)0, (void*)0, (void*)0, (void*)0);
}

void*           pThread1;
void*           pThread2;

void* PingPongRoutine(void* pParam)
{
  int             retVal;
  int             value;
  struct timespec sleepTime;
  struct timespec remainingTime;
  value = (int)(unsigned long long)pParam;

  while(1)
  {
    if (value == 0)
    {
      sleepTime.tv_nsec = 0;
      sleepTime.tv_sec  = 100;
      retVal = nanosleep(&sleepTime, &remainingTime);
      if (retVal >= 0 || errno != EINTR)
      {
        printf("Ping thread intr sleep failed %d %d\n", retVal, errno);
      }

      printf("ng %ul\n", remainingTime.tv_sec * 1000000000 + remainingTime.tv_nsec);
      sleepTime.tv_nsec = 5000000;
      sleepTime.tv_sec  = 0;
      retVal = nanosleep(&sleepTime, &remainingTime);
      if (retVal != 0)
      {
        printf("Ping thread sleep failed %d %d\n", retVal, errno);
      }

      Syscall(SYSCALL_ID_SIGNAL, (void*)2, (void*)pThread2, (void*)0, (void*)0, (void*)0);
    }
    else
    {
      Syscall(SYSCALL_ID_SIGNAL, (void*)1, (void*)pThread1, (void*)0, (void*)0, (void*)0);

      sleepTime.tv_nsec = 0;
      sleepTime.tv_sec  = 100;
      retVal = nanosleep(&sleepTime, &remainingTime);
      if (retVal >= 0 || errno != EINTR)
      {
        printf("Ping thread intr sleep failed %d %d\n", retVal, errno);
      }

      printf("ng %ul\n", remainingTime.tv_sec * 1000000000 + remainingTime.tv_nsec);
      sleepTime.tv_nsec = 5000000;
      sleepTime.tv_sec  = 0;
      retVal = nanosleep(&sleepTime, &remainingTime);
      if (retVal != 0)
      {
        printf("Ping thread sleep failed %d %d\n", retVal, errno);
      }
    }
  }

  return NULL;
}

void testPingPong(void)
{
  S_ThreadAttr    threadAttr;

  Syscall(SYSCALL_ID_SIGNAL_REGISTER, (void*)1, (void*)handlerPing, (void*)0, (void*)0, (void*)0);
  Syscall(SYSCALL_ID_SIGNAL_REGISTER, (void*)2, (void*)handlerPong, (void*)0, (void*)0, (void*)0);

  threadAttr.mappedCPUs.mask[0] = 0xF;
  threadAttr.stackSize = 0x1000;
  threadAttr.priority = 20;
  memcpy(threadAttr.name, "Ping", 5);

  Syscall(SYSCALL_ID_THREAD_CREATE, (void*)&pThread1, (void*)&threadAttr, PingPongRoutine, (void*)0, (void*)0);

  memcpy(threadAttr.name, "Pong", 5);
  Syscall(SYSCALL_ID_THREAD_CREATE, (void*)&pThread2, (void*)&threadAttr, PingPongRoutine, (void*)1, (void*)0);
}

int main(void)
{
  void*  pThread;

  Syscall(SYSCALL_ID_THREAD_GET_SELF, &pThread, (void*)0, (void*)0, (void*)0, (void*)0);

  consoleFd = open("/dev/vga-text", O_RDWR, 0);
  write(consoleFd, "Opened console\n", 16);
  close(consoleFd);

  printf("Current thread is 0x%p\n", pThread);

  printf("Starting signal test with System Calls\n");
  testSignalSyscall(pThread);

  printf("Starting signal test with Interrupts\n");
  testSignalInterrupt();

  sleep(5);
  testPingPong();


  return 0;
}
