#if 0
#include <CPU.h>
#include <Console.h>
#include <Scheduler.h>
#include <KernelOutput.h>

void TestKernel(void);
void* StatsInfoRoutine(void* args);

void* StatsInfoRoutine(void* args)
{
  uint32_t i;
  double usage;
  uint32_t cpuCount;
  const S_ScheduleContextStatistics *pStats;
  S_ConsoleCursor cursor;

  (void)args;

  ConsoleClear();
  cpuCount = CPUGetCount();
  cursor.x = 0;
  cursor.y = 0;

  while (true)
  {
    ConsoleClear();
    ConsoleSetCursor(&cursor);
    KPrintfDebug("TimeTest\n");

    KPrintf("> Data From Core %d\n", CPUGetId());
    for (i = 0; i < cpuCount; ++i)
    {
      pStats = SchedulerGetStatistics(i);

      usage = 100.0 - (100 * (double)pStats->idleTime / (double)pStats->totalTime);

      KPrintf("==== CPU %d\n", i);
      KPrintf("Usage: %f%% | Idle Time: %llu | Total Time: %llu\n", usage, pStats->idleTime, pStats->totalTime);
      KPrintf("Score %llu\n\n", pStats->score);
    }


    SleepNs(100000000);

  }

  return NULL;
}

void TestKernel(void)
{
  char            name[32] = "StartThread\0";
  S_KernelThread* pThread;
  S_CPUMask       cpuMask;

  CPU_MASK_RESET(cpuMask);
  CPU_MASK_SET(cpuMask, 0);
  CPU_MASK_SET(cpuMask, 1);
  CPU_MASK_SET(cpuMask, 2);
  CPU_MASK_SET(cpuMask, 3);

  if (CreateThread(&pThread, true, 0, name, 0x1000, cpuMask, StatsInfoRoutine, NULL) != NO_ERROR)
  {
    KPrintf("Failed to create test thread\n");
  }
}
#endif