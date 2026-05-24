#include "../include/nostd_sys.h"

// ---------------------------------------------------------------------
// 1. The Naked Bootstrapper (Entry Point)
// ---------------------------------------------------------------------
// Intercepts the raw hardware handoff, extracts arguments, aligns 
// the stack to 16 bytes, and transitions to our clean C environment.
__asm__(
    ".text\n"
    ".global _start\n"
    "_start:\n"
    "    pop %rdi\n"          // Extract argc into RDI
    "    mov %rsp, %rsi\n"    // Extract argv into RSI
    "    and $-16, %rsp\n"    // 16-byte stack alignment (CRITICAL)
    "    call nostd_main\n"   // Jump to C execution
    "    mov %rax, %rdi\n"    // Capture exit code from nostd_main
    "    mov $60, %rax\n"     // SYS_EXIT
    "    syscall\n"           // Clean termination
);

// ---------------------------------------------------------------------
// 2. The Clean C Environment
// ---------------------------------------------------------------------
long nostd_main(long argc, char **argv) {
    
    sys_print_str("\n=== NoSTD Framework Execution Engine ===\n\n");

    // ==========================================
    // MODULE 1: Arguments & Console I/O
    // ==========================================
    sys_print_str("[*] MODULE 1: Reading Raw Stack Arguments\n");
    for (long i = 0; i < argc; i++) {
        sys_print_str("    -> ");
        sys_print_str(argv[i]);
        sys_print_str("\n");
    }

    // ==========================================
    // MODULE 2: File System Operations
    // ==========================================
    sys_print_str("\n[*] MODULE 2: Dropping Payload to Disk\n");
    
    long fd = sys_open("nostd_artifact.txt", O_WRONLY | O_CREAT | O_TRUNC, FILE_MODE);
    if (fd > 0) {
        char *file_msg = "Hello from Ring 0! No glibc was harmed in the making of this file.\n";
        sys_write(fd, file_msg, nostd_strlen(file_msg));
        sys_close(fd);
        sys_print_str("    -> File 'nostd_artifact.txt' created successfully.\n");
    }

    // ==========================================
    // MODULE 3: Virtual Memory Management
    // ==========================================
    sys_print_str("\n[*] MODULE 3: Direct Memory Allocation (mmap)\n");
    
    // Requesting 1 page (4096 bytes) directly from the OS
    char *buffer = (char *)sys_mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    char *msg = "Memory OK!\n";
    nostd_memcpy(buffer, msg, nostd_strlen(msg) + 1);
    
    sys_print_str("    -> Mapped Page Output: ");
    sys_print_str(buffer);
    
    // Returning the memory to the OS
    sys_munmap(buffer, 4096);

    // ==========================================
    // MODULE 4: Process & Time Management
    // ==========================================
    sys_print_str("\n[*] MODULE 4: Forking and Nanosleeping\n");
    
    // Using the Universal Macro for raw syscalls
    long pid = syscall(SYS_FORK);
    
    if (pid == 0) {
        // --- CHILD PROCESS ---
        sys_print_str("    -> Child Process: I am alive! Sleeping for 2 seconds...\n");
        
        struct timespec sleep_req = {2, 0}; 
        syscall(SYS_NANOSLEEP, (unsigned long)&sleep_req, 0);
        
        sys_print_str("    -> Child Process: Waking up and terminating cleanly.\n");
        sys_exit(42); 
    } 
    else {
        // --- PARENT PROCESS ---
        sys_print_str("    -> Parent Process: Waiting for Child to finish...\n");
        
        long child_status;
        syscall(SYS_WAIT4, pid, (unsigned long)&child_status, 0, 0);
        
        sys_print_str("    -> Parent Process: Child terminated. Proceeding to shutdown.\n");
    }

    sys_print_str("\n=== Execution Complete. Entering the Void. ===\n");
    
    return 0; // Automatically becomes the exit code for the assembly bootstrapper
}