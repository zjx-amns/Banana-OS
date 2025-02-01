#ifndef _LOG_H_
#define _LOG_H_

#include "kernel/print.h"

/*
1: ok
2: info
3: warning
4: error
5: init
*/

int type_return(int type);

void mylog(const char*,int type);
void mylog2(const char*,int type, const char*);

#endif // _LOG_H_