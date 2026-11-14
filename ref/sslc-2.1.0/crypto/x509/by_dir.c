/* crypto/x509/by_dir.c */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 * See crypto/README for full SSLeay licensing terms.
 */

#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "cryptlib.h"
#include "lhash.h"
#include "x509.h"


typedef struct lookup_dir_st
	{
	BUF_MEM *buffer;
	int num_dirs;
	char **dirs;
	int *dirs_type;
	int num_dirs_alloced;
	} BY_DIR;

static int new_dir();
static void free_dir();
static int dir_ctrl();
static int add_cert_dir();
static int get_cert_by_subject();

static X509_LOOKUP_METHOD x509_dir_lookup=
	{
	"Load certs from files in a directory",
	new_dir,		/* new */
	free_dir,		/* free */
	NULL, 			/* init */
	NULL,			/* shutdown */
	dir_ctrl,		/* ctrl */
	get_cert_by_subject,	/* get_by_subject */
	NULL,			/* get_by_issuer_serial */
	NULL,			/* get_by_fingerprint */
	NULL,			/* get_by_alias */
	};

X509_LOOKUP_METHOD *X509_LOOKUP_hash_dir()
	{
	return(&x509_dir_lookup);
	}

static int dir_ctrl(ctx,cmd,argp,argl,retp)
X509_LOOKUP *ctx;
int cmd;
char *argp;
long argl;
char **retp;
	{
	int ret=0;
	BY_DIR *ld;
	char *dir;

	ld=(BY_DIR *)ctx->method_data;

	switch (cmd)
		{
	case X509_L_ADD_DIR:
		if (argl == X509_FILETYPE_DEFAULT)
			{
			ret=add_cert_dir(ld,X509_get_default_cert_dir(),
				X509_FILETYPE_PEM);
			if (!ret)
				{
#line 95 /* the line number is part of the binary */
				X509err(215,156);
				}
			else
				{
				dir=(char *)Getenv(X509_get_default_cert_dir_env());
				ret=add_cert_dir(ld,dir,X509_FILETYPE_PEM);
				}
			}
		else
			ret=add_cert_dir(ld,argp,(int)argl);
		break;
		}
	return(ret);
	}

static int new_dir(lu)
X509_LOOKUP *lu;
	{
	BY_DIR *a;

	if ((a=(BY_DIR *)R_malloc(sizeof(BY_DIR))) == NULL)
		return(0);
	if ((a->buffer=BUF_MEM_new()) == NULL)
		{
		R_free(a);
		return(0);
		}
	a->num_dirs=0;
	a->dirs=NULL;
	a->dirs_type=NULL;
	a->num_dirs_alloced=0;
	lu->method_data=(char *)a;
	return(1);
	}

static void free_dir(lu)
X509_LOOKUP *lu;
	{
	BY_DIR *a;
	int i;

	a=(BY_DIR *)lu->method_data;
	for (i=0; i<a->num_dirs; i++)
		if (a->dirs[i] != NULL) R_free(a->dirs[i]);
	if (a->dirs != NULL) R_free(a->dirs);
	if (a->dirs_type != NULL) R_free(a->dirs_type);
	if (a->buffer != NULL) BUF_MEM_free(a->buffer);
	R_free(a);
	}

static int add_cert_dir(ctx,dir, type)
BY_DIR *ctx;
char *dir;
int type;
	{
	int j,len;
	int *ip;
	char *s,*ss,*p;
	char **pp;

	if ((dir == NULL) || !strlen(dir)) return(0);

	s=dir;
	p=s;
	for (;;)
		{
		if ((*p == ':') || (*p == '\0'))
			{
			ss=s;
			s=p+1;
			len=(int)(p-ss);
			if (len == 0) continue;
			for (j=0; j<ctx->num_dirs; j++)
				if (strncmp(ctx->dirs[j],ss,(unsigned int)len) == 0)
					continue;
			if (ctx->num_dirs_alloced < (ctx->num_dirs+1))
				{
				ctx->num_dirs_alloced+=10;
				pp=(char **)R_malloc(ctx->num_dirs_alloced*
					sizeof(char *));
				ip=(int *)R_malloc(ctx->num_dirs_alloced*
					sizeof(int));
				if ((pp == NULL) || (ip == NULL))
					{
#line 181 /* the line number is part of the binary */
					X509err(213,ERR_R_MALLOC_FAILURE);
					return(0);
					}
				memcpy(pp,ctx->dirs,(ctx->num_dirs_alloced-10)*
					sizeof(char *));
				memcpy(ip,ctx->dirs_type,(ctx->num_dirs_alloced-10)*
					sizeof(int));
				if (ctx->dirs != NULL)
					R_free((char *)ctx->dirs);
				if (ctx->dirs_type != NULL)
					R_free((char *)ctx->dirs_type);
				ctx->dirs=pp;
				ctx->dirs_type=ip;
				}
			ctx->dirs_type[ctx->num_dirs]=type;
			ctx->dirs[ctx->num_dirs]=(char *)R_malloc((unsigned int)len+1);
			if (ctx->dirs[ctx->num_dirs] == NULL) return(0);
			strncpy(ctx->dirs[ctx->num_dirs],ss,(unsigned int)len);
			ctx->dirs[ctx->num_dirs][len]='\0';
			ctx->num_dirs++;
			}
		if (*p == '\0') break;
		p++;
		}
	return(1);
	}

static int get_cert_by_subject(xl,type,name,ret)
X509_LOOKUP *xl;
int type;
X509_NAME *name;
X509_OBJECT *ret;
	{
	BY_DIR *ctx;
	union	{
		struct	{
			X509 st_x509;
			X509_CINF st_x509_cinf;
			} x509;
		struct	{
			X509_CRL st_crl;
			X509_CRL_INFO st_crl_info;
			} crl;
		} data;
	int ok=0;
	int i,j,k;
	unsigned long h;
	BUF_MEM *b=NULL;
	struct stat st;
	X509_OBJECT stmp,*tmp;
	char *postfix="";

	if (name == NULL) return(0);

	stmp.type=type;
	if (type == X509_LU_X509)
		{
		data.x509.st_x509.cert_info= &data.x509.st_x509_cinf;
		data.x509.st_x509_cinf.subject=name;
		stmp.data.x509= &data.x509.st_x509;
		postfix="";
		}
	else if (type == X509_LU_CRL)
		{
		}
	else
		{
#line 258 /* the line number is part of the binary */
		X509err(216,165);
		goto finish;
		}

	if ((b=BUF_MEM_new()) == NULL)
		{
		X509err(216,ERR_R_BUF_LIB);
		goto finish;
		}

	ctx=(BY_DIR *)xl->method_data;

	h=X509_NAME_hash(name);
	for (i=0; i<ctx->num_dirs; i++)
		{
		j=strlen(ctx->dirs[i])+1+8+6+1+1;
		if (!BUF_MEM_grow(b,j))
			{
			X509err(216,ERR_R_MALLOC_FAILURE);
			goto finish;
			}
		k=0;
		for (;;)
			{
			sprintf(b->data,"%s/%08lx.%s%d",ctx->dirs[i],h,
				postfix,k);
			k++;
			if (stat(b->data,&st) < 0)
				break;
			/* found one. */
			if (type == X509_LU_X509)
				{
				if ((X509_load_cert_file(xl,b->data,
					ctx->dirs_type[i])) == 0)
					break;
				}
			else if (type == X509_LU_CRL)
				{
				}
			/* else case will caught higher up */
			}

		/* we have added it to the cache so now pull
		 * it out again */
#line 309 /* the line number is part of the binary */
		R_lock_ctrl(CRYPTO_LOCK|CRYPTO_READ,CRYPTO_LOCK_X509_STORE,__FILE__,__LINE__);
		tmp=(X509_OBJECT *)lh_retrieve(xl->store_ctx->certs,
			(char *)&stmp);
		R_lock_ctrl(CRYPTO_UNLOCK|CRYPTO_READ,CRYPTO_LOCK_X509_STORE,__FILE__,__LINE__);

		if (tmp != NULL)
			{
			ok=1;
			ret->type=tmp->type;
			memcpy(&ret->data,&tmp->data,sizeof(ret->data));
			goto finish;
			}
		}
finish:
	if (b != NULL) BUF_MEM_free(b);
	return(ok);
	}
