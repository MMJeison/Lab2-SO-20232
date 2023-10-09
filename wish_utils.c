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

void execute_path(char *mypath[100], char *paths) {
  int i = 0;
  char *path = strtok_r(paths, " ", &paths);
  while (path != NULL) {
    mypath[i] = strdup(path);
    // verificamos si el ultimo caracter es un '/'
    if (mypath[i][strlen(mypath[i]) - 1] != '/') {
      // si no lo es, le agregamos uno
      mypath[i] = strcat(mypath[i], "/");
    }
    path = strtok_r(NULL, " ", &paths);
    i++;
  }
  mypath[i] = "";
}

void print_error_msg(char *msg) { write(STDERR_FILENO, msg, strlen(msg)); }

void trim(char *str) {
  char *p = str;
  int l = strlen(p);

  while (isspace(p[l - 1]))
    p[--l] = 0;
  while (*p && isspace(*p))
    ++p, --l;
  memmove(str, p, l + 1);
}
