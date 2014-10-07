/***************************************************************************
 *  Title: Input/Output 
 * -------------------------------------------------------------------------
 *    Purpose: Handles the input and output
 *    Author: Stefan Birrer
 *    Version: $Revision: 1.1 $
 *    Last Modification: $Date: 2005/10/13 05:24:59 $
 *    File: $RCSfile: io.c,v $
 *    Copyright: (C) 2002 by Stefan Birrer
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    $Log: io.c,v $
 *    Revision 1.1  2005/10/13 05:24:59  sbirrer
 *    - added the skeleton files
 *
 *    Revision 1.4  2002/10/24 21:32:47  sempi
 *    final release
 *
 *    Revision 1.3  2002/10/23 21:54:27  sempi
 *    beta release
 *
 *    Revision 1.2  2002/10/15 20:37:26  sempi
 *    Comments updated
 *
 *    Revision 1.1  2002/10/15 20:20:56  sempi
 *    Milestone 1
 *
 ***************************************************************************/
#define __IO_IMPL__

/************System include***********************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <termios.h>
#include <assert.h>

/************Private include**********************************************/
#include "io.h"
#include "runtime.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

/************Global Variables*********************************************/

/* indicates that the standard input stream is currently read  */
bool isReading = FALSE;

/************Function Prototypes******************************************/

/************External Declaration*****************************************/

/**************Implementation***********************************************/

void PrintNewline()
{
  putchar('\n');
}

void Print(char* msg)
{
  assert(msg != NULL);
  puts(msg);
}

void PrintPError(char* msg)
{
  char* format = "%s: %s";
  char str[MAXLINE];
  if (msg == NULL)
  {
    perror(SHELLNAME);
    return;
  }
  snprintf(str, MAXLINE-1, format, SHELLNAME, msg);
  perror(str);
}

bool IsReading()
{
  return isReading;
}

void getCommandLine(char** buf, int size)
{
  char ch;
  size_t used=0;
  char* cmd = *buf;
  cmd[0] = '\0';

  isReading = TRUE;
  while (((ch = getc(stdin)) != EOF) &&
      (ch != '\n'))
  {
    if (used == size)
    {
      size *= 2;
      cmd = realloc(cmd, sizeof(char)*(size+1));
    }		
    cmd[used] = ch;
    used++;
    cmd[used] = '\0';
  }
  isReading = FALSE;
}

