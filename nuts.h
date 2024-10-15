int create_fs(char *filename);
int decrypt(char *filename, char *outfile);
//int do_operation(int (f)(char *, char *), char *filename, char *outfile, char *op_name);
int do_operation(char *fname, char *filename, char *outfile, char *op_name);
int encrypt(char *filename, char *outfile);
int mount_fs(char *filename, char *mntpoint);
void umount_fs(char *mntpoint);
void usage(char *proc);

#define MAX_SIZE 1024
#define MIN_SIZE 20

