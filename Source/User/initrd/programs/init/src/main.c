#include <Syscall.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

int main(void)
{
  int    consoleFd;
  int    retVal;
  struct timespec sleepTime;
  struct timespec remainingTime;
  char   string[64];

  consoleFd = Syscall(SYSCALL_ID_OPEN, (void*)"/dev/vga-text", (void*)6, (void*)0, (void*)0, (void*)0);

  while (1)
  {
    if (consoleFd >= 0)
    {
      Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)"First\n", (void*)6, (void*)0, (void*)0);
    }
    sleepTime.tv_sec = 1;
    sleepTime.tv_nsec = 0;
    retVal = nanosleep(&sleepTime, &remainingTime);
    string[0] = '0' - retVal;
    string[1] = '0' + errno;
    string[2] = '\n';
    Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)string, (void*)3, (void*)0, (void*)0);
    sleepTime.tv_sec = -1;
    sleepTime.tv_nsec = 0;
    retVal = nanosleep(&sleepTime, &remainingTime);
    string[0] = '0' - retVal;
    string[1] = '0' + errno;
    string[2] = '\n';
    Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)string, (void*)3, (void*)0, (void*)0);

    errno = 0;
    if (consoleFd >= 0)
    {
      Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)"Second\n", (void*)7, (void*)0, (void*)0);
    }
    retVal = sleep(1);
    string[0] = '0' - retVal;
    string[1] = '0' + errno;
    string[2] = '\n';
    Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)string, (void*)3, (void*)0, (void*)0);
    retVal = sleep(0);
    string[0] = '0' - retVal;
    string[1] = '0' + errno;
    string[2] = '\n';
    Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)string, (void*)3, (void*)0, (void*)0);

    errno = 0;
    if (consoleFd >= 0)
    {
      Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)"Third\n", (void*)6, (void*)0, (void*)0);
    }
    retVal = usleep(1000000);
    string[0] = '0' - retVal;
    string[1] = '0' + errno;
    string[2] = '\n';
    Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)string, (void*)3, (void*)0, (void*)0);
    retVal = usleep(0);
    string[0] = '0' - retVal;
    string[1] = '0' + errno;
    string[2] = '\n';
    Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)string, (void*)3, (void*)0, (void*)0);
  }

  return 0;
}
