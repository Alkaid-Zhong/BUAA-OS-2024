# Shell 挑战性任务文档

## 实现不带 `.b` 后缀指令

执行指令最终会调用`int runcmd(char *s, int background_exc)`函数，函数传入要执行的文件名和是否后台执行，返回执行程序的返回值。

该函数会先将命令拆分到`argv[]`中，`argv[0]`为指令所要执行的文件名。之后运行`spawn(argv[0], argv)`，如果返回错误，则尝试在命令的末尾加入`.b`或者去除末尾的`.b`，之后重新执行`spawn(argv[0], argv)`。

```c
int child = spawn(argv[0], argv);
if (child < 0) {
    char cmd_b[1024];
    int cmd_len = strlen(argv[0]);
    strcpy(cmd_b, argv[0]);
    if (cmd_b[cmd_len - 1] == 'b' && cmd_b[cmd_len - 2] == '.') {
        cmd_b[cmd_len - 2] = '\0';
    } else {
        cmd_b[cmd_len] = '.';
        cmd_b[cmd_len + 1] = 'b';
        cmd_b[cmd_len + 2] = '\0';
    }
    child = spawn(cmd_b, argv);
}
```

## 实现指令条件执行

首先对`libos.c`中的`exit()`函数进行修改，让其支持返回值。在`exit()`函数中，将子进程`main()`的返回值通过IPC传输给调用该进程的Shell。

```c
int exit_status = -1;
void exit(void) {
	close_all();
	syscall_ipc_try_send(env->env_parent_id, exit_status, 0, 0);
	syscall_env_destroy(0);
}
const volatile struct Env *env;
extern int main(int, char **);
void libmain(int argc, char **argv) 
	env = &envs[ENVX(syscall_getenvid())];
	exit_status = main(argc, argv);
	exit();
}
```

在父进程中，接收子Shell执行的返回值

```c
if (child >= 0) {
    if (background_exc) {
        syscall_ipc_try_send(env->env_parent_id, child, 0, 0);
        syscall_ipc_recv(0);
        wait(child);
        exit_status = env->env_ipc_value;
    } else { // 这部分
        syscall_ipc_recv(0);
        wait(child);
        exit_status = env->env_ipc_value;
    }
}
```

之后，在Shell接收到命令之后，先调用`void runcmd_conditional(char *s) `函数，条件执行接收到的命令。这个函数会将指令按照`||`和`&&`拆分，之后条件执行，根据上一条指令的结果，判断接下来的是否执行。

```c
op = '\0';
int in_quotes = 0, in_backquotes = 0;
while(*s) { // 获取下一个token
    //......
}
if (last_op == 0 || 
    (last_op == '&' && exit_status == 0) ||
    (last_op == '|' && exit_status != 0) ||
    (last_op == ';')) { // 条件执行
    
    if ((r = fork()) < 0) {
        user_panic("fork: %d", r);
    }
    if (r == 0) {
        exit_status = runcmd(cmd_buf, background_exc); // 子进程的返回结果
        syscall_ipc_try_send(env->env_parent_id, exit_status, 0, 0);
        exit();
    } else {
        syscall_ipc_recv(0);
        wait(r);
        exit_status = env->env_ipc_value;
    }
}
last_op = op;
if (op == '\0') {
    break;
}
```

## 实现更多指令

### touch

在文件系统的请求中，加入新建文件的请求。

```c
enum 
	......
	FSREQ_CREATE,
	......
};
struct Fsreq_create {
	u_char req_path[MAXPATHLEN];
	int type;
};
```

实现创建服务的IPC以及对应函数实现

```c
void serve_create(u_int envid, struct Fsreq_create *rq) {
	int r;
	char *path = rq->req_path;
	struct File *file;
	if((r = file_create(path, &file)) < 0)
	{
		ipc_send(envid, r, 0, 0);
		return;
	}
	file->f_type = rq->type;
	ipc_send(envid, 0, 0, 0);
}
```

在`touch.c`中，只需要调用对应服务即可

```c
int main(int argc, char **argv) {
    if (argc != 2) {
        debugf("touch: use touch <filename>\n");
        return 0;
    }
    int r = open(argv[1], O_RDONLY);
    if (r >= 0) {
        printf("touch: cannot touch \'%s\': File exists\n", argv[1]);
        close(r);
        return 0;
    } else {
        if ((r = create(argv[1], FTYPE_REG)) != 0) {
            printf("touch: cannot touch \'%s\': No such file or directory\n", argv[1]);
            return 1;
        }
        return 0;
    }
}
```

### mkdir

和touch几乎一样，只是改变下创建文件的类型即可

```c
int main(int argc, char **argv) {
    char *path;
    int p = 0;
    if (argc == 2) {
        path = argv[1];
    } else if (argc == 3 && strcmp(argv[1], "-p") == 0) {
        p = 1;
        path = argv[2];
    } else {
        debugf("mkdir: use mkdir [-p] <dirname>\n");
        return 0;
    }
    int r = open(path, O_RDONLY);
    if (r >= 0) {
        printf("mkdir: cannot create directory '%s': File exists\n", path);
        close(r);
        return 0;
    } else {
        if ((r = create(path, FTYPE_DIR)) != 0) {
            if (!p) {
                printf("mkdir: cannot create directory '%s': No such file or directory\n", path);
                return 1;
            }
            char sub_path[1024];
            int len = strlen(path);
            int i;
            for (i = 0; i <= len; i++) {
                if (path[i] == '/' || path[i] == '\0') {
                    sub_path[i] = '\0';
                    create(sub_path, FTYPE_DIR);
                }
                sub_path[i] = path[i];
            }
        }
    }
    return 0;
}
```

### rm

需要在删除之前，判断对应文件的类型以及是否能删除，删除的具体实现调用lab5完成的remove函数即可

```c
int main(int argc, char **argv) {
    char *path;
    int r = 0;
    int f = 0;
    if (argc == 2) {
        path = argv[1];
    } else if (argc == 3) {
        path = argv[2];
        if (strcmp(argv[1], "-r") == 0) {
            r = 1;
        }
        if (strcmp(argv[1], "-rf") == 0) {
            r = 1;
            f = 1;
        }
    } else {
        debugf("rm: use rm [-r|f] <filename|dirname>\n");
        return 0;
    }
    int fdnum = open(path, O_RDONLY);
    if (fdnum < 0) {
        if (!f) {
            printf("rm: cannot remove '%s': No such file or directory\n", path);
        }
        return 1;
    }
    struct Fd *fd;
    fd_lookup(fdnum, &fd);
    struct Filefd *ffd = (struct Filefd*) fd;
    int type = ffd->f_file.f_type;

    if (type == FTYPE_REG) {
        remove(path);
    } else {
        if (!r) {
            printf("rm: cannot remove '%s': Is a directory\n", path);
            return 1;
        }
        remove(path);
    }
    return 0;
}
```

## 实现反引号

在`runcmd`函数执行之前，先替换反引号内的字符内容为执行结果。

```c
int runcmd(char *s, int background_exc) {
	char ori_cmd[1024];
	strcpy(ori_cmd, s);

	replaceBackquoteCommands(s);
	......
}
```

在执行的时候，将输出重定向到管道，之后将结果取出，并且拼接在原指令里面

```c
int executeCommandAndCaptureOutput(char *cmd, char *output, int maxLen) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        return -1;
    }

    int pid = fork();
    if (pid == -1) {
        return -1;
    }

    if (pid == 0) { // Child process
        dup(pipefd[1], 1);
        close(pipefd[1]);
        close(pipefd[0]);
		runcmd_conditional(cmd);
    } else { // Parent process
        close(pipefd[1]);
		int r;
		for (int i = 0; i < maxLen; i++) {
			if ((r = read(pipefd[0], output + i, 1)) != 1) {
				if (r < 0) {
					debugf("read error: %d\n", r);
				}
				break;
			}
		};
        close(pipefd[0]);
        wait(pid);

    }
    return 0;
}

int replaceBackquoteCommands(char *cmd) {
    char *begin, *end;
    char output[1024];
	
	while (1) {
		char *t = cmd;
		int in_quotes = 0;

		begin = 0;
		while(*t != '\0') {
			if (*t == '\"') {
				in_quotes = !in_quotes;
			}
			if (*t == '`' && !in_quotes) {
				begin = t;
				t++;
				break;
			}
			t++;
		}
		if (begin == NULL) {
			return 0; // No backquote found
		}
		end = 0;
		while(*t != '\0') {
			if (*t == '\"') {
				in_quotes = !in_quotes;
			}
			if (*t == '`' && !in_quotes) {
				end = t;
				t++;
				break;
			}
			t++;
		}
        if (end == NULL) {
            return -1; // Syntax error: unmatched backquote
        }

        *begin = '\0'; // Terminate the string at the start of the backquote command
        *end = '\0'; // Terminate the backquote command

		char temp_cmd[1024];
		strcpy(temp_cmd, end + 1);

        // Execute the command and capture its output
        if (executeCommandAndCaptureOutput(begin + 1, output, sizeof(output)) == -1) {
            return -1;
        }

        // Concatenate the parts
        strcat(cmd, output);
        strcat(cmd, temp_cmd);
    }
    return 0;
}
```

这样就可以实现指令的替换

## 实现注释功能

在`parsecmd(char **argv, int *rightpipe)`的过程中，遇到`#`，则直接返回`argc`，停止对后面的解析即可。

```c
int parsecmd(char **argv, int *rightpipe) {
	int argc = 0;
	while (1) {
		char *buf;
		int fd, r;
		int type = getNextToken(0, &buf);
		int p[2];
		char backquote_buf[1024] = {0};
		switch (type) {
		case TOKEN_EOF:
			return argc;
        ......
		case TOKEN_COMMENT:
			return argc;
		}
	}
	return argc;
}
```

## 实现历史指令

在开始接收命令之前，先打开`.mosh_history`文件，如果不存在则创建

```c
int history_fd = -1;
if ((history_fd = open("/.mosh_history", O_RDWR)) < 0) {
    if ((r = create("/.mosh_history", FTYPE_REG)) != 0) {
        debugf("canbit create /.mosh_history: %d\n", r);
    }
}
const int HISTORY_SIZE = 20;
char history_buf[20][1024];
int history_index = 0;
```

之后每次输入指令，都根据最近的20条指令，刷新该文件

```c
for (;;) {
    if (interactive) {
        printf("\n$ ");
    }
    readline(buf, sizeof buf);

    if (buf[0] == '#') {
        continue;
    }
    if (echocmds) {
        printf("# %s\n", buf);
    }
    // history
    strcpy(history_buf[history_index], buf);
    history_index = (history_index + 1) % HISTORY_SIZE;
    if ((history_fd = open("/.mosh_history", O_RDWR)) >= 0) {
        int i;
        for (i = 0; i < HISTORY_SIZE; i++) {
            char *history_cmd = history_buf[(history_index + i) % HISTORY_SIZE];
            if (history_cmd[0] == '\0') {
                continue;
            }
            fprintf(history_fd, "%s\n", history_cmd);
        }
        close(history_fd);
    }
}
```

当检测到`up`或者`down`的时候，只需要刷新`buf`为历史指令中的指令即可

当输入`history`时，只需要在`runcmd`中检测，并且将原本的`spawn`改为直接输出`.mosh_history`文件内容。

```c
int runcmd(char *s, int background_exc) {
	......
	// history
	if (strcmp(argv[0], "history") == 0) {
		int history_fd = -1;
		if ((history_fd = open("/.mosh_history", O_RDONLY)) < 0) {
			debugf("can not open /.mosh_history: %d\n", history_fd);
			return 1;
		}
		char history_buf[1024];
		int i = 0;
		int r;
		for (int i = 0; i < 1024; i++) {
			if ((r = read(history_fd, history_buf + i, 1)) != 1) {
				if (r < 0) {
					debugf("read error: %d\n", r);
				}
				break;
			}
		}
		printf("%s", history_buf);
		close(history_fd);
		close_all();
		if (rightpipe) {
			wait(rightpipe);
		}
		return 0;
	}
}
```

## 实现追加重定向

在原本的`parsecmd`中，重定向是将输出定向到文件，默认是文件的起始。追加重定向只需要将文件指针移到末尾即可。

```c
case TOKEN_APPEND_REDIRECT:
    if (getNextToken(0, &buf) != TOKEN_WORD) {
        debugf("syntax error: >> not followed by word\n");
        exit();
    }
    fd = open(buf, O_WRONLY);
    if (fd < 0) {
        create(buf, FTYPE_REG);
        fd = open(buf, O_WRONLY);
    }
    struct Fd *fd_struct = (struct Fd*) num2fd(fd);
    struct Filefd *ffd = (struct Filefd*) fd_struct;
    fd_struct->fd_offset = ffd->f_file.f_size;
    dup(fd, 1);
    close(fd);
    break;

```

## 实现引号支持

这个比较简单，只需要在`getNextToken()`的时候，将引号内解析为一个`TOKEN_WORD`即可。

```c
if (*cmd == '\"') { // parse a quoted word
    *cmd = '\0';
    cmd++;
    *begin = cmd;
    while (*cmd && *cmd != '\"') {
        cmd++;
    }
    if (*cmd == '\"') {
        *cmd = '\0';
        cmd++;
    }
    *end = cmd;
    return TOKEN_WORD;
} else { // parse a word
    *begin = cmd;
    while (*cmd && !strchr(WHITESPACE SYMBOLS, *cmd)) {
        cmd++;
    }
    *end = cmd;
    return TOKEN_WORD;
}
```

## 实现前后台任务管理

### 实现后台执行

在末尾解析到`&`时，代表该指令需要后台执行。

在后台执行指令前，需要记录该指令，以支持`jobs`的输出

```c
int job_counts = 0;
struct Jobs {
	int job_id;
	int pid;
	char cmd[1024];
	int status; // 0: running, 1: done
} jobs[1024];

```

```c
if (r == 0) {
    exit_status = runcmd(cmd_buf, background_exc);
    if (!background_exc) {
        syscall_ipc_try_send(env->env_parent_id, exit_status, 0, 0);
    }
    exit();
} else {
    if (!background_exc) {
        syscall_ipc_recv(0);
        wait(r); // 与前台执行的最大区别是不需要等待子shell执行结束
        exit_status = env->env_ipc_value;
    } else {
        syscall_ipc_recv(0);
        int child_pid = env->env_ipc_value;

        jobs[job_counts].job_id = job_counts + 1;
        jobs[job_counts].pid = child_pid;
        strcpy(jobs[job_counts].cmd, cmd_buf);
        jobs[job_counts].status = 0;
        job_counts++;

        exit_status = 0;
    }
}
```

第一次修改完之后后台指令并不能立刻执行，后来发现是等待输入字符的忙等导致的，修改后就可以实现后台执行了。

```c
int sys_cgetc(void) {
	int ch;
	while ((ch = scancharc()) == 0) {
		break; // 不加这个会忙等
	}
	return ch;
}
```

### 实现`jobs`

这个指令只需要输出`jobs`列表即可，与`history`一样实现为内置指令

```c
if (strcmp(argv[0], "jobs") == 0) {
    int i;
    for (i = 0; i < job_counts; i++) {
        if (jobs[i].status == 0) { // 在输出时，刷新进程状态
            jobs[i].status = envs[ENVX(jobs[i].pid)].env_status == ENV_FREE ? 1 : 0;
        }
        printf("[%d] %-10s 0x%08x %s\n", jobs[i].job_id, jobs[i].status == 0 ? "Running" : "Done", jobs[i].pid, jobs[i].cmd);
    }
    close_all();
    if (rightpipe) {
        wait(rightpipe);
    }
    return 0;
}
```

### 实现`fg`

 只需要简单`wait`下`job`对应的进程号即可

```c
if (strcmp(argv[0], "fg") == 0) {
    if (argc != 2) {
        user_panic("fg: invalid arguments\n");
    }
    int job_id = 0;
    char *s = argv[1];
    while (*s) {
        job_id = job_id * 10 + (*s++ - '0');
    }
    int i;
    for (i = 0; i < job_counts; i++) {
        if (jobs[i].status == 0) {
            jobs[i].status = envs[ENVX(jobs[i].pid)].env_status == ENV_FREE ? 1 : 0;
        }
    }
    if (job_id > job_counts) {
        user_panic("fg: job (%d) do not exist\n", job_id);
    }
    if (jobs[job_id - 1].status == 0) {
        wait(jobs[job_id - 1].pid);
        jobs[job_id - 1].status = 1;
    } else {
        user_panic("fg: (0x%08x) not running\n", jobs[job_id - 1].pid);
    }
    close_all();
    if (rightpipe) {
        wait(rightpipe);
    }
    return 0;
}
```

### 实现`kill`

直接强制结束进程号对应的进程即可

需要额外实现强制结束

```c
int sys_env_destroy_force(u_int envid) {
	struct Env *e;
	try(envid2env(envid, &e, 0)); // 这里不需要检查权限

	env_destroy(e);
	return 0;
}
```

```c
if (strcmp(argv[0], "kill") == 0) {
    if (argc != 2) {
        user_panic("kill: invalid arguments\n");
    }
    int job_id = 0;
    char *s = argv[1];
    while (*s) {
        job_id = job_id * 10 + (*s++ - '0');
    }
    int i;
    for (i = 0; i < job_counts; i++) {
        if (jobs[i].status == 0) {
            jobs[i].status = envs[ENVX(jobs[i].pid)].env_status == ENV_FREE ? 1 : 0;
        }
    }
    if (job_id > job_counts) {
        user_panic("kill: job (%d) do not exist\n", job_id);
    }
    if (jobs[job_id - 1].status == 0) {
        syscall_env_destroy(jobs[job_id - 1].pid);
        jobs[job_id - 1].status = 1;
    } else {
        user_panic("kill: (0x%08x) not running\n", jobs[job_id - 1].pid);
    }
    close_all();
    if (rightpipe) {
        wait(rightpipe);
    }
    return 0;
}
```

## 结语

至此，增强Shell所需的所有功能就都实现了~
