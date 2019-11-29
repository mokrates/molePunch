etags.emacs *.c *.h
gcc molepunchserver.c molepunch_common.c -o molepunchserver
gcc molepunchclient.c molepunch_common.c -o molepunchclient
