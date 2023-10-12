#include "wish_utils.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// iclimos librerias necesaria para deshabilitar el buffering de líneas, el modo
// canónico y el eco
#include <termios.h>
#include <unistd.h>

#define MAX_SIZE 1024
#define HISTOY_SIZE 50

typedef struct node_d node_d;
typedef struct list_d list_d;

struct node_d {
  char *command;
  struct node_d *next;
  struct node_d *prev;
};

struct list_d {
  struct node_d *head;
  struct node_d *tail;
};

node_d *create_node(char *command) {
  node_d *node = malloc(sizeof(node_d));
  node->command = strdup(command);
  node->next = NULL;
  node->prev = NULL;
  return node;
}

list_d *create_list() {
  list_d *list = malloc(sizeof(list_d));
  list->head = NULL;
  list->tail = NULL;
  return list;
}

node_d *prev_item(list_d *list, node_d *node) {
  if (node == NULL) {
    return NULL;
  }
  return node->prev;
}

node_d *next_item(list_d *list, node_d *node) {
  if (node == NULL) {
    return list->head;
  }
  if (node->next == NULL) {
    return node;
  }
  return node->next;
}

void destroy_list(list_d *list) {
  node_d *current = list->head;
  node_d *next;
  while (current != NULL) {
    next = current->next;
    free(current->command);
    free(current);
    current = next;
  }
  free(list);
}

void insert_first(list_d *list, char *command) {
  node_d *node = create_node(command);
  if (list->head == NULL) {
    list->head = node;
    list->tail = node;
  } else {
    if (strcmp(list->head->command, command) == 0) {
      return;
    }
    node->next = list->head;
    list->head->prev = node;
    list->head = node;
  }
}

void remove_first(list_d *list) {
  if (list->head == NULL) {
    return;
  }
  node_d *node = list->head;
  list->head = list->head->next;
  if (list->head == NULL) {
    list->tail = NULL;
  } else {
    list->head->prev = NULL;
  }
  free(node->command);
  free(node);
}

void insert_last(list_d *list, char *command) {
  node_d *node = create_node(command);
  if (list->head == NULL) {
    list->head = node;
    list->tail = node;
  } else {
    if (strcmp(list->tail->command, command) == 0) {
      return;
    }
    node->prev = list->tail;
    list->tail->next = node;
    list->tail = node;
  }
}

char *mypath[100] = {"/bin/", ""};
char CURRENT_PATH[MAX_SIZE];
char ERROR_MESSAGE[30] = "An error has occurred\n";

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

struct termios term;

void deshabilitar_buffering() {
  // deshabilitamos el buffering de líneas, el modo canónico y el eco
  tcgetattr(STDIN_FILENO, &term);
  term.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void enable_buffering() {
  // habilitamos el buffering de líneas, el modo canónico y el eco
  tcgetattr(STDIN_FILENO, &term);
  term.c_lflag |= ICANON | ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

int main(int argc, char *argv[]) {
  deshabilitar_buffering();
  // obtenemos el directorio actual
  getcwd(CURRENT_PATH, sizeof(CURRENT_PATH));
  // creamos la lista que almacena el historial de comandos
  list_d *history = create_list();
  char pathfile[MAX_SIZE];
  strcpy(pathfile, CURRENT_PATH);
  strcat(pathfile, "/commands-history.txt");
  // verificamos si existe el archivo
  FILE *fh;
  if (notExistFileOrDirectory(pathfile)) {
    // si no existe lo creamos
    int idf = open(pathfile, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    close(idf);
  } else {
    fh = fopen(pathfile, "r");
    if (fh != NULL) {
      char line[MAX_SIZE];
      while (fgets(line, MAX_SIZE, fh) != NULL) {
        char *ax = strchr(line, '\n');
        if (ax != NULL) {
          *ax = '\0';
        }
        insert_last(history, line);
      }
      fclose(fh);
    }
  }

  char str[MAX_SIZE];
  char straux[MAX_SIZE];
  char *s;
  char *command_string;
  int fd;
  char *s2;

  int subprocs[100];     // Almacena los PIDs de los procesos hijos
  int subproc_count = 0; // Contador de procesos hijos

  if (argc > 2) {
    print_error_msg(ERROR_MESSAGE);
    enable_buffering();
    exit(1);
  }

  FILE *fp;
  char commands[100][MAX_SIZE];
  int nroCom = 0;
  if (argc == 2) {
    fp = fopen(argv[1], "r");
    if (fp == NULL) {
      print_error_msg(ERROR_MESSAGE);
      enable_buffering();
      exit(1);
    } else {
      while (fgets(commands[nroCom], MAX_SIZE, fp) != NULL) {
        insert_last(history, commands[nroCom]);
        nroCom++;
      }
      strcpy(commands[nroCom], "exit");
      fclose(fp);
    }
  }
  int currCom = 0;
  do {
    if (argc == 1) {
      printf("\rwhish> ");
      int c = 0, ch;
      str[c] = '\0';
      straux[0] = '\0';
      // creamos un puntero al primer elemento de la lista de historial
      node_d *node = NULL;
      while ((ch = getchar()) != '\n' && ch != EOF) {
        // si se presiona la tecla de retroceso
        if (ch == 127) {
          node = NULL;
          if (c > 0) {
            printf("\b \b");
            --c;
          }
        } else if (ch == 27) {
          getchar();
          int aux = getchar();
          if (aux == 65) {
            // flecha arriba
            if (node == NULL) {
              strcpy(straux, str);
            }
            node = next_item(history, node);
            if (node != NULL) {
              while (c > 0) {
                printf("\b \b");
                --c;
              }
              strcpy(str, node->command);
              // actualizamos el contador de caracteres
              c = strlen(str);
              printf("\rwhish> %s", str);
            }
          } else if (aux == 66) {
            // flecha abajo
            while (c > 0) {
              printf("\b \b");
              --c;
            }
            node = prev_item(history, node);
            if (node != NULL) {
              strcpy(str, node->command);
            } else {
              strcpy(str, straux);
            }
            // actualizamos el contador de caracteres
            c = strlen(str);
            printf("\rwhish> %s", str);
          }
        } else {
          node = NULL;
          printf("%c", ch);
          str[c++] = ch;
        }
        str[c] = '\0';
      }
      printf("\n");
      if (str[c] == EOF) {
        destroy_list(history);
        enable_buffering();
        exit(0);
      }
      str[c] = '\0';
      // fgets(str, MAX_SIZE, stdin);
    } else {
      if (currCom == nroCom) {
        destroy_list(history);
        enable_buffering();
        exit(0);
      }
      strcpy(str, commands[currCom++]);
    }
    s2 = str;
    trim(s2);
    if (strlen(s2) == 0) {
      continue;
    }
    insert_first(history, s2);
    // abrimos el archivo en modo de escritura y lo sobreescribimos
    fh = fopen(pathfile, "w");
    if (fh != NULL) {
      node_d *node = history->head;
      int k = 0;
      while (node != NULL && k < HISTOY_SIZE) {
        fprintf(fh, "%s\n", node->command);
        node = node->next;
        k++;
      }
      fclose(fh);
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
          continue;
        }
        // habilitamos el buffering de líneas, el modo canónico y el eco
        destroy_list(history);
        enable_buffering();
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
      // if (WEXITSTATUS(status) != 0) {
      //   // terminamos el proceso padre con el mismo valor de retorno
      //   enable_buffering();
      //   execute_exit(WEXITSTATUS(status));
      // }
    }
    subproc_count = 0; // Reseteamos el contador de procesos hijos
  } while (1);
  enable_buffering();
  return 0;
}
