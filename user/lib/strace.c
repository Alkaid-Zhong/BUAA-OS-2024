#include <lib.h>

void strace_barrier(u_int env_id) {
	int straced_bak = straced;
	straced = 0;
	while (envs[ENVX(env_id)].env_status == ENV_RUNNABLE) {
		syscall_yield();
	}
	straced = straced_bak;
}


void strace_send(int sysno) {
	if (!((SYS_putchar <= sysno && sysno <= SYS_set_tlb_mod_entry) ||
	      (SYS_exofork <= sysno && sysno <= SYS_panic)) ||
	    sysno == SYS_set_trapframe) {
		return;
	}

	// Your code here. (1/2)
	debugf("send\n");

	if (straced != 0) {
		u_int cur_env_id = syscall_getenvid();
		struct Env cur_env = envs[ENVX(cur_env_id)];
		int r = straced;
		straced = 0;
		syscall_ipc_try_send(cur_env.env_parent_id, sysno, 0, 0);
		syscall_set_env_status(cur_env.env_id, ENV_NOT_RUNNABLE);
		straced = r;
	}
}

void strace_recv() {
	// Your code here. (2/2)
	u_int cur_env_id = syscall_getenvid();
	struct Env cur_env = envs[ENVX(cur_env_id)];

	while(1) {
		syscall_ipc_recv(0);
		u_int sysno = cur_env.env_ipc_value;
		if (sysno == SYS_env_destroy) {
			break;
		}
		u_int child_env_id = cur_env.env_ipc_from;
		strace_barrier(child_env_id);
		recv_sysno(child_env_id, sysno);
		syscall_set_env_status(child_env_id, ENV_RUNNABLE);
	}

}
