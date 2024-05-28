// lab5-1-exam
//fs/ide.c
int time_read() {
    int time = 0;
    if (syscall_read_dev((u_int) & time, 0x15000000, 4) < 0)
        user_panic("time_read panic");
    if (syscall_read_dev((u_int) & time, 0x15000010, 4) < 0)
        user_panic("time_read panic");
    return time;
}

void raid0_write(u_int secno, void *src, u_int nsecs) {
    int i;
    for (i = secno; i < secno + nsecs; i++) {
        if (i % 2 == 0) {
            ide_write(1, i / 2, src + (i - secno) * 0x200, 1);
        } else {
            ide_write(2, i / 2, src + (i - secno) * 0x200, 1);
        }
    }
}

void raid0_read(u_int secno, void *dst, u_int nsecs) {
    int i;
    for (i = secno; i < secno + nsecs; i++) {
        if (i % 2 == 0) {
            ide_read(1, i / 2, dst + (i - secno) * 0x200, 1);
        } else {
            ide_read(2, i / 2, dst + (i - secno) * 0x200, 1);
        }
    }
}

//lab5-1-extra
#define MAXL (128)

int raid4_write(u_int blockno, void *src) {
    int i;
    int invalid = 0;
    int check[MAXL];
    for (i = 0; i < 8; i++) {
        if (raid4_valid(i % 4 + 1)) {
            ide_write(i % 4 + 1, 2 * blockno + i / 4, src + i * 0x200, 1);
        } else { invalid++; }
    }
    if (!raid4_valid(5)) {
        return invalid / 2 + 1;
    }
    int j, k;
    for (i = 0; i < 2; i++) {
        for (j = 0; j < MAXL; j++) {
            check[j] = 0;
            for (k = 0; k < 4; k++) {
                check[j] ^= *(int *) (src + (4 * i + k) * 0x200 + j * 4);
            }
        }
        ide_write(5, 2 * blockno + i, (void *) check, 1);
    }
    return invalid / 2;
}

int raid4_read(u_int blockno, void *dst) {
    int i;
    int invalid = 0;
    int wrong = 0;
    int check[2 * MAXL];
    for (i = 1; i <= 5; i++) {
        if (!raid4_valid(i)) {
            invalid++;
            wrong = i;
        }
    }
    if (invalid > 1) {
        return invalid;
    }
    for (i = 0; i < 8; i++) {
        if (i % 4 + 1 != wrong) {
            ide_read(i % 4 + 1, 2 * blockno + i / 4, dst + i * 0x200, 1);
        }
    }
    if (wrong == 5) {
        return 1;
    }
    int j, k;
    ide_read(5, 2 * blockno, check, 2);
    for (i = 0; i < 2; i++) {
        for (j = 0; j < MAXL; j++) {
            for (k = 0; k < 4; k++) {
                check[i * MAXL + j] ^= *(int *) (dst + (4 * i + k) * 0x200 + j * 4);
            }
        }
    }
    if (!wrong) {
        for (j = 0; j < 2 * MAXL; j++) {
            if (check[j] != 0) {
                return -1;
            }
        }
        return 0;
    }
    wrong--;
    user_bcopy(check, dst + wrong * 0x200, 0x200);
    user_bcopy((void *) check + 0x200, dst + 0x800 + wrong * 0x200, 0x200);
    return 1;
}

//lab5-1-exam
//考察类似于 ide_read() 仿写，题目给的步骤很清晰，一步一步使用 syscall_read_dev() 或 syscall_write_dev() 系统调用实现即可。

//lab5-1-extra
本题考察 ide_read() 和 ide_write() 函数的应用，实现一个简单的 SSD ，具体题目见文章 Lab5-1-Extra-SSD题干。
引入三个全局变量，分别表示闪存映射表、物理块位图、物理块累计擦除次数表即可。
u_int ssdtable[32];
u_int ssdbitmap[32];//1 可写
u_int ssdnum[32];
void ssd_init() {
	for (u_int i = 0; i < 32; i++) {
		ssdtable[i] = 0xffffffff;
		ssdbitmap[i] = 1;
		ssdnum[i] = 0;
	}

}
int ssd_read(u_int logic_no, void *dst) {
	if (ssdtable[logic_no] == 0xffffffff) {
		return -1;
	}
	ide_read(0, ssdtable[logic_no], dst, 1);
	return 0;
}
void ssd_write(u_int logic_no, void *src) {
	if (ssdtable[logic_no] != 0xffffffff) {
		ssd_erase(logic_no);
	}	
	//alloc----------------------
	u_int physical_no = 0xffffffff;
	for (u_int i = 0; i < 32; i++) {
		if (ssdbitmap[i] == 1) {
			if (physical_no == 0xffffffff) {
				physical_no = i;
			} else {
				if (ssdnum[i] < ssdnum[physical_no])
					physical_no = i;
			}
		}
	}
	if (ssdnum[physical_no] >= 5) {
		u_int help_no = 0xffffffff;
		u_int help_logic = 0xffffffff;
		for(u_int i = 0; i < 32; i++) {
			if (ssdbitmap[i] == 0) {
				if (help_no == 0xffffffff) {
					help_no = i;
				} else {
					if (ssdnum[i] < ssdnum[help_no])
						help_no = i;
				}
			}
		}
		for (u_int i = 0; i < 32; i++) 
			if (ssdtable[i] == help_no) {
				help_logic = i;
				break;
			}
		u_int help_data[128];
		if (ssd_read(help_logic, (void *)help_data) != 0)
			user_panic("wrong in ssd_write's help_data\n");
		ide_write(0, physical_no, (void *)help_data, 1);
		ssdbitmap[physical_no] = 0;
		ssd_erase(help_logic);
		ssdtable[help_logic] = physical_no;
		physical_no = help_no;
	}
	//---------------------------
	ssdtable[logic_no] = physical_no;
	ide_write(0, physical_no, src, 1);
	ssdbitmap[physical_no] = 0;
}
void ssd_erase(u_int logic_no) {
	if (ssdtable[logic_no] == 0xffffffff) {
		return;
	}

	//all 0 in 物理块------------
	u_int zero[128];
	for (u_int i = 0; i < 128; i++)
		zero[i] = 0;
	ide_write(0, ssdtable[logic_no], (void *)zero, 1);
	//---------------------------
	ssdnum[ssdtable[logic_no]]++;
	ssdbitmap[ssdtable[logic_no]] = 1;
	
	ssdtable[logic_no] = 0xffffffff;
}

// lab5-2-exam
我们在课下已经实现了函数 int open(const char *path, int mode) ，利用绝对路径（相对于根目录的路径）path 定位文件。在实际的用户程序中，完成同一任务时打开的多个文件往往存储在同一目录下，然而系统每次打开文件时都需要从根目录开始查找路径，从而重复访问相同的目录。

在本次题中，要求实现 int openat(int dirfd, const char *path, int mode) 函数，利用相对于目录 dirfd 的相对路径 path 定位并打开文件，其中文件描述符 dirfd 指向已通过 open() 打开的目录。相对路径 path 也可能包含路径分隔符 /，表示查找目录 dirfd 下嵌套的目录中的文件。

实现思路：

在 user/include/fsreq.h 中增加一个对于文件系统的请求类型 #define FSREQ_OPENAT 8 和请求结构体：

struct Fsreq_openat {
  	u_int dir_fileid;
  	char req_path[MAXPATHLEN];
  	u_int req_omode;
};

在 user/lib/fsipc.c 中仿照 fsipc_open 实现 int fsipc_openat(u_int dir_fileid, const char *path, u_int omode, struct Fd *fd)，完成对 Fsreq_openat 各个字段的赋值，并与文件系统服务进程进行通信。

在 user/lib/file.c 中仿照 open 函数实现 int openat(int dirfd, const char *path, int mode)，实现这一函数的相关提示：

调用 fd_lookup 利用 dirfd 查找 dirfd 的文件描述符 struct Fd *dir
将 struct Fd *dir 指向的类型转换为 struct Filefd 后获得 dirfd 对应的 fileid
调用 fsipc_openat 打开文件并完成对指针 fd 的赋值。
在 fs/fs.c 中，仿照 walk_path 实现 int walk_path_at(struct File *par_dir, char *path, struct File **pdir, struct File **pfile, char *lastelem)；

在 par_dir 目录下按相对路径 path 查找文件，并仿照 file_open 实现 int file_openat(struct File *dir, char *path, struct File **file) 调用 walk_path_at 函数。

在 fs/serv.c 中仿照 serve_open 实现 serve_openat 函数，并在 serve 函数中增加关于 openat 请求的判断。

提示：可以参考以下实现，利用 dir_fileid 查找已经被打开的 dirfd 对应的文件控制块：

struct Open *pOpen;
if ((r = open_lookup(envid, rq->dir_fileid, &pOpen)) < 0) {
	ipc_send(envid, r, 0, 0);
	return;
}
struct File *dir = pOpen->o_file;
上述函数中，需要在 user/include/lib.h 中增加 int openat(int dirfd, const char *path, int mode) 、 int fsipc_openat(u_int, const char *, u_int, struct Fd *) 函数声明，在 fs/serv.h 中增加 int file_openat(struct File *dir, char *path, struct File **pfile) 函数声明。

// lab5-2-extra
//tools/fsformat.c
void write_symlink(struct File *dirf, const char *path) {
	int iblk = 0, r = 0, n = sizeof(disk[0].data);
	struct File *target = create_file(dirf);
	char targetpath[2048] = {0};
	int len = readlink(path, targetpath, 2047);
	/* in case `create_file` is't filled */
	memcpy(disk[nextbno].data, targetpath, len);
	disk[nextbno].data[len]='\0';
	
	// Get file name with no path prefix.
	const char *fname = strrchr(path, '/');
	if (fname) {
		fname++;
	} else {
		fname = path;
	}
	strcpy(target->f_name, fname);

	target->f_size = 2048;
	target->f_type = FTYPE_LNK;

	save_block_link(target, 0 , next_block(BLOCK_DATA));
}

//user/lib/file.c
int open(const char *path, int mode) {
	//......略
    
	if(ffd->f_file.f_type == FTYPE_LNK){
		return open(fd2data(fd), mode);
	}else{
		return fd2num(fd);
	}
}
