#include <args.h>
#include <lib.h>

#define WHITESPACE " \t\r\n"
#define SYMBOLS "<|>&;()`\"#"

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
int mainEnvid = 0;
int res = -1;
char lastFlag = 0;
int re_alloc = 0;
int flagOfRellocate = 0;
int flagOr = 0;
int flagAnd = 0;
int _gettoken(char *s, char **p1, char **p2) {
	*p1 = 0;
	*p2 = 0;
	static int in_quot1 = 0;
	if (s == 0) {
		in_quot1 = 0;
		return 0;
	}

	while (strchr(WHITESPACE, *s)) {
		*s++ = 0;
	}
	if (*s == 0) {
		return 0;
	}
	//lab6-challenge 寮曞彿
	if(*s == '\"') {
		*s = 0;
		s++;
		*p1 = s;
		while(*s && (*s != '\"')){
			s++;
		}
		*s++ = 0;
		*p2 = s;
		return 'w';
	}
	//lab6-challenge 娉ㄩ噴
	if(*s == '#'){
		while(*s) {
			s++;
		}
		return 0;
	}
	if(*s == '>') {
		if(*(s+1) == '>') {
			flagOfRellocate = 1;
			s++;
		}
	}
	
	if(*s == '|') {
		if(*(s+1) == '|') {
			flagOr = 1;
			s++;
		}
	}
	if(*s == '&') {
		if(*(s+1) == '&') {
			flagAnd = 1;
			s++;
		}
	}
	
	if (strchr(SYMBOLS, *s)) {
		if (*s == '`') {
                         in_quot1 = !in_quot1;
                }
		int t = *s;
		*p1 = s;
		*s++ = 0;
		*p2 = s;
		return t;
	}

	*p1 = s;
	while (*s && ((in_quot1 && !strchr("`", *s)) || !strchr(WHITESPACE SYMBOLS, *s))) {
		s++;
	}
	*p2 = s;
	return 'w';
}

int gettoken(char *s, char **p1) {
	static int c, nc;
	static char *np1, *np2;

	if (s) {
		nc = _gettoken(s, &np1, &np2);
		return 0;
	}
	c = nc;
	*p1 = np1;
	nc = _gettoken(np2, &np1, &np2);
	return c;
}

#define MAXARGS 128
int jobs_n;
struct Jobs {
	char cmd[MAXPATHLEN];
	int envid;
	int id;
} jobs[300];
int parsecmd(char **argv, int *rightpipe) {
	int argc = 0;
	int len = 0;
	int flagOfIpc = 0;
	while (1) {
		char *t;
		int fd, r;
		int c = gettoken(0, &t);
		int forktemp = 0;
		int cmdlen = 0;
		char temp[128];
		//printf("%c\n",c);
		switch (c) {
		case '#':
		case 0:
			//printf("%d %d %d\n", syscall_getenvid(), envs[ENVX(syscall_getenvid())].env_parent_id, mainEnvid);
			//ipc_recv(&flagOfIpc, 0 ,0);
			//debugf("End res flag= %d %c\n",res, lastFlag);
			if(res == -1) {
				return argc;
			} else {
				if(res == 0 && lastFlag == '&')
					return argc;
				else if (res == 1 && lastFlag == '|')
					return argc;
				else 
					return 0;
			
			}
		case 'w':
			if (argc >= MAXARGS) {
				debugf("too many arguments\n");
				exit();
			}
			argv[argc++] = t;
			break;
		case '<':
			if (gettoken(0, &t) != 'w') {
				debugf("syntax error: < not followed by word\n");
				exit();
			}
			// Open 't' for reading, dup it onto fd 0, and then close the original fd.
			// If the 'open' function encounters an error,
			// utilize 'debugf' to print relevant messages,
			// and subsequently terminate the process using 'exit'.
			/* Exercise 6.5: Your code here. (1/3) */
			//printf("%s\n",t);
			if ((r = open(t, O_RDONLY)) < 0) {
				user_panic("< open failed");
			}
			fd = r;
			dup(fd, 0);
			close(fd);
			//user_panic("< redirection not implemented");

			break;
		case '>':
			//printf("Line125: %d\n",flagOfRellocate);
			if (gettoken(0, &t) != 'w') {
				debugf("syntax error: > not followed by word\n");
				exit();
			}
			// Open 't' for writing, create it if not exist and trunc it if exist, dup
			// it onto fd 1, and then close the original fd.
			// If the 'open' function encounters an error,
			// utilize 'debugf' to print relevant messages,
			// and subsequently terminate the process using 'exit'.
			/* Exercise 6.5: Your code here. (2/3) */
			re_alloc = 1;
			if(flagOfRellocate) {		//challenge
				if((r = open(t, O_APPEND | O_CREAT)) < 0) {
					user_panic(">> open failed");
				}
				fd = r;
				dup(fd, 1);
				close(fd);
			} else {
				if ((r = open (t, O_WRONLY | O_CREAT)) < 0) {
					user_panic("> open failed");
				}
               	 		fd = r;
                		dup(fd, 1);
                		close(fd);
			}
			flagOfRellocate = 0;
			//user_panic("> redirection not implemented");
			break;
		case '|':;
			/*
			 * First, allocate a pipe.
			 * Then fork, set '*rightpipe' to the returned child envid or zero.
			 * The child runs the right side of the pipe:
			 * - dup the read end of the pipe onto 0
			 * - close the read end of the pipe
			 * - close the write end of the pipe
			 * - and 'return parsecmd(argv, rightpipe)' again, to parse the rest of the
			 *   command line.
			 * The parent runs the left side of the pipe:
			 * - dup the write end of the pipe onto 1
			 * - close the write end of the pipe
			 * - close the read end of the pipe
			 * - and 'return argc', to execute the left of the pipeline.
			 */
			int p[2];
			/* Exercise 6.5: Your code here. (3/3) */
			if(!flagOr) {
				pipe(p);
				if ((rightpipe = fork()) == 0) {
                    			dup(p[0], 0);
                    			close(p[0]);
                    			close(p[1]);
                    			return parsecmd(argv, rightpipe);
             	   		} else {
                    			dup(p[1], 1);
                  	  		close(p[1]);
                   			close(p[0]);
                    			return argc;
               	 		}
			} else {
				if(res == -1 || (res == 1 && lastFlag == '|') || (res == 0 && lastFlag == '&')) {
					forktemp = fork();
                                	if(forktemp == 0) {
                                        	return argc;
                                	} else if (forktemp > 0) {
                                      	  	res = ipc_recv(&flagOfIpc, 0 ,0);
                                        	wait(forktemp);
                          //              	debugf("res = %d\n",res);
                                        	*rightpipe = 0;
                                        	flagOr = 0;
						lastFlag = '|';
						return parsecmd(argv,rightpipe);
                               		}
				} else if (res == 0) {
					*rightpipe = 0;
					flagOr = 0;
					lastFlag = '|';
					return parsecmd(argv, rightpipe);
				}
				flagOr = 0;
				lastFlag = '|';
			}
			//user_panic("| not implemented");
			break;
		case ';':
			forktemp = fork();
			if (forktemp == 0) {
				return argc;
			} else if (forktemp > 0) {
				
				if(re_alloc == 0){ // 如果前一条命令出现了重定向，那么再重定向回来
					dup(1, 0);
				} else if(re_alloc == 1) {
					dup(0, 1);
				}
				ipc_recv(&flagOfIpc, 0 ,0);
				wait(forktemp);
				//ipc_recv(&flagOfIpc, 0 ,0);
				*rightpipe = 0;
				return parsecmd(argv, rightpipe);
			}
			break;
		case '&':
			//printf("%d",flagAnd);
			if(!flagAnd) {
				return argc;
			} else {
				if(res == -1 || (res == 1 && lastFlag == '|') || (res == 0 && lastFlag == '&')) {
					forktemp = fork();
                                	if(forktemp == 0) {
                                        	return argc;
                                	} else if (forktemp > 0) {
                                      	  	res = ipc_recv(&flagOfIpc, 0 ,0);
                                        	wait(forktemp);
                            //            	debugf("res = %d\n",res);
                                        	*rightpipe = 0;
                                        	flagAnd = 0;
						lastFlag = '&';
						return parsecmd(argv,rightpipe);
                               		}
				} else {
					flagAnd = 0;
					lastFlag = '&';
					*rightpipe = 0;
					return parsecmd(argv, rightpipe);
				}
			}
			break;

		case '`':
			gettoken(0, &t);
			argv[argc++] = t;
			runcmd(t);
			if(gettoken(0,&argc) == 0){
				return argc;
			}
			break;	
		case '\"':
			gettoken(0, &t);
			argv[argc++] = t;
			if(gettoken(0,&t) == 0) {
				return argc;
			}

			break;
		}
	}
	return argc;
}

void runcmd(char *s) {
	gettoken(s, 0);

	char *argv[MAXARGS];
	int rightpipe = 0;
	int argc = parsecmd(argv, &rightpipe);
	if (argc == 0) {
		return;
	}
	
	argv[argc] = 0;
	int child = spawn(argv[0], argv);
	close_all();
	if (child >= 0) {
		wait(child);
		res = envs[ENVX(child)].env_res;
	} else {
		debugf("spawn %s: %d\n", argv[0], child);
		res = 1;
	}

	//debugf("%d %d\n", syscall_getenvid(), envs[ENVX(syscall_getenvid())].env_parent_id);
	int to = envs[ENVX(syscall_getenvid())].env_parent_id;
	if(to != mainEnvid) {
		ipc_send(to , res, 0, 0);
	}
	if (rightpipe) {
		wait(rightpipe);
	}

	exit();
}

int readline(int fd, char *buf, int n) {
	int i = 0;
	while (readn(fd, buf + i, 1) == 1) {
		if (i >= n)
			return n;
		if (buf[i] == '\n')
			break;
		i++;
	}
	buf[i] = '\0';
	return i;
}

int hsty_num;
int hsty_num;
int hsty_now;
char cmdbuf[1024];

void init_history() {
	int fd;

	if ((fd = open("/.mosh_history", O_CREAT | O_RDONLY)) < 0) {
		user_panic("open .mosh_history, %d", fd);
	}

	char tmp[1024];
	hsty_num = 0;
	while (readline(fd, tmp, 1024) > 0) {
		hsty_num++;
	}

	hsty_now = hsty_num;

	close(fd);
}

void savecmd(char *s) {
	int fd;

	if ((fd = open("/.mosh_history", O_CREAT | O_WRONLY)) < 0) {
		user_panic("open .mosh_history, %d", fd);
	}

	struct Stat stat_buf;
	fstat(fd, &stat_buf);

	seek(fd, stat_buf.st_size);

	int len = strlen(s);
	s[len] = '\n';

	write(fd, s, len + 1);
	
	s[len] = '\0';

	hsty_num++;
	hsty_now = hsty_num;
	
	close(fd);
}

void loadcmd_from_buf(int *p_cursor, char *dst, char *from) {
	int buf_len = strlen(dst);
	int cursor = *p_cursor;

	for (int i = 0; i < buf_len; i++)
		printf(" ");
	for (int i = 0; i < buf_len; i++)
		printf("\b");

	memset(dst, 0, 1024);
	strcpy(dst, from);

	printf("%s", dst);
	*p_cursor = strlen(dst);
}

void loadcmd(int *p_cursor, char *buf, int no) {
	int fd;

	if ((fd = open("/.mosh_history", O_CREAT | O_RDONLY)) < 0) {
		user_panic("open .mosh_history, %d", fd);
	}
	
	char tmp[1024];
	for (int i = 0; i <= no; i++) {
		readline(fd, tmp, 1024);
	}
	
	loadcmd_from_buf(p_cursor, buf, tmp);

	close(fd);
}

int insert_char(char *buf, int i, char ch) {
	int len = strlen(buf);
	if (len + i >= 1024) {
		return -1;
	}
	for (int j = len; j > i; j--) {
		buf[j] = buf[j - 1];
	}
	buf[i] = ch;

	len++;
	for (int j = i + 1; j < len; j++) {
		printf("%c", buf[j]);
	}
	for (int j = i + 1; j < len; j++) {
		printf("\b");
	}
	
	return 0;
}

void remove_char(char *buf, int i) {
	if (i < 0) {
		return;
	}
	for (int j = i; buf[j]; j++) {
		buf[j] = buf[j + 1];
	}

	printf("\b");
	for (int j = i; buf[j]; j++) {
		printf("%c", buf[j]);
	}
	printf(" \b");
	for (int j = i; buf[j]; j++) {
		printf("\b");
	}
}

void readcmd(char *buf) {
	int r;
	int cursor = 0;
	char ch;

	memset(buf, 0, 1024);
	
	while (1) {	
		if ((r = read(0, &ch, 1)) != 1) {
			if (r < 0) {
				debugf("read error: %d\n", r);
			}
			exit();
		}

		switch (ch) {
		case '\b':
		case 0x7f:	
			if (cursor > 0) {
				remove_char(buf, cursor - 1);
				cursor--;
			}
			break;
		case '\r':
		case '\n':
			return;
			break;
		case 0x1b: // read \e
			read(0, &ch, 1); // read [
			read(0, &ch, 1); // read A B C D for arrow keys
			switch(ch) {
			case 'A':	
				printf("%c[B", 27); 
				while(cursor > 0) {
					remove_char(buf, cursor-1);
					cursor--;
				}
				if (hsty_now == hsty_num) {
					strcpy(cmdbuf, buf);
				}
				hsty_now = hsty_now > 0 ? hsty_now - 1 : 0;
				loadcmd(&cursor, buf, hsty_now);
				break;
			case 'B':
				while(cursor>0) {
					remove_char(buf, cursor-1);
					cursor--;
				}
				hsty_now = hsty_now < hsty_num ? hsty_now + 1 : hsty_num;
				if (hsty_now == hsty_num) {
					loadcmd_from_buf(&cursor, buf, cmdbuf);
				} else {
					loadcmd(&cursor, buf, hsty_now);
				}
				break;
			case 'C':
				if (cursor < strlen(buf)) {
					cursor++;
				} else {
					printf("\b");
				}
				break;
			case 'D':
				if (cursor > 0) {
					cursor--;
				} else {
					printf(" ");
				}
				break;
			default:
				break;
			}
			break;
		default:
			if (insert_char(buf, cursor, ch) < 0) {
				goto err;
			}
			cursor++;
			break;
		}
	}

err:
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
	int jobs_n = 0;
	int len;
	int flag;
	u_int id = 0;
	int res = 0;
	int fd;
	char buffer[4096];
	char out[4096];
	int count = 0;
	int outlen = 0;
	mainEnvid = syscall_getenvid();
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

	init_history();

	for (;;) {
		if (interactive) {
			printf("\n$ ");
		}
		//readcmd(buf, sizeof buf);
		readcmd(buf);
		//printf("buf: is %s\n",buf);
		///////////////////////////////////////////&&&&&&&&&&&&&&&&&&&&&&&&&
		flag = buf[strlen(buf) - 1] == '&';
		if (buf[0] != '\0') {
			savecmd(buf);
		}

		if (buf[0] == '#') {
			continue;
		}
		if (echocmds) {
			printf("# %s\n", buf);
		}
		
		if(strlen(buf) >= 7) {
			if(buf[0] == 'h' && buf[1] == 'i' && buf[2] == 's' && buf[3] == 't' && buf[4] == 'o' && buf[5] == 'r' && buf[6] == 'y') {
				if((fd = open("/.mosh_history", O_RDONLY| O_CREAT)) < 0) {
					user_panic("open .mosh_history, fd = %d", fd);
				}
				count = 0;
				outlen = 0;	
				while((r= read(fd, buffer,4096)) >0) {
					buf[r]='\0';
					//printf("%s",buf)
					for(int i = r - 1; i >= 0; i --) {
						if(buffer[i] == '\n') {
							count++;
							if(count > 20) {
								break;
							}
						}
						out[outlen++] = buffer[i];
					}
				}
				for(int i= outlen-1; i > 0;i--) {
					printf("%c",out[i]);
				}
				//printf("\n");
				close(fd);
				continue;
			}
		}

		if(strlen(buf) >= 4) {
			if(buf[0] == 'j' && buf[1] == 'o' && buf[2] == 'b' && buf[3] == 's') {
				for(int i = 0; i < jobs_n; i++) {
					 //printf("%d\n",getStatus(jobs[i].envid));
					 printf("[%d] %-10s 0x%08x %s\n", jobs[i].id,
					 getStatus(jobs[i].envid) == 1 ? "Running" : "Done",
					 jobs[i].envid, jobs[i].cmd);
				}
				continue;	
			}
		}
		if(strlen(buf) > 4) {
			if(buf[0] == 'k' && buf[1] == 'i' && buf[2] == 'l' && buf[3] == 'l') {
				len = strlen(buf);
				id = 0;
				for(int i = 5; i < len;i++) {
					id = id * 10 + buf[i] - '0';
				}
				if(id > jobs_n) {
					printf("fg: job (%d) do not exist\n", id);
				} else {
					setStatus(jobs[id - 1].envid, 2);
				}
				continue;
			}
		}
		if(strlen(buf) > 2) {
			if(buf[0] == 'f' && buf[1] == 'g') {
				len = strlen(buf);
				id = 0;
				
				for(int i = 3; i < len;i++) {
					id = id * 10 + buf[i] - '0';
				}
				if(id > jobs_n || id <= 0) {
					printf("fg: job (%d) do not exist\n", id);
				} else {
					changeEnv(jobs[id-1].envid, jobs[id-1].cmd);
				}
				continue;
			}
		}
		
		if ((r = fork()) < 0) {
			user_panic("fork: %d", r);
		}
		if (r == 0) {
			runcmd(buf);
			exit();
		} else {
			if(flag) {
				len = strlen(buf);
				//debugf("%s",buf);
				for(int i = 0; i < len; i++) {
					jobs[jobs_n].cmd[i] = buf[i];
				}
				jobs[jobs_n].cmd[len] = '\0'; 
				jobs[jobs_n].id = jobs_n + 1;
				jobs[jobs_n].envid = r;
				jobs_n++;
			} else {	
				wait(r);
			}
		}
	}
	return 0;
}
