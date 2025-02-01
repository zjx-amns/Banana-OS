#include "global.h"
#include "kernel/print.h"
#include "memory.h"
#include "thread.h"
#include "kernel/list.h"
#include "string.h"
#include "timer.h"
#include "io.h"
#include "stdint.h"
#include "log.h"
#include "sync.h"
#include "interrupt.h"

#define NULL ((void *)0)

#define LOG_OK 1
#define LOG_INFO 2
#define LOG_WARN 3
#define LOG_ERR 4
#define LOG_INIT 5