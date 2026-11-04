/*******************************************************************************
 * @file VGAText.c
 *
 * @see VGAText.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 23/05/2024
 *
 * @version 2.0
 *
 * @brief VGA text mode driver.
 *
 * @details Allows the kernel to display text and general ASCII characters to be
 * displayed on the screen. Includes cursor management, screen colors management
 * and other fancy screen driver things.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <CPU.h>
#include <IOCTL.h>
#include <Panic.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <Memory.h>
#include <X64Cpu.h>
#include <Console.h>
#include <Critical.h>
#include <Scheduler.h>
#include <VirtualFS.h>
#include <KernelError.h>
#include <KernelOutput.h>
#include <DriverManager.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* No unit test: this module is tested in real-world conditions. */

/* Header file */
#include <VGAText.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Module's name */
#define MODULE_NAME "X86_VGA_TEXT"

/** @brief VGA cursor position command low. */
#define VGA_CONSOLE_CURSOR_COMM_LOW  0x0F
/** @brief VGA cursor position command high. */
#define VGA_CONSOLE_CURSOR_COMM_HIGH 0x0E
/** @brief VGA synchronization port */
#define VGA_CONSOLE_CURSOR_SYNC 0x3DA

/** @brief VGA console line count */
#define VGA_CONSOLE_LINE_COUNT 25
/** @brief VGA console column count */
#define VGA_CONSOLE_COLUMN_COUNT 80
/** @brief VGA console size in bytes */
#define VGA_CONSOLE_COMM_PORT 0x3D4
/** @brief VGA console data port */
#define VGA_CONSOLE_DATA_PORT 0x3D5
/** @brief VGA console frame buffer address */
#define VGA_CONSOLE_FRAMEBUFFER_ADDRESS 0xB8000ULL
/** @brief VGA console frame buffer size in bytes */
#define VGA_CONSOLE_FRAMEBUFFER_SIZE \
  (VGA_CONSOLE_LINE_COUNT * VGA_CONSOLE_COLUMN_COUNT * 2)

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief x86 VGA driver controller. */
typedef struct
{
  /** @brief Stores the curent screen's color scheme. */
  S_ColorScheme screenScheme;
  /** @brief Stores the curent screen's cursor settings. */
  S_ConsoleCursor screenCursor;
  /** @brief VGA frame buffer address. */
  uint16_t* pFramebuffer;
  /** @brief VGA internal buffer address. */
  uint16_t* pInternalBuffer;
  /** @brief Size in bytes of the framebuffer. */
  size_t framebufferSize;
  /** @brief The VGA refresh rate. */
  uint32_t frameRate;
  /** @brief VGA display thread. */
  S_KernelThread* pDisplayThread;
  /** @brief Stores the device path */
  const char* kpDevicePath;
  /** @brief Buffer lock */
  T_Spinlock bufferLock;
} S_VGAControler;


/*******************************************************************************
 * MACROS
 ******************************************************************************/
/**
 * @brief Assert macro used by the VGA to ensure correctness of execution.
 *
 * @details Assert macro used by the VGA to ensure correctness of execution.
 * Due to the critical nature of the VGA, any error generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define VGA_ASSERT(COND, MSG, ERROR) {                    \
  if ((COND) == false)                                    \
  {                                                       \
    PANIC(ERROR, MODULE_NAME, MSG, false, false);         \
  }                                                       \
}

/**
 * @brief Get the VGA frame buffer virtual address.
 *
 * @details Get the VGA frame buffer virtual address correponding to a
 * certain region of the buffer given the parameters.
 *
 * @param[in] LINE The frame buffer line.
 * @param[in] COLUMN The frame buffer column.
 *
 * @return The frame buffer virtual address is get correponding to a
 * certain region of the buffer given the parameters.
 */
#define GET_FRAME_BUFFER_AT(LINE, COL)                           \
  ((sVGADriverCtrl.pInternalBuffer) +                            \
  ((COL) + (LINE) * VGA_CONSOLE_COLUMN_COUNT))

/**
 * @brief Unrolled action for the fast memcpy used by the VGA driver.
 *
 * @details Unrolled action for the fast memcpy used by the VGA driver. Used in
 * the Duff's device.
 *
 * @param[in] X The switch case.
 */
#define VGA_FAST_CPY_UNROLL_ACTION(X) {                     \
  case (X):                                               \
    __asm__ __volatile__ ("movups (%0), %%xmm0\n\t"       \
                          "movntdq %%xmm0, (%1)\n\t"      \
                          :                               \
                          : "r"(pSrcPtr), "r"(pDstPtr)    \
                          : "memory");                    \
    pDstPtr += 16;                                        \
    pSrcPtr += 16;                                        \
}

/**
 * @brief Unrolled action for the fast fill used by the VGA driver.
 *
 * @details Unrolled action for the fast fill used by the VGA driver. Used in
 * the Duff's device.
 *
 * @param[in] X The switch case.
 */
#define VGA_FAST_FILL_UNROLL_ACTION(X) {                    \
  case (X):                                                 \
    __asm__ __volatile__ ("movntdq %%xmm0, (%0)\n\t"        \
                          :                                 \
                          : "r"(pDestPtr)                   \
                          : "memory");                      \
    pDestPtr += 16;                                         \
}
/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Attaches the VGA driver to the system.
 *
 * @details This function is called during the system initialization to attach
 * the VGA driver to the system.
 *
 * @return The success state or the error code.
 */
static E_Return _Attach(void);

/**
 * @brief Prints a character to the selected coordinates.
 *
 * @details Prints a character to the selected coordinates by setting the memory
 * accordingly.
 *
 * @param[in] kLine The line index where to write the character.
 * @param[in] kColumn The colums index where to write the character.
 * @param[in] kCharacter The character to display on the screem.
 */
static inline void _PrintChar(const uint32_t kLine,
                              const uint32_t kColumn,
                              const char     kCharacter);

/**
 * @brief Processes the character in parameters.
 *
 * @param[in, out] pDriverCtrl The VGA driver controller to use.
 * @details Check the character nature and code. Corresponding to the
 * character's code, an action is taken. A regular character will be printed
 * whereas \\n will create a line feed.
 *
 * @param[in] kCharacter The character to process.
 */
static void _ProcessChar(const char kCharacter);

/**
 * @brief Clears the screen by printing null character character on black
 * background.
 */
static void _ClearFramebuffer(void);

/**
 * @brief Saves the cursor attributes in the buffer given as parameter.
 *
 * @details Fills the buffer given as parrameter with the current cursor
 * settings.
 *
 * @param[out] pBuffer The cursor buffer in which the current cursor
 * position is going to be saved.
 */
static void _GetCursor(S_ConsoleCursor* pBuffer);

/**
 * @brief Restores the cursor attributes from the buffer given as parameter.
 *
 * @details The function will restores the cursor attributes from the buffer
 * given as parameter.
 *
 * @param[in] kpBuffer The cursor buffer containing the new
 * coordinates of the cursor.
 */
static void _SetCursorDirect(const S_ConsoleCursor* kpBuffer);

/**
 * @brief Restores the cursor attributes from the buffer given as parameter.
 *
 * @details The function will restores the cursor attributes from the buffer
 * given as parameter.
 *
 * @param[in] kLine The line index to set the cursor to.
 * @param[in] kColumn The column index to set the cursor to.
 */
static void _SetCursor(const uint32_t kLine, const uint32_t kColumn);

/**
 * @brief Scrolls in the desired direction of lines_count lines.
 *
 * @details The function will scroll of lines_count line in the desired
 * direction.
 *
 * @param[in] kDirection The direction to whoch the console
 * should be scrolled.
 * @param[in] kLines The number of lines to scroll.
 */
static void _Scroll(const E_ScrollDirection kDirection, const uint32_t kLines);

/**
 * @brief Sets the color scheme of the screen.
 *
 * @details Replaces the curent color scheme used t output data with the new
 * one given as parameter.
 *
 * @param[in] kpColorScheme The new color scheme to apply to
 * the screen console.
 */
static void _SetScheme(const S_ColorScheme* kpColorScheme);


/**
 * @brief Saves the color scheme in the buffer given as parameter.
 *
 * @details Fills the buffer given as parameter with the current screen's
 * color scheme value.
 *
 * @param[out] pBuffer The buffer that will receive the current
 * color scheme used by the screen console.
 */
static void _GetScheme(S_ColorScheme* pBuffer);

/**
 * @brief Flushes the screen output.
 *
 * @details The function will request a flush to the screen output driver.
 */
static void _Flush(void);

/**
 * @brief VGA display routine.
 *
 * @details This function transfers the internal buffer data to the VGA frame
 * buffer. If the VGA frame buffer is used as internal buffer, then no action
 * is performed.
 *
 * @param[in] pArgs Unused.
 *
 * @return The function should never return.
 */
static void* _DisplayRoutine(void* pArgs);

/**
 * @brief VGA fast fill function.
 *
 * @details VGA fast fill function. Using SSE instructions to speedup the
 * filling of the back buffer.
 *
 * @param[in] bufferAddr The start address of the region of the buffer to fill.
 * @param[in] kPixel The color pixel to fill.
 * @param[in] pixelCount The amount of pixel the fill.
 */
static inline void _FastFill(uintptr_t      bufferAddr,
                             const uint16_t kPixel,
                             uint32_t       pixelCount);

/**
 * @brief VGA fast copy function.
 *
 * @details VGA fast copy function. Using SSE instructions to speedup the
 * copy of the back buffer.
 *
 * @param[out] pDest The start address of the region of the buffer to copy.
 * @param[in] kpSrc The start address of the source region to copy.
 * @param[in] sizeThe The size in bytes to copy.
 */
static inline void _FastMemcpy(void*       pDest,
                               const void* kpSrc,
                               size_t      size);

/**
 * @brief VGA VFS open hook.
 *
 * @details VGA VFS open hook. This function returns a handle to control the
 * VGA driver through VFS.
 *
 * @param[in, out] pDrvCtrl The VGA driver that was registered in the VFS.
 * @param[in] kpPath The path in the VGA driver mount point.
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
 * @brief VGA VFS close hook.
 *
 * @details VGA VFS close hook. This function closes a handle that was created
 * when calling the open function.
 *
 * @param[in, out] pDrvCtrl The VGA driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static int32_t _VFSClose(void* pDrvCtrl, void* pHandle);

/**
 * @brief VGA VFS write hook.
 *
 * @details VGA VFS write hook. This function writes a string to the VGA
 * framebuffer.
 *
 * @param[in, out] pDrvCtrl The VGA driver that was registered in the VFS.
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

/**
 * @brief VGA VFS IOCTL hook.
 *
 * @details VGA VFS IOCTL hook. This function performs the IOCTL for the VGA
 * driver.
 *
 * @param[in, out] pDrvCtrl The VGA driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] operation The operation to perform.
 * @param[in, out] pArgs The arguments for the IOCTL operation.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static ssize_t _VFSIOCTL(void*    pDriverData,
                         void*    pHandle,
                         uint32_t operation,
                         void*    pArgs);
/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief VGA driver instance. */
static S_LayerDriver  sX86VGADriver =
{
  .pName         = "X86 VGA driver",
  .pDescription  = "X86 VGA driver for roOs",
  .pVersion      = "2.0",
  .layer         = 1,
  .pDriverAttach = _Attach
};

/** @brief VGA driver controller. */
static S_VGAControler sVGADriverCtrl;

/** @brief VGA VFS driver controller. */
static T_VFSDriver sVFSDriver;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static E_Return _Attach(void)
{
  E_Return   retCode;
  uintptr_t  frameBufferPhysAddr;
  size_t     frameBufferPhysSize;
  void*      mappedFrameBufferAddr;
  S_CPUMask  cpuMask;
  uint32_t   i;
  const char threadName[THREAD_NAME_MAX_LENGTH] = "Vga-Display\0";

  if (CONFIG_USE_VESA == 0)
  {
    /* Init structures */
    memset(&sVGADriverCtrl, 0, sizeof(S_VGAControler));

    sVGADriverCtrl.frameRate = CONFIG_VGA_TEXT_REFRESH_RATE;
    sVGADriverCtrl.kpDevicePath = CONFIG_VGA_TEXT_DEVICE_PATH;
    SPINLOCK_INIT(sVGADriverCtrl.bufferLock);

    /* Align and map framebuffer */
    frameBufferPhysAddr = VGA_CONSOLE_FRAMEBUFFER_ADDRESS;
    frameBufferPhysSize = ALIGN_UP(VGA_CONSOLE_FRAMEBUFFER_SIZE,
                                    KERNEL_PAGE_SIZE);
    mappedFrameBufferAddr = MemoryKernelMap((void*)frameBufferPhysAddr,
                                            frameBufferPhysSize,
                                            MEMMGR_MAP_HARDWARE |
                                            MEMMGR_MAP_KERNEL   |
                                            MEMMGR_MAP_RW       |
                                            MEMMGR_MAP_WRITE_COMBINING,
                                            &retCode);

    if (mappedFrameBufferAddr != NULL && retCode == NO_ERROR)
    {
      /* Update framebuffer address but not size even if we mapped more */
      sVGADriverCtrl.pFramebuffer = mappedFrameBufferAddr;
      sVGADriverCtrl.framebufferSize = VGA_CONSOLE_FRAMEBUFFER_SIZE;

      /* Register the driver */
      sVFSDriver = RegisterVFSDriver(sVGADriverCtrl.kpDevicePath,
                                      NULL,
                                      _VFSOpen,
                                      _VFSClose,
                                      NULL,
                                      _VFSWrite,
                                      NULL,
                                      _VFSIOCTL);

      if (sVFSDriver != VFS_DRIVER_INVALID)
      {
        /* Set initial scheme */
        sVGADriverCtrl.screenScheme.background = BG_BLACK;
        sVGADriverCtrl.screenScheme.foreground = FG_WHITE;

        /* Set initial cursor position */
        sVGADriverCtrl.screenCursor.x = 0;
        sVGADriverCtrl.screenCursor.y = 0;

        /* Create the display thread an use internal buffer */
        CPU_MASK_RESET(cpuMask);
        for (i = 0; i < CPUGetCount(); ++i)
        {
          CPU_MASK_SET(cpuMask, i);
        }

        if (sVGADriverCtrl.frameRate > 0)
        {
          retCode = CreateThread(&sVGADriverCtrl.pDisplayThread,
                                  true,
                                  0,
                                  threadName,
                                  0x1000,
                                  cpuMask,
                                  _DisplayRoutine,
                                  NULL,
                                  NULL);
          if (retCode == NO_ERROR)
          {
            /* Create the internal buffer */
            sVGADriverCtrl.pInternalBuffer = MemoryKernelAllocate(
                                              frameBufferPhysSize,
                                              MEMMGR_MAP_KERNEL   |
                                              MEMMGR_MAP_RW,
                                              &retCode);
            if (retCode != NO_ERROR)
            {
              /* Use the VGA buffer directy */
              sVGADriverCtrl.pInternalBuffer = sVGADriverCtrl.pFramebuffer;
              retCode = NO_ERROR;
            }
          }
          else
          {
            /* Use the VGA buffer directy */
            sVGADriverCtrl.pInternalBuffer = sVGADriverCtrl.pFramebuffer;
            retCode = NO_ERROR;
          }
        }
        else
        {
          /* Use the VGA buffer directy */
          sVGADriverCtrl.pInternalBuffer = sVGADriverCtrl.pFramebuffer;
          retCode = NO_ERROR;
        }
      }
      else
      {
        retCode = MemoryKernelUnmap(mappedFrameBufferAddr,
                                  frameBufferPhysSize);
        VGA_ASSERT(retCode == NO_ERROR,
                    "Failed to unmap framebuffer",
                    retCode);
        retCode = ERR_EXCEEDED_LIMIT;
      }

    }
    else
    {
      retCode = MemoryKernelUnmap(mappedFrameBufferAddr, frameBufferPhysSize);
      VGA_ASSERT(retCode == NO_ERROR, "Failed to unmap framebuffer", retCode);
      retCode = ERR_EXCEEDED_LIMIT;
    }
  }
  else
  {
    retCode = NO_ERROR;
  }
  return retCode;
}

static inline void _PrintChar(const uint32_t kLine,
                              const uint32_t kColumn,
                              const char     kCharacter)
{
  volatile uint16_t* pScreenMem;

  SpinlockAcquire(&sVGADriverCtrl.bufferLock);

  if ((uint8_t)kLine < VGA_CONSOLE_LINE_COUNT &&
      (uint8_t)kColumn < VGA_CONSOLE_COLUMN_COUNT)
  {
    /* Get address to inject */
    pScreenMem = GET_FRAME_BUFFER_AT(kLine, kColumn);

    /* Inject the character with the current colorscheme */
    *pScreenMem = kCharacter |
                  ((sVGADriverCtrl.screenScheme.background << 8) & 0xF000) |
                  ((sVGADriverCtrl.screenScheme.foreground << 8) & 0x0F00);
  }

  SpinlockRelease(&sVGADriverCtrl.bufferLock);
}

static void _ProcessChar(const char kCharacter)
{
  /* If character is a normal ASCII character */
  if (kCharacter > 31 && kCharacter < 127)
  {
    /* Manage end of line cursor position */
    if ((uint8_t)sVGADriverCtrl.screenCursor.x > VGA_CONSOLE_COLUMN_COUNT - 1)
    {
      sVGADriverCtrl.screenCursor.x = 0;
      ++sVGADriverCtrl.screenCursor.y;
    }

    /* Manage end of screen cursor position */
    if ((uint8_t)sVGADriverCtrl.screenCursor.y >= VGA_CONSOLE_LINE_COUNT)
    {
      _Scroll(SCROLL_DOWN, 1);
    }

    /* Display character and move cursor */
    _PrintChar(sVGADriverCtrl.screenCursor.y,
               sVGADriverCtrl.screenCursor.x++,
               kCharacter);
  }
  else
  {
    /* Manage special ACSII characters*/
    switch (kCharacter)
    {
      /* Backspace */
      case '\b':
          if (sVGADriverCtrl.screenCursor.x > 0)
          {
            _SetCursor(sVGADriverCtrl.screenCursor.y,
                       sVGADriverCtrl.screenCursor.x - 1);
          }
          else if (sVGADriverCtrl.screenCursor.y > 0)
          {
            _SetCursor(sVGADriverCtrl.screenCursor.y - 1,
                       VGA_CONSOLE_COLUMN_COUNT - 1);
          }
          break;
      /* Tab */
      case '\t':
        if ((uint8_t)sVGADriverCtrl.screenCursor.x + 4 <
            VGA_CONSOLE_COLUMN_COUNT - 1)
        {
          _SetCursor(sVGADriverCtrl.screenCursor.y,
                     sVGADriverCtrl.screenCursor.x  +
                     (4 - sVGADriverCtrl.screenCursor.x % 4));
        }
        else
        {
          _SetCursor(sVGADriverCtrl.screenCursor.y,
                     VGA_CONSOLE_COLUMN_COUNT - 1);
        }
        break;
      /* Line feed */
      case '\n':
        if ((uint8_t)sVGADriverCtrl.screenCursor.y <
           VGA_CONSOLE_LINE_COUNT - 1)
        {
          _SetCursor(sVGADriverCtrl.screenCursor.y + 1, 0);
        }
        else
        {
          _Scroll(SCROLL_DOWN, 1);
        }
          break;
      /* Clear screen */
      case '\f':
        /* Clear all screen */
        _FastFill((uintptr_t)sVGADriverCtrl.pInternalBuffer,
                  0,
                  sVGADriverCtrl.framebufferSize / sizeof(uint16_t));
        break;
      /* Line return */
      case '\r':
        _SetCursor(sVGADriverCtrl.screenCursor.y, 0);
        break;
      /* Undefined */
      default:
        break;
    }
  }
}

static void _ClearFramebuffer(void)
{
  SpinlockAcquire(&sVGADriverCtrl.bufferLock);
  /* Clear all screen */
  _FastFill((uintptr_t)sVGADriverCtrl.pInternalBuffer,
            0,
            sVGADriverCtrl.framebufferSize / sizeof(uint16_t));
  SpinlockRelease(&sVGADriverCtrl.bufferLock);
}

static void _GetCursor(S_ConsoleCursor* pBuffer)
{
  /* Save cursor attributes */
  pBuffer->x = sVGADriverCtrl.screenCursor.x;
  pBuffer->y = sVGADriverCtrl.screenCursor.y;
}

static void _SetCursorDirect(const S_ConsoleCursor* kpBuffer)
{
  _SetCursor(kpBuffer->y, kpBuffer->x);
}

static inline void _SetCursor(const uint32_t kLine, const uint32_t kColumn)
{
  /* Checks the values of line and column */
  if (kLine < VGA_CONSOLE_LINE_COUNT && kColumn < VGA_CONSOLE_COLUMN_COUNT)
  {
    /* Set new cursor position */
    sVGADriverCtrl.screenCursor.x = kColumn;
    sVGADriverCtrl.screenCursor.y = kLine;
  }
}

static void _Scroll(const E_ScrollDirection kDirection, const uint32_t kLines)
{
  uint8_t   toScroll;
  uint8_t   i;
  uint8_t   j;
  uint16_t* pScreenMem;

  if (VGA_CONSOLE_LINE_COUNT < kLines)
  {
    toScroll = VGA_CONSOLE_LINE_COUNT;
  }
  else
  {
    toScroll = kLines;
  }

  /* Select scroll direction */
  if (kDirection == SCROLL_DOWN)
  {
    SpinlockAcquire(&sVGADriverCtrl.bufferLock);
    /* For each line scroll we want */
    for (j = 0; j < toScroll; ++j)
    {
      /* Copy all the lines to the above one */
      for (i = 0; i < VGA_CONSOLE_LINE_COUNT - 1; ++i)
      {
        memmove(GET_FRAME_BUFFER_AT(i, 0),
                GET_FRAME_BUFFER_AT(i + 1, 0),
                sizeof(uint16_t) * VGA_CONSOLE_COLUMN_COUNT);
      }
    }
    /* Clear last line */
    pScreenMem = GET_FRAME_BUFFER_AT(VGA_CONSOLE_LINE_COUNT - 1, 0);
    memset(pScreenMem, 0, sizeof(uint16_t) * VGA_CONSOLE_COLUMN_COUNT);

    /* Replace cursor */
    _SetCursor(VGA_CONSOLE_LINE_COUNT - toScroll, 0);
    SpinlockRelease(&sVGADriverCtrl.bufferLock);
  }
}

static void _SetScheme(const S_ColorScheme* kpColorScheme)
{
  sVGADriverCtrl.screenScheme.foreground = kpColorScheme->foreground;
  sVGADriverCtrl.screenScheme.background = kpColorScheme->background;
}

static void _GetScheme(S_ColorScheme* pBuffer)
{
  /* Save color scheme into buffer */
  pBuffer->foreground = sVGADriverCtrl.screenScheme.foreground;
  pBuffer->background = sVGADriverCtrl.screenScheme.background;
}

static void _Flush(void)
{
  uint16_t  cursorPosition;
  uint16_t* pFrameBuffer;

  if (sVGADriverCtrl.pFramebuffer != sVGADriverCtrl.pInternalBuffer)
  {
    SpinlockAcquire(&sVGADriverCtrl.bufferLock);
    _FastMemcpy(sVGADriverCtrl.pFramebuffer,
                sVGADriverCtrl.pInternalBuffer,
                sVGADriverCtrl.framebufferSize);
    SpinlockRelease(&sVGADriverCtrl.bufferLock);
  }

  /* Display new position on screen */
  cursorPosition = sVGADriverCtrl.screenCursor.x +
                   sVGADriverCtrl.screenCursor.y * VGA_CONSOLE_COLUMN_COUNT;

  pFrameBuffer = GET_FRAME_BUFFER_AT(sVGADriverCtrl.screenCursor.y,
                                     sVGADriverCtrl.screenCursor.x);
  *pFrameBuffer = ' ' |((FG_WHITE | BG_BLACK) << 8);
  /* Send low part to the screen */
  CPUPortWriteByte(VGA_CONSOLE_CURSOR_COMM_LOW, VGA_CONSOLE_COMM_PORT);
  CPUPortWriteByte((int8_t)(cursorPosition & 0x00FF), VGA_CONSOLE_DATA_PORT);

  /* Send high part to the screen */
  CPUPortWriteByte(VGA_CONSOLE_CURSOR_COMM_HIGH, VGA_CONSOLE_COMM_PORT);
  CPUPortWriteByte((int8_t)((cursorPosition & 0xFF00) >> 8),
                   VGA_CONSOLE_DATA_PORT);
}

static void* _DisplayRoutine(void* pArgs)
{
  E_Return retCode;
  uint8_t  traceStatus;

  (void)pArgs;

  while (true)
  {
    /* VSync wait */
    do
    {
      traceStatus = CPUPortReadByte(VGA_CONSOLE_CURSOR_SYNC);
    } while (traceStatus != 0);
    do
    {
      traceStatus = CPUPortReadByte(VGA_CONSOLE_CURSOR_SYNC);
    } while (traceStatus == 0);

    _Flush();

    retCode = SleepNs(1000000000 / sVGADriverCtrl.frameRate, NULL);
    if (retCode != NO_ERROR)
    {
      KERNEL_ERROR("VGA Text Driver failed to sleep.\n");
    }
  }

  return NULL;
}

static inline void _FastFill(uintptr_t      bufferAddr,
                             const uint16_t kPixel,
                             uint32_t       pixelCount)
{
  register size_t sseSize;
  register size_t n;
  register char*  pDestPtr;
  uint32_t        intstate;
  uint16_t        xmmSave[16]       __attribute__((aligned(16)));
  uint16_t        replicateValue[8] __attribute__((aligned(16)));

  /* Compute the replicated value */
  replicateValue[0] = kPixel;
  replicateValue[1] = kPixel;
  replicateValue[2] = kPixel;
  replicateValue[3] = kPixel;
  replicateValue[4] = kPixel;
  replicateValue[5] = kPixel;
  replicateValue[6] = kPixel;
  replicateValue[7] = kPixel;

  pDestPtr = (char*)bufferAddr;

  /* First unaligned */
  while (((uintptr_t)pDestPtr & 0xF) != 0 && pixelCount > 0)
  {
    *(uint16_t*)pDestPtr = *(uint16_t*)replicateValue;
    pDestPtr += sizeof(uint16_t);
    --pixelCount;
  }

  sseSize = pixelCount / 8;

  if (sseSize > 0)
  {
    KERNEL_ENTER_CRITICAL_LOCAL(intstate);
    /* Save the XMM0 register */
    __asm__ __volatile__ ("movaps %%xmm0, %0\n\t"
                          : "=m" (xmmSave)
                          :
                          : "memory");

    /* Aligned */
    __asm__ __volatile__ ("movups (%0), %%xmm0\n\t"
                          :
                          : "r"(replicateValue)
                          : "memory");

    pixelCount -= sseSize * 8;
    n = (sseSize + 15) / 16;
    switch (sseSize % 16)
    {
      case 0:
        do {
            __asm__ __volatile__ ("movntdq %%xmm0, (%0)\n\t"
                      :
                      : "r"(pDestPtr)
                      : "memory");
            pDestPtr += 16;
      __attribute__ ((fallthrough));
      VGA_FAST_FILL_UNROLL_ACTION(15)
      __attribute__ ((fallthrough));
      VGA_FAST_FILL_UNROLL_ACTION(14)
      __attribute__ ((fallthrough));
      VGA_FAST_FILL_UNROLL_ACTION(13)
      __attribute__ ((fallthrough));
      VGA_FAST_FILL_UNROLL_ACTION(12)
      __attribute__ ((fallthrough));
      VGA_FAST_FILL_UNROLL_ACTION(11)
      __attribute__ ((fallthrough));
      VGA_FAST_FILL_UNROLL_ACTION(10)
      __attribute__ ((fallthrough));
      VGA_FAST_FILL_UNROLL_ACTION(9)
      __attribute__ ((fallthrough));
      VGA_FAST_FILL_UNROLL_ACTION(8)
      __attribute__ ((fallthrough));
      VGA_FAST_FILL_UNROLL_ACTION(7)
      __attribute__ ((fallthrough));
      VGA_FAST_FILL_UNROLL_ACTION(6)
      __attribute__ ((fallthrough));
      VGA_FAST_FILL_UNROLL_ACTION(5)
      __attribute__ ((fallthrough));
      VGA_FAST_FILL_UNROLL_ACTION(4)
      __attribute__ ((fallthrough));
      VGA_FAST_FILL_UNROLL_ACTION(3)
      __attribute__ ((fallthrough));
      VGA_FAST_FILL_UNROLL_ACTION(2)
      __attribute__ ((fallthrough));
      case 1:
          __asm__ __volatile__ ("movntdq %%xmm0, (%0)\n\t"
                    :
                    : "r"(pDestPtr)
                    : "memory");
            pDestPtr += 16;
      } while (--n > 0);
    }

    /* Restore the XMM0 register */
    __asm__ __volatile__ ("movaps %0, %%xmm0\n\t"
                          :
                          : "m" (xmmSave)
                          : "memory");
    KERNEL_EXIT_CRITICAL_LOCAL(intstate);
  }

  /* Last unaligned */
  while (pixelCount > 0)
  {
    *(uint16_t*)pDestPtr = *(uint16_t*)replicateValue;
    pDestPtr += sizeof(uint16_t);
    --pixelCount;
  }
}

static inline void _FastMemcpy(void*       pDest,
                               const void* kpSrc,
                               size_t      size)
{
  register const char* pSrcPtr;
  register char*       pDstPtr;
  register size_t      sseSize;
  register size_t      n;
  uint32_t             intstate;
  uint8_t              xmmSave[16] __attribute__((aligned(16)));

  pSrcPtr = kpSrc;
  pDstPtr = pDest;

  /* If not the same alignement, we will never be able to align, use memcpy */
  if (((uintptr_t)pSrcPtr & 0xF) != ((uintptr_t)pDstPtr & 0xF) || size <= 20)
  {
    memcpy(pDstPtr, pSrcPtr, size);
    return;
  }

  /* First unaligned */
  while (((uintptr_t)pSrcPtr & 0xF) != 0 && size > 0)
  {
    *(uint16_t*)pDstPtr = *(uint16_t*)pSrcPtr;
    pSrcPtr += sizeof(uint16_t);
    pDstPtr += sizeof(uint16_t);
    size -= sizeof(uint16_t);
  }

  sseSize = size / (sizeof(uint64_t) * 2);

  if (sseSize > 0)
  {
    KERNEL_ENTER_CRITICAL_LOCAL(intstate);
    /* Save the XMM0 register */
    __asm__ __volatile__ ("movaps %%xmm0, %0\n\t"
                          : "=m" (xmmSave)
                          :
                          : "memory");
    size -= sseSize * (sizeof(uint64_t) * 2);
    n = (sseSize + 31) / 32;
    switch (sseSize % 32)
    {
      case 0:
        do {
            __asm__ __volatile__ ("movups (%0), %%xmm0\n\t"
                                  "movntdq %%xmm0, (%1)\n\t"
                                  :
                                  :"r"(pSrcPtr), "r"(pDstPtr)
                                  : "memory");
            pDstPtr += 16;
            pSrcPtr += 16;
        __attribute__ ((fallthrough));
        VGA_FAST_CPY_UNROLL_ACTION(31)
        __attribute__ ((fallthrough));
        VGA_FAST_CPY_UNROLL_ACTION(30)
        __attribute__ ((fallthrough));
        VGA_FAST_CPY_UNROLL_ACTION(29)
        __attribute__ ((fallthrough));
        VGA_FAST_CPY_UNROLL_ACTION(28)
        __attribute__ ((fallthrough));
        VGA_FAST_CPY_UNROLL_ACTION(27)
        __attribute__ ((fallthrough));
        VGA_FAST_CPY_UNROLL_ACTION(26)
        __attribute__ ((fallthrough));
        VGA_FAST_CPY_UNROLL_ACTION(25)
        __attribute__ ((fallthrough));
        VGA_FAST_CPY_UNROLL_ACTION(24)
        __attribute__ ((fallthrough));
        VGA_FAST_CPY_UNROLL_ACTION(23)
        __attribute__ ((fallthrough));
        VGA_FAST_CPY_UNROLL_ACTION(22)
        __attribute__ ((fallthrough));
        VGA_FAST_CPY_UNROLL_ACTION(21)
        __attribute__ ((fallthrough));
        VGA_FAST_CPY_UNROLL_ACTION(20)
        __attribute__ ((fallthrough));
        VGA_FAST_CPY_UNROLL_ACTION(19)
        __attribute__ ((fallthrough));
        VGA_FAST_CPY_UNROLL_ACTION(18)
        __attribute__ ((fallthrough));
        VGA_FAST_CPY_UNROLL_ACTION(17)
        __attribute__ ((fallthrough));
        VGA_FAST_CPY_UNROLL_ACTION(16)
        __attribute__ ((fallthrough));
        VGA_FAST_CPY_UNROLL_ACTION(15)
        __attribute__ ((fallthrough));
        VGA_FAST_CPY_UNROLL_ACTION(14)
        __attribute__ ((fallthrough));
        VGA_FAST_CPY_UNROLL_ACTION(13)
        __attribute__ ((fallthrough));
        VGA_FAST_CPY_UNROLL_ACTION(12)
        __attribute__ ((fallthrough));
        VGA_FAST_CPY_UNROLL_ACTION(11)
        __attribute__ ((fallthrough));
        VGA_FAST_CPY_UNROLL_ACTION(10)
        __attribute__ ((fallthrough));
        VGA_FAST_CPY_UNROLL_ACTION(9)
        __attribute__ ((fallthrough));
        VGA_FAST_CPY_UNROLL_ACTION(8)
        __attribute__ ((fallthrough));
        VGA_FAST_CPY_UNROLL_ACTION(7)
        __attribute__ ((fallthrough));
        VGA_FAST_CPY_UNROLL_ACTION(6)
        __attribute__ ((fallthrough));
        VGA_FAST_CPY_UNROLL_ACTION(5)
        __attribute__ ((fallthrough));
        VGA_FAST_CPY_UNROLL_ACTION(4)
        __attribute__ ((fallthrough));
        VGA_FAST_CPY_UNROLL_ACTION(3)
        __attribute__ ((fallthrough));
        VGA_FAST_CPY_UNROLL_ACTION(2)
        __attribute__ ((fallthrough));
      case 1:
          __asm__ __volatile__ ("movups (%0), %%xmm0\n\t"
                                "movntdq %%xmm0, (%1)\n\t"
                                :
                                :"r"(pSrcPtr), "r"(pDstPtr)
                                : "memory");
          pDstPtr += 16;
          pSrcPtr += 16;
        } while (--n > 0);
    }
    /* Restore the XMM0 register */
    __asm__ __volatile__ ("movaps %0, %%xmm0\n\t"
                          :
                          : "m" (xmmSave)
                          : "memory");
    KERNEL_EXIT_CRITICAL_LOCAL(intstate);
  }

  /* Last unaligned */
  while (size > 0)
  {
    *(uint16_t*)pDstPtr = *(uint16_t*)pSrcPtr;
    pSrcPtr += sizeof(uint16_t);
    pDstPtr += sizeof(uint16_t);
    size    -= sizeof(uint16_t);
  }
}

static void* _VFSOpen(void*       pDrvCtrl,
                      const char* kpPath,
                      int         flags,
                      int         mode)
{
  void* retVal;

  (void)pDrvCtrl;
  (void)mode;

  /* The path must be empty */
  if ((*kpPath == VFS_PATH_DELIMITER && *(kpPath + 1) == 0) || *kpPath == 0)
  {
    /* The flags must be O_RDWR */
    if (flags == O_RDWR)
    {
      /* We don't need a handle, return NULL */
      retVal = NULL;
    }
    else
    {
      retVal = (void*)-1;
    }
  }
  else
  {
    retVal = (void*)-1;
  }

  return retVal;
}

static int32_t _VFSClose(void* pDrvCtrl, void* pHandle)
{
  int32_t retVal;

  (void)pDrvCtrl;

  if (pHandle != (void*)-1)
  {
    retVal = 0;
  }
  else
  {
    retVal = -1;
  }

  /* Nothing to do */
  return retVal;
}

static ssize_t _VFSWrite(void*       pDrvCtrl,
                         void*       pHandle,
                         const void* kpBuffer,
                         size_t      count)
{
  size_t      coutSave;
  const char* kpCharBuffer;

  (void)pDrvCtrl;

  if (pHandle != (void*)-1)
  {
    kpCharBuffer = kpBuffer;

    /* Output each character of the string */
    coutSave = count;
    while (kpCharBuffer != NULL && *kpCharBuffer != 0 && count > 0)
    {
      _ProcessChar(*kpCharBuffer);
      ++kpCharBuffer;
      --count;
    }
  }
  else
  {
    coutSave = 0;
    count = -1;
  }

  return coutSave - count;
}

static ssize_t _VFSIOCTL(void*    pDriverData,
                            void*    pHandle,
                            uint32_t operation,
                            void*    pArgs)
{
  int32_t                 retVal;
  S_IOCTLScrollArguments* pScrollArgs;

  (void)pDriverData;

  if (pHandle != (void*)-1)
  {
    /* Switch on the operation */
    retVal = 0;
    switch(operation)
    {
      case VFS_IOCTL_CONS_RESTORE_CURSOR:
        _SetCursorDirect(pArgs);
        break;
      case VFS_IOCTL_CONS_SAVE_CURSOR:
        _GetCursor(pArgs);
        break;
      case VFS_IOCTL_CONS_SCROLL:
        pScrollArgs = pArgs;
        _Scroll(pScrollArgs->direction,
                pScrollArgs->lineCount);
        break;
      case VFS_IOCTL_CONS_SET_COLORSCHEME:
        _SetScheme(pArgs);
        break;
      case VFS_IOCTL_CONS_SAVE_COLORSCHEME:
        _GetScheme(pArgs);
        break;
      case VFS_IOCTL_CONS_CLEAR:
        _ClearFramebuffer();
        break;
      case VFS_IOCTL_CONS_FLUSH:
        _Flush();
        break;
      default:
        retVal = -1;
    }
  }
  else
  {
    retVal = -1;
  }

  return retVal;
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_LAYER(sX86VGADriver);

/************************************ EOF *************************************/