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
  char *i = str;
  // eliminamos los esiacios al inicio
  while (isspace(*i)) {
    i++;
  }
  if (*i == '\0') {
    *str = '\0';
    return;
  }
  memmove(str, i, strlen(i) + 1);
  // eliminamos los espacios al final
  for (i = str + strlen(str) - 1; isspace(*i); i--) {
    *i = '\0';
  }
}

// definimos una struct para almacenar los comandos ejecutados, tipo lista
// doblemente enlazada
typedef struct node {
  char *command;
  struct node *next;
  struct node *prev;
} node_dl;

node_dl *prev_item(node_dl *node) {
  if (node->prev == NULL) {
    return node;
  }
  return node->prev;
}

node_dl *next_item(node_dl *node) {
  if (node->next == NULL) {
    return node;
  }
  return node->next;
}

typedef struct list {
  node_dl *head;
  node_dl *tail;
} list_dl;

// inicializamos la lista
list_dl *init_list() {
  list_dl *list = malloc(sizeof(list_dl));
  list->head = NULL;
  list->tail = NULL;
  return list;
}

// agregamos un elemento al inicio de la lista
void add_first(list_dl *list, char *command) {
  node_dl *new_node = malloc(sizeof(node_dl));
  new_node->command = strdup(command);
  new_node->next = list->head;
  new_node->prev = NULL;
  if (list->head != NULL) {
    list->head->prev = new_node;
  } else {
    list->tail = new_node;
  }
  list->head = new_node;
}
