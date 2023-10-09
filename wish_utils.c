#include "wish_utils.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void execute_exit(int value) { exit(value); }

void execute_cd(char *newpath) {
  char *path = strtok_r(newpath, " ", &newpath);
  chdir(path);
}

void execute_path() {}

void print_error_msg() {
  char error_message[30] = "An error has occurred\n";
  write(STDERR_FILENO, error_message, strlen(error_message));
}

void trim(char *str) {
  char *p = str;
  int l = strlen(p);

  while (isspace(p[l - 1]))
    p[--l] = 0;
  while (*p && isspace(*p))
    ++p, --l;
  memmove(str, p, l + 1);
}
