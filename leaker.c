/******************************************************************
 *
 * Leaker.c - the Leaker memory leak detection module
 *
 * ****************************************************************/

#ifdef USE_LEAKER    //   see the makefile 
# include "leaker.h"

# ifdef	_WIN32
#  ifdef WIN_FORCED_COLOR
#   include <windows.h>
	HANDLE hWinConsoleL = 0;
#   define LRED			SetConsoleTextAttribute(hWinConsoleL, 12)
#   define LYELLOW		SetConsoleTextAttribute(hWinConsoleL, 14)
#   define LCYAN		SetConsoleTextAttribute(hWinConsoleL, 11)
#   define LNORMAL1		SetConsoleTextAttribute(hWinConsoleL, 7)
#   define LNORMAL2		SetConsoleTextAttribute(hWinConsoleL, 7)
#  else  /*  not WIN_FORCED_COLOR    using escape vt sequence with ConEmu*/
#  	define LRED			l_fprintf(2,(char *)"\033[31;1m")
#  	define LYELLOW		l_fprintf(1,(char *)"\033[33;1m")
#  	define LCYAN		l_fprintf(1,(char *)"\033[36;1m")
#  	define LNORMAL1		l_fprintf(1,(char *)"\033[0m")
#  	define LNORMAL2		l_fprintf(2,(char *)"\033[0m")  
#  endif /*  WIN_FORCED_COLOR */
# else /*  not win32 */ 
#  define LRED			l_fprintf(2,(char *)"\033[31;1m")
#  define LYELLOW		l_fprintf(1,(char *)"\033[33;1m")
#  define LCYAN			l_fprintf(1,(char *)"\033[36;1m")
#  define LNORMAL1		l_fprintf(1,(char *)"\033[0m")
#  define LNORMAL2		l_fprintf(2,(char *)"\033[0m")  
# endif   /*  _WIN32 */

/* disable macros to use normal functions in special functions */
# undef malloc
# undef calloc
# undef realloc
# undef free

# ifdef __cplusplus
const char *_leaker_file = "unknown";
const char *_leaker_func = "unknown";
unsigned long _leaker_line = 0;

#  undef new
#  undef delete
# endif   /*  __cplusplus */

/* global table containing allocation information , used as extern var */
_HTABLE_T _leaker = { NULL, 0, 0, 0, 0, 0, 0, 0 };

/* global var to check program has not exit on error, used here only*/
static int g_exit;

/* internal function prototypes */
static void 	_Leaker_Init(void);
static void 	_Leaker_Grow(void);
static void 	_Leaker_Add(void *addr, size_t size, const char *alloc,
                        const char *file, const char *func, size_t line);
static size_t 	_Leaker_Remove(void *addr, const char *dealloc, const char *file,
                            const char *func, size_t line);
static _LEAK_T 	**_Leaker_Find(void *addr);
static unsigned long _Leaker_Hash(void *addr);
static int 		_Leaker_Check_Dealloc(const char *alloc, const char *dealloc);
static void 	_Leaker_Init_Guard(void *addr, size_t size);
static int 		_Leaker_Check_Guard(void *addr, size_t size);
static void 	_Leaker_Scribble(void *ptr, size_t size);
static void 	_Leaker_Print_Entry(_LEAK_T *entry);
static void 	_Leaker_Report(void);
static void 	_Leaker_Dump_Entry(_LEAK_T *entry);
static _LEAK_T 	**_Leaker_Build_List(void);
static int 		_Leaker_Compare_Entries(const void *first, const void *second);


void begin_0 (void)	// will be done before main
{
	signal(SIGSEGV, ft_segfault);
	signal(SIGILL, ft_sigill);
	signal(SIGABRT, ft_sigabrt);
	# ifndef  _WIN32
		signal(SIGBUS, ft_buserror);
		signal(SIGSYS, ft_sigsys);
		signal(SIGFPE , ft_sigfpe);
	# endif
	_Leaker_Init();
	g_exit = 1;    
}
 
void end_0 (void)	// will be done before exit
{
  	if ( g_exit== 1)
  	{
	  	if(_VERBOSE_LEAKER_ > 0)
       		_Leaker_Dump(); 
  		_Leaker_Report();
	}  			
}

static void win_err_handle()
{
	#ifdef	_WIN32
	# ifdef WIN_FORCED_COLOR
    	if (hWinConsoleL == 0)
    		hWinConsoleL = GetStdHandle(STD_ERROR_HANDLE);
    # endif		
	#endif
}

static size_t		l_strlen(const char *s)
{
	size_t i;

	i = 0;
	while (s[i] != '\0')
		i++;
	return (i);
}

static void	l_putchar_fd(char c, int fd)
{
	write(fd, &c, 1);
}

static void	l_putstr_fd(char const *s, int fd)
{
	int i;

	i = 0;
	while (s[i] != '\0')
	{
		l_putchar_fd((char)s[i], fd);
		i++;
	}
}


static void	l_putnbr_fd(int n, int fd)
{
	unsigned int i;

	if (n < 0)
	{
		i = -n;
		l_putchar_fd('-', fd);
	}
	else
		i = n;
	if (i >= 10)
	{
		l_putnbr_fd(i / 10, fd);
		l_putnbr_fd(i % 10, fd);
	}
	else
		l_putchar_fd(i + '0', fd);
}

static void	puthex_rec_fd(unsigned int number,int fd)
{
	char	c;
	char	map[] = "0123456789ABCDEF";

	if (number)
	{
		c = (char)(number & 0xF);
		number = (number >> 4);
		puthex_rec_fd(number,fd);
		l_putchar_fd(map[(int)c],fd);
	}
}

static void		my_puthex_fd(unsigned int number,int fd)
{
	l_putstr_fd("0x",fd);
	puthex_rec_fd(number,fd);
}

static void	l_fprintf(int fd, char *all, ...)
{
    va_list args;
    int i;
    va_start(args, all);
    i = 0;
    while(all[i])
    {
        if(all[i] == '%')
        {
            if(all[i + 1] == 's')
                l_putstr_fd(va_arg(args, char *), fd);
            else if(all[i + 1] == 'd' || all[i + 1] == 'i'|| all[i + 1] == 'u')
                l_putnbr_fd(va_arg(args, int), fd);
            else if(all[i + 1] == 'c')
                l_putchar_fd((char)va_arg(args, int), fd);
            else if(all[i + 1] == 'p')
            	my_puthex_fd((unsigned int)va_arg(args, int), fd);
            else
                l_putchar_fd(all[i + 1], fd);
            i++;
        }
        else
            l_putchar_fd(all[i], fd);
        i++;
    }
    va_end(args);
}

static void show_wait(char * s)
{
	char a;
	fflush(stdout);
	fflush(stderr);
 	if(_PLEASE_PAUSE_ == 1)
 	{
	 	write(1, "\n\n     =>  ", 11);
		write(1, s, l_strlen(s));
		write(1, "\n", 1);	
		read(0, &a, 1);
 	}
}

void		ft_segfault(int c)//SIGSEGV 
{	
	(void)c;
	fflush(stdout);
	fflush(stderr);
	LRED;
	l_putstr_fd("\n Error : SegFault \n",2);
	LNORMAL2;
	show_wait((char *)"Press Enter to continue ...");		
}

void		ft_sigill(int c)//SIGILL
{
	(void)c;
	fflush(stdout);
	fflush(stderr);
	LRED;
	l_putstr_fd("\n Error  SigIll  :  Non Valid Instruction \n",2);
	LNORMAL2;
	show_wait((char *)"Press Enter to continue ...");
}

void		ft_sigabrt(int c)//SIGABRT
{
	(void)c;
	fflush(stdout);
	fflush(stderr);
	LRED;
	l_putstr_fd("\n Error  SigAbrt :  abort  \n",2);
	LNORMAL2;
	show_wait((char *)"Press Enter to continue ...");
}

#ifndef  _WIN32 
void		ft_buserror(int c)//SIGBUS
{
	(void)c;
	fflush(stdout);
	fflush(stderr);
	LRED;
	l_putstr_fd("\n Error  BusError  \n",2);
	LNORMAL2;
	show_wait((char *)"Press Enter to continue ...");
}

void		ft_sigsys(int c)//SIGSYS
{
	(void)c;
	fflush(stdout);
	fflush(stderr);
	LRED;
	l_putstr_fd("\n Error  Bad system call :  abort  \n",2);
	LNORMAL2;
	show_wait((char *)"Press Enter to continue ...");
}

void		ft_sigfpe(int c)//SIGFPE
{
	(void)c;
	fflush(stdout);
	fflush(stderr);
	LRED;
	l_putstr_fd("\n Error  Erroneous arithmetic operation :  abort  \n",2);
	LNORMAL2;
	show_wait((char *)"Press Enter to continue ...");
}
#endif
	

/* dump current allocation information and statistics */
void _Leaker_Dump(void)
{
    size_t overflows = _leaker.overflows;
    win_err_handle();
	if(_leaker.count == 0)
    {
		LYELLOW; l_fprintf(1, (char *)"\nCURRENT LEAKS REPORT:\n\n"); LNORMAL1;
		LCYAN; l_fprintf(1, (char *)"     No mem allocations currently left!\n\n"); LNORMAL1;
        return;
    }
 
 	LRED; l_fprintf(2, (char *)"\nCURRENT LEAKS REPORT:\n\n"); LNORMAL2;
	_LEAK_T **table = _Leaker_Build_List();
 	unsigned int i;
  	LRED; l_fprintf(2, (char *)"%u allocations (%u bytes).\n\n",
   			_leaker.count, _leaker.bytes - _leaker.count * GUARD_SIZE); LNORMAL2;   
    for(i = 0; i < _leaker.count; i++)
    {
   		_Leaker_Dump_Entry(table[i]);
    }
    l_fprintf(2, (char *)"\n");    
    free(table);
    if(_leaker.mismatches)
        {LRED; l_fprintf(2, (char *)"Mismatches: %u allocation/deallocations don't match!\n",
                _leaker.mismatches); LNORMAL2;}
    if(_leaker.overflows)
        {LRED; l_fprintf(2, (char *)"Overflows: %u allocations overflowed (wrote off end)!\n",
                _leaker.overflows); LNORMAL2;}
    if(_leaker.bad_frees)
        {LRED; l_fprintf(2, (char *)"Bad deallocs: %u attempts made to deallocate unallocated pointers!\n",
                _leaker.bad_frees); LNORMAL2;}
    _leaker.overflows = overflows; /* restore current overflow count */
}

int get_leaker_info(int wait)
{
	if(_leaker.count == 0)
		return(0);
	_Leaker_Dump();
	if (wait==1)
		show_wait((char *)"Press Enter to continue ...");
	return(-1);		
}

/* replacement for malloc */
void *_malloc(size_t size, const char *file, const char *func,
                     unsigned long line)
{
	void *ptr;
	win_err_handle();
    size += GUARD_SIZE;
    
    if(!(ptr = malloc(size)))
    {
        LRED; l_fprintf(2,(char *)"%s:%s():%i aborting: malloc() failed!\n", __FILE__,
                __func__, __LINE__); LNORMAL2;
        g_exit = 0;        
        exit(2);
    }
    
    _Leaker_Init_Guard(ptr, size);
    _Leaker_Add(ptr, size, "malloc", file, func, line);
    return ptr;
}

/* replacement for calloc */
void *_calloc(unsigned long count, size_t size, const char *file,
                     const char *func, unsigned long line)
{
    void *ptr;
    win_err_handle();
    size = size * count + GUARD_SIZE;
    
    if(!(ptr = calloc(1, size)))
    {
        LRED; l_fprintf(2,(char *)"%s:%s():%i aborting: calloc() failed!\n", __FILE__,
                __func__, __LINE__); LNORMAL2;
        g_exit = 0;        
        exit(2);
    }
    
    _Leaker_Init_Guard(ptr, size);
    _Leaker_Add(ptr, size, "calloc", file, func, line);
    return ptr;
}

/* replacement for realloc */
void *_realloc(void *ptr, size_t size, const char *file,
                      const char *func, unsigned long line)
{
    void *ptr_new;
    size_t old_size = 0, len;
    win_err_handle();
    size += GUARD_SIZE;
    
    /* check if pointer given to realloc is valid */
    if(ptr && !(old_size = _Leaker_Remove(ptr, "realloc", file, func, line)))
    {
        ptr = NULL;
    }
    
    /* to help catch realloc errors, ensure each realloc is at a new address */
    if(!(ptr_new = malloc(size)))
    {
        LRED; l_fprintf(2,(char *)"%s:%s():%i aborting: realloc() failed!\n",
                __FILE__, __func__, __LINE__); LNORMAL2;
        g_exit = 0;        
        exit(2);
    }
    
    /* if realloc was given a valid pointer, copy over the old data */
    if(ptr)
    {
        if(old_size > size) len = size - GUARD_SIZE;
        else len = old_size - GUARD_SIZE;
    
        memcpy(ptr_new, ptr, len);
        
        _Leaker_Scribble(ptr, old_size);
        free(ptr);
    }
        
    _Leaker_Init_Guard(ptr_new, size);
    _Leaker_Add(ptr_new, size, "realloc", file, func, line);
    return ptr_new;
}

/* replacement for free */
void _free(void *ptr, const char *file, const char *func,
                     unsigned long line)
{
	size_t size;
	win_err_handle();
    if((size = _Leaker_Remove(ptr, "free", file, func, line)))
    {
        _Leaker_Scribble(ptr, size);
        free(ptr);
    }
}

#ifdef __cplusplus

/* replacement for operator new */
void* operator new (size_t size) throw (std::bad_alloc)
{
    void *ptr = NULL;
    size += GUARD_SIZE;
    win_err_handle();
    if(!(ptr = malloc(size)))
    {
        LRED; l_fprintf(2,(char *)"%s:%s():%i aborting: calloc() failed!\n",
                __FILE__, __func__, __LINE__); LNORMAL2;
        g_exit = 0;        
        exit(2);
    }
    
    _Leaker_Init_Guard(ptr, size);
    _Leaker_Add(ptr, size, "new", _leaker_file, _leaker_func, _leaker_line);

    /* in case new is called from library code where macro has not overriden
     * new and updated _leaker_file, _leaker_func, etc */
    _leaker_file = "unknown";
    _leaker_func = "unknown";
    _leaker_line = 0;

    return ptr;
}

/* replacement for operator vector new */
void* operator new [] (size_t size) throw (std::bad_alloc)
{
    void *ptr;
    size += GUARD_SIZE;
    win_err_handle();
    if(!(ptr = malloc(size)))
    {
        LRED; l_fprintf(2,(char *)"%s:%s():%i aborting: calloc failed!\n",
                __FILE__, __func__, __LINE__); LNORMAL2;
        g_exit = 0;        
        exit(2);
    }
    
    _Leaker_Init_Guard(ptr, size);
    _Leaker_Add(ptr, size, "new[]", _leaker_file, _leaker_func, _leaker_line);
    
    _leaker_file = "unknown";
    _leaker_func = "unknown";
    _leaker_line = 0;
    
    return ptr;
}

/* replacement for operator delete */
void operator delete(void* ptr) throw ()
{
    size_t size;
    win_err_handle();
    if((size = _Leaker_Remove(ptr, "delete", _leaker_file, _leaker_func,
                              _leaker_line)))
    {
        _Leaker_Scribble(ptr, size);
        free(ptr);
    }
}

/* replacement for operator vector delete */
void operator delete [] (void* ptr) throw ()
{
    size_t size;
    win_err_handle();
    if((size = _Leaker_Remove(ptr, "delete[]", _leaker_file, _leaker_func,
                              _leaker_line)))
    {
        _Leaker_Scribble(ptr, size);
        free(ptr);
    }    
}

#endif

/* end of user-visible functions */

/* initialize table */
static void _Leaker_Init(void)
{
	win_err_handle();
    fflush(stdout);
    fflush(stderr);
	_leaker.table = (_LEAK_T **) calloc(START_SIZE, sizeof(_LEAK_T *));
    if(!_leaker.table)
    {
        LRED; l_fprintf(2,(char *)"%s:%s():%i aborting: calloc() for table failed!\n",
                __FILE__, __func__, __LINE__); LNORMAL2;
        g_exit = 0;        
        exit(2);
    }
    _leaker.rows = START_SIZE;
    /* register so that leak information always displayed upon termination */
   //atexit(_Leaker_Report);
}

/* grow table to accommodate more entries (by factor of 4) */
static void _Leaker_Grow(void)
{
    unsigned int i;
    win_err_handle();
    _HTABLE_T old = _leaker;
    _leaker.rows *= 4;
    _leaker.count = 0;
    _leaker.bytes = 0;
    
    _leaker.table = (_LEAK_T **) calloc(_leaker.rows, sizeof(_LEAK_T *));
    if(!_leaker.table)
    {
        LRED; l_fprintf(2,(char *)"%s:%s():%i aborting: calloc() for table failed!\n",
                __FILE__, __func__, __LINE__); LNORMAL2;
        g_exit = 0;        
        exit(2);
    }    
    
    /* remove every entry from the old table and add it to the new */
    for(i = 0; i < old.rows; i++)
    {
        _LEAK_T *mover = old.table[i];
        while(mover)
        {
            _LEAK_T *temp = mover;
            mover = mover->next;
            _leaker.serial = temp->sequence;
            _Leaker_Add(temp->addr, temp->size, temp->alloc,
                        temp->file, temp->func, temp->line);
            free(temp);
        }
    }
    _leaker.serial = old.serial;
    free(old.table);
}

/* add a new allocation to the table */
void _Leaker_Add(void *addr, size_t size, const char *alloc,
                        const char *file, const char *func, size_t line)
{
    _LEAK_T **mover, *temp;
    win_err_handle();
    if(!_leaker.table) _Leaker_Init();
    if(_leaker.count > _leaker.rows / 2) _Leaker_Grow();
    
    mover = &(_leaker.table[_Leaker_Hash(addr)]);
    while(*mover)
    {
        if((*mover)->addr == addr) break;
        
        mover = &((*mover)->next);
    }
    
    if(*mover) /* if an address is allocated twice, we are in trouble! */
    {
        LRED; l_fprintf(2, (char *)"%s:%s():%u fatal error: address %p already in use!\n",
                file, func, line, addr); LNORMAL2;
        g_exit = 0;        
        exit(2);
    }
    
    if(!(temp = (_LEAK_T *)malloc(sizeof(_LEAK_T))))
    {
       LRED; l_fprintf(2,(char *)"%s:%s():%i aborting: malloc() for entry failed!\n",
                __FILE__, __func__, __LINE__); LNORMAL2;
        g_exit = 0;        
        exit(2);
    }
    
    temp->addr = addr;
    temp->size = size;
    temp->sequence = _leaker.serial++;
    temp->alloc = alloc;
    temp->file = file;
    temp->func = func;
    temp->line = line;
    temp->next = NULL;
    
    *mover = temp;
    
    _leaker.count++;
    _leaker.bytes += size;
}

/* remove an allocation from the table, and report any inconsistencies */
size_t _Leaker_Remove(void *addr, const char *dealloc, const char *file,
                            const char *func, size_t line)
{
    _LEAK_T **mover, *temp;
    size_t size;
    win_err_handle();
    if(!_leaker.table) _Leaker_Init();
    
    mover = _Leaker_Find(addr);
    
    if(!*mover) /* given a bad pointer */
    {
        LRED; l_fprintf(2,(char *)"\nLEAKS : %s:%s():%u %s error: pointer was not allocated!\n\n",
                file, func, line, dealloc); LNORMAL2;
        _leaker.bad_frees++;
        return 0;
    }
    
    temp = *mover;
    *mover = temp->next;
    _leaker.count--;
    _leaker.bytes -= temp->size;
    
    size = temp->size;
        
    if(!_Leaker_Check_Guard(addr, temp->size)) /* guard overwritten */
    {
        LRED; l_fprintf(2, (char *)"\nLEAKS : %s:%s():%u checking error: wrote off end of memory allocated at %s:%s():%u.\n\n",
                file, func, line, temp->file, temp->func, temp->line); LNORMAL2;
        _leaker.overflows++;
    }
    if(!_Leaker_Check_Dealloc(temp->alloc, dealloc)) /* wrong dealloc function */
    {
        LRED; l_fprintf(2, (char *)"\nLEAKS : %s:%s():%u mismatch error: memory allocated at %s:%s():%u with %s, deallocated with %s.\n\n",
                file, func, line, temp->file, temp->func, temp->line,
                temp->alloc, dealloc); LNORMAL2;
        _leaker.mismatches++;
    }
        
    free(temp);
    return size;
}

/* find a pointer in the table, return a pointer to a pointer to it */
static _LEAK_T **_Leaker_Find(void *addr)
{
    win_err_handle();
	_LEAK_T **mover = &(_leaker.table[_Leaker_Hash(addr)]);
    while(*mover)
    {
        if((*mover)->addr == addr) break;
        
        mover = &((*mover)->next);
    }
    return mover;
}

/* Thomas Wang's 64-bit hash function - works well for integers, and is
 * significantly faster than the DJB function since.  It is also slightly
 * better in distributing keys.
 * http://www.concentric.net/~Ttwang/tech/inthash.htm
 */
static unsigned long _Leaker_Hash(void *addr)
{
    win_err_handle();
    #ifdef __x86_64__
    	unsigned long long address1 = (unsigned long long) addr;
    	unsigned long address = (unsigned long) address1;
    #else
    	unsigned long address = (unsigned long) addr;
    #endif
	address = (~address) + (address << 21); /* (a << 21) - a - 1; */
	address = address ^ (address >> 24);
	address = (address + (address << 3)) + (address << 8); /* a * 265 */
	address = address ^ (address >> 14);
	address = (address + (address << 2)) + (address << 4); /* a * 21 */
	address = address ^ (address >> 28);
	address = address + (address << 31);
	return address % _leaker.rows;
}

/* return 1 if the allocator and deallocator are compatible, 0 otherwise */
static int _Leaker_Check_Dealloc(const char *alloc, const char *dealloc)
{
    win_err_handle();
	if((strcmp(dealloc, "free") == 0) || (strcmp(dealloc, "realloc") == 0))
    {
        if     (strcmp(alloc,  "malloc") == 0) return 1;
        else if(strcmp(alloc,  "calloc") == 0) return 1;
        else if(strcmp(alloc, "realloc") == 0) return 1;
        else return 0;
    }
    if((strcmp(alloc, "new"  ) == 0) && (strcmp(dealloc, "delete"  ) == 0))
        return 1;
    if((strcmp(alloc, "new[]") == 0) && (strcmp(dealloc, "delete[]") == 0))
        return 1;
    
    return 0;
}

/* initialize the guard at the end of the allocation */
static void _Leaker_Init_Guard(void *addr, size_t size)
{
	win_err_handle();
	strncpy((char *)addr + size - GUARD_SIZE, GUARD_STR, GUARD_SIZE);
}

/* verify the guard at the end of the allocation, return 1 if successful */
static int _Leaker_Check_Guard(void *addr, size_t size)
{    
    win_err_handle();
	return (strncmp((char *)addr + size - GUARD_SIZE, GUARD_STR, GUARD_SIZE)
                == 0);
}

/* zap a give allocation - mainly useful to make later mistakes easier to see */
static void _Leaker_Scribble(void *ptr, size_t size)
{
    win_err_handle();
	memset(ptr, '\0', size);
}

/* report leaks and errors, and deallocate all remaining memory */
static void _Leaker_Report(void)
{
    win_err_handle();
    
	if(!(_leaker.count || _leaker.mismatches || _leaker.overflows
         || _leaker.bad_frees))
    {
        if(_VERBOSE_LEAKER_ == 1)
        {
        	LYELLOW; l_fprintf(1, (char *)"\n\n*******EXIT LEAKS REPORT: \n");
			LCYAN; l_fprintf(1, (char *)"      No error found at normal exit, good job !!!\n\n");
        	l_fprintf(1, (char *)"      No mem allocations currently left.\n\n");LNORMAL1;
			show_wait((char *)"Press Enter to exit ...");
        }
		if(_leaker.table) free(_leaker.table);
        return;
    }
    else
    	fflush(stdout);
    LYELLOW; l_fprintf(2, (char *)"\n\n*******EXIT LEAKS REPORT:");
	LRED; l_fprintf(2, (char *)"  Errors found!\n\n"); LNORMAL2;
    
    if(_leaker.count) /* print out list of leaks, clean up */
    {
        unsigned int i;
        _LEAK_T **table = _Leaker_Build_List();
        
        LRED; l_fprintf(2, (char *)"Leaks found: %u allocations (%u bytes).\n",
                _leaker.count, _leaker.bytes - _leaker.count * GUARD_SIZE);LNORMAL2;
        
        for(i = 0; i < _leaker.count; i++)
        {
            _Leaker_Print_Entry(table[i]);
            free(table[i]->addr);
            free(table[i]);
        }
        
        l_fprintf(2, (char *)"\n");
        free(table);
    }
    
	if(_leaker.table) free(_leaker.table);
    
    /* report other errors */
    if(_leaker.mismatches)
        {LRED; l_fprintf(2, (char *)"Mismatches: %u allocation/deallocations don't match.\n",
                _leaker.mismatches); LNORMAL2;}
    
    if(_leaker.overflows)
        {LRED; l_fprintf(2, (char *)"Overflows: %u allocations overflowed (wrote off end).\n",
                _leaker.overflows); LNORMAL2;}
    if(_leaker.bad_frees)
        {LRED;l_fprintf(2, (char *)"Bad deallocs: %u attempts made to deallocate unallocated pointers.\n",
                _leaker.bad_frees); LNORMAL2;}
    show_wait((char *)"Press Enter to exit ...");
}

/* print the given entry */
static void _Leaker_Print_Entry(_LEAK_T *entry)
{
    win_err_handle();
	LRED; l_fprintf(2, (char *)"%s:%s():%u memory leak: memory was not deallocated.\n",
            entry->file, entry->func, entry->line); LNORMAL2;
    if(!_Leaker_Check_Guard(entry->addr, entry->size))
    {
        LRED; l_fprintf(2, (char *)"%s:%s():%u checking error: wrote off end of allocation.\n",
                entry->file, entry->func, entry->line); LNORMAL2;
        _leaker.overflows++;
    }
}

/* dump the given entry */
static void _Leaker_Dump_Entry(_LEAK_T *entry)
{
    win_err_handle();
	LRED; l_fprintf(2, (char *)"%s:%s():%u address: %p bytes: %u",
            entry->file, entry->func, entry->line, entry->addr,
            entry->size - GUARD_SIZE); LNORMAL2;
    if(!_Leaker_Check_Guard(entry->addr, entry->size))
    {
        LRED; l_fprintf(2, (char *)" OVERFLOWED.\n"); LNORMAL2;
        _leaker.overflows++;
    }
    else
        {LRED; l_fprintf(2, (char *)".\n"); LNORMAL2;}
}

/* compare two leak entries based upon their sequence number */
static int _Leaker_Compare_Entries(const void *first, const void *second)
{
    win_err_handle();
	const _LEAK_T *f = *(_LEAK_T **)first, *s = *(_LEAK_T **)second;
    
    return (int)(f->sequence - s->sequence);
}

/* build and return a sorted list of leak entry pointers */
static _LEAK_T **_Leaker_Build_List(void)
{
    win_err_handle();
	_LEAK_T **table, **mover;
    unsigned int i;

    if(!_leaker.table || !_leaker.count) return NULL;
    
    if(!(table = (_LEAK_T **)malloc(sizeof(_LEAK_T *) * _leaker.count)))
    {
        LRED; l_fprintf(2,(char *)"%s:%s():%i aborting: malloc failed!\n",
                __FILE__, __func__, __LINE__); LNORMAL2;
       	g_exit = 0;        
        exit(2);
    }
    mover = table;
    for(i = 0; i < _leaker.rows; i++)
    {
        _LEAK_T *temp = _leaker.table[i];
        
        while(temp)
        {
            *mover = temp;
            mover++;
            temp = temp->next;
        }
    }
    qsort((void *)table, _leaker.count, sizeof(_LEAK_T *), _Leaker_Compare_Entries);
    return table;
}

#endif		// USE_LEAKER 

