/* ssl/ssl_ciph.c */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 * 
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 * 
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from 
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 * 
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

#include <stdio.h>
#include "objects.h"
#include "ssl_locl.h"






#undef ERR_file_name
#define ERR_file_name "./ssl/../ssl/ssl_ciph.c"

#define SSL_ENC_DES_IDX		0
#define SSL_ENC_3DES_IDX	1
#define SSL_ENC_RC4_IDX		2
#define SSL_ENC_RC2_IDX		3
#define SSL_ENC_IDEA_IDX	4
#define SSL_ENC_eFZA_IDX	5
#define SSL_ENC_NULL_IDX	6
#define SSL_ENC_NUM_IDX		7

static EVP_CIPHER *ssl_cipher_methods[SSL_ENC_NUM_IDX]={
	NULL,NULL,NULL,NULL,NULL,NULL,
	};

#define SSL_MD_MD5_IDX	0
#define SSL_MD_SHA1_IDX	1
#define SSL_MD_NUM_IDX	2
static EVP_MD *ssl_digest_methods[SSL_MD_NUM_IDX]={
	NULL,NULL,
	};

typedef struct cipher_sort_st
	{
	SSL_CIPHER *cipher;
	int pref;
	} CIPHER_SORT;

#define CIPHER_ADD	1
#define CIPHER_KILL	2
#define CIPHER_DEL	3
#define CIPHER_ORD	4

/* SSL-C: the fork's choice also carries the alias name, a "single cipher name" flag and the
 * id of a cipher given in hex ("0x...."). */
typedef struct cipher_choice_st
	{
	char *name;
	int type;
	unsigned long algorithms;
	unsigned long mask;
	int single;
	unsigned long id;
	} CIPHER_CHOICE;

typedef struct cipher_order_st
	{
	SSL_CIPHER *cipher;
	int active;
	int dead;
	struct cipher_order_st *next,*prev;
	} CIPHER_ORDER;

static SSL_CIPHER cipher_aliases[]={
	{0,SSL_TXT_ALL, 0,SSL_ALL,   0,SSL_ALL},	/* must be first */
	{0,SSL_TXT_kRSA,0,SSL_kRSA,  0,SSL_MKEY_MASK},
	{0,SSL_TXT_kDHr,0,SSL_kDHr,  0,SSL_MKEY_MASK},
	{0,SSL_TXT_kDHd,0,SSL_kDHd,  0,SSL_MKEY_MASK},
	{0,SSL_TXT_kEDH,0,SSL_kEDH,  0,SSL_MKEY_MASK},
	{0,SSL_TXT_kFZA,0,SSL_kFZA,  0,SSL_MKEY_MASK},
	{0,SSL_TXT_DH,	0,SSL_DH,    0,SSL_MKEY_MASK},
	{0,SSL_TXT_EDH,	0,SSL_EDH,   0,SSL_MKEY_MASK|SSL_AUTH_MASK},

	{0,SSL_TXT_aRSA,0,SSL_aRSA,  0,SSL_AUTH_MASK},
	{0,SSL_TXT_aDSS,0,SSL_aDSS,  0,SSL_AUTH_MASK},
	{0,SSL_TXT_aFZA,0,SSL_aFZA,  0,SSL_AUTH_MASK},
	{0,SSL_TXT_aNULL,0,SSL_aNULL,0,SSL_AUTH_MASK},
	{0,SSL_TXT_aDH, 0,SSL_aDH,   0,SSL_AUTH_MASK},
	{0,SSL_TXT_DSS,	0,SSL_DSS,   0,SSL_AUTH_MASK},

	{0,SSL_TXT_DES,	0,SSL_DES,   0,SSL_ENC_MASK},
	{0,SSL_TXT_3DES,0,SSL_3DES,  0,SSL_ENC_MASK},
	{0,SSL_TXT_RC4,	0,SSL_RC4,   0,SSL_ENC_MASK},
	{0,SSL_TXT_RC2,	0,SSL_RC2,   0,SSL_ENC_MASK},
	{0,SSL_TXT_IDEA,0,SSL_IDEA,  0,SSL_ENC_MASK},
	{0,SSL_TXT_eNULL,0,SSL_eNULL,0,SSL_ENC_MASK},
	{0,SSL_TXT_eFZA,0,SSL_eFZA,  0,SSL_ENC_MASK},

	{0,SSL_TXT_MD5,	0,SSL_MD5,   0,SSL_MAC_MASK},
	{0,SSL_TXT_SHA1,0,SSL_SHA1,  0,SSL_MAC_MASK},
	{0,SSL_TXT_SHA,	0,SSL_SHA,   0,SSL_MAC_MASK},

	{0,SSL_TXT_NULL,0,SSL_NULL,  0,SSL_ENC_MASK},
	{0,SSL_TXT_RSA,	0,SSL_RSA,   0,SSL_AUTH_MASK|SSL_MKEY_MASK},
	{0,SSL_TXT_ADH,	0,SSL_ADH,   0,SSL_AUTH_MASK|SSL_MKEY_MASK},
	{0,SSL_TXT_FZA,	0,SSL_FZA,   0,SSL_AUTH_MASK|SSL_MKEY_MASK|SSL_ENC_MASK},

	{0,SSL_TXT_EXP,	0,SSL_EXP,   0,SSL_EXP_MASK},
	{0,SSL_TXT_EXPORT,0,SSL_EXPORT,0,SSL_EXP_MASK},
	{0,SSL_TXT_SSLV2,0,SSL_SSLV2,0,SSL_SSL_MASK},
	{0,SSL_TXT_SSLV3,0,SSL_SSLV3,0,SSL_SSL_MASK},
	{0,"TLSv1",	 0,SSL_SSLV3,0,SSL_SSL_MASK},	/* SSL-C: extra alias */
	{0,SSL_TXT_LOW,  0,SSL_LOW,0,SSL_STRONG_MASK},
	{0,SSL_TXT_MEDIUM,0,SSL_MEDIUM,0,SSL_STRONG_MASK},
	{0,SSL_TXT_HIGH, 0,SSL_HIGH,0,SSL_STRONG_MASK},
	/* SSL-C: extra alias with the name of the SSLv2 cipher (algorithm2 as a literal) */

	{0,SSL2_TXT_RC4_64_WITH_MD5,0,
		SSL_kRSA|SSL_aRSA|SSL_RC4|SSL_MD5|SSL_EXP|SSL_SSLV2|SSL_LOW,
		0x408,SSL_ALL_CIPHERS},
	};

static int init_ciphers=1;
static void load_ciphers();

static int cmp_by_name(a,b)
SSL_CIPHER **a,**b;
	{
	return(strcmp((*a)->name,(*b)->name));
	}

/* SSL-C: three stacks, not two (sk_new(NULL) and two comparators) */





static int cmp_by_id(a,b)
SSL_CIPHER **a,**b;
	{
	unsigned long l,r;

	l=(*a)->id;
	r=(*b)->id;
	l&=0xffffffL;
	r&=0xffffffL;
	if (l == r)
		return(0);
	else
		return((l > r)?1:-1);
	}

static void load_ciphers()
	{
	init_ciphers=0;
	ssl_cipher_methods[SSL_ENC_DES_IDX]= 
		EVP_OBJ_get_cipherbyname(SN_des_cbc);
	ssl_cipher_methods[SSL_ENC_3DES_IDX]=
		EVP_OBJ_get_cipherbyname(SN_des_ede3_cbc);
	ssl_cipher_methods[SSL_ENC_RC4_IDX]=
		EVP_OBJ_get_cipherbyname(SN_rc4);
	ssl_cipher_methods[SSL_ENC_RC2_IDX]= 
		EVP_OBJ_get_cipherbyname(SN_rc2_cbc);
	ssl_cipher_methods[SSL_ENC_IDEA_IDX]= 
		EVP_OBJ_get_cipherbyname(SN_idea_cbc);

	ssl_digest_methods[SSL_MD_MD5_IDX]=
		EVP_OBJ_get_digestbyname(SN_md5);
	ssl_digest_methods[SSL_MD_SHA1_IDX]=
		EVP_OBJ_get_digestbyname(SN_sha1);
	}

int ssl_cipher_get_evp(c,enc,md)
SSL_CIPHER *c;
EVP_CIPHER **enc;
EVP_MD **md;
	{
	int i;

	if (c == NULL) return(0);

	switch (c->algorithms & SSL_ENC_MASK)
		{
	case SSL_DES:
		i=SSL_ENC_DES_IDX;
		break;
	case SSL_3DES:
		i=SSL_ENC_3DES_IDX;
		break;
	case SSL_RC4:
		i=SSL_ENC_RC4_IDX;
		break;
	case SSL_RC2:
		i=SSL_ENC_RC2_IDX;
		break;
	case SSL_IDEA:
		i=SSL_ENC_IDEA_IDX;
		break;
	case SSL_eNULL:
		i=SSL_ENC_NULL_IDX;
		break;

	default:
		i= -1;
		break;
		}

	if ((i < 0) || (i > SSL_ENC_NUM_IDX))
		*enc=NULL;
	else
		{
		if (i == SSL_ENC_NULL_IDX)
			*enc=EVP_enc_null();
		else
			*enc=ssl_cipher_methods[i];
		}

	switch (c->algorithms & SSL_MAC_MASK)
		{
	case SSL_MD5:
		i=SSL_MD_MD5_IDX;
		break;
	case SSL_SHA1:
		i=SSL_MD_SHA1_IDX;
		break;
	default:
		i= -1;
		break;
		}
	if ((i < 0) || (i > SSL_MD_NUM_IDX))
		*md=NULL;
	else
		*md=ssl_digest_methods[i];

	if ((*enc != NULL) && (*md != NULL))
		return(1);
	else
		return(0);
	}

#define ITEM_SEP(a) \
	(((a) == ':') || ((a) == ' ') || ((a) == ';') || ((a) == ','))

static void ll_append_tail(head,curr,tail)
CIPHER_ORDER **head,*curr,**tail;
	{
	if (curr == *tail) return;
	if (curr == *head)
		*head=curr->next;
	if (curr->prev != NULL)
		curr->prev->next=curr->next;
	if (curr->next != NULL) /* should always be true */
		curr->next->prev=curr->prev;
	(*tail)->next=curr;
	curr->prev= *tail;
	curr->next=NULL;
	*tail=curr;
	}

STACK *ssl_create_cipher_list(ssl_method,cipher_list,cipher_list_by_id,str)
SSL_METHOD *ssl_method;
STACK **cipher_list,**cipher_list_by_id;
char *str;
	{
	SSL_CIPHER *c;
	char *l;
	STACK *ret=NULL,*ok=NULL;
#define CL_BUF	40
	char buf[CL_BUF];
	char *tmp_str=NULL;
	unsigned long mask,algorithms,ma;
	char *start;
	int i,j,k,num=0,ch,multi;
	int rule;
	unsigned long al;
	STACK *ca_list=NULL,*ca_list2=NULL;
	int current_x,num_x;
	CIPHER_CHOICE *ops=NULL;
	CIPHER_ORDER *list=NULL,*head=NULL,*tail=NULL,*curr,*tail2,*curr2;
	int list_num;
	int type;
	int single;
	unsigned long id;
	char *name;
	SSL_CIPHER c_tmp,*cp;

	if (str == NULL) return(NULL);

	if (strncmp(str,"DEFAULT",7) == 0)
		{
		i=strlen(str)+2+strlen(SSL_DEFAULT_CIPHER_LIST);
		if ((tmp_str=Malloc(i)) == NULL)
			{
#line 295 /* the line number is part of the binary */
			SSLerr(166,ERR_R_MALLOC_FAILURE);
			goto err;
			}
		strcpy(tmp_str,SSL_DEFAULT_CIPHER_LIST);
		strcat(tmp_str,":");
		strcat(tmp_str,&(str[7]));
		str=tmp_str;
		}
	if (init_ciphers) load_ciphers();

	mask =SSL_kFZA;
	/* SSL-C: the NO_DSA/NO_DH/... ifdefs are runtime guards */


	if (!SSL_feature_test(2)) mask|=SSL_aDSS;
	if (!SSL_feature_test(1)) mask|=0x216L;
	if (!SSL_feature_test(4)) mask|=0x10000L;
	if (!SSL_feature_test(8)) mask|=0x100L;

	mask|=(ssl_cipher_methods[SSL_ENC_DES_IDX ] == NULL)?SSL_DES :0;
	mask|=(ssl_cipher_methods[SSL_ENC_3DES_IDX] == NULL)?SSL_3DES:0;
	mask|=(ssl_cipher_methods[SSL_ENC_RC4_IDX ] == NULL)?SSL_RC4 :0;
	mask|=(ssl_cipher_methods[SSL_ENC_RC2_IDX ] == NULL)?SSL_RC2 :0;
	mask|=(ssl_cipher_methods[SSL_ENC_IDEA_IDX] == NULL)?SSL_IDEA:0;
	mask|=(ssl_cipher_methods[SSL_ENC_eFZA_IDX] == NULL)?SSL_eFZA:0;

	mask|=(ssl_digest_methods[SSL_MD_MD5_IDX ] == NULL)?SSL_MD5 :0;
	mask|=(ssl_digest_methods[SSL_MD_SHA1_IDX] == NULL)?SSL_SHA1:0;

	num=ssl_method->num_ciphers();

	if ((ret=(STACK *)sk_new(NULL)) == NULL) goto err;
	if ((ca_list=(STACK *)sk_new(cmp_by_name)) == NULL) goto err;
	if ((ca_list2=(STACK *)sk_new(cmp_by_id)) == NULL) goto err;

	if ((list=(CIPHER_ORDER *)Malloc(sizeof(CIPHER_ORDER)*num+1)) == NULL)
		goto err;

	/* Get the initial list of ciphers */
	list_num=0;
	for (i=0; i<num; i++)
		{
		c=ssl_method->get_cipher((unsigned int)i);
		/* drop those that use any of that is not available */
		if ((c != NULL) && c->valid && !(c->algorithms & mask))
			{
			list[list_num].cipher=c;
			list[list_num].next=NULL;
			list[list_num].prev=NULL;
			list[list_num].active=0;
			list_num++;
			if (!sk_push(ca_list,(char *)c)) goto err;
			if (!sk_push(ca_list2,(char *)c)) goto err;
			}
		}
	
	for (i=1; i<list_num-1; i++)
		{
		list[i].prev= &(list[i-1]);
		list[i].next= &(list[i+1]);
		}
	if (list_num > 0)
		{
		head= &(list[0]);
		head->prev=NULL;
		head->next= &(list[1]);
		tail= &(list[list_num-1]);
		if (list_num < 2)
			tail->prev=NULL;
		else
			tail->prev= &(list[list_num-2]);
		tail->next=NULL;
		}

	/* special case */
	cipher_aliases[0].algorithms= ~mask;

	/* get the aliases */
	k=sizeof(cipher_aliases)/sizeof(SSL_CIPHER);
	for (j=0; j<k; j++)
		{
		al=cipher_aliases[j].algorithms;
		/* Drop those that are not relevent */
		if ((al & mask) == al) continue;
		if (!sk_push(ca_list,(char *)&(cipher_aliases[j]))) goto err;
		}

	/* ca_list now holds a 'stack' of SSL_CIPHERS, some real, some
	 * 'aliases' */

	/* how many parameters are there? */
	num=1;
	for (l=str; *l; l++)
		if (ITEM_SEP(*l))
			num++;
	ops=(CIPHER_CHOICE *)Malloc(sizeof(CIPHER_CHOICE)*num);
	if (ops == NULL) goto err;
	memset(ops,0,sizeof(CIPHER_CHOICE)*num);

	/* we now parse the input string and create our operations */
	l=str;
	i=0;
	current_x=0;

	for (;;)
		{
		ch= *l;

		if (ch == '\0') break;

		if (ch == '-')
			{ rule=CIPHER_DEL; l++; }
		else if (ch == '+')
			{ rule=CIPHER_ORD; l++; }
		else if (ch == '!')
			{ rule=CIPHER_KILL; l++; }
		else	
			{ rule=CIPHER_ADD; }

		if (ITEM_SEP(ch))
			{
			l++;
			continue;
			}

		start=l;
		for (;;)
			{
			ch= *l;
			i=0;
			while (	((ch >= 'A') && (ch <= 'Z')) ||
				((ch >= '0') && (ch <= '9')) ||
				((ch >= 'a') && (ch <= 'z')) ||
				 (ch == '-'))
				 {
				 buf[i]=ch;
				 ch= *(++l);
				 i++;
				 if (i >= (CL_BUF-2)) break;
				 }
			buf[i]='\0';

			/* check for multi-part specification */
			if (ch == '+')
				{
				multi=1;
				l++;
				/* SSL-C: after a '+' another name must start,
				 * otherwise the whole element is skipped */
				if (!(((*l >= 'A') && (*l <= 'Z')) ||
					((*l >= '0') && (*l <= '9')) ||
					((*l >= 'a') && (*l <= 'z')) ||
					(*l == '-')))
					{
					while (!ITEM_SEP(*l) && (*l != '\0'))
						l++;
					goto end_loop;
					}
				}
			else
				{
				multi=0;
				/* SSL-C: the name must end on a separator */
				if (!((ITEM_SEP(*l) || (*l == '\0')) && (i != 0)))
					{
					while (!ITEM_SEP(*l) && (*l != '\0'))
						l++;
					goto end_loop;
					}
				}

			c_tmp.name=buf;
			j=sk_find(ca_list,(char *)&c_tmp);
			if (j < 0)
				{
				/* SSL-C: not a name: try the id of the cipher
				 * in hex ("0x0300000A") */
				char *p=buf;
				unsigned long val=0;

				if ((p[0] == '0') && (p[1] == 'x'))
					p+=2;
				while (*p)
					{
					val<<=4;
					if ((*p >= '0') && (*p <= '9'))
						val+= *p-'0';
					else if ((*p >= 'A') && (*p <= 'F'))
						val+= *p-'A'+10;
					else if ((*p >= 'a') && (*p <= 'f'))
						val+= *p-'a'+10;
					else
						goto end_loop;
					p++;
					}
				c_tmp.id=val;
				j=sk_find(ca_list2,(char *)&c_tmp);
				if (j < 0)
					goto end_loop;
				else
					{
					cp=(SSL_CIPHER *)sk_value(ca_list2,j);
					ops[current_x].id=cp->id;
					}
				}
			else
				{
				cp=(SSL_CIPHER *)sk_value(ca_list,j);
				ops[current_x].id=0;
				}
			ops[current_x].type=rule;
			ops[current_x].name=cp->name;
			ops[current_x].algorithms|=cp->algorithms;
			/* We add the SSL_SSL_MASK so we can match the
			 * SSLv2 and SSLv3 versions of RC4-MD5 */
			ops[current_x].mask|=cp->mask;
			ops[current_x].single=(cp->valid == 1);
			if (!multi || ops[current_x].single) break;
			}
		current_x++;
		if (ch == '\0') break;
end_loop:
		/* Make sure we scan until the next valid start point */
		while ((*l != '\0') && ITEM_SEP(*l))
			l++;
		}

	num_x=current_x;
	current_x=0;

	/* We will now process the list of ciphers, once for each category, to
	 * decide what we should do with it. */
	for (j=0; j<num_x; j++)
		{
		algorithms=ops[j].algorithms;
		type=ops[j].type;
		mask=ops[j].mask;
		name=ops[j].name;
		single=ops[j].single;
		id=ops[j].id;

		curr=head;
		curr2=head;
		tail2=tail;
		for (;;)
			{
			if ((curr == NULL) || (curr == tail2)) break;
			curr=curr2;
			curr2=curr->next;

			cp=curr->cipher;
			/* SSL-C: a single cipher is chosen by name, and one
			 * given by id only by id */
			if (single && strcmp(name,cp->name))
				continue;
			if (id && (cp->id != id))
				continue;
			ma=mask & cp->algorithms;
			if ((ma == 0) || ((ma & algorithms) != ma))
				{
				/* does not apply */
				continue;
				}

			/* add the cipher if it has not been added yet. */
			if (type == CIPHER_ADD)
				{
				if (!curr->active)
					{
					ll_append_tail(&head,curr,&tail);
					curr->active=1;
					}
				}
			/* Move the added cipher to this location */
			else if (type == CIPHER_ORD)
				{
				if (curr->active)
					{
					ll_append_tail(&head,curr,&tail);
					}
				}
			else if	(type == CIPHER_DEL)
				curr->active=0;
			if (type == CIPHER_KILL)
				{
				if (head == curr)
					head=curr->next;
				else
					curr->prev->next=curr->next;
				if (tail == curr)
					tail=curr->prev;
				curr->active=0;
				if (curr->next != NULL)
					curr->next->prev=curr->prev;
				if (curr->prev != NULL)
					curr->prev->next=curr->next;
				curr->next=NULL;
				curr->prev=NULL;
				}
			}
		}

	for (curr=head; curr != NULL; curr=curr->next)
		{
		if (curr->active)
			{
			sk_push(ret,(char *)curr->cipher);
#ifdef CIPHER_DEBUG
			printf("<%s>\n",curr->cipher->name);
#endif
			}
		}

	if (cipher_list != NULL)
		{
		if (*cipher_list != NULL)
			sk_free(*cipher_list);
		*cipher_list=ret;
		if (*cipher_list == NULL)
			goto err;
		}

	if (cipher_list_by_id != NULL)
		{
		if (*cipher_list_by_id != NULL)
			sk_free(*cipher_list_by_id);
		*cipher_list_by_id=sk_dup(ret);
		if (*cipher_list_by_id == NULL)
			goto err;
		sk_set_cmp_func(*cipher_list_by_id,ssl_cipher_ptr_id_cmp);
		}

	ok=ret;
	ret=NULL;
err:
	if (tmp_str) Free(tmp_str);
	if (ops != NULL) Free(ops);
	if (ret != NULL) sk_free(ret);
	if (ca_list != NULL) sk_free(ca_list);
	if (ca_list2 != NULL) sk_free(ca_list2);
	if (list != NULL) Free(list);
	return(ok);
	}

char *SSL_CIPHER_description(cipher,buf,len)
SSL_CIPHER *cipher;
char *buf;
int len;
	{
	int export;
	int enckeybits;
	char *ver,*exp;
	char *kx,*au,*enc,*mac;
	unsigned long alg,alg2;
	char encbuf[16];
	char kxbuf[32];
	static char *format="%-23s %s Kx=%-8s Au=%-4s Enc=%-9s Mac=%-4s%s"; /* SSL-C: without the final \n */

	alg=cipher->algorithms;
	alg2=cipher->algorithm2;

	export=(alg&SSL_EXP)?1:0;
	exp=(export)?" export":"";

	if (alg & SSL_SSLV2)
		ver="SSLv2";
	else if (alg & SSL_SSLV3)
		ver="SSLv3";
	else
		ver="unknown";

	switch (alg&SSL_MKEY_MASK)
		{
	case SSL_kRSA:
		if (export)
			{
			sprintf(kxbuf,"RSA(%d)",(int)(alg2&0xff00));
			kx=kxbuf;
			}
		else
			kx="RSA";
		break;
	case SSL_kDHr:
		kx="DH/RSA";
		break;
	case SSL_kDHd:
		kx="DH/DSS";
		break;
	case SSL_kFZA:
		kx="Fortezza";
		break;
	case SSL_kEDH:
		if (export)
			{
			sprintf(kxbuf,"DSA(%d)",(int)(alg2&0xff00));
			kx=kxbuf;
			}
		else
			kx="DH";
		break;
	default:
		kx="unknown";
		}

	switch (alg&SSL_AUTH_MASK)
		{
	case SSL_aRSA:
		au="RSA";
		break;
	case SSL_aDSS:
		au="DSS";
		break;
	case SSL_aDH:
		au="DH";
		break;
	case SSL_aFZA:
	case SSL_aNULL:
		au="None";
		break;
	default:
		au="unknown";
		break;
		}

	enckeybits=(((int)alg2)&0xff)*8;
	switch (alg&SSL_ENC_MASK)
		{
	case SSL_DES:
		if ((enckeybits == 0) || (enckeybits >= 57))
			enckeybits=56;
		enc="DES";
		break;
	case SSL_3DES:
		if (enckeybits == 0)
			enckeybits=168;
		enc="3DES";
		break;
	case SSL_RC4:
		if (enckeybits == 0)
			enckeybits=128;
		enc="RC4";
		break;
	case SSL_RC2:
		if (enckeybits == 0)
			enckeybits=128;
		enc="RC2";
		break;
	case SSL_IDEA:
		if (enckeybits == 0)
			enckeybits=128;
		enc="IDEA";
		break;
	case SSL_eFZA:
		enckeybits=-1;
		enc="Fortezza";
		break;
	case SSL_eNULL:
		enckeybits=-1;
		enc="None";
		break;
	default:
		enckeybits=-1;
		enc="unknown";
		break;
		}
	switch (alg&SSL_MAC_MASK)
		{
	case SSL_MD5:
		mac="MD5";
		break;
	case SSL_SHA1:
		mac="SHA1";
		break;
	default:
		mac="unknown";
		break;
		}

	if (buf == NULL)
		{
		buf=Malloc(128);
		if (buf == NULL)
			{
			#line 892 /* the line number is part of the binary */
			SSLerr(216,33);
			return(NULL);
			}
		}
	else if (len < 128)
		{
		SSLerr(216,16);
		return(NULL);
		}

	if (enckeybits <= 0)
		strcpy(encbuf,enc);
	else
		sprintf(encbuf,"%s(%d)",enc,enckeybits);

	sprintf(buf,format,cipher->name,ver,kx,au,encbuf,mac,exp);
	return(buf);
	}

unsigned long SSL_CIPHER_get_id(c)
SSL_CIPHER *c;
	{
	if (c == NULL) return(0xffffffffL);
	return(c->id&0xffffffL);
	}

char *SSL_CIPHER_get_version(c)
SSL_CIPHER *c;
	{
	int i;

	if (c == NULL) return("(NONE)");
	i=(int)(c->id>>24L);
	if (i == 3)
		return("TLSv1/SSLv3");
	else if (i == 2)
		return("SSLv2");
	else
		return("unknown");
	}

/* return the actual cipher being used */
char *SSL_CIPHER_get_name(c)
SSL_CIPHER *c;
	{
	if (c != NULL)
		return(c->name);
	return("(NONE)");
	}

int SSL_CIPHER_get_valid(c)
SSL_CIPHER *c;
	{
	return(c->valid);
	}

/* number of bits for symetric cipher */
int SSL_CIPHER_get_bits(c,alg_bits)
SSL_CIPHER *c;
int *alg_bits;
	{
	int ret=0,a=0;
	EVP_CIPHER *enc;
	EVP_MD *md;

	if (c != NULL)
		{
		if (!ssl_cipher_get_evp(c,&enc,&md))
			return(0);

		a=EVP_CIPHER_key_length(enc)*8;
		/* SSL-C: for DES/3DES the real bits are 7 of 8 (parity), and the
		 * effective bits come from algorithm2 (key bytes) when set */
		if (c->algorithms & (SSL_DES|SSL_3DES))
			a=a*7/8;

		ret=(int)(c->algorithm2&0xff)*8;
		if (ret == 0)
			ret=a;
		}

	if (alg_bits != NULL) *alg_bits=a;
	
	return(ret);
	}

