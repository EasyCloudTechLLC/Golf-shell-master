#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "sushi.h"

int sushi_exit = 0;

static void refuse_to_die(int sig)
{
  fputs("Type exit to exit the shell\n", stderr);
}

static void prevent_interruption() {
  struct sigaction sa = {.sa_handler = refuse_to_die, .sa_flags = 0};
  if (sigaction(SIGINT, &sa, NULL)) {
    // Cannot be fixed, anyway.
    perror("sigaction");
  }
}

int main(int argc, char *argv[]) {
  prevent_interruption();
  lookup_table_setup();

  // Initialize the SHELL variable
  setenv("SHELL", argv[0], 1);

  if (sushi_read_config("sushi.conf", 1))
    return EXIT_FAILURE;

  // Execute the command line parameters
  for (int i = 1; i < argc; i++) {
    if (sushi_read_config(argv[i], 0))
      return EXIT_FAILURE;
  }

  while (!sushi_exit) {
    // Has the prompt changed?
    char *prompt = strlen(sushi_safe_getenv("PS1"))
      ? sushi_safe_getenv("PS1")
      : SUSHI_DEFAULT_PROMPT;  
    printf("%s", prompt);

    char *line;  
    if ((line = sushi_read_line(stdin)) && !sushi_parse_command(line))
      sushi_store(line);
  }
  
  return EXIT_SUCCESS;
}
