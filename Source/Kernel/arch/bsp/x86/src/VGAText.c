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
#include <Panic.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <Memory.h>
#include <X64Cpu.h>
#include <Console.h>
#include <Critical.h>
#include <DeviceTree.h>
#include <KernelError.h>
#include <DriverManager.h>

/* Configuration files */
#include <config.h>

/* Header file */
#include <VGAText.h>

/* Unit test header */
/* None TODO */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Module's name */
#define MODULE_NAME "X86_VGA_TEXT"

/** @brief VGA cursor position command low. */
#define VGA_CONSOLE_CURSOR_COMM_LOW  0x0F
/** @brief VGA cursor position command high. */
#define VGA_CONSOLE_CURSOR_COMM_HIGH 0x0E


/** @brief FDT property for registers */
#define VGA_FDT_REG_PROP    "reg"
/** @brief FDT property for comm ports */
#define VGA_FDT_COMM_PROP   "comm"
/** @brief FDT property for resolution */
#define VGA_FDT_RES_PROP    "resolution"
/** @brief FDT property for device path */
#define VGA_FDT_DEVICE_PROP "device"
/** @brief FDT property for console output */
#define VGA_FDT_OUTPUT_PROP "console-output"

/** @brief Cast a pointer to a VGA driver controler */
#define GET_CONTROLER(PTR) ((S_VGAControler*)PTR)

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief x86 VGA driver controler. */
typedef struct
{
  /** @brief Screen line resolution. */
  uint8_t lineCount;
  /** @brief Screen column resolution. */
  uint8_t columnCount;
  /** @brief CPU command port. */
  uint16_t cpuCommPort;
  /** @brief CPU data port. */
  uint16_t cpuDataPort;
  /** @brief Stores the curent screen's color scheme. */
  S_ColorScheme screenScheme;
  /** @brief Stores the curent screen's cursor settings. */
  S_ConsoleCursor screenCursor;
  /** @brief VGA frame buffer address. */
  uint16_t* pFramebuffer;
  /** @brief Size in bytes of the framebuffer. */
  size_t framebufferSize;
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
  if ((COND) == false)                                     \
  {                                                       \
    PANIC(ERROR, MODULE_NAME, MSG, false);                \
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
  ((sVGADriverCtrl.pFramebuffer) +                              \
  ((COL) + (LINE) * sVGADriverCtrl.columnCount))

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Attaches the VGA driver to the system.
 *
 * @details Attaches the VGA driver to the system. This function will use the
 * FDT to initialize the VGA hardware and retreive the VGA parameters.
 *
 * @param[in] kpFdtNode The FDT node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static E_Return _Attach(const S_FDTNode* kpFdtNode);

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
 * @param[in, out] pDriverCtrl The VGA driver controler to use.
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
 * @brief Puts a character to the screen.
 *
 * @details Outputs the given character to the screen. The function will process
 * the character and take the appropriate action.
 *
 * @param[in] kCharacter The character to output.
 */
static void _PutCharacter(const char kCharacter);

/**
 * @brief Puts a string to the screen.
 *
 * @details Outputs the given string to the screen. The function will process
 * the string and take the appropriate action.
 *
 * @param[in] pkString The string to output.
 */
static void _PutString(const char* pkString);

/**
 * @brief Flushes the screen output.
 *
 * @details The function will request a flush to the screen output driver.
 */
static void _Flush(void);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief VGA driver instance. */
static S_Driver sX86VGADriver =
{
  .pName         = "X86 VGA driver",
  .pDescription  = "X86 VGA driver for roOs",
  .pCompatible   = "x86,x86-vga-text",
  .pVersion      = "2.0",
  .pDriverAttach = _Attach
};

/** @brief VGA driver controler. */
static S_VGAControler sVGADriverCtrl;

/** @brief VGA console driver controler. */
static S_ConsoleDriver sConsoleDriver =
{
  .pClear          = _ClearFramebuffer,
  .pGetCursor      = _GetCursor,
  .pSetCursor      = _SetCursorDirect,
  .pScroll         = _Scroll,
  .pSetColorScheme = _SetScheme,
  .pGetColorScheme = _GetScheme,
  .pPutString      = _PutString,
  .pPutChar        = _PutCharacter,
  .pFlush          = _Flush,
  .pRead           = NULL
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static E_Return _Attach(const S_FDTNode* kpFdtNode)
{
  const uintptr_t* ptrProp;
  const uint32_t*  kpUintProp;
  size_t           propLen;
  E_Return         retCode;
  uintptr_t        frameBufferPhysAddr;
  size_t           frameBufferPhysSize;
  void*            mappedFrameBufferAddr;

  /* Init structures */
  memset(&sVGADriverCtrl, 0, sizeof(S_VGAControler));

  /* Get the VGA framebuffer address */
  ptrProp = FDTGetProp(kpFdtNode, VGA_FDT_REG_PROP, &propLen);
  if (ptrProp != NULL && propLen == sizeof(uintptr_t) * 2)
  {
#ifdef ARCH_32_BITS
    sVGADriverCtrl.pFramebuffer    = (uint16_t*)FDTTOCPU32(*ptrProp);
    sVGADriverCtrl.framebufferSize = (size_t)FDTTOCPU32(*(ptrProp + 1));
#elif defined(ARCH_64_BITS)
    sVGADriverCtrl.pFramebuffer    = (uint16_t*)FDTTOCPU64(*ptrProp);
    sVGADriverCtrl.framebufferSize = (size_t)FDTTOCPU64(*(ptrProp + 1));
#else
    #error "Invalid architecture"
#endif

    /* Align and map framebuffer */
    frameBufferPhysAddr = (uintptr_t)sVGADriverCtrl.pFramebuffer &
                          ~PAGE_SIZE_MASK;
    frameBufferPhysSize = sVGADriverCtrl.framebufferSize;

    frameBufferPhysSize += (uintptr_t)sVGADriverCtrl.pFramebuffer -
                            frameBufferPhysAddr;
    frameBufferPhysSize = (frameBufferPhysSize + PAGE_SIZE_MASK) &
                          ~PAGE_SIZE_MASK;

    mappedFrameBufferAddr = MemoryKernelMap((void*)frameBufferPhysAddr,
                                            frameBufferPhysSize,
                                            MEMMGR_MAP_HARDWARE |
                                            MEMMGR_MAP_KERNEL   |
                                            MEMMGR_MAP_RW,
                                            &retCode);


    if (mappedFrameBufferAddr != NULL && retCode == NO_ERROR)
    {
      /* Update framebuffer address but not size even if we mapped more */
      sVGADriverCtrl.pFramebuffer = mappedFrameBufferAddr;

      /* Get the VGA CPU communication ports */
      kpUintProp = FDTGetProp(kpFdtNode, VGA_FDT_COMM_PROP, &propLen);
      if (kpUintProp != NULL && propLen == sizeof(uint32_t) * 2)
      {
        sVGADriverCtrl.cpuCommPort = (uint16_t)FDTTOCPU32(*kpUintProp);
        sVGADriverCtrl.cpuDataPort = (uint16_t)FDTTOCPU32(*(kpUintProp + 1));

        /* Get the resolution */
        kpUintProp = FDTGetProp(kpFdtNode, VGA_FDT_RES_PROP, &propLen);
        if (kpUintProp != NULL && propLen == sizeof(uint32_t) * 2)
        {
          sVGADriverCtrl.columnCount = (uint8_t)FDTTOCPU32(*kpUintProp);
          sVGADriverCtrl.lineCount   = (uint8_t)FDTTOCPU32(*(kpUintProp + 1));

          /* Set initial scheme */
          sVGADriverCtrl.screenScheme.background = BG_BLACK;
          sVGADriverCtrl.screenScheme.foreground = FG_WHITE;

          /* Set initial cursor position */
          sVGADriverCtrl.screenCursor.x = 0;
          sVGADriverCtrl.screenCursor.y = 0;

          kpUintProp = FDTGetProp(kpFdtNode, VGA_FDT_OUTPUT_PROP, &propLen);
          if (kpUintProp != NULL)
          {
            ConsoleSetDriver(&sConsoleDriver);
          }
        }
        else
        {
          retCode = MemoryKernelUnmap(mappedFrameBufferAddr,
                                      frameBufferPhysSize);
          VGA_ASSERT(retCode == NO_ERROR,
                     "Failed to unmap framebuffer",
                     retCode);
        }
      }
      else
      {
        retCode = MemoryKernelUnmap(mappedFrameBufferAddr, frameBufferPhysSize);
        VGA_ASSERT(retCode == NO_ERROR, "Failed to unmap framebuffer", retCode);
      }
    }
  }
  else
  {
    retCode = ERR_INVALID_PARAMETER;
  }

  return retCode;
}

static inline void _PrintChar(const uint32_t kLine,
                              const uint32_t kColumn,
                              const char     kCharacter)
{
    volatile uint16_t* pScreenMem;

    if ((uint8_t)kLine < sVGADriverCtrl.lineCount &&
       (uint8_t)kColumn < sVGADriverCtrl.columnCount)
    {
      /* Get address to inject */
      pScreenMem = GET_FRAME_BUFFER_AT(kLine, kColumn);

      /* Inject the character with the current colorscheme */
      *pScreenMem = kCharacter |
                    ((sVGADriverCtrl.screenScheme.background << 8) & 0xF000) |
                    ((sVGADriverCtrl.screenScheme.foreground << 8) & 0x0F00);
    }
}

static void _ProcessChar(const char kCharacter)
{
  /* If character is a normal ASCII character */
  if (kCharacter > 31 && kCharacter < 127)
  {
    /* Manage end of line cursor position */
    if ((uint8_t)sVGADriverCtrl.screenCursor.x > sVGADriverCtrl.columnCount - 1)
    {
      _SetCursor(sVGADriverCtrl.screenCursor.y + 1, 0);
    }

    /* Manage end of screen cursor position */
    if ((uint8_t)sVGADriverCtrl.screenCursor.y >= sVGADriverCtrl.lineCount)
    {
      _Scroll(SCROLL_DOWN, 1);
    }
    else
    {
      /* Move cursor */
      _SetCursor(sVGADriverCtrl.screenCursor.y, sVGADriverCtrl.screenCursor.x);
    }

    /* Display character and move cursor */
    _PrintChar(sVGADriverCtrl.screenCursor.y,
               sVGADriverCtrl.screenCursor.x++,
               kCharacter);
  }
  else
  {
    /* Manage special ACSII characters*/
    switch(kCharacter)
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
                       sVGADriverCtrl.columnCount - 1);
          }
          break;
      /* Tab */
      case '\t':
        if ((uint8_t)sVGADriverCtrl.screenCursor.x + 4 <
            sVGADriverCtrl.columnCount - 1)
        {
          _SetCursor(sVGADriverCtrl.screenCursor.y,
                     sVGADriverCtrl.screenCursor.x  +
                     (4 - sVGADriverCtrl.screenCursor.x % 4));
        }
        else
        {
          _SetCursor(sVGADriverCtrl.screenCursor.y,
                     sVGADriverCtrl.columnCount - 1);
        }
        break;
      /* Line feed */
      case '\n':
        if ((uint8_t)sVGADriverCtrl.screenCursor.y <
           sVGADriverCtrl.lineCount - 1)
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
        memset(sVGADriverCtrl.pFramebuffer, 0, sVGADriverCtrl.framebufferSize);
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
  /* Clear all screen */
  memset(sVGADriverCtrl.pFramebuffer, 0, sVGADriverCtrl.framebufferSize);
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
  uint16_t cursorPosition;

  /* Checks the values of line and column */
  if (kLine < sVGADriverCtrl.lineCount && kColumn < sVGADriverCtrl.columnCount)
  {
    /* Set new cursor position */
    sVGADriverCtrl.screenCursor.x = kColumn;
    sVGADriverCtrl.screenCursor.y = kLine;

    /* Display new position on screen */
    cursorPosition = kColumn + kLine * sVGADriverCtrl.columnCount;

    /* Send low part to the screen */
    CPUPortWriteByte(VGA_CONSOLE_CURSOR_COMM_LOW, sVGADriverCtrl.cpuCommPort);
    CPUPortWriteByte((int8_t)(cursorPosition & 0x00FF),
                     sVGADriverCtrl.cpuDataPort);

    /* Send high part to the screen */
    CPUPortWriteByte(VGA_CONSOLE_CURSOR_COMM_HIGH, sVGADriverCtrl.cpuCommPort);
    CPUPortWriteByte((int8_t)((cursorPosition & 0xFF00) >> 8),
                     sVGADriverCtrl.cpuDataPort);
  }
}

static void _Scroll(const E_ScrollDirection kDirection, const uint32_t kLines)
{
  uint8_t toScroll;
  uint8_t i;
  uint8_t j;

  if (sVGADriverCtrl.lineCount < kLines)
  {
    toScroll = sVGADriverCtrl.lineCount;
  }
  else
  {
    toScroll = kLines;
  }

  /* Select scroll direction */
  if (kDirection == SCROLL_DOWN)
  {
    /* For each line scroll we want */
    for (j = 0; j < toScroll; ++j)
    {
      /* Copy all the lines to the above one */
      for (i = 0; i < sVGADriverCtrl.lineCount - 1; ++i)
      {
          memmove(GET_FRAME_BUFFER_AT(i, 0),
                  GET_FRAME_BUFFER_AT(i + 1, 0),
                  sizeof(uint16_t) * sVGADriverCtrl.columnCount);
      }
    }
    /* Clear last line */
    for (i = 0; i < sVGADriverCtrl.columnCount; ++i)
    {
      _PrintChar(sVGADriverCtrl.lineCount - 1, i, ' ');
    }

    /* Replace cursor */
    _SetCursor(sVGADriverCtrl.lineCount - toScroll, 0);
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

static void _PutString(const char* kpBuffer)
{
  /* Output each character of the string */
  while (*kpBuffer)
  {
    _ProcessChar(*kpBuffer);
    ++kpBuffer;
  }
}

static void _PutCharacter(const char kCharacter)
{
  _ProcessChar(kCharacter);
}

static void _Flush(void)
{
  /* Nothing to do for VGA */
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_FDT(sX86VGADriver);

/************************************ EOF *************************************/