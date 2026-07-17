/*******************************************************************************
 * @file CPUSet.h
 *
 * @see CPUSet.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 30/03/2023
 *
 * @version 1.0
 *
 * @brief Generic CPU set definitions.
 *
 * @details Generic CPU set definitions.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __CPU_CPUSET_H_
#define __CPU_CPUSET_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdint.h>
#include <string.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Defines the table size for a CPU bitmask. */
#define CPU_MASK_TABLE_SIZE \
  (SOC_MAX_CPU_COUNT / 64ULL + ((SOC_MAX_CPU_COUNT % 64ULL != 0) ? 1 : 0))


/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Defines a CPU mask. */
typedef struct
{
  /** @brief The CPU mask table. */
  uint64_t mask[CPU_MASK_TABLE_SIZE];
} S_CPUMask;

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/**
 * @brief Sets the corresponding bit in the CPU mask.
 *
 * @param[out] CPU_MASK The mask to udpate.
 * @param[in] CPU_ID The CPU Id to set in the mask.
 */
#define CPU_MASK_SET(CPU_MASK, CPU_ID) {                        \
  CPU_MASK.mask[CPU_ID / 64ULL] |= (1ULL << (CPU_ID % 64ULL));  \
}

/**
 * @brief Resets the bits in the CPU mask.
 *
 * @param[out] CPU_MASK The mask to reset.
 */
#define CPU_MASK_RESET(CPU_MASK) {        \
  memset(&CPU_MASK, 0, sizeof(CPU_MASK)); \
}

/**
 * @brief Clears the corresponding bit in the CPU mask.
 *
 * @param[out] CPU_MASK The mask to udpate.
 * @param[in] CPU_ID The CPU Id to clear in the mask.
 */
#define CPU_MASK_CLEAR(CPU_MASK, CPU_ID) {                      \
  CPU_MASK.mask[CPU_ID / 64ULL] &= ~(1ULL << (CPU_ID % 64ULL)); \
}

/**
 * @brief Gets the corresponding bit in the CPU mask.
 *
 * @param[out] CPU_MASK The mask to use.
 * @param[in] CPU_ID The CPU Id to get in the mask.
 */
#define CPU_MASK_GET(CPU_MASK, CPU_ID) \
  (CPU_MASK.mask[CPU_ID / 64ULL] & (1ULL << (CPU_ID % 64ULL)))

  /**
 * @brief Copy a CPU mask.
 *
 * @param[out] CPU_MASK_DEST The destination mask.
 * @param[in] CPU_MASK_SRC The source mask to copy.
 */
#define CPU_MASK_COPY(CPU_MASK_DEST, CPU_MASK_SRC) {            \
  memcpy(&CPU_MASK_DEST, &CPU_MASK_SRC, sizeof(CPU_MASK_DEST)); \
}


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
/* None */

#endif /* #ifndef __CPU_CPU_H_ */

/************************************ EOF *************************************/