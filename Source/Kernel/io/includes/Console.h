/*******************************************************************************
 * @file Console.h
 *
 * @see Console.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 30/03/2023
 *
 * @version 2.0
 *
 * @brief Console drivers abtraction.
 *
 * @details Console driver abtraction layer. The functions of this module allows
 * to abtract the use of any supported console driver and the selection of the
 * desired driver.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __IO_CONSOLE_H_
#define __IO_CONSOLE_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Background color definition: black. */
#define BG_BLACK            0x00
/** @brief Background color definition: blue. */
#define BG_BLUE             0x10
/** @brief Background color definition: green. */
#define BG_GREEN            0x20
/** @brief Background color definition: cyan. */
#define BG_CYAN             0x30
/** @brief Background color definition: red. */
#define BG_RED              0x40
/** @brief Background color definition: magenta. */
#define BG_MAGENTA          0x50
/** @brief Background color definition: brown. */
#define BG_BROWN            0x60
/** @brief Background color definition: grey. */
#define BG_GREY             0x70
/** @brief Background color definition: dark grey. */
#define BG_DARKGREY         0x80
/** @brief Background color definition: bright blue. */
#define BG_BRIGHTBLUE       0x90
/** @brief Background color definition: bright green. */
#define BG_BRIGHTGREEN      0xA0
/** @brief Background color definition: bright cyan. */
#define BG_BRIGHTCYAN       0xB0
/** @brief Background color definition: bright red. */
#define BG_BRIGHTRED        0xC0
/** @brief Background color definition: bright magenta. */
#define BG_BRIGHTMAGENTA    0xD0
/** @brief Background color definition: yellow. */
#define BG_YELLOW           0xE0
/** @brief Background color definition: white. */
#define BG_WHITE            0xF0

/** @brief Foreground color definition: black. */
#define FG_BLACK            0x00
/** @brief Foreground color definition: blue. */
#define FG_BLUE             0x01
/** @brief Foreground color definition: green. */
#define FG_GREEN            0x02
/** @brief Foreground color definition: cyan. */
#define FG_CYAN             0x03
/** @brief Foreground color definition: red. */
#define FG_RED              0x04
/** @brief Foreground color definition: magenta. */
#define FG_MAGENTA          0x05
/** @brief Foreground color definition: brown. */
#define FG_BROWN            0x06
/** @brief Foreground color definition: grey. */
#define FG_GREY             0x07
/** @brief Foreground color definition: dark grey. */
#define FG_DARKGREY         0x08
/** @brief Foreground color definition: bright blue. */
#define FG_BRIGHTBLUE       0x09
/** @brief Foreground color definition: bright green. */
#define FG_BRIGHTGREEN      0x0A
/** @brief Foreground color definition: bright cyan. */
#define FG_BRIGHTCYAN       0x0B
/** @brief Foreground color definition: bright red. */
#define FG_BRIGHTRED        0x0C
/** @brief Foreground color definition: bright magenta. */
#define FG_BRIGHTMAGENTA    0x0D
/** @brief Foreground color definition: yellow. */
#define FG_YELLOW           0x0E
/** @brief Foreground color definition: white. */
#define FG_WHITE            0x0F

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/**
 * @brief Scroll direction enumeration, enumerates all the possible scrolling
 * direction supported by the driver.
 */
typedef enum
{
  /** @brief Scroll down direction. */
  SCROLL_DOWN,
  /** @brief Scroll up direction. */
  SCROLL_UP
} E_ScrollDirection;

/**
 * @brief Console cursor representation for the driver. The structures contains
 * the required data to keep track of the current cursor's position.
 */
typedef struct
{
  /** @brief The x position of the cursor. */
  uint32_t x;
  /** @brief The y position of the cursor. */
  uint32_t y;
} S_ConsoleCursor;

/**
 * @brief Console color scheme representation. Keeps the different display
 * format such as background color in memory.
 */
typedef struct
{
  /** @brief The foreground color to be used when outputing data. */
  uint16_t foreground;
  /** @brief The background color to be used when outputing data. */
  uint16_t background;
} S_ColorScheme;

/**
 * @brief Defines the driver for the console.
 */
typedef struct
{
  /**
   * @brief Clears the console.
   *
   * @details Clears the console using the driver. The background color is set
   * to the current background color.
   */
  void (*pClear)(void);

  /**
   * @brief Gets the cursor attributes.
   * @details Fills the buffer given as parameter with the current value of the
   * cursor.
   *
   * @param[out] pBuffer The cursor buffer in which the current cursor
   * position is going to be saved.
   */
  void (*pGetCursor)(S_ConsoleCursor* pBuffer);

  /**
   * @brief Sets the cursor attributes.
   *
   * @details The function will set the cursor attributes from the buffer
   * given as parameter.
   *
   * @param[in] kpBuffer The buffer containing the cursor's attributes.
   */
  void (*pSetCursor)(const S_ConsoleCursor* kpBuffer);

  /**
   * @brief Scrolls the console in the desired direction.
   *
   * @details The function will use the driver to scroll of the number of lines
   * in the desired direction.
   *
   * @param[in] kDirection The direction to which the console
   * @param[in] kLines The number of lines to scroll.
   */

  void (*pScroll)(const E_ScrollDirection kDirection, const uint32_t kLines);

  /**
   * @brief Sets the color scheme of the console.
   *
   * @details Replaces the curent color scheme used t output data with the new
   * one given as parameter.
   *
   * @param[in] pkColorScheme The new color scheme to apply to the console.
   */
  void (*pSetColorScheme)(const S_ColorScheme* pkColorScheme);

  /**
   * @brief Saves the color scheme of the console.
   *
   * @details Fills the buffer given as parameter with the current console's
   * color scheme value.
   *
   * @param[out] pBuffer The buffer that will receive the current color scheme/
   */
  void (*pGetColorScheme)(S_ColorScheme* pBuffer);

  /**
   * @brief Puts a string to the console.
   *
   * @details Outputs the given string to the console using the driver.
   *
   * @param[in] kpString The string to output.
   */
  void (*pPutString)(const char* kpString);

  /**
   * @brief Puts a character to the console.
   *
   * @details Outputs the given character to the console using the driver.
   *
   * @param[in] kCharacter The character to output.
   */
  void (*pPutChar)(const char kCharacter);

  /**
   * @brief Reads data from the console input buffer.
   *
   * @details Reads data from the console input buffer. The function returns
   * the number of bytes read. If the buffer is empty, the function is
   * blocking until the buffer is filled with the required number of bytes.
   *
   * @param[out] pBuffer The buffer used to receive data.
   * @param[in] kBufferSize The number of bytes to read.
   *
   * @return The function returns the number of bytes read or -1 on error.
   */
  ssize_t (*pRead)(char* pBuffer, const size_t kBufferSize);

  /**
   * @brief Flushes the console output.
   *
   * @details The function will request a flush to the console output driver.
   */
  void (*pFlush)(void);
} S_ConsoleDriver;

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
/**
 * @brief Initializes the console.
 *
 * @details Initializes the console. On error, this function fails silently and
 * no input nor output will be made.
 */
void ConsoleInit(void);

/**
 * @brief Sets the console driver to use.
 *
 * @details Sets the console driver to use. The driver must be initialized and
 * ready to use. The function will copy the driver given as parameter to the
 * internal driver used by the console.
 *
 * @param[in] kpDriver The driver to use for the console.
 */
void ConsoleSetDriver(const S_ConsoleDriver* kpDriver);

/**
 * @brief Clears the console, the background color is set to black.
 */
void ConsoleClear(void);

/**
 * @brief Gets the cursor attributes in the buffer given as paramter.
 *
 * @details Fills the buffer given s parameter with the current value of the
 * cursor.
 *
 * @param[out] pBuffer The cursor buffer in which the current cursor
 * position is going to be saved.
 */
void ConsoleGetCursor(S_ConsoleCursor* pBuffer);

/**
 * @brief Set the cursor attributes from the buffer given as parameter.
 *
 * @details The function will set the cursor attributes from the buffer
 * given as parameter.
 *
 * @param[in] kpBuffer The buffer containing the cursor's attributes.
 */
void ConsoleSetCursor(const S_ConsoleCursor* kpBuffer);

/**
 * @brief Scrolls in the desired direction of lines_count lines.
 *
 * @details The function will use the driver to scroll of lines_count line in
 * the desired direction.
 *
 * @param[in] kDirection The direction to which the console
 * should be scrolled.
 * @param[in] kLines The number of lines to scroll.
 */
void ConsoleScroll(const E_ScrollDirection kDirection, const uint32_t kLines);

/**
 * @brief Sets the color scheme of the console.
 *
 * @details Replaces the curent color scheme used t output data with the new
 * one given as parameter.
 *
 * @param[in] pkColorScheme The new color scheme to apply to
 * the console.
 */
void ConsoleSetColorScheme(const S_ColorScheme* pkColorScheme);

/**
 * @brief Saves the color scheme in the buffer given as parameter.
 *
 * @details Fills the buffer given as parameter with the current console's
 * color scheme value.
 *
 * @param[out] pBuffer The buffer that will receive the current
 * color scheme used by the console.
 */
void ConsoleGetColorScheme(S_ColorScheme* pBuffer);

/**
 * ­@brief Put a string to console.
 *
 * @details The function will display the string given as parameter to the
 * console using the selected driver.
 *
 * @param[in] kpString The string to display on the console.
 *
 * @warning kpString must be NULL terminated.
 */
void ConsolePutString(const char* kpString);

/**
 * ­@brief Put a character to console.
 *
 * @details The function will display the character given as parameter to the
 * console using the selected driver.
 *
 * @param[in] kCharacter The char to display on the console.
 */
void ConsolePutChar(const char kCharacter);

/**
 * @brief Reads data from the console input buffer.
 *
 * @details Reads data from the console input buffer. The function returns
 * the number of bytes read. If the buffer is empty, the function is
 * blocking until the buffer is filled with the required number of bytes.
 *
 * @param[out] pBuffer The buffer used to receive data.
 * @param[in] kBufferSize The number of bytes to read.
 *
 * @return The function returns the number of bytes read or -1 on error.
 */
ssize_t ConsoleRead(char* pBuffer, const size_t kBufferSize);

/**
 * ­@brief Flushes the console output.
 *
 * @details The function will request a flush to the console output driver.
 *
 */
void ConsoleFlush(void);

#endif /* #ifndef __IO_CONSOLE_H_ */

/************************************ EOF *************************************/