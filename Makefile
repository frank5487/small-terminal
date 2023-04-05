# Copyright ©2023 Travis McGaha.  All rights reserved.  Permission is
# hereby granted to students registered for University of Pennsylvania
# CIT 5950 for use solely during Spring Semester 2023 for purposes of
# the course.  No other use, copying, distribution, or modification
# is permitted without prior written consent. Copyrights for
# third-party components of this work must be honored.  Instructors
# interested in reusing these course materials should contact the
# author.

all: pipe_shell sh stdin_echo

pipe_shell: pipe_shell.cc
	g++ -g -Wall -std=c++11 -o pipe_shell pipe_shell.cc

sh: sh.cc
	g++ -g -Wall -std=c++11 -o sh sh.cc

stdin_echo: stdin_echo.cc
	g++ -g -Wall -std=c++11 -o stdin_echo stdin_echo.cc

multi_pipe: multi_pipe.cc
	g++ -g -Wall -std=c++11 -o multi_pipe multi_pipe.cc

two_pipe: two_pipe.cc
	g++ -g -Wall -std=c++11 -o two_pipe two_pipe.cc

fail_pipe_shell: fail_pipe_shell.cc
	g++ -g -Wall -std=c++11 -o fail_pipe_shell fail_pipe_shell.cc

clean:
	rm -f *.o pipe_shell sh stdin_echo
