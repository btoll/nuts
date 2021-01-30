int create_fs(char *filename);
int decrypt(char *filename, char *outfile);
//int doOperation(int (f)(char *, char *), char *filename, char *outfile, char *opName);
int doOperation(char *fname, char *filename, char *outfile, char *opName);
int encrypt(char *filename, char *outfile);
int mount_fs(char *filename, char *mntpoint);
void umount_fs(char *mntpoint);
void usage(char *proc);

#define MAX_SIZE 1024
#define MIN_SIZE 20

