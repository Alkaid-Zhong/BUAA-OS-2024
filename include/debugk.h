#ifndef _DBGK_H_
#define _DBGK_H_
#include <printk.h>
#define DEBUGK // 可注释

#ifdef DEBUGK
#define DEBUG_OUTPUT 1
#else
#define DEBUG_OUTPUT 0
#endif
#endif // !_DBGK_H_