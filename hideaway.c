// TODO: Right now it's assuming that it needs to encrypt when closing.
//
// create_fs() -> mount_fs()
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

//
// fallocate -l $SIZE $NAME;
// sudo mkfs.ext3 $NAME;
//
void create_fs(char *filename, char *mntpoint) {
    pid_t pid;

    if ((pid = fork()) == 0) {
        char *buf;

        if ((buf = malloc(10)) == NULL) {
            perror("malloc");
            exit(1);
        }

        printf("\nSize of file [1GB]: ");
        fgets(buf, 10, stdin);
        strip_newline(buf, '\0');

        if (!strlen(buf))
            buf = "1GB";

        execl("/usr/bin/fallocate", "fallocate", "-l", buf, filename, NULL);
        free(buf);

        _exit(0);
    } else if (pid == -1) {
        perror("fallocate");
        exit(2);
    } else {
        wait(NULL);

        if ((pid = fork()) == 0) {
            char bin[20] = "/sbin/";
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
        } else {
            wait(NULL);
            mount_fs(filename, mntpoint);
        }
    }
}

void decrypt(char *filename, char *outfile) {
    pid_t pid;

    // `outfile` is a buffer that will be filled with the name of the outfile.
    get_basename(filename, outfile);

    if ((pid = fork()) == 0) {
        execl("/usr/bin/gpg", "gpg", "-o", outfile, "-d", filename, NULL);
        _exit(0);
    } else if (pid == -1) {
        perror("gpg");
        exit(4);
    } else {
        wait(NULL);
        printf("Decrypted to %s\n", outfile);
    }
}

/**
 * Sign and encrypt.
 * Note that recipients should be given on the cli after prompting (that's GPG itself, not this program).
 */
void encrypt(char *filename) {
    pid_t pid;

    if ((pid = fork()) == 0) {
        execl("/usr/bin/gpg", "gpg", "-se", filename, NULL);
        _exit(0);
    } else if (pid == -1) {
        perror("gpg");
        exit(5);
    } else {
        wait(NULL);
        printf("Signed and encrypted.\n");
    }
}

// Copy the filename (less its extension) into `buf`.
void get_basename(char *filename, char *buf) {
    // Find pointer to file extension and copy all chars except it.
    char *c = strrchr(filename, '.');
    int i;

    for (i = 0; filename[i] != *c; ++i)
        ;

    strncpy(buf, filename, i);
    buf[i] = '\0';
}

//
// lsmod | grep loop &> /dev/null;
// if [ $? -gt 0 ]; then
//     sudo modprobe loop;
// fi
//
// mkdir -p $MNT;
// sudo mount -o loop $NAME $MNT;
//
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

// TODO: Maybe remove this or inline.
void strip_newline(char *buf, char swap) {
    buf[strcspn(buf, "\n")] = swap;
}

void umount_fs(char *mntpoint) {
    pid_t pid;
    int r;

    if ((pid = fork()) == 0) {
        execl("/usr/bin/sudo", "sudo", "umount", mntpoint, NULL);
        _exit(0);
    } else if (pid == -1) {
        perror("umount");
        exit(9);
    } else {
        wait(NULL);
        printf("Unmounted.\n");

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

    struct stat file_stat;

    // Get file extension, if exists, and check if gpg.
    char *c;

    if (strcmp(cmd, "open") == 0) {
        if ((c = strrchr(filename, '.')) != NULL) {
            if (strncmp(c, ".gpg", 4) == 0) {
                char buf[1024];

                decrypt(filename, buf);
                filename = buf;
            }
        }

        // TODO: Automatically skip creating if file already exists?
        if ((stat(filename, &file_stat)) == -1)
            create_fs(filename, mntpoint);
        else
            mount_fs(filename, mntpoint);
    } else if (strcmp(cmd, "close") == 0) {
        encrypt(filename);
        umount_fs(mntpoint);
    } else {
        fprintf(stderr, "[%s] Unrecognized command\n", argv[0]);
        exit(11);
    }

    return 0;
}

