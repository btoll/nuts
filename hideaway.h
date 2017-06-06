void create_fs(char *filename, char *mntpoint);
void decrypt(char *filename, char *outfile);
void encrypt(char *filename);
void get_basename(char *filename, char *buf);
void mount_fs(char *filename, char *mntpoint);
void strip_newline(char *buf, char swap);
void umount_fs(char *mntpoint);

