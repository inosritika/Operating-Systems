#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Maximun number of characters allowed
#define MAX_INPUT_LENGTH 2048

volatile int inf_loop = 1; // For running infinite loop until Ctrl+C is pressed
int got_eof = 0; // To take care if the program reaches End Of File

// Commands to implement
char *const cd_command = "cd";
char *const history_command = "history";

// Initial directory paths
char *current_dir = NULL;
char *printable_dir = NULL;
char *initial_dir = NULL;

struct {
  char *arr[2048]; // array to store the commands
  int oldest_pos;  // position of last element
} History;

void initialize_program(); // For initialising the variables and history
void sigint_handler(int sig_no);
char *input_string();
char *get_command(); // For getting commands from terminal
char **get_arguments(char *cmd); // For converting command string into arguments array
int is_integer(const char *str); // To check if secong argument in "history" is an integer or not
size_t get_array_size(char **array);
void print_history(int number_of_arguments); // For prinitng the history of last "n" commands
void update_current_directory();
void change_directory(char **args); // Converting the current directory to custom directory
char *format_directory_for_printing(const char *path);
bool is_pipe_command(const char *cmd); // For checking if the command is a valid pipe command or not
                              
void split_command(const char *command, char **before_pipe,char **after_pipe); // Splitting commands of pipe into two separate commands
void execute_piped_commands(char *command1, char *command2); // For implementing the pipe command using pipe system call
void run_command(char **args); // For running other linux commands
void cleanup_all(char **args); // For freeing dynamically allocated strings and the array of pointers
void cleanup();

int main() {
  initialize_program();
  char *input_prompt = "MTL458 >"; // Custom command prompt

  signal(SIGINT, sigint_handler);

  while (inf_loop) {
    printf("%s ", input_prompt); // Print the custom command prompt

    char *command1 = get_command();
    char *command = strdup(command1);

    if (strlen(command) == 0) {
      free(command);
      if (got_eof) { // Exit on stream EOF, else new prompt
        printf("\n");
        break;
      }
      continue; // Prompt the user for the next command
    }

    // Split command into arguments
    char **arguments = get_arguments(command);
    size_t args_size = get_array_size(arguments);

    if (!arguments) {
      // Some error occurred in processing & has been printed, put up a new
      // prompt
      continue; // Prompt the user for the next command
    }

    if (strcmp(arguments[0], history_command) == 0) {
      if (args_size == 1) {
        print_history(History.oldest_pos);
      } 
      else if (is_integer(arguments[1]) && args_size == 2) {
        int n = atoi(arguments[1]); // Convert the argument to an integer
        print_history(n);
      } 
      else if (arguments[2] && args_size >= 3) {
        fprintf(stderr, "history: Too many arguments\n");
      } 
      else {
        printf("history: Invalid command!\n");
      }
    } 
    
    else if (strcmp(arguments[0], cd_command) == 0) {
      change_directory(arguments);
    } 
    
    else if (args_size > 2 && is_pipe_command(command)) {
      char *before_pipe = NULL;
      char *after_pipe = NULL;
      split_command(command, &before_pipe, &after_pipe);
      execute_piped_commands(before_pipe, after_pipe);
      free(before_pipe);
      free(after_pipe);
    } 
    
    else {
      run_command(arguments); // need to implement
    }
    
    cleanup_all(arguments);

  }
  cleanup();
}

// Function to initialize program variables and structures
void initialize_program() {
  
  // Get and store initial directory paths
  current_dir = getcwd(NULL, 0);
  initial_dir = strdup(current_dir);
  printable_dir = format_directory_for_printing(current_dir);
  
  // Initialize history data structure
  History.oldest_pos = 0;
  for (int i = 0; i < 2048; i++) {
    History.arr[i] = NULL;
  }

}

void sigint_handler(int sig_no) {
  cleanup();
  printf("\n");
  exit(0);
}

// Function for checking the input string
char *input_string() {
  char ch;
  size_t length = 0;
  int size = MAX_INPUT_LENGTH;
  char *string_new = malloc(sizeof(*string_new) * size);

  if (!string_new) {
    perror("Memory allocation failed");
    fprintf(stderr, "Custom error message: Something went wrong in input!\n");
    return string_new; // Error will be generated
  }

  while (EOF != (ch = fgetc(stdin)) && ch != '\n') {
    string_new[length++] = ch;
    if (length == size) {
      printf(
          "OOPS!!...Invalid input!!...Only 2048 characters are allowed :() ");
      return NULL;
    }
  }
  if (ch == EOF) {
    got_eof = 1;
  }

  string_new[length++] = '\0';
  return string_new;
}

char *get_command() {
  char *cmd = input_string();
  if (!cmd) {
    exit(1); // Error occcured
  }

  free(History.arr[History.oldest_pos]);
  History.arr[History.oldest_pos++] = strdup(cmd);
  return cmd;
}

char **get_arguments(char *cmd) {
  char **args = malloc(10 * sizeof(char *));
  memset(args, '\0', 10 * sizeof(char *));
  int args_capacity = 10;
  int args_size = 0;

  int cmd_len = strlen(cmd);
  int i = 0;

  while (1) { // Iterate over arguments
    if (args_size == args_capacity) {
      args = realloc(args, sizeof(*args) * (args_capacity += 10));
      memset(args + args_capacity - 10, '\0', 10 * sizeof(char *));
      if (!args) { // realloc failed
        perror("get_args: realloc");
        exit(1);
      }
    }
    // Now args has space for a new argument

    while (i < cmd_len && cmd[i] == ' ') {
      i++;
    }
    if (i == cmd_len) {
      args[args_size++] = NULL;
      return args;
    }

    args[args_size++] = malloc((cmd_len + 1) * sizeof(char));
    args[args_size - 1][0] = '\0';

    int begin = i;
    int escape_space = 0; // escape_space <=> ' ' is part of argument
    while (i <= cmd_len) {
      if (i == cmd_len) {
        strcat(args[args_size - 1], cmd + begin);
        break; // Next argument iteration will add NULL arg and return
      } 
      else if (cmd[i] == ' ' && !escape_space) {
        // Argument is cmd[begin..i-1]
        cmd[i] = '\0';
        strcat(args[args_size - 1], cmd + begin);
        cmd[i] = ' ';
        break;
      } 
      else if (cmd[i] == '"') {
        escape_space = !escape_space;
        cmd[i] = '\0';
        strcat(args[args_size - 1], cmd + begin);
        cmd[i] = '"';
        begin = i + 1;
        i++;
      } 
      else {
        i++;
      }
    }

    if (i == cmd_len && escape_space) { // => Unclosed double quotes
      fprintf(stderr, "Unclosed double quotes\n");
      return NULL;
    }
  }
}

int is_integer(const char *str) {
  char *endptr;
  errno = 0; // Initialize errno to zero before the call

  // Attempt to convert the string to a long integer
  long int converted = strtol(str, &endptr, 10);

  // Check if conversion was successful and if the entire string was consumed
  if (errno == 0 && *endptr == '\0') {
    return 1; // String is a valid integer
  } 
  else {
    return 0; // String is not a valid integer
  }
}

size_t get_array_size(char **array) {
  size_t size = 0;
  while (array[size] != NULL) {
    size++;
  }
  return size;
}

void print_history(int number_of_arguments) {
  if (number_of_arguments > History.oldest_pos) {
    printf("Error: Commands implemented are less than %d\n",
           number_of_arguments);
    return;
  }
  for (int i = History.oldest_pos - number_of_arguments; i < History.oldest_pos;
       i++) {
    printf("%d %s\n", i, History.arr[i]);
  }
}

// For updating the directory
void update_current_directory() {
  free(current_dir);
  free(printable_dir);
  current_dir = getcwd(NULL, 0);
  printable_dir = format_directory_for_printing(current_dir);
}

void change_directory(char **args) {
  if (!args[1]) {             // No argument to cd goes to starting directory
    if (chdir(initial_dir)) { // "0" indicates success & non-zero indicates
                              // failure
      perror("cd");
      return;
    }
    update_current_directory();
    return;
  }
  if (args[2]) {
    fprintf(stderr, "cd: Too many arguments\n");
    return;
  }
  char *dir;
  dir = strdup(args[1]);

  // check again
  if (chdir(dir)) { // for valid path
    printf("%s\n", dir);
    perror("cd");
    free(dir);
    return;
  }

  free(dir);
  update_current_directory();
}

// Function to format a directory path for printing
char *format_directory_for_printing(const char *path) {
  char *formatted_path = strdup(path);
  int initial_path_length = strlen(initial_dir);
  int matches_initial = strncmp(initial_dir, path, initial_path_length) == 0;
  if (matches_initial) {
    int formatted_length = strlen(path) - initial_path_length + 1;
    free(formatted_path);
    formatted_path = (char *)malloc(sizeof(char) * formatted_length);
    formatted_path[0] = '\0';
    strcat(formatted_path, " ");
    strcat(formatted_path, path + initial_path_length);
  }
  return formatted_path;
}

bool is_pipe_command(const char *cmd) {
  // Search for the pipe symbol in the command string
  bool ans = false;
  if (memchr(cmd, '|', strlen(cmd)) != NULL) {
    ans = true;
  };
  return ans;
}

void split_command(const char *command, char **before_pipe, char **after_pipe) {
  char command_copy[strlen(command) + 1];
  strcpy(command_copy, command);

  char *before = strtok(command_copy, "|");
  char *after = strtok(NULL, "|");

  // Trim leading and trailing spaces from parts
  char *trimmed_before = before;
  while (*trimmed_before == ' ') {
    trimmed_before++;
  }
  char *last_char = trimmed_before + strlen(trimmed_before) - 1;
  while (last_char >= trimmed_before && *last_char == ' ') {
    *last_char = '\0';
    last_char--;
  }

  char *trimmed_after = after;
  while (*trimmed_after == ' ') {
    trimmed_after++;
  }
  last_char = trimmed_after + strlen(trimmed_after) - 1;
  while (last_char >= trimmed_after && *last_char == ' ') {
    *last_char = '\0';
    last_char--;
  }
  *before_pipe = strdup(trimmed_before);
  *after_pipe = strdup(trimmed_after);
}

void execute_piped_commands(char *command1, char *command2) {
  int pipefd[2];
  pid_t child1, child2;

  char **arguments1 = get_arguments(command1);
  char **arguments2 = get_arguments(command2);

  // Create a pipe
  if (pipe(pipefd) == -1) {
    perror("pipe");
    exit(EXIT_FAILURE);
  }

  // Fork the first child process
  child1 = fork();
  if (child1 == -1) {
    perror("fork");
    exit(EXIT_FAILURE);
  }

  if (child1 == 0) { // Child process 1 (command1)
    
    close(pipefd[0]); // Close the read end of the pipe

    // Redirect stdout to the write end of the pipe
    dup2(pipefd[1], STDOUT_FILENO);

    // Close the unused end of the pipe
    close(pipefd[1]);

    // Execute the first command
    run_command(arguments1);
    exit(EXIT_FAILURE);
  }

  // Fork the second child process
  child2 = fork();
  if (child2 == -1) {
    perror("fork");
    exit(EXIT_FAILURE);
  }

  if (child2 == 0) { // Child process 2 (command2)
    
    close(pipefd[1]); // Close the write end of the pipe

    // Redirect stdin to the read end of the pipe
    dup2(pipefd[0], STDIN_FILENO);

    // Close the unused end of the pipe
    close(pipefd[0]);

    // Execute the second command
    run_command(arguments2);
    exit(EXIT_FAILURE);
  }

  // Close both ends of the pipe in the parent process
  close(pipefd[0]);
  close(pipefd[1]);

  // Wait for both child processes to complete
  waitpid(child1, NULL, 0);
  waitpid(child2, NULL, 0);
}

// As command is one of the Linux built-in commands, exec the corresponding
// Linux executable
void run_command(char **args) {
  int pid = fork();
  if (pid == -1) {
    perror("fork");
  } else if (pid == 0) {
    execvp(args[0], args);
    // If execvp returned, it means an error must have occurred
    perror("exec");
    exit(1); // Exit out of child process
  } else {
    if (wait(NULL) == -1) {
      perror("wait");
    }
  }
}

void cleanup() {
  free(current_dir);
  free(printable_dir);
  free(initial_dir);
  for (int i = 0; i < 2048; i++) {
    free(History.arr[i]);
  }
}

// Function for freeing dynamically allocated strings and the array of pointers
void cleanup_all(char **args) {
  for (int i = 0; args[i] != NULL; i++) {
    free(args[i]);
  }
  free(args);
}
