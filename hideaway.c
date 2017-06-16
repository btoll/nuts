//
// create_fs() -> mount_fs()
//
// Dependencies:
//      fallocate
//      gpg
//      mkfs.*
//      modprobe
//      mount
//      sudo
//      umount
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include "hideaway.h"

void create_fs(char *filename) {
    pid_t pid;

    if ((pid = fork()) == 0) {
        char buf[MIN_SIZE];
        char *fsize;

        if ((fsize = malloc(MIN_SIZE)) == NULL) {
            perror("malloc");
            exit(1);
        }

        printf("\nSize of file [1GB]: ");
        fgets(buf, MIN_SIZE, stdin);
        sscanf(buf, "%s", fsize);

        if (!strlen(fsize))
            fsize = "1GB";

        execl("/usr/bin/fallocate", "fallocate", "-l", fsize, filename, NULL);
        free(fsize);

        _exit(0);
    } else if (pid == -1) {
        perror("fallocate");
        exit(2);
    } else {
        wait(NULL);

        if ((pid = fork()) == 0) {
            char bin[MIN_SIZE] = "/sbin/";
            char *fs_type;
            int d;

            char *list[5] = {
                "mkfs.ext2",
                "mkfs.ext3",
                "mkfs.ext4",
                "mkfs.fat"
            };

            char fs[] =
                "Select the filesystem type:\n\n"
                "\t1. ext2\n"
                "\t2. ext3\n"
                "\t3. ext4\n"
                "\t4. fat\n"
                "\nSelect: "
            ;

            printf(fs);
            scanf("%d%*c", &d);

            if (d > 4) {
                printf("Choice out of range, defaulting to ext3.\n");
                d = 2;
            }

            // Zero-based.
            fs_type = list[d - 1];

            strncat(bin, fs_type, strlen(fs_type));

            // ex. execl("/sbin/mkfs.ext3", "mkfs.ext3", ...);
            execl(bin, fs_type, filename, NULL);
            _exit(0);
        } else if (pid == -1) {
            perror("mkfs.ext3");
            exit(3);
        } else
            wait(NULL);
    }
}

void decrypt(char *filename, char *outfile) {
    pid_t pid;

    if ((pid = fork()) == 0) {
        execl("/usr/bin/gpg", "gpg", "-o", outfile, "-d", filename, NULL);
        _exit(0);
    } else if (pid == -1) {
        perror("gpg");
        exit(4);
    } else {
        wait(NULL);
        printf("Decrypting to %s\n", outfile);
    }
}

// Pass a file pointer.
void doOperation(void (f)(char *, char *), char *filename, char *outfile, char *opName) {
    char buf[MIN_SIZE];
    char r;

    printf("%s? [Y/n] ", opName);
    fgets(buf, sizeof(buf) - 1, stdin);
    sscanf(buf, "%c", &r);

    if (r == 'Y' || r == 'y' || r == '\n') {
        char buf[MAX_SIZE];

        printf("Name of outfile: ");
        fgets(buf, sizeof(buf) - 1, stdin);
        sscanf(buf, "%s", outfile);
        (*f)(filename, outfile);
    }
}

/**
 * Sign and encrypt.
 * Note that recipients should be given on the cli after prompting (that's GPG itself, not this program).
 */
void encrypt(char *filename, char *outfile) {
    pid_t pid;

    if ((pid = fork()) == 0) {
        execl("/usr/bin/gpg", "gpg", "-o", outfile, "-se", filename, NULL);
        _exit(0);
    } else if (pid == -1) {
        perror("gpg");
        exit(5);
    } else {
        wait(NULL);
        printf("Signing and encrypting %s\n", outfile);
    }
}

void mount_fs(char *filename, char *mntpoint) {
    pid_t pid;
    int r;

    if ((pid = fork()) == 0) {
        execl("/sbin/modprobe", "modprobe", "loop", NULL);
        _exit(0);
    } else if (pid == -1) {
        perror("modprobe");
        exit(6);
    } else {
        wait(NULL);

        printf("Mounting to %s/\n", mntpoint);

        if ((r = mkdir(mntpoint, 0700)) == -1) {
            // 17 == "File exists", which *should* be ok :)
            if (errno != 17) {
                perror("mkdir");
                fprintf(stderr, "%d\n", errno);
                exit(7);
            }
        }

        if ((pid = fork()) == 0) {
            execl("/usr/bin/sudo", "sudo", "mount", "-o", "loop", filename, mntpoint, NULL);
            _exit(0);
        } else if (pid == -1) {
            perror("mount");
            exit(8);
        } else {
            wait(NULL);
            printf("Done!\n");
        }
    }
}

void umount_fs(char *mntpoint) {
    pid_t pid;
    int r;

    printf("Unmounting %s/\n", mntpoint);

    if ((pid = fork()) == 0) {
        execl("/usr/bin/sudo", "sudo", "umount", mntpoint, NULL);
        _exit(0);
    } else if (pid == -1) {
        perror("umount");
        exit(9);
    } else {
        wait(NULL);

        if ((r = rmdir(mntpoint)) == -1) {
            perror("rmdir");
            exit(10);
        } else
            printf("Done!\n");
    }
}

/**
 * Commands are "open" or "close".
 */
int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <command> <filename> <mntpoint>\n", argv[0]);
        return 1;
    }

    char *cmd = argv[1];
    char *filename = argv[2];
    char *mntpoint = argv[3];
    char outfile[MAX_SIZE];
    void (*fptr)(char *, char *);

    memset(outfile, 0, MAX_SIZE);
    if (strcmp(cmd, "open") == 0) {
        struct stat file_stat;

        fptr = &decrypt;
        doOperation(fptr, filename, outfile, "Decrypt");

        if (outfile[0] != '\0')
            filename = outfile;

        // Don't create if file already exists.
        if ((stat(filename, &file_stat)) == -1) {
            create_fs(filename);
            mount_fs(filename, mntpoint);
        } else
            mount_fs(filename, mntpoint);
    } else if (strcmp(cmd, "close") == 0) {
        fptr = &encrypt;
        doOperation(fptr, filename, outfile, "Encrypt");

        umount_fs(mntpoint);
    } else {
        fprintf(stderr, "[%s] Unrecognized command\n", argv[0]);
        exit(11);
    }

    return 0;
}

