/*******************************************************************************
 * @file UART.c
 *
 * @see UART.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 24/05/2024
 *
 * @version 2.1
 *
 * @brief UART communication driver.
 *
 * @details UART communication driver. Initializes the uart ports as in and
 * output. The uart can be used to output data or communicate with other
 * prepherals that support this communication method
 *
 * @warning Only one UART can be used as input at the moment.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <Panic.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <X64Cpu.h>
#include <Critical.h>
#include <VirtualFS.h>
#include <DeviceTree.h>
#include <KernelHeap.h>
#include <KernelError.h>
#include <DebugOutput.h>
#include <DriverManager.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* No unit test: this module is tested in real-world conditions. */

/* Header file */
#include <UART.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "X86 UART"

/** @brief FDT property for baudrate */
#define UART_FDT_RATE_PROP   "baudrate"
/** @brief FDT property for comm ports */
#define UART_FDT_COMM_PROP   "comm"
/** @brief FDT property for interrupt  */
#define UART_FDT_INT_PROP "interrupts"
/** @brief FDT property for device path */
#define UART_FDT_DEVICE_PROP "device"

/** @brief Serial data length flag: 5 bits. */
#define SERIAL_DATA_LENGTH_5 0x00
/** @brief Serial data length flag: 6 bits. */
#define SERIAL_DATA_LENGTH_6 0x01
/** @brief Serial data length flag: 7 bits. */
#define SERIAL_DATA_LENGTH_7 0x02
/** @brief Serial data length flag: 8 bits. */
#define SERIAL_DATA_LENGTH_8 0x03

/** @brief Serial parity bit flag: 1 bit. */
#define SERIAL_STOP_BIT_1   0x00
/** @brief Serial parity bit flag: 2 bits. */
#define SERIAL_STOP_BIT_2   0x04

/** @brief Serial parity bit settings flag: none. */
#define SERIAL_PARITY_NONE  0x00
/** @brief Serial parity bit settings flag: odd. */
#define SERIAL_PARITY_ODD   0x01
/** @brief Serial parity bit settings flag: even. */
#define SERIAL_PARITY_EVEN  0x03
/** @brief Serial parity bit settings flag: mark. */
#define SERIAL_PARITY_MARK  0x05
/** @brief Serial parity bit settings flag: space. */
#define SERIAL_PARITY_SPACE 0x07

/** @brief Serial break control flag enabled. */
#define SERIAL_BREAK_CTRL_ENABLED  0x40
/** @brief Serial break control flag disabled. */
#define SERIAL_BREAK_CTRL_DISABLED 0x00

/** @brief Serial dlab flag enabled. */
#define SERIAL_DLAB_ENABLED  0x80
/** @brief Serial dlab flag disabled. */
#define SERIAL_DLAB_DISABLED 0x00

/** @brief Serial fifo enable flag. */
#define SERIAL_ENABLE_FIFO       0x01
/** @brief Serial fifo clear receive flag. */
#define SERIAL_CLEAR_RECV_FIFO   0x02
/** @brief Serial fifo clear send flag. */
#define SERIAL_CLEAR_SEND_FIFO   0x04
/** @brief Serial DMA accessed fifo flag. */
#define SERIAL_DMA_ACCESSED_FIFO 0x08

/** @brief Serial fifo depth flag: 14 bits. */
#define SERIAL_FIFO_DEPTH_14     0x00
/** @brief Serial fifo depth flag: 64 bits. */
#define SERIAL_FIFO_DEPTH_64     0x10

/**
 * @brief Computes the data port for the serial port which base port ID is
 * given as parameter.
 *
 * @param[in] port The base port ID of the serial port.
 */
#define SERIAL_DATA_PORT(port) (port)
/**
 * @brief Computes the aux data port for the serial port which base port ID is
 * given as parameter.
 *
 * @param[in] port The base port ID of the serial port.
 */
#define SERIAL_DATA_PORT_2(port) (port + 1)
/**
 * @brief Computes the fifo command port for the serial port which base port ID
 * is given as parameter.
 *
 * @param[in] port The base port ID of the serial port.
 */
#define SERIAL_FIFO_COMMAND_PORT(port) (port + 2)
/**
 * @brief Computes the line command port for the serial port which base port ID
 * is given as parameter.
 *
 * @param[in] port The base port ID of the serial port.
 */
#define SERIAL_LINE_COMMAND_PORT(port) (port + 3)
/**
 * @brief Computes the modem command port for the serial port which base port ID
 * is given as parameter.
 *
 * @param[in] port The base port ID of the serial port.
 */
#define SERIAL_MODEM_COMMAND_PORT(port) (port + 4)
/**
 * @brief Computes the line status port for the serial port which base port ID
 * is given as parameter.
 *
 * @param[in] port The base port ID of the serial port.
 */
#define SERIAL_LINE_STATUS_PORT(port) (port + 5)

/** @brief Defines the port that is used to print debug data. */
#define SERIAL_DEBUG_PORT 0x3F8

/** @brief Cast a pointer to a UART driver controller */
#define GET_CONTROLER(PTR) ((S_UARTControler*)PTR)

/** @brief Defines the maximal size of the UART input buffer */
#define UART_INPUT_BUFFER_SIZE 128

/** @brief UART Interrupt status data available mask */
#define UART_INT_STATUS_DATA_AVAILABLE 0x1

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Serial baudrate enumation. Enumerates all the supported baudrates.
 * The value of the enumeration is the transmission rate divider.
 */
typedef enum
{
  /** @brief Baudrate 50Bd. */
  BAURDATE_50     = 2304,
  /** @brief Baudrate 75Bd. */
  BAUDRATE_75     = 1536,
  /** @brief Baudrate 150Bd. */
  BAUDRATE_150    = 768,
  /** @brief Baudrate 300Bd. */
  BAUDRATE_300    = 384,
  /** @brief Baudrate 600Bd. */
  BAUDRATE_600    = 192,
  /** @brief Baudrate 1200Bd. */
  BAUDRATE_1200   = 96,
  /** @brief Baudrate 1800Bd. */
  BAUDRATE_1800   = 64,
  /** @brief Baudrate 2400Bd. */
  BAUDRATE_2400   = 48,
  /** @brief Baudrate 4800Bd. */
  BAUDRATE_4800   = 24,
  /** @brief Baudrate 7200Bd. */
  BAUDRATE_7200   = 16,
  /** @brief Baudrate 9600Bd. */
  BAUDRATE_9600   = 12,
  /** @brief Baudrate 14400Bd. */
  BAUDRATE_14400  = 8,
  /** @brief Baudrate 19200Bd. */
  BAUDRATE_19200  = 6,
  /** @brief Baudrate 38400Bd. */
  BAUDRATE_38400  = 3,
  /** @brief Baudrate 57600Bd. */
  BAUDRATE_57600  = 2,
  /** @brief Baudrate 115200Bd. */
  BAUDRATE_115200 = 1,
} E_UARTBaudrate;

/** @brief x86 UART driver controller. */
typedef struct
{
  /** @brief CPU command port. */
  uint16_t cpuCommPort;
  /** @brief Baudrate */
  E_UARTBaudrate baudrate;
  /** @brief Driver's lock */
  S_KernelSpinlock lock;
  /** @brief Device path */
  const char* kpDevicePath;
  /** @brief VFS driver */
  T_VFSDriver* pVFSDriver;
} S_UARTControler;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the UART to ensure correctness of execution.
 *
 * @details Assert macro used by the UART to ensure correctness of execution.
 * Due to the critical nature of the UART, any error generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define UART_ASSERT(COND, MSG, ERROR) {                   \
  if ((COND) == false)                                    \
  {                                                       \
    PANIC(ERROR, MODULE_NAME, MSG, false, false);         \
  }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Attaches the UART driver to the system.
 *
 * @details Attaches the UART driver to the system. This function will use the
 * FDT to initialize the UART hardware and retreive the UART parameters.
 *
 * @param[in] kpFdtNode The FDT node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static E_Return _UartAttach(const S_FDTNode* kpFdtNode);

/**
 * @brief Sets line parameters for the desired port.
 *
 * @details Sets line parameters for the desired port.
 *
 * @param[in] kAttr The settings for the port's line.
 * @param[in] kCom The port to set.
 */
static inline void _UartSetLine(const uint8_t kAttr, const uint16_t kCom);

/**
 * @brief Sets buffer parameters for the desired port.
 *
 * @details Sets buffer parameters for the desired port.
 *
 * @param[in] kAttr The settings for the port's line.
 * @param[in] kCom The port to set.
 *
 * @return The success state or the error code.
 */
static inline void _UartSetBuffer(const uint8_t kAttr, const uint16_t kCom);

/**
 * @brief Sets the port's baudrate.
 *
 * @details Sets the port's baudrate.
 *
 * @param[in] kRate The desired baudrate for the port.
 * @param[in] kCom The port to set.
 */
static inline void _UartSetBaudrate(const E_UARTBaudrate kRate,
                                    const uint16_t       kCom);

/**
 * @brief Writes the data given as patameter on the desired port.
 *
 * @details The function will output the data given as parameter on the selected
 * port. This call is blocking until the data has been sent to the uart port
 * controller.
 *
 * @param[in] kPort The desired port to write the data to.
 * @param[in] kData The byte to write to the uart port.
 */
static inline void _UartWrite(const uint16_t kPort, const uint8_t kData);

/**
 * @brief Returns the canonical baudrate for a given BPS baudrate
 *
 * @details Returns the canonical baudrate for a given BPS baudrate based on the
 * driver's specifications.
 *
 * @param[in] kBaudrate The BPS baudrate to convert.
 *
 * @return The canonical baudrate for a given BPS baudrate is returned.
*/
static E_UARTBaudrate _UartGetCanonicalRate(const uint32_t kBaudrate);

/**
 * @brief UART VFS open hook.
 *
 * @details UART VFS open hook. This function returns a handle to control the
 * UART driver through VFS.
 *
 * @param[in, out] pDrvCtrl The UART driver that was registered in the VFS.
 * @param[in] kpPath The path in the UART driver mount point.
 * @param[in] flags The open flags, must be O_RDWR.
 * @param[in] mode Unused.
 *
 * @return The function returns an internal handle used by the driver during
 * file operations.
 */
static void* _VFSOpen(void*       pDrvCtrl,
                      const char* kpPath,
                      int         flags,
                      int         mode);

/**
 * @brief UART VFS close hook.
 *
 * @details UART VFS close hook. This function closes a handle that was created
 * when calling the open function.
 *
 * @param[in, out] pDrvCtrl The UART driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static int32_t _VFSClose(void* pDrvCtrl, void* pHandle);

/**
 * @brief UART VFS Read hook.
 *
 * @details UART VFS read hook. This function reads data from the UART
 * framebuffer.
 *
 * @param[in, out] pDrvCtrl The UART driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] pBuffer The buffer that contains the string to write.
 * @param[in] count The number of bytes of the string to write.
 *
 * @return The function returns the number of bytes written or -1 on error;
 */
static ssize_t _VFSRead(void*  pDrvCtrl,
                        void*  pHandle,
                        void*  pBuffer,
                        size_t count);

/**
 * @brief UART VFS write hook.
 *
 * @details UART VFS write hook. This function writes a string to the UART
 * framebuffer.
 *
 * @param[in, out] pDrvCtrl The UART driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] kpBuffer The buffer that contains the string to write.
 * @param[in] count The number of bytes of the string to write.
 *
 * @return The function returns the number of bytes written or -1 on error;
 */
static ssize_t _VFSWrite(void*       pDrvCtrl,
                         void*       pHandle,
                         const void* kpBuffer,
                         size_t      count);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief UART driver instance. */
static S_Driver sX86UARTDriver =
{
  .pName          = "X86 UART Driver",
  .pDescription   = "X86 UART Driver for roOs",
  .pCompatible    = "x86,x86-generic-serial",
  .pVersion       = "2.1",
  .pDriverAttach  = _UartAttach,
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static E_Return _UartAttach(const S_FDTNode* kpFdtNode)
{
  const uint32_t*  kpUintProp;
  const char*      kpStrProp;
  size_t           propLen;
  E_Return         retCode;
  S_UARTControler* pDrvCtrl;
  E_UARTBaudrate   baudRate;

  /* Init structures */
  pDrvCtrl = KMalloc(sizeof(S_UARTControler), KMALLOC_NO_FREE_POOL);

  KERNEL_SPINLOCK_INIT(pDrvCtrl->lock);

  retCode = ERR_INVALID_PARAMETER;

  /* Get the UART CPU communication ports */
  kpUintProp = FDTGetProp(kpFdtNode, UART_FDT_COMM_PROP, &propLen);
  UART_ASSERT(kpUintProp != NULL && propLen == sizeof(uint32_t),
              "Failed to get the UART comm port",
              retCode);

  pDrvCtrl->cpuCommPort = (uint16_t)FDTTOCPU32(*kpUintProp);

  /* Get the UART CPU baudrate */
  kpUintProp = FDTGetProp(kpFdtNode, UART_FDT_RATE_PROP, &propLen);
  UART_ASSERT(kpUintProp != NULL && propLen == sizeof(uint32_t),
              "Failed to get the UART baudrate",
              retCode);

  pDrvCtrl->baudrate = FDTTOCPU32(*kpUintProp);

  /* Get the device path */
  kpStrProp = FDTGetProp(kpFdtNode, UART_FDT_DEVICE_PROP, &propLen);
  UART_ASSERT(kpStrProp != NULL && propLen > 0,
              "Failed to get the UART device path",
              retCode);

  pDrvCtrl->kpDevicePath = kpStrProp;

  baudRate = _UartGetCanonicalRate(pDrvCtrl->baudrate);

  /* Init line */
  CPUPortWriteByte(0x00, SERIAL_DATA_PORT_2(pDrvCtrl->cpuCommPort));
  _UartSetBaudrate(baudRate, pDrvCtrl->cpuCommPort);
  _UartSetLine(SERIAL_DATA_LENGTH_8 | SERIAL_STOP_BIT_1, pDrvCtrl->cpuCommPort);
  _UartSetBuffer(0xC0                   |
                 SERIAL_ENABLE_FIFO     |
                 SERIAL_CLEAR_RECV_FIFO |
                 SERIAL_CLEAR_SEND_FIFO |
                 SERIAL_FIFO_DEPTH_14,
                 pDrvCtrl->cpuCommPort);

  /* Register VFS driver */
  pDrvCtrl->pVFSDriver = RegisterVFSDriver(pDrvCtrl->kpDevicePath,
                                           NULL,
                                           _VFSOpen,
                                           _VFSClose,
                                           _VFSRead,
                                           _VFSWrite,
                                           NULL,
                                           NULL);
  UART_ASSERT(pDrvCtrl->pVFSDriver != VFS_DRIVER_INVALID,
              "Failed to register the UART driver in the VFS",
              retCode);

  /* Register driver */
  retCode = DriverManagerSetDeviceData(kpFdtNode, pDrvCtrl);
  UART_ASSERT(retCode == NO_ERROR,
              "Failed to register the UART driver in the driver manager",
              retCode);

  return retCode;
}

static inline void _UartSetLine(const uint8_t kAttr, const uint16_t kCom)
{
  CPUPortWriteByte(kAttr, SERIAL_LINE_COMMAND_PORT(kCom));
}

static inline void _UartSetBuffer(const uint8_t kAttr, const uint16_t kCom)
{
  CPUPortWriteByte(kAttr, SERIAL_FIFO_COMMAND_PORT(kCom));
}

static inline void _UartSetBaudrate(const E_UARTBaudrate kRate,
                                    const uint16_t       kCom)
{
  CPUPortWriteByte(SERIAL_DLAB_ENABLED, SERIAL_LINE_COMMAND_PORT(kCom));
  CPUPortWriteByte((kRate >> 8) & 0x00FF, SERIAL_DATA_PORT(kCom));
  CPUPortWriteByte(kRate & 0x00FF, SERIAL_DATA_PORT_2(kCom));
}

static inline void _UartWrite(const uint16_t kPort, const uint8_t kData)
{
  /* Wait for empty transmit */
  while ((CPUPortReadByte(SERIAL_LINE_STATUS_PORT(kPort)) & 0x20) == 0){}
  if (kData == '\n')
  {
    CPUPortWriteByte('\r', kPort);
    while ((CPUPortReadByte(SERIAL_LINE_STATUS_PORT(kPort)) & 0x20) == 0){}
    CPUPortWriteByte('\n', kPort);
  }
  else
  {
    CPUPortWriteByte(kData, kPort);
  }
}

static E_UARTBaudrate _UartGetCanonicalRate(const uint32_t kBaudrate)
{
  return 115200 / kBaudrate;
}

static void* _VFSOpen(void*       pDrvCtrl,
                      const char* kpPath,
                      int         flags,
                      int         mode)
{
  (void)pDrvCtrl;
  (void)mode;
  (void)flags;

  /* The path must be empty */
  if (*kpPath != 0)
  {
    return (void*)-1;
  }

  /* We don't need a handle, return NULL */
  return NULL;
}

static int32_t _VFSClose(void* pDrvCtrl, void* pHandle)
{
  (void)pDrvCtrl;

  if (pHandle == (void*)-1)
  {
    return -1;
  }

  /* Nothing to do */
  return 0;
}

static ssize_t _VFSWrite(void*       pDrvCtrl,
                         void*       pHandle,
                         const void* kpBuffer,
                         size_t      count)
{
  const char* pCursor;
  size_t      coutSave;

  if (pHandle == (void*)-1)
  {
    return -1;
  }

  pCursor = (char*)kpBuffer;

  /* Output each character of the string */
  coutSave = count;
  while (pCursor != NULL && *pCursor != 0 && count > 0)
  {
    _UartWrite(GET_CONTROLER(pDrvCtrl)->cpuCommPort, *pCursor);
    ++pCursor;
    --count;
  }

  return coutSave - count;
}

static ssize_t _VFSRead(void*  pDrvCtrl,
                        void*  pHandle,
                        void*  pBuffer,
                        size_t count)
{
  (void)pDrvCtrl;
  (void)pBuffer;
  (void)count;

  if (pHandle == (void*)-1)
  {
    return -1;
  }

  /* Not implemented */

  return -1;
}

#if OUTPUT_DEBUG_ENABLE

void DebugOutputInit(void)
{
  /* Init line */
  _UartSetBaudrate(DEBUG_LOG_UART_RATE, SERIAL_DEBUG_PORT);
  _UartSetLine(SERIAL_DATA_LENGTH_8 | SERIAL_STOP_BIT_1, SERIAL_DEBUG_PORT);
  _UartSetBuffer(0xC0 |
                 SERIAL_ENABLE_FIFO |
                 SERIAL_CLEAR_RECV_FIFO |
                 SERIAL_CLEAR_SEND_FIFO |
                 SERIAL_FIFO_DEPTH_14,
                 SERIAL_DEBUG_PORT);
}

void DebugOutputPutString(const char* kpString)
{
  while (*kpString)
  {
    _UartWrite(SERIAL_DEBUG_PORT, *kpString);
    ++kpString;
  }
}

void DebugOutputPutChar(const char kCharacter)
{
  _UartWrite(SERIAL_DEBUG_PORT, kCharacter);
}
#endif

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_FDT(sX86UARTDriver);

/************************************ EOF *************************************/