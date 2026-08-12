# NoSTD: O Framework x86_64 de Baixo Nível (Bare-Metal)

## 💀 A Filosofia: Erradicando a Abstração Forçada

O desenvolvimento em C moderno está completamente divorciado do hardware. Quando você escreve `int main()` e o compila usando a Biblioteca C padrão do GNU (`glibc`), você não está escrevendo um programa que conversa com o computador; você está escrevendo um script que conversa com um intermediário massivo e inchado.

**Este projeto existe para eliminar o intermediário.** O framework `NoSTD` é um ecossistema x86_64 puro e sem dependências, construído do zero. Ele elimina violentamente a biblioteca padrão do C, contornando o ambiente protegido do Userland e conectando sua lógica diretamente ao Kernel do Linux por meio de chamadas de sistema de hardware brutas.

## Demo

https://github.com/user-attachments/assets/708bc42c-c523-4521-8a4d-c02ef300db01

*Clique na imagem para assistir ao NoSTD-Framework em ação.*

---

## 🏗️ O Problema: Userland vs. Ring 0 & O Inchaço da Glibc

Para entender por que o `NoSTD` é necessário, você deve compreender os anéis de privilégio dos processadores modernos e o que os compiladores padrão escondem de você.

### Userland (Ring 3) vs. Espaço do Kernel (Ring 0)
Sua CPU opera em anéis de privilégio. O Kernel do Linux vive no **Ring 0** — ele possui controle absoluto e quase divino sobre a RAM, o cache da CPU e os periféricos de hardware. Suas aplicações padrão vivem no **Ring 3 (Userland)** — uma sandbox restrita e sem privilégios.
Um processo no Ring 3 não pode alocar memória, ler um arquivo ou criar um processo. Para fazer qualquer coisa útil, ele deve disparar uma interrupção de hardware (a instrução `syscall`) e pedir educadamente ao Kernel (Ring 0) que faça o trabalho por ele.

### O Desastre do Inchaço da Glibc
Quando você compila um programa em C padrão, a `glibc` sequestra o ponto de entrada do seu binário. Antes mesmo de o seu `main()` ser executado, a rotina interna `_start` da `glibc` faz o seguinte:
1. Inicializa o Armazenamento Local de Threads (TLS).
2. Configura as arenas de memória do `malloc` (mesmo que você nunca as utilize).
3. Analisa variáveis de ambiente e configura estados globais.
4. Registra os destrutores `atexit`.
5. Injeta Canários de Pilha (*Stack Canaries*) para prevenir estouros de buffer (disparando `__stack_chk_fail`).

Isso gera milhares de linhas de instruções de assembly inúteis, inflando o tamanho do seu binário, adicionando sobrecarga de execução e escondendo o verdadeiro estado da máquina do programador.

**O NoSTD resolve isso sequestrando completamente o ponto de entrada ELF.** Nós removemos a `glibc` inteiramente. O seu código se torna a primeira instrução absoluta que a CPU executa quando o Kernel entrega o processo. Sem threads ocultas, sem inicializações em segundo plano, sem alocação de memória forçada.

---

## Comparação: NoSTD-Framework vs. Glibc (Estática)

Abaixo está a comparação entre um "Hello World" padrão usando `glibc` (vinculada estaticamente para remover dependências dinâmicas) e o `NoSTD-Framework`.

<img width="1424" height="483" alt="PoC" src="https://github.com/user-attachments/assets/f1843408-a36b-4f44-854b-e7126cf26df4" />

| Característica | Padrão (Glibc Estática) | NoSTD-Framework |
| :--- | :--- | :--- |
| **Tamanho do Binário** | ~825 KB | **~9.1 KB** |
| **Syscalls na Inicialização** | Dezenas (`mmap`, `brk`, `arch_prctl`, etc.) | **3 (`execve`, `write`, `exit`)** |
| **Abstração** | Alta (Runtime Complexo) | **Nenhuma (Bare-metal)** |

### 1. Inchaço vs. Eficiência
O tamanho massivo da `glibc` estática ocorre porque a biblioteca padrão precisa incluir rotinas complexas de formatação (`printf`), alocação de memória (`malloc`/`free`), tratamento de sinais e localização. O `NoSTD` é cirúrgico: ele inclui apenas o código necessário para as chamadas de sistema que o seu software realmente utiliza.

### 2. Análise de Rastreamento de Execução (strace)
Ao analisar a execução via `strace`, a diferença de comportamento torna-se crítica. A `glibc` realiza uma série de chamadas de sistema "invisíveis" antes de executar a primeira linha do seu `main`. Isso gera "ruído" que pode ser detectado por EDRs ou ferramentas de monitoramento. O `NoSTD` executa o seu código imediatamente, sem nenhuma inicialização de tempo de execução.

<img width="1427" height="756" alt="strace" src="https://github.com/user-attachments/assets/7abf9879-7bcc-4ea9-a64d-7655e0129f88" />

**Principais observações do rastreamento:**
* **A Trindade das Syscalls:** Observe o fluxo de execução limpo à direita. Iniciamos com `execve`, realizamos nossa tarefa com `write` e encerramos limpidamente com `exit`. Não há chamadas ocultas de configuração de armazenamento local de thread ou registros de manipuladores de sinal.
* **Acesso Direto ao Kernel:** Como contornamos a biblioteca C, operamos no nível de hardware. O Kernel não sabe — nem se importa — se um "programa" está rodando; ele simplesmente processa as requisições passadas diretamente para os registradores da CPU.
* **Determinismo:** Sem o tempo de execução da `glibc`, não há surpresas. Você tem controle total sobre o estado da pilha e dos registradores no ponto de entrada, garantindo que o comportamento do binário seja idêntico em qualquer ambiente Linux compatível.

## 🧬 Mergulho Profundo na Arquitetura

### 1. O Modelo de Dados LP64: Por que `long` em vez de `int`
Se você olhar o código-fonte dos engines do `NoSTD`, notará a erradicação absoluta do tipo de dado `int` em favor de `unsigned long`. Isso não é uma escolha estilística; **é um requisito rígido de hardware.**

O Linux em x86_64 usa o **modelo de dados LP64**:
* `int` = 32 bits (4 bytes)
* `long` = 64 bits (8 bytes)
* Ponteiros (`void *`) = 64 bits (8 bytes)

Os registradores da CPU usados para conversar com o Kernel (`RAX`, `RDI`, `RSI`, etc.) possuem 64 bits de largura. Se nossos engines de syscall aceitassem um `int` (32 bits), e você tentasse passar um ponteiro de memória (por exemplo, o endereço de uma string `0x00007FFE8B3A1234`), o compilador C **truncaria** os 32 bits superiores. O ponteiro seria destruído, tornando-se `0x8B3A1234`. Quando o Kernel tentasse ler esse endereço corrompido, ele dispararia instantaneamente uma **Falha de Segmentação (Segmentation Fault)**.

O uso de `unsigned long` garante uma paridade mapeada de 1:1 com os registradores de hardware, assegurando que ponteiros e dados viajem perfeitamente de suas variáveis em C para o silício da CPU.

### 2. O Bootstrapper Nu (Contornando o `_start`)
Como eliminamos a `glibc`, o Kernel não passa educadamente `argc` e `argv` como argumentos para uma função. Em vez disso, após o retorno da syscall `execve`, o Kernel despeja os argumentos brutos diretamente na Pilha (`RSP`) da CPU.

Para traduzir esse layout de memória bruta em um ambiente C limpo, usamos um Bootstrapper em Assembly embutido (*inline*):

```c
__asm__(
    ".text\n"
    ".global _start\n"
    "_start:\n"
    "    pop %rdi\n"           // 1. Remove argc do topo da pilha diretamente para RDI (1º argumento C)
    "    mov %rsp, %rsi\n"    // 2. RSP agora aponta para argv[0]. Move para RSI (2º argumento C)
    "    and $-16, %rsp\n"    // 3. CRÍTICO: Alinha a pilha em 16 bytes para respeitar a System V ABI
    "    call nostd_main\n"   // 4. Transiciona com segurança para nossa lógica em C
    "    mov %rax, %rdi\n"    // 5. Captura o valor de retorno da função C para RDI
    "    mov $60, %rax\n"     // 6. Carrega SYS_EXIT (60) em RAX
    "    syscall\n"           // 7. Ordena que o Kernel encerre o processo limpidamente
);