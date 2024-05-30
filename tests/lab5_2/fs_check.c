#include "../../fs/serv.h"
#include <lib.h>

int strecmp(char *a, char *b) {
	while (*b) {
		if (*a++ != *b++) {
			return 1;
		}
	}

	return 0;
}

static char *msg = "This is the NEW message of the day!\n";
static char *diff_msg = "This is a different massage of the day!\n";
/*
void fs_check() {
	struct File *f;
	int r;
	void *blk;
	u_int *bits;

	// back up bitmap
	if ((r = syscall_mem_alloc(0, (void *)UTEMP, PTE_D)) < 0) {
		user_panic("syscall_mem_alloc: %d", r);
	}

	bits = (u_int *)UTEMP;
	memcpy(bits, bitmap, PAGE_SIZE);

	// allocate block
	if ((r = alloc_block()) < 0) {
		user_panic("alloc_block: %d", r);
	}

	// check that block was free
	user_assert(bits[r / 32] & (1 << (r % 32)));
	// and is not free any more
	user_assert(!(bitmap[r / 32] & (1 << (r % 32))));
	debugf("alloc_block is good\n");

	if ((r = file_open("/not-found", &f)) < 0 && r != -E_NOT_FOUND) {
		user_panic("file_open /not-found: %d", r);
	} else if (r == 0) {
		user_panic("file_open /not-found succeeded!");
	}

	if ((r = file_open("/newmotd", &f)) < 0) {
		user_panic("file_open /newmotd: %d", r);
	}

	debugf("file_open is good\n");

	if ((r = file_get_block(f, 0, &blk)) < 0) {
		user_panic("file_get_block: %d", r);
	}

	if (strecmp(blk, msg) != 0) {
		user_panic("file_get_block returned wrong data");
	}

	debugf("file_get_block is good\n");

	file_flush(f);
	debugf("file_flush is good\n");

	if ((r = file_set_size(f, 0)) < 0) {
		user_panic("file_set_size: %d", r);
	}

	user_assert(f->f_direct[0] == 0);
	debugf("file_truncate is good\n");

	if ((r = file_set_size(f, strlen(diff_msg))) < 0) {
		user_panic("file_set_size 2: %d", r);
	}

	if ((r = file_get_block(f, 0, &blk)) < 0) {
		user_panic("file_get_block 2: %d", r);
	}

	strcpy((char *)blk, diff_msg);
	file_flush(f);
	file_close(f);
	debugf("file rewrite is good\n");
}*/

int main() {
	fs_init();
	//fs_check();
	struct File *f;
	int r;
	if ((r = file_open("/newmotd", &f)) < 0) {
		user_panic("file_open /newmotd: %d", r);
	}
	debugf("r: %d\n", r);

	char buf[512];
	int pid;
	if ((pid = fork()) == 0) {
		read(r, buf, 511);
		debugf("child fd after read: %d\n", r);
		struct Fd *fdd;
		fd_lookup(r, &fdd);
		debugf("child fd->offset: %d\n", fdd->fd_offset);
	} else {
		read(r, buf, 511);
		debugf("parent fd after read: %d\n", r);
		struct Fd *fdd;
		fd_lookup(r, &fdd);
		debugf("parent fd->offset: %d\n", fdd->fd_offset);
	}
 	return 0;
}
