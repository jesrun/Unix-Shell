// Jessica Runandy
// April 10, 2020
// This program uses C as a shell interface that accepts user commands from the
// command line and executes the commands. The program supports input and output
// redirection and pipes, and it involves using system calls that can be executed on
// any Linux, UNIX, or macOS system.

#include <fcntl.h>      // O_RDWR, O_CREAT
#include <stdbool.h>    // bool
#include <stdio.h>      // printf, getline
#include <stdlib.h>     // calloc
#include <string.h>     // strcmp
#include <unistd.h>     // execvp
#include <sys/wait.h>   // wait

#define MAX_LINE 80 // The maximum length command

static int history_count = 0; // keeps track of the history count
char *input_file = NULL; // stores the input file name
char *output_file = NULL; // stores the output file name
char *commands_history[MAX_LINE / 2 + 1]; // keeps track of the history

// Splits the string into tokens and returns the number of tokens.
// Class code based on http://www.cplusplus.com/reference/cstring/strtok/
int tokenize(char *line, char **tokens) {
  char *pch;
  pch = strtok(line, " ");
  int num = 0;
  while (pch != NULL) {
    tokens[num] = pch;
    ++num;
    pch = strtok(NULL, " ");
  }
  return num;
}

// Reads the line from stdin and returns the number of tokens.
int readline(char **buffer) {
  size_t len;
  int number_of_chars = getline(buffer, &len, stdin);
  if (number_of_chars > 0) {
    // get rid of \n
    (*buffer)[number_of_chars - 1] = '\0';
  }
  return number_of_chars;
}

// Runs the commands and determines the output based on the redirection
// and if the parent needs to wait for the child
void runCommand(char *args[], bool waitForChild, bool inputRedirection,
                bool outputRedirection) {
  enum { READ, WRITE };
  int pipeFD[2];
  pid_t pid;

  // Creates the pipe
  if (pipe(pipeFD) < 0) {
    perror("Error in creating pipe");
    exit(EXIT_FAILURE);
  }

  // Forks
  if ((pid = fork()) < 0) {
    perror("Fork Error");
    exit(EXIT_FAILURE);
  }

  if (pid == 0) { // Child process being executed

    // Input redirection
    if (inputRedirection) {
      pipeFD[READ] = open(input_file, O_RDONLY);
      if (pipeFD[READ] < 0) {
        fprintf(stderr, "ERROR: CANNOT OPEN %s\n", input_file);
        exit(EXIT_FAILURE);
      }
      dup2(pipeFD[READ], STDIN_FILENO);
      close(pipeFD[READ]);
    }

    // Output redirected to the output file name
    if (outputRedirection) {
      // Create a new file or rewrite an existing one
      pipeFD[WRITE] = creat(output_file, 0644);
      if (pipeFD[WRITE] < 0) {
        fprintf(stderr, "ERROR: CANNOT OPEN %s\n", output_file);
        exit(EXIT_FAILURE);
      }
      dup2(pipeFD[WRITE], STDOUT_FILENO);
      close(pipeFD[WRITE]);
    }
    int success = execvp(args[0], args); // Executes the commands

    // Only gets here if execvp failed
    printf("execvp for %s failed with %d\n", *args, success);
    exit(EXIT_FAILURE);
  }
  if (waitForChild) {
    waitpid(pid, NULL, 0);
  }
}

// Update history count and save the previous history in commands_history
void updateHistory(int num_of_tokens, char *args[]) {
  history_count++;
  for (int i = 0; i < num_of_tokens; i++) {
    commands_history[i] = args[i];
  }
}

// Returns true if there is a redirection in the tokens. redirection_string
// checks the redirection (either < or >)
bool checkRedirection(char *args[], char redirection_string[]) {
  // Checks if there is a redirection
  bool redirection = false;
  int i = 0; // Keeps track of args index
  int j = 0; // To set the redirection_string and file name to NULL in args
  while (args[i] != NULL) {
    if (strcmp(args[i], redirection_string) == 0) {
      redirection = true;

      // Reads the file name
      if (args[i + 1] != NULL) {

        // If the redirection is <, assign the input_file to the args
        if (strcmp(redirection_string, "<") == 0) {
          input_file = args[i + 1];
        } else { // If the redirection is >, assign the output_file to the args
          output_file = args[i + 1];
        }
      }

      // Sets < and the file name to NULL (removing it so that there is no
      // error when args is passed in execvp)
      j = i;
      while (args[j] != NULL) {
        args[j] = NULL;
        j++;
      }
    }
    i++;
  }
  return redirection;
}

// Executes the previous command
void previousCommand(int previous_command_length, char *args[]) {
  // Echoes the previous command on the user's screen
  for (int i = 0; i < previous_command_length; i++) {
    printf("%s ", commands_history[i]);
  }
  printf("\n");

  // Assigns args to the previous command
  for (int i = 0; i < previous_command_length; i++) {
    args[i] = commands_history[i];
  }
}

int main(void) {
  char *args[MAX_LINE / 2 + 1]; // Command line arguments
  int should_run = 1; // Flag to determine when to exit program
  int previous_command_length = 0; // Number of tokens from previous command

  // Initialize the commands_history to NULL
  for (int i = 0; i <= MAX_LINE / 2 + 1; ++i) {
    commands_history[i] = NULL;
  }
  while (should_run) {
    printf("osh>");
    fflush(stdout);

    // Initialize all commands to NULL when continuing the while loop
    for (int i = 0; i <= MAX_LINE / 2 + 1; ++i) {
      args[i] = NULL;
    }

    // Tokenize needs a char* it can modify, so need malloc
    char *command = (char *) malloc(MAX_LINE * sizeof(char));
    int length = readline(&command);

    if (length <= 0) {
      break;
    }
    if (strcmp(command, "") == 0) {
      continue;
    }
    if (strcmp(command, "exit") == 0) {
      should_run = 0;
      break;
    }

    // Tokenizes the commands into separate indexes and strings in args
    int num_of_tokens = tokenize(command, args);

    // Sets all firstcmd to NULL based on the number of tokens
    char **firstcmd = (char **) malloc(MAX_LINE * sizeof(char *));
    for (int i = 0; i <= num_of_tokens; ++i) {
      // Initialize all to NULL
      firstcmd[i] = NULL;
    }

    // If the command is "!!", call the previous command
    if (num_of_tokens == 1 && strcmp(args[0], "!!") == 0) {
      // If a previous command exist, execute the previous command
      if (history_count > 0) {
        previousCommand(previous_command_length, args);
        num_of_tokens = previous_command_length;
      } else {
        printf("No commands in history");
        printf("\n");
        continue;
      }
    }

    // Updates the previous history
    updateHistory(num_of_tokens, args);
    previous_command_length = num_of_tokens;

    // True if there is an input redirection
    bool inputRedirection = checkRedirection(args, "<");

    // True if there is an output redirection
    bool outputRedirection = checkRedirection(args, ">");

    int i = 0; // index of the args array (all of the tokenized commands)
    int j = 0; // index of firstcmd (the copy of the tokenized commands)
    bool pipe_exists = false;
    bool semicolon_exists = false;
    bool ampersand_exists = false;

    // Loops through all of the tokens
    while (args[i] != NULL) {
      for (int m = 0; m <= num_of_tokens; ++m) {
        // Initialize firstcmd to NULL
        firstcmd[m] = NULL;
      }

      // Create a copy of the arguments up to first ";" or "&" or "|"
      while (args[i] != NULL && strcmp(args[i], ";") != 0 &&
             strcmp(args[i], "&") != 0 && strcmp(args[i], "|") != 0) {
        firstcmd[j] = args[i];
        ++i;
        ++j;
      }

      if (args[i] == NULL) {  // Case when reach end of tokens
        if (semicolon_exists || ampersand_exists || !pipe_exists) {
          runCommand(firstcmd, true, inputRedirection, outputRedirection);
        }
      } else {
        // Case "&" : parent does not wait for child
        if (strcmp(args[i], "&") == 0) {
          ampersand_exists = true;
          runCommand(firstcmd, false, inputRedirection, outputRedirection);
        } else if (strcmp(args[i], ";") == 0) { // Case ";" : parent waits for child
          semicolon_exists = true;
          runCommand(firstcmd, true, inputRedirection, outputRedirection);
        } else if (strcmp(args[i], "|") == 0) { // Case "|"
          pipe_exists = true;
          int index = i + 1; // Index to copy commands from args to secondcmd
          char *secondcmd[MAX_LINE / 2 + 1]; // Store commands on right side of pipe
          for (int k = 0; k <= MAX_LINE / 2 + 1; ++k) {
            secondcmd[k] = NULL;
          }
          int check_index = i;  // Check if there's NULL, ";", or "&" after the pipe

          // Checks if there is NULL, ";", or "&" after the pipe
          while (args[check_index] != NULL &&
                 strcmp(args[check_index], ";") != 0 &&
                 strcmp(args[check_index], "&") != 0) {
            check_index++;
          }

          // If NULL, copy every command on the right of pipe and before NULL
          if (args[check_index] == NULL) {
            for(int x = 0; x < num_of_tokens - i - 1; x++) {
              secondcmd[x] = args[index];
              index++;
            }
            i = index; // Updates the index so that args[i] == NULL
          } else {
            // Copies the commands on the right side of the pipe,
            // doesn't include ";" or "&"
            int x; // Stores the size - 1 and index of the secondcmd
            for(x = 0; x < check_index - i - 1; x++) {
              secondcmd[x] = args[index];
              index++;
            }
            i = i + x; // Updates i by adding i + x.
                       // x = size - 1 of the secondcmd
          }
          enum { READ, WRITE };
          pid_t pid;
          int pipeFD[2];

          if (pipe(pipeFD) < 0) {
            perror("Error in creating pipe");
            exit(EXIT_FAILURE);
          }
          pid = fork();
          if (pid == 0) { // Child
            close(pipeFD[READ]);
            dup2(pipeFD[WRITE], STDOUT_FILENO);

            int success = execvp(firstcmd[0], firstcmd);

            // only gets here if execvp failed
            printf("execvp for %s failed with %d\n", *firstcmd, success);
            exit(EXIT_FAILURE);
          } else { // Parent
            if ((pid = fork()) < 0) {
              perror("Fork Error");
              exit(EXIT_FAILURE);
            }
            if (pid == 0) { // Child
              close(pipeFD[WRITE]);
              dup2(pipeFD[READ], STDIN_FILENO);
              int success = execvp(secondcmd[0], secondcmd);

              // Only gets here if execvp failed
              printf("execvp for %s failed with %d\n", *secondcmd, success);
              exit(EXIT_FAILURE);
            }
            close(pipeFD[READ]);
            close(pipeFD[WRITE]);
            waitpid(pid, NULL, 0);
            waitpid(pid, NULL, 0);
          }
        }
        i++;
      }
      j = 0;
    }
  }
  printf("Exiting shell\n");
  return 0;
}
