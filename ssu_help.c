#include "ssu_header.h"

void help(int cmd_bit) {
  if(!cmd_bit) {
    printf("Usage: \n");
  }

  if(!cmd_bit || cmd_bit & CMD_BACKUP) {
    printf("%s backup <PATH> [OPTION]... : backup file if <PATH> is file\n", (cmd_bit?"Usage:":"  >"));
    printf("%s  -d : backup files in directory if <PATH> is directory\n", (!cmd_bit?"  ":""));
    printf("%s  -r : backup files in directory recursive if <PATH> is directory\n", (!cmd_bit?"  ":""));
    printf("%s  -y : backup file although already backuped\n", (!cmd_bit?"  ":""));
  }

  if(!cmd_bit || cmd_bit & CMD_REMOVE) {
    printf("%s remove <PATH> [OPTION]... : remove backuped file if <PATH> is file\n", (cmd_bit?"Usage:":"  >"));
    printf("%s  -d : remove backuped files in directory if <PATH> is directory\n", (!cmd_bit?"  ":""));
    printf("%s  -r : remove backuped files in directory recursive if <PATH> is directory\n", (!cmd_bit?"  ":""));
    printf("%s  -a : remove all backuped files\n", (!cmd_bit?"  ":""));
  }

  if(!cmd_bit || cmd_bit & CMD_RECOVER) {
    printf("%s recover <PATH> [OPTION]... : recover backuped file if <PATH> is file\n", (cmd_bit?"Usage:":"  >"));
    printf("%s  -d : recover backuped files in directory if <PATH> is directory\n", (!cmd_bit?"  ":""));
    printf("%s  -r : recover backuped files in directory recursive if <PATH> is directory\n", (!cmd_bit?"  ":""));
    printf("%s  -l : recover latest backuped file\n", (!cmd_bit?"  ":""));
    printf("%s  -n <NEW_PATH> : recover backuped file with new path\n", (!cmd_bit?"  ":""));
  }

  if(!cmd_bit || cmd_bit & CMD_LIST) {
    printf("%s list [PATH] : show backup list by directory structure\n", (cmd_bit?"Usage:":"  >"));
    printf("%s  >> rm <INDEX> [OPTION]... : remove backuped files of [INDEX] with [OPTION]\n", (!cmd_bit?"  ":""));
    printf("%s  >> rc <INDEX> [OPTION]... : recover backuped files of [INDEX] with [OPTION]\n", (!cmd_bit?"  ":""));
    printf("%s  >> vi(m) <INDEX> : edit original file of [INDEX]\n", (!cmd_bit?"  ":""));
    printf("%s  >> exit : exit program\n", (!cmd_bit?"  ":""));
  }

  if(!cmd_bit || cmd_bit & CMD_HELP) {
    printf("%s help [COMMAND] : show commands for program\n", (cmd_bit?"Usage:":"  >"));
  }
}

int help_process(int argc, char* argv[]) {

  if(argc == 0) {
    help(0);
  } else if(!strcmp(argv[0], "backup")) {
    help(CMD_BACKUP);
  } else if(!strcmp(argv[0], "remove")) {
    help(CMD_REMOVE);
  } else if(!strcmp(argv[0], "recover")) {
    help(CMD_RECOVER);
  } else if(!strcmp(argv[0], "list")) {
    help(CMD_LIST);
  } else if(!strcmp(argv[0], "help")) {
    help(CMD_HELP);
  } else {
    fprintf(stderr, "ERROR: invalid command -- '%s'\n", argv[0]);
    fprintf(stderr, "%s help : show commands for program\n", exe_path);
    return -1;
  }

  return 0;
}
