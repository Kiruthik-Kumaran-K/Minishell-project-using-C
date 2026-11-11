/**
 * @file minishell.h
 * @brief Header file for the Minimalistic Shell (msh).
 *
 * This file contains all standard library includes, system call headers, 
 * macro definitions, data structures, and function prototypes required 
 * for the shell's operation.
 */

#ifndef MAIN_H
#define MAIN_H

/* --- Standard and POSIX Library Includes --- */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio_ext.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
 
/* --- Constants & Macros --- */

/** @def MAX_INPUT Maximum character length for a single terminal input line. */
#define MAX_INPUT 1024

/* Command Routing Identifiers */
#define BUILTIN     1  /**< Identifier for internally executed commands (e.g., cd, exit) */
#define EXTERNAL    2  /**< Identifier for externally executed system binaries (e.g., ls, cat) */
#define NO_COMMAND  3  /**< Identifier for empty or invalid input */

/* ANSI Terminal Color Codes for UI Formatting */
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

/* --- Data Structures --- */

/**
 * @struct JobNode
 * @brief Node definition for the background job control linked list.
 * 
 * Implements a LIFO (Last-In, First-Out) stack structure to keep track of 
 * processes suspended by SIGTSTP (Ctrl+Z) or running in the background.
 */
typedef struct JobNode {
    pid_t pid;                 /**< Process ID of the suspended/background job */
    char command[MAX_INPUT];   /**< The full command string executed by the job */
    struct JobNode *next;      /**< Pointer to the next job in the list */
} JobNode;


/* --- Function Prototypes --- */

/* Job Control Functions */
void add_job(pid_t pid, char *command);
void remove_job(pid_t pid);
void execute_jobs();
void execute_fg();
void execute_bg();

/* Input Scanning & Parsing Functions */
void scan_input(char *prompt, char *input_string);
char *get_command(char *input_string);
int check_command_type(char *command);
void copy_change(char *prompt, char *input_string);

/* Execution Engines */
void execute_internal_commands(char *input_string);
void extract_external_commands(char **external_commands);
void echo(char *input_string, int status);

/* Signal Handling */
void signal_handler(int sig_num);

#endif /* MAIN_H */