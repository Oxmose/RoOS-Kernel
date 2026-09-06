/*******************************************************************************
 * @file TimerManagerTest.c
 *
 * @brief Kernel time manager integration tests.
 ******************************************************************************/
#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <CPU.h>
#include <stdint.h>
#include <TimerManager.h>

/* Header file */
#include <TestFramework.h>

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
void TimeManagerTest(void)
{
  S_Date    date;
  S_DayTime daytime;
  S_TimeSpec timeSpec;
  uint64_t  startTime;
  uint64_t  endTime;
  uint64_t  cpuCount;
  uint64_t  startTicks;
  uint64_t  endTicks;
  uint64_t  invalidTicks;
  uint8_t   currentCpu;
  uint8_t   invalidCpu;

  /* Exercise all public data types and their documented fields. */
  timeSpec.tvSec = 0;
  timeSpec.tvNsec = 0;
  TEST_POINT_ASSERT_DWORD(TEST_TIME_TYPES_ID,
                          sizeof(timeSpec) >= sizeof(T_Time),
                          sizeof(T_Time),
                          sizeof(timeSpec),
                          TEST_TIME_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_TIME_TYPES_ID + 1,
                         MAIN_TIMER < RTC_TIMER && RTC_TIMER < AUX_TIMER &&
                         AUX_TIMER < LIFETIME_TIMER,
                         1,
                         MAIN_TIMER < RTC_TIMER && RTC_TIMER < AUX_TIMER &&
                         AUX_TIMER < LIFETIME_TIMER,
                         TEST_TIME_ENABLED);

  startTime = TimeGetUptime();
  TimeWaitNoScheduler(0);
  endTime = TimeGetUptime();
  TEST_POINT_ASSERT_UDWORD(TEST_TIME_UPTIME_ID,
                           endTime >= startTime,
                           1,
                           endTime >= startTime,
                           TEST_TIME_ENABLED);

  daytime = TimeGetDayTime();
  TEST_POINT_ASSERT_UBYTE(TEST_TIME_DAYTIME_ID,
                          daytime.hours < 24,
                          1,
                          daytime.hours < 24,
                          TEST_TIME_ENABLED);
  TEST_POINT_ASSERT_UBYTE(TEST_TIME_DAYTIME_ID + 1,
                          daytime.minutes < 60,
                          1,
                          daytime.minutes < 60,
                          TEST_TIME_ENABLED);
  TEST_POINT_ASSERT_UBYTE(TEST_TIME_DAYTIME_ID + 2,
                          daytime.seconds < 60,
                          1,
                          daytime.seconds < 60,
                          TEST_TIME_ENABLED);

  date = TimeGetDate();
  TEST_POINT_ASSERT_HUINT(TEST_TIME_DATE_ID,
                          date.month >= 1 && date.month <= 12,
                          1,
                          date.month,
                          TEST_TIME_ENABLED);
  TEST_POINT_ASSERT_HUINT(TEST_TIME_DATE_ID + 1,
                          date.day >= 1 && date.day <= 31,
                          1,
                          date.day,
                          TEST_TIME_ENABLED);
  TEST_POINT_ASSERT_HUINT(TEST_TIME_DATE_ID + 2,
                          date.year >= 1970,
                          1970,
                          date.year,
                          TEST_TIME_ENABLED);

  currentCpu = CPUGetId();
  cpuCount = CPUGetCount();
  invalidCpu = cpuCount < 256 ? (uint8_t)cpuCount : UINT8_MAX;
  startTicks = TimeGetTicks(currentCpu);
  endTicks = TimeGetTicks(currentCpu);
  invalidTicks = TimeGetTicks(invalidCpu);
  TEST_POINT_ASSERT_UDWORD(TEST_TIME_TICKS_ID,
                           currentCpu < cpuCount,
                           1,
                           currentCpu < cpuCount,
                           TEST_TIME_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_TIME_TICKS_ID + 1,
                           endTicks >= startTicks,
                           1,
                           endTicks >= startTicks,
                           TEST_TIME_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_TIME_TICKS_ID + 2,
                           cpuCount < 256 && invalidTicks == 0,
                           0,
                           cpuCount < 256 && invalidTicks == 0,
                           TEST_TIME_ENABLED);

  startTime = TimeGetUptime();
  TimeWaitNoScheduler(1000000);
  endTime = TimeGetUptime();
  TEST_POINT_ASSERT_UDWORD(TEST_TIME_WAIT_ZERO_ID,
                           endTime >= startTime,
                           1,
                           endTime >= startTime,
                           TEST_TIME_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_TIME_WAIT_SHORT_ID,
                           endTime - startTime >= 1000000,
                           1000000,
                           endTime - startTime,
                           TEST_TIME_ENABLED);

  TEST_FRAMEWORK_END();
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/