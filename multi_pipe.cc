#include <unistd.h>
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

//void convertToChar(const char**, vector<string>&);

struct command
{
  const char **argv;
};

int spawn_proc (int in, int out, struct command *cmd)
{
  pid_t pid;

  if ((pid = fork ()) == 0)
    {
      if (in != 0)
        {
          dup2 (in, 0);
          close (in);
        }

      if (out != 1)
        {
          dup2 (out, 1);
          close (out);
        }

      return execvp (cmd->argv [0], (char * const *)cmd->argv);
    }

  return pid;
}

int fork_pipes (int n, struct command *cmd)
{
    int i;
    pid_t pid;
    int in, fd [2];

    /* The first process should get its input from the original file descriptor 0.  */
    in = 0;

    /* Note the loop bound, we spawn here all, but the last stage of the pipeline.  */
    for (i = 0; i < n - 1; ++i)
    {
        pipe (fd);

        /* f [1] is the write end of the pipe, we carry `in` from the prev iteration.  */
        spawn_proc (in, fd [1], cmd + i);

        /* No need for the write end of the pipe, the child will write here.  */
        close (fd [1]);

        /* Keep the read end of the pipe, the next child will read from there.  */
        in = fd [0];
    }

    /* Last stage of the pipeline - set stdin be the read end of the previous pipe
       and output to the original file descriptor 1. */
    if (in != 0)
        dup2 (in, 0);

    /* Execute the last stage with the current process. */
    return execvp (cmd [i].argv [0], (char * const *)cmd [i].argv);
}

int main ()
{
//    const char *ls[] = { "ls", "-l", 0 };
//    const char *awk[] = { "awk", "{print $1}", 0 };
//    const char *sort[] = { "sort", 0 };
//    const char *uniq[] = { "uniq", 0 };
//
//    struct command cmd [] = { {ls}, {awk}, {sort}, {uniq} };
//    cout << cmd[0].argv[0] << endl;
//    cout << cmd[0].argv[1] << endl;
//    cout << cmd[1].argv[0] << endl;
//    cout << cmd[1].argv[1] << endl;

//    return fork_pipes (4, cmd);

    string line;

    while(true) {

        if(!getline(cin, line)) {
            if(!cin.eof()){
                cerr << "Warning: fatal error while reading input from user" << endl;
            }
            break;
        }

//        cout << line << endl;
        vector<string> v1;
        boost::algorithm::trim(line);
        boost::algorithm::split(v1,line,boost::algorithm::is_any_of(" "),boost::token_compress_on);

//        cout << v1.size() << endl;
//        for (int i = 0; i < v1.size(); i++) {
//            cout << v1[i] << endl;
//        }
        int n = 0;
        for (auto& item : v1) {
//            cout << item << endl;
            if (item.compare("|") == 0) {
                n++;
            }
        }

        struct command cmds[n];

        const char* cc[n][256];
        int p = 0;
        int j = 0;
        for (auto& item : v1 ) {
            if (item.compare("|") == 0) {
//                cout << "p: " << p << endl;
                cc[p++][j] = nullptr;
                j = 0;
//                cout << cc[p-1][0] << endl;
//                cout << cc[p-1][1] << endl;
            } else {
                cc[p][j++] = &item.front();
//                cout << cc[p][j-1] << endl;
            }
        }

        for (int i = 0; i < n; i++) {
            cmds[i] = {cc[i]};
        }

//        cout << cmds[0].argv[0] << endl;
//        cout << cmds[0].argv[1] << endl;
//        cout << cmds[1].argv[0] << endl;
//        cout << cmds[1].argv[1] << endl;

        fork_pipes(n, cmds);
    }

    return 0;
}
