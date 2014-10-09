/***************************************************************************
 *  Title: Runtime environment 
 * -------------------------------------------------------------------------
 *    Purpose: Runs commands
 *    Author: Stefan Birrer
 *    Version: $Revision: 1.1 $
 *    Last Modification: $Date: 2005/10/13 05:24:59 $
 *    File: $RCSfile: runtime.c,v $
 *    Copyright: (C) 2002 by Stefan Birrer
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    $Log: runtime.c,v $
 *    Revision 1.1  2005/10/13 05:24:59  sbirrer
 *    - added the skeleton files
 *
 *    Revision 1.6  2002/10/24 21:32:47  sempi
 *    final release
 *
 *    Revision 1.5  2002/10/23 21:54:27  sempi
 *    beta release
 *
 *    Revision 1.4  2002/10/21 04:49:35  sempi
 *    minor correction
 *
 *    Revision 1.3  2002/10/21 04:47:05  sempi
 *    Milestone 2 beta
 *
 *    Revision 1.2  2002/10/15 20:37:26  sempi
 *    Comments updated
 *
 *    Revision 1.1  2002/10/15 20:20:56  sempi
 *    Milestone 1
 *
 ***************************************************************************/
#define __RUNTIME_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

/************Private include**********************************************/
#include "runtime.h"
#include "io.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

/************Global Variables*********************************************/

#define NBUILTINCOMMANDS (sizeof BuiltInCommands / sizeof(char*))

char* BuiltInCommands[] = {"cd", "fg", "bg", "jobs", "alias", "unalias"};

typedef struct bgjob_l {
  pid_t pid;
  int order;
  char* cmdline;
  struct bgjob_l* next;
} bgjobL;

/* the pids of the background processes */
bgjobL *bgjobs = NULL;

/************Function Prototypes*****************************************/
/* run command */
static void RunCmdFork(commandT*, bool);
/* runs an external program command after some checks */
static void RunExternalCmd(commandT*, bool);
/* resolves the path and checks for exutable flag */
static bool ResolveExternalCmd(commandT*);
/* forks and runs a external program */
static void Exec(commandT*, bool);
/* runs a builtin command */
static void RunBuiltInCmd(commandT*);
/* checks whether a command is a builtin command */
static bool IsBuiltIn(char*);

/************External Declaration*****************************************/
/* cd:
 *   change directory of current process
**/
static void ChangeDirectory(char*);

/* Background Jobs: */
static void AddBgJob(pid_t, char*);
static bgjobL* QueryBgJob(pid_t);
static void RemoveBgJob(pid_t);
static void FreeBgJob(bgjobL*);

/**************Implementation***********************************************/
int total_task;
void RunCmd(commandT** cmd, int n)
{
  int i;
  total_task = n;
  if (n == 1) {
    if (cmd[0]->is_redirect_in) {
      if (cmd[0]->is_redirect_out) {
        RunCmdRedirInOut(cmd[0], cmd[0]->redirect_in, cmd[0]->redirect_out);
      }
      else {
        RunCmdRedirIn(cmd[0], cmd[0]->redirect_in);
      }
    }
    else if (cmd[0]->is_redirect_out) {
      RunCmdRedirOut(cmd[0], cmd[0]->redirect_out);
    }
    else {
      RunCmdFork(cmd[0], TRUE);
    }
  }
  else{
    RunCmdPipe(cmd[0], cmd[1]);
    for(i = 0; i < n; i++)
      ReleaseCmdT(&cmd[i]);
  }
}

void RunCmdFork(commandT* cmd, bool fork)
{
  if (cmd->argc<=0)
    return;
  if (IsBuiltIn(cmd->argv[0]))
  {
    RunBuiltInCmd(cmd);
  }
  else
  {
    RunExternalCmd(cmd, fork);
  }
}

void RunCmdBg(commandT* cmd)
{
  // TODO
}

void RunCmdPipe(commandT* cmd1, commandT* cmd2)
{
}

void RunCmdRedirInOut(commandT* cmd, char* inFile, char* outFile)
{
  int fidOut = open(outFile, O_WRONLY | O_CREAT, 0644);
  int fidIn = open(inFile, O_RDONLY);

  int origStdout = dup(1);
  int origStdin = dup(0);

  dup2(fidOut, 1);
  dup2(fidIn, 0);

  RunCmdFork(cmd, TRUE);

  dup2(origStdout, 1);
  dup2(origStdin, 0);

  close(origStdout);
  close(origStdin);
}

void RunCmdRedirOut(commandT* cmd, char* file)
{
  int fid = open(file, O_WRONLY | O_CREAT, 0644);
  int origStdout = dup(1);
  dup2(fid, 1);

  RunCmdFork(cmd, TRUE);

  dup2(origStdout, 1);
  close(origStdout);
}

void RunCmdRedirIn(commandT* cmd, char* file)
{
  int fid = open(file, O_RDONLY);
  int origStdin = dup(0);
  dup2(fid, 0);

  RunCmdFork(cmd, TRUE);

  dup2(origStdin, 0);
  close(origStdin);
}


/*Try to run an external command*/
static void RunExternalCmd(commandT* cmd, bool fork)
{
  if (ResolveExternalCmd(cmd)){
    Exec(cmd, fork);
  }
  else {
    printf("%s: command not found\n", cmd->argv[0]);
    fflush(stdout);
    ReleaseCmdT(&cmd);
  }
}

/*Find the executable based on search list provided by environment variable PATH*/
static bool ResolveExternalCmd(commandT* cmd)
{
  char *pathlist, *c;
  char buf[1024];
  int i, j;
  struct stat fs;

  if(strchr(cmd->argv[0],'/') != NULL){
    if(stat(cmd->argv[0], &fs) >= 0){
      if(S_ISDIR(fs.st_mode) == 0)
        if(access(cmd->argv[0],X_OK) == 0){/*Whether it's an executable or the user has required permisson to run it*/
          cmd->name = strdup(cmd->argv[0]);
          return TRUE;
        }
    }
    return FALSE;
  }
  pathlist = getenv("PATH");
  if(pathlist == NULL) return FALSE;
  i = 0;
  while(i<strlen(pathlist)){
    c = strchr(&(pathlist[i]),':');
    if(c != NULL){
      for(j = 0; c != &(pathlist[i]); i++, j++)
        buf[j] = pathlist[i];
      i++;
    }
    else{
      for(j = 0; i < strlen(pathlist); i++, j++)
        buf[j] = pathlist[i];
    }
    buf[j] = '\0';
    strcat(buf, "/");
    strcat(buf,cmd->argv[0]);
    if(stat(buf, &fs) >= 0){
      if(S_ISDIR(fs.st_mode) == 0)
        if(access(buf,X_OK) == 0){/*Whether it's an executable or the user has required permisson to run it*/
          cmd->name = strdup(buf); 
          return TRUE;
        }
    }
  }
  return FALSE; /*The command is not found or the user don't have enough priority to run.*/
}

static void Exec(commandT* cmd, bool forceFork)
{
  int status;
  pid_t childPid = fork();

  if (childPid > 0) {
    if (cmd->bg == 0) {
      waitpid(childPid, &status, 0);
    }
    AddBgJob(childPid, cmd->cmdline);
  }
  else if (childPid == 0) {
    execv(cmd->name, cmd->argv);
    PrintPError("External command error!");
    exit(1);
  }
  else {
    PrintPError("Fork failed!");
  }

}

static bool IsBuiltIn(char* cmd)
{
  int i;
  for (i = 0; i < NBUILTINCOMMANDS; i++) {
    if (strcmp(cmd, BuiltInCommands[i]) == 0)
      return TRUE;
  }
  return FALSE;     
}


static void RunBuiltInCmd(commandT* cmd)
{
  if (strcmp("cd", cmd->argv[0]) == 0) {
    ChangeDirectory(cmd->argv[1]);
  }
  if (strcmp("fg", cmd->argv[0]) == 0) {
    // FIXME fg
  }
  if (strcmp("bg", cmd->argv[0]) == 0) {
    // FIXME bg
  }
  if (strcmp("jobs", cmd->argv[0]) == 0) {
    // FIXME jobs
  }
  if (strcmp("alias", cmd->argv[0]) == 0) {
    // FIXME alias
  }
  if (strcmp("unalias", cmd->argv[0]) == 0) {
    // FIXME unalias
  }
}

void CheckJobs()
{
  pid_t childPid;
  bgjobL* bgjob;
  int status;

  while((childPid = waitpid(-1, &status, WNOHANG)) > 0) {
    bgjob = QueryBgJob(childPid);
    
    printf("[%d]   Done                    %s\n", bgjob->order, bgjob->cmdline);
    fflush(stdout);

    RemoveBgJob(childPid);
  }
}

void ChangeDirectory(char* path)
{
  char* newPath = (path != NULL) ? path : getenv("HOME");
  int err = chdir(newPath);

  char* errMsg = malloc(sizeof(char*) * (strlen(newPath + 3)));
  if (err < 0) {
    sprintf(errMsg, "cd: %s", newPath);
    PrintPError(errMsg);
  }
  free(errMsg);
}

void AddBgJob(pid_t pid, char* cmdline) {
  bgjobL* newJob;
  bgjobL* current = bgjobs;

  newJob = (bgjobL*)malloc(sizeof(bgjobL));
  newJob->pid = pid;
  newJob->cmdline = (char*)malloc(sizeof(char*) * MAXLINE);
  newJob->cmdline = strdup(cmdline);
  newJob->next = NULL;
  
  if (current == NULL) {
    newJob->order = 1;
    bgjobs = newJob;
  }
  else {
    while (current->next) {
      current = current->next;
    }
    newJob->order = current->order + 1;
    current->next = newJob;
  }
  
}

bgjobL* QueryBgJob(pid_t pid) {
  bgjobL* current = bgjobs;

  while (current->pid != pid)
    current = current->next;
  
  return current;
}

void RemoveBgJob(pid_t pid) {
  bgjobL* previous = NULL;
  bgjobL* current = bgjobs;

  while (current->pid != pid) {
    previous = current;
    current = current->next;
  }

  if (previous)
    previous->next = current->next;
  else
    bgjobs = current->next;

  FreeBgJob(current);
}


void FreeBgJob(bgjobL* bgJob) {
  if(bgJob->cmdline != NULL) free(bgJob->cmdline);
  free(bgJob);
}

commandT* CreateCmdT(int n)
{
  int i;
  commandT * cd = malloc(sizeof(commandT) + sizeof(char *) * (n + 1));
  cd -> name = NULL;
  cd -> cmdline = NULL;
  cd -> is_redirect_in = cd -> is_redirect_out = 0;
  cd -> redirect_in = cd -> redirect_out = NULL;
  cd -> argc = n;
  for(i = 0; i <=n; i++)
    cd -> argv[i] = NULL;
  return cd;
}

/*Release and collect the space of a commandT struct*/
void ReleaseCmdT(commandT **cmd){
  int i;
  if((*cmd)->name != NULL) free((*cmd)->name);
  if((*cmd)->cmdline != NULL) free((*cmd)->cmdline);
  if((*cmd)->redirect_in != NULL) free((*cmd)->redirect_in);
  if((*cmd)->redirect_out != NULL) free((*cmd)->redirect_out);
  for(i = 0; i < (*cmd)->argc; i++)
    if((*cmd)->argv[i] != NULL) free((*cmd)->argv[i]);
  free(*cmd);
}
