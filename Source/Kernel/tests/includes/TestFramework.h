/*******************************************************************************
 * @file TestFramework.h
 *
 * @see TestFramework.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 25/04/2023
 *
 * @version 1.0
 *
 * @brief Testing framework.
 *
 * @details Testing framework. This modules allows to add dynamic test points
 * to the kernel an run a test suite.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __TEST_FRAMEWORK_TEST_FRAMEWORK_H_
#define __TEST_FRAMEWORK_TEST_FRAMEWORK_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
/* None to ensure correct inclusion in other files */

/* Configuration files */
#include <config.h>

/* Header file */
#include <TestList.h>
#include <TestFramework.h>

#ifdef _TESTING_FRAMEWORK_ENABLED

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
/**
 * @brief Assert macro for unsigned 32-bit integers.
 *
 * @details This macro reports the result of a uint32 assertion to the test
 * framework when the test point is enabled.
 *
 * @param[in] ID The test point ID.
 * @param[in] COND The condition that should be true.
 * @param[in] EXPECTED The expected value.
 * @param[in] VALUE The value to check.
 * @param[in] TEST_ENABLED The test point enabled state.
 */
#define TEST_POINT_ASSERT_UINT(ID, COND, EXPECTED, VALUE, TEST_ENABLED) { \
  if(TEST_ENABLED)                                                        \
  {                                                                       \
    TestFrameworkAssertUint(ID, COND, EXPECTED, VALUE);                   \
  }                                                                       \
}

/**
 * @brief Assert macro for signed 32-bit integers.
 *
 * @details This macro reports the result of an int32 assertion to the test
 * framework when the test point is enabled.
 *
 * @param[in] ID The test point ID.
 * @param[in] COND The condition that should be true.
 * @param[in] EXPECTED The expected value.
 * @param[in] VALUE The value to check.
 * @param[in] TEST_ENABLED The test point enabled state.
 */
#define TEST_POINT_ASSERT_INT(ID, COND, EXPECTED, VALUE, TEST_ENABLED)  { \
  if(TEST_ENABLED)                                                        \
  {                                                                       \
    TestFrameworkAssertInt(ID, COND, EXPECTED, VALUE);                    \
  }                                                                       \
}

/**
 * @brief Assert macro for unsigned 16-bit values.
 *
 * @details This macro reports the result of a uint16 assertion to the test
 * framework when the test point is enabled.
 *
 * @param[in] ID The test point ID.
 * @param[in] COND The condition that should be true.
 * @param[in] EXPECTED The expected value.
 * @param[in] VALUE The value to check.
 * @param[in] TEST_ENABLED The test point enabled state.
 */
#define TEST_POINT_ASSERT_HUINT(ID, COND, EXPECTED, VALUE, TEST_ENABLED) {  \
  if(TEST_ENABLED)                                                          \
  {                                                                         \
    TestFrameworkAssertHuint(ID, COND, EXPECTED, VALUE);                    \
  }                                                                         \
}

/**
 * @brief Assert macro for signed 16-bit integers.
 *
 * @details This macro reports the result of an int16 assertion to the test
 * framework when the test point is enabled.
 *
 * @param[in] ID The test point ID.
 * @param[in] COND The condition that should be true.
 * @param[in] EXPECTED The expected value.
 * @param[in] VALUE The value to check.
 * @param[in] TEST_ENABLED The test point enabled state.
 */
#define TEST_POINT_ASSERT_HINT(ID, COND, EXPECTED, VALUE, TEST_ENABLED) {   \
  if(TEST_ENABLED)                                                          \
  {                                                                         \
    TestFrameworkAssertHint(ID, COND, EXPECTED, VALUE);                     \
  }                                                                         \
}

/**
 * @brief Assert macro for unsigned 8-bit values.
 *
 * @details This macro reports the result of a ubyte assertion to the test
 * framework when the test point is enabled.
 *
 * @param[in] ID The test point ID.
 * @param[in] COND The condition that should be true.
 * @param[in] EXPECTED The expected value.
 * @param[in] VALUE The value to check.
 * @param[in] TEST_ENABLED The test point enabled state.
 */
#define TEST_POINT_ASSERT_UBYTE(ID, COND, EXPECTED, VALUE, TEST_ENABLED) {  \
  if(TEST_ENABLED)                                                          \
  {                                                                         \
    TestFrameworkAssertUbyte(ID, COND, EXPECTED, VALUE);                    \
  }                                                                         \
}

/**
 * @brief Assert macro for signed 8-bit values.
 *
 * @details This macro reports the result of a byte assertion to the test
 * framework when the test point is enabled.
 *
 * @param[in] ID The test point ID.
 * @param[in] COND The condition that should be true.
 * @param[in] EXPECTED The expected value.
 * @param[in] VALUE The value to check.
 * @param[in] TEST_ENABLED The test point enabled state.
 */
#define TEST_POINT_ASSERT_BYTE(ID, COND, EXPECTED, VALUE, TEST_ENABLED) { \
  if(TEST_ENABLED)                                                        \
  {                                                                       \
    TestFrameworkAssertByte(ID, COND, EXPECTED, VALUE);                   \
  }                                                                       \
}

/**
 * @brief Assert macro for unsigned 64-bit values.
 *
 * @details This macro reports the result of a udword assertion to the test
 * framework when the test point is enabled.
 *
 * @param[in] ID The test point ID.
 * @param[in] COND The condition that should be true.
 * @param[in] EXPECTED The expected value.
 * @param[in] VALUE The value to check.
 * @param[in] TEST_ENABLED The test point enabled state.
 */
#define TEST_POINT_ASSERT_UDWORD(ID, COND, EXPECTED, VALUE, TEST_ENABLED) { \
  if(TEST_ENABLED)                                                          \
  {                                                                         \
    TestFrameworkAssertUdword(ID, COND, EXPECTED, VALUE);                   \
  }                                                                         \
}

/**
 * @brief Assert macro for signed 64-bit values.
 *
 * @details This macro reports the result of a dword assertion to the test
 * framework when the test point is enabled.
 *
 * @param[in] ID The test point ID.
 * @param[in] COND The condition that should be true.
 * @param[in] EXPECTED The expected value.
 * @param[in] VALUE The value to check.
 * @param[in] TEST_ENABLED The test point enabled state.
 */
#define TEST_POINT_ASSERT_DWORD(ID, COND, EXPECTED, VALUE, TEST_ENABLED) {  \
  if(TEST_ENABLED)                                                          \
  {                                                                         \
    TestFrameworkAssertDword(ID, COND, EXPECTED, VALUE);                    \
  }                                                                         \
}

/**
 * @brief Assert macro for 32-bit floating point values.
 *
 * @details This macro reports the result of a float assertion to the test
 * framework when the test point is enabled.
 *
 * @param[in] ID The test point ID.
 * @param[in] COND The condition that should be true.
 * @param[in] EXPECTED The expected value.
 * @param[in] VALUE The value to check.
 * @param[in] TEST_ENABLED The test point enabled state.
 */
#define TEST_POINT_ASSERT_FLOAT(ID, COND, EXPECTED, VALUE, TEST_ENABLED) {  \
  if(TEST_ENABLED)                                                          \
  {                                                                         \
    TestFrameworkAssertFloat(ID, COND, EXPECTED, VALUE);                    \
  }                                                                         \
}

/**
 * @brief Assert macro for 64-bit floating point values.
 *
 * @details This macro reports the result of a double assertion to the test
 * framework when the test point is enabled.
 *
 * @param[in] ID The test point ID.
 * @param[in] COND The condition that should be true.
 * @param[in] EXPECTED The expected value.
 * @param[in] VALUE The value to check.
 * @param[in] TEST_ENABLED The test point enabled state.
 */
#define TEST_POINT_ASSERT_DOUBLE(ID, COND, EXPECTED, VALUE, TEST_ENABLED) { \
  if(TEST_ENABLED)                                                          \
  {                                                                         \
    TestFrameworkAssertDouble(ID, COND, EXPECTED, VALUE);                   \
  }                                                                         \
}

/**
 * @brief Assert macro for OS return codes.
 *
 * @details This macro reports the result of an OS return code assertion to
 * the test framework when the test point is enabled.
 *
 * @param[in] ID The test point ID.
 * @param[in] COND The condition that should be true.
 * @param[in] EXPECTED The expected value.
 * @param[in] VALUE The value to check.
 * @param[in] TEST_ENABLED The test point enabled state.
 */
#define TEST_POINT_ASSERT_RCODE(ID, COND, EXPECTED, VALUE, TEST_ENABLED) {  \
  if(TEST_ENABLED)                                                          \
  {                                                                         \
    TestFrameworkAssertErrCode(ID, COND, EXPECTED, VALUE);                  \
  }                                                                         \
}

/**
 * @brief Assert macro for pointer values.
 *
 * @details This macro reports the result of a pointer assertion to the test
 * framework when the test point is enabled.
 *
 * @param[in] ID The test point ID.
 * @param[in] COND The condition that should be true.
 * @param[in] EXPECTED The expected value.
 * @param[in] VALUE The value to check.
 * @param[in] TEST_ENABLED The test point enabled state.
 */
#define TEST_POINT_ASSERT_POINTER(ID, COND, EXPECTED, VALUE, TEST_ENABLED) {  \
  if(TEST_ENABLED)                                                            \
  {                                                                           \
    TestFrameworkAssertPointer(ID, COND, EXPECTED, VALUE);                    \
  }                                                                           \
}

/**
 * @brief Call a function only when testing is enabled.
 *
 * @details This macro invokes the provided function only if the supplied
 * test point enable flag is true.
 *
 * @param[in] FUNCTION_NAME The function to call.
 * @param[in] TEST_ENABLED The enabled state of the test point.
 */
#define TEST_POINT_FUNCTION_CALL(FUNCTION_NAME, TEST_ENABLED){              \
  if(TEST_ENABLED)                                                          \
  {                                                                         \
    FUNCTION_NAME();                                                        \
  }                                                                         \
}

/**
 * @brief Start the test framework.
 *
 * @details Initialize the test framework before running any test points.
 */
#define TEST_FRAMEWORK_START() TestFrameworkInit();

/**
 * @brief End the test framework.
 *
 * @details Finalize the test framework after all test points have run.
 */
#define TEST_FRAMEWORK_END() TestFrameworkEnd();

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
 * @brief Initialize the test framework.
 *
 * @details Prepare the test framework for use. This function must be called
 * before any test points or assertions are evaluated.
 */
void TestFrameworkInit(void);

/**
 * @brief Finalize the test framework.
 *
 * @details Clean up test framework resources after running test points. This
 * function should be called once testing is complete.
 */
void TestFrameworkEnd(void);

/**
 * @brief Assert an unsigned 32-bit value.
 *
 * @details Compare an expected uint32 value against an actual value and report
 * the result to the test framework.
 *
 * @param[in] kTestId The test point identifier.
 * @param[in] kCondition The condition to evaluate for the assertion.
 * @param[in] kExpected The expected unsigned 32-bit value.
 * @param[in] kValue The actual unsigned 32-bit value to compare.
 */
void TestFrameworkAssertUint(const uint32_t kTestId,
                             const bool     kCondition,
                             const uint32_t kExpected,
                             const uint32_t kValue);

/**
 * @brief Assert a signed 32-bit value.
 *
 * @details Compare an expected int32 value against an actual value and report
 * the result to the test framework.
 *
 * @param[in] kTestId The test point identifier.
 * @param[in] kCondition The condition to evaluate for the assertion.
 * @param[in] kExpected The expected signed 32-bit value.
 * @param[in] kValue The actual signed 32-bit value to compare.
 */
void TestFrameworkAssertInt(const uint32_t kTestId,
                            const bool     kCondition,
                            const int32_t  kExpected,
                            const int32_t  kValue);

/**
 * @brief Assert an unsigned 16-bit value.
 *
 * @details Compare an expected uint16 value against an actual value and report
 * the result to the test framework.
 *
 * @param[in] kTestId The test point identifier.
 * @param[in] kCondition The condition to evaluate for the assertion.
 * @param[in] kExpected The expected unsigned 16-bit value.
 * @param[in] kValue The actual unsigned 16-bit value to compare.
 */
void TestFrameworkAssertHuint(const uint32_t kTestId,
                              const bool     kCondition,
                              const uint16_t kExpected,
                              const uint16_t kValue);

/**
 * @brief Assert a signed 16-bit value.
 *
 * @details Compare an expected int16 value against an actual value and report
 * the result to the test framework.
 *
 * @param[in] kTestId The test point identifier.
 * @param[in] kCondition The condition to evaluate for the assertion.
 * @param[in] kExpected The expected signed 16-bit value.
 * @param[in] kValue The actual signed 16-bit value to compare.
 */
void TestFrameworkAssertHint(const uint32_t kTestId,
                             const bool     kCondition,
                             const int16_t  kExpected,
                             const int16_t  kValue);

/**
 * @brief Assert an unsigned 8-bit value.
 *
 * @details Compare an expected uint8 value against an actual value and report
 * the result to the test framework.
 *
 * @param[in] kTestId The test point identifier.
 * @param[in] kCondition The condition to evaluate for the assertion.
 * @param[in] kExpected The expected unsigned 8-bit value.
 * @param[in] kValue The actual unsigned 8-bit value to compare.
 */
void TestFrameworkAssertUbyte(const uint32_t kTestId,
                              const bool     kCondition,
                              const uint8_t  kExpected,
                              const uint8_t  kValue);

/**
 * @brief Assert a signed 8-bit value.
 *
 * @details Compare an expected int8 value against an actual value and report
 * the result to the test framework.
 *
 * @param[in] kTestId The test point identifier.
 * @param[in] kCondition The condition to evaluate for the assertion.
 * @param[in] kExpected The expected signed 8-bit value.
 * @param[in] kValue The actual signed 8-bit value to compare.
 */
void TestFrameworkAssertByte(const uint32_t kTestId,
                             const bool     kCondition,
                             const int8_t   kExpected,
                             const int8_t   kValue);

/**
 * @brief Assert an unsigned 64-bit value.
 *
 * @details Compare an expected uint64 value against an actual value and report
 * the result to the test framework.
 *
 * @param[in] kTestId The test point identifier.
 * @param[in] kCondition The condition to evaluate for the assertion.
 * @param[in] kExpected The expected unsigned 64-bit value.
 * @param[in] kValue The actual unsigned 64-bit value to compare.
 */
void TestFrameworkAssertUdword(const uint32_t kTestId,
                               const bool     kCondition,
                               const uint64_t kExpected,
                               const uint64_t kValue);

/**
 * @brief Assert a signed 64-bit value.
 *
 * @details Compare an expected int64 value against an actual value and report
 * the result to the test framework.
 *
 * @param[in] kTestId The test point identifier.
 * @param[in] kCondition The condition to evaluate for the assertion.
 * @param[in] kExpected The expected signed 64-bit value.
 * @param[in] kValue The actual signed 64-bit value to compare.
 */
void TestFrameworkAssertDword(const uint32_t kTestId,
                              const bool     kCondition,
                              const int64_t  kExpected,
                              const int64_t  kValue);

/**
 * @brief Assert a 32-bit floating-point value.
 *
 * @details Compare an expected float value against an actual value and report
 * the result to the test framework.
 *
 * @param[in] kTestId The test point identifier.
 * @param[in] kCondition The condition to evaluate for the assertion.
 * @param[in] kExpected The expected float value.
 * @param[in] kValue The actual float value to compare.
 */
void TestFrameworkAssertFloat(const uint32_t kTestId,
                              const bool     kCondition,
                              const float    kExpected,
                              const float    kValue);

/**
 * @brief Assert a 64-bit floating-point value.
 *
 * @details Compare an expected double value against an actual value and report
 * the result to the test framework.
 *
 * @param[in] kTestId The test point identifier.
 * @param[in] kCondition The condition to evaluate for the assertion.
 * @param[in] kExpected The expected double value.
 * @param[in] kValue The actual double value to compare.
 */
void TestFrameworkAssertDouble(const uint32_t kTestId,
                               const bool     kCondition,
                               const double   kExpected,
                               const double   kValue);

/**
 * @brief Assert an OS return code value.
 *
 * @details Compare an expected OS return code against an actual value and
 * report the result to the test framework.
 *
 * @param[in] kTestId The test point identifier.
 * @param[in] kCondition The condition to evaluate for the assertion.
 * @param[in] kExpected The expected OS return code.
 * @param[in] kValue The actual OS return code to compare.
 */
void TestFrameworkAssertErrCode(const uint32_t kTestId,
                                const bool     kCondition,
                                const E_Return kExpected,
                                const E_Return kValue);

/**
 * @brief Assert a pointer value.
 *
 * @details Compare an expected pointer value against an actual value and
 * report the result to the test framework.
 *
 * @param[in] kTestId The test point identifier.
 * @param[in] kCondition The condition to evaluate for the assertion.
 * @param[in] kExpected The expected pointer value.
 * @param[in] kValue The actual pointer value to compare.
 */
void TestFrameworkAssertPointer(const uint32_t  kTestId,
                                const bool      kCondition,
                                const uintptr_t kExpected,
                                const uintptr_t kValue);

#else /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* None */

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
#define TEST_POINT_ASSERT_UINT(ID, COND, EXPECTED, VALUE, TEST_ENABLED)

#define TEST_POINT_ASSERT_INT(ID, COND, EXPECTED, VALUE, TEST_ENABLED)

#define TEST_POINT_ASSERT_HUINT(ID, COND, EXPECTED, VALUE, TEST_ENABLED)

#define TEST_POINT_ASSERT_HINT(ID, COND, EXPECTED, VALUE, TEST_ENABLED)

#define TEST_POINT_ASSERT_UBYTE(ID, COND, EXPECTED, VALUE, TEST_ENABLED)

#define TEST_POINT_ASSERT_BYTE(ID, COND, EXPECTED, VALUE, TEST_ENABLED)

#define TEST_POINT_ASSERT_UDWORD(ID, COND, EXPECTED, VALUE, TEST_ENABLED)

#define TEST_POINT_ASSERT_DWORD(ID, COND, EXPECTED, VALUE, TEST_ENABLED)

#define TEST_POINT_ASSERT_FLOAT(ID, COND, EXPECTED, VALUE, TEST_ENABLED)

#define TEST_POINT_ASSERT_DOUBLE(ID, COND, EXPECTED, VALUE, TEST_ENABLED)

#define TEST_POINT_ASSERT_RCODE(ID, COND, EXPECTED, VALUE, TEST_ENABLED)

#define TEST_POINT_FUNCTION_CALL(FUNCTION_NAME, TEST_ENABLED)

#define TEST_FRAMEWORK_START()

#define TEST_FRAMEWORK_END()

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

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

#endif /* #ifndef __TEST_FRAMEWORK_TEST_FRAMEWORK_H_ */

/************************************ EOF *************************************/