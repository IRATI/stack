/*
 * Written by Francesco Salvestrini <f.salvestrini@nextworks.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <sys/user.h>
#include <asm/ptrace-abi.h>
#include <asm/unistd.h>

/* We will do System-Calls-Interposition (SCI) here */

/* http://wiki.virtualsquare.org/wiki/index.php/System_Call_Interposition:_how_to_implement_virtualization */

/* strace */

int main(int argc, char * argv[])
{
        pid_t child;

        child = fork();
        if (child == -1) {
                printf("Error forking\n");
                return -1;
        }

        if (child == 0) {
                /* Child */

                if (ptrace(PTRACE_TRACEME, 0, NULL, NULL)) {
                        printf("Couldn't ptrace(TRACEME)\n");
                        return -1;
                }
                if (ptrace(PTRACE_SYSCALL, 0)) {
                        printf("Couldn't ptrace(SETOPTIONS)\n");
                        return -1;
                }
                argv++;
                execvp(argv[0],argv);
        } else {
                /* Parent */

                pid_t w;
                int   status;
                
                do {
                        printf("Ciclo");
                        w = waitpid(child, &status, WSTOPPED|WCONTINUED);
                        if (w == -1) {
                                printf("Error waitpid");
                                return -1;                        }
                        printf("Sbloccato\n");
                } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }

        return 0;
}
