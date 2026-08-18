/*******************************************************************************
 * @file Keyboard.c
 *
 * @see Keyboard.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 03/07/2024
 *
 * @version 2.0
 *
 * @brief Keyboard driver (PS2/USB) for the kernel.
 *
 * @details Keyboard driver (PS2/USB) for the kernel. Enables the user inputs
 * through the keyboard.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <Panic.h>
#include <X64Cpu.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <VirtualFS.h>
#include <DeviceTree.h>
#include <Interrupts.h>
#include <KernelHeap.h>
#include <KernelError.h>
#include <DriverManager.h>
#include <KernelSemaphore.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* No unit test: this module is tested in real-world conditions. */

/* Header file */
#include <Keyboard.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "X86 Keyboard"

/** @brief FDT property for comm ports */
#define KBD_FDT_COMM_PROP "comm"
/** @brief FDT property for interrupt  */
#define KBD_FDT_INT_PROP "interrupts"
/** @brief FDT property for device path */
#define KBD_FDT_DEVICE_PROP "device"

/** @brief Cast a pointer to a keyboard driver controler */
#define GET_CONTROLER(PTR) ((kbd_controler_t*)PTR)

/** @brief Defines the maximal size of the keyboard input buffer */
#define KBD_INPUT_BUFFER_SIZE 128

/** @brief Defines the read available status on the keyboard */
#define KBD_INT_STATUS_DATA_AVAILABLE 0x01

/** @brief Keyboard specific key code: backspace. */
#define KEY_BACKSPACE '\b'
/** @brief Keyboard specific key code: tab. */
#define KEY_TAB '\t'
/** @brief Keyboard specific key code: return. */
#define KEY_RETURN '\n'
/** @brief Keyboard specific key code: left shift. */
#define KEY_LSHIFT 0x0400
/** @brief Keyboard specific key code: right shift. */
#define KEY_RSHIFT 0x0500

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Keyboard code to key mapping. */
typedef struct
{
  /** @brief Regular mapping. */
  uint16_t regular[128];
  /** @brief Maj mapping. */
  uint16_t shifted[128];
} S_KeyMapper;


/** @brief x86 Keyboard driver controler. */
typedef struct
{
  /** @brief CPU command port. */
  uint16_t cpuCommPort;
  /** @brief CPU data port. */
  uint16_t cpuDataPort;
  /** @brief The keyboard IRQ number */
  uint32_t irqNumber;
  /** @brief Current start keyboard input buffer cursor */
  size_t inputBufferStartCursor;
  /** @brief Current end keyboard input buffer cursor */
  size_t inputBufferEndCursor;
  /** @brief Input buffer */
  uint8_t pInputBuffer[KBD_INPUT_BUFFER_SIZE];
  /** @brief Input buffer semaphore */
  S_KernelSemaphore inputBufferSem;
  /** @brief Keyboard state flags */
  uint32_t flags;
  /** @brief Stores the VFS driver */
  T_VFSDriver vfsDriver;
  /** @brief Buffer lock */
  S_KernelSpinlock bufferLock;
} S_KeyboardControler;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the keyboard to ensure correctness of execution.
 *
 * @details Assert macro used by the keyboard to ensure correctness of
 * execution. Due to the critical nature of the keyboard, any error generates a
 * kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define KBD_ASSERT(COND, MSG, ERROR) {                    \
  if ((COND) == false)                                    \
  {                                                       \
    PANIC(ERROR, MODULE_NAME, MSG, true, false);          \
  }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Attaches the keyboard driver to the system.
 *
 * @details Attaches the keyboard driver to the system. This function will use
 * the FDT to initialize the keyboard hardware and retreive the keyboard
 * parameters.
 *
 * @param[in] pkFdtNode The FDT node with the compatible declared by the driver.
 *
 * @return The success state or the error code.
 */
static E_Return _Attach(const S_FDTNode* pkFdtNode);

/**
 * @brief Handles a keyboard interrupt.
 *
 * @details Handles a keyboard interrupt. Fills the input buffer with the input
 * data and unblock a thread if it is blocked on the input.
 *
 * @return Returns if the scheduler must be called on return.
 */
static bool _KeyboardInterruptHandler(void);

/**
 * @brief Reads data from the keyboard input buffer.
 *
 * @details Reads data from the keyboard input buffer. The function returns the
 * number of bytes read. If the buffer is empty, the function is blocking until
 * the buffer is filled with the required number of bytes.
 *
 * @param[in] pDrvCtrl The driver to be used.
 * @param[out] pBuffer The buffer used to receive data.
 * @param[in] kBufferSize The number of bytes to read.
 *
 * @return The function returns the number of bytes read or -1 on error.
 */
static ssize_t _KeyboardRead(void*        pDrvCtrl,
                             char*        pBuffer,
                             const size_t kBufferSize);

/**
 * @brief Parses a keyboard keycode.
 *
 * @details Parses the keycode given as parameter and execute the corresponding
 * action.
 *
 * @param[in] kKey The keycode to parse.
 *
 * @return When the keycode corresponds to a character, the chacater is
 * returned, otherwise, the NULL character is returned.
 */
static char _ManageKeycode(const int8_t kKey);

/**
 * @brief Keyboard VFS open hook.
 *
 * @details Keyboard VFS open hook. This function returns a handle to control the
 * keyboard driver through VFS.
 *
 * @param[in, out] pDrvCtrl The keyboard driver that was registered in the VFS.
 * @param[in] kpPath The path in the keyboard driver mount point.
 * @param[in] flags The open flags, must be O_RDONLY.
 * @param[in] mode Unused.
 *
 * @return The function returns an internal handle used by the driver during
 * file operations.
 */
static void* _KeyboardVFSOpen(void*       pDrvCtrl,
                              const char* kpPath,
                              int         flags,
                              int         mode);

/**
 * @brief Keyboard VFS close hook.
 *
 * @details Keyboard VFS close hook. This function closes a handle that was
 * created when calling the open function.
 *
 * @param[in, out] pDrvCtrl The keyboard driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static int32_t _KeyboardVFSClose(void* pDrvCtrl, void* pHandle);

/**
 * @brief Keyboard VFS read hook.
 *
 * @details Keyboard VFS read hook. This function reads a string from the
 * keyboard.
 *
 * @param[in, out] pDrvCtrl The keyboard driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[out] pBuffer The buffer that receives the string to read.
 * @param[in] count The number of bytes of the string to read.
 *
 * @return The function returns the number of bytes read or -1 on error;
 */
static ssize_t _KeyboardVFSRead(void*  pDrvCtrl,
                                void*  pHandle,
                                void*  pBuffer,
                                size_t count);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief Keyboard driver instance. */
static S_Driver sX86KeyboardDriver =
{
  .pName         = "X86 Keyboard Driver",
  .pDescription  = "X86 Keyboard Driver for roOs",
  .pCompatible   = "x86,x86-generic-keyboard",
  .pVersion      = "2.0",
  .pDriverAttach = _Attach
};

/** @brief Tells if a keyboard is initialized. */
static bool sKeyboardInitialized = false;

/** @brief Stores the keyboard used for input, only one can be used */
static S_KeyboardControler sInputCtrl;

/** @brief Keyboard map. */
static const S_KeyMapper ksQwertyMap =
{
    .regular =
    {
        0,
        0,   // ESCAPE
        '1',
        '2',
        '3',
        '4',
        '5',
        '6',
        '7',
        '8',
        '9',                      // 10
        '0',
        '-',
        '=',
        KEY_BACKSPACE,   // BACKSPACE
        KEY_TAB,   // TAB
        'q',
        'w',
        'e',
        'r',
        't',                    // 20
        'y',
        'u',
        'i',
        'o',
        'p',
        0,   // MOD ^
        0,   // MOD ¸
        KEY_RETURN,   // ENTER
        0,   // VER MAJ
        'a',                    // 30
        's',
        'd',
        'f',
        'g',
        'h',
        'j',
        'k',
        'l',
        ';',
        0,   // MOD `           // 40
        0,
        KEY_LSHIFT,   // LEFT SHIFT
        '<',
        'z',
        'x',
        'c',
        'v',
        'b',
        'n',
        'm',                    // 50
        ',',
        '.',
        0, // é
        KEY_RSHIFT,    // RIGHT SHIFT
        0,
        0,     // ALT left / right
        ' ',
        0,
        0,     // F1
        0,     // F2               // 60
        0,     // F3
        0,     // F4
        0,     // F5
        0,     // F6
        0,     // F7
        0,     // F8
        0,     // F9
        0,     // SCROLL LOCK
        0,     // PAUSE
        0,                     // 70
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,                  // 80
        0,
        0
    },

    .shifted =
    {
        0,
        0,   // ESCAPE
        '!',
        '"',
        '/',
        '$',
        '%',
        '?',
        '&',
        '*',
        '(',                      // 10
        ')',
        '_',
        '+',
        KEY_BACKSPACE,   // BACKSPACE
        KEY_TAB,         // TAB
        'Q',
        'W',
        'E',
        'R',
        'T',                    // 20
        'Y',
        'U',
        'I',
        'O',
        'P',
        0,   // MOD ^
        0,   // MOD ¨
        KEY_RETURN,   // ENTER
        0,   // VER MAJ
        'A',                    // 30
        'S',
        'D',
        'F',
        'G',
        'H',
        'J',
        'K',
        'L',
        ':',
        0,   // MOD `           // 40
        0,
        KEY_LSHIFT,   // LEFT SHIFT
        '>',
        'Z',
        'X',
        'C',
        'V',
        'B',
        'N',
        'M',                    // 50
        '\'',
        '.',
        0, // É
        KEY_RSHIFT,    // RIGHT SHIFT
        0,
        0,     // ALT left / right
        ' ',
        0,
        0,     // F1
        0,     // F2               // 60
        0,     // F3
        0,     // F4
        0,     // F5
        0,     // F6
        0,     // F7
        0,     // F8
        0,     // F9
        0,     // SCROLL LOCK
        0,     // PAUSE
        0,                     // 70
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,                  // 80
        0,
        0
    }
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static E_Return _Attach(const S_FDTNode* pkFdtNode)
{
  const uint32_t* kpUintProp;
  const char*     kpStrProp;
  size_t          propLen;
  E_Return        retCode;
  E_Return        error;

  if (sKeyboardInitialized == false)
  {
    /* Init structures */
    memset(&sInputCtrl, 0, sizeof(S_KeyboardControler));
    retCode = NO_ERROR;

    /* Get the keyboard CPU communication ports */
    kpUintProp = FDTGetProp(pkFdtNode, KBD_FDT_COMM_PROP, &propLen);
    if (kpUintProp != NULL && propLen == sizeof(uint32_t) * 2)
    {
      sInputCtrl.cpuCommPort = (uint16_t)FDTTOCPU32(*kpUintProp);
      sInputCtrl.cpuDataPort = (uint16_t)FDTTOCPU32(*(kpUintProp + 1));
    }
    else
    {
      retCode = ERR_INVALID_VALUE;
    }

    /* Get IRQ lines */
    kpUintProp = FDTGetProp(pkFdtNode, KBD_FDT_INT_PROP, &propLen);
    if (kpUintProp != NULL && propLen == sizeof(uint32_t) * 2)
    {
      sInputCtrl.irqNumber = (uint8_t)FDTTOCPU32(*(kpUintProp + 1));
    }
    else
    {
      retCode = ERR_INVALID_VALUE;
    }

    kpStrProp = FDTGetProp(pkFdtNode, KBD_FDT_DEVICE_PROP, &propLen);
    if (kpStrProp == NULL || propLen  == 0)
    {
      retCode = ERR_INVALID_VALUE;
    }

    if (retCode == NO_ERROR)
    {
      /* Init semaphore */
      retCode = KernelSemaphoreInit(&sInputCtrl.inputBufferSem,
                                    0,
                                    KSEMAPHORE_FLAG_QUEUING_PRIO |
                                    KSEMAPHORE_FLAG_BINARY);
      if (retCode == NO_ERROR)
      {
        /* Register the interrupt */
        retCode = InterruptRegister(sInputCtrl.irqNumber,
                                    _KeyboardInterruptHandler,
                                    true);

        /* If we are already registered, error */
        if (retCode == NO_ERROR)
        {
          /* Register the driver */
          sInputCtrl.vfsDriver = RegisterVFSDriver(kpStrProp,
                                                   &sInputCtrl,
                                                   _KeyboardVFSOpen,
                                                   _KeyboardVFSClose,
                                                   _KeyboardVFSRead,
                                                   NULL,
                                                   NULL,
                                                   NULL);
          if (sInputCtrl.vfsDriver != VFS_DRIVER_INVALID)
          {
            /* Set typematic settings */
            while ((CPUPortReadByte(sInputCtrl.cpuCommPort) & 2) != 0){}
            CPUPortWriteByte(0xF3, sInputCtrl.cpuDataPort);
            while ((CPUPortReadByte(sInputCtrl.cpuCommPort) & 2) != 0){}
            CPUPortWriteByte(0x20, sInputCtrl.cpuDataPort);
            while ((CPUPortReadByte(sInputCtrl.cpuCommPort) & 2) != 0){}
            CPUPortReadByte(sInputCtrl.cpuDataPort);

            /* Set the interrupt mask */
            InterruptSetIRQMask(sInputCtrl.irqNumber, true);
            InterruptSetEOI(sInputCtrl.irqNumber);

            /* Initialize the lock */
            KERNEL_SPINLOCK_INIT(sInputCtrl.bufferLock);

            /* Init buffer */
            sInputCtrl.inputBufferStartCursor = 0;
            sInputCtrl.inputBufferEndCursor   = 0;
          }
          else
          {
            retCode = ERR_UNAUTHORIZED_ACTION;
            error = InterruptRemove(sInputCtrl.irqNumber, true);
            KBD_ASSERT(error == NO_ERROR,
                        "Failed to unregister keyboard interrupt",
                        error);
            error = KernelSemaphoreDestroy(&sInputCtrl.inputBufferSem);
            KBD_ASSERT(error == NO_ERROR,
                      "Failed to destroy keyboard semaphore",
                      error);
          }
        }
        else
        {
          error = KernelSemaphoreDestroy(&sInputCtrl.inputBufferSem);
          KBD_ASSERT(error == NO_ERROR,
                     "Failed to destroy keyboard semaphore",
                     error);
        }
      }
    }
  }
  else
  {
    retCode = ERR_UNAUTHORIZED_ACTION;
  }

  if (retCode == NO_ERROR)
  {
    sKeyboardInitialized = true;
  }
  return retCode;
}

static bool _KeyboardInterruptHandler(void)
{
    uint8_t  data;
    E_Return error;
    size_t   availableSpace;
    bool     schedule;

    if (sKeyboardInitialized == true)
    {
      /* Check is we received a data */
      data = CPUPortReadByte(sInputCtrl.cpuDataPort);
      data = _ManageKeycode(data);

      if (data != 0)
      {
        /* Try to add the new data to the buffer */
        KERNEL_LOCK(sInputCtrl.bufferLock);

        if (sInputCtrl.inputBufferEndCursor >=
            sInputCtrl.inputBufferStartCursor)
        {
          availableSpace = KBD_INPUT_BUFFER_SIZE -
                           sInputCtrl.inputBufferEndCursor +
                           sInputCtrl.inputBufferStartCursor;
        }
        else
        {
          availableSpace = sInputCtrl.inputBufferStartCursor -
                           sInputCtrl.inputBufferEndCursor;
        }

        if (availableSpace > 1)
        {
          /* Read the data */
          sInputCtrl.pInputBuffer[sInputCtrl.inputBufferEndCursor] = data;

          sInputCtrl.inputBufferEndCursor =
              (sInputCtrl.inputBufferEndCursor + 1) %
              KBD_INPUT_BUFFER_SIZE;
        }

        KERNEL_UNLOCK(sInputCtrl.bufferLock);

        if (availableSpace > 1)
        {
          /* Post the semaphore */
          error = KernelSemaphorePost(&sInputCtrl.inputBufferSem);
          KBD_ASSERT(error == NO_ERROR,
                    "Failed to post keyboard semaphore",
                    error);
        }

        schedule = true;
      }
      else
      {
        schedule = false;
      }
    }
    else
    {
      schedule = false;
    }

    /* Set EOI */
    InterruptSetEOI(sInputCtrl.irqNumber);

    return schedule;
}

static ssize_t _KeyboardRead(void*        pDrvCtrl,
                             char*        pBuffer,
                             const size_t kBufferSize)
{
  E_Return error;
  size_t   toRead;
  size_t   bytesToRead;
  size_t   usedSpace;
  size_t   i;
  ssize_t  readBytes;

  (void)pDrvCtrl;

  if (sKeyboardInitialized == true)
  {
    readBytes = 0;
    toRead = kBufferSize;
    while (toRead != 0)
    {
      /* Copy if we can */
      error = KernelSemaphoreWait(&sInputCtrl.inputBufferSem);
      KBD_ASSERT(error == NO_ERROR,
                  "Failed to wait keyboard semaphore",
                  error);

      KERNEL_LOCK(sInputCtrl.bufferLock);

      if (sInputCtrl.inputBufferEndCursor >=
          sInputCtrl.inputBufferStartCursor)
      {
        usedSpace = sInputCtrl.inputBufferEndCursor -
                    sInputCtrl.inputBufferStartCursor;
      }
      else
      {
        usedSpace = KBD_INPUT_BUFFER_SIZE -
                    sInputCtrl.inputBufferStartCursor +
                    sInputCtrl.inputBufferEndCursor;
      }

      /* Get what we can read */
      bytesToRead = MIN(toRead, usedSpace);

      /* Copy */
      for (i = 0; i < bytesToRead; ++i)
      {
        *pBuffer = sInputCtrl.pInputBuffer[sInputCtrl.inputBufferStartCursor];
        ++pBuffer;
        sInputCtrl.inputBufferStartCursor =
            (sInputCtrl.inputBufferStartCursor + 1) %
            KBD_INPUT_BUFFER_SIZE;
      }

      readBytes += bytesToRead;
      toRead -= bytesToRead;
      usedSpace -= bytesToRead;

      KERNEL_UNLOCK(sInputCtrl.bufferLock);

      /* If we can still read data, post the semaphore, we read what we had to
       * since we used
       * bytesToRead = MIN(toRead, sInputCtrl.inputBufferCursor)
       */
      if (usedSpace > 0)
      {
        error = KernelSemaphorePost(&sInputCtrl.inputBufferSem);
        KBD_ASSERT(error == NO_ERROR,
                    "Failed to post keyboard semaphore",
                    error);
      }
    }
  }
  else
  {
    readBytes = -1;
  }

  return readBytes;
}

static char _ManageKeycode(const int8_t kKey)
{
  char retChar;
  bool mod;
  bool shifted;

  retChar = 0;

  /* Manage push of release */
  if (kKey >= 0)
  {
    mod = false;

    /* Manage modifiers */
    switch(ksQwertyMap.regular[kKey])
    {
      case KEY_LSHIFT:
        sInputCtrl.flags |= KEY_LSHIFT;
        mod = true;
        break;
      case KEY_RSHIFT:
        sInputCtrl.flags |= KEY_RSHIFT;
        mod = true;
        break;
      default:
          break;
    }

    /* Manage only set characters */
    if (mod == false &&
        (ksQwertyMap.regular[kKey] != 0 || ksQwertyMap.shifted[kKey] != 0))
    {
      shifted = (sInputCtrl.flags & KEY_LSHIFT) != 0 ||
                (sInputCtrl.flags & KEY_RSHIFT) != 0;
      retChar = (shifted > 0) ?
                  ksQwertyMap.shifted[kKey] :
                  ksQwertyMap.regular[kKey];
    }
  }
  else
  {
    /* Manage modifiers */
    switch(ksQwertyMap.shifted[kKey + 128])
    {
      case KEY_LSHIFT:
        sInputCtrl.flags &= ~KEY_LSHIFT;
        break;
      case KEY_RSHIFT:
        sInputCtrl.flags &= ~KEY_RSHIFT;
        break;
      default:
        break;
    }
  }

  return retChar;
}

static void* _KeyboardVFSOpen(void*       pDrvCtrl,
                              const char* kpPath,
                              int         flags,
                              int         mode)
{
  void* pRetHandle;

  (void)pDrvCtrl;
  (void)mode;

  if ((*kpPath == VFS_PATH_DELIMITER && *(kpPath + 1) != 0) || *kpPath != 0)
  {
    /* The path must be empty */
    pRetHandle = (void*)-1;
  }
  else if (flags != O_RDONLY)
  {
    /* The flags must be O_RDONLY */
    pRetHandle = (void*)-1;
  }
  else
  {
    /* We don't need a handle, return NULL */
    pRetHandle = (void*)NULL;
  }
  return pRetHandle;
}

static int32_t _KeyboardVFSClose(void* pDrvCtrl, void* pHandle)
{
  int32_t retCode;
  (void)pDrvCtrl;

  if (pHandle == (void*)-1)
  {
    retCode = -1;
  }
  else
  {
    retCode = 0;
  }

  return retCode;
}

static ssize_t _KeyboardVFSRead(void*  pDrvCtrl,
                                void*  pHandle,
                                void*  pBuffer,
                                size_t count)
{
  ssize_t retVal;

  if (pHandle == (void*)-1)
  {
    retVal = -1;
  }
  else
  {
    retVal = _KeyboardRead(pDrvCtrl, pBuffer, count);
  }

  return retVal;
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_FDT(sX86KeyboardDriver);

/************************************ EOF *************************************/