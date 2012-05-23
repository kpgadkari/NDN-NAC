SUBDIR = inform nac
export CCNX_DIR = /Users/fan/NDN-NAC/ccnx-0.6.0
all:
	for i in ${SUBDIR}; do\
		echo "make" $$i : `pwd`/$$i;\
		cd $$i;\
		make;\
		cd ..;\
	done
clean:
	for i in ${SUBDIR}; do\
		echo "clean" $$i : `pwd`/$$i;\
		cd $$i;\
		make clean;\
		cd ..;\
	done
