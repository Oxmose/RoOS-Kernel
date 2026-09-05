#include <Syscall.h>

int main(void)
{
  int consoleFd;

  consoleFd = Syscall(SYSCALL_ID_OPEN, (void*)"/dev/vga-text", (void*)6, (void*)0, (void*)0, (void*)0);

  while (1)
  {
    if (consoleFd >= 0)
    {
      Syscall(SYSCALL_ID_WRITE, (void*)(unsigned long long)consoleFd, (void*)"Hello, World!\n", (void*)14, (void*)0, (void*)0);
    }
    Syscall(SYSCALL_ID_SLEEP, (void*)1000000000, (void*)0, (void*)0, (void*)0, (void*)0);
  }

  return 0;
}
