# NoSTD: The Bare-Metal x86_64 Framework

## 💀 The Philosophy: Eradicating Forced Abstraction

Modern C development is completely divorced from the hardware. When you write `int main()` and compile it using the standard GNU C Library (`glibc`), you are not writing a program that talks to the computer; you are writing a script that talks to a massive, bloated middleman.

**This project exists to kill the middleman.** The `NoSTD` framework is a pure, zero-dependency x86_64 ecosystem built from scratch. It violently strips away the C standard library, bypassing the padded environment of Userland, and connects your logic directly to the Linux Kernel via raw hardware system calls.

## Demo

https://github.com/user-attachments/assets/708bc42c-c523-4521-8a4d-c02ef300db01

*Click the image to watch the NoSTD-Framework in action.*

---

## 🏗️ The Problem: Userland vs. Ring 0 & The Glibc Bloat

To understand why `NoSTD` is necessary, you must understand the privilege rings of modern processors and what standard compilers hide from you.

### Userland (Ring 3) vs. Kernel Space (Ring 0)
Your CPU operates in privilege rings. The Linux Kernel lives in **Ring 0**—it has absolute, god-like control over the RAM, the CPU cache, and the hardware peripherals. Your standard applications live in **Ring 3 (Userland)**—a restricted, unprivileged sandbox. 
A Ring 3 process cannot allocate memory, read a file, or spawn a process. To do anything useful, it must trigger a hardware interrupt (the `syscall` opcode) and politely beg the Kernel (Ring 0) to do the job for it.

### The Glibc Bloat Disaster
When you compile a standard C program, `glibc` hijacks the entry point of your binary. Before your `main()` is even executed, `glibc`'s internal `_start` routine:
1. Initializes Thread-Local Storage (TLS).
2. Sets up `malloc` memory arenas (even if you never use them).
3. Parses environment variables and sets up global states.
4. Registers `atexit` destructors.
5. Injects Stack Canaries to prevent buffer overflows (triggering `__stack_chk_fail`).

This generates thousands of lines of useless assembly instructions, inflating your binary size, adding execution overhead, and hiding the true state of the machine from the programmer.

**NoSTD resolves this by completely hijacking the ELF entry point.** We remove `glibc` entirely. Your code becomes the absolute first instruction the CPU executes when the Kernel hands over the process. No hidden threads, no background initializations, no forced memory allocation.

---

## Comparison: NoSTD-Framework vs. Glibc (Static)

Below is the comparison between a standard "Hello World" using `glibc` (statically linked to remove dynamic dependencies) and the `NoSTD-Framework`.

<img width="1424" height="483" alt="PoC" src="https://github.com/user-attachments/assets/f1843408-a36b-4f44-854b-e7126cf26df4" />

| Characteristic | Standard (Static Glibc) | NoSTD-Framework |
| :--- | :--- | :--- |
| **Binary Size** | ~825 KB | **~9.1 KB** |
| **Startup Syscalls** | Dozens (`mmap`, `brk`, `arch_prctl`, etc.) | **3 (`execve`, `write`, `exit`)** |
| **Abstraction** | High (Complex Runtime) | **None (Bare-metal)** |

### 1. Bloat vs. Efficiency
The massive size of the static `glibc` occurs because the standard library must include complex routines for formatting (`printf`), memory allocation (`malloc`/`free`), signal handling, and localization. `NoSTD` is surgical: it includes only the code necessary for the system calls your software actually uses.

### 2. Execution Trace Analysis (strace)
When analyzing execution via `strace`, the difference in behavior becomes critical. `glibc` performs a series of "invisible" system calls before executing the first line of your `main`. This generates "noise" that can be detected by EDRs or monitoring tools. `NoSTD` executes your code immediately, with no runtime initialization.

<img width="1427" height="756" alt="strace" src="https://github.com/user-attachments/assets/7abf9879-7bcc-4ea9-a64d-7655e0129f88" />

**Key observations from the trace:**
* **The Syscall Trinity:** Note the clean execution flow on the right. We initiate with `execve`, perform our task with `write`, and terminate cleanly with `exit`. There are no hidden thread-local storage setup calls or signal handler registrations.
* **Kernel Direct Access:** Because we bypass the C library, we operate at the hardware level. The Kernel does not know—or care—that a "program" is running; it simply processes the requests passed directly to the CPU registers.
* **Determinism:** Without the `glibc` runtime, there are no surprises. You have total control over the stack state and registers at the entry point, ensuring the binary's behavior is identical across any compatible Linux environment.

## 🧬 Architecture Deep-Dive

### 1. The LP64 Data Model: Why `long` instead of `int`
If you look at the source code of the `NoSTD` engines, you will notice the absolute eradication of the `int` data type in favor of `unsigned long`. This is not a stylistic choice; **it is a strict hardware requirement.**

Linux on x86_64 uses the **LP64 data model**:
* `int` = 32 bits (4 bytes)
* `long` = 64 bits (8 bytes)
* Pointers (`void *`) = 64 bits (8 bytes)

The CPU registers used to talk to the Kernel (`RAX`, `RDI`, `RSI`, etc.) are 64 bits wide. If our syscall engines accepted an `int` (32 bits), and you attempted to pass a memory pointer (e.g., the address of a string `0x00007FFE8B3A1234`), the C compiler would **truncate** the upper 32 bits. The pointer would be destroyed, becoming `0x8B3A1234`. When the Kernel attempted to read that corrupted address, it would instantly trigger a **Segmentation Fault**.

Using `unsigned long` guarantees a 1:1 mapped parity with the hardware registers, ensuring pointers and data travel flawlessly from your C variables into the CPU's silicon.

### 2. The Naked Bootstrapper (Bypassing `_start`)
Because we killed `glibc`, the Kernel does not politely pass `argc` and `argv` as arguments to a function. Instead, upon the `execve` syscall return, the Kernel dumps the raw arguments directly onto the CPU's Stack (`RSP`).

To translate this raw memory layout into a clean C environment, we use an inline Assembly Bootstrapper:

```c
__asm__(
    ".text\n"
    ".global _start\n"
    "_start:\n"
    "    pop %rdi\n"          // 1. Pop argc from the top of the stack straight into RDI (1st C argument)
    "    mov %rsp, %rsi\n"    // 2. RSP now points to argv[0]. Move it to RSI (2nd C argument)
    "    and $-16, %rsp\n"    // 3. CRITICAL: Align the stack to 16-bytes to respect the System V ABI
    "    call nostd_main\n"   // 4. Safely transition into our C logic
    "    mov %rax, %rdi\n"    // 5. Capture the C function's return value into RDI
    "    mov $60, %rax\n"     // 6. Load SYS_EXIT (60) into RAX
    "    syscall\n"           // 7. Command the Kernel to cleanly kill the process
);
```

**The Stack Alignment:** The `and $-16, %rsp` instruction is the magic bullet. If the stack is not aligned to 16 bytes before calling a C function, any modern CPU executing SIMD/Vector instructions (like `movaps`) will immediately crash the program.

### 3. The Universal Syscall Router (Metaprogramming)
The Linux x86_64 Kernel defines over 470 system calls. Writing manual wrapper functions for each one is bloated and inefficient. `NoSTD` solves this using advanced C Preprocessor (CPP) Metaprogramming.

```c
// Argument counter (Counts up to 6 arguments + 1 Syscall ID)
#define __SYSCALL_NARGS(_1, _2, _3, _4, _5, _6, _7, N, ...) N
#define __SYSCALL_COUNT(...) __SYSCALL_NARGS(__VA_ARGS__, 6, 5, 4, 3, 2, 1, 0)

// Token concatenators
#define __SYSCALL_CONCAT(a, b) a ## b
#define _SYSCALL_CONCAT(a, b) __SYSCALL_CONCAT(a, b)

// The Universal Gateway
#define syscall(...) _SYSCALL_CONCAT(_sys_call, __SYSCALL_COUNT(__VA_ARGS__))(__VA_ARGS__)
```

**How it works:**
When you write `syscall(SYS_WRITE, 1, buffer, length);`, the macro dynamically counts the parameters at compile-time (4 total). It then seamlessly morphs your code into `_sys_call3(...)`. 

This routes the execution to an inline assembly block that strictly aligns the variables into the System V ABI registers (`RAX` for the syscall ID, followed by `RDI`, `RSI`, `RDX`, `R10`, `R8`, `R9` for arguments) before executing the hardware `syscall` instruction. All of this happens instantly in the silicon, with zero runtime overhead and zero memory allocation.

---

## 🚀 Compilation & Usage

Because this framework rejects the standard ecosystem, you must strictly command the compiler to step down.

Compile your tools using:

```bash
gcc -O2 -nostdlib -fno-stack-protector -static src/main.c -o my_tool
```

* `-O2`: Forces the compiler to aggressively inline the syscall engines, flattening the assembly and eliminating `call`/`ret` stack frames overhead.
* `-nostdlib`: The core directive. Instructs the Linker to completely ignore `glibc` and standard startup routines (`crt0`).
* `-fno-stack-protector`: Prevents the compiler from injecting `__stack_chk_fail` routines when allocating local arrays on the stack, giving you ultimate control over your memory bounds.

## ⚠️ The Reality of Ring 0

This framework provides **zero safety nets**. You are speaking directly to the Kernel. If you fail to null-terminate a string, pass a bad pointer, or request execution jumps to `PROT_NONE` memory, the Kernel will instantly murder your process with a Segmentation Fault. 

Welcome to the bare metal.
