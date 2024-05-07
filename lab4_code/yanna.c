//lab4-1-exam
主要考察添加一个系统调用的步骤，如下以用户进程调用函数 
user_lib_func(u_int whom, u_int val, const void *srcva, u_int perm) 
过程中，会使用到系统调用 syscall_func 为例归纳步骤：

1. 在user/include/lib.h 中添加：
void user_lib_func(u_int whom, u_int val, const void *srcva, u_int perm);
void syscall_func(u_int envid, u_int value, const void *srcva, u_int perm);

2. 在user/lib/syscall_lib.c中添加:
void syscall_func(u_int envid, u_int value, const void *srcva, u_int perm) {
    msyscall(SYS_func, envid, value, srcva, perm);
}

3.在user/lib中的使用 user_lib_func 函数的目标文件中编写实现该函数（注意在该函数过程中会调用 syscall_func 函数）

4. 在 include/syscall.h 中的enum 的MAX_SYSNO 前面加上 SYS_func,

5. 在 kern/syscall_all.c 的 void *syscall_table[MAX_SYSNO] 的最后加上 [SYS_func] = sys_func, （注意最后有逗号）

6. 在 kern/syscall_all.c 的 void *syscall_table[MAX_SYSNO] 的前面具体编写实现函数

int sys_func(u_int envid, u_int value, u_int srcva, u_int perm) {
	//......
}

//lab4-1-extra
//kern/syscall_all.c
extern struct Env envs[NENV]; //注意 extern！！
int sys_ipc_try_broadcast(u_int value, u_int srcva, u_int perm) {
	struct Env *e;
	struct Page *p;

	/* Step 1: Check if 'srcva' is either zero or a legal address. */
	/* 抄的sys_ipc_try_send */
	if (srcva != 0 && is_illegal_va(srcva)) {
		return -E_IPC_NOT_RECV;
	}
    
	/* 函数核心：遍历envs找后代进程 */
	int signal[NENV];
	for (u_int i = 0; i < NENV; i++) {
		if (curenv->env_id == envs[i].env_parent_id) {
			signal[i] = 1;
		} else {
			signal[i] = 0;
		}
	}
	int flag = 0;
	while(flag == 0) {
		flag = 1;
		for (u_int i = 0; i < NENV; i++) {
			if (signal[i] == 1) {
    				for (u_int j = 0; j < NENV; j++) {
					if (signal[j] == 0 && envs[i].env_id == envs[j].env_parent_id) {
						signal[j] = 1;
						flag = 0;
					}
				}
			}
		}
	}
	
	/* Step 3: Check if the target is waiting for a message. */
	/* 基于sys_ipc_try_send修改 */
	for (u_int i = 0; i < NENV; i++) {
		if(signal[i] == 1) {
			e = &(envs[i]);
            /* 以下都是抄的sys_ipc_try_send */
            if (e->env_ipc_recving == 0) {
				return -E_IPC_NOT_RECV;
			}
			e->env_ipc_value = value;
			e->env_ipc_from = curenv->env_id;
			e->env_ipc_perm = PTE_V | perm;
			e->env_ipc_recving = 0;
			e->env_status = ENV_RUNNABLE;
			TAILQ_INSERT_TAIL(&env_sched_list, e, env_sched_link);
			if (srcva != 0) {
				p = page_lookup(curenv->env_pgdir, srcva, NULL);
				if(p == NULL) return -E_INVAL;
				if (page_insert(e->env_pgdir, e->env_asid, p, e->env_ipc_dstva, perm) != 0) { 
		            return -E_INVAL;
 		       }
			}
		}
	}
	return 0;
}
