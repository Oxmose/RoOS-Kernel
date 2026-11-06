/*******************************************************************************
 * @file RTC.c
 *
 * @see RTC.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 25/05/2024
 *
 * @version 2.0
 *
 * @brief RTC (Real Time Clock) driver.
 *
 * @details RTC (Real Time Clock) driver. Used as the kernel's time base. Timer
 * source in the kernel. This driver provides basic access to the RTC.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <Panic.h>
#include <X64Cpu.h>
#include <ProcFS.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <IOAPIC.h>
#include <Critical.h>
#include <KernelHeap.h>
#include <Interrupts.h>
#include <TimerManager.h>
#include <DriverManager.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* No unit test: this module is tested in real-world conditions. */

/* Header file */
#include <RTC.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief PROCFS directory path */
#define PROCFS_DIR_PATH "rtc"

/** @brief Initial RTC rate */
#define RTC_INIT_RATE_HZ 2

/* CMOS registers  */
/** @brief CMOS seconds register id. */
#define CMOS_SECONDS_REGISTER 0x00
/** @brief CMOS minutes register id. */
#define CMOS_MINUTES_REGISTER 0x02
/** @brief CMOS hours register id. */
#define CMOS_HOURS_REGISTER   0x04
/** @brief CMOS day of the week register id. */
#define CMOS_WEEKDAY_REGISTER 0x06
/** @brief CMOS day register id. */
#define CMOS_DAY_REGISTER     0x07
/** @brief CMOS month register id. */
#define CMOS_MONTH_REGISTER   0x08
/** @brief CMOS year register id. */
#define CMOS_YEAR_REGISTER    0x09
/** @brief CMOS century register id. */
#define CMOS_CENTURY_REGISTER 0x32

/* CMOS setings */
/** @brief CMOS NMI disabler bit. */
#define CMOS_NMI_DISABLE_BIT 0x01
/** @brief CMOS RTC enabler bit. */
#define CMOS_ENABLE_RTC 0x40
/** @brief CMOS A register id. */
#define CMOS_REG_A 0x0A
/** @brief CMOS B register id. */
#define CMOS_REG_B 0x0B
/** @brief CMOS C register id. */
#define CMOS_REG_C 0x0C

/** @brief RTC IRQ number */
#define RTC_IRQ_NUMBER 8

/** @brief RTC CPU communication port */
#define RTC_CPU_COMM_PORT 0x70
/** @brief RTC CPU data port */
#define RTC_CPU_DATA_PORT 0x71

/** @brief RTC quartz frequency */
#define RTC_QUART_FREQUENCY 32768
/** @brief RTC frequency low boundary */
#define RTC_FREQUENCY_LOW 2
/** @brief RTC frequency high boundary */
#define RTC_FREQUENCY_HIGH 8192

/** @brief Current module name */
#define MODULE_NAME "X86 RTC"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief x86 RTC driver controller. */
typedef struct
{
  /** @brief RTC IRQ number. */
  int16_t irqNumber;
  /** @brief Selected interrupt frequency. */
  uint64_t selectedFrequency;
  /** @brief Keeps track on the RTC enabled state. */
  uint32_t disabledNesting;
  /** @brief Driver's lock */
  T_Spinlock lock;
} S_RTCControler;

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/**
 * @brief Assert macro used by the RTC to ensure correctness of execution.
 *
 * @details Assert macro used by the RTC to ensure correctness of execution.
 * Due to the critical nature of the RTC, any error generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define RTC_ASSERT(COND, MSG, ERROR) {                    \
  if ((COND) == false)                                    \
  {                                                       \
    PANIC(ERROR, MODULE_NAME, MSG, false, false);         \
  }                                                       \
}

/** @brief Cast a pointer to a RTC driver controller */
#define GET_CONTROLER(PTR) ((S_RTCControler*)PTR)

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Attaches the RTC driver to the system.
 *
 * @details Attaches the RTC driver to the system. This function will initialize
 * the RTC hardware and retreive the RTC parameters.
 *
 * @return The success state or the error code.
 */
static E_Return _Attach(void);

/**
 * @brief Initial RTC interrupt handler.
 *
 * @details RTC interrupt handler set at the initialization of the RTC.
 * Dummy routine setting EOI.
 *
 * @return Returns if the scheduler shall be called on return.
 */
static bool _DummyHandler(void);

/**
 * @brief Enables RTC ticks.
 *
 * @details Enables RTC ticks by clearing the RTC's IRQ mask.
 *
 * @param[in, out] pDrvCtrl The driver controller used by the registered
 * driver.
 */
static void _Enable(void* pDrvCtrl);

/**
 * @brief Disables RTC ticks.
 *
 * @details Disables RTC ticks by setting the RTC's IRQ mask.
 *
 * @param[in, out] pDrvCtrl The driver controller used by the registered
 * driver.
 */
static void _Disable(void* pDrvCtrl);

/**
 * @brief Sets the RTC's tick frequency.
 *
 * @details Sets the RTC's tick frequency. The value must be between 2Hz and
 * 8192Hz.
 *
 * @param[in, out] pDrvCtrl The driver controller used by the registered
 * driver.
 * @param[in] kFrequency The new frequency to be set to the RTC.
 *
 * @warning The value must be between 2Hz and 8192Hz. The lower boundary RTC
 * frequency will be selected (refer to the code to understand the 14 available
 * frequencies).
 */
static void _SetFrequency(void* pDrvCtrl, const uint64_t kFrequency);

/**
 * @brief Returns the RTC tick frequency in Hz.
 *
 * @details Returns the RTC tick frequency in Hz.
 *
 * @param[in, out] pDrvCtrl The driver controller used by the registered
 * driver.
 *
 * @return The RTC tick frequency in Hz.
 */
static uint64_t _GetFrequency(void* pDrvCtrl);

/**
 * @brief Sets the RTC tick handler.
 *
 * @details Sets the RTC tick handler. This function will be called at each RTC
 * tick received.
 *
 * @param[in, out] pDrvCtrl The driver controller used by the registered
 * driver.
 * @param[in] handler The handler of the RTC interrupt.
 *
 * @return The success state or the error code.
 */
static E_Return _SetHandler(void* pDrvCtrl, T_InterruptHandler handler);

/**
 * @brief Removes the RTC tick handler.
 *
 * @details Removes the RTC tick handler.
 *
 * @param[in, out] pDrvCtrl The driver controller used by the registered
 * driver.
 *
 * @return The success state or the error code.
 */
static E_Return _RemoveHandler(void* pDrvCtrl);

/**
 * @brief Returns the current date.
 *
 * @details Returns the current date in RTC date format.
 *
 * @param[in, out] pDrvCtrl The driver controller used by the registered
 * driver.
 *
 * @return The current date in in RTC date format
 */
static S_Date _GetDate(void* pDrvCtrl);

/**
 * @brief Returns the current daytime in seconds.
 *
 * @details Returns the current daytime in seconds.
 *
 * @param[in, out] pDrvCtrl The driver controller used by the registered
 * driver.
 *
 * @return The current daytime in seconds.
 */
static S_DayTime _GetDaytime(void* pDrvCtrl);

/**
 * @brief Updates the system's time and date.
 *
 * @details Updates the system's time and date. This function also reads the
 * CMOS registers. By doing that, the RTC registers are cleaned and the RTC able
 * to interrupt the CPU again.
 *
 * @param[in, out] pDrvCtrl The driver controller used by the registered
 * driver.
 * @param[out] pDate The date to update.
 * @param[out] pTime The time to update.
 *
 * @warning You MUST call that function in every RTC handler or the RTC will
 * never raise interrupt again.
 */
static void _UpdateTime(void* pDrvCtrl, S_Date* pDate, S_DayTime* pTime);

/**
 * @brief Sends EOI to RTC itself.
 *
 * @details Sends EOI to RTC itself. The RTC requires to acknoledge its
 * interrupts otherwise, no further interrupt is generated.
 *
 * @param[in, out] pDrvCtrl The driver controller used by the registered
 * driver.
 */
static void _AcknowledgeInt(void* pDrvCtrl);

/**
 * @brief Opens the ProcFS main entry.
 *
 * @details Opens the ProcFS main entry. This function will open the
 * ProcFS main entry and return a pointer to the file handle. The file handle
 * will be used to read the information from the kernel.
 *
 * @param[in] pDriverData The driver data pointer.
 * @param[in] kpPath The path to the ProcFS entry.
 * @param[in] flags The flags for opening the file.
 * @param[in] mode The mode for opening the file.
 *
 * @return The pointer to the file handle or -1 in case of error.
 */
static void* _ProcFSOpen(void*       pDriverData,
                         const char* kpPath,
                         int32_t     flags,
                         int32_t     mode);

/**
 * @brief Closes the ProcFS main entry.
 *
 * @details Closes the ProcFS main entry. This function will close the
 * ProcFS main entry and free the resources used by the entry.
 *
 * @param[in] pDriverData The driver data pointer.
 * @param[in] pFileHandle The file handle pointer.
 *
 * @return The success state or the error code.
 */
static int32_t _ProcFSClose(void* pDriverData, void* pFileHandle);

/**
 * @brief Read the ProcFS main entry.
 *
 * @details Read the ProcFS main entry. This function will read the
 * information from the kernel and write it to the buffer given as parameter.
 * The function will return the number of bytes read or an error code.
 *
 * @param[in] pDriverData The driver data pointer.
 * @param[in] pFileHandle The file handle pointer.
 * @param[out] pBuffer The buffer to write the information to.
 * @param[in] count The size of the buffer given as parameter.
 *
 * @return The number of bytes read or an error code.
 */
static ssize_t _ProcFSRead(void*  pDriverData,
                           void*  pFileHandle,
                           void*  pBuffer,
                           size_t count);

/**
 * @brief Creates the ProcFS entry for the driver.
 *
 * @details Creates the ProcFS entry for the driver. This function will
 * create the ProcFS entry for the driver and register it in the ProcFS.
 * The entry will be used to read the information from the kernel. The
 * entry will be created in the /proc/rtc directory.
 *
 * @return The success state or the error code.
 */
static E_Return _CreateProcFSEntry(void);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief RTC driver instance. */
static S_LayerDriver  sX86RTCDriver =
{
  .pName         = "X86 RTC Driver",
  .pDescription  = "X86 Real Time Clock Driver for roOs",
  .pVersion      = "2.0",
  .layer         = 2,
  .pDriverAttach = _Attach
};

/** @brief ProcFS entry for the RTC driver  */
static S_ProcFSDirEntry* spProcFSEntry = NULL;

/** @brief PROCFS operations */
static S_ProcFSFileOperations sProcFSOps =
{
  .pOpen    = _ProcFSOpen,
  .pClose   = _ProcFSClose,
  .pRead    = _ProcFSRead,
  .pWrite   = NULL,
  .pReadDir = NULL,
  .pIOCTL   = NULL
};

/** @brief RTC driver control structure */
static S_RTCControler* spDrvCtrl = NULL;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static E_Return _Attach(void)
{
  E_Return       retCode;
  int8_t         prevOred;
  int8_t         prevRate;
  S_KernelTimer* pTimerDrv;
  E_Return       error;

  if (spDrvCtrl == NULL)
  {
    /* Init structures */
    spDrvCtrl = KMalloc(sizeof(S_RTCControler), KMALLOC_NO_FREE_POOL);
    memset(spDrvCtrl, 0, sizeof(S_RTCControler));
    SPINLOCK_INIT(spDrvCtrl->lock);

    pTimerDrv = KMalloc(sizeof(S_KernelTimer), KMALLOC_NO_FREE_POOL);
    memset(pTimerDrv, 0, sizeof(S_KernelTimer));

    pTimerDrv->pGetFrequency  = _GetFrequency;
    pTimerDrv->pGetDate       = _GetDate;
    pTimerDrv->pGetDaytime    = _GetDaytime;
    pTimerDrv->pEnable        = _Enable;
    pTimerDrv->pDisable       = _Disable;
    pTimerDrv->pSetHandler    = _SetHandler;
    pTimerDrv->pRemoveHandler = _RemoveHandler;
    pTimerDrv->pTickManager   = _AcknowledgeInt;
    pTimerDrv->pDriverCtrl    = spDrvCtrl;

    spDrvCtrl->irqNumber = AllocateInterruptLine();
    RTC_ASSERT(spDrvCtrl->irqNumber >= 0,
               "Failed to allocate interrupt line.",
               ERR_EXCEEDED_LIMIT);
    spDrvCtrl->selectedFrequency = RTC_INIT_RATE_HZ;

    /* Init system times */
    spDrvCtrl->disabledNesting = 1;

    /* Init CMOS IRQ8 */
    CPUPortWriteByte((CMOS_NMI_DISABLE_BIT << 7) | CMOS_REG_B,
                    RTC_CPU_COMM_PORT);
    prevOred = CPUPortReadByte(RTC_CPU_DATA_PORT);
    CPUPortWriteByte((CMOS_NMI_DISABLE_BIT << 7) | CMOS_REG_B,
                    RTC_CPU_COMM_PORT);
    CPUPortWriteByte(prevOred | CMOS_ENABLE_RTC, RTC_CPU_DATA_PORT);

    /* Init CMOS IRQ8 rate */
    CPUPortWriteByte((CMOS_NMI_DISABLE_BIT << 7) | CMOS_REG_A,
                    RTC_CPU_COMM_PORT);
    prevRate = CPUPortReadByte(RTC_CPU_DATA_PORT);
    CPUPortWriteByte((CMOS_NMI_DISABLE_BIT << 7) | CMOS_REG_A,
                    RTC_CPU_COMM_PORT);
    CPUPortWriteByte((prevRate & 0xF0) | RTC_INIT_RATE_HZ, RTC_CPU_DATA_PORT);

    /* Set RTC frequency */
    _SetFrequency(spDrvCtrl, spDrvCtrl->selectedFrequency);

    /* Just dummy read register C to unlock interrupt */
    _AcknowledgeInt(spDrvCtrl);

    error = TimeManagerAddTimer(pTimerDrv, RTC_TIMER);
    RTC_ASSERT(error == NO_ERROR,
               "Failed to register the RTC driver in the timer manager",
               error);

    /* Register the ProcFS driver */
    retCode = _CreateProcFSEntry();
    RTC_ASSERT(retCode == NO_ERROR,
               "Failed to create ProcFS entry",
               retCode);

    _Enable(spDrvCtrl);
  }
  else
  {
    retCode = ERR_UNAUTHORIZED_ACTION;
  }

  return retCode;
}

static bool _DummyHandler(void)
{
  PANIC(ERR_UNAUTHORIZED_ACTION, MODULE_NAME, "RTC Dummy handler", true, false);

  return false;
}

static void _Enable(void* pDrvCtrl)
{
  S_RTCControler* pRtcCtrl;

  pRtcCtrl = GET_CONTROLER(pDrvCtrl);

  SpinlockAcquire(&pRtcCtrl->lock);

  if (pRtcCtrl->disabledNesting > 0)
  {
    --pRtcCtrl->disabledNesting;
  }

  if (pRtcCtrl->disabledNesting == 0 && pRtcCtrl->selectedFrequency != 0)
  {
    IOAPICSetupIRQ(RTC_IRQ_NUMBER,
                   spDrvCtrl->irqNumber,
                   0,
                   IOAPIC_TRIGGER_MODE_EDGE,
                   IOAPIC_POLARITY_HIGH);
  }

  SpinlockRelease(&pRtcCtrl->lock);
}

static void _Disable(void* pDrvCtrl)
{
  S_RTCControler* pRtcCtrl;

  pRtcCtrl = GET_CONTROLER(pDrvCtrl);

  SpinlockAcquire(&pRtcCtrl->lock);

  if (pRtcCtrl->disabledNesting < UINT32_MAX)
  {
    ++pRtcCtrl->disabledNesting;
  }

  IOAPICDisableIRQ(RTC_IRQ_NUMBER);

  SpinlockRelease(&pRtcCtrl->lock);
}

static void _SetFrequency(void* pDrvCtrl, const uint64_t kFrequency)
{
  uint32_t        prevRate;
  uint32_t        rate;
  S_RTCControler* pRtcCtrl;

  pRtcCtrl = GET_CONTROLER(pDrvCtrl);

  if (kFrequency >= RTC_FREQUENCY_LOW && kFrequency <= RTC_FREQUENCY_HIGH)
  {
    /* Choose the closest rate to the frequency */
    if (kFrequency < 4)
    {
      rate = 15;
    }
    else if (kFrequency < 8)
    {
      rate = 14;
    }
    else if (kFrequency < 16)
    {
      rate = 13;
    }
    else if (kFrequency < 32)
    {
      rate = 12;
    }
    else if (kFrequency < 64)
    {
      rate = 11;
    }
    else if (kFrequency < 128)
    {
      rate = 10;
    }
    else if (kFrequency < 256)
    {
      rate = 9;
    }
    else if (kFrequency < 512)
    {
      rate = 8;
    }
    else if (kFrequency < 1024)
    {
      rate = 7;
    }
    else if (kFrequency < 2048)
    {
      rate = 6;
    }
    else if (kFrequency < 4096)
    {
      rate = 5;
    }
    else if (kFrequency < 8192)
    {
      rate = 4;
    }
    else
    {
      rate = 3;
    }

    SpinlockAcquire(&pRtcCtrl->lock);

    /* Set clock frequency */
    /* Init CMOS IRQ8 rate */
    CPUPortWriteByte((CMOS_NMI_DISABLE_BIT << 7) | CMOS_REG_A,
                     RTC_CPU_COMM_PORT);
    prevRate = CPUPortReadByte(RTC_CPU_DATA_PORT);
    CPUPortWriteByte((CMOS_NMI_DISABLE_BIT << 7) | CMOS_REG_A,
                     RTC_CPU_COMM_PORT);
    CPUPortWriteByte((prevRate & 0xF0) | rate, RTC_CPU_DATA_PORT);

    pRtcCtrl->selectedFrequency = (RTC_QUART_FREQUENCY >> (rate - 1));

    SpinlockRelease(&pRtcCtrl->lock);
  }
}

static uint64_t _GetFrequency(void* pDrvCtrl)
{
  S_RTCControler* pRtcCtrl;

  pRtcCtrl = GET_CONTROLER(pDrvCtrl);

  return pRtcCtrl->selectedFrequency;
}

static E_Return _SetHandler(void* pDrvCtrl, T_InterruptHandler handler)
{
  E_Return        err;
  S_RTCControler* pRtcCtrl;


  if (handler != NULL)
  {
    pRtcCtrl = GET_CONTROLER(pDrvCtrl);

    SpinlockAcquire(&pRtcCtrl->lock);
    err = InterruptRegister(pRtcCtrl->irqNumber, handler);
    SpinlockRelease(&pRtcCtrl->lock);
  }
  else
  {
    err = ERR_INVALID_PARAMETER;
  }

  return err;
}

static E_Return _RemoveHandler(void* pDrvCtrl)
{
  return _SetHandler(pDrvCtrl, _DummyHandler);
}

static S_DayTime _GetDaytime(void* pDrvCtrl)
{
  S_DayTime retTime;
  S_Date    retDate;

  _UpdateTime(pDrvCtrl, &retDate, &retTime);
  return retTime;
}

static S_Date _GetDate(void* pDrvCtrl)
{
  S_DayTime retTime;
  S_Date    retDate;

  _UpdateTime(pDrvCtrl, &retDate, &retTime);
  return retDate;
}

static void _UpdateTime(void* pDrvCtrl, S_Date* pDate, S_DayTime* pTime)
{
  uint8_t         century;
  uint8_t         regB;
  S_RTCControler* pRtcCtrl;

  pRtcCtrl = GET_CONTROLER(pDrvCtrl);

  SpinlockAcquire(&pRtcCtrl->lock);

  /* Select CMOS seconds register and read */
  CPUPortWriteByte(CMOS_SECONDS_REGISTER, RTC_CPU_COMM_PORT);
  pTime->seconds = CPUPortReadByte(RTC_CPU_DATA_PORT);

  /* Select CMOS minutes register and read */
  CPUPortWriteByte(CMOS_MINUTES_REGISTER, RTC_CPU_COMM_PORT);
  pTime->minutes = CPUPortReadByte(RTC_CPU_DATA_PORT);

  /* Select CMOS hours register and read */
  CPUPortWriteByte(CMOS_HOURS_REGISTER, RTC_CPU_COMM_PORT);
  pTime->hours = CPUPortReadByte(RTC_CPU_DATA_PORT);

  /* Select CMOS day register and read */
  CPUPortWriteByte(CMOS_DAY_REGISTER, RTC_CPU_COMM_PORT);
  pDate->day = CPUPortReadByte(RTC_CPU_DATA_PORT);

  /* Select CMOS month register and read */
  CPUPortWriteByte(CMOS_MONTH_REGISTER, RTC_CPU_COMM_PORT);
  pDate->month = CPUPortReadByte(RTC_CPU_DATA_PORT);

  /* Select CMOS years register and read */
  CPUPortWriteByte(CMOS_YEAR_REGISTER, RTC_CPU_COMM_PORT);
  pDate->year = CPUPortReadByte(RTC_CPU_DATA_PORT);

  /* Select CMOS century register and read */
  CPUPortWriteByte(CMOS_CENTURY_REGISTER, RTC_CPU_COMM_PORT);
  century = CPUPortReadByte(RTC_CPU_DATA_PORT);

  /* Convert BCD to binary if necessary */
  CPUPortWriteByte(CMOS_REG_B, RTC_CPU_COMM_PORT);
  regB = CPUPortReadByte(RTC_CPU_DATA_PORT);

  if ((regB & 0x04) == 0)
  {
    pTime->seconds = (pTime->seconds & 0x0F) + ((pTime->seconds / 16) * 10);
    pTime->minutes = (pTime->minutes & 0x0F) + ((pTime->minutes / 16) * 10);
    pTime-> hours = ((pTime->hours & 0x0F) +
                      (((pTime->hours & 0x70) / 16) * 10)) |
                      (pTime->hours & 0x80);
    pDate->day = (pDate->day & 0x0F) + ((pDate->day / 16) * 10);
    pDate->month = (pDate->month & 0x0F) + ((pDate->month / 16) * 10);
    pDate->year = (pDate->year & 0x0F) + ((pDate->year / 16) * 10);

    if (CMOS_CENTURY_REGISTER != 0)
    {
      century = (century & 0x0F) + ((century / 16) * 10);
    }
  }

  /*  Convert to 24H */
  if ((regB & 0x02) == 0 && (pTime->hours & 0x80) >= 1)
  {
    pTime->hours = ((pTime->hours & 0x7F) + 12) % 24;
  }

  /* Get year */
  if (CMOS_CENTURY_REGISTER != 0)
  {
    pDate->year += century * 100;
  }
  else
  {
    pDate->year = pDate->year + 2000;
  }

  /* Compute week day and day time */
  pDate->weekday = ((pDate->day + pDate->month + pDate->year + pDate->year / 4)
                     + 1) % 7 + 1;

  SpinlockRelease(&pRtcCtrl->lock);
}

static void _AcknowledgeInt(void* pDrvCtrl)
{
  S_RTCControler* pRtcCtrl;

  pRtcCtrl = GET_CONTROLER(pDrvCtrl);

  /* Clear C Register */
  CPUPortWriteByte(CMOS_REG_C, RTC_CPU_COMM_PORT);
  CPUPortReadByte(RTC_CPU_DATA_PORT);

  /* Set EOI */
  InterruptSetEOI(pRtcCtrl->irqNumber);
}

static void* _ProcFSOpen(void*       pDriverData,
                         const char* kpPath,
                         int32_t     flags,
                         int32_t     mode)
{
  size_t* pHandle;

  (void)pDriverData;
  (void)mode;
  if (flags == O_RDONLY && *kpPath == 0)
  {
    pHandle = KMallocUser(sizeof(size_t), NULL);
    if (pHandle != NULL)
    {
      *pHandle = 0;
    }
    else
    {
      pHandle = (void*)-1;
    }
  }
  else
  {
    pHandle = (void*)-1;
  }

  return pHandle;
}

static int32_t _ProcFSClose(void* pDriverData, void* pFileHandle)
{
  int32_t retCode;

  (void)pDriverData;

  if (pFileHandle != (void*)-1 && pFileHandle != NULL)
  {
    KFreeUser(pFileHandle, NULL);
    retCode = 0;
  }
  else
  {
    retCode = -1;
  }

  return retCode;
}

static ssize_t _ProcFSRead(void*  pDriverData,
                           void*  pFileHandle,
                           void*  pBuffer,
                           size_t count)
{
  int32_t   retCode;
  char      tempBuffer[128];
  size_t    size;
  ssize_t   written;
  size_t    offset;
  size_t    toWrite;
  size_t*   pBufferOffset;
  size_t    bufferOffset;
  S_DayTime retTime;
  S_Date    retDate;

  (void)pDriverData;

  if (pFileHandle != (void*)-1 && pFileHandle != NULL)
  {
    retCode = 0;
    pBufferOffset = (size_t*)pFileHandle;
    bufferOffset = *pBufferOffset;

    /* Generate CPU info */
    size = snprintf(tempBuffer,
                    sizeof(tempBuffer),
                    "Ports 0x%x (Comm) 0x%x (Data)\n"
                    "IRQ Identifier: %d\n"
                    "Qartz Frequency: %llu Hz\n",
                    RTC_CPU_COMM_PORT,
                    RTC_CPU_DATA_PORT,
                    spDrvCtrl->irqNumber,
                    RTC_QUART_FREQUENCY
                  );
    written = size - bufferOffset;

    if (written > 0)
    {
      offset  = size - written;
      toWrite = MIN(written, (ssize_t)count);
      memcpy(pBuffer, tempBuffer + offset, toWrite);
      *pBufferOffset += toWrite;
      pBuffer        += toWrite;
      retCode        += toWrite;
      count          -= toWrite;
      bufferOffset   = 0;
    }
    else
    {
      bufferOffset -= size;
    }

    if (count > 0)
    {
      size = snprintf(tempBuffer,
                      sizeof(tempBuffer),
                      "Interrupt Frequency: %d Hz\n"
                      "Frequency Range: [%d; %d]\n"
                      "Nesting: %d\n",
                      spDrvCtrl->selectedFrequency,
                      RTC_FREQUENCY_LOW,
                      RTC_FREQUENCY_HIGH,
                      spDrvCtrl->disabledNesting);
      written = size - bufferOffset;

      if (written > 0)
      {
        offset  = size - written;
        toWrite = MIN(written, (ssize_t)count);
        memcpy(pBuffer, tempBuffer + offset, toWrite);
        *pBufferOffset += toWrite;
        pBuffer        += toWrite;
        retCode        += toWrite;
        count          -= toWrite;
        bufferOffset   = 0;
      }
      else
      {
        bufferOffset -= size;
      }

      if (count > 0)
      {

        _UpdateTime(spDrvCtrl, &retDate, &retTime);

        size = snprintf(tempBuffer,
                        sizeof(tempBuffer),
                        "%d:%d:%d %d/%d/%d",
                        retTime.hours,
                        retTime.minutes,
                        retTime.seconds,
                        retDate.day,
                        retDate.month,
                        retDate.year);
        written = size - bufferOffset;

        if (written > 0)
        {
          offset  = size - written;
          toWrite = MIN(written, (ssize_t)count);
          memcpy(pBuffer, tempBuffer + offset, toWrite);
          *pBufferOffset += toWrite;
          pBuffer        += toWrite;
          retCode        += toWrite;
          count          -= toWrite;
          bufferOffset   = 0;
        }
        else
        {
          bufferOffset -= size;
        }
      }
    }
  }
  else
  {
    retCode = -1;
  }

  return retCode;
}

static E_Return _CreateProcFSEntry(void)
{
  E_Return retCode;

  /* Create the entry */
  retCode = ProcFSCreateEntry(PROCFS_DIR_PATH,
                              0,
                              NULL,
                              &sProcFSOps,
                              NULL,
                              &spProcFSEntry);
  return retCode;
}


/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_LAYER(sX86RTCDriver);

/************************************ EOF *************************************/