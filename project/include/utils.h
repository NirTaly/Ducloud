/******************************************************************************
                              	   	 Utils
                                    Nir Tali
                                    25.12.18
 *****************************************************************************/

#include <cstdio>		/* FILE					*/
#include <cstring> 		/* strcmp				*/
#include <unistd.h>		/* close				*/

#include "driver_proxy.h"

#ifndef __UTILS_H__
#define __UTILS_H__

/******************************************************************************/
#define BITS_IN_BYTE	(8)
#define SET_BYTE 		(0xFF)
#define BYTES_IN_WORD	(sizeof(size_t))
#define ARRAY_SIZE(A)	(sizeof(A)/sizeof((A)[0]))
#define UNUSED_VAR(A)	((void)(A))
#define MAX(A,B)		((A) > (B) ? (A) : (B))
#define MIN(A,B)		((A) < (B) ? (A) : (B))

#ifndef NDEBUG
#define DBG(X) X
#else
#define DBG(X)
#endif

#define KB          (1024)
#define MB          (KB * KB)
#define GB          (KB * MB)
#define STORAGE_SIZE (200*MB)

#define SEC         (1000)
#define TIMEOUT     (3 * SEC)

/******************************************************************************/
						/* COLORS */
#define COLOR_RED     "\x1b[01;31m"
#define COLOR_GREEN   "\x1b[01;32m"
#define COLOR_YELLOW  "\x1b[01;33m"
#define COLOR_BLUE    "\x1b[01;34m"
#define COLOR_MAGENTA "\x1b[01;35m"
#define COLOR_CYAN    "\x1b[01;36m"
#define COLOR_RESET   "\x1b[0m"

#define LOG_RED(X) 	 { printf("%s\t\t\t%s%s",COLOR_RED,X,COLOR_RESET); }
#define LOG_GREEN(X) { printf("%s\t\t\t%s%s",COLOR_GREEN,X,COLOR_RESET); }

/******************************************************************************/
						/* Test Macro */
		/* Test Case For return val == VAL:*/
#define TEST(INDEX,EXPR,VAL,TEXT) { \
		if (VAL != EXPR) { \
			printf("%d) Test: " TEXT COLOR_RED "\t\t\tFAIL\n" COLOR_RESET, INDEX); \
			fprintf(stderr, "%s: %03d: %s: FAILED!\n", __FILE__, __LINE__, #EXPR); \
		} else { \
			printf("%d) Test " TEXT ":" COLOR_GREEN "\t\t\tTRUE!\n" COLOR_RESET, INDEX); \
		} }

		/* Test Case For return val != VAL:*/
#define NOT_EQUAL_TEST(INDEX,EXPR,VAL,TEXT) \
		if (VAL == EXPR) { \
			printf("%d) Test: " TEXT COLOR_RED "\t\t\tFAIL\n" COLOR_RESET, INDEX); \
			fprintf(stderr, "%s: %03d: %s: FAILED!\n", __FILE__, __LINE__, #EXPR); \
		} else { \
			printf("%d) Test " TEXT ":" COLOR_GREEN "\t\t\tTRUE!\n" COLOR_RESET, INDEX); \
		}

/* Format For Any Type */
#define TEST_BASE(INDEX, EXPR, VAL, TYPE, FORMAT, TEXT) { \
			TYPE val = VAL; TYPE expr = EXPR; \
		if (val != expr) { \
			printf("%d) Test: " TEXT COLOR_RED "\t\t\tFAIL\n" COLOR_RESET "Returned: " FORMAT \
				"\tExpected: " FORMAT "\n", INDEX, expr,val); \
			fprintf(stderr, "%s: %03d: %s: FAILED!\n", __FILE__, __LINE__, #EXPR); \
		} else { \
			printf("%d) Test " TEXT ":" COLOR_GREEN "\t\t\tTRUE!\n" COLOR_RESET, INDEX); \
		} }

/* comparisson between 2 strings */
#define TEST_STR(INDEX, EXPR, VAL, TEXT) TEST_BASE(INDEX, strcmp(EXPR,VAL), 0, \
			int, "%d", TEXT)

#define TEST_INT(INDEX, EXPR, VAL, TEXT) TEST_BASE(INDEX, EXPR, VAL, int, "%d", TEXT)
		
#define TEST_PTR(INDEX, EXPR, VAL, TEXT) TEST_BASE(INDEX, EXPR, VAL, \
			void*, "%p", TEXT)		

/* check is EXPR isn't 0 or NULL */
#define TEST_IS_TRUE(EXPR) { \
		if(!(EXPR)) { \
			fprintf(stderr, "%s: %03d: %s: FAILED!\n", __FILE__, __LINE__, #EXPR); \
				  } \
							}
/* check is EXPR is 0 or NULL */
#define TEST_FUNC_TRUE(EXPR) { \
		if(EXPR) { \
			fprintf(stderr, "%s: %03d: %s: FAILED!\n", __FILE__, __LINE__, #EXPR); \
				  } \
							}

#define TEST_TRUE(EXPR, VAL, I) { \
		if(EXPR != VAL) { \
			fprintf(stderr, "i =%d ,%s: %03d: %s: FAILED!\n",I, __FILE__, __LINE__, #EXPR); \
				  } \
							}

#define TEST_ARRAY(ARR1, ARR2, LENGTH) { \
		size_t i = 0; \
		while (ARR1[i] == ARR2[i] && i < LENGTH) {i++;} \
		if (i != LENGTH) \
				fprintf(stderr, "%s: %03d: Arrays Match: FAILED!\n" \
								"arr[%lu]\n", \
					__FILE__, __LINE__, i); \
										}

#define ERROR_HANDLER(VAR, VALUE, TEXT, EXIT_STATUS) { \
		if (VAR == VALUE) { \
		fprintf(stderr, TEXT "ERROR\n"); \
		return (EXIT_STATUS); \
						 } \
						 							 }											  
#define CHECK_VLG { \
	printf(COLOR_CYAN "Check Valgrind!" COLOR_RESET "\n"); \
					}

/******************************************************************************/
/******************************************************************************/

// File Descriptor Manager
class FD
{
public:
	explicit FD(int fd = -1) : m_fd(fd) { }
	~FD() { if (m_fd != -1) close(m_fd); }

	int GetFD() const { return m_fd; }

	FD& operator=(FD&& other) noexcept
	{
		if (m_fd != -1)
		{
			close(m_fd);
		}


		m_fd = other.m_fd;
		
		other.m_fd = -1;
		
		return *this;
	}
	operator int() { return m_fd; }

	FD(const FD&) = delete;
	FD& operator=(const FD&) = delete;
private:
	int m_fd;
};

/*****************************************************************************/



#endif //__UTILS_H__
