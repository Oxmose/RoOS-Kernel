/*******************************************************************************
 * @file string.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 30/03/2023
 *
 * @version 1.0
 *
 * @brief Kernel's strings and memory manipulation functions.
 *
 * @details Strings and memory manipulation functions.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __LIB_STRING_H_
#define __LIB_STRING_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stddef.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/* None */

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
 * @brief Copies a block of memory from one location to another.
 *
 * @details Copies exactly count bytes from the object pointed to by src to the
 * object pointed to by dest. The source and destination objects must not
 * overlap.
 *
 * @param[out] dest Pointer to the destination memory area.
 * @param[in] src Pointer to the source memory area.
 * @param[in] c Character to stop copying at, converted to unsigned char.
 * @param[in] count Number of bytes to copy.
 *
 * @return Returns a pointer to the next byte after the first occurrence of c in
 * dest, or NULL if c was not found in the copied range.
 */
void *memccpy(void *restrict dest,
              const void *restrict src,
              int c,
              size_t count);

/**
 * @brief Scans a memory region for the first occurrence of a byte value.
 *
 * @details Finds the first occurrence of ch in the first count bytes of the
 * memory block pointed to by ptr.
 *
 * @param[in] ptr Pointer to the memory block to inspect.
 * @param[in] ch Byte value to search for.
 * @param[in] count Number of bytes to inspect.
 *
 * @return Returns a pointer to the matched byte, or NULL if no match is found.
 */
void *memchr(const void *ptr, int ch, size_t count);

/**
 * @brief Scans a memory region for the last occurrence of a byte value.
 *
 * @details Finds the last occurrence of ch in the first count bytes of the
 * memory block pointed to by ptr.
 *
 * @param[in] ptr Pointer to the memory block to inspect.
 * @param[in] ch Byte value to search for.
 * @param[in] count Number of bytes to inspect.
 *
 * @return Returns a pointer to the matched byte, or NULL if no match is found.
 */
void *memrchr(const void *ptr, int ch, size_t count);

/**
 * @brief Compares two memory blocks.
 *
 * @details Compares the first count bytes of the objects pointed to by lhs and
 * rhs.
 *
 * @param[in] lhs Pointer to the first memory block.
 * @param[in] rhs Pointer to the second memory block.
 * @param[in] count Number of bytes to compare.
 *
 * @return Returns a negative value if lhs is less than rhs, a positive value if
 * lhs is greater than rhs, and zero if the memory blocks are equal.
 */
int memcmp(const void *lhs, const void *rhs, size_t count);

/**
 * @brief Copies a memory block.
 *
 * @details Copies count bytes from the object pointed to by src to the object
 * pointed to by dest. The objects may overlap, in which case the behavior is
 * defined as if the bytes are copied to a temporary buffer.
 *
 * @param[out] dest Pointer to the destination memory area.
 * @param[in] src Pointer to the source memory area.
 * @param[in] count Number of bytes to copy.
 *
 * @return Returns a pointer to dest.
 */
void *memcpy(void *dest, const void *src, size_t count);

/**
 * @brief Moves a memory block.
 *
 * @details Copies count bytes from the object pointed to by src to the object
 * pointed to by dest. The objects may overlap.
 *
 * @param[out] dest Pointer to the destination memory area.
 * @param[in] src Pointer to the source memory area.
 * @param[in] count Number of bytes to move.
 *
 * @return Returns a pointer to dest.
 */
void *memmove(void *dest, const void *src, size_t count);

/**
 * @brief Fills a memory block with a byte value.
 *
 * @details Converts value to an unsigned char and fills the first count bytes of
 * the object pointed to by ptr with that value.
 *
 * @param[out] ptr Pointer to the memory block to fill.
 * @param[in] value Byte value used to fill the memory block.
 * @param[in] count Number of bytes to set.
 *
 * @return Returns a pointer to the memory block.
 */
void *memset(void *ptr, int value, size_t count);

/**
 * @brief Finds the first occurrence of a sequence in memory.
 *
 * @details Searches for the first occurrence of the byte sequence needle within
 * the memory range haystack.
 *
 * @param[in] haystack Pointer to the memory block to search.
 * @param[in] haystackLen Length of the haystack memory block.
 * @param[in] needle Pointer to the sequence to search for.
 * @param[in] needleLen Length of the sequence to search for.
 *
 * @return Returns a pointer to the first occurrence of needle, or NULL if not
 * found.
 */
void *memmem(const void *haystack,
             size_t haystackLen,
             const void *needle,
             size_t needleLen);

/**
 * @brief Swaps two memory blocks.
 *
 * @details Exchanges the contents of the two memory areas pointed to by lhs and
 * rhs, each of size count bytes.
 *
 * @param[in,out] lhs Pointer to the first memory block.
 * @param[in,out] rhs Pointer to the second memory block.
 * @param[in] count Number of bytes to swap.
 */
void memswap(void *lhs, void *rhs, size_t count);

/**
 * @brief Compares two strings ignoring case.
 *
 * @details Compares the null-terminated strings pointed to by lhs and rhs using
 * a case-insensitive comparison.
 *
 * @param[in] lhs Pointer to the first string.
 * @param[in] rhs Pointer to the second string.
 *
 * @return Returns a value less than, equal to, or greater than zero according
 * to the comparison result.
 */
int strcasecmp(const char *lhs, const char *rhs);

/**
 * @brief Compares up to n characters of two strings ignoring case.
 *
 * @details Compares at most count characters from the strings pointed to by lhs
 * and rhs, ignoring case differences.
 *
 * @param[in] lhs Pointer to the first string.
 * @param[in] rhs Pointer to the second string.
 * @param[in] count Maximum number of characters to compare.
 *
 * @return Returns a value less than, equal to, or greater than zero according
 * to the comparison result.
 */
int strncasecmp(const char *lhs, const char *rhs, size_t count);

/**
 * @brief Appends a string to another string.
 *
 * @details Appends the null-terminated string pointed to by src to the end of the
 * null-terminated string pointed to by dest.
 *
 * @param[in,out] dest Pointer to the destination string.
 * @param[in] src Pointer to the source string.
 *
 * @return Returns a pointer to dest.
 */
char *strcat(char *dest, const char *src);

/**
 * @brief Finds the first occurrence of a character in a string.
 *
 * @details Searches the null-terminated string pointed to by str for the first
 * occurrence of ch.
 *
 * @param[in] str Pointer to the string to inspect.
 * @param[in] ch Character to search for.
 *
 * @return Returns a pointer to the matched character or NULL if no match is
 * found.
 */
char *strchr(const char *str, int ch);

/**
 * @brief Finds the last occurrence of a character in a string.
 *
 * @details Searches the null-terminated string pointed to by str for the last
 * occurrence of ch.
 *
 * @param[in] str Pointer to the string to inspect.
 * @param[in] ch Character to search for.
 *
 * @return Returns a pointer to the matched character or NULL if no match is
 * found.
 */
char *strrchr(const char *str, int ch);

/**
 * @brief Compares two strings.
 *
 * @details Compares the null-terminated strings pointed to by lhs and rhs.
 *
 * @param[in] lhs Pointer to the first string.
 * @param[in] rhs Pointer to the second string.
 *
 * @return Returns a value less than, equal to, or greater than zero according
 * to the comparison result.
 */
int strcmp(const char *lhs, const char *rhs);

/**
 * @brief Copies a string.
 *
 * @details Copies the null-terminated string pointed to by src to the buffer
 * pointed to by dest.
 *
 * @param[out] dest Pointer to the destination buffer.
 * @param[in] src Pointer to the source string.
 *
 * @return Returns a pointer to dest.
 */
char *strcpy(char *dest, const char *src);

/**
 * @brief Computes the length of the initial segment of a string not containing
 * any characters from a given set.
 *
 * @details Scans the string pointed to by str until a character is found that is
 * included in the set pointed to by reject.
 *
 * @param[in] str Pointer to the string to inspect.
 * @param[in] reject Pointer to the set of characters to reject.
 *
 * @return Returns the length of the initial segment of str containing no
 * characters from reject.
 */
size_t strcspn(const char *str, const char *reject);

/**
 * @brief Duplicates a string.
 *
 * @details Allocates a new string that is a duplicate of the null-terminated
 * string pointed to by str.
 *
 * @param[in] str Pointer to the string to duplicate.
 *
 * @return Returns a pointer to the newly allocated duplicate string, or NULL on
 * allocation failure.
 */
char *strdup(const char *str);

/**
 * @brief Duplicates a string up to a maximum length.
 *
 * @details Allocates a new string containing at most count characters from the
 * null-terminated string pointed to by str.
 *
 * @param[in] str Pointer to the string to duplicate.
 * @param[in] count Maximum number of characters to copy.
 *
 * @return Returns a pointer to the newly allocated duplicate string, or NULL on
 * allocation failure.
 */
char *strndup(const char *str, size_t count);

/**
 * @brief Maps an errno value to an error message string.
 *
 * @details Returns a pointer to a descriptive error message corresponding to the
 * given error code.
 *
 * @param[in] errnum Error number to translate.
 *
 * @return Returns a pointer to the corresponding error string.
 */
char *strerror(int errnum);

/**
 * @brief Maps a signal value to a signal description string.
 *
 * @details Returns a pointer to a short description of the signal identified by
 * signum.
 *
 * @param[in] signum Signal number to translate.
 *
 * @return Returns a pointer to the corresponding signal description.
 */
char *strsignal(int signum);

/**
 * @brief Computes the length of a string.
 *
 * @details Computes the length of the null-terminated string pointed to by str,
 * excluding the terminating null byte.
 *
 * @param[in] str Pointer to the string to measure.
 *
 * @return Returns the number of characters in the string.
 */
size_t strlen(const char *str);

/**
 * @brief Computes the length of a string up to a maximum count.
 *
 * @details Computes the length of the string pointed to by str, but does not
 * scan beyond count characters.
 *
 * @param[in] str Pointer to the string to measure.
 * @param[in] count Maximum number of characters to inspect.
 *
 * @return Returns the number of characters in the string before the null byte or
 * before count is reached, whichever comes first.
 */
size_t strnlen(const char *str, size_t count);

/**
 * @brief Appends a string to another string up to a maximum length.
 *
 * @details Appends at most count characters from the null-terminated string
 * pointed to by src to the end of the string pointed to by dest.
 *
 * @param[in,out] dest Pointer to the destination string.
 * @param[in] src Pointer to the source string.
 * @param[in] count Maximum number of characters to append.
 *
 * @return Returns a pointer to dest.
 */
char *strncat(char *dest, const char *src, size_t count);

/**
 * @brief Appends a string to another string with size limit.
 *
 * @details Appends the null-terminated string pointed to by src to the end of the
 * string pointed to by dest, while respecting the size of the destination buffer.
 *
 * @param[in,out] dest Pointer to the destination buffer.
 * @param[in] size Size of the destination buffer in bytes.
 * @param[in] src Pointer to the source string.
 *
 * @return Returns the total length of the destination string.
 */
size_t strlcat(char *dest, const char *src, size_t size);

/**
 * @brief Compares two strings up to a maximum length.
 *
 * @details Compares at most count characters from the null-terminated strings
 * pointed to by lhs and rhs.
 *
 * @param[in] lhs Pointer to the first string.
 * @param[in] rhs Pointer to the second string.
 * @param[in] count Maximum number of characters to compare.
 *
 * @return Returns a value less than, equal to, or greater than zero according
 * to the comparison result.
 */
int strncmp(const char *lhs, const char *rhs, size_t count);

/**
 * @brief Copies a string up to a maximum length.
 *
 * @details Copies at most count characters from the null-terminated string
 * pointed to by src to the buffer pointed to by dest.
 *
 * @param[out] dest Pointer to the destination buffer.
 * @param[in] src Pointer to the source string.
 * @param[in] count Maximum number of characters to copy.
 *
 * @return Returns a pointer to dest.
 */
char *strncpy(char *dest, const char *src, size_t count);

/**
 * @brief Copies a string with size limit.
 *
 * @details Copies the string pointed to by src into the buffer pointed to by
 * dest, ensuring that the destination buffer is not overflowed.
 *
 * @param[out] dest Pointer to the destination buffer.
 * @param[in] size Size of the destination buffer in bytes.
 * @param[in] src Pointer to the source string.
 *
 * @return Returns the total length of the source string.
 */
size_t strlcpy(char *dest, const char *src, size_t size);

/**
 * @brief Searches a string for any character from a set.
 *
 * @details Finds the first occurrence in the string pointed to by str of any
 * character contained in the set pointed to by accept.
 *
 * @param[in] str Pointer to the string to inspect.
 * @param[in] accept Pointer to the set of accepted characters.
 *
 * @return Returns a pointer to the first matching character or NULL if none are
 * found.
 */
char *strpbrk(const char *str, const char *accept);

/**
 * @brief Extracts a token from a string.
 *
 * @details In the string pointed to by *stringp, finds the next token separated
 * by characters in delim and replaces the delimiter by a null terminator.
 *
 * @param[in,out] stringp Pointer to the string pointer to parse.
 * @param[in] delim Pointer to the set of delimiter characters.
 *
 * @return Returns a pointer to the next token or NULL when no more tokens are
 * available.
 */
char *strsep(char **stringp, const char *delim);

/**
 * @brief Computes the length of the initial segment of a string containing only
 * characters from a given set.
 *
 * @details Scans the string pointed to by str until a character is found that is
 * not in the set pointed to by accept.
 *
 * @param[in] str Pointer to the string to inspect.
 * @param[in] accept Pointer to the set of accepted characters.
 *
 * @return Returns the length of the initial segment of str containing only
 * characters from accept.
 */
size_t strspn(const char *str, const char *accept);

/**
 * @brief Finds the first occurrence of a substring.
 *
 * @details Searches the null-terminated string pointed to by haystack for the
 * first occurrence of the substring pointed to by needle.
 *
 * @param[in] haystack Pointer to the string to search within.
 * @param[in] needle Pointer to the substring to find.
 *
 * @return Returns a pointer to the first occurrence of needle, or NULL if the
 * substring is not found.
 */
char *strstr(const char *haystack, const char *needle);

/**
 * @brief Extracts tokens from a string.
 *
 * @details Breaks the string pointed to by str into a sequence of tokens, each
 * delimited by characters contained in delim.
 *
 * @param[in,out] str Pointer to the string to parse. A NULL pointer indicates
 * that the function continues parsing the previous string.
 * @param[in] delim Pointer to the set of delimiter characters.
 *
 * @return Returns a pointer to the next token, or NULL if no more tokens are
 * available.
 */
char *strtok(char *str, const char *delim);

/**
 * @brief Computes a span over a set of characters with parity filtering.
 *
 * @details Evaluates the length of the prefix of s that matches the character
 * map given by map according to a parity rule.
 *
 * @param[in] s Pointer to the string to inspect.
 * @param[in] map Pointer to the character map used for matching.
 * @param[in] parity Parity criterion used by the internal span evaluation.
 *
 * @return Returns the length of the matching prefix.
 */
size_t __strxspn(const char *s, const char *map, int parity);

#endif /* #ifndef __LIB_STRING_H_ */

/************************************ EOF *************************************/
