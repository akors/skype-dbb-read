
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <stdarg.h>
#include <dirent.h>

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>


#include "skypelog-reader.hpp"

int main(int argc, char **argv)
{
	DIR *d;
	struct dirent *ent;

	if(argc != 2)
    {
		fprintf(stderr, "Usage: %s path/to/skype/profile\n", *argv);
        return EXIT_FAILURE;
    }

	if(chdir(argv[1]) == -1)
    {
        perror("chdir()");
        return EXIT_FAILURE;
    }

	d = opendir(".");
	if(!d)
    {
		perror("opendir()");
        return EXIT_FAILURE;
    }

	while(errno = 0, ent = readdir(d))
		if(!strncmp(ent->d_name, "chat", 4)){
			int len = strlen(ent->d_name);

			if(len > 3 && !strcmp(ent->d_name + len - 4, ".dbb"))
				process_file(ent->d_name);
		}

	if(errno)
    {
		perror("readdir()");
        return EXIT_FAILURE;
    }

	closedir(d);

	return 0;
}

