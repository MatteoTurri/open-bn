TARGET = all

SRCS = dmalib.c
PUBLIC_HEADERS = ps2dma.h
PUBLIC_DOCS = ps2dma_jp.txt
INCLUDES =  -I../vpufile

include ../mk/lib-def.mk

${TARGET}:  ${OBJS}

include ../mk/lib-rule.mk

install:  install_objs install_heads install_docs

