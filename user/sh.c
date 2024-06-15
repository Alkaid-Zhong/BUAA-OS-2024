#include <args.h>
#include <lib.h>

#define WHITESPACE " \t\r\n"
#define SYMBOLS "<|>&;()`#\""

/* Overview:
 *   Parse the next token from the string at s.
 *
 * Post-Condition:
 *   Set '*p1' to the beginning of the token and '*p2' to just past the token.
 *   Return:
 *     - 0 if the end of string is reached.
 *     - '<' for < (stdin redirection).
 *     - '>' for > (stdout redirection).
 *     - '|' for | (pipe).
 *     - 'w' for a word (command, argument, or file name).
 *
 *   The buffer is modified to turn the spaces after words into zero bytes ('\0'), so that the
 *   returned token is a null-terminated string.
 */

#define TOKEN_EOF 0				// \0
#define TOKEN_STDIN_REDIRECT 1	// <
#define TOKEN_STDOUT_REDIRECT 2	// >
#define TOKEN_PIPE 3			// |
#define TOKEN_WORD 4			// word
#define TOKEN_BACKQUOTE 5		// `
#define TOKEN_COMMENT 6			// #
#define TOKEN_SIMICOLON 7		// ;
#define TOKEN_APPEND_REDIRECT 8	// >>
#define TOKEN_QUOTATION 9		// "
#define TOKEN_BACKGOUND_EXC 10	// &
#define TOKEN_AND 11			// &&
#define TOKEN_OR 12				// ||
#define TOKEN_ERR -1			// error

// return token type and set **begin and **end to the beginning and end of the token.
int _getNextToken(char **begin, char **end) {
	char *cmd = *begin;
	// null cmd
	if (cmd == 0) {
		return TOKEN_EOF;
	}
	// skip leading whitespace
	while (strchr(WHITESPACE, *cmd)) {
		*cmd++ = 0;
	}
	if (*cmd == '\0') {
		*begin = *end = cmd;
		return TOKEN_EOF;
	}
	// check for special symbols
	if (strchr(SYMBOLS, *cmd)) {
		switch (*cmd) {
		case '<':
			*begin = cmd;
			cmd[0] = 0;
			*end = cmd + 1;
			return TOKEN_STDIN_REDIRECT;
		case '>':
			*begin = cmd;
			if (cmd[1] == '>') {
				cmd[0] = cmd[1] = 0;
				*end = cmd + 2;
				return TOKEN_APPEND_REDIRECT;
			} else {
				cmd[0] = 0;
				*end = cmd + 1;
				return TOKEN_STDOUT_REDIRECT;
			}
		case '|':
			*begin = cmd;
			if (cmd[1] == '|') {
				cmd[0] = cmd[1] = 0;
				*end = cmd + 2;
				return TOKEN_OR;
			} else {
				cmd[0] = 0;
				*end = cmd + 1;
				return TOKEN_PIPE;
			}
		case '`':
			*begin = cmd;
			cmd[0] = 0;
			*end = cmd + 1;
			return TOKEN_BACKQUOTE;
		case '#':
			*begin = cmd;
			cmd[0] = 0;
			*end = cmd + 1;
			return TOKEN_COMMENT;
		case ';':
			*begin = cmd;
			cmd[0] = 0;
			*end = cmd + 1;
			return TOKEN_SIMICOLON;
		case '\"':
			*begin = cmd;
			cmd[0] = 0;
			*end = cmd + 1;
			return TOKEN_QUOTATION;
		case '&':
			*begin = cmd;
			if (cmd[1] == '&') {
				cmd[0] = cmd[1] = 0;
				*end = cmd + 2;
				return TOKEN_AND;
			} else {
				cmd[0] = 0;
				*end = cmd + 1;
				return TOKEN_BACKGOUND_EXC;
			}
		}
		return TOKEN_ERR;
	}
	// parse a word
	*begin = cmd;
	while (*cmd && !strchr(WHITESPACE SYMBOLS, *cmd)) {
		cmd++;
	}
	*end = cmd;
	return TOKEN_WORD;
}

/*
 * if cmd is null, return the next token in the current string.
 * if cmd is not null, parse the string and return the next token.
 */

int getNextToken(char *cmd, char **buf) {
	
	static int type;
	static char *begin, *last_end;

	debugf("getNextToken\n");

	if (cmd != 0) { // set new cmd
		begin = cmd;
		last_end = cmd;
		debugf("set command: <%s>\n", begin);
		return TOKEN_EOF;
	} else { // get next token
		begin = last_end;
		type = _getNextToken(&begin, &last_end);
		*buf = begin;
		debugf("token type %d, begin <%s>, end <%s>\n", type, begin, last_end);
		return type;
	}
}

#define MAXARGS 128

int parsecmd(char **argv, int *rightpipe) {
	int argc = 0;
	while (1) {
		char *buf;
		int fd, r;
		int type = getNextToken(0, &buf);
		debugf("type %d, buf %s\n", type, buf);
		int p[2];
		switch (type) {
		case TOKEN_EOF:
			return argc;
		case TOKEN_WORD:
			if (argc >= MAXARGS) {
				debugf("too many arguments\n");
				exit();
			}
			argv[argc++] = buf;
			break;
		case TOKEN_STDIN_REDIRECT:
			if (getNextToken(0, &buf) != 'w') {
				debugf("syntax error: < not followed by word\n");
				exit();
			}
			fd = open(buf, O_RDONLY);
			if (fd < 0) {
				debugf("open %s failed!\n", buf);
				exit();
			}
			dup(fd, 0);
			close(fd);

			break;
		case TOKEN_STDOUT_REDIRECT:
			if (getNextToken(0, &buf) != 'w') {
				debugf("syntax error: > not followed by word\n");
				exit();
			}
			fd = open(buf, O_WRONLY);
			if (fd < 0) {
				debugf("open %s failed!\n", buf);
				exit();
			}
			dup(fd, 1);
			close(fd);
			break;
		case TOKEN_PIPE:
			pipe(p);
			*rightpipe = fork();
			if (*rightpipe == 0) {
				dup(p[0], 0);
				close(p[0]);
				close(p[1]);
				return parsecmd(argv, rightpipe);
			} else if (*rightpipe > 0) {
				dup(p[1], 1);
				close(p[1]);
				close(p[0]);
				return argc;
			}
			break;
		}
	}

	return argc;
}

int runcmd(char *s) {
	debugf("running command %s\n", s);
	getNextToken(s, 0);

	char *argv[MAXARGS];
	int rightpipe = 0;
	int argc = parsecmd(argv, &rightpipe);
	if (argc == 0) {
		return 0;
	}
	argv[argc] = 0;

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

	close_all();

	int exit_status = -1;

	if (child >= 0) {
		syscall_ipc_recv(0);
		wait(child);
		exit_status = env->env_ipc_value;
	} else {
		debugf("spawn %s: %d\n", argv[0], child);
	}
	if (rightpipe) {
		wait(rightpipe);
	}
	debugf("command %s exit with return value %d\n", argv[0], exit_status);
	return exit_status;
	// exit();
}


void runcmd_conditional(char *s) {
	char cmd_buf[1024];
	int cmd_buf_len = 0;
	char op;
	char last_op = 0;
	int r;
	int exit_status = 0;

	while(1) {
		op = '\0';
		while(*s) {
			if (*s == '|' && *(s+1) == '|') {
				cmd_buf[cmd_buf_len] = '\0';
				op = '|';
				s += 2;
				cmd_buf_len = 0;
				break;
			} else if (*s == '&' && *(s+1) == '&') {
				cmd_buf[cmd_buf_len] = '\0';
				op = '&';
				s += 2;
				cmd_buf_len = 0;
				break;
			} else {
				cmd_buf[cmd_buf_len++] = *s;
				cmd_buf[cmd_buf_len] = '\0';
				s++;
			}
		}
		debugf("running cmd: %s, op %c\n", cmd_buf, op);

		if (last_op == 0 || 
		    (last_op == '&' && exit_status == 0) ||
			(last_op == '|' && exit_status != 0)) {

			if ((r = fork()) < 0) {
				user_panic("fork: %d", r);
			}
			if (r == 0) {
				exit_status = runcmd(cmd_buf);
				syscall_ipc_try_send(env->env_parent_id, exit_status, 0, 0);
				exit();
			} else {
				syscall_ipc_recv(0);
				wait(r);
				exit_status = env->env_ipc_value;
				debugf("command %s and op %c exit with return value %d\n", cmd_buf, op, exit_status);
			}

		}

		last_op = op;

		if (op == '\0') {
			break;
		}
	}
}



void readline(char *buf, u_int n) {
	int r;
	for (int i = 0; i < n; i++) {
		if ((r = read(0, buf + i, 1)) != 1) {
			if (r < 0) {
				debugf("read error: %d\n", r);
			}
			exit();
		}
		if (buf[i] == '\b' || buf[i] == 0x7f) {
			if (i > 0) {
				i -= 2;
			} else {
				i = -1;
			}
			if (buf[i] != '\b') {
				printf("\b");
			}
		}
		if (buf[i] == '\r' || buf[i] == '\n') {
			buf[i] = 0;
			return;
		}
	}
	debugf("line too long\n");
	while ((r = read(0, buf, 1)) == 1 && buf[0] != '\r' && buf[0] != '\n') {
		;
	}
	buf[0] = 0;
}

char buf[1024];

void usage(void) {
	printf("usage: sh [-ix] [script-file]\n");
	exit();
}

int main(int argc, char **argv) {
	int r;
	int interactive = iscons(0);
	int echocmds = 0;
	printf("\n:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
	printf("::                                                         ::\n");
	printf("::                     MOS Shell 2024                      ::\n");
	printf("::                                                         ::\n");
	printf(":::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
	ARGBEGIN {
	case 'i':
		interactive = 1;
		break;
	case 'x':
		echocmds = 1;
		break;
	default:
		usage();
	}
	ARGEND

	if (argc > 1) {
		usage();
	}
	if (argc == 1) {
		close(0);
		if ((r = open(argv[0], O_RDONLY)) < 0) {
			user_panic("open %s: %d", argv[0], r);
		}
		user_assert(r == 0);
	}
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
		runcmd_conditional(buf);
		// if ((r = fork()) < 0) {
		// 	user_panic("fork: %d", r);
		// }
		// if (r == 0) {
		// 	// runcmd(buf);
		// 	// exit();
		// } else {
		// 	wait(r);
		// }
	}
	return 0;
}
