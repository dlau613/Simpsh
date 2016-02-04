#! /bin/sh

function should_fail() {
  result=$?;

  echo -n "==> $1 ";

  if [ $result -lt 1 ]; then
    echo "FAILURE";
    exit 1;
  else
    echo;
  fi
}

function should_succeed() {
  result=$?;

  echo -n "==> $1 ";

  if [ $result -gt 0 ]; then
    echo "FAILURE";
    exit 1;
  else
    echo;
  fi
}

>a
>b
>c
>d

echo "hello \nsome test \ntext to play \n with" > a

###### TO DO ###########
#if wait isn't declared and the only errors are in the subcommand itself then how do i report an error 
#	(since waitpid isn't checking the exit status of the commands)
#NEED TO FIGURE OUT HOW TO HANDLE WHEN OPTIONS THAT REQUIRE ARGUMENT DONT HAVE ANY
#should ./simpsh --invalid option ...working stuff   should exit with 1 or 0?
#should ./simpsh --rdonly --rdonly a ...working stuff  ??? how to deal with --rdonly

#syntax errors
#options requiring only one argument
./simpsh --rdonly a b  2>&1 | grep "STATUS: 1" > /dev/null
should_succeed "syntax error if option requiring exactly 1 argument has more"

#options requiring zero arguments
./simpsh --append a b 2>&1 | grep "STATUS: 1" > /dev/null
should_succeed "syntax error if options requring 0 arguments has 1 or more"

#command requires at least 4 arguments
./simpsh --command 0 1 2  2>&1 | grep "STATUS: 1" > /dev/null
should_succeed "syntax error if command has less than 4 arguments"

# #command errors
#command file descriptors must be numbers
./simpsh --command a 1 2 sort 2>&1 | grep "STATUS: 1" > /dev/null
should_succeed "syntax error if a file descriptor is a letter"

# #command file descriptors must be valid
./simpsh --command 0 1 2 sort 2>&1 | grep "STATUS: 1" > /dev/null
should_succeed "error if command has bad file descriptors"

./simpsh --rdonly a --wronly c --wronly d --command 0 1 2 cat --wait 2>&1 | grep "STATUS: 0"> /dev/null
should_succeed "--rdonly a --wronly c --wronly d --command cat 0 1 2 cat --wait"

#example from spec, if it can do this i should be okay for most real cases (handling bad commands is a different story)
./simpsh --rdonly a --pipe --pipe --creat --trunc --wronly c --creat --append --wronly d --command 0 2 6 sort \
--command 1 4 6 cat b - --command 3 5 6 tr a-z A-Z --wait 2>&1 | grep "STATUS: 0" > /dev/null
should_succeed "(sort < a | cat b - | tr a-z A-Z > c) 2>>d"

./simpsh --rdonly a --pipe --pipe --creat --trunc --wronly c --creat --append --wronly d --command 1 4 6 cat b - \
--command 0 2 6 sort --command 3 5 6 tr a-z A-Z --wait 2>&1 | grep "STATUS: 0" > /dev/null
should_succeed "put commands out of order"

./simpsh --rdonly a --pipe --pipe --creat --trunc --wronly c --creat --append --wronly d --command 0 2 6 sort \
 --command 3 5 6 tr a-z A-Z --command 1 4 6 cat b - --wait 2>&1 | grep "STATUS: 0" > /dev/null
 should_succeed "put commands out of order again"

./simpsh --rdonly a --pipe --pipe --creat --trunc --wronly c --creat --append --wronly d --command 0 2 6 sort \
--command 1 4 6 cat b - --command 3 5 6 tra a-z A-Z --wait 2>&1 | grep "STATUS: 1" > /dev/null
should_succeed "(sort < a | cat b - | tra a-z A-Z > c) 2>>d invald subcommand (tra) leads to error"

./simpsh --rdnly --rdonly a --wronly c --wronly d --command 0 1 2 cat --wait 2>&1 | grep "STATUS: 0" > /dev/null
should_succeed "recognize invalid options, but this recorded as an error (not sure how to)"

./simpsh --rdonly --rdonly a --wronly c --wronly d --command 0 1 2 cat --wait 2>&1 | grep "STATUS: 0" > /dev/null
should_succeed "recognize --rdonly with no argument as a syntax error and keep going"

./simpsh --rdonly a --wronly c --wronly d --command 0 1 3 cat 2>&1 | grep "STATUS: 0" > /dev/null
should_succeed "exit status is 0 is subcommand has an error but no wait"

./simpsh --rdonly a --wronly c --wronly d --command 0 1 3 cat --wait 2>&1 | grep "STATUS: 1" > /dev/null
should_succeed "exit status 1 if subcommand error"

./simpsh --rdonly a --wronly c --wronly d --command 0 1 2 cats 2>&1 | grep "STATUS: 0" > /dev/null
should_succeed "invalid subommand, no wait"

./simpsh --rdonly a --wronly c --wronly d --command 0 1 2 cats --wait 2>&1 | grep "STATUS: 1" > /dev/null
should_succeed "wait is declared and there is a invalid subcommand"

./simpsh --rdonly dafds --rdonly a --wronly c --wronly d --command 1 2 3 cat --wait 2>&1 | grep "STATUS: 0" > /dev/null
should_succeed "failed rdonly/wronly still takes up a logical file descriptor"

./simpsh --profile --rdonly a --pipe --pipe --creat --trunc --wronly c --creat --append --wronly d --command 0 2 6 sort \
--command 3 5 6 tr a-z A-Z --command 1 4 6 cat b - --wait 2>&1 | grep "STATUS: 0" > /dev/null
should_succeed "benchmark1"

./simpsh --profile --rdonly a --pipe --pipe --creat --trunc --wronly c --creat --append --wronly d --command 0 2 6 cat a \
--command 3 5 6 cat - a --command 1 4 6 cat - a a --wait 2>&1 | grep "STATUS: 0" >/dev/null
should_succeed "benchmark2"

./simpsh --profile --rdonly a --pipe --pipe --pipe --creat --trunc --wronly c --creat --append --wronly d --command 0 2 8 \
sort --command 1 4 8 cat - a --command 3 6 8 tr '[:space:]' 'z' --command 5 7 8 uniq -c --wait 2>&1 | grep "STATUS: 0" > /dev/null
should_succeed "benchmark3"

# ./simpsh --rdonly dafds --rdonly a --wronly c --wronly d --command 1 2 3 cat --wait --abort 2>&1 | grep "Segmentation" > /dev/null
# should_succeed "abort is working, i think"

# ./simpsh --catch 11 --abort 2>&1 | grep "11 caught" > /dev/null
# should_succeed "caught segmentation fault"

./simpsh --catch 11 --ignore 11 --abort --rdonly a --wronly c --wronly d --command 0 1 2 cat --wait 2>&1 | grep "STATUS: 0" > /dev/null
should_succeed "ignored abort and executed command"


echo "Woohoo"