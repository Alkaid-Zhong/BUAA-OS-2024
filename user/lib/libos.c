#include <env.h>
#include <lib.h>
#include <mmu.h>

void exit(void) {
	// After fs is ready (lab5), all our open files should be closed before dying.
#if !defined(LAB) || LAB >= 5
	close_all();
#endif

	syscall_env_destroy(0);
	user_panic("unreachable code");
}

const volatile struct Env *env;
extern int main(int, char **);

void libmain(int argc, char **argv) {
	// set env to point at our env structure in envs[].
	env = &envs[ENVX(syscall_getenvid())];

	// call user main routine
	debugf("lib main called, pid: %d\n", env->env_id);
	exit_status = main(argc, argv);
	debugf("lib main returned, pid: %d, return value: %d\n", env->env_id, exit_status);

	// exit gracefully
	exit();
}
