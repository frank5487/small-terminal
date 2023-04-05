#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

using std::cin;
using std::cout;
using std::string;
using std::endl;
using std::cerr;

// Will be used to create an array to hold individual arguments passed by
// the user on the command line.
const int MAX_ARGS = 256;

enum PipeRedirect {PIPE, REDIRECT, NEITHER};

// Splits a user's command into two commands, or a command and a file name.
PipeRedirect parse_command(int, char**, char**, char**);

// Pipes the first command's output into the second.
void pipe_cmd(char**, char**);

// Reads input from the user into the given array and returns the number of
// arguments taken in.
int read_args(char**);

// Redirects the output from the given command into the given file.
void redirect_cmd(char**, char**);

// Given the number of arguments and an array of arguments, this will execute
// those arguments.  The first argument in the array should be a command.
void run_cmd(int, char**);

// Given a string of user input, this will determine if the user wants to
// quit the shell.
bool want_to_quit(string);

int main() {
    char *argv[MAX_ARGS], *cmd1[MAX_ARGS], *cmd2[MAX_ARGS];
    PipeRedirect pipe_redirect;
    int argc;

    // Keep returning the user to the prompt ad infinitum unless they enter
    // 'quit' or 'exit' (without quotes).
    while (true) {
        // Display a prompt.
        cout << "SarahShell> ";

        // Read in a command from the user.
        argc = read_args(argv);

        // Decipher the command we just read in and split it, if necessary, into
        // cmd1 and cmd2 arrays.  Set pipe_redirect to a PipeRedirect enum value to
        // indicate whether the given command pipes, redirects, or does neither.
        pipe_redirect = parse_command(argc, argv, cmd1, cmd2);

        // Determine how to handle the user's command(s).
        if (pipe_redirect == PIPE)          // piping
            pipe_cmd(cmd1, cmd2);
        else if (pipe_redirect == REDIRECT) // redirecting
            redirect_cmd(cmd1, cmd2);
        else
            run_cmd(argc, argv);              // neither

        // Reset the argv array for next time.
        for (int i=0; i<argc; i++)
            argv[i] = NULL;
    }

    // Let the OS know everything is a-okay.
    return 0;
}

// Given the number of arguments (argc) in an array of arguments (argv), this
// will go through those arguments and, if necessary, bifurcate the arguments
// into arrays cmd1 and cmd2.  It will return a PipeRedirect enum representing
// whether there was a pipe in the command, a redirect to a file, or neither.
// cmd1 and cmd2 will only be populated if there was a pipe or a redirect.
PipeRedirect parse_command(int argc, char** argv, char** cmd1, char** cmd2) {
    // Assume no pipe or redirect will be found.
    PipeRedirect result = NEITHER;

    // Will hold the index of argv where the pipe or redirect is found.
    int split = -1;

    // Go through the array of arguments...
    for (int i=0; i<argc; i++) {
        // Pipe found!
        if (strcmp(argv[i], "|") == 0) {
            result = PIPE;
            split = i;

            // Redirect found!
        } else if (strcmp(argv[i], ">>") == 0) {
            result = REDIRECT;
            split = i;
        }
    }

    // If either a pipe or a redirect was found...
    if (result != NEITHER) {
        // Go through the array of arguments up to the point where the
        // pipe/redirect was found and store each of those arguments in cmd1.
        for (int i=0; i<split; i++)
            cmd1[i] = argv[i];

        // Go through the array of arguments from the point where the pipe/redirect
        // was found through the end of the array of arguments and store each
        // argument in cmd2.
        int count = 0;
        for (int i=split+1; i<argc; i++) {
            cmd2[count] = argv[i];
            count++;
        }

        // Terminate cmd1 and cmd2 with NULL, so that execvp likes them.
        cmd1[split] = NULL;
        cmd2[count] = NULL;
    }

    // Return an enum showing whether a pipe, redirect, or neither was found.
    return result;
}

// This pipes the output of cmd1 into cmd2.
void pipe_cmd(char** cmd1, char** cmd2) {
    int fds[2]; // file descriptors
    pipe(fds);
    pid_t pid;

    // child process #1
    if (fork() == 0) {
        // Reassign stdin to fds[0] end of pipe.
        dup2(fds[0], 0);

        // Not going to write in this child process, so we can close this end
        // of the pipe.
        close(fds[1]);

        // Execute the second command.
        execvp(cmd2[0], cmd2);
        perror("execvp failed");

        // child process #2
    } else if ((pid = fork()) == 0) {
        // Reassign stdout to fds[1] end of pipe.
        dup2(fds[1], 1);

        // Not going to read in this child process, so we can close this end
        // of the pipe.
        close(fds[0]);

        // Execute the first command.
        execvp(cmd1[0], cmd1);
        perror("execvp failed");

        // parent process
    } else
        waitpid(pid, NULL, 0);
}

// This will get input from the user, split the input into arguments, insert
// those arguments into the given array, and return the number of arguments as
// an integer.
int read_args(char **argv) {
    char *cstr;
    string arg;
    int argc = 0;

    // Read in arguments till the user hits enter
    while (cin >> arg) {
        // Let the user exit out if their input suggests they want to.
        if (want_to_quit(arg)) {
            cout << "Goodbye!\n";
            exit(0);
        }

        // Convert that std::string into a C string.
        cstr = new char[arg.size()+1];
        strcpy(cstr, arg.c_str());
        argv[argc] = cstr;

        // Increment our counter of where we're at in the array of arguments.
        argc++;

        // If the user hit enter, stop reading input.
        if (cin.get() == '\n')
            break;
    }

    // Have to have the last argument be NULL so that execvp works.
    argv[argc] = NULL;

    // Return the number of arguments we got.
    return argc;
}

void redirect_cmd(char** cmd, char** file) {
    int fds[2]; // file descriptors
    int count;  // used for reading from stdout
    int fd;     // single file descriptor
    char c;     // used for writing and reading a character at a time
    pid_t pid;  // will hold process ID; used with fork()

    pipe(fds);

    // child process #1
    if (fork() == 0) {
        // Thanks to http://linux.die.net/man/2/open for showing which headers
        // need to be included to use this function and its flags.
        fd = open(file[0], O_RDWR | O_CREAT, 0666);

        // open() returns a -1 if an error occurred
        if (fd < 0) {
            printf("Error: %s\n", strerror(errno));
            return;
        }

        dup2(fds[0], 0);

        // Don't need stdout end of pipe.
        close(fds[1]);

        // Read from stdout...
        while ((count = read(0, &c, 1)) > 0)
            write(fd, &c, 1); // Write to file.

        // Okay, so this is a bit contrived, but when I didn't have any kind of exec
        // function call here, I got my SarahShell prompt repeated over and over
        // again on the Multilab machines, I think because of this crazy child
        // process or something.  When I put this execlp here with the useless call
        // to echo, however, that looping stops and you can actually enter things
        // at the prompt again, hurray!
        execlp("echo", "echo", NULL);

        // child process #2
    } else if ((pid = fork()) == 0) {
        dup2(fds[1], 1);

        // Don't need stdin end of pipe.
        close(fds[0]);

        // Output contents of the given file to stdout.
        execvp(cmd[0], cmd);
        perror("execvp failed");

        // parent process
    } else {
        waitpid(pid, NULL, 0);
        close(fds[0]);
        close(fds[1]);
    }
}

// Given the number of arguments (argc) and an array of arguments (argv),
// this will fork a new process and run those arguments.
// Thanks to http://tldp.org/LDP/lpg/node11.html for their tutorial on pipes
// in C, which allowed me to handle user input with ampersands.
void run_cmd(int argc, char** argv) {
    pid_t pid;
    const char *amp;
    amp = "&";
    bool found_amp = false;

    // If we find an ampersand as the last argument, set a flag.
    if (strcmp(argv[argc-1], amp) == 0)
        found_amp = true;

    // Fork our process
    pid = fork();

    // error
    if (pid < 0)
        perror("Error (pid < 0)");

        // child process
    else if (pid == 0) {
        // If the last argument is an ampersand, that's a special flag that we
        // don't want to pass on as one of the arguments.  Catch it and remove
        // it here.
        if (found_amp) {
            argv[argc-1] = NULL;
            argc--;
        }

        execvp(argv[0], argv);
        perror("execvp error");

        // parent process
    } else if (!found_amp)
        waitpid(pid, NULL, 0); // only wait if no ampersand
}

// Given a string of user input, this determines whether or not the user
// wants to exit the shell.
bool want_to_quit(string choice) {
    // Lowercase the user input
    for (unsigned int i=0; i<choice.length(); i++)
        choice[i] = tolower(choice[i]);

    return (choice == "quit" || choice == "exit");
}