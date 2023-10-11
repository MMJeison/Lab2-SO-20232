#include "wish_utils.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_SIZE 1024
#define HISTOY_SIZE 20

char *mypath[100] = {"/bin/", ""};
char CURRENT_PATH[MAX_SIZE];
char ERROR_MESSAGE[30] = "An error has occurred\n";
char PREVIOUS_COMMANDS[HISTOY_SIZE][MAX_SIZE];
int current_command = 0;

int notExistFileOrDirectory(char *path) {
  if (access(path, F_OK) == -1) {
    char err_msg[MAX_SIZE + 70] = "ls: cannot access '";
    strcat(err_msg, path);
    strcat(err_msg, "': No such file or directory\n");
    print_error_msg(err_msg);
    return 1;
  }
  return 0;
}

void build_path(char *path, char *aux) {
  if (path[0] != '/') {
    strcat(aux, CURRENT_PATH);
    if (path[0] == '.') {
      path++;
      if (path[0] != '/') {
        print_error_msg(ERROR_MESSAGE);
        return;
      }
    } else {
      strcat(aux, "/");
    }
  }
  strcat(aux, path);
}

int redirectoutputto(char *path) {
  // buscamos la ultima ocurrencia de '/'
  char *last = strrchr(path, '/');
  if (last == NULL) {
    print_error_msg(ERROR_MESSAGE);
    return 1;
  }
  char dir[MAX_SIZE]; // directorio donde se encuentra el archivo
  size_t len = strlen(path) - strlen(last);
  strncpy(dir, path, len);
  strcat(dir, "/\0");
  char *filename = last + 1; // nombre del archivo
  // verificamos que el nombre del archivo no sea vacio
  if (strlen(filename) == 0) {
    print_error_msg(ERROR_MESSAGE);
    return 1;
  }
  // verificamos si existe el directorio
  if (notExistFileOrDirectory(dir)) {
    return 1;
  }
  // verificamos si el archivo ya existe y si no lo creamos
  int fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
  if (fd < 0) {
    print_error_msg(ERROR_MESSAGE);
    return 1;
  }
  dup2(fd, STDOUT_FILENO);
  dup2(fd, STDERR_FILENO);
  close(fd);
  return 0;
}

int redirectoutput(char *s) {
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
  char aux3[MAX_SIZE];
  aux3[0] = '\0';
  build_path(aux2, aux3);

  return redirectoutputto(aux3);
}

int procces_comand_ls(char *entr, char *aux) {
  char *aux2 = strtok_r(entr, " ", &entr);
  if (aux2 != NULL) {
    // verificamos si hay mas de un argumento despues de 'ls'
    if (strtok_r(entr, " ", &entr) != NULL) {
      print_error_msg(ERROR_MESSAGE);
      return 1;
    }
    build_path(aux2, aux);
    return notExistFileOrDirectory(aux);
  } else {
    strcat(aux, ".");
  }
  return 0;
}

int main(int argc, char *argv[]) {
  char str[MAX_SIZE];
  char *s;
  char *command_string;
  int fd;
  char *s2;

  int subprocs[100];     // Almacena los PIDs de los procesos hijos
  int subproc_count = 0; // Contador de procesos hijos

  if (argc > 2) {
    print_error_msg(ERROR_MESSAGE);
    exit(1);
  }

  // obtenemos el directorio actual
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
    s2 = str;
    while (*s2 != '\n') {
      ++s2;
    }
    *s2 = '\0';
    s2 = str;
    trim(s2);
    if (strlen(s2) == 0) {
      continue;
    }
    while ((s = strtok_r(s2, "&", &s2)) != NULL) {
      trim(s);
      if (strlen(s) == 0) {
        continue;
      }
      command_string = strtok_r(s, " ", &s);
      if (strcmp(command_string, "exit") == 0) {
        char *aux = strtok_r(s, " ", &s);
        if (aux != NULL) {
          print_error_msg(ERROR_MESSAGE);
          // continue;*
          execute_exit(0);
        }
        execute_exit(0);
      } else if (strcmp(command_string, "cd") == 0) {
        if (strlen(s) == 0) {
          print_error_msg(ERROR_MESSAGE);
          // continue;*
          execute_exit(0);
        }
        char *aux = strtok_r(s, " ", &s);
        if (strtok_r(s, " ", &s) != NULL) {
          print_error_msg(ERROR_MESSAGE);
          // continue;*
          execute_exit(0);
        }
        execute_cd(aux);
        // actualizamos el directorio actual
        getcwd(CURRENT_PATH, sizeof(CURRENT_PATH));
      } else if (strcmp(command_string, "path") == 0) {
        execute_path(mypath, s);
      } else {
        int subproid = fork();
        if (subproid < 0) {
          printf("Error launching the subprocess");
        } else if (subproid == 0) {
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
              char *myargs[4];
              int nroArgs = 0;
              myargs[nroArgs++] = strdup(specificpath);
              char aux[MAX_SIZE];
              char *aux2;
              aux[0] = '\0';
              char *s3 = s;
              // verificamos si s3 contiene el caracter '>' y si lo contiene
              int flag = strchr(s3, '>') != NULL;
              if (flag) {
                if (*s3 == '>') {
                  ++s3;
                  *s = '\0';
                } else {
                  s = strtok_r(s3, ">", &s3);
                }
              }
              if (strcmp(command_string, "ls") == 0) {
                if (procces_comand_ls(s, aux)) {
                  // continue;*
                  execute_exit(0);
                }
                myargs[nroArgs++] = strdup(aux);
              } else if (strcmp(command_string, "cat") == 0) {
                // verificamos que solo haya un argumento despues de 'cat'
                aux2 = strtok_r(s, " ", &s);
                if (aux2 == NULL) {
                  print_error_msg(ERROR_MESSAGE);
                  // continue;*
                  execute_exit(0);
                }
                if (strtok_r(s, " ", &s) != NULL) {
                  print_error_msg(ERROR_MESSAGE);
                  // continue;*
                  execute_exit(0);
                }
                build_path(aux2, aux);
                myargs[nroArgs++] = strdup(aux);
              } else if (strcmp(command_string, "rm") == 0) {
                aux2 = strtok_r(s, " ", &s);
                if (aux2 == NULL) {
                  print_error_msg(ERROR_MESSAGE);
                  // continue;*
                  execute_exit(0);
                }
                myargs[nroArgs++] = strdup(aux2);
                // verificamos que haya un argumento despues de '-f' o '-r'
                aux2 = strtok_r(s, " ", &s);
                if (aux2 == NULL) {
                  print_error_msg(ERROR_MESSAGE);
                  // continue;*
                  execute_exit(0);
                }
                if (strtok_r(s, " ", &s) != NULL) {
                  print_error_msg(ERROR_MESSAGE);
                  // continue;*
                  execute_exit(0);
                }
                build_path(aux2, aux);
                myargs[nroArgs++] = strdup(aux);
              } else if (strcmp(command_string, "echo") == 0) {
                strcat(aux, s);
                myargs[nroArgs++] = strdup(aux);
              } else {
                strcat(aux, ".");
                myargs[nroArgs++] = strdup(aux);
              }

              if (flag) {
                if (redirectoutput(s3)) {
                  // continue;*
                  execute_exit(0);
                }
              }
              myargs[nroArgs++] = NULL;
              execvp(myargs[0], myargs);
            } else {
              // esperamos a que termine el proceso hijo y almacenamos su valor
              // de retorno
              int status;
              waitpid(subprocess, &status, 0);
              // salimos con el valor de retorno del proceso hijo
              execute_exit(WEXITSTATUS(status));
            }
          } else {
            print_error_msg(ERROR_MESSAGE);
          }
          execute_exit(0);
        } else {
          subprocs[subproc_count] = subproid; // PID del proceso hijo
          subproc_count++;
        }
      }
    }
    for (int i = 0; i < subproc_count; i++) {
      int status;
      waitpid(subprocs[i], &status, 0);
      // si el proceso hijo termino con un valor de retorno distinto de 0
      if (WEXITSTATUS(status) != 0) {
        // terminamos el proceso padre con el mismo valor de retorno
        execute_exit(WEXITSTATUS(status));
      }
    }
    subproc_count = 0; // Reseteamos el contador de procesos hijos
  } while (1);
}
