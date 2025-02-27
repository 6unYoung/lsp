#include "ssu_header.h"

dirNode *get_dirNode_from_path(dirNode *dirList, char *path) {
  char *ptr;

  if(dirList == NULL) return NULL;
  
  if(ptr = strchr(path, '/')) {
    char *dir_name = substr(path, 0, strlen(path) - strlen(ptr));
    dirNode* curr_dir = dirNode_get(dirList->subdir_head, dir_name);
    return get_dirNode_from_path(curr_dir, ptr+1);
  } else {
    char *dir_name = path;
    dirNode* curr_dir = dirNode_get(dirList->subdir_head, dir_name);
    return curr_dir;
  }
}

fileNode *get_fileNode_from_path(dirNode *dirList, char *path) {
  char *ptr;
  dirNode *curr_dir;
  fileNode *curr_file;

  ptr = strrchr(path, '/');

  if((curr_dir = get_dirNode_from_path(dirList, substr(path, 0, strlen(path)-strlen(ptr)))) == NULL) return NULL;
  curr_file = fileNode_get(curr_dir->file_head, ptr+1);
  return curr_file;
}

backupNode *get_backupNode_from_path(dirNode *dirList, char *path, char *backup_time) {
  fileNode *curr_file;
  backupNode *curr_backup;

  if((curr_file = get_fileNode_from_path(dirList, path)) == NULL) return NULL;
  curr_backup = backupNode_get(curr_file->backup_head, backup_time);
  return curr_backup;
}


int check_path_access(char* path) {
  int i;
  int cnt;
  char *origin_path = (char*)malloc(sizeof(char)*(strlen(path)+1));
  char* ptr;
  int depth = 0;
  
  strcpy(origin_path, path);

  while(ptr = strchr(origin_path, '/')) {
    char *tmp_path = substr(origin_path, 0, strlen(origin_path) - strlen(ptr));

    depth++;
    origin_path = ptr+1;

    if(depth == 2 && strcmp(substr(path, 0, strlen(path) - strlen(origin_path)), "/home/")) {
      fprintf(stderr, "ERROR: path must be in user directory\n");
      fprintf(stderr, " - '%s' is not in user directory\n", path);
      return -1;
    }
    
    if(depth == 3 && !strcmp(substr(path, 0, strlen(path) - strlen(origin_path)), BACKUP_PATH)) {
      fprintf(stderr, "ERROR: path must be in user directory\n");
      fprintf(stderr, " - '%s' is not in user directory\n", path);
      return -1;
    }
  }

  if(depth < 3) {
    fprintf(stderr, "ERROR: path must be in user directory\n");
    fprintf(stderr, " - '%s' is not in user directory\n", path);
    return -1;
  }

  return 0;
}

void get_backup_info(dirNode* dirList, char* path, int *is_backuped_file, int *is_backuped_dir) {
  char* ptr;

  if(ptr = strchr(path, '/')) {
    char* dir_name = substr(path, 0, strlen(path) - strlen(ptr));
    
    dirNode* curr_dir = dirList->subdir_head;
    dirNode* next_dir;

    while(curr_dir->next_dir != NULL) {
      if(!strcmp(dir_name, curr_dir->next_dir->dir_name)) {
        break;
      }
      curr_dir = curr_dir->next_dir;
    }

    if(curr_dir->next_dir == NULL) {
      return;
    }

    next_dir = curr_dir->next_dir;
    get_backup_info(next_dir, ptr+1, is_backuped_file, is_backuped_dir);
  } else {
    char* tmp_name = path;

    fileNode* curr_file = dirList->file_head;
    dirNode* curr_dir = dirList->subdir_head;
    
    while(curr_file->next_file != NULL) {
      if(!strcmp(tmp_name, curr_file->next_file->file_name)) {
        *is_backuped_file = 1;
        break;
      }
      curr_file = curr_file->next_file;
    }
    
    while(curr_dir->next_dir != NULL) {
      if(!strcmp(tmp_name, curr_dir->next_dir->dir_name)) {
        *is_backuped_dir = 1;
        break;
      }
      curr_dir = curr_dir->next_dir;
    }
  }

  return;
}

int check_option(int argc, char *argv[], char *path, int cmd_bit, char **new_path) {
  int i;
  int opt_bit = 0;
  struct stat statbuf;

  int is_backuped_file = 0;
  int is_backuped_dir = 0;
  get_backup_info(backup_dir_list, path, &is_backuped_file, &is_backuped_dir);

  for(i = 0; i < argc; i++) {
    if(!strcmp(argv[i], "-d") && (cmd_bit & (CMD_BACKUP | CMD_REMOVE | CMD_RECOVER))) opt_bit |= OPT_D;
    else if(!strcmp(argv[i], "-r") && (cmd_bit & (CMD_BACKUP | CMD_REMOVE | CMD_RECOVER))) opt_bit |= OPT_R;
    else if(!strcmp(argv[i], "-y") && (cmd_bit & CMD_BACKUP)) opt_bit |= OPT_Y;
    else if(!strcmp(argv[i], "-a") && (cmd_bit & CMD_REMOVE)) opt_bit |= OPT_A;
    else if(!strcmp(argv[i], "-l") && (cmd_bit & CMD_RECOVER)) opt_bit |= OPT_L;
    else if(!strcmp(argv[i], "-n") && (cmd_bit & CMD_RECOVER)) {
      if(i+1 == argc) {
        fprintf(stderr, "ERROR: missing operand -- '%s'\n", argv[i]);
        fprintf(stderr, "ERROR: '%s' option needs <NEW_PATH>\n", argv[i]);
        return -1;
      }

      if((*new_path = cvt_path_2_realpath(argv[i+1])) == NULL) {
        fprintf(stderr, "ERROR: wrong new path\n");
        return -1;
      }

      if(check_path_access(*new_path)) {
        return -1;
      }
      opt_bit |= OPT_N;
      i++;
    }
    else {
      fprintf(stderr, "ERROR: invalid option -- '%s'\n", argv[i]);
      help(cmd_bit);
      return -1;
    }
  }

  if(cmd_bit & CMD_BACKUP && stat(path, &statbuf) == -1) {
    fprintf(stderr, "ERROR: stat error for '%s'\n", path);
    return -1;
  }

  if (opt_bit & (OPT_D | OPT_R)) {
    if(cmd_bit & CMD_BACKUP && S_ISREG(statbuf.st_mode)) {
      fprintf(stderr, "ERROR: '%s' is file.\n", path);
      fprintf(stderr, " - dont use '-d' option for file path or input directory path.\n");
      return -1;
    } else if(cmd_bit & (CMD_REMOVE | CMD_RECOVER) && get_dirNode_from_path(backup_dir_list, path) == NULL) {
      fprintf(stderr, "ERROR: can't find backup directory for '%s'.\n", path);
      fprintf(stderr, " - dont use '-d' option for file path or input directory path.\n");
      return -1;
    }
  }
  else {
    if(cmd_bit & CMD_BACKUP && S_ISDIR(statbuf.st_mode)) {
      fprintf(stderr, "ERROR: '%s' is directory.\n", path);
      fprintf(stderr, " - use '-d' option for directory path or input file path.\n");
      return -1;
    } else if(cmd_bit & (CMD_REMOVE | CMD_RECOVER) && get_fileNode_from_path(backup_dir_list, path) == NULL) {
      fprintf(stderr, "ERROR: can't find backup file for '%s'.\n", path);
      fprintf(stderr, " - use '-d' option for directory path or input file path.\n");
      return -1;
    }
  }

  return opt_bit;
}

int check_already_backuped(char* path) {
  char* tmp_path = (char*)malloc(sizeof(char)*PATH_MAX);
  char* ptr;
  dirNode* curr_dir;
  fileNode* curr_file;
  backupNode* curr_backup;

  strcpy(tmp_path, path);
  curr_dir = backup_dir_list->subdir_head;

  while((ptr = strchr(tmp_path, '/')) != NULL) {
    char* dir_name = substr(tmp_path, 0, strlen(tmp_path) - strlen(ptr));
    while(curr_dir->next_dir != NULL) {
      if(!strcmp(dir_name, curr_dir->next_dir->dir_name)) {
        break;
      }
      curr_dir = curr_dir->next_dir;
    }

    if(curr_dir->next_dir == NULL) {
      return 0;
    }

    tmp_path = ptr + 1;
    if(strchr(tmp_path, '/')) {
      curr_dir = curr_dir->next_dir->subdir_head;
    }
  }

  curr_file = curr_dir->next_dir->file_head;
  while(curr_file->next_file != NULL) {
    if(!strcmp(tmp_path, curr_file->next_file->file_name)) {
      break;
    }
    curr_file = curr_file->next_file;
  }

  if(curr_file->next_file == NULL) {
    return 0;
  }

  curr_backup = curr_file->next_file->backup_head;

  while(curr_backup->next_backup != NULL) {
    if(!cmpHash(path, curr_backup->next_backup->backup_path)) {
      printf("\"%s\" already backuped to \"%s\"\n", path, curr_backup->next_backup->backup_path);
      return 1;
    }
    curr_backup = curr_backup->next_backup;
  }

  return 0;
}

int backup_file(char* origin_path, char* backup_path, char* time_str, int opt_bit, int log_fd) {
  int fd1, fd2;
  int len;
  char buf[BUF_MAX];
  char backup_file_path[PATH_MAX];

  //check already backuped
  if(!(opt_bit & OPT_Y) && check_already_backuped(origin_path)) {
    return 0;
  }

  sprintf(backup_file_path, "%s%s", backup_path, get_file_name(origin_path));

  if((fd1 = open(origin_path, O_RDONLY)) == -1) {
    fprintf(stderr, "ERROR: open error for '%s'\n", origin_path);
    return -1;
  }

  if((fd2 = open(backup_file_path, O_WRONLY|O_CREAT|O_TRUNC, 0777)) == -1) {
    fprintf(stderr, "ERROR: open error for '%s'\n", backup_file_path);
    return -1;
  }

  while(len = read(fd1, buf, BUF_MAX)) {
    write(fd2, buf, len);
  }
  
  close(fd1);
  close(fd2);

  sprintf(buf, "%s : \"%s\" backuped to \"%s\"\n", time_str, origin_path, c_str(backup_file_path));
  write(log_fd, buf, strlen(buf));
  printf("\"%s\" backuped to \"%s\"\n", origin_path, backup_file_path);
  
  return 1;
}

//add to be empty directory not backup
int backup_dir(char* origin_path, char* backup_path, char* time_str, int opt_bit, int log_fd) {
  struct dirent **namelist;
  struct stat statbuf;
  dirNode *dirList;
  dirNode *curr_dir;
  char sub_backup_path[PATH_MAX];
  int cnt;
  int sub_backup_cnt = 0;
  int backup_cnt = 0;

  dirList = (dirNode*)malloc(sizeof(dirNode));
  dirNode_init(dirList);
  dirNode_append(dirList, "", "");

  curr_dir = dirList->next_dir;

  while(curr_dir != NULL) {
    char origin_dir_path[PATH_MAX];
    sprintf(origin_dir_path, "%s%s", origin_path, c_str(curr_dir->dir_name));

    if((cnt = scandir(origin_dir_path, &namelist, NULL, alphasort)) == -1) {
      fprintf(stderr, "ERROR: scandir error for %s\n", origin_dir_path);
      return -1;
    }

    for(int i = 0; i < cnt; i++) {
      if(!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) continue;

      char tmp_path[PATH_MAX];
      sprintf(tmp_path, "%s%s", c_str(origin_dir_path), namelist[i]->d_name);
      if (lstat(tmp_path, &statbuf) < 0) {
        fprintf(stderr, "ERROR: lstat error for %s\n", tmp_path);
        return -1;
      }

      if(S_ISREG(statbuf.st_mode)) {
        char new_backup_path[PATH_MAX];
        sprintf(new_backup_path, "%s%s", backup_path, c_str(curr_dir->dir_name));

        if((sub_backup_cnt = backup_file(tmp_path, new_backup_path, time_str, opt_bit, log_fd)) == -1) {
          return -1;
        };
        backup_cnt += sub_backup_cnt;
      } else if(opt_bit & OPT_R && S_ISDIR(statbuf.st_mode)) {

        sprintf(sub_backup_path, "%s%s%s", backup_path, c_str(curr_dir->dir_name), namelist[i]->d_name);
        if(mkdir(sub_backup_path, 0755) == -1) {
          fprintf(stderr, "ERROR: mkdir error for '%s'\n", sub_backup_path);
          return -1;
        };

        // if((sub_backup_cnt = backup_dir(tmp_path, sub_backup_path, time_str, opt_bit, log_fd)) == -1) {
        //   return -1;
        // };
        
        // if(sub_backup_cnt == 0 && rmdir(sub_backup_path)) {
        //   fprintf(stderr, "ERROR: rmdir error for '%s'\n", sub_backup_path);
        //   return -1;
        // }
        
        char new_dir_path[PATH_MAX];
        sprintf(new_dir_path, "%s%s/", c_str(curr_dir->dir_name), namelist[i]->d_name);
        dirNode_append(dirList, new_dir_path, "");

        backup_cnt += sub_backup_cnt;
      }
    }
    curr_dir = curr_dir->next_dir;
  }

  return backup_cnt;
}

int backup_process(int argc, char* argv[]) {
  int backup_cnt = 0;
  int opt_bit;
  int log_fd;
  char *origin_path;
  char new_backup_dir_path[PATH_MAX];
  char *now_time_str = cvt_time_2_str(time(NULL));
경로를 입력하지 않을 경우, add 명령어에 대한 에러 처리 및 Usage 출력 후 프롬프트 재출력
- 인자로 입력받은 경로가 길이 제한(4,096 Byte)를 넘거나 경로가 올바르지 않은 경우, 에러 처리 후 프롬프트 재출력
- 인자로 입력받은 경로가 사용자 홈 디렉토리 내의 경로(사용자 홈 디렉토리 제외)가 아니거나 백업 디렉토리를 포함하는 경로일 경우, 에러 처리 후 프롬프트 재출력
- 인자로 입력받은 옵션 중 ­d, -r을 사용하지 않았는데, 인자로 입력받은 경로가 디렉토리일 경우, 에러 처리 후 프롬프트 재출력
- 인자로 입력받은 옵션 중 ­d, -r을 사용하였는데, 인자로 입력받은 경로가 파일일 경우, 에러 처리 후 프롬프트 재출력
  if(argc < 1) {
    fprintf(stderr, "ERROR: <PATH> is not include.\n");
    help(CMD_BACKUP);
    return -1;
  }

  if((origin_path = cvt_path_2_realpath(argv[0])) == NULL) {
    fprintf(stderr, "ERROR: wrong path\n");
    return -1;
  }

  if(check_path_access(origin_path)) {
    return -1;
  }

  if(access(origin_path, F_OK)) {
    fprintf(stderr, "ERROR: <PATH> is not exist.\n");
    return -1;
  }
  
  if((opt_bit = check_option(argc-1, argv+1, origin_path, CMD_BACKUP, NULL)) == -1) {
    return -1;
  }

  sprintf(new_backup_dir_path, "%s%s/", BACKUP_PATH, now_time_str);
  if(mkdir(new_backup_dir_path, 0755) == -1) {
    fprintf(stderr, "ERROR: mkdir error for '%s'\n", new_backup_dir_path);
    return -1;
  };

  if((log_fd = open(BACKUP_LOG_PATH, O_WRONLY|O_CREAT|O_APPEND, 0777)) == -1) {
    fprintf(stderr, "ERROR: open error for '%s'\n", BACKUP_LOG_PATH);
    return -1;
  }

  if(!(opt_bit & (OPT_D | OPT_R))) {
    backup_cnt = backup_file(origin_path, new_backup_dir_path, now_time_str, opt_bit, log_fd);
  } else {
    strcat(origin_path, "/");
    backup_cnt = backup_dir(origin_path, new_backup_dir_path, now_time_str, opt_bit, log_fd);
  }

  if(backup_cnt == 0 && rmdir(new_backup_dir_path)) {
    fprintf(stderr, "ERROR: rmdir error for '%s'\n", new_backup_dir_path);
    return -1;
  }

  close(log_fd);

  return 0;
}


int check_root_dir(dirNode *dir_node) {
  if(dir_node->backup_cnt != 0 || !strcmp(dir_node->dir_path, BACKUP_PATH)) {
    return 0;
  }

  if(rmdir(dir_node->dir_path)) {
    fprintf(stderr, "ERROR: rmdir error for '%s'\n", dir_node->dir_path);
    return -1;
  }
  return check_root_dir(dir_node->root_dir);
}

int add_cnt_root_dir(dirNode *dir_node, int cnt) {
  if(dir_node == NULL) return 0;
  dir_node->backup_cnt += cnt;
  return add_cnt_root_dir(dir_node->root_dir, cnt);
}


backupNode *backup_select_prompt(fileNode* curr_file) {
  int idx;
  char input[BUF_MAX];
  int select;
  struct stat statbuf;
  backupNode *curr_backup;
  backupNode *del_backup;

  printf("backup files of %s\n", curr_file->file_path);
  printf("0. exit\n");
  curr_backup = curr_file->backup_head;
  idx = 1;
  while(curr_backup->next_backup != NULL) {
    if (lstat(curr_backup->next_backup->backup_path, &statbuf) < 0) {
      fprintf(stderr, "ERROR: lstat error for %s\n", curr_backup->next_backup->backup_path);
      return NULL;
    }
    // 천단위 마침표
    printf("%d. %s\t\t%ldbytes\n", idx, curr_backup->next_backup->backup_time, statbuf.st_size);
    curr_backup = curr_backup->next_backup;
    idx++;
  }

  printf(">> ");
  fgets(input, sizeof(input), stdin);
  input[strlen(input) - 1] = '\0';
  select = atoi(input);
  printf("%d\n", select);

  if(select < 1 || select > idx) return NULL;

  idx = 1;
  del_backup = curr_file->backup_head;
  while(del_backup->next_backup != NULL) {
    if(idx == select) {
      del_backup = del_backup->next_backup;
      break;
    }
    del_backup = del_backup->next_backup;
    idx++;
  }

  return del_backup;
}


int remove_backup_file(backupNode *del_backup, char* time_str, int log_fd) {
  char buf[BUF_MAX];
  char *remove_time = time_str;
  char *backup_path = del_backup->backup_path;
  char *origin_path = del_backup->origin_path;
  
  if(remove(backup_path)) {
    fprintf(stderr, "ERROR: remove error for %s\n", backup_path);
    return -1;
  }

  add_cnt_root_dir(del_backup->root_version_dir, -1);
  check_root_dir(del_backup->root_version_dir);

  sprintf(buf, "%s : \"%s\" removed by \"%s\"\n", remove_time, backup_path, origin_path);
  write(log_fd, buf, strlen(buf));
  printf("\"%s\" removed by \"%s\"\n", backup_path, origin_path);

  return 0;
}

int remove_file(fileNode* curr_file, char* time_str, int opt_bit, int log_fd) {
  backupNode *curr_backup;
  backupNode *del_backup;

  if(curr_file->backup_cnt > 1) {
    if(opt_bit & OPT_A) {
      curr_backup = curr_file->backup_head;
      while(curr_backup->next_backup != NULL) {
        del_backup = curr_backup->next_backup;
        remove_backup_file(del_backup, time_str, log_fd);
        curr_backup = curr_backup->next_backup;
      }
    } else {
      if((del_backup = backup_select_prompt(curr_file))  == NULL) {
        return -1;
      };
      remove_backup_file(del_backup, time_str, log_fd);
    }
  } else {
    del_backup = curr_file->backup_head->next_backup;
    remove_backup_file(del_backup, time_str, log_fd);
  }
}

int remove_dir(dirNode* dir_root, char* time_str, int opt_bit, int log_fd) {
  fileNode* curr_file = dir_root->file_head;
  dirNode* curr_dir = dir_root->subdir_head;

  while(curr_file->next_file != NULL) {
    remove_file(curr_file->next_file, time_str, opt_bit, log_fd);
    curr_file = curr_file->next_file;
  }

  if(opt_bit & OPT_R) {
    while(curr_dir->next_dir != NULL) {
      remove_dir(curr_dir->next_dir, time_str, opt_bit, log_fd);
      curr_dir = curr_dir->next_dir;
    }
  }
}

int remove_process(int argc, char* argv[]) {
  int backup_cnt = 0;
  int opt_bit;
  int log_fd;
  char *origin_path;
  char *now_time_str = cvt_time_2_str(time(NULL));

  if(argc < 1) {
    fprintf(stderr, "ERROR: <PATH> is not include.\n");
    help(CMD_REMOVE);
    return -1;
  }

  if((origin_path = cvt_path_2_realpath(argv[0])) == NULL) {
    fprintf(stderr, "ERROR: wrong path\n");
    return -1;
  }

  if(check_path_access(origin_path)) {
    return -1;
  }

  // backup path
  if((opt_bit = check_option(argc-1, argv+1, origin_path, CMD_REMOVE, NULL)) == -1) {
    return -1;
  }

  if((log_fd = open(BACKUP_LOG_PATH, O_WRONLY|O_CREAT|O_APPEND, 0777)) == -1) {
    fprintf(stderr, "ERROR: open error for '%s'\n", BACKUP_LOG_PATH);
    return -1;
  }

  if(!(opt_bit & (OPT_D | OPT_R))) {
    remove_file(get_fileNode_from_path(backup_dir_list, origin_path), now_time_str, opt_bit, log_fd);
  } else {
    remove_dir(get_dirNode_from_path(backup_dir_list, origin_path), now_time_str, opt_bit, log_fd);
  }

  close(log_fd);

  return 0;
}


int recover_backup_file(backupNode *backup_node, char *dest_path, char* time_str, int log_fd) {
  int fd1, fd2;
  int len;
  char buf[BUF_MAX];
  char *recover_time = time_str;
  char *backup_path = backup_node->backup_path;
  char *origin_path = backup_node->origin_path;
  
  if((fd1 = open(backup_path, O_RDONLY)) == -1) {
    fprintf(stderr, "ERROR: open error for '%s'\n", backup_path);
    return -1;
  }

  if((fd2 = open(dest_path, O_WRONLY|O_CREAT|O_TRUNC, 0777)) == -1) {
    fprintf(stderr, "ERROR: open error for '%s'\n", dest_path);
    return -1;
  }
  
  while(len = read(fd1, buf, BUF_MAX)) {
    write(fd2, buf, len);
  }
  
  close(fd1);
  close(fd2);

  if(remove(backup_path)) {
    fprintf(stderr, "ERROR: remove error for %s\n", backup_path);
    return -1;
  }

  add_cnt_root_dir(backup_node->root_version_dir, -1);
  check_root_dir(backup_node->root_version_dir);

  sprintf(buf, "%s : \"%s\" recovered to \"%s\"\n", recover_time, backup_path, dest_path);
  write(log_fd, buf, strlen(buf));
  printf("\"%s\" recovered to \"%s\"\n", backup_path, dest_path);

  return 0;
}

int recover_file(fileNode *curr_file, char *dest_path, char *time_str, int opt_bit, int log_fd) {
  backupNode *curr_backup;
  backupNode *backup_node;

  //check already backuped
  // if(!(opt_bit & OPT_Y) && check_already_backuped(origin_path)) {
  //   return 0;
  // }

  if(curr_file->backup_cnt > 1) {
    //L 옵션
    if(opt_bit & OPT_L) {
      curr_backup = curr_file->backup_head->next_backup;
      while(curr_backup->next_backup != NULL) {
        curr_backup = curr_backup->next_backup;
      }
      recover_backup_file(curr_backup, dest_path, time_str, log_fd);
    } else {
      if((backup_node = backup_select_prompt(curr_file))  == NULL) {
        return -1;
      };
      recover_backup_file(backup_node, dest_path, time_str, log_fd);
    }
  } else {
    backup_node = curr_file->backup_head->next_backup;
    recover_backup_file(backup_node, dest_path, time_str, log_fd);
  }
}

int recover_dir(dirNode* dir_root, char *dest_path, char* time_str, int opt_bit, int log_fd) {
  fileNode* curr_file = dir_root->file_head;
  dirNode* curr_dir = dir_root->subdir_head;
  struct stat statbuf;
  char next_dest[PATH_MAX];

  while(curr_file->next_file != NULL) {
    sprintf(next_dest, "%s/%s", dest_path, curr_file->next_file->file_name);
    recover_file(curr_file->next_file, next_dest, time_str, opt_bit, log_fd);
    curr_file = curr_file->next_file;
  }

  if(opt_bit & OPT_R) {
    while(curr_dir->next_dir != NULL) {
      sprintf(next_dest, "%s/%s", dest_path, curr_dir->next_dir->dir_name);

      if(!access(next_dest, F_OK)) {
        if (lstat(next_dest, &statbuf) < 0) {
          fprintf(stderr, "ERROR: lstat error for %s\n", next_dest);
          return -1;
        }

        if(!S_ISDIR(statbuf.st_mode)) {
          fprintf(stderr, "ERROR: %s is already exist as file\n", next_dest);
          return -1;
        }
      } else {
        if(mkdir(next_dest, 0755) == -1) {
          fprintf(stderr, "ERROR: mkdir error for '%s'\n", next_dest);
          return -1;
        };
      }

      recover_dir(curr_dir->next_dir, next_dest, time_str, opt_bit, log_fd);
      curr_dir = curr_dir->next_dir;
    }
  }
}

int recover_process(int argc, char* argv[]) {
  int backup_cnt = 0;
  int opt_bit;
  int log_fd;
  char *origin_path;
  char *new_origin_path;
  char *now_time_str = cvt_time_2_str(time(NULL));

  if(argc < 1) {
    fprintf(stderr, "ERROR: <PATH> is not include.\n");
    help(CMD_REMOVE);
    return -1;
  }

  if((origin_path = cvt_path_2_realpath(argv[0])) == NULL) {
    fprintf(stderr, "ERROR: wrong path\n");
    return -1;
  }

  new_origin_path = c_str(origin_path);

  if(check_path_access(origin_path)) {
    return -1;
  }

  // backup path
  if((opt_bit = check_option(argc-1, argv+1, origin_path, CMD_RECOVER, &new_origin_path)) == -1) {
    return -1;
  }

  if((log_fd = open(BACKUP_LOG_PATH, O_WRONLY|O_CREAT|O_APPEND, 0777)) == -1) {
    fprintf(stderr, "ERROR: open error for '%s'\n", BACKUP_LOG_PATH);
    return -1;
  }


  if(!(opt_bit & (OPT_D | OPT_R))) {
    if(make_dir_path(substr(new_origin_path, 0, strlen(new_origin_path) - strlen(strrchr(new_origin_path, '/')))) == -1) {
      return -1;
    };
    recover_file(get_fileNode_from_path(backup_dir_list, origin_path),
      new_origin_path, now_time_str, opt_bit, log_fd);
  } else {
    if(make_dir_path(new_origin_path) == -1) {
      return -1;
    };
    recover_dir(get_dirNode_from_path(backup_dir_list, origin_path),
      new_origin_path, now_time_str, opt_bit, log_fd);
  }

  close(log_fd);

  return 0;
}

int main(int argc, char* argv[]) {

  //파일이 변하면 백업 -> 크기 비교 후, 스탯 비교, 해시값 비교..?

  //백업 폴더 위치 : 사용자 홈 디렉터리 바로 아래 ssu_bak
  //백업 파일 이름 : 기존 경로 + 이름 + 확장자 + 백업시간(YYMMDDhhmmss) + .ssubak
  
  //파일 이동 추적?

  //레포지토리 set, add, push, log, pull, 
  //add : -
  //add시 새로운 폴더면 확인 질문
  //레포지토리 log
  //diff
  //
  if(init(argv[0]) == -1) {
    fprintf(stderr, "ERROR: init error.\n");
    return -1;
  }
  
  if(argc < 2) {
    fprintf(stderr, "ERROR: wrong input.\n");
    fprintf(stderr, "%s help : show commands for program\n", exe_path);
    return -1;
  }

  if(!strcmp(argv[1], "backup")) return backup_process(argc-2, argv+2);
  else if(!strcmp(argv[1], "remove")) return remove_process(argc-2, argv+2);
  else if(!strcmp(argv[1], "recover")) return recover_process(argc-2, argv+2);
  else if(!strcmp(argv[1], "list")) {

  } else if(!strcmp(argv[1], "help")) return help_process(argc-2, argv+2);
  else {
    fprintf(stderr, "ERROR: invalid command -- '%s'\n", argv[1]);
    fprintf(stderr, "%s help : show commands for program\n", exe_path);
    return -1;
  }  

  return 0;
}