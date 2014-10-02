###############################################################################
#
# File:         Makefile
# RCS:          $Id: Makefile,v 1.2 2005/10/14 03:52:59 sbirrer Exp $
# Description:  Guess
# Author:       Fabian E. Bustamante
#               Northwestern Systems Research Group
#               Department of Computer Science
#               Northwestern University
# Created:      Fri Sep 12, 2003 at 15:56:30
# Modified:     Wed Sep 24, 2003 at 18:31:43 fabianb@cs.northwestern.edu
# Language:     Makefile
# Package:      N/A
# Status:       Experimental (Do Not Distribute)
#
# (C) Copyright 2003, Northwestern University, all rights reserved.
#
###############################################################################

# handin info
TEAM = `whoami`
VERSION = `date +%Y%m%d%H%M%S`
PROJ = tsh

CC = gcc
MV = mv
CP = cp
RM = rm
MKDIR = mkdir
TAR = tar cvf
COMPRESS = gzip
CFLAGS = -g -Wall -O2 -D HAVE_CONFIG_H

DELIVERY = Makefile *.h *.c test_type
PROGS = tsh
SRCS = interpreter.c io.c runtime.c tsh.c 
OBJS = ${SRCS:.c=.o}

TESTING_SRCS = myspin.c mysplit.c mystop.c
TESTING_OBJS = ${TESTING_SRCS:.c=.o}
TESTING_PROGS = myspin mysplit mystop

VM_NAME = "Ubuntu_1404"
VM_PORT = "3022"

SHELL_ARCH = "64"

all: ${PROGS}

test-reg: handin
	HANDIN=`pwd`/${TEAM}-${VERSION}-${PROJ}.tar.gz;\
	cd testsuite;\
	bash ./run_testcase.sh $${HANDIN} ${SHELL_ARCH};

start-vm:
	VBoxManage startvm ${VM_NAME} --type headless

kill-vm:
	VBoxManage controlvm ${VM_NAME} poweroff

test-vm:
	scp -r -i id_aqualab -P 3022 * aqualab@localhost:~/.aqualab/project1/.
	ssh -i id_aqualab -p 3022 aqualab@localhost 'bash -s' < vm_test.sh

	
handin: cleanAll testing-tools
	${TAR} ${TEAM}-${VERSION}-${PROJ}.tar ${DELIVERY}
	${COMPRESS} ${TEAM}-${VERSION}-${PROJ}.tar

.o:
	${CC} *.c

tsh: ${OBJS}
	${CC} -o $@ ${OBJS}

clean:
	${RM} -f *.o *~

cleanAll: clean
	${RM} -f ${PROGS} ${TEAM}-${VERSION}-${PROJ}.tar.gz
	cd testsuite;\
	${RM} ${TESTING_PROGS}

testing-tools: 
	cd testsuite;\
	${CC} -o myspin myspin.c
	cd testsuite;\
	${CC} -o mysplit mysplit.c
	cd testsuite;\
	${CC} -o mystop mystop.c
	
