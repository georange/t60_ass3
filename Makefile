.phony all:
all : diskinfo 
#prog02 prog03

diskinfo : diskinfo.c
    gcc -Wall diskinfo.c -o diskinfo

#prog02 : m.c n.c
#    gcc -o prog02 m.c n.c

#prog03 : u.c v.c w.c
#    gcc -o prog03 u.c v.c w.c
	 
.PHONY clean:
clean:
	-rm -rf *.o *.exe