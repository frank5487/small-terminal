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
#include <sys/wait.h>  // for waitpid
#include <cstring>  // for strerror
#include <cstdlib>  // for exit(), EXIT_FAILURE, and EXIT_SUCCESS

#include <iostream>

using std::cin;
using std::cout;
using std::cerr;
using std::endl;

// given some command and args for it as
// command line args to this program
// forks a process to run that command
// with the specified commands

int main(int argc, char** argv) {
  if (argc <= 1) {
    cerr << "Usage: " << argv[0] << " command <command_args>*" << endl;
    return EXIT_FAILURE;
  }
  
  pid_t pid = fork();
  if (!pid) {
    // child
    char** args = new char*[argc];
    for (int i = 1; i < argc; i++) {
      args[i-1] = argv[i];
    }
    args[argc-1] = nullptr; // null terminate args array
    execvp(argv[1], args);
    
    // Exec didn't work, so an error must have been encountered
    cerr << strerror(errno) << endl;
    delete[] args;
    exit(EXIT_FAILURE);
  }

  // parent
  waitpid(pid, nullptr, 0);
  return EXIT_SUCCESS;
}

/*
int main(int argc, char** argv) {
    if (argc <= 1) {
        cerr << "Usage: " << argv[0] << " commands:<command args>" << endl;
        return EXIT_FAILURE;
    }

    int pid = fork();
    if (pid == 0) {
        char** args = new char*[argc];

        for (int i = 1; i < argc; i++) {
            args[i-1] = argv[i];
        }

        args[argc-1] = nullptr;

        execvp(argv[1], args);

        cerr << strerror(errno) << endl;
        delete[] args;
        exit(EXIT_FAILURE);
    }

    waitpid(pid, nullptr, 0);
    return EXIT_SUCCESS;
}
 */