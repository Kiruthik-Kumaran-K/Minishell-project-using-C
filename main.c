/**
 * @file main.c
 * @brief Entry point for the Minimalistic Shell (msh) project.
 *
 * This file initializes the shell environment, configures global signal 
 * handlers for terminal interrupts, and launches the main input-evaluation loop.
 */

#include "minishell.h"

/** 
 * @var prompt
 * @brief Global shell prompt string.
 * 
 * Accessible globally so it can be dynamically modified by environmental 
 * variable assignments (e.g., PS1=NEW_PROMPT) and accessed by signal handlers 
 * to cleanly reprint the prompt when terminal interrupts occur.
 */
char prompt[MAX_INPUT] = "msh$ ";

/**
 * @brief Main function to initialize and run the shell.
 * 
 * Registers POSIX signal handlers to prevent the shell process from terminating 
 * or suspending upon receiving SIGINT (Ctrl+C) and SIGTSTP (Ctrl+Z), then passes 
 * control to the primary input scanning loop.
 *
 * @return 0 upon successful termination (though standard exit is handled via the 'exit' built-in).
 */
int main() {
    /* 
     * Override default terminal signal behavior.
     * Instead of terminating or stopping the shell itself, route these 
     * signals to the custom handler to manage foreground child processes gracefully.
     */
    signal(SIGINT, signal_handler);
    signal(SIGTSTP, signal_handler);
    
    // Buffer allocated to hold raw command-line input from the user
    char input_string[MAX_INPUT];

    // Launch the continuous read-eval-print loop (REPL)
    scan_input(prompt, input_string);
    
    return 0;
}