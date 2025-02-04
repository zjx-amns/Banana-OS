# include "debug.h"
# include "log.h"
# include "kernel/print.h"
# include "interrupt.h"

void panic_spin(char* filename, int line, const char* func, const char* condition) {
    intr_disable(); // disable interrupts 

    mylog("Error, something went wrong.\n", 4);
    
    put_str("FileName: ");
    put_str(filename);
    put_char('\n');

    put_str("Line: ");
    put_int(line);
    put_char('\n');

    put_str("Function: ");
    put_str(func);
    put_char('\n');

    put_str("Condition: ");
    put_str(condition);
    put_char('\n');

    while (1);
}