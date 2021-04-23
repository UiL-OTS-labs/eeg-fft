
#ifndef EDF_TEST_MACRO_S_H
#define EDF_TEST_MACRO_S_H

#include <stdio.h>
#include <CUnit/CUnit.h>

/**
 * \brief initialize the test suite.
 *
 * This macro assumes you have a static const char* called SUITE_NAME in the
 * same compilation unit that contains a descriptive name for the suite.
 *
 * It declares and defines a CU_pSuite called suite and registers the suite
 * with the CU_UNIT registry. Additionally it declares a CU_pTest called test.
 *
 * Further will any errors be printed to stderr and a error will be returned.
 * That can inform about what went wrong.
 *
 * @param init_func a function that sets up the resources for the suite.
 * @param finalize_func
 *
 * \private
 */
#define UNIT_SUITE_CREATE(init_func, finalize_func)                     \
    CU_pTest test;                                                          \
    CU_pSuite suite = CU_add_suite(SUITE_NAME, init_func, finalize_func);   \
    do {                                                                    \
        if (!suite) {                                                       \
            fprintf(stderr, "Unable to create suite %s:%s.\n",              \
                SUITE_NAME,                                                 \
                CU_get_error_msg()                                          \
                );                                                          \
            return CU_get_error();                                          \
        }                                                                   \
    } while(0)

/**
 * \brief add a test to the current test suite as created by the
 *        UNIT_SUITE_CREATE macro.
 * \param test_name, the test name must be the name of a function that does
 *        runs the actual test.
 */
#define UNIT_TEST_CREATE(test_name)                                     \
    do {                                                                    \
        test = CU_add_test(suite, #test_name, test_name);                   \
        if (!test){                                                         \
            fprintf(stderr, "Unable to create test %s:%s:%s",               \
                SUITE_NAME,                                                 \
                #test_name,                                                 \
                CU_get_error_msg()                                          \
                );                                                          \
            return CU_get_error();                                          \
        }                                                                   \
    } while (0)

#endif
