void mips_init(u_int argc, char **argv, char **penv, u_int ram_low_size) {
	printk("init.c:\tmips_init() is called\n");

	mips_detect_memory(ram_low_size);
	mips_vm_init();
	page_init();
	env_init();

 ENV_CREATE(test_fktest);

	schedule(0);
	panic("init.c:\tend of mips_init() reached!");
}
