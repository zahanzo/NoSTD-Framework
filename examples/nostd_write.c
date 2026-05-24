#include "nostd_sys.h"

long _start() {
    sys_print_str("[+] Raw syscall executed. Userland abstractions eradicated!!\n");
    sys_exit(0);
}