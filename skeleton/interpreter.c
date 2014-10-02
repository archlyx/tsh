/* -*-C-*-
 *******************************************************************************
 *
 * File:         interpreter.c
 * RCS:          $Id: interpreter.c,v 1.1 2009/10/09 04:38:08 npb853 Exp $
 * Description:  Handles the input from stdin
 * Author:       Stefan Birrer
 *               AquaLab Research Group
 *               EECS
 *               Northwestern University
 * Created:      Fri Oct 06, 2006 at 13:33:58
 * Modified:     Fri Oct 06, 2006 at 13:39:14 fabianb@cs.northwestern.edu
 * Language:     C
 * Package:      N/A
 * Status:       Experimental (Do Not Distribute)
 *
 * (C) Copyright 2006, Northwestern University, all rights reserved.
 *
 *******************************************************************************
 */
#define __INTERPRETER_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
/************Private include**********************************************/
#include "interpreter.h"
#include "io.h"
#include "runtime.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */
typedef struct string_l {
  char* s;
  struct string_l* next;
} stringL;

/*Parse a single word from the param. Get rid of '"' or '''*/
char* single_param(char *st)
{
  int quot1 = 0,quot2 = 0, start = 0;
  char *t = st;
  static int idx;

  idx = 0;
  while(1){
    if(start == 1){
      if(st[idx] == '\0') return t;
      if(st[idx] == '<' || st[idx] == '>') {st[idx] = '\0'; return t;}
      if(st[idx] == ' ' && quot1 == 0 && quot2 == 0) {st[idx] = '\0';return t;}
      if(st[idx] == '\'' && quot1 == 1) {st[idx] = '\0';return t;}
      if(st[idx] == '"' && quot2 == 1) {st[idx] = '\0';return t;}
    }
    else{
      if(st[idx] == ' ' || st[idx] =='\0');
      else if(st[idx] == '"') {quot2 = 1; start = 1; t = &(st[idx+1]);}
      else if(st[idx] == '\'') {quot1 = 1; start = 1; t = &(st[idx+1]);}
      else {start = 1; t = &(st[idx]);}
    }
    idx++;
  }
}

/*Parse the single command and call single_param to parse each word in the command*/
void parser_single(char *c, int sz, commandT** cd, int bg)
{
  int i, task_argc = 0, quot1 = 0, quot2 = 0;
  char *in = NULL, * out = NULL, *tmp;
  int cmd_length;
  c[sz] = '\0';
  for(i = 0; i < sz; i++)
    if(c[i] != ' ') break;
  c = &(c[i]);
  sz = sz - i;
  cmd_length = sz;
  for(i = 0; i < sz; i++){
    if(c[i] == '\''){
      if(quot2) continue;
      else if(quot1){
        quot1 = 0;continue;
      }
      else quot1 = 1;
    }
    if(c[i] == '"'){
      if(quot1) continue;
      else if(quot2){
        quot2 = 0;continue;
      }
      else quot2 = 1;
    }
    if(c[i] == '<' && quot1 != 1 && quot2 != 1){
      if(cmd_length == sz) cmd_length = i; 
      while(i < (sz - 1) && c[i + 1] == ' ') i++;
      in = &(c[i+1]);
    }
    if(c[i] == '>' && quot1 != 1 && quot2 != 1){
      if(cmd_length == sz) cmd_length = i;
      while(i < (sz - 1) && c[i+1] == ' ') i++;
      out = &(c[i+1]);
    }
    if(c[i] == ' ' && quot1 != 1 && quot2 != 1){
      if(cmd_length == sz) task_argc++;
      while(i < (sz - 1) && c[i+1] == ' ') i++;
    }
  }
  if(c[cmd_length - 1] != ' ') task_argc++;
  //printf("%d\n",task_argc);
  (*cd) = CreateCmdT(task_argc);
  (*cd) -> bg = bg;
  (*cd)->cmdline = strdup(c);
  tmp = c;
  for(i = 0; i < task_argc; i++){
    (*cd) -> argv[i] = strdup(single_param(tmp));
    while ((*tmp) != '\0') tmp++;
    tmp ++;
  }
  if(in){
    (*cd) -> is_redirect_in = 1;
    (*cd) -> redirect_in = strdup(single_param(in));
  }
  if(out){
    (*cd) -> is_redirect_out = 1;
    (*cd) -> redirect_out = strdup(single_param(out));
  }
}


/**************Implementation***********************************************/
/*Parse the whole command line and split commands if a piped command is sent.*/
void Interpret(char* cmdLine)
{
  int task = 1;
  int bg = 0, i,k,j = 0, quotation1 = 0, quotation2 = 0;
  commandT **command;

  if(cmdLine[0] == '\0') return;

  for(i = 0; i < strlen(cmdLine); i++){
    if(cmdLine[i] == '\''){
      if(quotation2) continue;
      else if(quotation1){
        quotation1 = 0;continue;
      }
      else quotation1 = 1;
    }
    if(cmdLine[i] == '"'){
      if(quotation1) continue;
      else if(quotation2){
        quotation2 = 0;continue;
      }
      else quotation2 = 1;
    }

    if(cmdLine[i] == '|' && quotation1 != 1 && quotation2 != 1){
      task ++;
    }
  }

  command = (commandT **) malloc(sizeof(commandT *) * task);
  i = strlen(cmdLine) - 1;
  while(i >= 0 && cmdLine[i] == ' ') i--;
  if(cmdLine[i] == '&'){
    if(i == 0) return;
    bg = 1;
    cmdLine[i] = '\0';
  }

  quotation1 = quotation2 = 0;
  task = 0;
  k = strlen(cmdLine);
  for(i = 0; i < k; i++, j++){
    if(cmdLine[i] == '\''){
      if(quotation2) continue;
      else if(quotation1){
        quotation1 = 0;continue;
      }
      else quotation1 = 1;
    }
    if(cmdLine[i] == '"'){
      if(quotation1) continue;
      else if(quotation2){
        quotation2 = 0;continue;
      }
      else quotation2 = 1;
    }
    if(cmdLine[i] == '|' && quotation1 != 1 && quotation2 != 1){
      parser_single(&(cmdLine[i-j]), j, &(command[task]),bg);
      task++;
      j = -1;
    }
  }
  parser_single(&(cmdLine[i-j]), j, &(command[task]),bg);

  RunCmd(command, task+1);
  free(command);
}
