tpm2xx:	tpm2xx.c
	gcc `pkg-config --cflags libmodbus` tpm2xx.c `pkg-config --libs libmodbus` -lm -g -o tpm2xx

