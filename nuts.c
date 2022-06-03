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
#include "nuts.h"

int create_fs(char *filename) {
    char buf[MAX_SIZE];
    char r;
    pid_t pid;

    printf("[WARN] There is no filename `%s`... Create? [Y/n]: ", filename);
    fgets(buf, sizeof(buf) - 1, stdin);
    sscanf(buf, "%c", &r);

    if (r == 'n') {
        return 1;
    }

    if ((pid = fork()) == 0) {
        char buf[MIN_SIZE];
        char *fsize;

        if ((fsize = malloc(MIN_SIZE)) == NULL) {
            perror("malloc");
            exit(1);
        }

        printf("\nSize of file [250MB]: ");
        fgets(buf, MIN_SIZE, stdin);
        sscanf(buf, "%s", fsize);

        if (!strlen(fsize))
            fsize = "250MB";

        if ((r = execlp("fallocate", "fallocate", "-l", fsize, filename, NULL)) == -1) {
            perror("fallocate operation");
            free(fsize);
            return 1;
        }

        free(fsize);

        _exit(0);
    } else if (pid == -1) {
        perror("fallocate: could not fork");
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

            // example -> execl("/sbin/mkfs.ext3", "mkfs.ext3", ...);
            if ((r = execl(bin, fs_type, filename, NULL)) == -1) {
                perror("mkfs.ext3 operation");
            }

            _exit(0);
        } else if (pid == -1) {
            perror("mkfs.ext3: could not fork");
            exit(3);
        } else
            wait(NULL);
    }

    return 0;
}

int decrypt(char *filename, char *outfile) {
    pid_t pid;
    int r;

    if ((pid = fork()) == 0) {
        if ((r = execlp("gpg", "gpg", "-o", outfile, "-d", filename, NULL)) == -1) {
            perror("gpg decrypt operation");
            _exit(127);
        } else _exit(0);
    } else if (pid == -1) {
        perror("gpg decrypt: could not fork");
        exit(4);
        return 1;
    } else {
        wait(NULL);
        printf("Decrypting to %s\n", outfile);
    }

    return 0;
}

// Pass a file pointer.
//int doOperation(int (f)(char *, char *), char *filename, char *outfile, char *opName) {
int doOperation(char *fname, char *filename, char *outfile, char *opName) {
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

        if (fname == "encrypt")
            r = encrypt(filename, outfile);
        else
            r = decrypt(filename, outfile);
    }

    return 0;
}

/**
 * Sign and encrypt.
 * Note that recipients should be given on the cli after prompting (that's GPG itself, not this program).
 */
int encrypt(char *filename, char *outfile) {
    pid_t pid;
    int r;

    if ((pid = fork()) == 0) {
        if ((r = execlp("gpg", "gpg", "-o", outfile, "-se", filename, NULL)) == -1) {
            perror("gpg encrypt operation");
            _exit(127);
        } else _exit(0);
    } else if (pid == -1) {
        perror("gpg encrypt: could not fork");
        return 1;
        exit(5);
    } else {
        wait(NULL);
        printf("Signing and encrypting %s\n", outfile);
    }

    return 0;
}

int mount_fs(char *filename, char *mntpoint) {
    pid_t pid;
    int r;

    if ((pid = fork()) == 0) {
        if ((r = execlp("modprobe", "modprobe", "loop", NULL)) == -1) {
            perror("modprobe operation");
            return 1;
        }
        _exit(0);
    } else if (pid == -1) {
        perror("mount_fs: could not fork");
        return 1;
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
            if ((r = execlp("sudo", "sudo", "mount", "-o", "loop", filename, mntpoint, NULL)) == -1) {
                perror("mount operation");
                return 1;
            }

            _exit(0);
        } else if (pid == -1) {
            perror("mount: could not fork");
            return 1;
            exit(8);
        } else {
            wait(NULL);
            printf("Done!\n");
        }
    }

    return 0;
}

void umount_fs(char *mntpoint) {
    pid_t pid;
    int r;

    printf("Unmounting %s/\n", mntpoint);

    if ((pid = fork()) == 0) {
        if ((r = execlp("sudo", "sudo", "umount", mntpoint, NULL)) == -1) {
            perror("umount operation");
        }

        _exit(0);
    } else if (pid == -1) {
        perror("umount: could not fork");
        exit(9);
    } else {
        wait(NULL);

        if ((r = rmdir(mntpoint)) == -1) {
            perror("rmdir");
            exit(10);
        } else printf("Done!\n");
    }
}

void usage(char *proc) {
    fprintf(stderr, "Usage: %s <command> <filename> <mntpoint>\n", proc);
}

/**
 * Commands are "open" or "close".
 */
int main(int argc, char **argv) {
    if (argc < 4) {
        usage(argv[0]);
        return 1;
    }

    char *cmd = argv[1];
    char *filename = argv[2];
    char *mntpoint = argv[3];
    char outfile[MAX_SIZE];
//    void (*fptr)(char *, char *);

    memset(outfile, 0, MAX_SIZE);
    if (strcmp(cmd, "open") == 0) {
        struct stat file_stat;

        // Only create new filesystem if nothing is found by that name.
        if ((stat(filename, &file_stat)) == -1) {
            if (create_fs(filename)) {
                fprintf(stderr, "Aborting!\n");
                return 1;
            }
//        } else (doOperation(&decrypt, filename, outfile, "Decrypt"));
        } else (doOperation("decrypt", filename, outfile, "Decrypt"));

        if (outfile[0] != '\0')
            filename = outfile;

        mount_fs(filename, mntpoint);
    } else if (strcmp(cmd, "close") == 0) {
//        doOperation(&encrypt, filename, outfile, "Encrypt");
        doOperation("encrypt", filename, outfile, "Encrypt");
        umount_fs(mntpoint);
    } else {
        fprintf(stderr, "[%s] Unrecognized command\n", argv[0]);
        exit(11);
    }

    return 0;
}

