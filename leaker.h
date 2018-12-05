/*******************************************************************************
 *
 * Leaker.h - defines for the Leaker memory leak detection module
 *
 * should be used in all modules of program needing memory allocation
 * in practice in c :	add "leaker.h" (when needed in module)  
 *						or replace <stdlib.h> by "leaker.h" in your includes
 * 						taking care of the path of leaker.h
 *						and compile/link the module leaker.c with other .c files 
 ******************************************************************************/
#ifndef _LEAKER_H
# define _LEAKER_H
# include <stdarg.h>
# include <unistd.h>
# include <stdlib.h>
# include <string.h>
# include <stdio.h>  // needed for fflush
# include <signal.h>

# ifdef __cplusplus
#  include <memory>
# endif


/*******************************************************************************
 * Options : comment or uncomment  under the define(s) if you want some option
 ******************************************************************************/

# define	VERBOSE_MODE	// to show info at exit even without error nor leaks

//# define	PAUSE_MODE		// to pause when failed

# ifdef	_WIN32
//#   define WIN_FORCED_COLOR	// to force win color way under normal console
# endif 

/*******************************************************************************
 * Options : add the define if you want 
 ******************************************************************************/
# ifdef	VERBOSE_MODE		// shows info at exit even without error nor leaks
#  ifdef	_VERBOSE_LEAKER_
#   undef	_VERBOSE_LEAKER_	
#  endif
#  define 	_VERBOSE_LEAKER_		1    
# else 						// shows only errors/leaks at exit, else do nothing
#  ifdef	_VERBOSE_LEAKER_
#  	undef 	_VERBOSE_LEAKER_
#  endif
#  define 	_VERBOSE_LEAKER_		0 
# endif   

# ifdef	PAUSE_MODE			// pauses when failed
#  ifdef	_PLEASE_PAUSE_
#   undef	_PLEASE_PAUSE_	
#  endif
#  define 	_PLEASE_PAUSE_		1    
# else 
#  ifdef	_PLEASE_PAUSE_
#  	undef 	_PLEASE_PAUSE_
#  endif
#  define 	_PLEASE_PAUSE_		0 
# endif   


/*******************************************************************************
 * please no change after
 ******************************************************************************/

# define START_SIZE  128     /* Initial size of memory allocation table */

# define GUARD_SIZE  4       /* Padding at the end of each allocated block */
# define GUARD_STR   "\014\033\014\033"  /* magic string to pad allocation */

typedef struct _LEAK_T
{
    void *addr;             /* address of memory allocated              */
    const char *alloc;      /* type of allocator used                   */
    size_t size;            /* size of allocated memory (bytes)         */
    size_t sequence;		/* relative position of allocation in code  */
    const char *file;       /* name of file where allocation made       */
    const char *func;       /* name of function where allocation made   */
    size_t line;			/* line number where allocation made        */
    struct _LEAK_T *next;   /* linked list pointer                      */
} _LEAK_T;

typedef struct
{
    _LEAK_T **table;
    size_t rows;			/* number of rows in table                */
    size_t count;			/* number of entries in table             */
    size_t bytes;			/* number of bytes currently allocated    */
    size_t serial;			/* number of next insertion               */
    
    size_t overflows;		/* number of incorrect deallocations      */
    size_t mismatches;		/* number of mismatched allocs/deallocs   */
    size_t bad_frees;		/* number of bad attempts to free         */
} _HTABLE_T;

extern _HTABLE_T _leaker;


void begin_0(void) __attribute__((constructor (101))); // execute before main
void end_0(void) __attribute__((destructor (101)));	// execute before exit

/* report information on current memory allocations */
void 	_Leaker_Dump(void);
/* report information on current memory allocations and wait id wait =1*/
int 	get_leaker_info(int wait);

void	ft_segfault(int);
void	ft_sigill(int);
void	ft_sigabrt(int);
# ifndef  _WIN32
void	ft_sigsys(int);
void	ft_sigfpe(int);
void	ft_buserror(int);
# endif

/* replacement for standard C allocation and deallocation functions */
void *_malloc(size_t size, const char *file, const char *func,
					unsigned long line);
void *_calloc(unsigned long count, size_t size, const char *file,
					const char *func, unsigned long line);
void *_realloc(void *ptr, size_t size, const char *file,
					const char *func, unsigned long line);
void _free(void *ptr, const char *file, const char *func,
					unsigned long line);
/* preprocessor magic to override built-in allocation functions with our own */
# define malloc(size)		_malloc(size, __FILE__, __func__, __LINE__)
# define calloc(n, size)	_calloc(n, size, __FILE__, __func__, __LINE__)
# define free(ptr)			_free(ptr, __FILE__, __func__, __LINE__)
# define realloc(ptr, size)	_realloc(ptr, size, __func__, __FILE__, __LINE__)

# ifdef __cplusplus

/* hackish solution to the problem of overriding C++ operator new/delete */
extern const char *_leaker_file;
extern const char *_leaker_func;
extern unsigned long _leaker_line;

#  define new (_leaker_file=__FILE__, _leaker_func=__func__, \
    _leaker_line=__LINE__) && 0 ? NULL : new
#  define delete _leaker_file=__FILE__, _leaker_func=__func__, \
    _leaker_line=__LINE__, delete

# endif   /*  __cplusplus  */

#endif  /*  _LEAKER_H  */ 
