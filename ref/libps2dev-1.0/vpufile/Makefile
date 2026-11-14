TARGET = all

SRCS = vpu_file.c
PUBLIC_HEADERS = ps2vpufile.h
PUBLIC_DOCS = vpu_file_jp.txt \
		vpu_file_en.txt

LD_SCRIPT =  vpu.cmd
INCLUDES = -I../ps2mem

include ../mk/lib-def.mk

${TARGET}:  ${OBJS}

include ../mk/lib-rule.mk

install:  install_objs install_heads install_docs install_ldscript

install_ldscript:
	test -d ${OBJSDIR}/ldscripts || \
		mkdir ${OBJSDIR}/ldscripts
	cp -p ${LD_SCRIPT} ${OBJSDIR}/ldscripts/${LD_SCRIPT}


