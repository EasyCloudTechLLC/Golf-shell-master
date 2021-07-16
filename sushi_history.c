#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sushi.h"

static char *history[SUSHI_HISTORY_LENGTH] = {NULL};

void sushi_store(char *line) {
  // Avoid memory leaks
  if (history[SUSHI_HISTORY_LENGTH - 1])
    free(history[SUSHI_HISTORY_LENGTH - 1]);

  // Shift the history
  for (int i = SUSHI_HISTORY_LENGTH - 1; i > 0; i--)
    history[i] = history[i - 1];
  
  // Add the most recent line to history
  history[0] = line;
}

void sushi_show_history() {
  for (int i = 0; i < SUSHI_HISTORY_LENGTH && history[i]; i++)
    printf("% 5d  %s\n", i + 1, history[i]); // Right justified to 5 places
}
