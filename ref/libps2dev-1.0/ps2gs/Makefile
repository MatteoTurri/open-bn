
TARGET = all

SRCS = \
	gs_alpha.c \
	gs_clear.c \
	gs_dbuff.c \
	gs_disp.c \
	gs_draw.c \
	gs_gparam.c \
	gs_image.c \
	gs_internal.c \
	gs_open.c \
	gs_reset.c \
	gs_tex.c \
	gs_vc.c

PUBLIC_HEADERS = gs_vc.h ps2gs.h
PUBLIC_DOCS = ps2gs_jp.txt ps2gs_en.txt

INCLUDES = -I../dma -I../vpufile

include ../mk/lib-def.mk

${TARGET}:  ${OBJS}

include ../mk/lib-rule.mk

install:  install_objs install_heads install_docs

