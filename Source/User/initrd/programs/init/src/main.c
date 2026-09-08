#include <Syscall.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

int consoleFd;
int nestedTest = 0;

void testSignalSyscall(void* pThread);
void testSignalInterrupt(void);

void* threadRoutine(void* pParam);

void handler(int signum, void* uContext);
void handler2(int signum, void* uContext);
void handler3(int signum, void* uContext);

void testPingPong(void);
void* PingPongRoutine(void* pParam);
void handlerPong(int signum, void* uContext);
void handlerPing(int signum, void* uContext);

void handler(int signum, void* uContext)
{
  char string[64];

  void* pThread;
  Syscall(SYSCALL_ID_THREAD_GET_SELF, &pThread, (void*)0, (void*)0, (void*)0, (void*)0);

  if (signum == 1)
  {

    Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)" -> ", (void*)4, (void*)0, (void*)0);
    string[0] = '0' + signum / 10;
    string[1] = '0' + (signum % 10);
    string[2] = '.';
    string[3] = '0' + nestedTest;
    Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)string, (void*)4, (void*)0, (void*)0);

    if (signum == 1 && nestedTest < 3)
    {
      nestedTest++;
      Syscall(SYSCALL_ID_SIGNAL, (void*)1, pThread, (void*)0, (void*)0, (void*)0);
      --nestedTest;
    }

    Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)" -> ", (void*)4, (void*)0, (void*)0);
    string[0] = '0' + signum / 10;
    string[1] = '0' + (signum % 10);
    string[2] = '.';
    string[3] = '0' + nestedTest;
    Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)string, (void*)4, (void*)0, (void*)0);

    if (nestedTest == 0)
    {
      Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)"\n", (void*)1, (void*)0, (void*)0);
    }
  }
  else
  {
    Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)" -> Sig ", (void*)8, (void*)0, (void*)0);
    string[0] = '0' + signum / 10;
    string[1] = '0' + (signum % 10);
    Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)string, (void*)2, (void*)0, (void*)0);
  }


  Syscall(SYSCALL_ID_SIGNAL_RETURN, uContext, (void*)0, (void*)0, (void*)0, (void*)0);

  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)"Invalid signal\n", (void*)15, (void*)0, (void*)0);
}

volatile int sigRecv = 0;
void* pThreadMain;

void handler2(int signum, void* uContext)
{
  char string[64];

  void* pThread;
  Syscall(SYSCALL_ID_THREAD_GET_SELF, &pThread, (void*)0, (void*)0, (void*)0, (void*)0);

  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)" -> ", (void*)4, (void*)0, (void*)0);
  string[0] = '0' + signum / 10;
  string[1] = '0' + (signum % 10);
  string[2] = '.';
  string[3] = '0' + nestedTest;
  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)string, (void*)4, (void*)0, (void*)0);

  if (signum == 1 && nestedTest < 3)
  {
    nestedTest++;
    Syscall(SYSCALL_ID_SIGNAL, (void*)1, pThread, (void*)0, (void*)0, (void*)0);
    --nestedTest;
  }

  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)" -> ", (void*)4, (void*)0, (void*)0);
  string[0] = '0' + signum / 10;
  string[1] = '0' + (signum % 10);
  string[2] = '.';
  string[3] = '0' + nestedTest;
  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)string, (void*)4, (void*)0, (void*)0);

  if (nestedTest == 0)
  {
    Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)"\n", (void*)1, (void*)0, (void*)0);
    Syscall(SYSCALL_ID_SIGNAL, (void*)1, pThreadMain, (void*)0, (void*)0, (void*)0);
    sigRecv = 1;
  }

  Syscall(SYSCALL_ID_SIGNAL_RETURN, uContext, (void*)0, (void*)0, (void*)0, (void*)0);
}

void handler3(int signum, void* uContext)
{
  char string[64];

  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)" -> ", (void*)4, (void*)0, (void*)0);
  string[0] = '0' + signum / 10;
  string[1] = '0' + (signum % 10);
  string[2] = 'H';
  string[3] = '3';
  string[4] = '\n';
  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)string, (void*)5, (void*)0, (void*)0);

  Syscall(SYSCALL_ID_SIGNAL_RETURN, uContext, (void*)0, (void*)0, (void*)0, (void*)0);
}

void testSignalSyscall(void* pThread)
{
  int    retVal;
  char   string[64];

  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)"Registering signal handler 1: ", (void*)30, (void*)0, (void*)0);
  retVal = Syscall(SYSCALL_ID_SIGNAL_REGISTER, (void*)1, (void*)handler, (void*)0, (void*)0, (void*)0);
  string[0] = '0' - retVal;
  string[1] = '0' + errno;
  string[2] = '\n';
  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)string, (void*)3, (void*)0, (void*)0);
  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)"Registering signal handler 2: ", (void*)30, (void*)0, (void*)0);
  retVal = Syscall(SYSCALL_ID_SIGNAL_REGISTER, (void*)2, (void*)handler, (void*)0, (void*)0, (void*)0);
  string[0] = '0' - retVal;
  string[1] = '0' + errno;
  string[2] = '\n';
  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)string, (void*)3, (void*)0, (void*)0);

  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)"=> Send 1\n", (void*)10, (void*)0, (void*)0);
  retVal = Syscall(SYSCALL_ID_SIGNAL, (void*)1, pThread, (void*)0, (void*)0, (void*)0);
  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)"=> Send 1 Returned: ", (void*)20, (void*)0, (void*)0);
  string[0] = '0' - retVal;
  string[1] = '0' + errno;
  string[2] = '\n';
  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)string, (void*)3, (void*)0, (void*)0);
  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)"\n=> Send 2\n", (void*)11, (void*)0, (void*)0);
  retVal = Syscall(SYSCALL_ID_SIGNAL, (void*)2, pThread, (void*)0, (void*)0, (void*)0);
  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)"\n=> Send 2 Returned: ", (void*)21, (void*)0, (void*)0);
  string[0] = '0' - retVal;
  string[1] = '0' + errno;
  string[2] = '\n';
  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)string, (void*)3, (void*)0, (void*)0);
}

void* threadRoutine(void* pParam)
{
  int   retVal;
  char  string[64];
  int   value;

  (void)pParam;
  value = (int)(unsigned long long)pParam;

  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)"Thread started with arg: ", (void*)25, (void*)0, (void*)0);

  string[0] = '0' + value / 10;
  string[1] = '0' + value % 10;
  string[2] = '\n';
  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)string, (void*)3, (void*)0, (void*)0);

  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)"Registering signal handler 1: ", (void*)30, (void*)0, (void*)0);
  retVal = Syscall(SYSCALL_ID_SIGNAL_REGISTER, (void*)1, (void*)handler2, (void*)0, (void*)0, (void*)0);
  string[0] = '0' - retVal;
  string[1] = '0' + errno;
  string[2] = '\n';
  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)string, (void*)3, (void*)0, (void*)0);

  while (sigRecv != 1) {}

  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)"Thread finished\n", (void*)16, (void*)0, (void*)0);

  Syscall(SYSCALL_ID_THREAD_EXIT, (void*)28, (void*)0, (void*)0, (void*)0, (void*)0);

  return NULL;
}

void testSignalInterrupt(void)
{
  int   retVal;
  char  string[64];
  void* pThread;
  int threadRetVal;
  S_ThreadAttr threadAttr;

  Syscall(SYSCALL_ID_THREAD_GET_SELF, &pThreadMain, (void*)0, (void*)0, (void*)0, (void*)0);

  threadAttr.mappedCPUs.mask[0] = 1;
  threadAttr.stackSize = 0x1000;
  threadAttr.priority = 20;
  threadAttr.name[0] = 'T';
  threadAttr.name[1] = 'e';
  threadAttr.name[2] = 's';
  threadAttr.name[3] = 't';
  threadAttr.name[4] = 'S';
  threadAttr.name[5] = 'i';
  threadAttr.name[6] = 'g';
  threadAttr.name[7] = 'n';
  threadAttr.name[8] = 'a';
  threadAttr.name[9] = 'l';
  threadAttr.name[10] = '\0';

  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)"Registering signal handler 1: ", (void*)30, (void*)0, (void*)0);
  retVal = Syscall(SYSCALL_ID_SIGNAL_REGISTER, (void*)1, (void*)handler3, (void*)0, (void*)0, (void*)0);
  string[0] = '0' - retVal;
  string[1] = '0' + errno;
  string[2] = '\n';
  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)string, (void*)3, (void*)0, (void*)0);

  /* Create the thread */
  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)"Creating thread for signal test\n", (void*)32, (void*)0, (void*)0);
  retVal = Syscall(SYSCALL_ID_THREAD_CREATE, (void*)&pThread, (void*)&threadAttr, threadRoutine, (void*)53, (void*)0);
  string[0] = '0' - retVal;
  string[1] = '0' + errno;
  string[2] = '\n';
  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)string, (void*)3, (void*)0, (void*)0);

  /* Sleep for a bit*/
  sleep(2);

  /* Signal the thread */
  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)"=> Send 1\n", (void*)10, (void*)0, (void*)0);
  retVal = Syscall(SYSCALL_ID_SIGNAL, (void*)1, pThread, (void*)0, (void*)0, (void*)0);
  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)"=> Send 1 Returned: ", (void*)20, (void*)0, (void*)0);
  string[0] = '0' - retVal;
  string[1] = '0' + errno;
  string[2] = '\n';
  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)string, (void*)3, (void*)0, (void*)0);

  /* Join the thread */
  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)"Joining thread for signal test\n", (void*)31, (void*)0, (void*)0);
  retVal = Syscall(SYSCALL_ID_THREAD_JOIN, (void*)pThread, (void*)&threadRetVal, (void*)0, (void*)0, (void*)0);
  string[0] = '0' - retVal;
  string[1] = '0' + errno;
  string[2] = '-';
  string[3] = '0' + threadRetVal / 10;
  string[4] = '0' + threadRetVal % 10;
  string[5] = '\n';
  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)string, (void*)6, (void*)0, (void*)0);
}

void handlerPing(int signum, void* uContext)
{
  (void)signum;
  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)" -> Pi", (void*)6, (void*)0, (void*)0);
  Syscall(SYSCALL_ID_SIGNAL_RETURN, uContext, (void*)0, (void*)0, (void*)0, (void*)0);
}

void handlerPong(int signum, void* uContext)
{
  (void)signum;
  Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)" -> Po", (void*)6, (void*)0, (void*)0);
  Syscall(SYSCALL_ID_SIGNAL_RETURN, uContext, (void*)0, (void*)0, (void*)0, (void*)0);
}

void*           pThread1;
void*           pThread2;

void* PingPongRoutine(void* pParam)
{
  int             retVal;
  char            string[64];
  int             value;
  struct timespec sleepTime;
  struct timespec remainingTime;
  value = (int)(unsigned long long)pParam;

  if (value == 0)
  {
    Syscall(SYSCALL_ID_SIGNAL_REGISTER, (void*)1, (void*)handlerPing, (void*)0, (void*)0, (void*)0);
  }
  else
  {
    Syscall(SYSCALL_ID_SIGNAL_REGISTER, (void*)1, (void*)handlerPong, (void*)0, (void*)0, (void*)0);
  }

  while(1)
  {
    if (value == 0)
    {
      sleepTime.tv_nsec = 0;
      sleepTime.tv_sec  = 100;
      retVal = nanosleep(&sleepTime, &remainingTime);
      if (retVal >= 0 || errno != EINTR)
      {
        Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)"Ping thread intr sleep failed\n", (void*)31, (void*)0, (void*)0);
      }

      string[0] = 'n';
      string[1] = 'g';
      string[2] = ' ';
      string[3] = '0' + remainingTime.tv_sec / 10000;
      string[4] = '0' + remainingTime.tv_sec % 10000 / 1000;
      string[5] = '0' + remainingTime.tv_sec % 1000 / 100;
      string[6] = '0' + remainingTime.tv_sec % 100 / 10;
      string[7] = '0' + remainingTime.tv_sec % 10;
      string[8] = '.';
      string[9] = '0' + remainingTime.tv_nsec / 100000000;
      string[10] = '0' + remainingTime.tv_nsec % 100000000 / 10000000;
      string[11] = '0' + remainingTime.tv_nsec % 10000000 / 1000000;
      string[12] = '0' + remainingTime.tv_nsec % 1000000 / 100000;
      string[13] = '0' + remainingTime.tv_nsec % 100000 / 10000;
      string[14] = '0' + remainingTime.tv_nsec % 10000 / 1000;
      string[15] = '0' + remainingTime.tv_nsec % 1000 / 100;
      string[16] = '0' + remainingTime.tv_nsec % 100 / 10;
      string[17] = '0' + remainingTime.tv_nsec % 10;
      string[18] = '\n';
      Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)string, (void*)19, (void*)0, (void*)0);
      sleepTime.tv_nsec = 5000;
      sleepTime.tv_sec  = 0;
      retVal = nanosleep(&sleepTime, &remainingTime);
      if (retVal != 0)
      {
        Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)"Ping thread sleep failed\n", (void*)27, (void*)0, (void*)0);
      }

      Syscall(SYSCALL_ID_SIGNAL, (void*)1, (void*)pThread2, (void*)0, (void*)0, (void*)0);
    }
    else
    {
      Syscall(SYSCALL_ID_SIGNAL, (void*)1, (void*)pThread1, (void*)0, (void*)0, (void*)0);

      sleepTime.tv_nsec = 0;
      sleepTime.tv_sec  = 100;
      retVal = nanosleep(&sleepTime, &remainingTime);
      if (retVal >= 0 || errno != EINTR)
      {
        Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)"Ping thread intr sleep failed\n", (void*)31, (void*)0, (void*)0);
      }

      string[0] = 'n';
      string[1] = 'g';
      string[2] = ' ';
      string[3] = '0' + remainingTime.tv_sec / 10000;
      string[4] = '0' + remainingTime.tv_sec % 10000 / 1000;
      string[5] = '0' + remainingTime.tv_sec % 1000 / 100;
      string[6] = '0' + remainingTime.tv_sec % 100 / 10;
      string[7] = '0' + remainingTime.tv_sec % 10;
      string[8] = '.';
      string[9] = '0' + remainingTime.tv_nsec / 100000000;
      string[10] = '0' + remainingTime.tv_nsec % 100000000 / 10000000;
      string[11] = '0' + remainingTime.tv_nsec % 10000000 / 1000000;
      string[12] = '0' + remainingTime.tv_nsec % 1000000 / 100000;
      string[13] = '0' + remainingTime.tv_nsec % 100000 / 10000;
      string[14] = '0' + remainingTime.tv_nsec % 10000 / 1000;
      string[15] = '0' + remainingTime.tv_nsec % 1000 / 100;
      string[16] = '0' + remainingTime.tv_nsec % 100 / 10;
      string[17] = '0' + remainingTime.tv_nsec % 10;
      string[18] = '\n';
      Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)string, (void*)19, (void*)0, (void*)0);
      sleepTime.tv_nsec = 500;
      sleepTime.tv_sec  = 0;
      retVal = nanosleep(&sleepTime, &remainingTime);
      if (retVal != 0)
      {
        Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)"Ping thread sleep failed\n", (void*)27, (void*)0, (void*)0);
      }
    }
  }

  return NULL;
}

void testPingPong(void)
{
  S_ThreadAttr    threadAttr;

  threadAttr.mappedCPUs.mask[0] = 1;
  threadAttr.stackSize = 0x1000;
  threadAttr.priority = 20;
  threadAttr.name[0] = 'P';
  threadAttr.name[1] = 'i';
  threadAttr.name[2] = 'n';
  threadAttr.name[3] = 'g';
  threadAttr.name[4] = '\0';

  Syscall(SYSCALL_ID_THREAD_CREATE, (void*)&pThread1, (void*)&threadAttr, PingPongRoutine, (void*)0, (void*)0);

  threadAttr.name[0] = 'P';
  threadAttr.name[1] = 'o';
  threadAttr.name[2] = 'n';
  threadAttr.name[3] = 'g';
  threadAttr.name[4] = '\0';

  Syscall(SYSCALL_ID_THREAD_CREATE, (void*)&pThread2, (void*)&threadAttr, PingPongRoutine, (void*)1, (void*)0);
}

int main(void)
{
  void*  pThread;

  Syscall(SYSCALL_ID_THREAD_GET_SELF, &pThread, (void*)0, (void*)0, (void*)0, (void*)0);

  consoleFd = Syscall(SYSCALL_ID_OPEN, (void*)"/dev/vga-text", (void*)6, (void*)0, (void*)0, (void*)0);

  testSignalSyscall(pThread);
  testSignalInterrupt();
  testPingPong();

  while(1)
  {
    sleep(100);
  }

  return 0;
}
