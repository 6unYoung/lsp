CC = gcc

HEADER = ssu_header
BACKUP = ssu_backup
HELP = ssu_help
INIT = ssu_init
STRUCT = ssu_struct

$(BACKUP) : $(BACKUP).o $(HELP).o $(HEADER).o $(INIT).o $(STRUCT).o
	$(CC) -g -o $@ $^ -lcrypto

$(BACKUP).o : $(BACKUP).c
	$(CC) -c -o $@ $^ -lcrypto

$(HELP).o : $(HELP).c
	$(CC) -c -o $@ $^ -lcrypto

$(HEADER).o : $(HEADER).c
	$(CC) -c -o $@ $^ -lcrypto

$(INIT).o : $(INIT).c
	$(CC) -c -o $@ $^ -lcrypto

$(STRUCT).o : $(STRUCT).c
	$(CC) -c -o $@ $^ -lcrypto

clean :
	rm -rf $(BACKUP)
	rm -rf *.o
