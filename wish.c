#include "wish_utils.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_SIZE 1024

// char *mypath[100] = {"bli", "bla", "/bin/", ""};
char *mypath[100] = {"/bin/", ""};
char CURRENT_PATH[MAX_SIZE];
char ERROR_MESSAGE[30] = "An error has occurred\n";

int redirectoutput(char *s, char *aux) {
  char *aux2 = strtok_r(s, " ", &s);
  // verificamos si hay mas argumentos despues de 'ls >'
  if (aux2 == NULL) {
    print_error_msg(ERROR_MESSAGE);
    return 1;
  }
  // verificamos si hay mas argumentos despues del archivo
  if (strtok_r(s, " ", &s) != NULL) {
    print_error_msg(ERROR_MESSAGE);
    return 1;
  }
  strcat(aux, aux2);
  return 0;
}

int main(int argc, char *argv[]) {
  char str[MAX_SIZE];
  char *command_string;
  char *s;
  int fd;

  if (argc > 2) {
    print_error_msg(ERROR_MESSAGE);
    exit(1);
  }

  getcwd(CURRENT_PATH, sizeof(CURRENT_PATH));

  FILE *fp;
  if (argc == 2) {
    fp = fopen(argv[1], "r");
    if (fp == NULL) {
      print_error_msg(ERROR_MESSAGE);
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
    trim(s);
    command_string = strtok_r(s, " ", &s);
    trim(s);

    if (strcmp(command_string, "exit") == 0) {
      char *aux = strtok_r(s, " ", &s);
      if (aux != NULL) {
        print_error_msg(ERROR_MESSAGE);
        continue;
      }
      execute_exit(0);
    } else if (strcmp(command_string, "cd") == 0) {
      if (strlen(s) == 0) {
        print_error_msg(ERROR_MESSAGE);
        continue;
      }
      char *aux = strtok_r(s, " ", &s);
      if (strtok_r(s, " ", &s) != NULL) {
        print_error_msg(ERROR_MESSAGE);
        continue;
      }
      execute_cd(aux);
      getcwd(CURRENT_PATH, sizeof(CURRENT_PATH));
    } else if (strcmp(command_string, "path") == 0) {
      execute_path(mypath, s);
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
              // verificamos si el argumento despues de 'ls' es '>'
              if (strcmp(aux2, ">") == 0) {
                strcat(aux, ">");
                if (redirectoutput(s, aux)) {
                  continue;
                }
              } else {
                // verificamos si el directorio es absoluto
                if (aux2[0] == '.') {
                  aux2++;
                }
                if (aux2[0] == '/') {
                  aux2++;
                }
                strcat(aux, CURRENT_PATH);
                strcat(aux, "/");
                strcat(aux, aux2);

                // verificamos si existe el directorio
                if (access(aux, F_OK) == -1) {
                  // print_error_msg();
                  char *err_msg =
                      "ls: cannot access '/no/such/file': No such file "
                      "or directory\n\0";
                  print_error_msg(err_msg);
                  continue;
                }

                // verificamos si hay mas argumentos despues de 'ls <dir>'
                aux2 = strtok_r(s, " ", &s);
                if (aux2 != NULL) {
                  if (strcmp(aux2, ">") != 0) {
                    print_error_msg(ERROR_MESSAGE);
                    continue;
                  }
                  strcat(aux, ">");
                  if (redirectoutput(s, aux)) {
                    continue;
                  }
                }
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
        print_error_msg(ERROR_MESSAGE);
      }
    }
  } while (1);
}
