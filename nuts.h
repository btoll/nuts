void create_fs(char *filename);
void decrypt(char *filename, char *outfile);
void doOperation(void (f)(char *, char *), char *filename, char *outfile, char *opName);
void encrypt(char *filename, char *outfile);
void mount_fs(char *filename, char *mntpoint);
void umount_fs(char *mntpoint);

#define MAX_SIZE 1024
#define MIN_SIZE 20

