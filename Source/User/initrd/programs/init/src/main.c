#include <Syscall.h>

int main(void)
{
  while (1)
  {
    Syscall(0, (void*)1, (void*)2, (void*)3, (void*)4, (void*)5);
  }

  return 0;
}
