#include <lib.h>
char temp[MAXPATHLEN];
void usage(void) {
	printf("usage: mkdir dir...\n");
	exit();
}

int main(int argc, char *argv[]) {
	int r;
	int rt = 0;
	struct Stat stat_buf;
	if (argc < 2) {
		usage();
	}
	int i;
	int flag = 1;
	if(strlen(argv[1]) == 2 && argv[1][0] == '-' && argv[1][1] == 'p') {
		i = 2;
		flag = 0;
	} else {
		i = 1;
	}
	for (; i < argc; i++) {
		if (flag && (stat(argv[i], &stat_buf) >= 0)) {
			printf("mkdir: cannot create directory '%s': File exists\n", argv[i]);
			rt = -1;
			continue;
		}
		if ((r = createDir(argv[i])) < 0) {
			if(flag) {
				printf("mkdir: cannot create directory '%s': No such file or directory\n", argv[i]);
				rt = r;
			} else {
				//r = stat(argv[i], &stat_buf);
				//r = createDir(argv[i]);
				//rt = r;
				int len = strlen(argv[i]);
				for(int j = 0; j < len; j++) {
					if(argv[i][j] == '/') {
						if(stat(temp, &stat_buf) < 0) {
							r = createDir(temp);
						}
					}
					temp[j] = argv[i][j];
				}
				r = createDir(argv[i]);
				rt = r;
			}
		}
	}

	return rt;
}
