#include <lib.h>

void usage(void) {
	printf("usage: touch file...\n");
	exit();
}

int main(int argc, char *argv[]) {
	int fd;
	if (argc < 2) {
		usage();
	}
	
	for (int i = 1; i < argc; i++) {
		if ((fd = create(argv[i], FTYPE_REG)) < 0) {
			//printf("%d\n", fd);
			if(fd == -10)
				printf("touch: cannot touch '%s': No such file or directory\n", argv[i]);
		}
	}
	
	return 0;
}
