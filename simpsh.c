#define _GNU_SOURCE
// #define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>

#include <ucontext.h>
#include <sys/mman.h>


static int append_flag = 0, cloexec_flag = 0, creat_flag = 0, directory_flag = 0, dsync_flag = 0, excl_flag = 0, nofollow_flag = 0, 
	nonblock_flag = 0, rsync_flag = 0, sync_flag = 0, trunc_flag = 0;
static int verbose_flag = 0, wait_flag = 0;

void segfault_sighandler(int signal, siginfo_t* s, void * arg) {
	printf("%d caught\n", signal);
	exit(signal);
}
void segfault_sighandler_ignore(int signal, siginfo_t*  s, void * arg) {
  printf("trying to ignore\n");
	ucontext_t  *context = (ucontext_t *) arg;
	context->uc_mcontext.gregs[REG_RIP]++;
}
int main(int argc, char *argv[]) 
{
	int length = argc;
	int logical_fd = 0;
	int fd[length];
	int fdinfo[length];
	int fd1;
	int fdpipe[2];
	int c;
	int input, output,error;
	int i,j,k;
	pid_t pid[argc];
	int pid_done[argc];
	int status;
	int pid_total;
	int exit_status;
	char * option_args[argc];
	char * string_status;
	char temp_buff[1000];
	int temp;
	char * command_stats[argc];
	int errnum;
	int option_index = 0;

	for (i = 0; i < length; i++) {//initialize as -1 otherwise they're all valid as 0
		fd[i] = -1;
	}

	for (i = 0; i < argc; i++) {
		pid_done[i] = 0;
		option_args[i] = NULL;
		command_stats[i] = 0;
	}
	temp = 0;
	exit_status = 0;
	k = 0; //keeps track of the number of subcommands
	while(1)//use getopt_long to get one option at a time from left to right and execute it
	{
		static struct option long_options[] =
		{
			//file flags
			{"append", no_argument, &append_flag,1},
			{"cloexec", no_argument, &cloexec_flag,1},
			{"creat", no_argument, &creat_flag,1},
			{"directory", no_argument, &directory_flag,1},
			{"dsync", no_argument, &dsync_flag,1},
			{"excl", no_argument, &excl_flag,1},
			{"nofollow", no_argument, &nofollow_flag,1},
			{"nonblock", no_argument, &nonblock_flag,1},
			{"rsync", no_argument, &rsync_flag,1},
			{"sync", no_argument, &sync_flag,1},
			{"trunc", no_argument, &trunc_flag,1},

			//file opening options
			{"rdonly", required_argument, 0, 'r'},
			{"rdwr", required_argument, 0, 'b'},
			{"wronly", required_argument, 0, 'w'},
			{"pipe", no_argument, 0, 'p'},

			//subcommand options
			{"command", required_argument, 0, 'c'},
			{"wait", no_argument, &wait_flag, 1},

			//miscellaneous options
			{"close", required_argument, 0, 'j'},
			{"verbose", no_argument, 0, 'v'},
			{"profile", no_argument, 0, 1},
			{"abort", no_argument, 0, 'a'},
			{"catch", required_argument, 0, 'e'},
			{"ignore", required_argument, 0, 'i'},
			{"default", required_argument, 0, 'd'},
			{"pause", no_argument, 0, 'f'},

			{0, 0, 0, 0}
		};

		option_index = 0;
		c= getopt_long(argc,argv, "", long_options, &option_index);
		if (c == -1)
		{
			for (j=0; j < length; j++) {
				close(fd[j]);
			}
			break;
		}
		if (c == '?') {
			// optind--;
			// printf("optarg is %s\n",optarg);
			continue;
		}
		if (optarg != NULL)//if there is a command that takes exactly one argument but has none
			if (optarg[0] == '-')//ie --rdonly --rdonly a --wronly c....valid command
				if (optarg[1] == '-')//it ends up giving taking --rdonly a as the optarg 
				{						//this code stops that and decrements optind so that
					optind--;	//the next getopt_long will get the working --rdonly a
					exit_status = 1;
					fprintf(stderr, "Syntax error: --%s requires exactly 0 arguments\n", long_options[option_index].name);
					continue;
				}

		option_args[0] = optarg;
		i = 1;
		while (optind < argc ){//pull and store extra arguments till the next option is reached
			if (argv[optind][0] == '-') {//stop when you reach the next option
				if (argv[optind][1] == '-')
					break;
			}
			option_args[i] = argv[optind];
			i++;									
			optind++;
		}
		option_args[i] = NULL;//last one should be NULL for execvp (also helps for determing how many args there are)
		
		i = 0;
		strcpy(temp_buff, "\0");//temp_buff holds all the option arguments
		while (option_args[i] != NULL) {//turn all the option_args into one nice string
			strcat(temp_buff, option_args[i]);
			strcat(temp_buff, " ");
			i++;
		}
		//format the string to be printed out if --verbose was declared. add the option in front of the args
		asprintf(&string_status, "--%s %s\n", long_options[option_index].name, temp_buff);
		if (verbose_flag) //PUT THIS BACK LATER
			printf("%s", string_status);
		

		//check for syntax errors after we've put together the 
		if (long_options[option_index].has_arg == no_argument) //if 0 args required then NULL is stored into option_args[0]
			if (option_args[1] != NULL) {//check if option_args one isn't NULL
				fprintf(stderr, "Syntax error: --%s requires exactly 0 arguments\n", long_options[option_index].name);
				exit_status = 1;
				continue;//print the syntax error, set exit_status and keep going
			}
		if (long_options[option_index].has_arg == required_argument) {
			if (c == 'c') {//if it was the --command option
				if (option_args[3] == NULL) { //if there's only three arguments then break
					fprintf(stderr, "Syntax error: --command requires at least 4 arguments\n");
					exit_status = 1;
					continue;
				}
			}
			else {//if not the --command option but requires argument, then there should be exactly one argument
				if (option_args[1] != NULL) {//then there's more than 1 argument
					fprintf(stderr, "Syntax error: --%s requires exactly 1 argument , optarg is %s\n", long_options[option_index].name,option_args[1]);
					exit_status = 1;
					continue;
				}
				if (option_args[0] == NULL) {//then there was no argument
					fprintf(stderr, "Syntax error: --%s requires exactly 1 argument , optarg is %s\n", long_options[option_index].name,option_args[1]);
					exit_status = 1;
					continue;
				}
			}
		}

		switch(c)
		{
			case 0:
				break;

			case 1:
				break;

			case 'a': { //abort
				int * a = NULL;
				int b = *a;
				break;
			}

			case 'c': {//command
				if (isdigit(option_args[0][0])) //if the first argument (input) is not a number then break
					input = atoi(option_args[0]);
				else {
					fprintf(stderr, "Command syntax error: input fd is not a digit: %s\n", option_args[0]);
					exit_status = 1;
					break;
				}
				if (isdigit(option_args[1][0])) //if the second argument (output) is not a number then break
					output = atoi(option_args[1]);
				else {
					fprintf(stderr, "Command syntax error: output fd is not a digit: %s\n", option_args[1]);
					exit_status = 1;
					break;
				}
				if (isdigit(option_args[2][0])) //if the third argument (error) is not a number then break
					error = atoi(option_args[2]);
				else {
					fprintf(stderr, "Command syntax error: error fd is not a digit: %s\n", option_args[2]);
					exit_status = 1;
					break;
				}
				
				pid[k] = fork();//keep track of all the child process id numbers to be waited on at the end
				if (pid[k] == -1) {
					fprintf(stderr,"System call fork failed: %s\n", strerror(errno));
				}
				else if (pid[k] == 0) {
					if (dup2(fd[input], 0) == -1) {
						fprintf(stderr, "System call dup2 failed on logical file descriptor %d: %s\n", input, strerror(errno));
						exit(1);
					}
					if (dup2(fd[output], 1 ) == -1) {
						fprintf(stderr, "System call dup2 failed on logical file descriptor %d: %s\n", output, strerror(errno));
						exit(1);
					}
					if (dup2(fd[error], 2) == -1) {
						fprintf(stderr, "System call dup2 failed on logical file descriptor %d: %s\n", error, strerror(errno));
						exit(1);
					}
					if (close(fd[input]) == -1) {
						fprintf(stderr, "System call close failed on logical file descriptor %d: %s\n",input, strerror(errno));
						exit(1);
					}
					if (close(fd[output]) == -1) {
						fprintf(stderr, "System call close failed on logical file descriptor %d: %s\n",output, strerror(errno));
						exit(1);
					}
					if (close(fd[error]) == -1) {
						fprintf(stderr, "System call close failed on logical file descriptor %d: %s\n",error, strerror(errno));
						exit(1);
					}
					// if (fdinfo[input] != 0)
					// 	close(fd[input+1]);
					// if (fdinfo[output] != 0)
					// 	close(fd[input-1]);
					if (execvp(option_args[3], option_args+3) == -1) {
						fprintf(stderr,"System call execvp failed: %s\n", strerror(errno));
						exit(1); 
					}
				}
				else {	
					// printf("this is parent\n");
					if (fdinfo[input] != 0)	{//if input is the read end of the pipe, close it
						if (close(fd[input]) == -1) {
							fprintf(stderr, "aSystem call close failed on logical file descriptor %d: %s\n",input, strerror(errno));
							exit_status = 1;
						}
					}
					if (fdinfo[output] != 0) {//if output is the write end of the pipe, close it 
						if (close(fd[output]) == -1) {
							fprintf(stderr, "bSystem call close failed on logical file descriptor %d: %s\n",output, strerror(errno));
							exit_status = 1;
						}
					}
					//make string with format "--command optarg1 optarg2 ...."
					asprintf(&command_stats[k], "%s %s\n", long_options[option_index].name, temp_buff);
					k++;//increment the number of subcommands
				}
				break;
			}


			case 'b': {//rdwr
				if ((fd1 = open(optarg, O_RDWR | append_flag*O_APPEND | cloexec_flag*O_CLOEXEC | creat_flag*O_CREAT | directory_flag*O_DIRECTORY |
					dsync_flag*O_DSYNC | excl_flag*O_EXCL | nofollow_flag*O_NOFOLLOW | nonblock_flag*O_NONBLOCK | /*rsync_flag*O_RSYNC | */
					sync_flag*O_SYNC | trunc_flag*O_TRUNC , 0644)) == -1)
					errnum == errno;
				fd[logical_fd] = fd1;
				fdinfo[logical_fd++] = 0;//fdinfo[i]=1 means its a pipe
				if (fd1 == -1) 
				{
					fprintf(stderr, "Error opening file named %s: %s\n", optarg, strerror(errno));
					exit_status = 1;
					// break;
				}
				append_flag =0; cloexec_flag =0; creat_flag=0; directory_flag=0; dsync_flag=0; excl_flag=0; 
					nofollow_flag=0; nonblock_flag=0; rsync_flag=0; sync_flag=0; trunc_flag=0;
				// printf("%s fd is %d\n",optarg,fd1);
				break;
			}

			case 'r': {//read
				if ((fd1 = open(optarg, O_RDONLY | append_flag*O_APPEND | cloexec_flag*O_CLOEXEC | creat_flag*O_CREAT | directory_flag*O_DIRECTORY |
					dsync_flag*O_DSYNC | excl_flag*O_EXCL | nofollow_flag*O_NOFOLLOW | nonblock_flag*O_NONBLOCK | /*rsync_flag*O_RSYNC | */
					sync_flag*O_SYNC | trunc_flag*O_TRUNC , 0644)) == -1)
					errnum = errno;
				printf("logical fd is %d, actual fd is %d\n", logical_fd, fd1);
				fd[logical_fd] = fd1;
				fdinfo[logical_fd++] = 0;
				if (fd1 == -1)
				{
					fprintf(stderr, "Error opening file named %s: %s\n", optarg, strerror(errnum));
					exit_status = 1;
					// break;
				}
				append_flag =0; cloexec_flag =0; creat_flag=0; directory_flag=0; dsync_flag=0; excl_flag=0; 
					nofollow_flag=0; nonblock_flag=0; rsync_flag=0; sync_flag=0; trunc_flag=0;
				break;
			}

			case 'w': {//write
				if ((fd1 = open(optarg, O_WRONLY  | append_flag*O_APPEND | cloexec_flag*O_CLOEXEC | creat_flag*O_CREAT | directory_flag*O_DIRECTORY |
					dsync_flag*O_DSYNC | excl_flag*O_EXCL | nofollow_flag*O_NOFOLLOW | nonblock_flag*O_NONBLOCK | /*rsync_flag*O_RSYNC | */
					sync_flag*O_SYNC | trunc_flag*O_TRUNC , 0644)) == -1)
					errnum = errno;
				printf("logical fd is %d, actual fd is %d\n",logical_fd, fd1);
				fd[logical_fd] = fd1;
				fdinfo[logical_fd++] = 0;
				if (fd1 == -1) 
				{
					fprintf(stderr, "Error opening file named %s: %s\n", optarg, strerror(errnum));
					exit_status = 1;
					// break;
				}
				append_flag =0; cloexec_flag =0; creat_flag=0; directory_flag=0; dsync_flag=0; excl_flag=0; 
					nofollow_flag=0; nonblock_flag=0; rsync_flag=0; sync_flag=0; trunc_flag=0;
				break;
			}

			case 'p': {//pipe
				i = pipe(fdpipe);

				if(i == -1)
					fprintf(stderr, "Error making pipe: %s\n", strerror(errno));
				fd[logical_fd] = fdpipe[0];
				fdinfo[logical_fd++] = 1;
				fd[logical_fd] = fdpipe[1];
				fdinfo[logical_fd++] = 1;
				// printf("pipe fd's are %d and %d\n", fdpipe[0], fdpipe[1]);	
				break;
			}

			case 'j': {//close i
				printf("closed logical fd %s, actual fd %d\n", optarg, fd[atoi(optarg)]);
				fd[atoi(optarg)] = -1;
				break;
			}
			case 'v': {//verbose
				verbose_flag = 1;
				break;
			}

			case 'd': {//default
				struct sigaction sa;
				sa.sa_handler = SIG_DFL;
				sigaction(atoi(optarg), &sa, NULL);
				break;
			}
			case 'e': {//catch
				struct sigaction sa;
				sa.sa_sigaction = segfault_sighandler;
				sigaction(atoi(optarg), &sa, NULL);
				break;
			}

			case 'i': {//ignore
			    struct sigaction sa;
			  	sa.sa_sigaction = segfault_sighandler_ignore;
			  	sigaction(atoi(optarg), &sa, NULL);
				break;
			}

			case 'f': {//pause
				pause();//is this all i do?
				break;
			}
			case '?': {
				printf("in question mark case \n");
				exit_status = 1; //if something weird happens 
				break;
			}

			default: {
				abort();
			}
		}//end of switch
		free(string_status);

	}//end of while loop

	for (i = 0; i < argc;i++)
		pid_done[i] = 0;
	pid_total = 0;
	j = 0;
	//if wait was declared then loop through all child processes polling them to see if they finished
	//keep polling or can set a limit 
	if (wait_flag) {
		exit_status = 0;
		while (pid_total != k ) {//&& j < 100000000) {//will poll subprocesses in progress for a certain number of times
			for (i = 0; i < k;i++) {
				if (pid_done[i] != 1) {
					if (waitpid(pid[i], &status, WNOHANG) != 0) {
						pid_done[i] = 1;//mark that the child returned
						pid_total++;//increment the number of childs returned
						temp = WEXITSTATUS(status);//see if the exit status is the highest yet
						if (temp > exit_status)
							exit_status = temp;
						fprintf(stdout,"%d %s", temp, command_stats[i]);//print exit status and command
					}
				}	
			}
			// j++;
		}
	}
	// if (j == 100000000)
	// 	fprintf(stderr, "A subprocess never returned, probably got stuck in an infinite loop\n");

	fprintf(stdout, "EXIT STATUS: %d\n", exit_status);
	exit(exit_status);
}
