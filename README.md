# Minishell Shell (msh)

A custom, minimalistic Linux shell implemented in C. This project was developed as part of a Linux Internals module to demonstrate a foundational understanding of operating system mechanics, including process creation, signal handling, inter-process synchronization, and job control.

## 🚀 Features

### 1. Interactive Prompt & Execution

* **Customizable Prompt:** The default `msh>` prompt can be dynamically changed using the environmental variable format (e.g., `PS1=NEW_PROMPT`).
* **External Command Execution:** Seamlessly executes standard Linux binaries (e.g., `ls`, `cat`, `grep`) using `fork()` and `execvp()`.
* **Input Sanitization:** Automatically handles trailing newlines and empty inputs.

### 2. Built-In Commands

The shell natively handles several commands without spawning a new process:

* `cd`: Change the current working directory.
* `pwd`: Print the current working directory.
* `echo`: Print text to the terminal, including support for special variables.
* `exit`: Gracefully terminate the shell.

### 3. Special Variables

* `echo $?`: Prints the exit status of the most recently executed external command.
* `echo $$`: Prints the Process ID (PID) of the shell itself.
* `echo $SHELL`: Prints the executable path of the shell.

### 4. Advanced Signal Handling

* **`Ctrl+C` (SIGINT):** Interrupts the currently running foreground child process without killing the parent shell.
* **`Ctrl+Z` (SIGTSTP):** Suspends the currently running foreground child process and hands control back to the shell.

### 5. Job Control (LIFO Stack)

* `jobs`: Displays a list of all currently stopped or backgrounded processes.
* `bg`: Resumes the most recently stopped process in the background, allowing the shell to accept new commands concurrently.
* `fg`: Brings the most recently stopped process back into the foreground and blocks the shell until it completes.

---

## 🛠️ Project Architecture

The project is modularized into three main files:

* **`main.c`**: Initializes the global environment, sets up signal traps, and launches the core read-eval-print loop (REPL).
* **`minishell.c`**: Contains the core logic for input parsing (`strtok`), system calls (`fork`, `execvp`, `waitpid`), signal handling, and the job control linked list.
* **`minishell.h`**: The central header file containing standard library includes, macro definitions, and `JobNode` structures.

---

## ⚙️ Prerequisites

To compile and run this project, you will need:

* A Linux environment (or WSL on Windows).
* The GCC Compiler (`gcc`).
* Standard POSIX libraries (included by default in Linux C environments).

---

## 💻 Installation & Compilation

1. Clone the repository to your local machine:
```bash
git clone https://github.com/Kiruthik-Kumaran-K/Minishell-project-using-C.git
cd minishell

```


2. Compile the source code using `gcc`:
```bash
gcc main.c minishell.c -o msh

```


3. Run the executable:
```bash
./msh

```



---

## 📖 Usage Examples

**Customizing the Prompt**

```bash
msh> PS1=k_shell$ 
k_shell$ 

```

**Running External Commands & Checking Exit Status**

```bash
msh> ls -l
... (directory listing) ...
msh> echo $?
0

```

**Using Job Control (Ctrl+Z, jobs, fg, bg)**

```bash
msh> sleep 100
^Z
[1345] Stopped    sleep 100

msh> jobs
[1] Stopped    sleep 100 (PID: 1345)

msh> bg
[1345] sleep 100 &

msh> fg
sleep 100

```