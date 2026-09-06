/*******************************************************************************
 * @file LibcTest.c
 *
 * @brief Kernel libc integration tests.
 ******************************************************************************/
#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* Header file */
#include <TestFramework.h>

/*******************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************/
extern uint64_t __udivdi3(uint64_t numerator, uint64_t denominator);
extern uint64_t __umoddi3(uint64_t numerator, uint64_t denominator);
extern uint64_t __qdivrem(uint64_t numerator,
                          uint64_t denominator,
                          uint64_t* remainder);
extern uint64_t __udivmoddi4(uint64_t numerator,
                             uint64_t denominator,
                             uint64_t* remainder);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static int _CallVsnprintf(char*       pBuffer,
                          size_t      size,
                          const char* kpFormat,
                          ...)
{
  int result;
  __builtin_va_list args;

  __builtin_va_start(args, kpFormat);
  result = vsnprintf(pBuffer, size, kpFormat, args);
  __builtin_va_end(args);

  return result;
}

void LibcTest(void)
{
  char     buffer[64];
  char     source[32];
  char*    pToken;
  char*    pCursor;
  char*    pEnd;
  char*    pFound;
  uint64_t remainder;
  uint64_t quotient;
  int      result;
  size_t   length;

  /* Memory functions. */
  memset(buffer, 0xA5, sizeof(buffer));
  TEST_POINT_ASSERT_POINTER(TEST_LIBC_MEMORY_ID,
                            memset(buffer, 0x5A, 4) == buffer,
                            (uintptr_t)buffer,
                            (uintptr_t)buffer,
                            TEST_LIBC_ENABLED);

  TEST_POINT_ASSERT_UBYTE(TEST_LIBC_MEMORY_ID + 1,
                          buffer[0] == 0x5A && buffer[3] == 0x5A &&
                          buffer[4] == (char)0xA5,
                          1,
                          buffer[0] == 0x5A && buffer[3] == 0x5A &&
                          buffer[4] == (char)0xA5,
                          TEST_LIBC_ENABLED);

  memcpy(buffer, "abcdef", 7);
  TEST_POINT_ASSERT_POINTER(TEST_LIBC_MEMORY_ID + 2,
                            memcpy(source, buffer, 7) == source,
                            (uintptr_t)source,
                            (uintptr_t)source,
                            TEST_LIBC_ENABLED);
  TEST_POINT_ASSERT_UBYTE(TEST_LIBC_MEMORY_ID + 3,
                          source[0] == 'a' && source[5] == 'f' &&
                          source[6] == 0,
                          1,
                          source[0] == 'a' && source[5] == 'f' &&
                          source[6] == 0,
                          TEST_LIBC_ENABLED);

  memcpy(buffer, "0123456789", 11);
  TEST_POINT_ASSERT_POINTER(TEST_LIBC_MEMORY_ID + 4,
                            memmove(buffer + 2, buffer, 8) == buffer + 2,
                            (uintptr_t)(buffer + 2),
                            (uintptr_t)(buffer + 2),
                            TEST_LIBC_ENABLED);
  __asm__ __volatile__("cld");
  TEST_POINT_ASSERT_UBYTE(TEST_LIBC_MEMORY_ID + 5,
                          buffer[2] == '0' && buffer[9] == '7',
                          1,
                          buffer[2] == '0' && buffer[9] == '7',
                          TEST_LIBC_ENABLED);

  buffer[0] = 'a';
  buffer[1] = 'b';
  buffer[2] = 'c';
  buffer[3] = 'd';
  buffer[4] = 'e';
  buffer[5] = 'f';
  TEST_POINT_ASSERT_POINTER(TEST_LIBC_MEMORY_ID + 6,
                            memccpy(source, buffer, 'd', 6) == source + 4,
                            (uintptr_t)(source + 4),
                            (uintptr_t)(source + 4),
                            TEST_LIBC_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_LIBC_MEMORY_ID + 7,
                            memccpy(source, buffer, 'z', 6) == NULL,
                            0,
                            (uintptr_t)memccpy(source, buffer, 'z', 6),
                            TEST_LIBC_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_LIBC_MEMORY_ID + 18,
                            memccpy(source, buffer, 'a', 0) == NULL,
                            0,
                            (uintptr_t)memccpy(source, buffer, 'a', 0),
                            TEST_LIBC_ENABLED);
  pFound = memchr(buffer, 'c', 6);
  TEST_POINT_ASSERT_POINTER(TEST_LIBC_MEMORY_ID + 8,
                            pFound == buffer + 2,
                            (uintptr_t)(buffer + 2),
                            (uintptr_t)pFound,
                            TEST_LIBC_ENABLED);
  pFound = memrchr(buffer, 'c', 6);
  TEST_POINT_ASSERT_POINTER(TEST_LIBC_MEMORY_ID + 9,
                            pFound == buffer + 2,
                            (uintptr_t)(buffer + 2),
                            (uintptr_t)pFound,
                            TEST_LIBC_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_LIBC_MEMORY_ID + 10,
                            memchr(buffer, 'z', 6) == NULL,
                            0,
                            (uintptr_t)memchr(buffer, 'z', 6),
                            TEST_LIBC_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_LIBC_MEMORY_ID + 19,
                            memchr(buffer, 'a', 0) == NULL,
                            0,
                            (uintptr_t)memchr(buffer, 'a', 0),
                            TEST_LIBC_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_LIBC_MEMORY_ID + 20,
                            memrchr(buffer, 'a', 0) == NULL,
                            0,
                            (uintptr_t)memrchr(buffer, 'a', 0),
                            TEST_LIBC_ENABLED);
  result = memcmp("abc", "abc", 3);
  TEST_POINT_ASSERT_INT(TEST_LIBC_MEMORY_ID + 11,
                        result == 0,
                        0,
                        result,
                        TEST_LIBC_ENABLED);
  result = memcmp("abc", "abd", 3);
  TEST_POINT_ASSERT_INT(TEST_LIBC_MEMORY_ID + 12,
                        result < 0,
                        0,
                        result,
                        TEST_LIBC_ENABLED);
  result = memcmp("abd", "abc", 3);
  TEST_POINT_ASSERT_INT(TEST_LIBC_MEMORY_ID + 13,
                        result > 0,
                        0,
                        result,
                        TEST_LIBC_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_LIBC_MEMORY_ID + 14,
                            memmem("012345", 6, "234", 3) != NULL,
                            1,
                            memmem("012345", 6, "234", 3) != NULL,
                            TEST_LIBC_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_LIBC_MEMORY_ID + 15,
                            memmem("012345", 6, "999", 3) == NULL,
                            0,
                            (uintptr_t)memmem("012345", 6, "999", 3),
                            TEST_LIBC_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_LIBC_MEMORY_ID + 16,
                            memmem("abc", 3, "", 0) == NULL,
                            0,
                            (uintptr_t)memmem("abc", 3, "", 0),
                            TEST_LIBC_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_LIBC_MEMORY_ID + 21,
                            memmem("abc", 3, "b", 1) != NULL,
                            1,
                            memmem("abc", 3, "b", 1) != NULL,
                            TEST_LIBC_ENABLED);
  buffer[0] = '1';
  buffer[1] = '2';
  buffer[2] = '3';
  buffer[3] = '4';
  buffer[4] = '5';
  buffer[5] = '6';
  memswap(buffer, buffer + 3, 3);
  TEST_POINT_ASSERT_UBYTE(TEST_LIBC_MEMORY_ID + 17,
                          buffer[0] == '4' && buffer[1] == '5' &&
                          buffer[2] == '6' && buffer[3] == '1' &&
                          buffer[4] == '2' && buffer[5] == '3',
                          1,
                          buffer[0] == '4' && buffer[1] == '5' &&
                          buffer[2] == '6' && buffer[3] == '1' &&
                          buffer[4] == '2' && buffer[5] == '3',
                          TEST_LIBC_ENABLED);

  /* String functions. */
  TEST_POINT_ASSERT_UDWORD(TEST_LIBC_STRING_ID,
                           strlen("") == 0,
                           0,
                           strlen(""),
                           TEST_LIBC_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_LIBC_STRING_ID + 1,
                           strnlen("abcdef", 3) == 3,
                           3,
                           strnlen("abcdef", 3),
                           TEST_LIBC_ENABLED);
  strcpy(buffer, "hello");
  TEST_POINT_ASSERT_POINTER(TEST_LIBC_STRING_ID + 2,
                            strcat(buffer, " world") == buffer,
                            (uintptr_t)buffer,
                            (uintptr_t)buffer,
                            TEST_LIBC_ENABLED);
  TEST_POINT_ASSERT_UBYTE(TEST_LIBC_STRING_ID + 3,
                          buffer[11] == 0,
                          1,
                          buffer[11] == 0,
                          TEST_LIBC_ENABLED);
  strcpy(buffer, "ab");
  strncat(buffer, "cdef", 2);
  TEST_POINT_ASSERT_UBYTE(TEST_LIBC_STRING_ID + 4,
                          buffer[0] == 'a' && buffer[3] == 'd' &&
                          buffer[4] == 0,
                          1,
                          buffer[0] == 'a' && buffer[3] == 'd' &&
                          buffer[4] == 0,
                          TEST_LIBC_ENABLED);
  strncpy(buffer, "xy", 5);
  TEST_POINT_ASSERT_UBYTE(TEST_LIBC_STRING_ID + 5,
                          buffer[0] == 'x' && buffer[1] == 'y' &&
                          buffer[2] == 0 && buffer[4] == 0,
                          1,
                          buffer[0] == 'x' && buffer[1] == 'y' &&
                          buffer[2] == 0 && buffer[4] == 0,
                          TEST_LIBC_ENABLED);
  strncpy(buffer, "abcdef", 3);
  TEST_POINT_ASSERT_UBYTE(TEST_LIBC_STRING_ID + 26,
                          buffer[0] == 'a' && buffer[2] == 'c',
                          1,
                          buffer[0] == 'a' && buffer[2] == 'c',
                          TEST_LIBC_ENABLED);
  strcpy(buffer, "ab");
  length = strlcat(buffer, "cdef", sizeof(buffer));
  TEST_POINT_ASSERT_UDWORD(TEST_LIBC_STRING_ID + 6,
                           length == 6 && buffer[6] == 0,
                           6,
                           length,
                           TEST_LIBC_ENABLED);
  buffer[0] = 'a';
  buffer[1] = 0;
  length = strlcat(buffer, "bcdef", 4);
  TEST_POINT_ASSERT_UDWORD(TEST_LIBC_STRING_ID + 7,
                           length == 6 && buffer[3] == 0,
                           6,
                           length,
                           TEST_LIBC_ENABLED);
  buffer[0] = 'Q';
  length = strlcat(buffer, "xyz", 0);
  TEST_POINT_ASSERT_UDWORD(TEST_LIBC_STRING_ID + 24,
                           length == 3 && buffer[0] == 'Q',
                           3,
                           length,
                           TEST_LIBC_ENABLED);
  result = strcmp("abc", "abd");
  TEST_POINT_ASSERT_INT(TEST_LIBC_STRING_ID + 8,
                        result < 0,
                        0,
                        result,
                        TEST_LIBC_ENABLED);
  TEST_POINT_ASSERT_INT(TEST_LIBC_STRING_ID + 9,
                        strncmp("abc", "abd", 2) == 0,
                        0,
                        strncmp("abc", "abd", 2),
                        TEST_LIBC_ENABLED);
  TEST_POINT_ASSERT_INT(TEST_LIBC_STRING_ID + 10,
                        strncmp("abc", "abd", 0) == 0,
                        0,
                        strncmp("abc", "abd", 0),
                        TEST_LIBC_ENABLED);
  pFound = strchr("abcabc", 'a');
  TEST_POINT_ASSERT_POINTER(TEST_LIBC_STRING_ID + 11,
                            pFound != NULL && pFound[1] == 'b',
                            1,
                            pFound != NULL && pFound[1] == 'b',
                            TEST_LIBC_ENABLED);
  pFound = strrchr("abcabc", 'a');
  TEST_POINT_ASSERT_POINTER(TEST_LIBC_STRING_ID + 12,
                            pFound != NULL && pFound[0] == 'a' &&
                            pFound[1] == 'b' && pFound[3] == 0,
                            1,
                            pFound != NULL && pFound[0] == 'a' &&
                            pFound[1] == 'b' && pFound[3] == 0,
                            TEST_LIBC_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_LIBC_STRING_ID + 13,
                           strcspn("abc123", "0123456789") == 3,
                           3,
                           strcspn("abc123", "0123456789"),
                           TEST_LIBC_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_LIBC_STRING_ID + 14,
                           strspn("123abc", "0123456789") == 3,
                           3,
                           strspn("123abc", "0123456789"),
                           TEST_LIBC_ENABLED);
  pFound = strpbrk("hello", "aeiou");
  TEST_POINT_ASSERT_POINTER(TEST_LIBC_STRING_ID + 15,
                            pFound != NULL && *pFound == 'e',
                            1,
                            pFound != NULL && *pFound == 'e',
                            TEST_LIBC_ENABLED);
  pFound = strstr("hello world", "world");
  TEST_POINT_ASSERT_POINTER(TEST_LIBC_STRING_ID + 16,
                            pFound != NULL && pFound[5] == 0,
                            1,
                            pFound != NULL && pFound[5] == 0,
                            TEST_LIBC_ENABLED);
  strcpy(source, "a,b,,c");
  pCursor = source;
  pToken = strsep(&pCursor, ",");
  TEST_POINT_ASSERT_POINTER(TEST_LIBC_STRING_ID + 17,
                            pToken != NULL && pToken[0] == 'a' &&
                            pToken[1] == 0 && pCursor[0] == 'b',
                            1,
                            pToken != NULL && pToken[0] == 'a' &&
                            pToken[1] == 0 && pCursor[0] == 'b',
                            TEST_LIBC_ENABLED);
  pToken = strsep(&pCursor, ",");
  TEST_POINT_ASSERT_POINTER(TEST_LIBC_STRING_ID + 18,
                            pToken != NULL && pToken[0] == 'b' &&
                            pToken[1] == 0 && pCursor[0] == ',',
                            1,
                            pToken != NULL && pToken[0] == 'b' &&
                            pToken[1] == 0 && pCursor[0] == ',',
                            TEST_LIBC_ENABLED);
  strcpy(source, "a,b");
  pToken = strtok(source, ",");
  TEST_POINT_ASSERT_POINTER(TEST_LIBC_STRING_ID + 19,
                            pToken != NULL && pToken[0] == 'a' &&
                            pToken[1] == 0,
                            1,
                            pToken != NULL && pToken[0] == 'a' &&
                            pToken[1] == 0,
                            TEST_LIBC_ENABLED);
  pToken = strtok(NULL, ",");
  TEST_POINT_ASSERT_POINTER(TEST_LIBC_STRING_ID + 20,
                            pToken != NULL && pToken[0] == 'b' &&
                            pToken[1] == 0,
                            1,
                            pToken != NULL && pToken[0] == 'b' &&
                            pToken[1] == 0,
                            TEST_LIBC_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_LIBC_STRING_ID + 21,
                            strtok(NULL, ",") == NULL,
                            0,
                            (uintptr_t)strtok(NULL, ","),
                            TEST_LIBC_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_LIBC_STRING_ID + 22,
                           __strxspn("123abc", "0123456789", 0) == 3,
                           3,
                           __strxspn("123abc", "0123456789", 0),
                           TEST_LIBC_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_LIBC_STRING_ID + 23,
                           __strxspn("123abc", "0123456789", 1) == 0,
                           0,
                           __strxspn("123abc", "0123456789", 1),
                           TEST_LIBC_ENABLED);

  /* Integer conversions. */
  itoa(-42, buffer, 10);
  TEST_POINT_ASSERT_UBYTE(TEST_LIBC_CONVERSION_ID,
                          buffer[0] == '-' && buffer[1] == '4' &&
                          buffer[2] == '2' && buffer[3] == 0,
                          1,
                          buffer[0] == '-' && buffer[1] == '4' &&
                          buffer[2] == '2' && buffer[3] == 0,
                          TEST_LIBC_ENABLED);
  uitoa(255, buffer, 16);
  TEST_POINT_ASSERT_UBYTE(TEST_LIBC_CONVERSION_ID + 1,
                          buffer[0] == 'F' && buffer[1] == 'F' &&
                          buffer[2] == 0,
                          1,
                          buffer[0] == 'F' && buffer[1] == 'F' &&
                          buffer[2] == 0,
                          TEST_LIBC_ENABLED);
  uitoa(0, buffer, 10);
  TEST_POINT_ASSERT_UBYTE(TEST_LIBC_CONVERSION_ID + 2,
                          buffer[0] == '0' && buffer[1] == 0,
                          1,
                          buffer[0] == '0' && buffer[1] == 0,
                          TEST_LIBC_ENABLED);
  buffer[0] = 'Q';
  buffer[1] = 0;
  itoa(1, buffer, 17);
  TEST_POINT_ASSERT_UBYTE(TEST_LIBC_CONVERSION_ID + 3,
                          buffer[0] == 'Q',
                          1,
                          buffer[0] == 'Q',
                          TEST_LIBC_ENABLED);
  pEnd = NULL;
  result = (int)strtol("-123x", &pEnd, 10);
  TEST_POINT_ASSERT_INT(TEST_LIBC_CONVERSION_ID + 4,
                        result == -123 && *pEnd == 'x',
                        1,
                        result == -123 && *pEnd == 'x',
                        TEST_LIBC_ENABLED);
  pEnd = NULL;
  result = (int)strtoul("456x", &pEnd, 10);
  TEST_POINT_ASSERT_INT(TEST_LIBC_CONVERSION_ID + 5,
                        result == 456 && *pEnd == 'x',
                        1,
                        result == 456 && *pEnd == 'x',
                        TEST_LIBC_ENABLED);
  pEnd = NULL;
  result = (int)strtol("abc", &pEnd, 10);
  TEST_POINT_ASSERT_INT(TEST_LIBC_CONVERSION_ID + 6,
                        result == 0 && *pEnd == 'a',
                        1,
                        result == 0 && *pEnd == 'a',
                        TEST_LIBC_ENABLED);
  pEnd = NULL;
  result = (int)strtol("12", &pEnd, 17);
  TEST_POINT_ASSERT_INT(TEST_LIBC_CONVERSION_ID + 7,
                        result == 0 && *pEnd == '1',
                        1,
                        result == 0 && *pEnd == '1',
                        TEST_LIBC_ENABLED);

  /* Formatting and return values. */
  result = snprintf(buffer, sizeof(buffer), "%s %d %u %x %c",
                    "ok", -12, 34U, 0xABU, 'Z');
  TEST_POINT_ASSERT_INT(TEST_LIBC_FORMAT_ID,
                        result == 14,
                        14,
                        result,
                        TEST_LIBC_ENABLED);
  TEST_POINT_ASSERT_UBYTE(TEST_LIBC_FORMAT_ID + 1,
                          strcmp(buffer, "ok -12 34 ab Z") == 0,
                          0,
                          strcmp(buffer, "ok -12 34 ab Z"),
                          TEST_LIBC_ENABLED);
  result = snprintf(buffer, 5, "abcdef");
  TEST_POINT_ASSERT_INT(TEST_LIBC_FORMAT_ID + 2,
                        result == 4 && buffer[4] == 0,
                        4,
                        result,
                        TEST_LIBC_ENABLED);
  TEST_POINT_ASSERT_INT(TEST_LIBC_FORMAT_ID + 3,
                        snprintf(NULL, 0, "x") == -1,
                        -1,
                        snprintf(NULL, 0, "x"),
                        TEST_LIBC_ENABLED);
  result = _CallVsnprintf(buffer, sizeof(buffer), "%u:%s", 7U, "ok");
  TEST_POINT_ASSERT_INT(TEST_LIBC_FORMAT_ID + 4,
                        result == 4 && buffer[0] == '7' &&
                        buffer[1] == ':' && buffer[2] == 'o' &&
                        buffer[3] == 'k' && buffer[4] == 0,
                        4,
                        result,
                        TEST_LIBC_ENABLED);

  /* Unsigned division helpers. */
  remainder = 0;
  quotient = __udivmoddi4(100, 7, &remainder);
  TEST_POINT_ASSERT_UDWORD(TEST_LIBC_ARITHMETIC_ID,
                           quotient == 14 && remainder == 2,
                           14,
                           quotient,
                           TEST_LIBC_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_LIBC_ARITHMETIC_ID + 1,
                           __udivdi3(100, 7) == 14,
                           14,
                           __udivdi3(100, 7),
                           TEST_LIBC_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_LIBC_ARITHMETIC_ID + 2,
                           __umoddi3(100, 7) == 2,
                           2,
                           __umoddi3(100, 7),
                           TEST_LIBC_ENABLED);
  quotient = __udivmoddi4(UINT64_MAX, 1, NULL);
  TEST_POINT_ASSERT_UDWORD(TEST_LIBC_ARITHMETIC_ID + 3,
                           quotient == UINT64_MAX,
                           UINT64_MAX,
                           quotient,
                           TEST_LIBC_ENABLED);
  remainder = 0;
  quotient = __qdivrem(0x20002, 0x10001, &remainder);
  TEST_POINT_ASSERT_UDWORD(TEST_LIBC_ARITHMETIC_ID + 4,
                           quotient == 2 && remainder == 0,
                           2,
                           quotient,
                           TEST_LIBC_ENABLED);
  remainder = 0;
  quotient = __qdivrem(5, 0, &remainder);
  TEST_POINT_ASSERT_UDWORD(TEST_LIBC_ARITHMETIC_ID + 5,
                           quotient == 0 && remainder == 5,
                           0,
                           quotient,
                           TEST_LIBC_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_LIBC_ARITHMETIC_ID + 6,
                           __qdivrem(3, 5, NULL) == 0,
                           0,
                           __qdivrem(3, 5, NULL),
                           TEST_LIBC_ENABLED);

  TEST_FRAMEWORK_END();
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/