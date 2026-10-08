/*******************************************************************************
 * @file VESA.c
 *
 * @see VESA.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 04/10/2026
 *
 * @version 3.0
 *
 * @brief VESA VBE 2 graphic driver.
 *
 * @details VESA VBE 2 graphic drivers. Allows the kernel to have a generic high
 * resolution output. The driver provides regular console output management and
 * generic screen drawing functions.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/
/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <CPU.h>
#include <CPUID.h>
#include <Panic.h>
#include <IOCTL.h>
#include <X64Cpu.h>
#include <stdint.h>
#include <Memory.h>
#include <Console.h>
#include <VirtualFS.h>
#include <Scheduler.h>
#include <KernelHeap.h>
#include <DeviceTree.h>
#include <KernelError.h>
#include <KernelOutput.h>
#include <DriverManager.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* No unit test: this module is tested in real-world conditions. */

/* Header file */
#include <VESA.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Current module name */
#define MODULE_NAME "X86 VESA"

/** @brief VESA FDT property: Screen resolution. */
#define VESA_FDT_RESOLUTION_PROP "resolution"
/** @brief VESA FDT property: Screen depth. */
#define VESA_FDT_DEPTH_PROP "depth"
/** @brief VESA FDT property: Screen refresh rate. */
#define VESA_FDT_REFRESH_RATE_PROP "refresh-rate"
/** @brief VESA FDT property: Device name. */
#define VESA_FDT_DEVICE_PROP "device"

/** @brief Defines the VESA BIOS call interrupt */
#define VESA_BIOS_CALL_INT 0x10

/** @brief Defines the VESA BIOS call get info function */
#define VESA_BIOS_CALL_GET_INFO_ID 0x4F00
/** @brief Defines the VESA BIOS call get mode info function */
#define VESA_BIOS_CALL_GET_MODE_ID 0x4F01
/** @brief Defines teh VESA BIOS call set mode function */
#define VESA_BIOS_CALL_SET_MODE 0x4F02

/** @brief Defines the BIOS call return value OK */
#define VESA_BIOS_CALL_RETURN_OK 0x004F

/** @brief Defines the OEM data size */
#define VESA_OEM_DATA_SIZE 256

/** @brief VESA mode attribute flag supported */
#define VESA_ATTRIBUTE_SUPPORTED 0x1
/** @brief VESA mode attribute flag linear frame buffer */
#define VESA_ATTRIBUTE_LINEAR_FB 0x90
/** @brief VESA memory model packed */
#define VESA_MEMORY_MODEL_PACKED 0x4
/** @brief VESA memory model direct color */
#define VESA_MEMORY_MODEL_DIRECTCOLOR 0x6

/** @brief VESA mode command: enable linear framebuffer. */
#define VESA_FLAG_LINEAR_FB_ENABLE 0x4000

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Represents the memory capabilities of the VESA controller. */
typedef enum
{
  /** @brief AVX memory capabilities. */
  MEMORY_AVX_SUPPORTED,
  /** @brief SSE memory capabilities. */
  MEMORY_SSE_SUPPORTED,
  /** @brief ERMS memory capabilities. */
  MEMORY_ERMS_SUPPORTED,
} E_MemoryCapability;

/** @brief VBE information structure, see the VBE standard for more information
 * about the contained data.
 */
typedef struct
{
  /** @brief The VBE signature. */
  char pSignature[4];
  /** @brief The VBE version. */
  uint16_t version;
  /** @brief The pointer to the OEM String. */
  uint32_t oem;
  /** @brief Capabilities of the graphics controller */
  uint32_t capabilities;
  /** @brief Pointer to the video mode list */
  uint32_t videoModes;
  /** @brief Number of memory blocks */
  uint16_t totalMemory;
  /** @brief VBE software revision */
  uint16_t softwareRev;
  /** @brief Pointer to the vendore name string */
  uint32_t vendor;
  /** @brief Pointer to the product name string */
  uint32_t productName;
  /** @brief Pointer to the product revision string */
  uint32_t productRev;
  /** @brief Reserved */
  uint8_t  reserved[222];
  /** @brief Data for OEM strings */
  uint8_t  oemData[VESA_OEM_DATA_SIZE];
} __attribute__((packed)) S_VBEInformation;

/** @brief VBE mode information structure, see the VBE standard for more
 * information about the contained data.
 */
typedef struct
{
  /** @brief Mode attributes */
  uint16_t attributes;
  /** @brief Window A attributes */
  uint8_t  windowA;
  /** @brief Window B attributes */
  uint8_t  windowB;
  /** @brief Window granularity */
  uint16_t granularity;
  /** @brief Window size */
  uint16_t windowSize;
  /** @brief Window A start segment */
  uint16_t segmentA;
  /** @brief Window B start segment */
  uint16_t segmentB;
  /** @brief Pointer to window function */
  uint32_t winFuncPtr;
  /** @brief Bytes per scan line */
  uint16_t bytesPerScanLine;
  /** @brief Horizontal resolution in pixels or characters */
  uint16_t width;
  /** @brief Vertical resolution in pixels or characters */
  uint16_t height;
  /** @brief Character width in pixels */
  uint8_t  wChar;
  /** @brief Character height in pixels */
  uint8_t  yChar;
  /** @brief Number of memory planes */
  uint8_t  planes;
  /** @brief Color depth (Bits Per Pixel) */
  uint8_t  bpp;
  /** @brief Number of banks */
  uint8_t  banks;
  /** @brief Memory model type */
  uint8_t  memoryModel;
  /** @brief Bank size in KB */
  uint8_t  bankSize;
  /** @brief Number of images */
  uint8_t  imagePages;
  /** @brief Reserved */
  uint8_t  reserved0;
  /** @brief Size of direct color red mask in bits */
  uint8_t  redMask;
  /** @brief Bit position if LSB of red mask */
  uint8_t  redPosition;
  /** @brief Size of direct color green mask in bits */
  uint8_t  greenMask;
  /** @brief Bit position if LSB of green mask */
  uint8_t  greenPosition;
  /** @brief Size of direct color blue mask in bits */
  uint8_t  blueMask;
  /** @brief Bit position if LSB of blue mask */
  uint8_t  bluePosition;
  /** @brief Size of direct color reserved mask in bits */
  uint8_t  reservedMask;
  /** @brief Bit position if LSB of reserved mask */
  uint8_t  reservedPosition;
  /** @brief Direct color mode attributes */
  uint8_t  directColorAttributes;
  /** @brief Physical address of the framebuffer */
  uint32_t framebuffer;
  /** @brief Pointer to the start of the off screen memory */
  uint32_t offScreenMemOff;
  /** @brief Amount of off screen momory in 1K unit */
  uint16_t offScreenMemSize;
  /** @brief Reserved */
  uint8_t  reserved1[206];
} __attribute__((packed)) S_VBEModeInformation;

typedef struct S_VBEModeInformationNode
{
  /** @brief VESA mode ID */
  uint16_t modeId;
  /** @brief VESA mode information */
  S_VBEModeInformation modeInfo;
  /** @brief Tells if the mode is supported */
  bool isSupported;
  /** @brief Pointer to the next mode information structure */
  struct S_VBEModeInformationNode* pNextMode;
} S_VBEModeInformationNode;

/** @brief Defines the VESA controller instance. */
typedef struct
{
  /** @brief VESA controller's framebuffer address. */
  uintptr_t framebufferAddr;
  /** @brief VESA controller's framebuffer size. */
  size_t framebufferSize;
  /** @brief VESA controller's back buffer address. */
  uintptr_t backBufferAddr;
  /** @brief VESA controller's back buffer lock. */
  S_KernelSpinlock bufferLock;

  /** @brief VESA controller's screen width. */
  uint16_t screenWidth;
  /** @brief VESA controller's screen height. */
  uint16_t screenHeight;
  /** @brief VESA controller's screen pitch. */
  uint16_t screenPitch;
  /** @brief VESA controller's screen refresh rate. */
  uint16_t screenRefreshRate;
  /** @brief VESA controller's bits per pixel. */
  uint8_t bitsPerPixel;
  /** @brief VESA controller's bytes per pixel. */
  uint8_t bytesPerPixel;

  /** @brief VESA controller's screen column count. */
  uint32_t columnCount;
  /** @brief VESA controller's screen line count. */
  uint32_t lineCount;
  /** @brief VESA controller's screen color scheme. */
  S_ColorScheme screenScheme;
  /** @brief VESA controller's screen cursor position. */
  S_ConsoleCursor screenCursor;

  /** @brief VESA controller's device name. */
  const char* kpDeviceName;
  /** @brief VESA controller's VBE current mode information. */
  S_VBEModeInformation* pCurrentModeInfo;
  /** @brief VESA controller's mode lock. */
  S_KernelSpinlock modeLock;

  /** @brief VESA controller's number of available modes. */
  uint32_t modeCount;
  /** @brief VESA controller's VBE information. */
  S_VBEInformation vbeInfo;
  /** @brief VESA controller's VBE mode information list. */
  S_VBEModeInformationNode* pVBEModeInfo;

  /** @brief VESA controller's display thread. */
  S_KernelThread* pDisplayThread;

  /** @brief Stores the memory capabilities of the VESA controller. */
  E_MemoryCapability memoryCapabilities;
} S_VESAController;


/*******************************************************************************
 * MACROS
 ******************************************************************************/
/**
 * @brief Assert macro used by the VESA driver to ensure correctness of
 * execution.
 *
 * @details Assert macro used by the VESA driver to ensure correctness of
 * execution.
 * Due to the critical nature of the VESA driver, any error generates a kernel
 * panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define VESA_ASSERT(COND, MSG, ERROR) {                   \
  if ((COND) == false)                                    \
  {                                                       \
    PANIC(ERROR, MODULE_NAME, MSG, false, false);         \
  }                                                       \
}

/**
 * @brief Get the VESA frame buffer virtual address.
 *
 * @details Get the VESA frame buffer virtual address correponding to a
 * certain region of the buffer given the parameters.
 *
 * @param[in] X The frame buffer column.
 * @param[in] Y The frame buffer line.
 *
 * @return The frame buffer virtual address is get correponding to a
 * certain region of the buffer given the parameters.
 */
#define GET_FRAME_BUFFER_AT(X, Y) \
  ((sController.backBufferAddr) + \
  ((X) * sController.bytesPerPixel) + \
  ((Y) * sController.screenPitch))

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Attaches the VESA driver to the system.
 *
 * @details Attaches the VESA driver to the system. This function will use the
 * FDT to initialize the VESA hardware and retreive the VESA parameters.
 *
 * @param[in] pkFdtNode The FDT node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static E_Return _Attach(const S_FDTNode* pkFdtNode);

/**
 * @brief Reads the VESA parameters from the FDT node.
 *
 * @details Reads the VESA parameters from the FDT node. This function will read
 * the VESA parameters from the FDT node and store them in the VESA driver
 * instance.
 *
 * @param[in] pkFdtNode The FDT node with the VESA parameters.
 *
 * @return The success state or the error code.
 */
static E_Return _GetVESAParameters(const S_FDTNode* pkFdtNode);

/**
 * @brief Retreives the VESA information from the BIOS.
 *
 * @details Retreives the VESA information from the BIOS. This function will use
 * the BIOS to retreive the VESA information and store it in the VESA driver
 * instance.
 *
 * @return The success state or the error code.
 */
static E_Return _GetVESAInformation(void);

/**
 * @brief Retreives the VESA modes from the BIOS.
 *
 * @details Retreives the VESA modes from the BIOS. This function will
 * use the BIOS to retreive the VESA modes and store them in the VESA driver
 * instance.
 *
 * @return The success state or the error code.
 */
static E_Return _GetVESAModes(void);

/**
 * @brief Retreives the VESA modes information from the BIOS.
 *
 * @details Retreives the VESA modes information from the BIOS. This function
 * will use the BIOS to retreive the VESA modes information and store it in the
 * VESA driver instance.
 *
 * @return The success state or the error code.
 */
static E_Return _GetVESAModesInformation(void);

/**
 * @brief Creates the display thread for the VESA driver.
 *
 * @details Creates the display thread for the VESA driver. This function will
 * create a thread that will be responsible for updating the display.
 * The thread will be created with the highest priority and will be scheduled on
 * all available CPUs.
 *
 * @return The success state or the error code.
 */
static E_Return _CreateDisplayThread(void);

/**
 * @brief The display routine for the VESA driver.
 *
 * @details The display routine for the VESA driver. This function will be
 * executed by the display thread to update the display.
 *
 * @param[in] pArg Unused.
 *
 * @return The function should never return.
 */
static void* _DisplayRoutine(void* pArg);

/**
 * @brief Prints the cursor on the screen.
 *
 * @details Prints the cursor on the screen. This function will print the cursor
 * on the screen at the current cursor position.
 *
 * @param[in] kX The cursor X position.
 * @param[in] kY The cursor Y position.
 * @param[in] kIsVisible The cursor visibility state.
 */
static void _PrintCursor(const uint32_t kX,
                         const uint32_t kY,
                         const bool     kIsVisible);

/**
 * @brief Draws a pixel on the screen.
 *
 * @details Draws a pixel on the screen at the specified coordinates with the
 * specified color.
 *
 * @param[in] kX The x-coordinate of the pixel.
 * @param[in] kY The y-coordinate of the pixel.
 * @param[in] kColor The color of the pixel.
 */
static void _DrawPixel(const uint32_t kX,
                       const uint32_t kY,
                       const uint32_t kColor);

/**
 * @brief Draws a rectangle on the screen.
 *
 * @details Draws a rectangle on the screen at the specified coordinates with the
 * specified dimensions and color.
 *
 * @param[in] kX The x-coordinate of the rectangle.
 * @param[in] kY The y-coordinate of the rectangle.
 * @param[in] kWidth The width of the rectangle.
 * @param[in] kHeight The height of the rectangle.
 * @param[in] kColor The color of the rectangle.
 */
static void _DrawRectangle(const uint32_t kX,
                           const uint32_t kY,
                           const uint32_t kWidth,
                           const uint32_t kHeight,
                           const uint32_t kColor);

/**
 * @brief Draws a line on the screen.
 *
 * @details Draws a line on the screen at the specified coordinates with the
 * specified width and color.
 *
 * @param[in] kpData The data for the line.
 * @param kX The x-coordinate of the line.
 * @param kY The y-coordinate of the line.
 * @param kWidth The width of the line.
 */
static void _DrawLineData(const void*    kpData,
                          const uint32_t kX,
                          const uint32_t kY,
                          const uint32_t kWidth);

/**
 * @brief Draws a line on the screen.
 *
 * @details Draws a line on the screen at the specified coordinates with the
 * specified width and color.
 *
 * @param[in] kX The x-coordinate of the line.
 * @param[in] kY The y-coordinate of the line.
 * @param[in] kWidth The width of the line.
 * @param[in] kColor The color of the line.
 */
static void _DrawLine(const uint32_t kX,
                      const uint32_t kY,
                      const uint32_t kWidth,
                      const uint32_t kColor);

/**
 * @brief Draws a bitmap on the screen.
 *
 * @details Draws a bitmap on the screen at the specified coordinates with the
 * specified dimensions and data.
 *
 * @param[in] kX The x-coordinate of the bitmap.
 * @param[in] kY The y-coordinate of the bitmap.
 * @param[in] kWidth The width of the bitmap.
 * @param[in] kHeight The height of the bitmap.
 * @param[in] kpData The data for the bitmap.
 */
static void _DrawBitmap(const uint32_t kX,
                        const uint32_t kY,
                        const uint32_t kWidth,
                        const uint32_t kHeight,
                        const void*    kpData);

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
 * @param[in, out] pDriverCtrl The VESA driver controller to use.
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
 * @brief VESA fast fill function.
 *
 * @details VESA fast fill function. Using SSE instructions to speedup the
 * filling of the back buffer.
 *
 * @param[in] bufferAddr The start address of the region of the buffer to fill.
 * @param[in] kPixel The color pixel to fill.
 * @param[in] pixelCount The amount of bytes to fill in the buffer.
 */
static inline void _FastFill(uintptr_t      bufferAddr,
                             const uint32_t kPixel,
                             uint32_t       size);

/**
 * @brief VESA fast copy function.
 *
 * @details VESA fast copy function. Using SSE instructions to speedup the
 * copy of the back buffer.
 *
 * @param[out] pDest The start address of the region of the buffer to copy.
 * @param[in] kpSrc The start address of the source region to copy.
 * @param[in] sizeThe The size in bytes to copy.
 */
static inline void _FastCopy(uintptr_t       pDest,
                             const uintptr_t kpSrc,
                             size_t          size);

/**
 * @brief VESA VFS open hook.
 *
 * @details VESA VFS open hook. This function returns a handle to control the
 * VESA driver through VFS.
 *
 * @param[in, out] pDrvCtrl The VESA driver that was registered in the VFS.
 * @param[in] kpPath The path in the VESA driver mount point.
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
 * @brief VESA VFS close hook.
 *
 * @details VESA VFS close hook. This function closes a handle that was created
 * when calling the open function.
 *
 * @param[in, out] pDrvCtrl The VESA driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static int32_t _VFSClose(void* pDrvCtrl, void* pHandle);

/**
 * @brief VESA VFS write hook.
 *
 * @details VESA VFS write hook. This function writes a string to the VESA
 * framebuffer.
 *
 * @param[in, out] pDrvCtrl The VESA driver that was registered in the VFS.
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
 * @brief VESA VFS IOCTL hook.
 *
 * @details VESA VFS IOCTL hook. This function performs the IOCTL for the VESA
 * driver.
 *
 * @param[in, out] pDrvCtrl The VESA driver that was registered in the VFS.
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
/** @brief VESA controller instance. */
static S_VESAController sController;

/** @brief VESA driver instance. */
static S_Driver sX86VESADriver =
{
  .pName         = "X86 VESA Driver",
  .pDescription  = "X86 VESA VBE 2 graphic driver for roOs",
  .pCompatible   = "x86,x86-vesa",
  .pVersion      = "1.0",
  .pDriverAttach = _Attach
};

/** @brief VGA color to RGB translation table. */
static const uint32_t skVGAColorTable[16] =
{
  0xFF000000,
  0xFF0000AA,
  0xFF00AA00,
  0xFF00AAAA,
  0xFFAA0000,
  0xFFAA00AA,
  0xFFAA5500,
  0xFFAAAAAA,
  0xFF555555,
  0xFF5555FF,
  0xFF55FF55,
  0xFF55FFFF,
  0xFFFF5555,
  0xFFFF55FF,
  0xFFFFFF55,
  0xFFFFFFFF
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static E_Return _GetVESAParameters(const S_FDTNode* pkFdtNode)
{
  const uint32_t* kpUintProp;
  const char*     kpStrProp;
  size_t          propLen;
  E_Return        retVal;

  retVal = NO_ERROR;

  kpUintProp = FDTGetProp(pkFdtNode, VESA_FDT_RESOLUTION_PROP, &propLen);
  if (kpUintProp == NULL || propLen != sizeof(uint32_t) * 2)
  {
    retVal = ERR_INVALID_VALUE;
  }

  if (retVal == NO_ERROR)
  {
    sController.screenWidth  = (uint16_t)FDTTOCPU32(*kpUintProp);
    sController.screenHeight = (uint16_t)FDTTOCPU32(*(kpUintProp + 1));

    kpUintProp = FDTGetProp(pkFdtNode, VESA_FDT_DEPTH_PROP, &propLen);
    if (kpUintProp == NULL || propLen != sizeof(uint32_t))
    {
      retVal = ERR_INVALID_VALUE;
    }
  }

  if (retVal == NO_ERROR)
  {
    sController.bitsPerPixel = (uint8_t)FDTTOCPU32(*kpUintProp);

    kpUintProp = FDTGetProp(pkFdtNode, VESA_FDT_REFRESH_RATE_PROP, &propLen);
    if (kpUintProp == NULL || propLen != sizeof(uint32_t))
    {
      retVal = ERR_INVALID_VALUE;
    }
  }

  if (retVal == NO_ERROR)
  {
    sController.screenRefreshRate = (uint16_t)FDTTOCPU32(*kpUintProp);

    kpStrProp = FDTGetProp(pkFdtNode, VESA_FDT_DEVICE_PROP, &propLen);
    if (kpStrProp == NULL || propLen  == 0)
    {
      retVal = ERR_INVALID_VALUE;
    }
    else
    {
      sController.kpDeviceName = kpStrProp;
    }
  }

  return retVal;
}

static E_Return _GetVESAInformation(void)
{
  E_Return        retVal;
  S_BIOSRegisters biosRegs;
  uintptr_t       initialLocation;
  uint8_t*        pOEMData;
  size_t          toMap;
  uintptr_t       addrToMap;
  uintptr_t       offset;

  /* Initialize the VESA information registers for the BIOS. */
  biosRegs.ax = VESA_BIOS_CALL_GET_INFO_ID;
  biosRegs.bx = 0;
  biosRegs.cx = 0;
  biosRegs.dx = 0;

  /* Initialize the VBE information. */
  memset(&sController.vbeInfo, 0, sizeof(S_VBEInformation));
  memcpy(&sController.vbeInfo.pSignature, "VBE2", 4);
  sController.vbeInfo.version = 0x0200;

  /* Call the BIOS to get the VESA information. */
  CPUBIOSCall(&biosRegs,
              VESA_BIOS_CALL_INT,
              &sController.vbeInfo,
              sizeof(S_VBEInformation),
              &initialLocation);

  /* Check the result from the call. */
  if (biosRegs.ax == VESA_BIOS_CALL_RETURN_OK)
  {
    /* Check the VBE information. */
    if (sController.vbeInfo.version >= 0x200 &&
        strncmp(sController.vbeInfo.pSignature, "VESA", 4) == 0)
    {
      /* Get the OEM Data */
      toMap     = KERNEL_PAGE_SIZE;
      addrToMap = ((sController.vbeInfo.oem >> 16) << 4) |
                   (sController.vbeInfo.oem & 0xFFFF);
      if(((addrToMap + VESA_OEM_DATA_SIZE) & ~PAGE_SIZE_MASK) !=
        (addrToMap & ~PAGE_SIZE_MASK))
      {
          toMap += KERNEL_PAGE_SIZE;
      }
      addrToMap = addrToMap & ~PAGE_SIZE_MASK;

      pOEMData = MemoryKernelMap((void*)addrToMap,
                                 toMap,
                                 MEMMGR_MAP_RO       |
                                 MEMMGR_MAP_KERNEL   |
                                 MEMMGR_MAP_HARDWARE,
                                 &retVal);
      if(retVal == NO_ERROR && pOEMData != NULL)
      {
        /* Copy the data to the OEM data */
        offset = sController.vbeInfo.oem & PAGE_SIZE_MASK;
        memcpy(sController.vbeInfo.oemData,
               pOEMData + offset,
               VESA_OEM_DATA_SIZE);

        retVal = MemoryKernelUnmap(pOEMData, toMap);
        VESA_ASSERT(retVal == NO_ERROR,
                    "Failed to unmap OEM data",
                    retVal);

        /* Update pointer to offsets */
        sController.vbeInfo.productRev -= sController.vbeInfo.oem;
        sController.vbeInfo.productName -= sController.vbeInfo.oem;
        sController.vbeInfo.vendor -= sController.vbeInfo.oem;

        // KERNEL_DEBUG(VESA_DERIVER_DEBUG_ENABLED,
        //              MODULE_NAME,
        //              "\nVESA Get Info Table (initial location 0x%x):\n"
        //              "\tSignature: %c%c%c%c\n"
        //              "\tVersion: 0x%x\n"
        //              "\tOEM: %s\n"
        //              "\tCapabilities: 0x%x\n"
        //              "\tVideo Modes Ptr: 0x%p\n"
        //              "\tEOM Ptr: 0x%p\n"
        //              "\tTotal Memory: 0x%x\n"
        //              "\tSoftware Rev.: %d\n"
        //              "\tVendor: %s\n"
        //              "\tProduct Name: %s\n"
        //              "\tProduct Rev.: %s\n",
        //              initialLocation,
        //              sController.vbeInfo.pSignature[0],
        //              sController.vbeInfo.pSignature[1],
        //              sController.vbeInfo.pSignature[2],
        //              sController.vbeInfo.pSignature[3],
        //              sController.vbeInfo.version,
        //              &sController.vbeInfo.oemData[0],
        //              sController.vbeInfo.capabilities,
        //              sController.vbeInfo.videoModes,
        //              sController.vbeInfo.oem,
        //              sController.vbeInfo.totalMemory,
        //              sController.vbeInfo.softwareRev,
        //              &sController.vbeInfo.oemData[sController.vbeInfo.vendor],
        //              &sController.vbeInfo.oemData[sController.vbeInfo.productName],
        //              &sController.vbeInfo.oemData[sController.vbeInfo.productRev]);
      }
    }
    else
    {
      KERNEL_DEBUG(VESA_DERIVER_DEBUG_ENABLED,
                   MODULE_NAME,
                   "Incompatible VBE version: 0x%X | %c%c%c%c",
                   sController.vbeInfo.version,
                   sController.vbeInfo.pSignature[0],
                   sController.vbeInfo.pSignature[1],
                   sController.vbeInfo.pSignature[2],
                   sController.vbeInfo.pSignature[3]);
      retVal = ERR_NOT_SUPPORTED;
    }
  }
  else
  {
    KERNEL_DEBUG(VESA_DERIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "VESA BIOS call failed with error code: 0x%X",
                 biosRegs.ax);
    retVal = ERR_NOT_SUPPORTED;
  }

  return retVal;
}

static E_Return _GetVESAModes(void)
{
  E_Return                  retVal;
  uint16_t*                 pModeId;
  uintptr_t                 offset;
  void*                     mapped;
  uint16_t                  currentMode;
  uintptr_t                 physAddr;
  S_VBEModeInformationNode* pModeInfo;

  sController.modeCount        = 0;
  sController.pVBEModeInfo     = NULL;
  sController.pCurrentModeInfo = NULL;

  physAddr =((sController.vbeInfo.videoModes & 0xFFFF0000) >> 12) +
            (sController.vbeInfo.videoModes & 0xFFFF);
  offset = physAddr & PAGE_SIZE_MASK;

  physAddr = ALIGN_DOWN(physAddr, KERNEL_PAGE_SIZE);

  /* Map one page and parse the modes */
  mapped = MemoryKernelMap((void*)physAddr,
                           KERNEL_PAGE_SIZE,
                           MEMMGR_MAP_RO       |
                           MEMMGR_MAP_KERNEL   |
                           MEMMGR_MAP_HARDWARE,
                           &retVal);
  if (retVal == NO_ERROR && mapped != NULL)
  {
    do
    {
      pModeId = (uint16_t*)((uintptr_t)mapped + offset);
      if (offset + sizeof(uint16_t) > KERNEL_PAGE_SIZE)
      {
        if (offset < KERNEL_PAGE_SIZE)
        {
          /* Get the first byte */
          currentMode = ((uint8_t)*pModeId) << 8;
        }

        /* Unmap the page */
        retVal = MemoryKernelUnmap(mapped, KERNEL_PAGE_SIZE);
        VESA_ASSERT(retVal == NO_ERROR,
                    "Failed to unmap VESA mode list",
                    retVal);

        /* Map the next page */
        physAddr += KERNEL_PAGE_SIZE;
        mapped = MemoryKernelMap((void*)physAddr,
                                 KERNEL_PAGE_SIZE,
                                 MEMMGR_MAP_RO       |
                                 MEMMGR_MAP_KERNEL   |
                                 MEMMGR_MAP_HARDWARE,
                                 &retVal);
        VESA_ASSERT(retVal == NO_ERROR,
                    "Failed to map VESA mode list",
                    retVal);

        /* Get the second byte */
        if (offset < KERNEL_PAGE_SIZE)
        {
          currentMode |= *((uint8_t*)mapped);
          offset = 1;
        }
        else
        {
          currentMode = *((uint16_t*)mapped);
          offset = 2;
        }
      }
      else
      {
        currentMode = *pModeId;
        offset += 2;
      }

      if (currentMode != 0xFFFF)
      {
        /* Create the mode information node and link */
        pModeInfo = KMalloc(sizeof(S_VBEModeInformationNode), KMALLOC_NO_FREE_POOL);
        pModeInfo->modeId = currentMode;
        pModeInfo->pNextMode = sController.pVBEModeInfo;
        pModeInfo->isSupported = false;
        sController.pVBEModeInfo = pModeInfo;

        ++sController.modeCount;
        KERNEL_DEBUG(VESA_DERIVER_DEBUG_ENABLED,
                     MODULE_NAME,
                     "VESA Mode: 0x%X",
                     currentMode);
      }
    } while (currentMode != 0xFFFF);

    /* Unmap the page */
    retVal = MemoryKernelUnmap(mapped, KERNEL_PAGE_SIZE);
    VESA_ASSERT(retVal == NO_ERROR,
                "Failed to unmap VESA mode list",
                retVal);

  }

  KERNEL_DEBUG(VESA_DERIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "VESA Modes: %d",
               sController.modeCount);

  return retVal;
}

static E_Return _GetVESAModesInformation(void)
{
  E_Return                  retVal;
  S_BIOSRegisters           biosRegs;
  S_VBEModeInformationNode* pModeInfo;
  uintptr_t                 initialLocation;

  pModeInfo = sController.pVBEModeInfo;
  retVal    = NO_ERROR;
  while (pModeInfo != NULL)
  {
    KERNEL_DEBUG(VESA_DERIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Getting VESA mode information for mode: 0x%X",
                 pModeInfo->modeId);

    /* Perform the bios call */
    biosRegs.ax = VESA_BIOS_CALL_GET_MODE_ID;
    biosRegs.bx = 0;
    biosRegs.cx = pModeInfo->modeId;
    biosRegs.dx = 0;
    memset(&pModeInfo->modeInfo, 0, sizeof(S_VBEModeInformation));
    CPUBIOSCall(&biosRegs,
                VESA_BIOS_CALL_INT,
                &pModeInfo->modeInfo,
                sizeof(S_VBEModeInformation),
                &initialLocation);

    /* Check the result from the call. */
    if (biosRegs.ax == VESA_BIOS_CALL_RETURN_OK)
    {
      /* Check for support */
      if ((pModeInfo->modeInfo.attributes & VESA_ATTRIBUTE_SUPPORTED) != 0)
      {
        /* We only support linear buffer now */
        if(((pModeInfo->modeInfo.attributes & VESA_ATTRIBUTE_LINEAR_FB) ==
           VESA_ATTRIBUTE_LINEAR_FB) &&
           pModeInfo->modeInfo.framebuffer != 0 &&
           (pModeInfo->modeInfo.memoryModel == VESA_MEMORY_MODEL_PACKED ||
            pModeInfo->modeInfo.memoryModel == VESA_MEMORY_MODEL_DIRECTCOLOR) &&
           (pModeInfo->modeInfo.bpp == 32))
        {
          pModeInfo->isSupported = true;

          KERNEL_DEBUG(VESA_DERIVER_DEBUG_ENABLED,
                      MODULE_NAME,
                      "VESA mode 0x%X information:\n"
                      "\tWidth: %d\n"
                      "\tHeight: %d\n"
                      "\tBPP: %d\n"
                      "\tPitch: %d\n"
                      "\tFramebuffer: 0x%X\n",
                      pModeInfo->modeId,
                      pModeInfo->modeInfo.width,
                      pModeInfo->modeInfo.height,
                      pModeInfo->modeInfo.bpp,
                      pModeInfo->modeInfo.bytesPerScanLine,
                      pModeInfo->modeInfo.framebuffer);

        }
        else
        {
          KERNEL_DEBUG(VESA_DERIVER_DEBUG_ENABLED,
                       MODULE_NAME,
                       "VESA mode 0x%X is not supported",
                       pModeInfo->modeId);
        }
      }
      else
      {
        KERNEL_DEBUG(VESA_DERIVER_DEBUG_ENABLED,
                     MODULE_NAME,
                     "VESA mode 0x%X is not supported",
                     pModeInfo->modeId);
      }
    }
    else
    {
      KERNEL_DEBUG(VESA_DERIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "VESA BIOS call failed with error code: 0x%X",
                 biosRegs.ax);
      retVal = ERR_NOT_SUPPORTED;
    }

    if (retVal != NO_ERROR)
    {
      break;
    }

    pModeInfo = pModeInfo->pNextMode;
  }

  return retVal;
}

static E_Return _SetVESAMode(const uint16_t kWidth,
                             const uint16_t kHeight,
                             const uint8_t  kBpp,
                             const uint16_t kRefreshRate)
{
  E_Return                  retVal;
  E_Return                  internalError;
  S_BIOSRegisters           biosRegs;
  S_VBEModeInformationNode* pModeInfo;
  void*                     mapped;
  uintptr_t                 initialLocation;
  uintptr_t                 oldBufferAddr;
  uintptr_t                 oldBufferSize;
  uintptr_t                 oldBackBufferAddr;
  size_t                    alignedSize;
  size_t                    newSize;
  uint16_t                  refreshRate;

  retVal = ERR_NOT_FOUND;

  /* TODO: Synchronize with display thread */


  if (kRefreshRate == 0)
  {
    refreshRate = 60;
  }
  else
  {
    refreshRate = kRefreshRate;
  }

  pModeInfo = sController.pVBEModeInfo;
  while (pModeInfo != NULL)
  {
    if (pModeInfo->isSupported == true &&
        pModeInfo->modeInfo.width == kWidth &&
        pModeInfo->modeInfo.height == kHeight &&
        pModeInfo->modeInfo.bpp == kBpp)
    {
      KERNEL_LOCK(sController.modeLock);
      /* Perform the bios call */
      biosRegs.ax = VESA_BIOS_CALL_SET_MODE;
      biosRegs.bx = VESA_FLAG_LINEAR_FB_ENABLE | pModeInfo->modeId;
      biosRegs.cx = 0;
      biosRegs.dx = 0;
      CPUBIOSCall(&biosRegs, VESA_BIOS_CALL_INT, NULL, 0, &initialLocation);

      /* Check the result from the call. */
      if (biosRegs.ax == VESA_BIOS_CALL_RETURN_OK)
      {
        /* Map the framebuffer */
        oldBufferAddr = sController.framebufferAddr;
        oldBufferSize = sController.framebufferSize;
        oldBackBufferAddr = sController.backBufferAddr;

        newSize = pModeInfo->modeInfo.bytesPerScanLine *
                  pModeInfo->modeInfo.height;
        alignedSize = ALIGN_UP(newSize, KERNEL_PAGE_SIZE);
        mapped = MemoryKernelMap((void*)(uintptr_t)pModeInfo->modeInfo.framebuffer,
                                 alignedSize,
                                 MEMMGR_MAP_RW       |
                                 MEMMGR_MAP_KERNEL   |
                                 MEMMGR_MAP_HARDWARE |
                                 MEMMGR_MAP_WRITE_COMBINING,
                                 &retVal);
        if (retVal == NO_ERROR && mapped != NULL)
        {
          sController.backBufferAddr = (uintptr_t)MemoryKernelAllocate(alignedSize,
                                                            MEMMGR_MAP_KERNEL |
                                                            MEMMGR_MAP_RW,
                                                            &retVal);
          if (retVal == NO_ERROR && sController.backBufferAddr != (uintptr_t)NULL)
          {

            /* Unmap the old framebuffer if it was mapped */
            if (sController.pCurrentModeInfo != NULL)
            {
              alignedSize = ALIGN_UP(oldBufferSize, KERNEL_PAGE_SIZE);
              retVal = MemoryKernelUnmap((void*)oldBufferAddr, alignedSize);
              VESA_ASSERT(retVal == NO_ERROR,
                          "Failed to unmap old framebuffer",
                          retVal);
              retVal = MemoryKernelFree((void*)oldBackBufferAddr, alignedSize);
              VESA_ASSERT(retVal == NO_ERROR,
                          "Failed to free old backbuffer",
                          retVal);
            }

            sController.framebufferAddr   = (uintptr_t)mapped;
            sController.framebufferSize   = newSize;
            sController.screenWidth       = pModeInfo->modeInfo.width;
            sController.screenHeight      = pModeInfo->modeInfo.height;
            sController.screenPitch       = pModeInfo->modeInfo.bytesPerScanLine;
            sController.screenRefreshRate = refreshRate;
            sController.bitsPerPixel      = pModeInfo->modeInfo.bpp;
            sController.bytesPerPixel     = sController.bitsPerPixel / 8;
            sController.pCurrentModeInfo  = &pModeInfo->modeInfo;
            sController.lineCount         = sController.screenHeight /
                                            VESA_TEXT_CHAR_HEIGHT;
            sController.columnCount       = sController.screenWidth /
                                            VESA_TEXT_CHAR_WIDTH;
            sController.screenCursor.x    = 0;
            sController.screenCursor.y    = 0;

            sController.screenScheme.background = BG_BLACK;
            sController.screenScheme.foreground = FG_WHITE;

            KERNEL_SPINLOCK_INIT(sController.bufferLock);

            KERNEL_DEBUG(VESA_DERIVER_DEBUG_ENABLED,
                        MODULE_NAME,
                        "VESA mode set to: %dx%d@%dHz %dbpp",
                        kWidth,
                        kHeight,
                        refreshRate,
                        kBpp);
            KERNEL_UNLOCK(sController.modeLock);
            break;
          }
          else
          {
            internalError = MemoryKernelUnmap(mapped, alignedSize);
            VESA_ASSERT(internalError == NO_ERROR,
                        "Failed to unmap old framebuffer",
                        internalError);

          }
        }
        else
        {
          KERNEL_DEBUG(VESA_DERIVER_DEBUG_ENABLED,
                       MODULE_NAME,
                       "Failed to map framebuffer at 0x%X with size 0x%X",
                       sController.framebufferAddr,
                       alignedSize);
        }
      }
      else
      {
        KERNEL_DEBUG(VESA_DERIVER_DEBUG_ENABLED,
                     MODULE_NAME,
                     "VESA BIOS call failed with error code: 0x%X",
                     biosRegs.ax);
        retVal = ERR_NOT_SUPPORTED;
      }

      KERNEL_UNLOCK(sController.modeLock);
      break;
    }

    pModeInfo = pModeInfo->pNextMode;
  }

  if (pModeInfo == NULL)
  {
    KERNEL_DEBUG(VESA_DERIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "VESA mode %dx%d@%dHz %dbpp not found",
                 kWidth,
                 kHeight,
                 kRefreshRate,
                 kBpp);
    retVal = ERR_NOT_SUPPORTED;
  }



  return retVal;
}

static E_Return _CreateDisplayThread(void)
{
  E_Return   retVal;
  const char threadName[THREAD_NAME_MAX_LENGTH] = "VESA-Display\0";
  S_CPUMask  cpuMask;
  uint32_t   i;

  CPU_MASK_RESET(cpuMask);
  for (i = 0; i < 1; ++i)
  {
    CPU_MASK_SET(cpuMask, i);
  }

  retVal = CreateThread(&sController.pDisplayThread,
                        true,
                        0,
                        threadName,
                        0x1000,
                        cpuMask,
                        _DisplayRoutine,
                        NULL,
                        NULL);

  return retVal;
}

static E_Return _Attach(const S_FDTNode* pkFdtNode)
{
  E_Return              retVal;
  T_VFSDriver           vfsDriver;
  const S_CPUFlagsInfo* pkCPUFlags;

  /* Get the VESA parameters from the FDT node. */
  retVal = _GetVESAParameters(pkFdtNode);
  if (retVal == NO_ERROR)
  {
    /* Get the memory capabilities. */
    pkCPUFlags = CPUIDGetFlags();
    if (pkCPUFlags != NULL)
    {
      if(pkCPUFlags->avx == true && pkCPUFlags->osxsave == true)
      {
        sController.memoryCapabilities = MEMORY_AVX_SUPPORTED;
      }
      else if(pkCPUFlags->erms == true)
      {
        sController.memoryCapabilities = MEMORY_ERMS_SUPPORTED;
      }
      else
      {
        sController.memoryCapabilities = MEMORY_SSE_SUPPORTED;
      }
    }
    else
    {
      sController.memoryCapabilities = MEMORY_SSE_SUPPORTED;
    }

    KERNEL_SPINLOCK_INIT(sController.modeLock);

    /* Get the VBE information from the BIOS. */
    retVal = _GetVESAInformation();
    if (retVal == NO_ERROR)
    {
      /* Get the supported VESA modes. */
      retVal = _GetVESAModes();
      if (retVal == NO_ERROR)
      {
        /* Get the supported VESA mode information. */
        retVal = _GetVESAModesInformation();
        if (retVal == NO_ERROR)
        {
          /* Set the VESA mode. */
          retVal = _SetVESAMode(sController.screenWidth,
                                sController.screenHeight,
                                sController.bitsPerPixel,
                                sController.screenRefreshRate);
          if (retVal == NO_ERROR)
          {
            /* Create the display thread */
            retVal = _CreateDisplayThread();
            if (retVal == NO_ERROR)
            {
              /* Register the driver */
              vfsDriver = RegisterVFSDriver(sController.kpDeviceName,
                                            NULL,
                                            _VFSOpen,
                                            _VFSClose,
                                            NULL,
                                            _VFSWrite,
                                            NULL,
                                            _VFSIOCTL);
              if (vfsDriver == VFS_DRIVER_INVALID)
              {
                retVal = ERR_INVALID_VALUE;
              }
            }
          }
        }
      }
    }
  }
  return retVal;
}

#include <TimerManager.h>
static void* _DisplayRoutine(void* pArg)
{
  E_Return retCode;
  uint64_t startTime;
  uint64_t endTime;
  uint64_t period;
  uint64_t cursorToggleTime;
  bool     cursorVisible;

  (void)pArg;

  cursorVisible    = false;
  cursorToggleTime = TimeGetUptime() + 500000000;

  while (true)
  {

    /* TODO: Used VBE3 Synchronization to ensure vertical sync*/
    startTime = TimeGetUptime();
    if (cursorToggleTime < startTime)
    {
      cursorVisible = !cursorVisible;
      cursorToggleTime = startTime + 500000000;
    }
    _PrintCursor(sController.screenCursor.x,
                 sController.screenCursor.y,
                 cursorVisible);
    _Flush();
    endTime = TimeGetUptime();
    period = 1000000000 / sController.screenRefreshRate;
    if (period < (endTime - startTime))
    {
      period = 0;
    }
    else
    {
      period -= (endTime - startTime);
    }
    retCode = SleepNs(period, NULL);
    if (retCode != NO_ERROR)
    {
      KERNEL_ERROR("VESA Display Driver failed to sleep: %d\n", retCode);
    }
  }

  return NULL;
}

static void _PrintCursor(const uint32_t kX,
                         const uint32_t kY,
                         const bool     kIsVisible)
{
  if (kIsVisible == false)
  {
    _DrawRectangle(kX * VESA_TEXT_CHAR_WIDTH,
                   kY * VESA_TEXT_CHAR_HEIGHT + VESA_TEXT_CHAR_HEIGHT - 3,
                   VESA_TEXT_CHAR_WIDTH,
                   3,
                   skVGAColorTable[sController.screenScheme.background >> 8]);
  }
  else
  {
    _DrawRectangle(kX * VESA_TEXT_CHAR_WIDTH,
                   kY * VESA_TEXT_CHAR_HEIGHT + VESA_TEXT_CHAR_HEIGHT - 3,
                   VESA_TEXT_CHAR_WIDTH,
                   3,
                   skVGAColorTable[sController.screenScheme.foreground]);
  }
}

static void _DrawPixel(const uint32_t kX,
                       const uint32_t kY,
                       const uint32_t kColor)
{
  uint32_t* pPixel;

  if (kX < sController.screenWidth && kY < sController.screenHeight)
  {
    pPixel = (uint32_t*)GET_FRAME_BUFFER_AT(kX, kY);
    *pPixel = kColor;
  }
}

static void _DrawRectangle(const uint32_t kX,
                           const uint32_t kY,
                           const uint32_t kWidth,
                           const uint32_t kHeight,
                           const uint32_t kColor)
{
  uint32_t i;

  if (kX + kWidth < sController.screenWidth &&
      kY + kHeight < sController.screenHeight)
  {
    for (i = 0; i < kHeight; ++i)
    {
      _DrawLine(kX, kY + i, kWidth, kColor);
    }
  }
}

static void _DrawLineData(const void*    kpData,
                          const uint32_t kX,
                          const uint32_t kY,
                          const uint32_t kWidth)
{
  uintptr_t pDest;
  if (kX + kWidth < sController.screenWidth && kY < sController.screenHeight)
  {
    pDest = GET_FRAME_BUFFER_AT(kX, kY);

    _FastCopy(pDest, (uintptr_t)kpData, kWidth * sizeof(uint32_t));
  }
}

static void _DrawLine(const uint32_t kX,
                      const uint32_t kY,
                      const uint32_t kWidth,
                      const uint32_t kColor)
{
  uintptr_t pDest;
  if (kX + kWidth < sController.screenWidth && kY < sController.screenHeight)
  {
    pDest = GET_FRAME_BUFFER_AT(kX, kY);

    _FastFill(pDest, kColor, kWidth * sizeof(uint32_t));
  }
}

static void _DrawBitmap(const uint32_t kX,
                        const uint32_t kY,
                        const uint32_t kWidth,
                        const uint32_t kHeight,
                        const void*    kpData)
{
  uint32_t i;
   if (kX + kWidth < sController.screenWidth &&
       kY + kHeight < sController.screenHeight)
  {
    for (i = 0; i < kHeight; ++i)
    {
      _DrawLineData(((uint8_t*)kpData) + (i * kWidth * sizeof(uint32_t)),
                    kX,
                    kY + i,
                    kWidth);
    }
  }
}

static inline void _PrintChar(const uint32_t kLine,
                              const uint32_t kColumn,
                              const char     kCharacter)
{
  uint32_t       x;
  uint32_t       y;
  uint32_t       cx;
  uint32_t       cy;
  const uint8_t* pGlyph;
  uint32_t       glyphLine[VESA_TEXT_CHAR_WIDTH];

  x = kColumn * VESA_TEXT_CHAR_WIDTH;
  y = kLine * VESA_TEXT_CHAR_HEIGHT;

  pGlyph = sVesaFontBitmap + (kCharacter - 31) * 16;
  for(cy = 0; cy < VESA_TEXT_CHAR_HEIGHT; ++cy)
  {
    for(cx = 0; cx < VESA_TEXT_CHAR_WIDTH; ++cx)
    {
      glyphLine[VESA_TEXT_CHAR_WIDTH - cx - 1] = (pGlyph[cy] & (1 << cx)) ?
                      skVGAColorTable[sController.screenScheme.foreground] :
                      skVGAColorTable[sController.screenScheme.background >> 8];
    }

    _DrawLineData(glyphLine, x, y + cy, VESA_TEXT_CHAR_WIDTH);
  }
}

static void _ProcessChar(const char kCharacter)
{
  /* If character is a normal ASCII character */
  if (kCharacter > 31 && kCharacter < 127)
  {
    /* Manage end of line cursor position */
    if (sController.screenCursor.x > sController.columnCount - 1)
    {
      sController.screenCursor.x = 0;
      ++sController.screenCursor.y;
    }

    /* Manage end of screen cursor position */
    if (sController.screenCursor.y >= sController.lineCount)
    {
      _Scroll(SCROLL_DOWN, 1);
    }

    _PrintCursor(sController.screenCursor.x,
                 sController.screenCursor.y,
                 false);

    /* Display character and move cursor */
    _PrintChar(sController.screenCursor.y,
               sController.screenCursor.x++,
               kCharacter);
  }
  else
  {
    /* Manage special ACSII characters*/
    switch (kCharacter)
    {
      /* Backspace */
      case '\b':
          if (sController.screenCursor.x > 0)
          {
            _PrintCursor(sController.screenCursor.x,
                         sController.screenCursor.y,
                         false);

            _SetCursor(sController.screenCursor.y,
                       sController.screenCursor.x - 1);
          }
          else if (sController.screenCursor.y > 0)
          {
            _PrintCursor(sController.screenCursor.x,
                         sController.screenCursor.y,
                         false);

            _SetCursor(sController.screenCursor.y - 1,
                       sController.columnCount - 1);
          }
          break;
      /* Tab */
      case '\t':
        if (sController.screenCursor.x + 4 <
            sController.columnCount - 1)
        {
          _PrintCursor(sController.screenCursor.x,
                       sController.screenCursor.y,
                       false);
          _SetCursor(sController.screenCursor.y,
                     sController.screenCursor.x  +
                     (4 - sController.screenCursor.x % 4));
        }
        else
        {
          _PrintCursor(sController.screenCursor.x,
                       sController.screenCursor.y,
                       false);
          _SetCursor(sController.screenCursor.y,
                     sController.columnCount - 1);
        }
        break;
      /* Line feed */
      case '\n':
        if (sController.screenCursor.y < sController.lineCount - 1)
        {
          _PrintCursor(sController.screenCursor.x,
                       sController.screenCursor.y,
                       false);
          _SetCursor(sController.screenCursor.y + 1, 0);
        }
        else
        {
          _Scroll(SCROLL_DOWN, 1);
        }
          break;
      /* Clear screen */
      case '\f':
        _PrintCursor(sController.screenCursor.x,
                     sController.screenCursor.y,
                     false);
        /* Clear all screen */
        _FastFill(sController.backBufferAddr, 0, sController.framebufferSize);
        break;
      /* Line return */
      case '\r':
        _PrintCursor(sController.screenCursor.x,
                     sController.screenCursor.y,
                     false);
        _SetCursor(sController.screenCursor.y, 0);
        break;
      /* Undefined */
      default:
        break;
    }
  }
}

static void _ClearFramebuffer(void)
{
  KERNEL_LOCK(sController.bufferLock);
  _FastFill(sController.backBufferAddr, 0, sController.framebufferSize);
  KERNEL_UNLOCK(sController.bufferLock);
}

static void _GetCursor(S_ConsoleCursor* pBuffer)
{
  /* Save cursor attributes */
  pBuffer->x = sController.screenCursor.x;
  pBuffer->y = sController.screenCursor.y;
}

static void _SetCursorDirect(const S_ConsoleCursor* kpBuffer)
{
  _SetCursor(kpBuffer->y, kpBuffer->x);
}

static void _SetCursor(const uint32_t kLine, const uint32_t kColumn)
{
  /* Checks the values of line and column */
  if (kLine < sController.lineCount && kColumn < sController.columnCount)
  {
    /* Set new cursor position */
    sController.screenCursor.x = kColumn;
    sController.screenCursor.y = kLine;
  }
}

static void _Scroll(const E_ScrollDirection kDirection, const uint32_t kLines)
{
  uint8_t   toScroll;
  uint8_t   i;
  uint8_t   j;
  uintptr_t screenMem;

  if (sController.lineCount < kLines)
  {
    toScroll = sController.lineCount;
  }
  else
  {
    toScroll = kLines;
  }

  /* Select scroll direction */
  if (kDirection == SCROLL_DOWN)
  {
    KERNEL_LOCK(sController.bufferLock);
    /* For each line scroll we want */
    for (j = 0; j < toScroll; ++j)
    {
      /* Copy all the lines to the above one */
      for (i = 0; i < sController.lineCount - 1; ++i)
      {
        _FastCopy(GET_FRAME_BUFFER_AT(0, i * VESA_TEXT_CHAR_HEIGHT),
                  GET_FRAME_BUFFER_AT(0, (i + 1) * VESA_TEXT_CHAR_HEIGHT),
                  sController.screenPitch * VESA_TEXT_CHAR_HEIGHT);
      }
    }
    /* Clear last line */
    screenMem = GET_FRAME_BUFFER_AT(0,
                                    (sController.lineCount - 1) *
                                    VESA_TEXT_CHAR_HEIGHT);
    _FastFill(screenMem, 0, sController.screenPitch * VESA_TEXT_CHAR_HEIGHT);

    /* Replace cursor */
    _SetCursor(sController.lineCount - toScroll, 0);
    KERNEL_UNLOCK(sController.bufferLock);
  }
}

static void _SetScheme(const S_ColorScheme* kpColorScheme)
{
  if (kpColorScheme->foreground <= FG_WHITE &&
      kpColorScheme->background <= BG_WHITE)
  {
    sController.screenScheme.foreground = kpColorScheme->foreground;
    sController.screenScheme.background = kpColorScheme->background;
  }
}

static void _GetScheme(S_ColorScheme* pBuffer)
{
  pBuffer->foreground = sController.screenScheme.foreground;
  pBuffer->background = sController.screenScheme.background;
}

static void _Flush(void)
{
#ifndef _TESTING_FRAMEWORK_ENABLED
  size_t   iterCount;
  uint8_t* src;
  uint8_t* dst;
  size_t   remainder;
  size_t   blocks;
  size_t   i;

  KERNEL_LOCK(sController.modeLock);
  KERNEL_LOCK(sController.bufferLock);
  src = (uint8_t*)sController.backBufferAddr;
  dst = (uint8_t*)sController.framebufferAddr;

  if (sController.memoryCapabilities == MEMORY_AVX_SUPPORTED)
  {
    iterCount = sController.framebufferSize / 128;

    for (i = 0; i < iterCount; ++i)
    {
      __asm__ __volatile__ (
        "vmovaps 0(%0), %%ymm0\n\t"
        "vmovaps 32(%0), %%ymm1\n\t"
        "vmovaps 64(%0), %%ymm2\n\t"
        "vmovaps 96(%0), %%ymm3\n\t"

        "vmovntdq %%ymm0, 0(%1)\n\t"
        "vmovntdq %%ymm1, 32(%1)\n\t"
        "vmovntdq %%ymm2, 64(%1)\n\t"
        "vmovntdq %%ymm3, 96(%1)\n\t"

        "addq $128, %0\n\t"
        "addq $128, %1\n\t"
        : "+r"(src), "+r"(dst)
        :
        : "ymm0", "ymm1", "ymm2", "ymm3", "memory"
      );
    }

    remainder = sController.framebufferSize % 128;
    blocks = remainder / 32;
    for (i = 0; i < blocks; ++i)
    {
      __asm__ __volatile__ (
        "vmovaps 0(%0), %%ymm0\n\t"
        "vmovntdq %%ymm0, 0(%1)\n\t"

        "addq $32, %0\n\t"
        "addq $32, %1\n\t"
        : "+r"(src), "+r"(dst)
        :
        : "ymm0", "memory"
      );
    }

    remainder %= 32;
    while (remainder--)
    {
      *dst++ = *src++;
    }

    __asm__ __volatile__ ("sfence" ::: "memory");
  }
  else
  {
    iterCount = sController.framebufferSize / 64;

    for (i = 0; i < iterCount; ++i)
    {
      __asm__ __volatile__ (
        "movaps 0(%0), %%xmm0\n\t"
        "movaps 16(%0), %%xmm1\n\t"
        "movaps 32(%0), %%xmm2\n\t"
        "movaps 48(%0), %%xmm3\n\t"

        "movntdq %%xmm0, 0(%1)\n\t"
        "movntdq %%xmm1, 16(%1)\n\t"
        "movntdq %%xmm2, 32(%1)\n\t"
        "movntdq %%xmm3, 48(%1)\n\t"

        "addq $64, %0\n\t"
        "addq $64, %1\n\t"
        : "+r"(src), "+r"(dst)
        :
        : "xmm0", "xmm1", "xmm2", "xmm3", "memory"
      );
    }

    remainder = sController.framebufferSize % 64;
    blocks = remainder / 16;
    for (i = 0; i < blocks; ++i)
    {
      __asm__ __volatile__ (
        "movaps 0(%0), %%xmm0\n\t"
        "movntdq %%xmm0, 0(%1)\n\t"

        "addq $16, %0\n\t"
        "addq $16, %1\n\t"
        : "+r"(src), "+r"(dst)
        :
        : "xmm0", "memory"
      );
    }

    remainder %= 16;
    while (remainder--)
    {
      *dst++ = *src++;
    }

    __asm__ __volatile__ ("sfence" ::: "memory");
  }

  KERNEL_UNLOCK(sController.bufferLock);
  KERNEL_UNLOCK(sController.modeLock);
#endif
}

static inline void _FastFill(uintptr_t      bufferAddr,
                             const uint32_t kPixel,
                             uint32_t       size)
{
  uint8_t*          dst;
  size_t            remainder;
  size_t            blocks;
  size_t            i;
  size_t            iterCount;
  uint8_t           c0;
  uint8_t           c1;
  uint8_t           c2;
  uint8_t           c3;
  volatile uint32_t pixel;
  size_t            pixelBytes;
  uint32_t          packedColor[4] __attribute__((aligned(16))) =
  {
    kPixel,
    kPixel,
    kPixel,
    kPixel
  };

  dst = (uint8_t*)bufferAddr;

  if (sController.memoryCapabilities == MEMORY_AVX_SUPPORTED)
  {
    pixel = kPixel;
    __asm__ __volatile__ (
        "vbroadcastss (%0), %%ymm0"
        :
        : "r"(&pixel)
        : "ymm0"
    );

    iterCount = size / 128;
    for (i = 0; i < iterCount; ++i)
    {
      __asm__ __volatile__ (
        "vmovups %%ymm0, 0(%0)\n\t"
        "vmovups %%ymm0, 32(%0)\n\t"
        "vmovups %%ymm0, 64(%0)\n\t"
        "vmovups %%ymm0, 96(%0)\n\t"

        "addq $128, %0\n\t"
        : "+r"(dst)
        :
        : "memory"
      );
    }

    remainder = size % 128;
    blocks = remainder / 32;
    for (i = 0; i < blocks; ++i)
    {
      __asm__ __volatile__ (
        "vmovups %%ymm0, 0(%0)\n\t"

        "addq $32, %0\n\t"
        : "+r"(dst)
        :
        : "memory"
      );
    }

    remainder %= 32;
    if (remainder > 0)
    {
      c0 = (kPixel >> 0)  & 0xFF;
      c1 = (kPixel >> 8)  & 0xFF;
      c2 = (kPixel >> 16) & 0xFF;
      c3 = (kPixel >> 24) & 0xFF;

      pixelBytes = remainder;
      while (pixelBytes >= 4)
      {
        *dst++ = c0;
        *dst++ = c1;
        *dst++ = c2;
        *dst++ = c3;
        pixelBytes -= 4;
      }

      if (pixelBytes >= 1)
      {
        dst[0] = c0;
        ++dst;
        --pixelBytes;
      }
      if (pixelBytes >= 1)
      {
        dst[0] = c1;
        ++dst;
        --pixelBytes;
      }
      if (pixelBytes >= 1)
      {
        dst[0] = c2;
      }
    }
  }
  else if (sController.memoryCapabilities == MEMORY_ERMS_SUPPORTED)
  {
    blocks    = size / 4;
    remainder = size % 4;

    __asm__ __volatile__ (
        "cld\n\t"
        "rep stosl"
        : "+D"(dst), "+c"(blocks)
        : "a"(kPixel)
        : "memory"
    );

    if (remainder > 0) {
      c0 = kPixel & 0xFF;
      __asm__ __volatile__ (
        "rep stosb"
        : "+D"(dst), "+c"(remainder)
        : "a"(c0)
        : "memory"
      );
    }
  }
  else
  {
    __asm__ __volatile__ (
      "movups (%0), %%xmm0"
      :
      : "r"(packedColor)
      : "xmm0"
    );

    iterCount = size / 64;
    for (i = 0; i < iterCount; ++i)
    {
      __asm__ __volatile__ (
        "movups %%xmm0, 0(%0)\n\t"
        "movups %%xmm0, 16(%0)\n\t"
        "movups %%xmm0, 32(%0)\n\t"
        "movups %%xmm0, 48(%0)\n\t"

        "addq $64, %0\n\t"
        : "+r"(dst)
        :
        : "memory"
      );
    }

    remainder = size % 64;
    blocks = remainder / 16;
    for (i = 0; i < blocks; ++i)
    {
      __asm__ __volatile__ (
        "movups %%xmm0, 0(%0)\n\t"

        "addq $16, %0\n\t"
        : "+r"(dst)
        :
        : "memory"
      );
    }

    remainder %= 16;
    if (remainder > 0)
    {
      c0 = (kPixel >> 0)  & 0xFF;
      c1 = (kPixel >> 8)  & 0xFF;
      c2 = (kPixel >> 16) & 0xFF;
      c3 = (kPixel >> 24) & 0xFF;

      pixelBytes = remainder;
      while (pixelBytes >= 4)
      {
        *dst++ = c0;
        *dst++ = c1;
        *dst++ = c2;
        *dst++ = c3;
        pixelBytes -= 4;
      }

      if (pixelBytes >= 1)
      {
        dst[0] = c0;
        ++dst;
        --pixelBytes;
      }
      if (pixelBytes >= 1)
      {
        dst[0] = c1;
        ++dst;
        --pixelBytes;
      }
      if (pixelBytes >= 1)
      {
        dst[0] = c2;
      }
    }
  }
}

static inline void _FastCopy(uintptr_t pDest,
                             uintptr_t kpSrc,
                             size_t    size)
{
  size_t   iterCount;
  uint8_t* src;
  uint8_t* dst;
  size_t   remainder;
  size_t   blocks;
  size_t   i;

  src = (uint8_t*)kpSrc;
  dst = (uint8_t*)pDest;

  if (sController.memoryCapabilities == MEMORY_AVX_SUPPORTED)
  {
    iterCount = size / 128;

    for (i = 0; i < iterCount; ++i)
    {
      __asm__ __volatile__ (
        "vmovups 0(%0), %%ymm0\n\t"
        "vmovups 32(%0), %%ymm1\n\t"
        "vmovups 64(%0), %%ymm2\n\t"
        "vmovups 96(%0), %%ymm3\n\t"

        "vmovups %%ymm0, 0(%1)\n\t"
        "vmovups %%ymm1, 32(%1)\n\t"
        "vmovups %%ymm2, 64(%1)\n\t"
        "vmovups %%ymm3, 96(%1)\n\t"

        "addq $128, %0\n\t"
        "addq $128, %1\n\t"
        : "+r"(src), "+r"(dst)
        :
        : "ymm0", "ymm1", "ymm2", "ymm3", "memory"
      );
    }

    remainder = size % 128;
    blocks = remainder / 32;
    for (i = 0; i < blocks; ++i)
    {
      __asm__ __volatile__ (
        "vmovups 0(%0), %%ymm0\n\t"
        "vmovups %%ymm0, 0(%1)\n\t"

        "addq $32, %0\n\t"
        "addq $32, %1\n\t"
        : "+r"(src), "+r"(dst)
        :
        : "ymm0", "memory"
      );
    }

    remainder %= 32;
    while (remainder--)
    {
      *dst++ = *src++;
    }
  }
  else if (sController.memoryCapabilities == MEMORY_ERMS_SUPPORTED)
  {
    __asm__ __volatile__ (
      "cld\n\t"
      "rep movsb"
      : "+D"(dst), "+S"(src), "+c"(size)
      :
      : "memory"
    );
  }
  else
  {
    iterCount = size / 64;

    for (i = 0; i < iterCount; ++i)
    {
      __asm__ __volatile__ (
        "movups 0(%0), %%xmm0\n\t"
        "movups 16(%0), %%xmm1\n\t"
        "movups 32(%0), %%xmm2\n\t"
        "movups 48(%0), %%xmm3\n\t"

        "movups %%xmm0, 0(%1)\n\t"
        "movups %%xmm1, 16(%1)\n\t"
        "movups %%xmm2, 32(%1)\n\t"
        "movups %%xmm3, 48(%1)\n\t"

        "addq $64, %0\n\t"
        "addq $64, %1\n\t"
        : "+r"(src), "+r"(dst)
        :
        : "xmm0", "xmm1", "xmm2", "xmm3", "memory"
      );
    }

    remainder = size % 64;
    blocks = remainder / 16;
    for (i = 0; i < blocks; ++i)
    {
      __asm__ __volatile__ (
        "movups 0(%0), %%xmm0\n\t"
        "movups %%xmm0, 0(%1)\n\t"

        "addq $16, %0\n\t"
        "addq $16, %1\n\t"
        : "+r"(src), "+r"(dst)
        :
        : "xmm0", "memory"
      );
    }

    remainder %= 16;
    while (remainder--)
    {
      *dst++ = *src++;
    }
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
  int32_t                        retVal;
  S_IOCTLScrollArguments*        pScrollArgs;
  S_IOCTLSetModeArguments*       pSetModeArgs;
  S_IOCTLDrawPixelArguments*     pDrawPixelArgs;
  S_IOCTLDrawRectangleArguments* pDrawRectangleArgs;
  S_IOCTLDrawLineArguments*      pDrawLineArgs;
  S_IOCTLDrawBitmapArguments*    pDrawBitmapArgs;

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
      case VFS_IOCTL_GRAPH_SET_VIDEOMODE:
        pSetModeArgs = pArgs;
        retVal = _SetVESAMode(pSetModeArgs->width,
                              pSetModeArgs->height,
                              pSetModeArgs->bpp,
                              pSetModeArgs->refreshRate);
        break;
      case VFS_IOCTL_GRAPH_DRAWPIXEL:
        pDrawPixelArgs = pArgs;
        _DrawPixel(pDrawPixelArgs->x,
                   pDrawPixelArgs->y,
                   pDrawPixelArgs->color);
        retVal = 0;
        break;
      case VFS_IOCTL_GRAPH_DRAWRECT:
        pDrawRectangleArgs = pArgs;
        _DrawRectangle(pDrawRectangleArgs->x,
                       pDrawRectangleArgs->y,
                       pDrawRectangleArgs->width,
                       pDrawRectangleArgs->height,
                       pDrawRectangleArgs->color);
        retVal = 0;
        break;
      case VFS_IOCTL_GRAPH_DRAWLINE:
        pDrawLineArgs = pArgs;
        _DrawLine(pDrawLineArgs->x,
                  pDrawLineArgs->y,
                  pDrawLineArgs->width,
                  pDrawLineArgs->color);
        retVal = 0;
        break;
      case VFS_IOCTL_GRAPH_DRAWBITMAP:
        pDrawBitmapArgs = pArgs;
        if (MemoryIsMappedWithFlags(pDrawBitmapArgs->kpData,
                                    pDrawBitmapArgs->width *
                                    pDrawBitmapArgs->height *
                                    sizeof(uint32_t),
                                    MEMMGR_MAP_RO | MEMMGR_MAP_KERNEL) == true)
        {
          _DrawBitmap(pDrawBitmapArgs->x,
                      pDrawBitmapArgs->y,
                      pDrawBitmapArgs->width,
                      pDrawBitmapArgs->height,
                      pDrawBitmapArgs->kpData);
          retVal = 0;
        }
        else
        {
          retVal = -1;
        }
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
DRIVERMGR_REG_FDT(sX86VESADriver);

/************************************ EOF *************************************/