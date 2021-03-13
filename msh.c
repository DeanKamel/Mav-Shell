// The MIT License (MIT)
//
// Copyright (c) 2016, 2017, 2021 Trevor Bakker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.


#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

// We want to split our command line up into tokens
// so we need to define what delimits our tokens.
// In this case  white space
// will separate the tokens on our command line
#define WHITESPACE " \t\n"

#define MAX_COMMAND_SIZE 255 // The maximum command-line size

#define MAX_NUM_ARGUMENTS 11 // For 1 command and up to 10 arguments

int main()
{
	// Malloc the input string of the shell
	char *cmd_str = (char *)malloc(MAX_COMMAND_SIZE);

	pid_t listpids[15];
	int index = 0;

	char *history[15];
	int hist_index = 0;

	for (int a = 0; a < 15; a++)
	{
		// Malloc space for the history array at each index
		history[a] = (char *)malloc(255);
	}
	while (1)
	{
		// Print out the msh prompt
		printf("msh> ");

		// Read the command from the commandline.  The
		// maximum command that will be read is MAX_COMMAND_SIZE
		// This while command will wait here until the user
		// inputs something since fgets returns NULL when there
		// is no input
		while (!fgets(cmd_str, MAX_COMMAND_SIZE, stdin));

		/* Parse input */
		char *token[MAX_NUM_ARGUMENTS];

		int token_count = 0;

		// Pointer to point to the token
		// parsed by strsep
		char *argument_ptr;

		// Save a duplicate of the command string into working_str
		// because strsep will alter the pointer head.
		char *working_str = strdup(cmd_str);

		// we are going to move the working_str pointer to
		// keep track of its original value so we can deallocate
		// the correct amount at the end
		char *working_root = working_str;

		// Zero out the contents of history to make sure
		// there is no junk and leftover chars after running
		// it multiple times
		memset(history[hist_index], 0, 255);

		// Copy the string in working_str into history
		// with the max size as the limit and then increment the
		// index for the following run.
		strncpy(history[hist_index++], working_str, MAX_COMMAND_SIZE);
		if (hist_index > 14)
		{
			hist_index = 0;
		}

		if (working_str[0] == '!')
		{
			// Convert the number after the '!' from a char
			// to a number so that I could save the command
			// and use it for the index in strcpy
			int command = atoi(&working_str[1]);
			if (hist_index <= command)
			{
				printf("Command not in history...\n");
				continue;
			}
			// Will save the command from !n to working str
			// so that it gets processed normally after
			strcpy(working_str, history[command - 1]);
		}

		// Tokenize the input strings with whitespace used as the delimiter
		while (((argument_ptr = strsep(&working_str, WHITESPACE)) != NULL) &&
					 (token_count < MAX_NUM_ARGUMENTS))
		{
			token[token_count] = strndup(argument_ptr, MAX_COMMAND_SIZE);
			// If the first parsed token is 0 then we know it's NULL
			if (strlen(token[token_count]) == 0)
			{
				token[token_count] = NULL;
			}
			token_count++;
		}

		if (token[0] != NULL)
		{
			// If the first command is 'exit' or 'quit' then we could exit the program
			if ((strcmp(token[0], "exit") == 0) || (strcmp(token[0], "quit") == 0))
			{
				exit(0);
			}
			if ((strcmp(token[0], "history") == 0))
			{
				// If the command is history we list the saved commands in the
				// history array
				// If the variable z gets to 15 then it resets to 0
				// because 15 is the max
				for (int z = 0; z < hist_index; z++)
				{
					if (z > 14)
					{
						z = 0;
					}
					printf("%d: %s", z + 1, history[z]);
				}
				continue;
			}

			// Listpids command lists all the processed pids from the previous commands
			if ((strcmp(token[0], "listpids") == 0) || (strcmp(token[0], "showpids") == 0))
			{
				int pid_counter = 0;
				if (pid_counter > 14)
				{
					pid_counter = 0;
				}

				for (pid_counter = 0; pid_counter < index; pid_counter++)
				{
					printf("%d:  %d\n", pid_counter + 1, listpids[pid_counter]);
				}
				continue;
			}

			// chdir() changes the directory to the second parameter
			if ((strcmp(token[0], "cd") == 0))
			{
				if (token[1] != NULL)
				{
					chdir(token[1]);
				}
				else
				{
					printf("invalid directory\n");
				}
			}

			else
			{

				pid_t pid = fork();
				int status;
				int ret;

				// After forking a new process,
				// it checks if the pid == 0 then that means it's
				// a child process
				if (pid == 0 && (!strcmp(token[0], "cd") == 0))
				{
					//1 command + 10 command line parameters + null
					char *arguments[12];
					int i, x;

					// malloc space and store them in the arguments array
					// then copy the tokens into the arguments array
					for (i = 0; i < token_count - 1; i++)
					{
						arguments[i] = (char *)malloc(strlen(token[i]));
					}
					for (x = 0; x < token_count - 1; x++)
					{
						strncpy(arguments[x], token[x], strlen(token[x]));
					}

					// Null terminating the array
					arguments[token_count - 1] = NULL;

					// the execvp takes the first argument in the array that
					// you want to run followed by a pointer array with the
					// rest of the arguments
					ret = execvp(arguments[0], arguments);

					// execvp returns -1 if the command can not be run
					if (ret == -1)
					{
						printf("%s: Command not found.\n", arguments[0]);
						exit(0);
					}
				}

				// The process then return to the parent and
				// the wait function waits until the child process exits.
				else
				{
					listpids[index++] = pid;
					if (index > 14)
					{
						index = 0;
					}
					wait(&status);
				}
			}
		}

		// deallocate the memory
		free(working_root);
	}
	return 0;
}
