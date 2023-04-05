/*
 * Copyright ©2023 Travis McGaha.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Pennsylvania
 * CIT 5950 for use solely during Spring Semester 2023 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <unistd.h>    // for fork()
#include <sys/types.h> // for pid_t
#include <sys/wait.h>  // for wait(), waitpid(), etc.

#include <iostream>
#include <string>
#include <cstring> // for strerror

#include <cstdlib>  // for exit(), EXIT_SUCCESS, and EXIT_FAILURE

#include <boost/algorithm/string.hpp> // for split(), trim()
#include <vector>

using std::cin;
using std::cout;
using std::string;
using std::endl;
using std::cerr;
using std::vector;

const int MAX_ARGS = 256;

// Pipes the first command's output into the second.
void pipe_cmd(char**, char**);

// Reads input from the user into the given array and returns the number of
// arguments taken in.
int read_args(char**, int&);

// Given a string of user input, this will determine if the user wants to
// quit the shell.
bool to_quit(string);

//void parse_command(int argc, char** argv, char** cmds[]);

//void parse_command(int, char**, char** cmds[][MAX_ARGS]);

int run_cmd(int, char**);

void pipe_cmd(char* cmds[][MAX_ARGS], int num_cmds);

int main() {
    // Todo: implement

    while (true) {

        char *argv[MAX_ARGS];
        int argc;
        // shell signature
        cout << "$ ";

        // read arg
        int n = 0;
        argc = read_args(argv, n);
//      cout << "n: " << n << endl;

        // parse command
        if (n == 0) {
//          cout << argv[0][0] << endl;
            run_cmd(argc, argv);
        } else {
//          parse_command(argc, argv, &cmds);
            char* cmds[n+1][MAX_ARGS];
            int c = 0;
            int j = 0;

            for (int i = 0; i < argc; i++) {
                if (strcmp(argv[i], "|") == 0) {
                    cmds[c++][j] = nullptr;
                    j = 0;
                } else {
                    cmds[c][j++] = argv[i];
//                  cout << argv[i] << endl;
                }
            }
            cmds[c][j] = nullptr;

            // execute command based on the command type
//            pipe_cmd(cmds[0], cmds[1]);
            pipe_cmd(cmds, n+1);
        }

        // Reset the argv array for next time.
//      for (int i=0; i<argc; i++) {
//          argv[i] = nullptr;
//      }

    }

    return EXIT_SUCCESS;
}

void pipe_cmd(char* cmds[][MAX_ARGS], int num_cmds) {
//    cout << "in pipe_cmd" << endl;
//    for (int i = 0; i < num_cmds; i++) {
//        cout << cmds[i][0] << endl;
//        cout << cmds[i][1] << endl;
//    }
    int fds[num_cmds - 1][2]; // file descriptors for pipes
    pid_t pids[num_cmds]; // child process ids
    int i;

    // create pipes
    for (i = 0; i < num_cmds - 1; i++) {
        if (pipe(fds[i]) < 0) {
            perror("pipe failed");
            exit(1);
        }
    }

    // create child processes
    for (i = 0; i < num_cmds; i++) {
        if ((pids[i] = fork()) < 0) {
            perror("fork failed");
            exit(1);
        } else if (pids[i] == 0) {
            // child process
            if (i == 0) {
                // first command, redirect stdout to pipe
                dup2(fds[i][1], STDOUT_FILENO);
            } else if (i == num_cmds - 1) {
                // last command, redirect stdin from pipe
                dup2(fds[i-1][0], STDIN_FILENO);
            } else {
                // middle command, redirect both stdin and stdout from/to pipe
                dup2(fds[i-1][0], STDIN_FILENO);
                dup2(fds[i][1], STDOUT_FILENO);
            }

            // close all pipe ends
            int j;
            for (j = 0; j < num_cmds - 1; j++) {
                close(fds[j][0]);
                close(fds[j][1]);
            }

            // execute command
            execvp(cmds[i][0], cmds[i]);
            perror("execvp failed");
            exit(1);
        }
    }

    // close all pipe ends in parent process
    for (i = 0; i < num_cmds - 1; i++) {
        close(fds[i][0]);
        close(fds[i][1]);
    }

    // wait for all child processes to finish
    for (i = 0; i < num_cmds; i++) {
        waitpid(pids[i], NULL, 0);
    }

}

//void parse_command(int argc, char** argv, char** cmds[][MAX_ARGS]) {
//    int c = 0;
//    int j = 0;
//
//    for (int i = 0; i < argc; i++) {
//        if (strcmp(argv[i], "|") == 0) {
//            cmds[c++][j] = nullptr;
//            j = 0;
//        } else {
//            cmds[c][j++] = argv[i];
//        }
//    }
//
//}

//void parse_command(int argc, char** argv, char** cmds[]) {
//
//}

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


int run_cmd(int argc, char** argv) {

    pid_t pid = fork();
    if (!pid) {
        // child
        char** args = new char*[argc+1];
        for (int i = 0; i < argc; i++) {
            args[i] = argv[i];
        }
        args[argc] = nullptr; // null terminate args array
        execvp(argv[0], args);

        // Exec didn't work, so an error must have been encountered
        cerr << strerror(errno) << endl;
        delete[] args;
        exit(EXIT_FAILURE);
    }

    // parent
    waitpid(pid, nullptr, 0);
    return EXIT_SUCCESS;
}

// Given a string of user input, this determines whether or not the user
// wants to exit the shell.
bool to_quit(string word) {
    return word.compare("exit") == 0;
}

int read_args(char** argv, int& n) {
    string line;

    if (!getline(cin, line) || to_quit(line)) {
        if (!cin.eof()) {
            cerr << "Warning: illegal eof..." << endl;
            exit(1);
        }
        exit(0);
    }

    vector<string> v1;
    boost::algorithm::trim(line);
    boost::algorithm::split(v1, line, boost::is_any_of(" "), boost::token_compress_on);

    int p = 0;
    for (auto& w : v1) {
        if (w.compare("|") == 0) {
            n++;
        }
        argv[p++] = &w.front();
    }

    return p;
}
