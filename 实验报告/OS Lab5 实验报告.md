# OS Lab5 实验报告

## 思考题

### Thinking 5.1

> 如果通过 kseg0 读写设备，那么对于设备的写入会缓存到 Cache 中。这是 一种错误的行为，在实际编写代码的时候这么做会引发不可预知的问题。
>
> 请思考：这么做 这会引发什么问题？对于不同种类的设备（如我们提到的串口设备和 IDE 磁盘）的操作会 有差异吗？可以从缓存的性质和缓存更新的策略来考虑。

在系统崩溃或断电的情况下，缓存中的数据未写入设备，可能导致数据丢失。

如果在写回的时候遇到中断，也会终止写回。

对于串口设备来说，读写频繁，信号多，在相同的时间内发生错误的概论远高于IDE磁盘。

### Thinking 5.2

> 查找代码中的相关定义，试回答一个磁盘块中最多能存储多少个文件控制 块？一个目录下最多能有多少个文件？我们的文件系统支持的单个文件最大为多大？ 

指导书中有表述：

> f_direct[NDIRECT] 为文件的直接指针，每个文件控制块设有 10 个直接指针，用来记录文件的数据块在磁盘上的位置。每个磁盘块的大小为 4KB， 也就是说，这十个直接指针能够表示最大 40KB 的文件，而当文件的大小大于 40KB 时，就需要 用到间接指针。f_indirect 指向一个间接磁盘块，用来存储指向文件内容的磁盘块的指针。为 了简化计算，我们不使用间接磁盘块的前十个指针。

也就是说，一个磁盘块是4KB，每个指针是4B，一个磁盘块可以存储`4KB/4B=1K`个指针，所以一个文件最多有1k个磁盘块，也就是最大大小为`1K*4KB=4M`

### Thinking 5.3

> 请思考，在满足磁盘块缓存的设计的前提下，我们实验使用的内核支持的最 大磁盘大小是多少？ 

由于将 DISKMAP ~ DISKMAP+DISKMAX 这一段虚存地址空间作为缓冲区， DISKMAX = 0x40000000 ，因此最多处理1GB。

### Thinking 5.4

> 在本实验中，fs/serv.h、user/include/fs.h 等文件中出现了许多宏定义， 试列举你认为较为重要的宏定义，同时进行解释，并描述其主要应用之处。 

在serv.h中，有5.3提到的，磁盘最大大小

```c
/* Disk block n, when in memory, is mapped into the file system
 * server's address space at DISKMAP+(n*BLOCK_SIZE). */
#define DISKMAP 0x10000000

/* Maximum disk size we can handle (1GB) */
#define DISKMAX 0x40000000
```

在fs.h中，有块大小的定义

```c
#define BLOCK_SIZE PAGE_SIZE
```

以及文件结构体的直接指针和间接指针数量

```c
#define NDIRECT 10
#define NINDIRECT (BLOCK_SIZE / 4)
```

和文件结构体的填充大小

```c
#define FILE_STRUCT_SIZE 256
```

### Thinking 5.5

> 在 Lab4“系统调用与 fork”的实验中我们实现了极为重要的 fork 函数。那么 fork 前后的父子进程是否会共享文件描述符和定位指针呢？请在完成上述练习的基础上编写一个程序进行验证。

会共享文件描述符和定位指针。

```c
int main() {
	int r;
	int fdnum;
	char buf[512];
	int n;

	if ((r = open("/motd", O_RDWR)) < 0) {
		user_panic("open /motd: %d", r);
	}
	if ((r = open("/newmotd", O_RDWR)) < 0) {
		user_panic("open /newmotd: %d", r);
	}
	fdnum = r;
	debugf("fdnum: %d\n", fdnum);

	int id;

	if ((id = fork()) == 0) {
		if ((n = read(fdnum, buf, 511)) < 0) {
			user_panic("child read /newmotd: %d", r);
		}
		struct Fd *fdd;
		fd_lookup(r,&fdd);
		debugf("child fd->offset == %d\n",fdd->fd_offset);
	}
	else {
		if((n = read(fdnum, buf, 511)) < 0) {
			user_panic("parent read /newmotd: %d", r);
		}
		struct Fd *fdd;
		fd_lookup(r,&fdd);
		debugf("parent fd->offset == %d\n",fdd->fd_offset);
	}
 	return 0;
}
```

输出结果：

```
init.c: mips_init() is called
Memory size: 65536 KiB, number of pages: 16384
to memory 80430000 for struct Pages.
pmap.c:  mips vm init success
FS is running
superblock is good
read_bitmap is good
fdnum: 1
child fd->offset == 0
parent fd->offset == 0
[00001802] destroying 00001802
[00001802] free env 00001802
i am killed ... 
[00000800] destroying 00000800
[00000800] free env 00000800
i am killed ... 
```

### Thinking 5.6

> 请解释 File, Fd, Filefd 结构体及其各个域的作用。比如各个结构体会在哪 些过程中被使用，是否对应磁盘上的物理实体还是单纯的内存数据等。说明形式自定，要 求简洁明了，可大致勾勒出文件系统数据结构与物理实体的对应关系与设计框架。

```c
struct File {
	char f_name[MAXNAMELEN]; 		// 文件名
	uint32_t f_size; 				// 文件的大小
	uint32_t f_type; 				// 文件类型,，普通文件FTYPE_REG，目录FTYPE_DIR
	uint32_t f_direct[NDIRECT];		// 直接指针
	uint32_t f_indirect;			// 间接指针
	
	struct File *f_dir; 			// 文件所属的目录
	char f_pad[BY2FILE - MAXNAMELEN - (3 + NDIRECT) * 4 - sizeof(void *)];		//填充
} __attribute__((aligned(4), packed));

struct Fd {
     u_int fd_dev_id;     // 外设的id。
     u_int fd_offset;     // 读写的偏移量
     u_int fd_omode;      // 打开方式
};

struct Filefd {
     struct Fd f_fd;     // 文件描述符
     u_int f_fileid;     // 文件的id
     struct File f_file; // 文件控制块
};
```

### Thinking 5.7

> 图 5.9 中有多种不同形式的箭头，请解释这些不同箭头的差别，并思考我们 的操作系统是如何实现对应类型的进程间通信的。 
>
> ![lab5_p1](C:\Users\zhong\Desktop\os\lab5_p1.png)

最开始，由`init()`函数执行`ENV_CREATE(user_env) `和 `ENV_CREATE(fs_serv)`，来启动文件系统fs和用户进程user。

fs 线程初始化`serv_init()` 和 `fs_init()` 完成后，进入 `serv()` 函数，被 `ipc_receive()`阻塞，等待IPC

user 线程向 fs 线程 `ipc_send(fsreq)` 发送请求，fs使用`ipc_send(dst_va)`来返回。

之后等待下一次服务。

## 实验难点

如何理解 File, Fd, Filefd 这三个结构体我认为是本次实验最难的地方。在文件打开、操作的过程中，哪一步使用了哪个结构体，这三个结构体之间有什么联系，是本次实验的重中之重。

在上面解释了三个结构体内部成员的含义，但是只知道含义是不够的，还需要知道他们的操作过程。这点在课上实验中非常重要。

在操作系统中打开文件的过程中，`File`, `Fd`, 和 `Filefd` 结构体的使用顺序大致如下：

1. **创建和初始化 `File` 结构体**：
   - 首先，操作系统需要为新打开的文件创建一个 `File` 结构体实例，并初始化其中的域。这包括设置文件操作函数指针、初始化文件指针位置、设置文件状态标志等。
   - 读取文件的数据，缓存在内存中，并将指针存储在 `File` 结构体的块指针中。
2. **分配 `Fd` 结构体**：
   - 操作系统为新打开的文件分配一个文件描述符（整数值），并创建一个 `Fd` 结构体实例。
   - 初始化 `Fd` 结构体，将文件描述符编号和指向 `File` 结构体的指针存储在其中。
3. **创建和初始化 `Filefd` 结构体**：
   - 每个进程有一个文件描述符表，`Filefd` 结构体表示该表中的一个条目。
   - 分配和初始化一个 `Filefd` 结构体实例，将指向 `File` 结构体的指针、文件指针位置和文件状态标志存储在其中。
   - 将该 `Filefd` 结构体放入进程的文件描述符表中对应的位置。

## 心得体会

本次实验有惊无险的完成了课上的实验，虽然是最后一次实验了，但是还是觉得自己对操作系统的认识还是不够深刻。好多不需要填的函数还是只停留在了怎么用，但是没有认真理解其原理。这学期过得好快，操作系统一直是我很感兴趣的一门课，在这门课上确实完成了一个能称为操作系统的软件，但是几乎是建立在填空的基础上的。如果让我仿写一个操作系统，可能我还是做不到。实验提供的工具都很完善，使用Makefile就可以快速构建一个完整的操作系统，但是对于我来说，这些任务我似乎都无法独立完成。虽然据我所知大部分学校都是以这样的形式，但是相信每一个喜欢操作系统的同学都有个完成操作系统的梦吧。可惜随着学习的深入，越来越体会到操作系统的精妙，以及自己的能力是多么的不足。希望以后什么时候有空，能够尝试在写写，或者在以后工作的某个时候，还能想起这学期实验的时候，某些功能实现时候的一些思路。