#include <lib.h>
void usage(void) {
	printf("usage: rm file...\n");
	exit();
}

int main(int argc, char *argv[]) {
	int rt = 0;
	int r;

	if (argc < 2) {
		usage();
	}
	int len = strlen(argv[1]);
	int allowRemoveDir = 0;
	int forbidOutput = 0;
	int i = 1;
	if(len == 2 && argv[1][0] == '-' && argv[1][1] == 'r') {
		allowRemoveDir = 1;
		i = 2;
	}
	if(len == 3 && argv[1][0] == '-' && argv[1][1] == 'r' && argv[1][2] == 'f') {
		allowRemoveDir = 1;
		forbidOutput = 1;
		i = 2;
	}
	for (; i < argc; i++) {
		if ((r = remove(argv[i], allowRemoveDir)) == -10) {
			if(!forbidOutput) {
				printf("rm: cannot remove '%s': No such file or directory\n", argv[i]);
			}
			rt = r;
		} else if (!forbidOutput && r == 3) {
			printf("rm: cannot remove '%s': Is a directory\n", argv[i]);
			rt = r;
		}
		//printf("%d\n", r);
	}
	//
	return rt;
}
