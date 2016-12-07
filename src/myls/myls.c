/* 
Course: COP 5570
Assignment: Project 1
Author: Robert Massicotte
*/

#include <dirent.h> /* for struct dirent */
#include <grp.h> /* for struct group */
#include <pwd.h> /* for struct passwd */
#include <stdio.h> /* for I/O functions */
#include <stdlib.h> /* for exit macros */
#include <string.h> /* for string functions */
#include <sys/stat.h> /* for struct stat */
#include <time.h> /* for formatting modified time */

#define MAX_SIZE 256

int main(int argc, char *argv[])
{
	DIR *dir;
	struct dirent *dir_entry;
	struct stat file_stat;
	struct passwd *user_info;
	struct group *group_info;
	char dir_name[MAX_SIZE];
	char file_path[MAX_SIZE];
	char time_stamp[MAX_SIZE];

	if (argc > 1) {
		strncpy(dir_name, argv[1], MAX_SIZE);
	} else {
		strcpy(dir_name, ".");
	}

	dir = opendir(dir_name);

	if (dir != NULL) {
		while ((dir_entry = readdir(dir)) != NULL) {
			if (dir_entry->d_name[0] == '.') {
				/* skip hidden files */
				continue;
			}
			
			/* append the file name to the directory name to create the full
			path to be used by the stat function */
			sprintf(file_path, "%s/%s", dir_name, dir_entry->d_name);
			if (lstat(file_path, &file_stat) < 0) {
				continue;
			}

			/* print file type */
			if (S_ISREG(file_stat.st_mode)) {
				/* regular file */
				printf("-");
			} else if (S_ISDIR(file_stat.st_mode)) {
				/* directory */
				printf("d");
			} else if (S_ISBLK(file_stat.st_mode)) {
				/* block special file */
				printf("b");
			} else if (S_ISCHR(file_stat.st_mode)) {
				/* character special file */
				printf("c");
			} else if (S_ISLNK(file_stat.st_mode)) {
				/* symbolic link */
				printf("l");
			} else if (S_ISSOCK(file_stat.st_mode)) {
				/* socket */
				printf("s");
			} else if (S_ISFIFO(file_stat.st_mode)) {
				/* named pipe */
				printf("p");
			}

			/* print owner's read permission */
			printf("%c", (file_stat.st_mode & S_IRUSR) ? 'r' : '-');
			/* print owner's write permission */
			printf("%c", (file_stat.st_mode & S_IWUSR) ? 'w' : '-');
			/* print owner's execution permission */
			printf("%c", (file_stat.st_mode & S_IXUSR) ? 'x' : '-');

			/* print group's read permission */
			printf("%c", (file_stat.st_mode & S_IRGRP) ? 'r' : '-');
			/* print group's write permission */
			printf("%c", (file_stat.st_mode & S_IWGRP) ? 'w' : '-');
			/* print group's execution permission */
			printf("%c", (file_stat.st_mode & S_IXGRP) ? 'x' : '-');

			/* print other users' read permission */
			printf("%c", (file_stat.st_mode & S_IROTH) ? 'r' : '-');
			/* print other users' write permission */
			printf("%c", (file_stat.st_mode & S_IWOTH) ? 'w' : '-');
			/* print other users' execute permission */
			printf("%c ", (file_stat.st_mode & S_IXOTH) ? 'x' : '-');

			/* print number of links */
			printf("%ld ", (long)file_stat.st_nlink);

			/* print owner's login name */
			user_info = getpwuid(file_stat.st_uid);
			printf("%s ", user_info->pw_name);

			/* print group name */
			group_info = getgrgid(file_stat.st_gid);
			printf("%s ", group_info->gr_name);

			/* print file size, right-aligned */
			printf("%5ld ", (long)file_stat.st_size);

			/* convert last modified time to string, trim by 4 characters in front
			to remove name of day, so output is more like that of "ls -l" */
			strncpy(time_stamp, ctime(&file_stat.st_mtime) + 4, MAX_SIZE);
			/* strip newline */
			time_stamp[strlen(time_stamp) - 1] = '\0';
			/* print it */
			printf("%s ", time_stamp);

			/* print file name */
			printf("%s\n", dir_entry->d_name);
		}

		closedir(dir);
	} else {
		fprintf(stderr, "Failed to open directory %s\n", dir_name);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}