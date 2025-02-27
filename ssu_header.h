#define OPENSSL_API_COMPAT 0x10100000L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <openssl/md5.h>

#include <sys/sysinfo.h>

#define DEBUG printf("DEBUG\n");

#define PATH_MAX 4096
#define BUF_MAX 4096
#define FILE_MAX 256

#define CMD_BACKUP    0b00001
#define CMD_REMOVE    0b00010
#define CMD_RECOVER   0b00100
#define CMD_LIST      0b01000
#define CMD_HELP      0b10000

#define OPT_D         0b000001
#define OPT_R         0b000010
#define OPT_Y         0b000100
#define OPT_A         0b001000
#define OPT_L         0b010000
#define OPT_N         0b100000

#define BACKUP_PATH       "/home/backup/"
#define BACKUP_LOG_PATH   "/home/backup/ssubak.log"

#define BACKUP_SEP        "\" backuped to \""
#define REMOVE_SEP        "\" removed by \""
#define RECOVER_SEP       "\" recovered to \""

#define HASH_MD5  33

typedef struct _backupNode {
  struct _dirNode *root_version_dir;
  struct _fileNode *root_file;

  char backup_time[13];
  char origin_path[PATH_MAX];
  char backup_path[PATH_MAX];

  struct _backupNode *prev_backup;
  struct _backupNode *next_backup;
} backupNode;

typedef struct _fileNode {
  struct _dirNode *root_dir;

  int backup_cnt;
  char file_name[FILE_MAX];
  char file_path[PATH_MAX];
  backupNode *backup_head;

  struct _fileNode *prev_file;
  struct _fileNode *next_file;
} fileNode;

typedef struct _dirNode {
  struct _dirNode *root_dir;

  int file_cnt;
  int subdir_cnt;
  int backup_cnt;
  char dir_name[FILE_MAX];
  char dir_path[PATH_MAX];
  fileNode *file_head;
  struct _dirNode *subdir_head;

  struct _dirNode* prev_dir;
  struct _dirNode *next_dir;
} dirNode;

typedef struct _pathNode {
  char path_name[FILE_MAX];
  int depth;

  struct _pathNode *prev_path;
  struct _pathNode *next_path;

  struct _pathNode *head_path;
  struct _pathNode *tail_path;
} pathNode;

dirNode* backup_dir_list;
dirNode *version_dir_list;

char *exe_path;
char *pwd_path;
char *home_path;

void help();
int help_process(int argc, char* argv[]);

char *get_file_name(char* path);
char *cvt_time_2_str(time_t time);
char *substr(char *str, int beg, int end);
char *c_str(char *str);
char *cvt_path_2_realpath(char* path);
int make_dir_path(char* path);

int md5(char *target_path, char *hash_result);
int cvtHash(char *target_path, char *hash_result);
int cmpHash(char *path1, char *path2);

int path_list_init(pathNode *curr_path, char *path);



//ssu_struct.c
void dirNode_init(dirNode *dir_node);
dirNode *dirNode_get(dirNode* dir_head, char* dir_name);
dirNode *dirNode_insert(dirNode* dir_head, char* dir_name, char* dir_path);
dirNode *dirNode_append(dirNode* dir_head, char* dir_name, char* dir_path);
void dirNode_remove(dirNode *dir_node);

int add_cnt_root_dir(dirNode *dir_node, int cnt);

void fileNode_init(fileNode *file_node);
fileNode *fileNode_get(fileNode *file_head, char *file_name);
fileNode *fileNode_insert(fileNode *file_head, char *file_name, char *dir_path);
void fileNode_remove(fileNode *file_node);

void backupNode_init(backupNode *backup_node);
backupNode *backupNode_get(backupNode *backup_head, char *backup_time);
backupNode *backupNode_insert(backupNode *backup_head, char *backup_time, char *file_name, char *dir_path, char *backup_path);
void backupNode_remove(backupNode *backup_node);

dirNode *get_dirNode_from_path(dirNode *dirList, char *path);
fileNode *get_fileNode_from_path(dirNode *dirList, char *path);
backupNode *get_backupNode_from_path(dirNode *dirList, char *path, char *backup_time);



//ssu_init.c
int init(char* path);
int get_backup_list(int log_fd);
void print_list(dirNode* dirList, int depth, int last_bit);
void print_depth(int depth, int is_last_bit);
int backup_list_insert(dirNode* dirList, char* backup_time, char* path, char* backup_path);
int backup_list_remove(dirNode* dirList, char* backup_time, char* path, char* backup_path);