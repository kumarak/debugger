
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <syscall.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/reg.h>
#include <sys/user.h>

void procmsg(const char* format, ...)
{
	va_list ap;
	fprintf(stdout, "[%d] ", getpid());
	va_start(ap, format);
	vfprintf(stdout, format, ap);
	va_end(ap);
}

void run_target(const char* name)
{
	procmsg("target process started. will run '%s'\n", name);

	if(ptrace(PTRACE_TRACEME, 0, 0, 0) < 0){
		perror("ptrace");
		return;
	}

	execl(name, name, 0);
}

void run_debugger(pid_t child_pid)
{
	int wait_status;
	unsigned icounter = 0;
	procmsg("debugger started\n");

	wait(&wait_status);
	
	while(WIFSTOPPED(wait_status)){
		icounter++;
		struct user_regs_struct regs;
		ptrace(PTRACE_GETREGS, child_pid, 0, &regs);
		unsigned instr = ptrace(PTRACE_PEEKTEXT, child_pid, regs.rip, 0);
		procmsg("icounter = %u EIP = 0x%08x instr = 0x%08x\n", icounter, regs.rip, instr);
		if(ptrace(PTRACE_SINGLESTEP, child_pid, 0, 0) < 0){
			perror("ptrace");
			return;
		}
		wait(&wait_status);
	}

	procmsg("the child executed %u instructions\n", icounter);
}

int main(int argc, char** argv)
{
	pid_t child_pid;
	
	if(argc < 2){
		fprintf(stderr, "Expected a program name as an argument\n");
		return -1;
	}

	child_pid = fork();
	if(child_pid == 0)
		run_target(argv[1]);
	else if (child_pid > 0)
		run_debugger(child_pid);
	else {
		perror("fork");
		return -1;
	}
	return 0;
}
