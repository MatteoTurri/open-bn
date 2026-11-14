/* lib/bio/bio_err.c */
/* Copyright (C) 1995-1997 Eric Young (eay@cryptsoft.com)
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
#include "err.h"
#include "bio.h"

/* BEGIN ERROR CODES */
#ifndef NO_ERR
static ERR_STRING_DATA BIO_str_functs[]=
	{
{ERR_PACK(0,BIO_F_ACPT_STATE,0),	"ACPT_STATE"},
{ERR_PACK(0,BIO_F_BIO_CTRL,0),	"BIO_ctrl"},
{ERR_PACK(0,BIO_F_BIO_GETS,0),	"BIO_gets"},
{ERR_PACK(0,BIO_F_BIO_NEW,0),	"BIO_new"},
{ERR_PACK(0,BIO_F_BIO_PUTS,0),	"BIO_puts"},
{ERR_PACK(0,BIO_F_BIO_READ,0),	"BIO_read"},
{ERR_PACK(0,BIO_F_BIO_WRITE,0),	"BIO_write"},
{ERR_PACK(0,BIO_F_BUFFER_CTRL,0),	"BUFFER_CTRL"},
{ERR_PACK(0,BIO_F_CONN_CTRL,0),	"CONN_CTRL"},
{ERR_PACK(0,BIO_F_CONN_STATE,0),	"CONN_STATE"},
{ERR_PACK(0,BIO_F_FILE_CTRL,0),	"FILE_CTRL"},
{ERR_PACK(0,BIO_F_MEM_WRITE,0),	"MEM_WRITE"},
{ERR_PACK(0,BIO_F_SSL_NEW,0),	"SSL_NEW"},
{ERR_PACK(0,BIO_F_UDP_ACPT_STATE,0),	"UDP_ACPT_STATE"},
{ERR_PACK(0,BIO_F_WSASTARTUP,0),	"WSASTARTUP"},
{ERR_PACK(0,BIO_F_SSL_NEW,0),	"SSL_NEW"},
{0,NULL},
	};

static ERR_STRING_DATA BIO_str_reasons[]=
	{
{BIO_R_BAD_FOPEN_MODE                    ,"bad fopen mode"},
{BIO_R_ERROR_SETTING_NBIO_ON_ACCEPTED_SOCKET,"error setting nbio on accepted socket"},
{BIO_R_ERROR_SETTING_NBIO_ON_ACCEPT_SOCKET,"error setting nbio on accept socket"},
{BIO_R_ERROR_SETTING_NBIO_ON_UDP_ACCEPT_SOCKET,"error setting nbio on udp accept socket"},
{BIO_R_KEEPALIVE                         ,"keepalive"},
{BIO_R_NBIO_CONNECT_ERROR                ,"nbio connect error"},
{BIO_R_NO_ACCEPT_PORT_SPECIFIED          ,"no accept port specified"},
{BIO_R_NO_UDP_ACCEPT_PORT_SPECIFIED      ,"no udp accept port specified"},
{BIO_R_NULL_PARAMETER                    ,"null parameter"},
{BIO_R_UNINITALISED                      ,"uninitalised"},
{BIO_R_UNSUPPORTED_METHOD                ,"unsupported method"},
{BIO_R_WSASTARTUP                        ,"wsastartup"},
{0,NULL},
	};

#endif

/* SSL-C: the library code is assigned at run time and kept here, shared with ERR_BIO_error. */
static int bio_lib_code=0;

void ERR_load_BIO_strings()
	{
	static int init=1;

	if (bio_lib_code == 0)
		bio_lib_code=ERR_get_next_error_library();
	if (init)
		{;
		init=0;
#ifndef NO_ERR
		ERR_load_strings(bio_lib_code,BIO_str_functs);
		ERR_load_strings(bio_lib_code,BIO_str_reasons);
#endif

		}
	}

/* SSL-C: registers the dynamic library code on the first call and passes the error to
 * ERR_put_error with it. */
void ERR_BIO_error(int func, int reason, char *file, int line)
{
    if (bio_lib_code == 0)
        bio_lib_code = ERR_get_next_error_library();
    ERR_put_error(bio_lib_code, func, reason, file, line);
}
