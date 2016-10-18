#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/wait.h>

#define BUFFSIZE 1024
#define TOKSIZE 16
#define DELIMETERS " \t\n\r"
#define YELLOW "\033[1;33m"
#define NC "\033[0m" // No Color
#define HISTSIZE 10


/**
*	structure for storing user commands
*/
typedef struct node
{
	char *command;
	struct node *next;
} node;

node *command_stack = NULL;


/**
*	function for inserting user command in the linked list
*/
void push_command(char *cmd)
{
	node *temp = (node *)malloc(sizeof(node));
	temp->command = (char *)malloc(sizeof(char)*(strlen(cmd)+1));
	strcpy(temp->command,cmd);
	temp->next = command_stack;
	command_stack = temp;
}


/**
*	function for deleting recent command in the linked list
*/
void pop_command()
{
	node *temp = command_stack;
	command_stack = command_stack->next;
	free(temp->command);
	free(temp);
}


/**
*	function for getting user input
*/
char *get_input()
{
	int buffer_size = BUFFSIZE;
	char *cmd=(char *)malloc(sizeof(char)*buffer_size);
	if(!cmd)
	{
		printf("Allocation error!!\n");
		exit(EXIT_FAILURE);
	}
	int c,idx=0;
	/**
	*	takes input character by character till EOF is encountered or '\n' is encountered.
	*/
	while(1)
	{
		c = getchar();
		if(c==EOF||c=='\n')
		{
			cmd[idx] = '\0';
			return cmd;
		}
		else
		{
			cmd[idx] = c;
			idx++;
		}
		if(idx>=buffer_size)
		{
			buffer_size += BUFFSIZE;
			cmd = realloc(cmd,buffer_size*sizeof(char));
			if(!cmd)
			{
				printf("Allocation error!!\n");
				exit(EXIT_FAILURE);
			}
		}
	}
}


/**
*	function to parse the user command and tokensize it by given delimeter.
*/
char **tokenize(char *cmd,char *delimeter)
{
	int token_size = TOKSIZE;
	char **tokens = (char **)malloc(sizeof(char *)*token_size);
	if(!tokens)
	{
		printf("Allocation error!!\n");
		exit(EXIT_FAILURE);
	}
	int idx=0;
	char *token;
	token = strtok(cmd,delimeter);
	while(token)
	{
		tokens[idx] = token;
		idx++;
		if(idx>=token_size)
		{
			token_size += TOKSIZE;
			tokens = realloc(tokens,token_size*sizeof(char *));
			if(!tokens)
			{
				printf("Allocation error!!\n");
				exit(EXIT_FAILURE);
			}
		}
		token = strtok(NULL,delimeter);
	}
	tokens[idx] = NULL;
	return tokens;
}

/**
*	prints the recent 10 commands entered by user.
*/
void history()
{
	int i = 0;
	node *temp = command_stack->next;
	while((i<HISTSIZE)&&temp)
	{
		printf("%d. %s\n",i+1,temp->command);
		temp = temp->next;
		i++;
	}
	return;
}


/**
*	function to free the allocated memory in heap.
*/
void free_commands()
{
	while(command_stack)
	{
		node *temp = command_stack;
		command_stack = command_stack->next;
		free(temp->command);
		free(temp);
	}
}


/**
*	function which executes the user command by identifying the first token of it.
*/
void execute_command(char *cmd,int in,int out)
{
	
	char **tokens = tokenize(cmd,DELIMETERS);
	/**
	*	All bulletin commands are run in bash itself and it figures this by various if else statement.
	*/
	if(tokens[0]==NULL)
		return;
	else if(!strcmp(tokens[0],"history"))
	{
		history();
	}
	else if(!strcmp(tokens[0],"!!"))
	{
		pop_command();
		char *new_cmd = (char *)malloc(sizeof(char)*(strlen(command_stack->command)+1));
		strcpy(new_cmd,command_stack->command);
		printf("%s\n",new_cmd);
		execute_command(new_cmd,in,out);
		free(new_cmd);
	}
	else if(!strcmp(tokens[0],"!"))
	{
		pop_command();
		if(tokens[1])
		{
			int cmd_no = atoi(tokens[1]);
			int i=1;
			node *temp = command_stack;
			while((i<cmd_no)&&temp)
			{
				temp = temp->next;
				i++;
			}
			if(!temp)
				printf("Not so many commands\n");
			else if(cmd_no==0)
				printf("Enter number greater than zero\n");
			else
			{
				char *new_cmd = (char *)malloc(sizeof(char)*(strlen(temp->command)+1));
				strcpy(new_cmd,temp->command);
				printf("%s\n",new_cmd);
				execute_command(new_cmd,in,out);
				free(new_cmd);
			}
		}
		else
		{
			printf("Expected argument to \"!\" \n");
		}
	}
	else if(!strcmp(tokens[0],"cd"))
	{
		if(tokens[1])
		{
			if (chdir(tokens[1]) != 0) 
			{
		    	perror("bash ");
		    }
		}
		else
		{
			printf("Expected argument to \"cd\" \n");
		}
	}
	else
	{	
		/**
		*	Here it runs all the commands which have their executable in /bin folder.
		*/
		int status;
		pid_t pid,wpid;
		pid = fork(); // creating child process.
		if(pid<0)
		{
			perror("bash ");
			exit(EXIT_FAILURE);
		}
		if(pid==0)
		{
			/**
			*	here child process runs and executes the command.
			*/
			if (in != 0)
	        {
	        	/**
				*	duplicating the file descriptor of stdin and 'in' for piping use.
				*/
          		dup2 (in, 0); 
          		close (in);
	        }

	    	if (out != 1)
	        {
	        	/**
				*	duplicating the file descriptor of stdout and 'out' for piping use.
				*/
          		dup2 (out, 1);
          		close (out);
	        }
			/**
			*	commands are run by the execvp function which have their executable in /bin directory.
			*/
			if(execvp(tokens[0],tokens)==-1)
			{
				perror("bash ");
			}
			exit(EXIT_SUCCESS);
		}
		else
		{
			/**
			*	here parent process runs and wait for the child process to terminate.
			*/
			do 
			{
	      		wpid = waitpid(pid, &status, WUNTRACED);
	    	} while (!WIFEXITED(status) && !WIFSIGNALED(status));
		}
			
	}
	/**
	*	freeing the memory used for storing tokens.
	*/
	free(tokens);
}

/**
*	function to check whether there is I/O redirection in the user command or not.
*	Return 1 if true else 0.
*/	
int is_io_redirection(char **tokens)
{
	int i;
	for(i=0;tokens[i];i++)
		if(!strcmp(tokens[i],"<")||!strcmp(tokens[i],">")||!strcmp(tokens[i],">>"))
			return i;
	return -1;
}


/**
*	utility function for copying strings.
*/
int copy(char *source,char *dest,int c)
{
	int i;
	for(i=0;source[i];i++)
		dest[c++] = source[i];
	return c;
}


/**
*	Function to handle the I/O redirection.
*/
void io_handler(char *cmd,int in,int ot)
{
	int len = strlen(cmd);
	char **tokens = tokenize(cmd,DELIMETERS);
	int flag = is_io_redirection(tokens);
	char inp[100],out[100];
	int i,in_op=-1,out_op=-1;


	if(flag!=-1)
	{
		/**
		*	have I/O redirection in user command.
		*/
		for(i=0;tokens[i];i++)
		{
			if(!strcmp(tokens[i],"<"))
			{
				in_op = 0;
				strcpy(inp,tokens[i+1]);
			}
			if(!strcmp(tokens[i],">"))
			{
				out_op = 1;
				strcpy(out,tokens[i+1]);
			}
			else if(!strcmp(tokens[i],">>"))
			{
				out_op = 2;
				strcpy(out,tokens[i+1]);
			}
		}

		/**
		*	opens file corresponding to I/O redirection.
		*	and sets corresponding file descriptors for duplicating.
		*/
		FILE *fp=NULL,*ft=NULL;
		if(in_op!=-1)
		{
			fp=fopen(inp,"r+");
		}
		if(out_op==1)
		{
			ft = fopen(out,"w+");
		}
		else if(out_op==2)
		{
			ft = fopen(out,"a+");
		}
		int file1,file2;
		if(fp)
		{
			file1 = fileno(fp);
			in=file1;
		}
		if(ft)
		{
			file2 = fileno(ft);
			ot=file2;
		}

		/**
		*	concating tokens to make user command.
		*/
		int c=0;
		char *new_cmd = (char *)malloc(sizeof(char)*(len+1));
		for(i=0;tokens[i];i++)
		{
			if(!strcmp(tokens[i],"<")||!strcmp(tokens[i],">")||!strcmp(tokens[i],">>"))
				i++;
			else
			{
				c = copy(tokens[i],new_cmd,c);
				new_cmd[c++] = ' ';
			}
		}
		new_cmd[c++] = '\0';
        execute_command(new_cmd,in,ot);
		if(fp)
			fclose(fp);
		if(ft)
			fclose(ft);
		free(new_cmd);
		free(tokens);
			
	}
	else
	{
		/**
		*	when I/O redirection is not there.
		*/
		int c=0;
		char *new_cmd = (char *)malloc(sizeof(char)*len);
		for(i=0;tokens[i];i++)
		{
			c = copy(tokens[i],new_cmd,c);
			new_cmd[c++] = ' ';
		}
		new_cmd[c++] = '\0';
        execute_command(new_cmd,in,ot);
		free(new_cmd);
		free(tokens);	
	}
}

/**
*	tokenize the user command by '|' and creates a pipe to run each command one by one.
*/
void pipe_handler(char *cmd)
{
	char **tokens;
	tokens = tokenize(cmd,"|");
	int n = 0,i;

	for(i=0;tokens[i];i++)
	{
		n++;
	}

	if(n==0)
		return;

	int in,f[2];
	pid_t pid;
	
	/**
	*	sets file descriptor for first use. First it should read from stdin.
	*/
	in=0;
	for(i=0;i<n-1;i++)
	{
		/**
		*	creating pipe and passing file descriptor for output as f[1].
		*/
		pipe(f);
		io_handler(tokens[i],in,f[1]);
		close(f[1]);
		/**
		*	setting input file descriptor for next iteration.
		*/
		in = f[0];
	}
	/**
	*	handling the last command.
	*/
    io_handler(tokens[i],in,1);
    free(tokens);
}

/**
*	function which prompts user for command and call appropriate functions.
*/
void bash()
{
	int status;
	char *cmd;
	while(1)
	{	
		printf("%s$%s ",YELLOW,NC);		
		cmd = get_input();
		if(!strcmp(cmd,"exit"))
		{
			free(cmd);
			free_commands();
			return;
		}
		push_command(cmd);
		pipe_handler(cmd);
	}
	free(cmd);
	free_commands();
}


int main()
{
	bash();
	return 0;
}