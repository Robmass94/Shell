/* 
Course: COP 5570
Assignment: Project 1
Author: Robert Massicotte
*/

#include <fcntl.h> /* for open */
#include <signal.h> /* for signals */
#include <stdio.h> /* for I/O functions */
#include <stdlib.h> /*for exit macros */
#include <string.h> /* for string functions */
#include <sys/types.h> /* for pid_t type */
#include <sys/wait.h> /* for waitpid function */
#include <unistd.h> /* for UNIX functions */

#define BUFF_SIZE 256
#define MAX_INPUT 100

void handleCommand(char*[], int, int);
void cdCommand(char*);
void pwdCommand();
void setCommand(char*);
void locateMylsCommand(char*[], int, int);
void executeCommand(char*[], int);
void pipedCommand(char*[], int);
void redirectInput(char*[], int);
void redirectOutput(char*[], int);
void handleBackgroundProcess(char*[], int);
int indexOfArg(char*[], char*, int);

int main()
{
	char input[MAX_INPUT];
	char *cmd_argv[MAX_INPUT];
	int cmd_argc;
	char *token;

	/* kernel will reap children spawned by background processes */
	signal(SIGCHLD, SIG_IGN);

	printf("$ ");
	while (fgets(input, MAX_INPUT, stdin) != NULL) {
		/* reset arg counter */
		cmd_argc = 0;
		/* strip newline */
		input[strlen(input) - 1] = '\0';

		/* split the input on spaces and add each segment to the command
		array */
		token = strtok(input, " ");
		while (token != NULL) {
			cmd_argv[cmd_argc++] = token;
			token = strtok(NULL, " ");
		}

		if (cmd_argc > 0) {
			if (strcmp(cmd_argv[0], "exit") == 0) {
				break;
			} else {
				/* NULL terminate arguments (needed for exec functions) */
				cmd_argv[cmd_argc] = NULL;
				handleCommand(cmd_argv, cmd_argc, 0);
			}
		}

		printf("$ ");
	}
	return EXIT_SUCCESS;
}

void handleCommand(char *cmd_argv[], int cmd_argc, int background)
{

	if (indexOfArg(cmd_argv, "&", cmd_argc) >= 0) {
		handleBackgroundProcess(cmd_argv, cmd_argc);
	} else if (indexOfArg(cmd_argv, "|", cmd_argc) >= 0) {
		pipedCommand(cmd_argv, cmd_argc);
	} else if (indexOfArg(cmd_argv, "<", cmd_argc) >= 0) {
		redirectInput(cmd_argv, cmd_argc);
	} else if (indexOfArg(cmd_argv, ">", cmd_argc) >= 0) {
		redirectOutput(cmd_argv, cmd_argc);
	} else if (strcmp(cmd_argv[0], "cd") == 0) {
		cdCommand(cmd_argv[1]);
	} else if (strcmp(cmd_argv[0], "pwd") == 0) {
		pwdCommand();
	} else if (strcmp(cmd_argv[0], "set") == 0) {
		setCommand(cmd_argv[1]);
	} else if (strcmp(cmd_argv[0], "myls") == 0) {
		locateMylsCommand(cmd_argv, cmd_argc, background);
	} else {
		executeCommand(cmd_argv, background);
	}
}

void cdCommand(char *dir)
{
	if (chdir(dir) < 0) {
		fprintf(stderr, "ERROR: Failed to change directory.\n");
	}
}

void pwdCommand()
{
	/* print the absolute path of the current working directory */
	char current_dir[BUFF_SIZE];
	getcwd(current_dir, BUFF_SIZE);
	printf("%s\n", current_dir);
}

void setCommand(char *env)
{
	char *name = strtok(env, "=");
	char *value = strtok(NULL, "=");
	
	if (name != NULL) {
		/* set the environment variable; overwrite it if it already exists */
		setenv(name, value, 1);
	} else {
		fprintf(stderr, "ERROR: Malformed set expression.\n");
	}
}

void locateMylsCommand(char *argv[], int argc, int background)
{
	char test_path[BUFF_SIZE];
	char *token;
	char *my_path = getenv("MYPATH");
	char *myls_argv[argc + 1];
	
	if (my_path == NULL) {
		fprintf(stderr, "ERROR: MYPATH environment variable not set.\n");
		return;
	}

	if (argc > 2) {
		fprintf(stderr, "ERROR: myls: Too many arguments.\n");
		return;
	}

	/* split MYPATH on the colon character, to locate the myls program */
	token = strtok(my_path, ":");
	while (token != NULL) {
		sprintf(test_path, "%s/%s", token, "myls");
		if (access(test_path, F_OK) == 0) {
			/* found location of myls program */
			/*printf("Location of myls: %s\n", test_path);*/
			break;
		}
		token = strtok(NULL, ":");
	}

	if (token != NULL) {
		myls_argv[0] = test_path;
		if (argc > 1) {
			myls_argv[1] = argv[1];
			myls_argv[2] = NULL;
		} else {
			myls_argv[1] = NULL;
		}
		executeCommand(myls_argv, background);
	} else {
		fprintf(stderr, "ERROR: could not locate myls program\n");
	}
}

void executeCommand(char *argv[], int background)
{
	pid_t pid = fork();
	if (pid < 0) {
		/* process creation failed */
	} else if (pid == 0) {
		/* child process */
		if (background) {
			setpgid(pid, 0);
		}
		/* look for command in PATH and execute it, unless filename contains
		backslash (like with myls) */
		execvp(argv[0], argv);
		/* if execution is successful, this point will not be reached */
		fprintf(stderr, "ERROR: Failed to execute %s.\n", argv[0]);
		exit(-1);
	} else {
		/* parent process */
		waitpid(pid, NULL, background ? WNOHANG : 0);
	}
}

void pipedCommand(char *argv[], int argc)
{
	char *left_args[MAX_INPUT];
	int left_argc;
	int j;
	int prev_pipe_index = -1;
	int fds[2];
	int fd_in  = -1;
	int fd_out = -1;
	pid_t pid;

	/* loop through command array until a pipe is found */
	for (int i = 0; i < argc; ++i) {
		if (strcmp(argv[i], "|") == 0 || i == argc - 1) {
			/* found a pipe or last command, need to process left-hand side */
			left_argc = 0;
			if (prev_pipe_index > 0) {
				/* a pipe has been seen before, so left_args will be populated 
				with the args after the previous pipe */
				j = prev_pipe_index + 1;
			} else {
				/* first pipe seen, so left_args will be populated 
				from the beginning */
				j = 0;
			}

			if (i < argc - 1) {
				/* update prev_pipe_index for future use */
				prev_pipe_index = i;

				/* populate left_args from j up to (but excluding) pipe */
				for (; j < i; ++j) {
					left_args[left_argc++] = argv[j];
				}
				left_args[left_argc] = NULL;

				/* create the pipe */
				pipe(fds);
				/* set fd_out to write end of pipe */
				fd_out = fds[1];
			} else {
				/* populate left_args from j up to (and including) 
				last command */
				for(; j <= i; ++j) {
					left_args[left_argc++] = argv[j];
				}
				left_args[left_argc] = NULL;

				/* set fd_out to -1 since final input is being read */
				fd_out = -1;
			}

			pid = fork();
			if (pid < 0) {
				/* process creation failed */
			} else if (pid == 0) {
				/* child process will handle command execution */

				if (fd_in != -1 && fd_in != 0) {
					/* point stdin to read end of pipe if not first command */
					dup2(fd_in, STDIN_FILENO);
					close(fd_in);
				}

				if (fd_out != -1 && fd_out != 1) {
					/* point stdout to write end of pipe if not last command */
					dup2(fd_out, STDOUT_FILENO);
					close(fd_out);
				}

				/* process the command and then terminate */
				handleCommand(left_args, left_argc, 0);
				exit(EXIT_SUCCESS);
			} else {
				/* parent process */
				waitpid(pid, NULL, 0);
				close(fd_in);
				close(fd_out);
				fd_in = fds[0];
			}
		}
	}
}

void redirectInput(char *argv[], int argc)
{
	int redirect_index = indexOfArg(argv, "<", argc);
	if (argc < 3 || redirect_index != argc - 2) {
		fprintf(stderr, "ERROR: Malformed input redirection expression.\n");
		return;
	}

	char *left_args[redirect_index + 1];
	int left_argc = redirect_index;
	for (int i = 0; i < left_argc; ++i) {
		left_args[i] = argv[i];
	}
	left_args[left_argc] = NULL;

	/* duplicate stdin file descriptor */
	int stdin_dup = dup(STDIN_FILENO);
	/* open file for reading */
	int input_file = open(argv[argc - 1], O_RDONLY);
	if (input_file < 0) {
		fprintf(stderr, "Failed to open file %s.\n", argv[argc - 1]);
	} else {
		/* point stdin to input_file */
		dup2(input_file, STDIN_FILENO);
		handleCommand(left_args, left_argc, 0);
		close(input_file);
		/* restore stdin */
		dup2(stdin_dup, STDIN_FILENO);
	}
}

void redirectOutput(char *argv[], int argc)
{
	int redirect_index = indexOfArg(argv, ">", argc);
	if (argc < 3 || redirect_index != argc - 2) {
		fprintf(stderr, "ERROR: Malformed output redirection expression.\n");
		return;
	}

	char *left_args[redirect_index + 1];
	int left_argc = redirect_index;
	for (int i = 0; i < left_argc; ++i) {
		left_args[i] = argv[i];
	}
	left_args[left_argc] = NULL;

	/* duplicate stdout file descriptor */
	int stdout_dup = dup(STDOUT_FILENO);
	/* open file for writing; create it if doesn't exist and set read/write bits 
	for user and read bit for group and other; truncate it if it does already
	exist */
	int output_file = open(argv[argc - 1], O_RDWR | O_CREAT | O_TRUNC, 0644);
	if (output_file < 0) {
		fprintf(stderr, "ERROR: Failed to open or create file %s.\n", argv[argc - 1]);
	} else {
		/* point stdout to output_file */
		dup2(output_file, STDOUT_FILENO);
		handleCommand(left_args, left_argc, 0);
		close(output_file);
		/* restore stdout */
		dup2(stdout_dup, STDOUT_FILENO);
	}
}

void handleBackgroundProcess(char *argv[], int argc)
{
	int amp_index = indexOfArg(argv, "&", argc);
	if (amp_index != argc - 1) {
		/* ampersand should be at end of command */
		fprintf(stderr, "ERROR: Improperly placed ampersand.\n");
	} else {
		/* remove the ampersand from the command, pass back to handleCommand
		with background flag set */
		argv[argc - 1] = NULL;
		--argc;
		handleCommand(argv, argc, 1);
	}
}

int indexOfArg(char* argv[], char *arg, int argc) 
{
	/* returns the index of the arg in array args, or -1 if not present */
	for (int i = 0; i < argc; ++i) {
		if (strcmp(argv[i], arg) == 0) {
			return i;
		}
	}
	return -1;
}
