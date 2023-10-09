#ifndef __WISH_U_H__
#define __WISH_U_H__

void execute_exit(int exit_value);
void execute_cd(char *newpath);
void execute_path(char *mypath[100], char *paths);
void print_error_msg(char *msg);
void trim(char *str);

#endif
