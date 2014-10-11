# TSH

[NAME](#NAME)

[DESCRIPTION](#DESCRIPTION)

[DESIGN](#DESIGN)

[COMMANDS](#COMMANDS)

[DIAGNOSTICS](#DIAGNOSTICS)

[BUGS](#BUGS)

[AUTHOR](#AUTHOR)

[SEE ALSO](#SEE ALSO)

* * *

## NAME
<a name="NAME"></a>

tsh &minus; A
tiny shell similar to bash

## DESCRIPTION
<a name="DESCRIPTION"></a>

**tsh** is a
tiny shell program written in C. It supports **job
control**, **pipe commands**, **redirection** and
**alias** as well as other basic linux shell
commands.

## DESIGN
<a name="DESIGN"></a>

### Job Control

Based on the linklist provided
in the skeleton to manage background processes, _jobid_
and _status_ are added to label the jobs and its status
(0: Done, 1: Running, 2: Stopped). Another struct for
foreground process is added to record whether there is a
foreground job running. The main program is consistently
check the status of the background jobs: if the return value
of the _waitpid()_ is larger than zero, then the status
of certain child processes are changed, then their newest
info are printed. Globally the shell can catch the signal
SIGINT and SIGTSTP sent externally. These signals are
directly sent to all the foreground processes using
_kill(2)_ function, according to information stored in
the foreground job struct.

### Extra Credit: Alias &amp; Unalias

A new linklist is built to
store the information of the aliases. When the alias with
argument is passed in the program, argument is splitted by
the &quot;=&quot; symbol, and the string on the two sides
are saved as they are. When a command is passed to
_Interpret()_ , all of the arguments separated by space
are firstly checked and replaced if they have alias in the
shell. Specifically, the shell can successfully process the
&quot;~&quot; symbol and replace it with **$HOME**
environmental variable. A special case for unalias command
is made in the Interpret() function, since the argument for
unalias should no be translated using unalias but directly
removed from the alias linklist.

### Extra Credit: Pipe

The pipe function is
implemented using _dup2()_ function to change the stdin
and stdout to the file identifier provided by _pipe()_
function. When multiple pipe commands are feed, they are
firstly parsed by the interpreters with an array of commandT
struct, and the execution of these commands are place in a
loop. In each step within the loop, a new child process is
forked to run the next command, and the parent process is
wait for the completion. Since the built-in command can also
be a command in the pipes, the forceFork parameter given in
the RunCmdFork is used to prevent the RunCmdFork from
forking new processes since this has already been done in
the RunCmdPipe.

### Extra Credit: IO Redirection

Similar to pipe function, the
IO redirection is implemented using function. The stdout and
stdin is directly replaced by the given file descriptors
according to the **is_redirect_in** and
**is_redirect_out** variables in the cmd struct.

## COMMANDS
<a name="COMMANDS"></a>
``` bash
cd path
```

Change current directory to the
given path. If the path is empty, it will change to
**$HOME** by default.

``` bash
fg jobID
```

Move the job from background to
foreground no matter it is stopped or not.

``` bash
bg jobID
```

Resume the stopped job in the
background and make it run in the background.

``` bash
jobs
```
Show the list of background jobs and their status.

``` bash
alias string='commands';
```

Alias the commands with the
given string. If no argument is given, it will print all the
existing aliases.

``` bash
unalias string
```

Unalias the alias made before.

``` bash
[Command] < filein > fileout
```

The standard output and input
of the command can be redirected to/from the specified
files.

``` bash
[Command 1] | [Command 2] | ...
```

Multiple pipe commands is
supported.

## DIAGNOSTICS
<a name="DIAGNOSTICS"></a>

The following
diagnostics may be issued on stderr:

* **Command not found** The command is not found in the **$PATH**
* **External command error** Exception happens during the execution of the external commands.
* **Fork failed** Cannot create a new process
* **Unalias error** The number of arguments for unalias is wrong.

## BUGS
<a name="BUGS"></a>

The pipe commands is not included in the job control. It also does
not support IO redirection. The builtIn command is supported
in pipe but does not work properly. Some of the errors are
not handled and may lead to fatal errors.

## AUTHOR
<a name="AUTHOR"></a>

Shuangping Liu

## SEE ALSO
<a name="SEE ALSO"></a>

**bash**(1) **waitpid**(2) **kill**(2)

* * *
