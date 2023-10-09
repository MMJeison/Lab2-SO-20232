#include "wish_utils.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_SIZE 1024

char *mypath[] = {"bli", "bla", "/bin/", ""};
char CURRENT_PATH[MAX_SIZE];
char ERROR_MESSAGE[30] = "An error has occurred\n";

int main(int argc, char *argv[]) {

  char str[MAX_SIZE];
  char *command_string;
  char *s;
  int fd;

  getcwd(CURRENT_PATH, sizeof(CURRENT_PATH));
  // printf("Current path: %s\n", CURRENT_PATH);

  if (argc > 2) {
    print_error_msg();
    exit(1);
  }

  FILE *fp;
  if (argc == 2) {
    fp = fopen(argv[1], "r");
    if (fp == NULL) {
      print_error_msg();
      exit(1);
    }
  }
  do {
    if (argc == 1) {
      printf("whish> ");
      fgets(str, MAX_SIZE, stdin);
    } else {
      fgets(str, MAX_SIZE, fp);
      if (feof(fp)) {
        fclose(fp);
        exit(0);
      }
    }
    s = str;
    while (*s != '\n') {
      ++s;
    }
    *s = '\0';
    s = str;
    command_string = strtok_r(s, " ", &s);
    trim(s);

    if (strcmp(command_string, "exit") == 0) {
      char *aux = strtok_r(s, " ", &s);
      if (aux != NULL) {
        print_error_msg();
        continue;
      }
      execute_exit(0);
    } else if (strcmp(command_string, "cd") == 0) {
      if (strlen(s) == 0) {
        print_error_msg();
        continue;
      }
      char *aux = strtok_r(s, " ", &s);
      if (strtok_r(s, " ", &s) != NULL) {
        print_error_msg();
        continue;
      }
      execute_cd(aux);
    } else if (strcmp(command_string, "path") == 0) {
      execute_path();
    } else {
      fd = -1;
      char **mp = mypath;
      char specificpath[MAX_SIZE];
      while ((strcmp(*mp, "") != 0) && fd != 0) {
        strcpy(specificpath, *mp++);
        strncat(specificpath, command_string, strlen(command_string));
        fd = access(specificpath, X_OK);
      }
      if (fd == 0) {
        int subprocess = fork();
        if (subprocess < 0) {
          printf("Error launching the subprocess");
        } else if (subprocess == 0) {
          // char aux[MAX_SIZE];
          // strcpy(aux, "./\0");
          char aux[MAX_SIZE];
          aux[0] = '\0';
          if (strcmp(command_string, "ls") == 0) {
            char *aux2 = strtok_r(s, " ", &s);
            if (aux2 != NULL) {
              if (aux2[0] == '.') {
                aux2++;
              }
              if (aux2[0] == '/') {
                aux2++;
              }
              strcat(aux, CURRENT_PATH);
              strcat(aux, "/");
              strcat(aux, aux2);
              aux2 = strtok_r(s, " ", &s);
              if (aux2 != NULL) {
                print_error_msg();
                continue;
              }
              // veamos si existe el directorio
              if (access(aux, F_OK) == -1) {
                // print_error_msg();
                char *msg = "ls: cannot access '/no/such/file': No such file "
                            "or directory\n\0";
                write(STDERR_FILENO, msg, strlen(msg));
                continue;
              }
            }
          }
          if (aux[0] == '\0') {
            strcat(aux, ".");
          }
          char *myargs[3];
          myargs[0] = strdup(specificpath);
          myargs[1] = strdup(aux);
          myargs[2] = NULL;

          execvp(myargs[0], myargs);
        } else {
          wait(NULL);
        }
      } else {
        // printf("Command not found: %s\n", str);
        print_error_msg();
      }
    }
  } while (1);
}
