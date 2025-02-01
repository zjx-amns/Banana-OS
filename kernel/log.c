#include "log.h"
#include "kernel/print.h"

int type_return(int type)
{
    switch (type)
    {
    case 1:
        put_str("[GOOD] ");
        break;
    case 2:
        put_str("[INFO] ");
        break;
    case 3:
        put_str("[WARN] ");
        break;
    case 4:
        put_str("[ERRO] ");
        break;
    case 5:
        put_str("[INIT] ");
        break;
    default:
        put_str("unknown type\n");
        return 0;
    }
    return 1;
}

void mylog(const char *message, int type)
{
    if (type_return(type))
        put_str(message);
}

void mylog2(const char *message, int type, const char *name){
    if (type_return(type)) {
        put_char('[');
        put_str(name);
        put_str("] ");
        put_str(message);
    }
}
