/**
 * @file minishell.c
 * @brief Core implementation of the Minimalistic Shell (msh).
 *
 * This file contains the primary logic for reading user input, parsing commands,
 * managing processes (fork/exec), handling POSIX signals, and maintaining job control.
 */

#include "minishell.h"

/* --- Global State Variables --- */
int last_exit_status = 0;      /**< Stores the exit status of the most recently executed external command */
extern char prompt[MAX_INPUT]; /**< Shell prompt string, customizable via PS1 environment variable */
pid_t pid = 0;                 /**< Tracks the PID of the currently running foreground process (0 if none) */
JobNode *jobs_head = NULL;     /**< Head pointer for the linked list of stopped background jobs */

/**
 * @brief Signal handler for SIGINT (Ctrl+C) and SIGTSTP (Ctrl+Z).
 * 
 * Intercepts signals sent from the terminal. If a child process is running in 
 * the foreground, the signal is ignored by the shell (passed to child). If no 
 * child is running, it gracefully reprints the shell prompt.
 *
 * @param sig_num The integer code of the caught signal.
 */
void signal_handler(int sig_num)
{
    if (sig_num == SIGINT)
    {
        if (pid == 0) // No foreground child process exists
        {
            printf("\n");
            printf("%s%s%s>", ANSI_COLOR_RED, prompt, ANSI_COLOR_RESET);
            fflush(stdout);
        }
        else
        {
            printf("\n"); // Push the upcoming prompt to a new line after ^C termination
        }
    }
    else if (sig_num == SIGTSTP)
    {
        if (pid == 0) // No foreground child process exists
        {
            printf("\n");
            printf("%s%s%s>", ANSI_COLOR_RED, prompt, ANSI_COLOR_RESET);
            fflush(stdout); 
        }
        else
        {
            printf("\n"); // Push the upcoming prompt to a new line after ^Z suspension
        }
    }
}

/**
 * @brief Executes built-in shell commands.
 * 
 * Handles execution of commands that must run within the shell's own process 
 * space (e.g., cd, pwd, exit, jobs, fg, bg) and evaluates special variables.
 *
 * @param input The raw command string entered by the user.
 */
void execute_internal_commands(char *input)
{
    char input_copy[MAX_INPUT];
    strcpy(input_copy, input);

    char *cmd = strtok(input_copy, " \t");
    char *arg = strtok(NULL, " \t");

    if(strcmp(cmd, "pwd") == 0)
    {
        char cwd[1024];
        if(getcwd(cwd, sizeof(cwd)) != NULL)
        {
            printf("%s\n", cwd);
        }
        return;
    }

    if(strcmp(cmd, "cd") == 0)
    {
        if(arg != NULL)
        {
            chdir(arg); // Change the shell's working directory
        }
        return;
    }

    if(strcmp(cmd, "exit") == 0)
    {
        exit(1); // Terminate the shell process
    }

    if(strcmp(cmd, "echo") == 0)
    {
        if(arg != NULL)
        {
            if(strcmp(arg, "$SHELL") == 0)
            {
                char *shell = getenv("SHELL");
                if(shell != NULL)
                {
                    printf("%s\n", shell);
                }
            }
            else if(strcmp(arg, "$$") == 0)
            {
                printf("%d\n", getpid()); // Print shell's PID
            }
            else if(strcmp(arg, "$?") == 0)
            {
                printf("%d\n", last_exit_status); // Print last command's exit status
            }
            else
            {
                printf("%s\n", arg);
            }
        }
        return;
    }

    /* Job Control Built-ins */
    if(strcmp(cmd, "jobs") == 0)
    {
        execute_jobs();
        return;
    }

    if(strcmp(cmd, "fg") == 0)
    {
        execute_fg();
        return;
    }

    if(strcmp(cmd, "bg") == 0)
    {
        execute_bg();
        return;
    }

    printf("Internal command not found\n");
}

/**
 * @brief Spawns a child process to execute an external system command.
 * 
 * Tokenizes the input to build an argument vector, forks a new process, 
 * restores default signal handlers in the child, and uses execvp() to run the 
 * binary. The parent process waits for completion or suspension.
 *
 * @param input The raw command string entered by the user.
 */
void execute_external_commands(char *input)
{
    char input_copy[MAX_INPUT];
    strcpy(input_copy, input); // Create a non-destructive copy for tokenization

    char *args[64];
    int i = 0;

    // Tokenize the string to build the argument vector for execvp
    char *token = strtok(input_copy, " \t"); 
    while (token != NULL)
    {
        args[i++] = token;
        token = strtok(NULL, " \t");
    }
    args[i] = NULL; // Null-terminate the argument array

    pid = fork();

    if (pid < 0)
    {
        perror("Fork failed");
    }
    else if (pid == 0)
    {
        // Child Process: Restore default signal behavior before execution
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);

        // Replace child process image with the target executable
        if (execvp(args[0], args) == -1)
        {
            perror("Command execution failed");
            exit(127); // Standard exit code for command not found
        }
    }
    else 
    {
        // Parent Process: Wait for the child to terminate or be stopped
        int status;
        
        // WUNTRACED flag ensures waitpid returns if the child is suspended via Ctrl+Z
        waitpid(pid, &status, WUNTRACED); 
        
        if (WIFEXITED(status))
        {
            last_exit_status = WEXITSTATUS(status);
        }
        else if (WIFSTOPPED(status))
        {
            // Child was suspended; register it in the jobs list
            printf("\n[%d] Stopped\t%s\n", pid, input);
            add_job(pid, input); 
        }

        pid = 0; // Clear the foreground process tracker
    }
}

/**
 * @brief Main execution loop for reading and evaluating terminal input.
 * 
 * Continually displays the shell prompt, sanitizes user input (trimming newlines), 
 * handles environmental variable assignments (PS1), and routes commands to the 
 * appropriate execution handler.
 *
 * @param prompt A reference to the shell's dynamic prompt string.
 * @param input_string Buffer to store the raw user input.
 */
void scan_input(char *prompt, char *input_string)
{
    system("clear"); // Clear terminal on startup

    while (1)
    {
        printf("%s%s%s> ", ANSI_COLOR_RED, prompt, ANSI_COLOR_RESET);

        // Read input and check for EOF (Ctrl+D)
        if(fgets(input_string, MAX_INPUT, stdin) == NULL)
        {
            break;
        }

        // Strip trailing newline character generated by fgets
        input_string[strcspn(input_string, "\n")] = 0;

        // Ignore empty inputs
        if(strlen(input_string) == 0)
        {
            continue;
        }

        // Handle PS1 prompt customization
        if(strncmp(input_string, "PS1=", 4) == 0)
        {
            strcpy(prompt, input_string + 4);
            continue;
        }

        char input_copy[MAX_INPUT];
        strcpy(input_copy, input_string);

        char *cmd = get_command(input_copy);
        if(cmd == NULL)
        {
            continue;
        }

        // Route command to internal or external execution engine
        int ret = check_command_type(cmd);
        if(ret == 1)
        {
            execute_internal_commands(input_string);
        }
        else if(ret == 2)
        {
            execute_external_commands(input_string);
        }
    }
}

/**
 * @brief Extracts the base command from the full input string.
 * 
 * @param input_string The raw command string.
 * @return A pointer to the first space/tab delimited token.
 */
char *get_command(char *input_string)
{
    return strtok(input_string," \t");
}

/**
 * @brief Identifies whether a command is built-in or external.
 * 
 * @param cmd The base command string to evaluate.
 * @return 1 if the command is built-in, 2 if the command is external.
 */
int check_command_type(char *cmd)
{
    char *builtins[] = {"echo", "printf", "read", "cd", "pwd", "pushd", "popd", 
                        "dirs", "let", "eval", "set", "unset", "export", "declare", 
                        "typeset", "readonly", "getopts", "source", "exit", "exec", 
                        "shopt", "caller", "true", "type", "hash", "bind", "help", 
                        "jobs", "fg", "bg", NULL};

    for(int i = 0; builtins[i] != NULL; i++)
    {
        if(strcmp(cmd, builtins[i]) == 0)
        {
            return 1; // Built-in match found
        }
    }

    // Default to external if no built-in matches
    return 2; 
}


/* ==========================================
 *        JOB CONTROL LINKED LIST
 * ========================================== */

/**
 * @brief Adds a stopped process to the head of the jobs list (LIFO architecture).
 * 
 * @param pid Process ID of the suspended job.
 * @param command The full command string associated with the process.
 */
void add_job(pid_t pid, char *command) 
{
    JobNode *new_node = malloc(sizeof(JobNode));
    new_node->pid = pid;
    strcpy(new_node->command, command);
    
    // Insert at head to ensure the most recently stopped command is accessed first
    new_node->next = jobs_head;
    jobs_head = new_node;
}

/**
 * @brief Removes a process from the jobs list by its Process ID.
 * 
 * @param pid The Process ID of the job to remove.
 */
void remove_job(pid_t pid) 
{
    JobNode *current = jobs_head;
    JobNode *prev = NULL;

    // Traverse list to locate target PID
    while (current != NULL && current->pid != pid) 
    {
        prev = current;
        current = current->next;
    }

    if (current == NULL) return; // Job not found in the list

    // Sever node connections and free memory
    if (prev == NULL) {
        jobs_head = current->next; // Target was head
    } else {
        prev->next = current->next; // Target was in middle/end
    }
    free(current);
}

/**
 * @brief Lists all currently stopped background jobs.
 */
void execute_jobs()
{
    JobNode *current = jobs_head;
    int job_num = 1;
    
    if (current == NULL) {
        printf("No stopped jobs.\n");
        return;
    }

    while (current != NULL)
    {
        printf("[%d] Stopped\t%s (PID: %d)\n", job_num++, current->command, current->pid);
        current = current->next;
    }
}

/**
 * @brief Brings the most recently stopped background job into the foreground.
 * 
 * Sends SIGCONT to the process, blocks the shell until the process completes 
 * or is stopped again, and handles re-insertion into the job list if necessary.
 */
void execute_fg()
{
    if (jobs_head == NULL) {
        printf("msh: fg: current: no such job\n");
        return;
    }

    // Retrieve data for the most recently stopped command (Head of list)
    pid_t fg_pid = jobs_head->pid;
    char fg_cmd[MAX_INPUT];
    strcpy(fg_cmd, jobs_head->command);

    // Remove it from the background list since it will now be in the foreground
    remove_job(fg_pid); 

    printf("%s\n", fg_cmd); // Echo the resuming command to the terminal

    kill(fg_pid, SIGCONT); // Resume process execution

    // Block parent shell and track process as the active foreground job
    pid = fg_pid; 
    int status;
    waitpid(fg_pid, &status, WUNTRACED); 

    // If the process is suspended again, re-register it in the jobs list
    if (WIFSTOPPED(status))
    {
        printf("\n[%d] Stopped\t%s\n", fg_pid, fg_cmd);
        add_job(fg_pid, fg_cmd); 
    }
    
    pid = 0; // Clear foreground tracker
}

/**
 * @brief Resumes the most recently stopped job in the background.
 * 
 * Sends SIGCONT to the process but does not block the shell, allowing the 
 * user to continue typing commands while the process runs asynchronously.
 */
void execute_bg()
{
    if (jobs_head == NULL) {
        printf("msh: bg: current: no such job\n");
        return;
    }

    // Retrieve data for the most recently stopped command
    pid_t bg_pid = jobs_head->pid;
    char bg_cmd[MAX_INPUT];
    strcpy(bg_cmd, jobs_head->command);

    // Remove from stopped list as it transitions to a running state
    remove_job(bg_pid); 
    
    printf("[%d] %s &\n", bg_pid, bg_cmd);

    kill(bg_pid, SIGCONT); // Resume process execution

    // Deliberately omit waitpid() to prevent blocking the shell parent
}