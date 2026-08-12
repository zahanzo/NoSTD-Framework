#include "../include/nostd_sys.h"

// --- 1. THE NAKED BOOTSTRAPPER (ENTRY POINT) / O BOOTSTRAPPER PURO (PONTO DE ENTRADA) ---

// Intercepts the raw hardware handoff, extracts arguments, aligns the stack
// to 16 bytes (System V ABI), and transitions to our clean C environment.
// Intercepta a transição bruta do hardware, extrai os argumentos, alinha a pilha
// para 16 bytes (System V ABI) e faz a transição para o nosso ambiente C limpo.
__asm__(
    ".text\n"
    ".global _start\n"
    "_start:\n"
    "    pop %rdi\n"          // Extracts argc into RDI / Extrai o argc para RDI
    "    mov %rsp, %rsi\n"    // Extracts argv into RSI / Extrai o argv para RSI
    "    and $-16, %rsp\n"    // 16-byte stack alignment (CRITICAL) / Alinhamento de 16 bytes da pilha (CRÍTICO)
    "    call nostd_main\n"   // Jumps to C execution / Salta para a execução em C
    "    mov %rax, %rdi\n"    // Captures exit code from nostd_main / Captura o código de saída do nostd_main
    "    mov $60, %rax\n"     // SYS_EXIT syscall number / Número da syscall SYS_EXIT
    "    syscall\n"           // Clean termination / Encerramento limpo
);

// --- 2. THE CLEAN C ENVIRONMENT / O AMBIENTE C LIMPO ---
long nostd_main(long argc, char **argv) {
    
    sys_print_str("\n=== NoSTD Framework Execution Engine ===\n\n");

    // --- MODULE 1: Arguments & Console I/O / MÓDULO 1: Argumentos e E/S de Console ---
    sys_print_str("[*] MODULE 1: Reading Raw Stack Arguments\n");
    for (long i = 0; i < argc; i++) {
        sys_print_str("    -> ");
        sys_print_str(argv[i]);
        sys_print_str("\n");
    }

    // --- MODULE 2: File System Operations / MÓDULO 2: Operações de Sistema de Arquivos ---
    sys_print_str("\n[*] MODULE 2: Dropping Payload to Disk\n");
    
    long fd = sys_open("nostd_artifact.txt", O_WRONLY | O_CREAT | O_TRUNC, FILE_MODE);
    if (fd > 0) {
        char *file_msg = "Hello from Ring 0! No glibc was harmed in the making of this file.\n";
        sys_write(fd, file_msg, nostd_strlen(file_msg));
        sys_close(fd);
        sys_print_str("    -> File 'nostd_artifact.txt' created successfully.\n");
    }

    // --- MODULE 3: Virtual Memory Management / MÓDULO 3: Gerenciamento de Memória Virtual ---
    sys_print_str("\n[*] MODULE 3: Direct Memory Allocation (mmap)\n");
    
    // Requests 1 page (4096 bytes) directly from the OS
    // Solicita 1 página (4096 bytes) diretamente do SO
    char *buffer = (char *)sys_mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    char *msg = "Memory OK!\n";
    nostd_memcpy(buffer, msg, nostd_strlen(msg) + 1);
    
    sys_print_str("    -> Mapped Page Output: ");
    sys_print_str(buffer);
    
    // Returns the memory to the OS
    // Devolve a memória para o SO
    sys_munmap(buffer, 4096);

    // --- MODULE 4: Process & Time Management / MÓDULO 4: Gerenciamento de Processos e Tempo ---
    sys_print_str("\n[*] MODULE 4: Forking and Nanosleeping\n");
    
    // Uses the Universal Macro for raw syscalls
    // Usa a Macro Universal para chamadas de sistema brutas
    long pid = syscall(SYS_FORK);
    
    if (pid == 0) {
        // --- CHILD PROCESS / PROCESSO FILHO ---
        sys_print_str("    -> Child Process: I am alive! Sleeping for 2 seconds...\n");
        
        struct timespec sleep_req = {2, 0}; 
        syscall(SYS_NANOSLEEP, (unsigned long)&sleep_req, 0);
        
        sys_print_str("    -> Child Process: Waking up and terminating cleanly.\n");
        sys_exit(42); 
    } else {
        // --- PARENT PROCESS / PROCESSO PAI ---
        sys_print_str("    -> Parent Process: Waiting for Child to finish...\n");
        
        long child_status;
        syscall(SYS_WAIT4, pid, (unsigned long)&child_status, 0, 0);
        
        sys_print_str("    -> Parent Process: Child terminated. Proceeding to shutdown.\n");
    }

    sys_print_str("\n=== Execution Complete. Entering the Void. ===\n");
    
    // Automatically becomes the exit code for the assembly bootstrapper
    // Torna-se automaticamente o código de saída para o bootstrapper em assembly
    return 0; 
}