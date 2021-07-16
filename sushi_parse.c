#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
// DZ: for open()
       #include <sys/types.h>
       #include <sys/stat.h>
       #include <fcntl.h>
       #include <limits.h>
#include "sushi.h"
#include "sushi_yyparser.tab.h"

static char lookup_table[128] = {'\0'};

// The function storesd the special characters in the lookup table
void lookup_table_setup() {
  static const char special[][2] = {
    {'a', '\a'}, {'b', '\b'}, {'f', '\f'}, {'n', '\n'}, {'r', '\r'},
    {'t', '\t'}, {'v', '\v'}, {'\\', '\\'}, {'\'', '\''}, {'\"', '\"'},
    {'?', '\?'}, {'0', '\0'},
  };
  for (int i = 0; special[i][0]; i++)
    lookup_table[(size_t)special[i][0]] = special[i][1];
}

// Decode special characters
char *sushi_unquote(char *s) {
  size_t j = 0;
  for (size_t i = 0; i < strlen(s); i++, j++) {
    // Checks for escape sequences
    char next = lookup_table[(size_t)s[i + 1]];
    s[j] = (s[i] == '\\' && next) ? (i++, next) : s[i];
  }
  s[j] = '\0'; // Null-terminate the string

  return s;
}

// Function skeletons for HW3
void free_memory(prog_t *exe) {
  for (int i = 0; i < exe->args.size; i++)
    if (exe->args.args[i]) free(exe->args.args[i]);
  free(exe->args.args);
  if (exe->redirection.in) free(exe->redirection.in);
  if (exe->redirection.out1) free(exe->redirection.out1);
  if (exe->redirection.out2) free(exe->redirection.out2);
  if(exe->prev) free_memory(exe->prev);
  free(exe);
}

void sushi_assign(char *name, char *value) {
  setenv(name, value, 1);
  //  free(name);
  //  free(value);
}

char *sushi_safe_getenv(char *name) {
  char *var = getenv(name);
  return var ? var : "";
}

/*------------------------------------------------------------------
 * You can use these "convenience" functions as building blocks for
 * HW5 instead of your code, if you want. You may change them, too.
 *------------------------------------------------------------------*/

// Find the number of programs on the command line
static size_t cmd_length(prog_t *exe) {
  int count = 0;
  while(exe->prev) {
    exe = exe->prev;
    count++;
  }
  return count;
}

// Wait for the process pid to terminate; once it does, set the
// environmental variable "_" to the exit code of the process.
static int wait_and_setenv(pid_t pid) {
  int status;
  if (-1 == waitpid(pid, &status, 0)) {
    perror("waitpid");
    status = 1; // Something bad happened
  }
  char retval[16]; // Just to be safe
  sprintf(retval, "%d", status);
  if(-1 == setenv("_", retval, 1)) {
    perror("_");
    return 1;
  } else
    return 0;
}

// Execute the program defined in "exe"
static void start(prog_t *exe) {
  arglist_t args = exe->args;
  args.args = super_realloc(args.args, sizeof(char*) * (args.size + 1));
  args.args[args.size] = (char*)NULL;
  execvp(args.args[0], args.args);
  perror(args.args[0]);
}

// "Rename" file descriptor "old" to "new," if necessary. After the
// execution of this function a program that "believes" that it uses
// the "old" descriptor (e.g., stdout #1 for output) will be actually
// using the "new" descriprot (e.g., an outgoinf pipe).  This
// functions terminates the process of error and should not be used in
// the parent, only in a child.

static void dup_me (int new, int old) {
  if (new != old && -1 == dup2(new, old)) {
    perror("dup2");
    exit(1);
  }
}


// Former spawn().
int sushi_spawn(prog_t *exe, int bgmode) {
  int pipe_length = 0, max_pipe = cmd_length(exe);
  pid_t pid[max_pipe];

  int old_stdout = STDOUT_FILENO;
  

  // For each program in the pipeline
  for(prog_t *prog = exe; prog; prog = prog->prev) {
    int pipefd[2] = {STDIN_FILENO, old_stdout};
    if (prog->prev && -1 == pipe(pipefd)) {
      perror("pipe");
      return 1;
    }
  
    switch(pid[pipe_length] = fork()) {

    case -1: // Error
      perror(prog->args.args[0]);
      return 1;
    case 0: // Child
    if (exe->redirection.in != NULL)
    {
      // DZ: open() must be declared
      // DZ: input undeclared
        int fd0 = open(exe->redirection.in/*input*/, O_RDONLY);
	// DZ: Did it open?
        dup_me(fd0, STDIN_FILENO);
        
    }
    if (exe->redirection.out1 != NULL)
    {
     // DZ: output undeclared
      int fd1 = open(exe->redirection.out1/*output*/ , O_WRONLY) ;
	// DZ: Did it open?
        dup_me(fd1, STDOUT_FILENO);
        
    }
    if (exe->redirection.out2 != NULL)
    {
     // DZ: output undeclared
      int fd2 = open(exe->redirection.out2/*output*/ , O_APPEND) ;
	// DZ: Did it open?
        dup_me(fd2, STDOUT_FILENO);
        //close(fd1);
    }
      dup_me(pipefd[0], STDIN_FILENO);
      dup_me(old_stdout, STDOUT_FILENO);
      if(pipefd[1] != STDOUT_FILENO)
	close(pipefd[1]);
      start(prog);
      exit(EXIT_FAILURE);
    default: // Parent
      if(pipefd[0] != STDIN_FILENO) close(pipefd[0]);
      if(old_stdout != STDOUT_FILENO) close(old_stdout);
      old_stdout = pipefd[1];
    }
    pipe_length++;
  }

  int status = 0;
  if (bgmode == 0) {
    for (int i = 0; i < pipe_length; i++)
      if(wait_and_setenv(pid[i]))
	status = 1;
  }

  if(old_stdout != STDOUT_FILENO) close(old_stdout);
  free_memory(exe);
  
  return status;
}

char *super_strdup(const char *s) {
  char *result = strdup(s);
  if (!result) abort();
  return result;
}

void *super_malloc(size_t size) {
  void *result = malloc(size);  
  if (!result) abort();
  return result;
}

void *super_realloc(void *ptr, size_t size) {
  void *result = realloc(ptr, size);  
  if (!result) abort();
  return result;
}

// Do not modify this function
void yyerror(const char* s) {
  fprintf(stderr, "Parse error: %s\n", s);
}

void __not_implemented__() {  
  fputs("This operation is not implemented yet\n", stderr);
}

/*
 * New skeleton functions
 */
void sushi_display_wd() {

  // DZ: PATH_MAX undeclared
  char cwd[PATH_MAX +1];
  if (getcwd(cwd, sizeof(cwd)) != NULL){
    puts(cwd);
  }
  else{
    // DZ: ????
    perror("perror(getcwd() error");
  }
}

void sushi_change_wd(char *new_wd) {
  
  // trying to change the cwd to new_wd
   if (chdir(new_wd) != 0) 
    perror(new_wd);
  free(new_wd);
}
