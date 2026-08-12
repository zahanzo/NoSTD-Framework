#ifndef NOSTD_SYS_H
#define NOSTD_SYS_H

#include "nostd_syscall_defs.h"

// --- 1. FUNDAMENTAL TYPES / TIPOS FUNDAMENTAIS ---
// Replacing <stddef.h> and <stdint.h> dependencies
// Substituindo as dependências de <stddef.h> e <stdint.h>

typedef unsigned long size_t;
typedef long ssize_t;
#define NULL ((void*)0)

// Kernel time structure (Required for sys_nanosleep)
// Estrutura de tempo do kernel (Necessária para o sys_nanosleep)
struct timespec {
    long tv_sec;        // Seconds / Segundos
    long tv_nsec;       // Nanoseconds / Nanosegundos
};

// --- 2. KERNEL FLAGS & CONSTANTS / FLAGS E CONSTANTES DO KERNEL ---

// File System Flags (sys_open)
// Flags de Sistema de Arquivos (sys_open)
#define O_RDONLY      00
#define O_WRONLY      01
#define O_RDWR        02
#define O_CREAT     0100
#define O_TRUNC    01000
#define FILE_MODE   0644

// Memory Protection Flags (sys_mmap / sys_mprotect)
// Flags de Proteção de Memória (sys_mmap / sys_mprotect)
#define PROT_NONE     0x0
#define PROT_READ     0x1
#define PROT_WRITE    0x2
#define PROT_EXEC     0x4

// Memory Mapping Flags
// Flags de Mapeamento de Memória
#define MAP_SHARED    0x01
#define MAP_PRIVATE   0x02
#define MAP_ANONYMOUS 0x20

// --- 3. PURE C UTILITIES / UTILITÁRIOS EM C PURO ---
// Replacing <string.h> standard library functions
// Substituindo as funções da biblioteca padrão <string.h>

// Calculates the length of a null-terminated string
// Calcula o comprimento de uma string terminada em nulo
static inline size_t nostd_strlen(const char *str) {
    const char *s = str;
    while (*s) s++;
    return (size_t)(s - str);
}

// Fills a block of memory with a specific byte value
// Preenche um bloco de memória com um valor de byte específico
static inline void *nostd_memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

// Copies a block of memory from source to destination
// Copia um bloco de memória da origem para o destino
static inline void *nostd_memcpy(void *dest, const void *src, size_t n) {
    char *d = (char *)dest;
    const char *s = (const char *)src;
    while (n--) *d++ = *s++;
    return dest;
}

// --- 4. INLINE ASSEMBLY ENGINES / ENGINES DE ASSEMBLY INLINE ---
// x86_64 Linux Syscall ABI (0 to 6 arguments)
// ABI de Syscalls do Linux x86_64 (0 a 6 argumentos)

static inline unsigned long _sys_call0(unsigned long num) {
    register unsigned long rax __asm__("rax") = num;
    __asm__ volatile (
        "syscall"
        : "=r" (rax)
        : "r" (rax)
        : "rcx", "r11", "memory"
    );
    return rax;
}

static inline unsigned long _sys_call1(unsigned long num, unsigned long arg1) {
    register unsigned long rax __asm__("rax") = num;
    register unsigned long rdi __asm__("rdi") = arg1;
    __asm__ volatile (
        "syscall"
        : "=r" (rax)
        : "r" (rax), "r" (rdi)
        : "rcx", "r11", "memory"
    );
    return rax;
}

static inline unsigned long _sys_call2(unsigned long num, unsigned long arg1, unsigned long arg2) {
    register unsigned long rax __asm__("rax") = num;
    register unsigned long rdi __asm__("rdi") = arg1;
    register unsigned long rsi __asm__("rsi") = arg2;
    __asm__ volatile (
        "syscall"
        : "=r" (rax)
        : "r" (rax), "r" (rdi), "r" (rsi)
        : "rcx", "r11", "memory"
    );
    return rax;
}

static inline unsigned long _sys_call3(unsigned long num, unsigned long arg1, unsigned long arg2, unsigned long arg3) {
    register unsigned long rax __asm__("rax") = num;
    register unsigned long rdi __asm__("rdi") = arg1;
    register unsigned long rsi __asm__("rsi") = arg2;
    register unsigned long rdx __asm__("rdx") = arg3;
    __asm__ volatile (
        "syscall"
        : "=r" (rax)
        : "r" (rax), "r" (rdi), "r" (rsi), "r" (rdx)
        : "rcx", "r11", "memory"
    );
    return rax;
}

static inline unsigned long _sys_call4(unsigned long num, unsigned long arg1, unsigned long arg2, unsigned long arg3, unsigned long arg4) {
    register unsigned long rax __asm__("rax") = num;
    register unsigned long rdi __asm__("rdi") = arg1;
    register unsigned long rsi __asm__("rsi") = arg2;
    register unsigned long rdx __asm__("rdx") = arg3;
    register unsigned long r10 __asm__("r10") = arg4;
    __asm__ volatile (
        "syscall"
        : "=r" (rax)
        : "r" (rax), "r" (rdi), "r" (rsi), "r" (rdx), "r" (r10)
        : "rcx", "r11", "memory"
    );
    return rax;
}

static inline unsigned long _sys_call5(unsigned long num, unsigned long arg1, unsigned long arg2, unsigned long arg3, unsigned long arg4, unsigned long arg5) {
    register unsigned long rax __asm__("rax") = num;
    register unsigned long rdi __asm__("rdi") = arg1;
    register unsigned long rsi __asm__("rsi") = arg2;
    register unsigned long rdx __asm__("rdx") = arg3;
    register unsigned long r10 __asm__("r10") = arg4;
    register unsigned long r8  __asm__("r8")  = arg5;
    __asm__ volatile (
        "syscall"
        : "=r" (rax)
        : "r" (rax), "r" (rdi), "r" (rsi), "r" (rdx), "r" (r10), "r" (r8)
        : "rcx", "r11", "memory"
    );
    return rax;
}

static inline unsigned long _sys_call6(unsigned long num, unsigned long arg1, unsigned long arg2, unsigned long arg3, unsigned long arg4, unsigned long arg5, unsigned long arg6) {
    register unsigned long rax __asm__("rax") = num;
    register unsigned long rdi __asm__("rdi") = arg1;
    register unsigned long rsi __asm__("rsi") = arg2;
    register unsigned long rdx __asm__("rdx") = arg3;
    register unsigned long r10 __asm__("r10") = arg4;
    register unsigned long r8  __asm__("r8")  = arg5;
    register unsigned long r9  __asm__("r9")  = arg6;
    __asm__ volatile (
        "syscall"
        : "=r" (rax)
        : "r" (rax), "r" (rdi), "r" (rsi), "r" (rdx), "r" (r10), "r" (r8), "r" (r9)
        : "rcx", "r11", "memory"
    );
    return rax;
}

// --- 5. THE UNIVERSAL SYSCALL ROUTER / O ROTEADOR UNIVERSAL DE SYSCALLS ---

// Step 1: An argument counter. It pushes the arguments to the right and returns the matching number (Maximum: 1 ID + 6 arguments = 7 total)
// Passo 1: Um contador de argumentos. Ele empurra os argumentos para a direita e retorna o número correspondente (Máximo: 1 ID + 6 argumentos = 7 total)
#define __SYSCALL_NARGS(_1, _2, _3, _4, _5, _6, _7, N, ...) N

// Step 2: Pass our arguments into the counter, followed by the fallback numbers
// Passo 2: Passa os nossos argumentos para o contador, seguidos pelos números de preenchimento (fallback)
#define __SYSCALL_COUNT(...) __SYSCALL_NARGS(__VA_ARGS__, 6, 5, 4, 3, 2, 1, 0)

// Step 3: Token concatenators. We need two levels to force expansion before gluing.
// Passo 3: Concatenadores de tokens. Precisamos de dois níveis para forçar a expansão antes da colagem.
#define __SYSCALL_CONCAT(a, b) a ## b
#define _SYSCALL_CONCAT(a, b) __SYSCALL_CONCAT(a, b)

// Step 4: The Ultimate Universal Function! It counts the parameters, glues the number to _sys_call, and executes it.
// Passo 4: A Função Universal Definitiva! Ela conta os parâmetros, cola o número ao _sys_call e executa a chamada.
#define syscall(...) _SYSCALL_CONCAT(_sys_call, __SYSCALL_COUNT(__VA_ARGS__))(__VA_ARGS__)

// --- 6. FUNCTION WRAPPERS / ENVOLVEDORES DE FUNÇÕES (WRAPPERS) ---
// Friendly Interface / Interface Amigável

static inline void sys_exit(int status) {
    _sys_call1(SYS_EXIT, (unsigned long)status);
}

static inline int sys_close(int fd) {
    return (int)_sys_call1(SYS_CLOSE, (unsigned long)fd);
}

static inline ssize_t sys_write(int fd, const void *buf, size_t count) {
    return (ssize_t)_sys_call3(SYS_WRITE, (unsigned long)fd, (unsigned long)buf, (unsigned long)count);
}

static inline ssize_t sys_read(int fd, void *buf, size_t count) {
    return (ssize_t)_sys_call3(SYS_READ, (unsigned long)fd, (unsigned long)buf, (unsigned long)count);
}

static inline int sys_open(const char *filename, int flags, int mode) {
    return (int)_sys_call3(SYS_OPEN, (unsigned long)filename, (unsigned long)flags, (unsigned long)mode);
}

static inline int sys_mprotect(void *addr, size_t len, int prot) {
    return (int)_sys_call3(SYS_MPROTECT, (unsigned long)addr, (unsigned long)len, (unsigned long)prot);
}

static inline int sys_munmap(void *addr, size_t len) {
    return (int)_sys_call2(SYS_MUNMAP, (unsigned long)addr, (unsigned long)len);
}

static inline void *sys_mmap(void *addr, size_t length, int prot, int flags, int fd, unsigned long offset) {
    return (void *)_sys_call6(SYS_MMAP, (unsigned long)addr, (unsigned long)length, (unsigned long)prot, (unsigned long)flags, (unsigned long)fd, offset);
}

// Convenience macros for fast printing
// Macros de conveniência para impressão rápida
#define sys_print(literal) sys_write(1, literal, sizeof(literal) - 1)

static inline ssize_t sys_print_str(const char *str) {
    return sys_write(1, str, nostd_strlen(str));
}

#endif // NOSTD_SYS_H