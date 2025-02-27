#include "ssu_header.h"


int backup_list_remove(dirNode* dirList, char* backup_time, char* path, char* backup_path) {
  char* ptr;
  
  if(ptr = strchr(path, '/')) {
    char* dir_name = substr(path, 0, strlen(path) - strlen(ptr));
    dirNode* curr_dir = dirNode_get(dirList->subdir_head, dir_name);
    backup_list_remove(curr_dir, backup_time, ptr+1, backup_path);
    curr_dir->backup_cnt--;
    if(curr_dir->backup_cnt == 0) {
      dirNode_remove(curr_dir);
      curr_dir = NULL;
    }

  } else {
    char* file_name = path;
    fileNode* curr_file = fileNode_get(dirList->file_head, file_name);
    backupNode *curr_backup = backupNode_get(curr_file->backup_head, backup_time);
    backupNode_remove(curr_backup);
    curr_backup = NULL;
    curr_file->backup_cnt--;
    if(curr_file->backup_cnt == 0) {
      fileNode_remove(curr_file);
      curr_file = NULL;
    }
  }

  return 0;
}

int backup_list_insert(dirNode* dirList, char* backup_time, char* path, char* backup_path) {
  char* ptr;
  
  if(ptr = strchr(path, '/')) {
    char* dir_name = substr(path, 0, strlen(path) - strlen(ptr));
    dirNode* curr_dir = dirNode_insert(dirList->subdir_head, dir_name, dirList->dir_path);
    backup_list_insert(curr_dir, backup_time, ptr+1, backup_path);
    curr_dir->backup_cnt++;

  } else {
    char* file_name = path;
    fileNode* curr_file = fileNode_insert(dirList->file_head, file_name, dirList->dir_path);
    backupNode_insert(curr_file->backup_head, backup_time, file_name, dirList->dir_path, backup_path);
  }

  return 0;
}

void print_depth(int depth, int is_last_bit) {
  for(int i = 1; i <= depth; i++) {
    if(i == depth) {
      if((1 << depth) & is_last_bit) {
          printf("┗ ");
      } else {
          printf("┣ ");
      }
      break;
    }
    if((1 << i) & is_last_bit) {
      printf("  ");
    } else {
      printf("┃ ");
    }
  }
}

void print_list(dirNode* dirList, int depth, int last_bit) {
  dirNode* curr_dir = dirList->subdir_head->next_dir;
  fileNode* curr_file = dirList->file_head->next_file;

  while(curr_dir != NULL && curr_file != NULL) {
    if(strcmp(curr_dir->dir_name, curr_file->file_name) < 0) {
      print_depth(depth, last_bit);
      printf("%s/ %d %d %d\n", curr_dir->dir_name, curr_dir->file_cnt,
        curr_dir->subdir_cnt, curr_dir->backup_cnt);
      print_list(curr_dir, depth+1, last_bit);
      curr_dir = curr_dir->next_dir;
    } else {
      print_depth(depth, last_bit);
      printf("%s %d\n", curr_file->file_name, curr_file->backup_cnt);
      
      backupNode* curr_backup = curr_file->backup_head->next_backup;
      while(curr_backup != NULL) {
        print_depth(depth+1, last_bit);
        printf("%s\n", curr_backup->backup_time);
        curr_backup = curr_backup->next_backup;
      }
      curr_file = curr_file->next_file;
    }
  }
  
  while(curr_dir != NULL) {
    last_bit |= (curr_dir->next_dir == NULL)?(1 << depth):0;
    
    print_depth(depth, last_bit);
    printf("%s/ %d %d %d\n", curr_dir->dir_name, curr_dir->file_cnt,
      curr_dir->subdir_cnt, curr_dir->backup_cnt);
    print_list(curr_dir, depth+1, last_bit);
    curr_dir = curr_dir->next_dir;
  }
  
  while(curr_file != NULL) {
    last_bit |= (curr_file->next_file == NULL)?(1 << depth):0;

    print_depth(depth, last_bit);
    printf("%s %d\n", curr_file->file_name, curr_file->backup_cnt);
    
    backupNode* curr_backup = curr_file->backup_head->next_backup;
    while(curr_backup != NULL) {
      print_depth(depth+1, (
        last_bit | ((curr_backup->next_backup == NULL)?(1 << depth+1):0)
        ));
      printf("%s\n", curr_backup->backup_time);
      curr_backup = curr_backup->next_backup;
    }
    curr_file = curr_file->next_file;
  }
}

int init_backup_list(int log_fd) {
  int len;
  char buf[BUF_MAX];

  char *backup_time, *origin_path, *backup_path;

  backup_dir_list = (dirNode*)malloc(sizeof(dirNode));
  dirNode_init(backup_dir_list);

  while(len = read(log_fd, buf, BUF_MAX)) {
    char *ptr = strchr(buf, '\n');
    ptr[0] = '\0';

    lseek(log_fd, -(len-strlen(buf))+1, SEEK_CUR);

    if((ptr = strstr(buf, BACKUP_SEP)) != NULL) {
      backup_time = substr(buf, 0, 12);
      origin_path = substr(buf, 16, strlen(buf)-strlen(ptr));
      backup_path = substr(buf, strlen(buf)-strlen(ptr)+strlen(BACKUP_SEP), strlen(buf)-1);
      
      backup_list_insert(backup_dir_list, backup_time, origin_path, backup_path);
    }
    if((ptr = strstr(buf, REMOVE_SEP)) != NULL) {
      char *tmp_ptr = strstr(buf, BACKUP_PATH);
      backup_time = substr(buf, strlen(buf)-strlen(tmp_ptr)+strlen(BACKUP_PATH), strlen(buf)-strlen(tmp_ptr)+strlen(BACKUP_PATH)+12);
      backup_path = substr(buf, 16, strlen(buf)-strlen(ptr));
      origin_path = substr(buf, strlen(buf)-strlen(ptr)+strlen(REMOVE_SEP), strlen(buf)-1);
      
      backup_list_remove(backup_dir_list, backup_time, origin_path, backup_path);
    }
    if((ptr = strstr(buf, RECOVER_SEP)) != NULL) {
      char *tmp_ptr = strstr(buf, BACKUP_PATH);
      backup_time = substr(buf, strlen(buf)-strlen(tmp_ptr)+strlen(BACKUP_PATH), strlen(buf)-strlen(tmp_ptr)+strlen(BACKUP_PATH)+12);
      backup_path = substr(buf, 16, strlen(buf)-strlen(ptr));
      origin_path = substr(buf, strlen(buf)-strlen(ptr)+strlen(RECOVER_SEP), strlen(buf)-1);
      
      backup_list_remove(backup_dir_list, backup_time, origin_path, backup_path);
    }
  }

  // print_list(backup_dir_list, 0, 0);

  return 0;
}

int get_version_list(dirNode *version_dir_head) {
  int cnt;
  int ret = 0;
  struct dirent **namelist;
  struct stat statbuf;
  
  if((cnt = scandir(version_dir_head->dir_path, &namelist, NULL, alphasort)) == -1) {
    fprintf(stderr, "ERROR: scandir error for %s\n", version_dir_head->dir_path);
    return -1;
  }

  for(int i = 0; i < cnt; i++) {
    if(!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) continue;

    char tmp_path[PATH_MAX];
    strcpy(tmp_path, version_dir_head->dir_path);
    strcat(tmp_path, namelist[i]->d_name);

    if (lstat(tmp_path, &statbuf) < 0) {
      fprintf(stderr, "ERROR: lstat error for %s\n", tmp_path);
      return -1;
    }

    if(S_ISDIR(statbuf.st_mode)) {
      dirNode *version_subdir_node =
        dirNode_insert(version_dir_head->subdir_head, namelist[i]->d_name, version_dir_head->dir_path);
      ret |= get_version_list(version_subdir_node);
    }
  }
  return ret;
}

int link_backup_node(dirNode *dirList) {
  dirNode *curr_dir = dirList->subdir_head->next_dir;
  fileNode *curr_file = dirList->file_head->next_file;
  backupNode *curr_backup;
  dirNode *curr_version_dir;

  while(curr_dir != NULL) {
    link_backup_node(curr_dir);
    curr_dir = curr_dir->next_dir;
  }

  while(curr_file != NULL) {
    curr_backup = curr_file->backup_head->next_backup;
    while(curr_backup != NULL) {
      char *tmp_path = substr(curr_backup->backup_path, strlen(BACKUP_PATH), strlen(curr_backup->backup_path));
      char *ptr = strrchr(tmp_path, '/');
      ptr[0] = '\0';

      curr_version_dir = get_dirNode_from_path(version_dir_list, tmp_path);
      curr_backup->root_version_dir = curr_version_dir;
      add_cnt_root_dir(curr_backup->root_version_dir, 1);

      curr_backup = curr_backup->next_backup;
    }
    curr_file = curr_file->next_file;
  }
}

int init_version_list() {
  version_dir_list = (dirNode*)malloc(sizeof(dirNode));
  dirNode_init(version_dir_list);
  strcpy(version_dir_list->dir_path, BACKUP_PATH);
  get_version_list(version_dir_list);

  link_backup_node(backup_dir_list);

  // print_list(version_dir_list, 0, 0);
}

int init(char* path) {
  int log_fd;

  exe_path = path;
  pwd_path = (char *)getenv("PWD");

  if(access(BACKUP_PATH, F_OK)) {
    if(mkdir(BACKUP_PATH, 0755) == -1) {
      return -1;
    };
  }

  if((log_fd = open(BACKUP_LOG_PATH, O_RDWR|O_CREAT, 0777)) == -1) {
    fprintf(stderr, "ERROR: open error for '%s'\n", BACKUP_LOG_PATH);
    return -1;
  }

  init_backup_list(log_fd);
  init_version_list();

  close(log_fd);

  return 0;
}
