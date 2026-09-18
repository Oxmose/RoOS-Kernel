/*******************************************************************************
 * @file mman.h
 *
 * @see mman.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 16/09/2026
 *
 * @version 1.0
 *
 * @brief Lib C memory management types for roOs.
 *
 * @details Lib C memory management types for roOs.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __LIB_SYS_MMAN_H_
#define __LIB_SYS_MMAN_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Memory protection flag for read access. */
#define PROT_READ  0x1
/** @brief Memory protection flag for write access. */
#define PROT_WRITE 0x2
/** @brief Memory protection flag for execute access. */
#define PROT_EXEC  0x4
/** @brief Memory protection flag for no access. */
#define PROT_NONE  0x8

/** @brief The value returned by mmap() on failure. */
#define MAP_FAILED ((void*)-1)

/** @brief Flag for a private mapping. */
#define MAP_PRIVATE 0x1
/** @brief Flag for an anonymous mapping. */
#define MAP_ANONYMOUS 0x2

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
 * @brief Maps files or devices into memory.
 *
 * @details Maps files or devices into memory. This function is used to map a
 * file or device into the process's address space. The mapping can be shared or
 * private, and the memory can be read-only or read-write.
 *
 * @param[in] addr The starting address for the new mapping. If NULL, the kernel
 * chooses the address.
 * @param[in] length The length of the mapping in bytes.
 * @param[in] prot The desired memory protection of the mapping. It can be a
 * combination of the following flags: PROT_READ, PROT_WRITE, PROT_EXEC,
 * PROT_NONE.
 * @param[in] flags The type of mapping. It can be a combination of the
 * following flags: MAP_SHARED, MAP_PRIVATE, MAP_ANONYMOUS.
 * @param[in] fd The file descriptor of the file to be mapped. If MAP_ANONYMOUS
 * is set in flags, this parameter is ignored.
 * @param[in] offset The offset in the file from which to start the mapping.
 * Must be a multiple of the page size.
 *
 * @return A pointer to the mapped memory, or MAP_FAILED on error.
 *
 */
void *mmap(void*  addr,
           size_t length,
           int    prot,
           int    flags,
           int    fd,
           off_t  offset);

/**
 * @brief Unmaps files or devices from memory.
 *
 * @details Unmaps files or devices from memory. This function is used to remove
 * a mapping from the process's address space. The memory is no longer
 * accessible after this call.
 *
 * @param[in] addr The starting address of the mapping to be removed.
 * @param[in] length The length of the mapping to be removed.
 *
 * @return 0 on success, -1 on error.
 */
int munmap(void* addr, size_t length);

#endif /* #ifndef __LIB_SYS_MMAN_H_ */

/************************************ EOF *************************************/