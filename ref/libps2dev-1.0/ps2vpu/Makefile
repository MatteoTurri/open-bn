TARGET = all

SRCS = ps2vpu.c
PUBLIC_HEADERS = ps2vpu.h
PUBLIC_DOCS = ps2vpu_jp.txt \
	ps2vpu_en.txt

include ../mk/lib-def.mk

${TARGET}:  ${OBJS}

include ../mk/lib-rule.mk

install:  install_objs install_heads install_docs

