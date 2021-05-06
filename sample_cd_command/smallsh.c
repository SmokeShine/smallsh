/*
 ============================================================================
 Name        : smallsh.c
 Author      : Prateek Gupta
 Version     : v1
 ============================================================================
/* Usage
command [arg1 arg2 ...] [< input_file] [> output_file] [&]
Items in brackets are optional
*/

/* Test Script Results Sample
$ ./p3testscript 
PRE-SCRIPT INFO
  Grading Script PID: 25077
  Note: your smallsh will report a different PID when evaluating $$
: BEGINNING TEST SCRIPT
: 
: --------------------
: ===1=== Using comment (5 points if only next prompt is displayed next)
: : 
: 
: --------------------
: ===2=== ls (10 points for returning dir contents)
: junk  junk2  p3testscript  smallsh  smallsh.c
: 
: 
: --------------------
: ls out junk
: : 
: 
: --------------------
: ===3a=== cat junk (15 points for correctly returning contents of junk)
: junk
junk2
p3testscript
smallsh
smallsh.c
: 
: 
: --------------------
: ===3b=== wc in junk (15 points for returning correct numbers from wc)
:  5  5 42 junk
: 
: 
: --------------------
: ===3b=== wc in junk out junk2; cat junk2 (10 points for returning correct numbers from wc)
: :  5  5 42 junk
: 
: 
: --------------------
: ===4=== test -f badfile (10 points for returning error value of 1, note extraneous &)
: : exit value 1
: background pid is 25114
: 
background pid 25114 is done:exit value 0
: --------------------
: ===5a=== wc in badfile (10 points for returning text error)
: wc: badfile: No such file or directory
: 
: 
: --------------------
: ===5b=== badfile (10 points for returning text error)
: badfile: no such file or directory
: 
: 
: --------------------
: ===6a=== sleep 100 background (10 points for returning process ID of sleeper)
: background pid is 25128
: 
: 
: --------------------
: ===6b,c=== pkill -signal SIGTERM sleep (10 points for pid of killed process, 10 points for signal)
: (Ignore message about Operation Not Permitted)
: background pid 25128 is done:terminated by signal 15
: 
: 
: --------------------
: ===6d,e=== sleep 1 background (10 pts for pid of bg ps when done, 10 for exit value)
: background pid is 25139
: background pid 25139 is done:exit value 0
: 
: 
: --------------------
: pwd
: /home/prateek/C/smallsh
: 
: 
: --------------------
: cd
: : 
: 
: --------------------
: ===7a=== pwd (10 points for being in the HOME dir)
: /home/prateek
: 
: 
: --------------------
: mkdir testdir25078
: : 
: 
: --------------------
: cd testdir25078
: : 
: 
: --------------------
: ===7b=== pwd (5 points for being in the newly created dir)
: /home/prateek/testdir25078
: --------------------
: ===8=== Testing foreground-only mode (20 points for entry & exit text AND ~5 seconds between times)
: 
Entering foreground-only mode (& is now ignored)
:: Saturday 16 May 2020 01:52:28 PM IST
: : Saturday 16 May 2020 01:52:33 PM IST
: 
Exiting foreground-only mode
:: Exiting smallsh
*/

/*Built in commands - exit, cd and status */

/* Library Imports */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

int cd(char *path)
{
    /*Changes current working directory to path provided*/
    /* Usage */
    /*
    : pwd
    /home/prateek/C/smallsh
    : cd sample_cd_command
    : pwd
    /home/prateek/C/smallsh/sample_cd_command
    */
    return chdir(path);
}

/*atomic entity accessible even in the presence of asynchronous interrupts made by signals.*/
volatile sig_atomic_t exitStatus = 0;
volatile sig_atomic_t disabled_background = -1;

int termSignal;

void background_toggle(int signal)
{
    /*
    Enter a state where subsequent commands can no longer be run in the background. 
    In this state, the & operator should simply be ignored - run all such commands as if they were foreground processes. 
    If the user sends SIGTSTP again, display another informative message immediately after any currently running foreground process terminates, 
    and then return back to the normal condition where the & operator is once again honored for subsequent commands, 
    allowing them to be placed in the background. 
    */

    /*Usage*/
    /*
    : sleep 10 &
    background pid is 30303
    : ^C^Z
    Entering foreground-only mode (& is now ignored)
    :sleep 10
    background pid 30303 is done:exit value 0
    : ps
        PID TTY          TIME CMD
    16087 pts/1    00:00:00 bash
    29664 pts/1    00:00:00 smallsh
    30317 pts/1    00:00:00 ps
    : ^Z
    Exiting foreground-only mode
    */
    disabled_background = disabled_background * -1;
    if (disabled_background == -1)
    {
        /*Background processes disabled*/
        write(1, "\nExiting foreground-only mode\n:", 32);
    }
    else if (disabled_background == 1)
    {
        /*Background processes enabled*/
        write(1, "\nEntering foreground-only mode (& is now ignored)\n:", 51);
    }
    fflush(stdout);
}

int main(int argc, char **argv)
{
    /*Variables initialization*/
    int background = 0;
    int numCharsEntered = -5;
    size_t bufferSize = 1024;
    char *line;
    pid_t pid = -5;
    /*Variable for holding write messages*/
    char msg[512];
    /*Current working Directory*/
    char cwd[100];
    getcwd(cwd, sizeof(cwd));

    unsigned int dollar_expansion;
    dollar_expansion = getpid();

    /*Ignore CTRL+C*/
    /*
    SIGINT does not terminate the shell, but only terminates the foreground command if one is running. 
    */
    struct sigaction SIGINT_handler_struct_parent = {0};

    sigset_t block_mask_parent;
    sigaddset(&block_mask_parent, SIGINT);
    sigaddset(&block_mask_parent, SIGQUIT);
    /*
    struct sigaction {
    void (*sa_handler)(int); - Pointer to a signal-catching function or one of the macros SIG_IGN or SIG_DFL.
    sigset_t sa_mask; - Additional set of signals to be blocked during execution of signal-catching function.
    int sa_flags; - Special flags to affect behavior of signal.
    void (*sa_restorer)(void); - old, Pointer to a signal-catching function.
    }
    */
    SIGINT_handler_struct_parent.sa_handler = SIG_IGN;
    SIGINT_handler_struct_parent.sa_mask = block_mask_parent;
    SIGINT_handler_struct_parent.sa_flags = 0;
    sigaction(SIGINT, &SIGINT_handler_struct_parent, NULL); /*Calling Sigaction function */

    /*Custom Signal Handling SIGTSTP - CTRL + Z*/
    /*
    foreground and background child processes should all ignore a SIGTSTP signal: only shell should react to it.
    */
    struct sigaction SIGTSTP_handler_struct_parent = {0};
    sigset_t block_mask_parent_sigtstp;
    /*adds the individual signal specified by the signo to the signal set pointed to by set*/
    sigaddset(&block_mask_parent_sigtstp, SIGTSTP);
    sigaddset(&block_mask_parent_sigtstp, SIGQUIT);
    SIGTSTP_handler_struct_parent.sa_handler = background_toggle;
    SIGTSTP_handler_struct_parent.sa_mask = block_mask_parent_sigtstp;
    /*SA_RESTART resumes the library function. Needed for avoiding infinite loop when CTRL+Z is entered by user to toggle background mode */
    SIGTSTP_handler_struct_parent.sa_flags = SA_RESTART; 
    /* Allows the calling process to examine and/or specify the action to be associated with a specific signal.  */
    sigaction(SIGTSTP, &SIGTSTP_handler_struct_parent, NULL); 
    while (1)
    {
        int status;
        pid_t kid;
        /*
        Check background process for completion
        This flag specifies that waitpid should return immediately instead of waiting, if there is no child process ready to be noticed.
        */
        while ((kid = waitpid(-1, &status, WNOHANG)) > 0)
        {
            /*
            If a background is completed, show the status and the pid. If killed by signal, show terminated by signal prompt
            */
            memset(msg, '\0', sizeof(msg));
            if (WIFEXITED(status) != 0)
            {
                /*
                : sleep 5 &
                background pid is 31003
                : 
                background pid 31003 is done:exit value 0
                */
                exitStatus = WEXITSTATUS(status);
                sprintf(msg, "background pid %d is done:exit value %d\n", kid, exitStatus);
                write(1, msg, strlen(msg));
                fflush(stdout);
            }

            if (WIFSIGNALED(status) != 0)
            {
                /*
                : sleep 20 &
                background pid is 31071
                : kill -15 31071
                background pid 31071 is done:terminated by signal 15
                */
                termSignal = WTERMSIG(status);
                sprintf(msg, "background pid %d is done:terminated by signal %d\n", kid, termSignal);
                write(1, msg, strlen(msg));
                fflush(stdout);
            }
        }
        /*
        User prompt for each command line. 
        */    
        write(STDOUT_FILENO, ": ", 2);
        fflush(stdout);

        line = (char *)malloc(bufferSize * sizeof(line));
        memset(line, '\0', sizeof(line));
        numCharsEntered = getline(&line, &bufferSize, stdin);
        line[numCharsEntered - 1] = '\0';
        /*Replacing $$ with process id*/
        int z;
        int counter = 0;
        /*Count the number of $$ replacements*/
        for (z = 0; z < numCharsEntered; z++)
        {
            if ((line[z] == '$') && (line[z + 1] == '$'))
            {
                counter++;
            }
        }
        /* Create a copy of user input with replacement for $$ with process id*/
        char *lineEntered = malloc((strlen(line) * sizeof(line)) + counter * (sizeof(unsigned int)));
        if (counter == 0)
        {
            /* No $$ Expansion */
            strcpy(lineEntered, line);
        }
        else
        {
            /*$$ Expansion present*/
            char *p = line;
            char *search = "$$";
            char *replace = malloc(21);
            sprintf(replace, "%d", dollar_expansion);
            while ((p = strstr(p, search)))
            {
                /*
                : ps 
                    PID TTY          TIME CMD
                16087 pts/1    00:00:00 bash
                30999 pts/1    00:00:00 smallsh
                31592 pts/1    00:00:00 ps
                : echo $$ is $$ is $$
                30999 is 30999 is 30999
                */
                strncpy(lineEntered, line, p - line);
                lineEntered[p - line] = '\0';
                strcat(lineEntered, replace);
                strcat(lineEntered, p + strlen(search));
                strcpy(line, lineEntered);
                p++;
            }
        }

        /*Breaking the arguments*/
        int k;
        int totalwords = 0;
        /* Count the total number of blanks in the input text*/
        for (k = 0; lineEntered[k] != '\0'; k++)
        {
            if (lineEntered[k] == ' ' || lineEntered[k] == '\n' || lineEntered[k] == '\t')
            {
                totalwords++;
            }
        }
        /* Get the total number of arguments*/
        int args = totalwords + 1;
        char **arguments;
        arguments = (char **)malloc(args * sizeof(char *));
        /* Dynamic memory allocation for the array*/
        for (k = 0; k < args; k++)
        {
            arguments[k] = (char *)malloc((100) * sizeof(char));
        }
        arguments[0] = strtok(lineEntered, " ");
        /* Split input text to arguments : to be used for file redirection and background checks*/        
        for (k = 1; k < args; k++)
        {
            arguments[k] = strtok(NULL, " ");
        }
        /*Skipping if comment of blank is entered*/
        if ((strcmp(lineEntered, "") == 0) || (strncmp(arguments[0], "#", 1) == 0))
        {
            /*
            : #this is a comment
            :           
            : # this is another comment
            */
            fflush(stdout);
            continue;
        }
        /* Check for background operator - Reduce the argument count if present*/
        if (strcmp(arguments[args - 1], "&") == 0)
        {
            /*
            : ls &
            background pid is 31948
            */
            background = 1;
            args = args - 1;
        }
        /*Check if CTRL+Z, SIGTSTP is entered*/
        if (disabled_background == 1)
        {
            /* Force foreground process*/
            background = 0;
        }

        /*  no forking for exit, cd and status */
        if (strcmp(arguments[0], "exit") == 0)
        {
            /*Exit shell with success*/
            /*: exit
            Exiting smallsh
            prateek (master #) smallsh
            $ echo $?
            0
            */
            write(1, "Exiting smallsh\n", 17);
            fflush(stdout);
            exit(EXIT_SUCCESS);
        }
        else if (strcmp(arguments[0], "cd") == 0)
        {
            /*Go to HOME if only cd is provided*/
            if (args == 1)
            {
                /*
                : pwd
                /home/prateek/C/smallsh
                : cd
                : pwd
                /home/prateek
                */
                chdir(getenv("HOME"));
            }
            else if (cd(arguments[1]) < 0)
            {
                /*Move to folder*/
                /*
                : pwd
                /home/prateek/C/smallsh
                : cd sample_cd_command
                : pwd
                /home/prateek/C/smallsh/sample_cd_command
                */
                perror(arguments[1]);
                fflush(stdout);
            }
        }
        else if (strcmp(arguments[0], "status") == 0)
        {
            /*Return status based on termination by signal or normal completion*/
            if (termSignal != 0)
            {
                /*
                : sleep 20 &
                background pid is 31071
                : kill -15 31071
                background pid 31071 is done:terminated by signal 15
                */
                memset(msg, '\0', sizeof(msg));
                sprintf(msg, "terminated by signal %d\n", termSignal);
                write(1, msg, strlen(msg));
            }
            else
            {
                /*
                : ls
                junk  junk2  p3testscript  sample_cd_command  smallsh  smallsh.c
                : status
                exit value 0
                */
                memset(msg, '\0', sizeof(msg));
                sprintf(msg, "exit value %d\n", exitStatus);
                write(1, msg, strlen(msg));
            }
            fflush(stdout);
        }

        else
        {
            /*the child gets 0, the parent gets the PID of the child  */
            pid = fork();
            if (pid == 0)
            {
                /* Child branch*/
                /*Ignore SIGTSTP - CTRL + Z*/
                /*
                foreground and background child processes should all ignore a SIGTSTP signal
                */
                struct sigaction SIGTSTP_handler_struct_child = {0};
                sigset_t block_mask_child_sigtstp;
                sigaddset(&block_mask_child_sigtstp, SIGTSTP);
                sigaddset(&block_mask_child_sigtstp, SIGQUIT);
                /*Ignoring signal with SIG_IGN*/
                SIGTSTP_handler_struct_child.sa_handler = SIG_IGN;
                SIGTSTP_handler_struct_child.sa_mask = block_mask_child_sigtstp;
                /*No special flags*/
                SIGTSTP_handler_struct_child.sa_flags = 0;
                sigaction(SIGTSTP, &SIGTSTP_handler_struct_child, NULL); /*Calling Sigaction function */

                if (background == 0)
                {
                    /*Killable by sig int now*/
                    /*the foreground child (if any) must terminate itself on receipt of SIGINT signal*/
                    struct sigaction SIGINT_handler_struct_child = {0};
                    sigset_t block_mask_child;
                    sigaddset(&block_mask_child, SIGINT);
                    sigaddset(&block_mask_child, SIGQUIT);
                    /*Reverting to default behaviour for SIGINT, kill the process*/
                    SIGINT_handler_struct_child.sa_handler = SIG_DFL;
                    SIGINT_handler_struct_child.sa_mask = block_mask_child;
                    SIGINT_handler_struct_child.sa_flags = 0;
                    sigaction(SIGINT, &SIGINT_handler_struct_child, NULL); /*Calling Sigaction function */
                }

                /*Checking if file redirection is present*/
                int hack_out = 0;
                int out_flag = 0;
                int hack_in = 0;
                int in_flag = 0;
                /* loop through all arguments */
                for (k = 0; k < args; k++)
                {
                    /* Create flag with position if output redirection is requested */
                    if (strcmp(arguments[k], ">") == 0)
                    {
                        hack_out = k;
                        out_flag = 1;
                    }
                    /* Create flag with position if input is provided */
                    if (strcmp(arguments[k], "<") == 0)
                    {
                        hack_in = k;
                        in_flag = 1;
                    }
                }
                char **filtered_arguments;
                filtered_arguments = (char **)malloc((args - in_flag - out_flag) * sizeof(char *));
                /*Dynamic Memory Allocation*/
                for (k = 0; k < args; k++)
                {
                    filtered_arguments[k] = (char *)malloc((100) * sizeof(char));
                }
                int z = 0;
                /* Remove redirections from arguments array*/
                for (k = 0; k < args; k++)
                {
                    if (!((strcmp(arguments[k], ">") == 0) || (strcmp(arguments[k], "<") == 0)))
                    {
                        /*Add argument to filter arguments array*/
                        strcpy(filtered_arguments[z], arguments[k]);
                        z++;
                    }
                    else
                    {
                        /* If any redirection is present, stop filtering*/
                        break;
                    }
                }
                /*Required for execvp, final argument should be null*/
                filtered_arguments[z] = NULL;
                int in_file, out_file;
                /**Create branching for dup2 file descriptors for background commands/
                /*No redirection*/
                if ((out_flag + in_flag) == 0)
                {
                    /*If background=1, redirect everything to dev null*/
                    if (background == 1)
                    {
                        in_file = open("/dev/null", O_RDONLY);
                        dup2(in_file, 0);
                        out_file = open("/dev/null", O_WRONLY);
                        dup2(out_file, 1);
                    }
                    /*Check if command entered by user is valid*/
                    if (execvp(filtered_arguments[0], filtered_arguments) == -1)
                    {
                        /*
                        : invalid_command
                        invalid_command: no such file or directory
                        */
                        memset(msg, '\0', sizeof(msg));
                        sprintf(msg, "%s: no such file or directory\n", filtered_arguments[0]);
                        write(1, msg, strlen(msg));
                        exit(EXIT_FAILURE);
                    }
                    /* Execute user command*/
                    execvp(filtered_arguments[0], filtered_arguments);
                    fflush(stdout);
                }
                /* if redirection is present*/
                else
                {
                    /*Check if the command has to be run on foreground or background*/    
                    if (background == 1)
                    {
                        /*Background commands redirected from /dev/null 
                        if the user did not specify some other file to 
                        take standard input from */
                        in_file = open("/dev/null", O_RDONLY);
                        dup2(in_file, 0);
                        out_file = open("/dev/null", O_WRONLY);
                        dup2(out_file, 1);
                    }
                    if (hack_out != 0)
                    {
                        /* Redirection to if output file is provided*/
                        if (background == 1)        
                        {
                            out_file = open("/dev/null", O_WRONLY);
                        }
                        else
                        {
                            /*Use the output file provided by user*/
                            out_file = open(arguments[hack_out + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IWRITE | S_IREAD);
                        }
                        dup2(out_file, 1);

                        filtered_arguments[z] = NULL;
                        /*sets the close-on-exec flag for the file descriptor, 
                        which causes the file descriptor to be automatically
                        (and atomically) closed when any of the exec-family functions succeed.*/
                        fcntl(out_file, F_SETFD, FD_CLOEXEC);
                    }

                    if (hack_in != 0)
                    {
                        /* Redirection to if output file is provided*/
                        if (background == 1)
                        {
                            in_file = open("/dev/null", O_RDONLY);
                        }
                        else 
                        {
                            /* Use the file provided by the user*/
                            in_file = open(arguments[hack_in + 1], O_RDONLY);
                        }
                        dup2(in_file, 0);
                        filtered_arguments[z] = arguments[hack_in + 1];
                        filtered_arguments[++z] = NULL;
                        /*sets the close-on-exec flag for the file descriptor, 
                        which causes the file descriptor to be automatically
                         (and atomically) closed when any of the exec-family functions succeed.*/
                        fcntl(in_file, F_SETFD, FD_CLOEXEC);
                    }
                    /*Check if command entered by user is correct*/
                    if (execvp(filtered_arguments[0], filtered_arguments) == -1)
                    {
                        /*
                        : invalid_command
                        invalid_command: no such file or directory
                        */
                        memset(msg, '\0', sizeof(msg));
                        sprintf(msg, "%s: no such file or directory\n", filtered_arguments[0]);
                        write(1, msg, strlen(msg));
                        fflush(stdout);
                        exit(EXIT_FAILURE);
                    }
                    /*Execute command*/
                    execvp(filtered_arguments[0], filtered_arguments);
                }
                fflush(stdout);
            }
            else
            {
                /* Parent branch after forking*/
                if (background == 1)
                {
                    /* If child is running in background, 
                    do not wait for it to complete*/
                    memset(msg, '\0', sizeof(msg));
                    sprintf(msg, "background pid is %d\n", pid);
                    /*Provide child pid on the screen*/
                    write(1, msg, strlen(msg));
                    fflush(stdout);
                    /*Reset flag for the next call*/
                    background = 0;
                    ;
                }
                else
                {
                    /* If child is running in foreground,
                    wait for it to complete */
                    waitpid(pid, &status, 0);
                    /*
                    When completed, show the status and the pid. If killed by signal, show terminated by signal prompt
                    */
                    if (WIFEXITED(status) != 0)
                    {
                        /*Update status variable*/
                        exitStatus = WEXITSTATUS(status);
                    }

                    if (WIFSIGNALED(status) != 0)
                    {
                        /*Show terminated by signal on screen with signal number*/
                        termSignal = WTERMSIG(status);
                        char message[80];
                        sprintf(message, "terminated by signal %d\n", termSignal);
                        /*Display to user*/
                        write(1, message, strlen(message));
                        fflush(stdout);
                    }
                }
            }
        }
    }

    return 0;
}
