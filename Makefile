tpm2xx-dump:	tpm2xx-dump.c
	gcc `pkg-config --cflags libmodbus` tpm2xx-dump.c `pkg-config --libs libmodbus` -g -o tpm2xx-dump

