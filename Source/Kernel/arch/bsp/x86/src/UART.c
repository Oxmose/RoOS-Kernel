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
#include <stdint.h>
#include <string.h>
#include <X64Cpu.h>
#include <Console.h>
#include <Critical.h>
#include <DeviceTree.h>
#include <KernelHeap.h>
#include <KernelError.h>
#include <DebugOutput.h>
#include <DriverManager.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* None TODO */

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
/** @brief FDT property for console output */
#define UART_FDT_OUTPUT_PROP "console-output"

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

#if OUTPUT_DEBUG_ENABLE
/** @brief Defines the port that is used to print debug data. */
#define SERIAL_DEBUG_PORT 0x3F8
#endif

/** @brief Cast a pointer to a UART driver controler */
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

/** @brief x86 UART driver controler. */
typedef struct
{
  /** @brief CPU command port. */
  uint16_t cpuCommPort;
  /** @brief Baudrate */
  E_UARTBaudrate baudrate;
  /** @brief Driver's lock */
  S_KernelSpinlock lock;
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
    PANIC(ERROR, MODULE_NAME, MSG);                       \
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
 * @brief Detaches the UART driver to the system.
 *
 * @details Detaches the UART driver to the system. This function will use the
 * FDT to de-initialize the UART hardware and retreive the UART parameters.
 *
 * @param[in] kpFdtNode The FDT node with the compatible declared by the driver.
 *
 * @return The success state or the error code.
 */
static E_Return _UartDettach(const S_FDTNode* kpFdtNode);

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
 * controler.
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
 * @brief Outputs a character to the UART console.
 *
 * @details Outputs a character to the UART console. This function is used by
 * the console driver to output data to the UART.
 *
 * @param[in] kCharacter The character to output to the UART console.
 */
static void _UartPutChar(const char kCharacter);

/**
 * @brief Outputs a string to the UART console.
 *
 * @details Outputs a string to the UART console. This function is used by the
 * console driver to output data to the UART.
 */
static void _UartPutString(const char* kpString);

/**
 * @brief Flushes the UART console.
 *
 * @details Flushes the UART console. This function is used by the console
 * driver to flush the UART console.
 */
static void _UartFlush(void);

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
  .pDriverDettach = _UartDettach
};

/** @brief Stores the console driver controler. */
static S_UARTControler* spConsoleDriverControler = NULL;

/** @brief Stores the console driver. */
static S_ConsoleDriver sConsoleDriver =
{
  .pPutChar   = _UartPutChar,
  .pPutString = _UartPutString,
  .pFlush     = _UartFlush
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static E_Return _UartAttach(const S_FDTNode* kpFdtNode)
{
    const uint32_t*  kpUintProp;
    size_t           propLen;
    E_Return         retCode;
    S_UARTControler* pDrvCtrl;
    E_UARTBaudrate   baudRate;

    /* Init structures */
    pDrvCtrl = KMalloc(sizeof(S_UARTControler),
                       ALIGN_ADDRESS,
                       KMALLOC_FREE_POOL);

    KERNEL_SPINLOCK_INIT(pDrvCtrl->lock);

    retCode = ERR_INVALID_PARAMETER;

    /* Get the UART CPU communication ports */
    kpUintProp = FDTGetProp(kpFdtNode, UART_FDT_COMM_PROP, &propLen);
    if (kpUintProp != NULL && propLen == sizeof(uint32_t))
    {
      pDrvCtrl->cpuCommPort = (uint16_t)FDTTOCPU32(*kpUintProp);

      /* Get the UART CPU baudrate */
      kpUintProp = FDTGetProp(kpFdtNode, UART_FDT_RATE_PROP, &propLen);
      if (kpUintProp != NULL && propLen == sizeof(uint32_t))
      {
        pDrvCtrl->baudrate = FDTTOCPU32(*kpUintProp);
        retCode = NO_ERROR;
      }
    }
    else
    {
      retCode = ERR_INVALID_PARAMETER;
    }

    if (retCode == NO_ERROR)
    {
      baudRate = _UartGetCanonicalRate(pDrvCtrl->baudrate);

      /* Init line */
      CPUPortWriteByte(0x00, SERIAL_DATA_PORT_2(pDrvCtrl->cpuCommPort));
      _UartSetBaudrate(baudRate, pDrvCtrl->cpuCommPort);
      _UartSetLine(SERIAL_DATA_LENGTH_8 | SERIAL_STOP_BIT_1,
                   pDrvCtrl->cpuCommPort);
      _UartSetBuffer(0xC0                   |
                     SERIAL_ENABLE_FIFO     |
                     SERIAL_CLEAR_RECV_FIFO |
                     SERIAL_CLEAR_SEND_FIFO |
                     SERIAL_FIFO_DEPTH_14,
                     pDrvCtrl->cpuCommPort);

      kpUintProp = FDTGetProp(kpFdtNode, UART_FDT_OUTPUT_PROP, &propLen);
      if (kpUintProp != NULL)
      {
        spConsoleDriverControler = pDrvCtrl;
        ConsoleSetDriver(&sConsoleDriver);
      }

      /* Register driver */
      retCode = DriverManagerSetDeviceData(kpFdtNode, pDrvCtrl);
      if (retCode != NO_ERROR)
      {
        KFree(pDrvCtrl);
      }
    }
    else
    {
      KFree(pDrvCtrl);
    }

    return retCode;
}

static E_Return _UartDettach(const S_FDTNode* kpFdtNode)
{
  S_UARTControler* pDrvCtrl;

  pDrvCtrl = GET_CONTROLER(kpFdtNode->pDevData);

  KFree(pDrvCtrl);

  return NO_ERROR;
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
  switch (kBaudrate)
  {
    case 50:
      return 2304;
      break;
    case 75:
      return 1536;
      break;
    case 150:
      return 768;
      break;
    case 300:
      return 384;
      break;
    case 600:
      return 192;
      break;
    case 1200:
      return 96;
      break;
    case 1800:
      return 64;
      break;
    case 2400:
      return 48;
      break;
    case 4800:
      return 24;
      break;
    case 7200:
      return 16;
      break;
    case 9600:
      return 12;
      break;
    case 14400:
      return 8;
      break;
    case 19200:
      return 6;
      break;
    case 38400:
      return 3;
      break;
    case 57600:
      return 2;
      break;
    case 115200:
    default:
       return 1;
  }
}

void _UartPutString(const char* kpString)
{
  while (*kpString != '\0')
  {
    _UartWrite(SERIAL_DEBUG_PORT, *kpString);
    ++kpString;
  }
}

void _UartPutChar(const char kCharacter)
{
  _UartWrite(SERIAL_DEBUG_PORT, kCharacter);
}

void _UartFlush(void)
{
  // Nothing to do, only here for compatibility with other architectures.
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