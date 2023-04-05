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

int read_args(vector<string>&);

bool to_quit(string);

int run_cmd(const vector<string>&);

void parse_commands(const vector<string>&, vector<vector<string>>&);

void pipe_cmds(const vector<vector<string>>& cmds);

int main() {
    // Todo: implement

    while (true) {

        // shell signature
        cout << "$ ";

        vector<string> args;
        int n = read_args(args);

        if (n == 0) {
            run_cmd(args);
        } else {
            // Parse the input into individual commands.
            vector<vector<string>> cmds;
            parse_commands(args, cmds);

            // Execute the pipeline of commands.
            pipe_cmds(cmds);
        }

    }

    return EXIT_SUCCESS;
}

void parse_commands(const vector<string>& args, vector<vector<string>>& cmds) {
    vector<string> t;
    for (const auto& arg : args) {
        if (arg.compare("|") == 0) {
            cmds.push_back(t);
            t.clear();
        } else {
            t.push_back(arg);
        }
    }
    cmds.push_back(t);
    t.clear();

}

void pipe_cmds(const vector<vector<string>>& cmds) {
    int num_cmds = cmds.size();

    // Create pipes.
    vector<int[2]> pipes(num_cmds - 1);
    for (int i = 0; i < num_cmds - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe failed");
            exit(1);
        }
    }

    // Execute the commands in the pipeline.
    vector<pid_t> pids(num_cmds);
    for (int i = 0; i < num_cmds; i++) {
        if ((pids[i] = fork()) < 0) {
            perror("fork failed");
            exit(1);
        } else if (pids[i] == 0) {
            // Child process.

            // Set up input redirection from the previous command, if there is one.
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }

            // Set up output redirection to the next command, if there is one.
            if (i < num_cmds - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            // Close all pipe ends.
            for (int j = 0; j < num_cmds - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Execute the command.
            const vector<string> &cmd = cmds[i];
            char **argv = new char *[cmd.size() + 1];
            for (size_t j = 0; j < cmd.size(); j++) {
                argv[j] = const_cast<char *>(cmd[j].c_str());
            }
            argv[cmd.size()] = nullptr;
            execvp(argv[0], argv);
            
            perror("execvp failed");
            delete[] argv;
            exit(1);
        }
    }
    // close all pipe ends in parent process
    for (int i = 0; i < num_cmds - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // wait for all child processes to finish
    for (int i = 0; i < num_cmds; i++) {
        waitpid(pids[i], nullptr, 0);
    }
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

bool to_quit(string word) {
    return word.compare("exit") == 0;
}

int read_args(vector<string>& args) {
    string line;

    if (!getline(cin, line) || to_quit(line)) {
        if(!cin.eof()){
            // complex output required
            exit(1);
        }
        // simple output required
        cout << endl;
        exit(0);
    }

    boost::algorithm::trim(line);
    boost::algorithm::split(args, line, boost::is_any_of(" "), boost::token_compress_on);

    int n = 0;
    for (auto& w : args) {
        if (w.compare("|") == 0) {
            n++;
        }
    }

    return n;
}

int run_cmd(const vector<string>& args) {
    pid_t pid = fork();
    if (pid == 0) {
        // child
        char** argv = new char*[args.size()+1];
        for (size_t i = 0; i < args.size(); i++) {
            argv[i] = const_cast<char*>(args[i].c_str());
        }
        argv[args.size()] = nullptr; // null terminate args array
        execvp(argv[0], argv);

        // Exec didn't work, so an error must have been encountered
        cerr << strerror(errno) << endl;
        exit(EXIT_FAILURE);
    }

    // parent
    waitpid(pid, nullptr, 0);
    return EXIT_SUCCESS;
}