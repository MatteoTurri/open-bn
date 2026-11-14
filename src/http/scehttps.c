/*
 * HTTPS layer of the HTTP library on SSL-C: certificate and key loading,
 * the TLS handshake with its verification callbacks, encrypted I/O, and
 * the debug traces of the handshake.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "http/scehttp.h"
#include "r_com.h"
#include "err.h"
#include "pem.h"
#include "sslcert.h"

/* loaded certificates (g_https_ncert entries) */
static X509 **g_https_cert = 0;
/* Lengths of the two 256-byte text accumulators of https_accum. */
static int g_https_acc1_len = 0;
static int g_https_acc2_len = 0;
/* set by the alerts 2 and 41 when sending */
static int g_https_alert_flag = 0;

/* Lengths of the two trace buffers g_https_msg and g_https_buf. */
static int g_https_msg_len = 0;
static int g_https_buf_len = 0;
/* trace level */
static int g_https_trace_level = 2;

/* number of loaded certificates */
static int g_https_ncert = 0;
/* SSL_METHOD * of the context */
static SSL_METHOD *g_https_method = 0;
/* How sceHTTPSOpen gets certificate and key: 2 = from the files,
   1 = already loaded in memory. */
static int g_https_mode = 0;
/* Allocation hooks, also installed for the RSA library. */
static void *(*g_https_alloc)(unsigned int size);
static void *(*g_https_realloc)(void *p, unsigned int size);
static void (*g_https_free)(void *p);
/* X509 * of the CA certificate loaded from memory */
static X509 *g_https_cacert = 0;
/* EVP_PKEY * of the private key loaded from memory */
static EVP_PKEY *g_https_privkey = 0;
/* Negotiated protocol: 2 = SSLv2, 3 = SSLv3, 1 = TLSv1, -1 = other. */
static int g_https_sslver;
/* The text accumulators of https_accum. */
static char g_https_acc1[256];
static char g_https_acc2[256];
/* last SSL state seen by https_info_cb */
static int g_https_last_state;
/* Trace buffers. */
static char g_https_msg[50];
static char g_https_buf[256];
/* Client certificate file and private key file of sceHTTPSOpen. */
static char g_https_certfile[256];
static char g_https_keyfile[256];
static void https_trace(int c);
static void https_busy_wait(void);

/* Installs the memory hooks (the user's, or xmalloc and friends) for the
   SSL and RSA libraries, initializes SSL and picks the client method from
   mode (0 = SSLv23, 1 = SSLv2, 2 = SSLv3, 3 = TLSv1). Returns 0, or < 0. */
static int https_ssl_setup(int mode)
{
	int ret;

	g_https_alloc = sceHTTPLibMallocFunction ? sceHTTPLibMallocFunction : xmalloc;
	g_https_realloc = sceHTTPLibReallocFunction ? sceHTTPLibReallocFunction : xrealloc;
	g_https_free = sceHTTPLibFreeFunction ? sceHTTPLibFreeFunction : xfree;
	R_set_mem_functions((R_MallocFunc)g_https_alloc, (R_ReallocFunc)g_https_realloc, g_https_free);
	SSL_load_error_strings();
	ret = SSL_library_init();
	if (ret < 0) {
		fprintf(stderr, "SSL_library_init() failed. err = 0x%x\n", ERR_get_error());
		return ret;
	}
	switch (mode) {
	case 0:
		g_https_method = SSLv23_client_method();
		break;
	case 1:
		g_https_method = SSLv2_client_method();
		break;
	case 2:
		g_https_method = SSLv3_client_method();
		break;
	case 3:
		g_https_method = TLSv1_client_method();
		break;
	default:
		fprintf(stderr, "invalid SSL version\n");
		ret = -1;
		goto done;
	}
	ret = 0;
done:
	return ret;
}

/* Initializes the HTTPS layer: the SSL method, the CA certificates from the
   NULL-terminated list of PEM files, and the paths of the client
   certificate and key. Returns 0, or -1 (and terminates the layer). */
int sceHTTPSInit(int mode, char **certfiles, char *cafile, char *capath)
{
	int ret;
	int i;
	BIO *bio;
	X509 *x;

	g_https_mode = 2;
	ret = https_ssl_setup(mode);
	if (ret < 0)
		return ret;

	bio = BIO_new(BIO_s_file());

	if (certfiles != 0) {
		for (i = 0; certfiles[i] != 0; i++)
			;
		g_https_ncert = i;
		g_https_cert = (X509 **)(*g_https_alloc)(i * 4);
		if (g_https_cert == 0) {
			fprintf(stderr, "malloc failed.\n");
			ret = -1;
			goto done;
		}
		memset(g_https_cert, 0, g_https_ncert * 4);
		for (i = 0; i < g_https_ncert; i++) {
			if (BIO_read_filename(bio, certfiles[i]) <= 0) {
				fprintf(stderr, "BIO_read_filename() failed!!\n");
				ret = -1;
				goto done;
			}
			x = (X509 *)PEM_ASN1_read_bio((char *(*)())d2i_X509, "CERTIFICATE", bio, 0, 0);
			if (x == 0) {
				fprintf(stderr, "PEM_read_bio_X509() failed. err = 0x%x\n", ERR_get_error());
				ret = -1;
				goto done;
			}
			g_https_cert[i] = x;
		}
	}

	if (cafile != 0) {
		if ((int)strlen(cafile) >= 256) {
			fprintf(stderr, "file path too long.\n");
			ret = -1;
			goto done;
		}
		strncpy(g_https_certfile, cafile, 256);
	} else {
		g_https_certfile[0] = 0;
	}

	if (capath != 0) {
		if ((int)strlen(capath) >= 256) {
			fprintf(stderr, "file path too long.\n");
			ret = -1;
			goto done;
		}
		strncpy(g_https_keyfile, capath, 256);
	} else {
		g_https_keyfile[0] = 0;
	}
	ret = 0;
done:
	BIO_free(bio);
	if (ret < 0)
		sceHTTPSTerminate();
	return ret;
}

/* Like sceHTTPSInit, with the certificate chain, the CA certificate and the
   private key given as PEM data in memory. */
int sceHTTPSInitCertFromMemory(int mode, int ncert, HttpsMemCert *certs, HttpsMemCert *ca, HttpsMemCert *key)
{
	int ret;
	BIO *bio;
	int i;
	unsigned int j;
	X509 *x;

	g_https_mode = 1;
	ret = https_ssl_setup(mode);
	if (ret < 0)
		return ret;

	bio = BIO_new_mem();
	if (bio == 0) {
		fprintf(stderr, "BIO_new_mem() failed.\n");
		return -1;
	}

	g_https_ncert = 0;
	g_https_cert = 0;
	g_https_cacert = 0;
	g_https_privkey = 0;

	if (certs != 0 && ncert > 0) {
		g_https_ncert = ncert;
		g_https_cert = (X509 **)g_https_alloc(ncert * 4);
		if (g_https_cert == 0) {
			fprintf(stderr, "malloc failed.\n");
			ret = -1;
			goto out;
		}
		memset(g_https_cert, 0, g_https_ncert * 4);
		for (i = 0; i < g_https_ncert; i++) {
			for (j = 0; j < certs[i].len; j++)
				BIO_printf(bio, "%c", certs[i].data[j]);
			x = (X509 *)PEM_ASN1_read_bio((char *(*)())d2i_X509, "CERTIFICATE", bio, 0, 0);
			if (x == 0) {
				fprintf(stderr, "PEM_read_bio_X509() failed\n");
				ret = -1;
				goto out;
			}
			g_https_cert[i] = x;
		}
	}
	BIO_reset(bio);

	if (ca != 0) {
		for (j = 0; j < ca->len; j++)
			BIO_printf(bio, "%c", ca->data[j]);
		g_https_cacert = (X509 *)PEM_ASN1_read_bio((char *(*)())d2i_X509, "CERTIFICATE", bio, 0, 0);
		if (g_https_cacert == 0) {
			fprintf(stderr, "PEM_read_bio_X509() failed\n");
			ret = -1;
			goto out;
		}
	}
	BIO_reset(bio);

	if (key != 0) {
		for (j = 0; j < key->len; j++)
			BIO_printf(bio, "%c", key->data[j]);
		g_https_privkey = (EVP_PKEY *)PEM_ASN1_read_bio((char *(*)())d2i_PrivateKey, "PRIVATE KEY", bio, 0, 0);
		if (g_https_privkey == 0) {
			fprintf(stderr, "PEM_read_bio_PrivateKey() failed\n");
			ret = -1;
			goto out;
		}
	}
	ret = 0;
out:
	BIO_free(bio);
	if (ret < 0)
		sceHTTPSTerminate();
	return ret;
}

static void https_dump_cert(X509 *x);

/* Callbacks installed by sceHTTPSOpen. */
static int https_verify_chain(void *unused, void *chain, int ncert, struct sceHTTPClient *conn, int *out);
static void https_info_cb(SSL *ssl, int where, int ret);
static void https_alert_cb(SSL *ssl, int where, int ret);
static int https_accum(void *unused1, int type, const char *src, int len, int unused5, int ret);

/* Clears the SSL error code of the connection. */
int sceHTTPSClearErrnum(struct sceHTTPClient *c)
{
	c->ssl_errnum = 0;
	return 0;
}


/* Creates the SSL context of the connection, sets certificate and key,
   installs the callbacks, binds a BIO to the socket and runs the handshake.
   Returns 0, or -1 with c->ssl_errnum set (-2003 setup, -2001 handshake). */
int sceHTTPSOpen(struct sceHTTPClient *c)
{
	int ret;
	int done;
	BIO *bio;

	done = 0;
	c->ssl_ctx = SSL_CTX_new(g_https_method);
	if (c->ssl_ctx == 0) {
		fprintf(stderr, "SSL_CTX_new() failed\n");
		ret = -2003;
		goto closeFail;
	}
	SSL_CTX_ctrl(c->ssl_ctx, SSL_CTRL_OPTIONS, 0xffff, NULL);

	if (g_https_mode == 2 && g_https_certfile[0] != 0) {
		fprintf(stderr, "cert file = %s\n", g_https_certfile);
		if (SSL_CTX_use_certificate_file(c->ssl_ctx, g_https_certfile, 1) == 0) {
			fprintf(stderr, "cert file import failed\n");
			ret = -2003;
			goto closeFail;
		}
	} else if (g_https_mode == 1 && g_https_cacert != 0) {
		if (SSL_CTX_use_certificate(c->ssl_ctx, g_https_cacert) == 0) {
			fprintf(stderr, "user cert importing failed\n");
			ret = -2003;
			goto closeFail;
		}
	}

	if (g_https_mode == 2 && g_https_keyfile[0] != 0) {
		fprintf(stderr, "key file = %s\n", g_https_keyfile);
		if (SSL_CTX_use_PrivateKey_file(c->ssl_ctx, g_https_keyfile, 1) == 0) {
			fprintf(stderr, "key file import failed\n");
			ret = -2003;
			goto closeFail;
		}
	} else if (g_https_mode == 1 && g_https_privkey != 0) {
		if (SSL_CTX_use_PrivateKey(c->ssl_ctx, g_https_privkey) == 0) {
			fprintf(stderr, "user private key importing failed\n");
			ret = -2003;
			goto closeFail;
		}
	}

	if (c->ssl_flags & 1) {
		SSL_CTX_set_verify(c->ssl_ctx, 5, 0);
		SSL_CTX_set_app_verify_cb(c->ssl_ctx, https_verify_chain, (char *)c);
	} else {
		SSL_CTX_set_verify(c->ssl_ctx, 0, 0);
	}
	SSL_CTX_ctrl(c->ssl_ctx, SSL_CTRL_OPTIONS, 0x40000, NULL);

	c->ssl = SSL_new(c->ssl_ctx);
	if (c->ssl == 0) {
		fprintf(stderr, "SSL_new() failed\n");
		ret = -2003;
		goto closeFail;
	}
	SSL_set_info_cb(c->ssl, https_info_cb);
	SSL_set_alert_info_cb(c->ssl, https_alert_cb, 0);
	SSL_set_connect_state(c->ssl);

	bio = BIO_new_socket(c->sock, 0);
	if (bio == 0) {
		fprintf(stderr, "BIO_new_socket() failed.\n");
		ret = -2003;
		goto closeFail;
	}
	BIO_set_cb(bio, (long (*)())https_accum);
	SSL_set_bio(c->ssl, bio, bio);

	if (c->ssl_flags & 0x40)
		fprintf(stderr, "SSL Handshake Start!!\n");
	c->ssl_errnum = -2001;

	do {
		switch (SSL_get_error(c->ssl, SSL_do_handshake(c->ssl))) {
		case 0:
			done = 1;
			break;
		case 1:
			fprintf(stderr, "SSL Handshake failure. err = 0x%x\n", ERR_get_error());
			if (c->ssl_flags & 1)
				ret = c->ssl_errnum;
			else
				ret = -2001;
			goto closeFail;
		case 5:
			fprintf(stderr, "System call error\n");
			ret = -2003;
			goto closeFail;
		case 6:
			fprintf(stderr, "socket closed\n");
			break;
		case 2:
		case 3:
		case 4:
		case 7:
		default:
			ret = -2003;
			goto closeFail;
		}
	} while (done == 0);

	c->ssl_errnum = 0;
	if (c->ssl_flags & 0x40)
		fprintf(stderr, "SSL Handshake Completed!!\n");
	return 0;

closeFail:
	sceHTTPSClose(c);
	c->ssl_errnum = ret;
	return -1;
}

int sceHTTPSSend(struct sceHTTPClient *c, const void *buf, int len)
{
	int result = SSL_write(c->ssl, (char *)buf, len);

	if (result < 0) {
		int err = ERR_get_error();

		fprintf(stderr, "SSL_write() failed. err = 0x%x\n", err);
		c->ssl_errnum = -2002;
	}
	return result;
}

/* Reads len bytes, or up to end of file. Returns the count, or -1. */
/* clang-format off */
int ssl_rcv(s,buf,len)
void *s;
char *buf;
int len;
	{
	int left;
	int n;

	left=len;
	for (;;)
		{
		if (left <= 0) break;
		n=SSL_read(s,buf,left);
		if (n < 0)
			{
			fprintf(stderr,"SSL_read() failed. err = 0x%x\n",
				ERR_get_error());
			return(-1);
			}
		if (n == 0) break;
		left-=n;
		buf+=n;
		}
	return(len-left);
	}
/* clang-format on */

int sceHTTPSRecv(struct sceHTTPClient *c, void *buf, unsigned int len)
{
	int result = ssl_rcv(c->ssl, buf, len);

	if (result < 0)
		c->ssl_errnum = -2002;
	return result;
}

int sceHTTPGetSSLError(struct sceHTTPClient *c)
{
	return c->ssl_errnum;
}

int sceHTTPSClose(struct sceHTTPClient *c)
{
	if (c->ssl != 0) {
		SSL_shutdown(c->ssl);
		SSL_free(c->ssl);
		c->ssl = 0;
	}
	if (c->ssl_ctx != 0) {
		SSL_CTX_free(c->ssl_ctx);
		c->ssl_ctx = 0;
	}
	return 0;
}

int sceHTTPSTerminate(void)
{
	if (g_https_cert != 0) {
		int i;

		i = 0;
		while (i < g_https_ncert) {
			X509 *cert = g_https_cert[i];

			i++;
			if (cert != 0)
				X509_free(cert);
		}
		g_https_free(g_https_cert);
		g_https_cert = 0;
	}
	if (g_https_cacert != 0) {
		X509_free(g_https_cacert);
		g_https_cacert = 0;
	}
	if (g_https_privkey != 0) {
		EVP_PKEY_free(g_https_privkey);
		g_https_privkey = 0;
	}
	SSL_library_cleanup();
	return 0;
}

/* SSL info callback: records the protocol version, traces each client
   state change with one character and flushes the trace buffers. */
static void https_info_cb(SSL *ssl, int where, int ret)
{
	int state;

	if (strcmp(SSL_get_version(ssl), "SSLv2") == 0)
		g_https_sslver = 2;
	else if (strcmp(SSL_get_version(ssl), "SSLv3") == 0)
		g_https_sslver = 3;
	else if (strcmp(SSL_get_version(ssl), "TLSv1") == 0)
		g_https_sslver = 1;
	else
		g_https_sslver = -1;

	if (g_https_msg_len > 0 && g_https_trace_level == 3) {
		https_busy_wait();
		memset(g_https_msg, 0, 50);
		g_https_msg_len = 0;
	}

	state = SSL_state(ssl);
	if (g_https_last_state == state) {
		if (g_https_acc1_len != 0) {
			memset(g_https_acc1, 0, 256);
			g_https_acc1_len = 0;
		}
		return;
	}
	g_https_last_state = state;

	switch (state) {
	case 0x1110:
	case 0x1111: /* SSL3_ST_CW_CLNT_HELLO_A/B */
	case 0x1210:
	case 0x1211: /* SSL23_ST_CW_CLNT_HELLO_A/B */
		https_trace(' ');
		break;
	case 0x1120:
	case 0x1121: /* SSL3_ST_CR_SRVR_HELLO_A/B */
		https_trace('!');
		break;
	case 0x1130:
	case 0x1131: /* SSL3_ST_CR_CERT_A/B */
		https_trace('"');
		break;
	case 0x1150:
	case 0x1151: /* SSL3_ST_CR_CERT_REQ_A/B */
		https_trace('#');
		break;
	case 0x1140:
	case 0x1141: /* SSL3_ST_CR_KEY_EXCH_A/B */
		https_trace('%');
		break;
	case 0x1160:
	case 0x1161: /* SSL3_ST_CR_SRVR_DONE_A/B */
		https_trace('$');
		break;
	case 0x1170:
	case 0x1171:
	case 0x1172: /* SSL3_ST_CW_CERT_A/B/C */
		break;
	case 0x1180:
	case 0x1181: /* SSL3_ST_CW_KEY_EXCH_A/B */
		https_trace('&');
		break;
	case 0x1190:
	case 0x1191: /* SSL3_ST_CW_CERT_VRFY_A/B */
		https_trace('\'');
		break;
	case 0x11a0:
	case 0x11a1: /* SSL3_ST_CW_CHANGE_A/B */
		https_trace(0x10);
		break;
	case 0x11b0:
	case 0x11b1: /* SSL3_ST_CW_FINISHED_A/B */
		https_trace('(');
		break;
	case 0x11c0:
	case 0x11c1: /* SSL3_ST_CR_CHANGE_A/B */
		https_trace(0x11);
		break;
	case 0x11d0:
	case 0x11d1: /* SSL3_ST_CR_FINISHED_A/B */
		https_trace(')');
		break;
	case 0x1010:
	case 0x1011: /* SSL2_ST_SEND_CLIENT_HELLO_A/B */
		https_trace('0');
		break;
	case 0x1020:
	case 0x1021: /* SSL2_ST_GET_SERVER_HELLO_A/B */
		https_trace('1');
		break;
	case 0x1030:
	case 0x1031: /* SSL2_ST_SEND_CLIENT_MASTER_KEY_A/B */
		https_trace('2');
		break;
	case 0x1040:
	case 0x1041: /* SSL2_ST_SEND_CLIENT_FINISHED_A/B */
		https_trace('3');
		break;
	case 0x1050:
	case 0x1051:
	case 0x1052:
	case 0x1053:
		/* SSL2_ST_SEND_CLIENT_CERTIFICATE_A..D */
		break;
	case 0x1060:
	case 0x1061: /* SSL2_ST_GET_SERVER_VERIFY_A/B */
		https_trace('4');
		break;
	case 0x1090:   /* SSL2_ST_X509_GET_CLIENT_CERTIFICATE */
		break;
	case 0x1091:
		https_trace('6');
		break;
	case 0x1070:
	case 0x1071: /* SSL2_ST_GET_SERVER_FINISHED_A/B */
		https_trace('5');
		break;
	}

	if (g_https_acc1_len > 0) {
		if (g_https_trace_level == 3)
			https_busy_wait();
		if (g_https_acc1_len != 0) {
			memset(g_https_buf, 0, 256);
			g_https_buf_len = 0;
			memset(g_https_acc1, 0, 256);
			g_https_acc1_len = 0;
		}
	}
	if (g_https_acc2_len > 0) {
		if (g_https_trace_level == 3)
			https_busy_wait();
		if (g_https_acc2_len != 0) {
			memset(g_https_acc2, 0, 256);
			g_https_acc2_len = 0;
		}
	}
}

/* SSL alert callback: traces each alert with one character (64 + code
   when sending, 80 + code when receiving) and flushes the trace buffers. */
static void https_alert_cb(SSL *ssl, int where, int ret)
{
	if (!(where & 0x4000))
		return;
	if (where & 8) {
		switch (ret & 0xff) {
		case 0:
			https_trace(64);
			break;
		case 10:
			https_trace(65);
			break;
		case 20:
			https_trace(66);
			break;
		case 30:
			https_trace(67);
			break;
		case 40:
			https_trace(68);
			break;
		case 41:
			https_trace(69);
			g_https_alert_flag = 1;
			break;
		case 42:
			https_trace(70);
			break;
		case 43:
			https_trace(71);
			break;
		case 44:
			https_trace(72);
			break;
		case 45:
			https_trace(73);
			break;
		case 46:
			https_trace(74);
			break;
		case 47:
			https_trace(75);
			break;
		case 21:
			https_trace(128);
			break;
		case 22:
			https_trace(129);
			break;
		case 48:
			https_trace(130);
			break;
		case 49:
			https_trace(131);
			break;
		case 50:
			https_trace(132);
			break;
		case 51:
			https_trace(133);
			break;
		case 60:
			https_trace(134);
			break;
		case 70:
			https_trace(135);
			break;
		case 71:
			https_trace(136);
			break;
		case 80:
			https_trace(137);
			break;
		case 90:
			https_trace(138);
			break;
		case 100:
			https_trace(139);
			break;
		case 1:
			https_trace(96);
			break;
		case 2:
			https_trace(97);
			g_https_alert_flag = 1;
			break;
		case 4:
			https_trace(98);
			break;
		case 6:
			https_trace(99);
			break;
		}
	}
	if (where & 4) {
		switch (ret & 0xff) {
		case 0:
			https_trace(80);
			break;
		case 10:
			https_trace(81);
			break;
		case 20:
			https_trace(82);
			break;
		case 30:
			https_trace(83);
			break;
		case 40:
			https_trace(84);
			break;
		case 41:
			https_trace(85);
			break;
		case 42:
			https_trace(86);
			break;
		case 43:
			https_trace(87);
			break;
		case 44:
			https_trace(88);
			break;
		case 45:
			https_trace(89);
			break;
		case 46:
			https_trace(90);
			break;
		case 47:
			https_trace(91);
			break;
		case 21:
			https_trace(144);
			break;
		case 22:
			https_trace(145);
			break;
		case 48:
			https_trace(146);
			break;
		case 49:
			https_trace(147);
			break;
		case 50:
			https_trace(148);
			break;
		case 51:
			https_trace(149);
			break;
		case 60:
			https_trace(150);
			break;
		case 70:
			https_trace(151);
			break;
		case 71:
			https_trace(152);
			break;
		case 80:
			https_trace(153);
			break;
		case 90:
			https_trace(154);
			break;
		case 100:
			https_trace(155);
			break;
		case 1:
			https_trace(112);
			break;
		case 2:
			https_trace(113);
			break;
		case 4:
			https_trace(114);
			break;
		case 6:
			https_trace(115);
			break;
		}
	}
	if (g_https_sslver == 2) {
		if (g_https_msg_len > 0) {
			if (g_https_trace_level == 3)
				https_busy_wait();
			memset(g_https_msg, 0, 50);
			g_https_msg_len = 0;
		}
		return;
	}
	if (g_https_acc1_len > 0 && (where & 4)) {
		if (g_https_trace_level == 3)
			https_busy_wait();
		memset(g_https_acc1, 0, 256);
		g_https_acc1_len = 0;
		return;
	}
	if (g_https_acc2_len > 0 && (where & 8)) {
		if (g_https_trace_level == 3)
			https_busy_wait();
		memset(g_https_acc2, 0, 256);
		g_https_acc2_len = 0;
	}
}

/* Returns 0 if the commonName of the certificate subject is host, else -1. */
static int https_match_hostname(const char *host, X509 *cert)
{
	unsigned char *oid;
	int oidlen;
	int c;
	char *val;
	int d;
	X509_NAME *name;
	int n;
	int i;
	X509_NAME_ENTRY *e;
	const char *s;

	name = X509_get_subject_name(cert);
	n = SSLCERT_NAME_get_entry_count(name);
	for (i = 0; i < n; i++) {
		e = SSLCERT_NAME_get_entry(name, i);
		if (SSLCERT_NAME_ENTRY_get_info(e, 0, &oid, &oidlen, &c, &val, &d)) {
			s = SSLCERT_OID_to_string(oid, oidlen, 0);
			if (strcmp(s ? s : "<UNKNOWN>", "commonName") == 0)
				break;
		}
	}
	if (i == n)
		i = -1;
	else
		i = strcmp(val, host) ? -1 : 0;
	return i;
}

/* Converts an ASN1 time to "YYYYMMDDHHMMSS": UTCTime (13 bytes) gets the
   century from its first digit (< '5' -> 20), GeneralizedTime (15 bytes)
   is copied. Returns 0, or -1. */
static int https_asn1_time_to_str(char *out, int type, const char *data, int len)
{
	if (type == 23) {
		if (len != 13) {
			fprintf(stderr, "invalid UTC time len. len = %d\n", len);
			return -1;
		}
		if ((unsigned char)data[0] >= '5') {
			out[0] = '1';
			out[1] = '9';
		} else {
			out[0] = '2';
			out[1] = '0';
		}
		strncpy(out + 2, data, 12);
	} else {
		if (len != 15) {
			fprintf(stderr, "invalid GEN time len. len = %d\n", len);
			return -1;
		}
		strncpy(out, data, 14);
	}
	out[14] = 0;
	return 0;
}

static int https_match_hostname(const char *host, X509 *cert);

/* Internal error (-2003). */
#define HTTPS_SET_INTERNAL_ERR(p)	do { *(p) = -2003; } while (0)

/* Checks the validity dates of the received certificates against the
   current time (flags 0x10 = notBefore, 0x08 = notAfter; 0x20 = take the
   dates of the matching loaded CA instead). Returns 0, 10 if out of date
   (*err = -2005 not yet valid, -2004 expired), or -1 (*err = -2003). */
static int https_check_expiration(X509 **certs, int ncert, int flags, int *err)
{
	char now[16];
	char date[16];
	long t;
	int type;
	char *data;
	int len;
	struct tm *tm;
	ASN1_UTCTIME *at;
	X509_NAME *name;
	int i;
	int j = 0;
	int local = 0;
	int ret;

	if (!(flags & 0x18))
		return 0;
	t = time(0);
	if (t == -1) {
		fprintf(stderr, "time() failed. errno = %d\n", errno);
		ret = -1;
		HTTPS_SET_INTERNAL_ERR(err);
		goto out;
	}
	tm = gmtime(&t);
	now[0] = '2';
	now[1] = '0';
	now[2] = '0' + (tm->tm_year + 1900) % 100 / 10;
	now[3] = '0' + (tm->tm_year + 1900) % 100 % 10;
	now[4] = '0' + (tm->tm_mon + 1) / 10;
	now[5] = '0' + (tm->tm_mon + 1) % 10;
	now[6] = '0' + tm->tm_mday / 10;
	now[7] = '0' + tm->tm_mday % 10;
	now[8] = '0' + tm->tm_hour / 10;
	now[9] = '0' + tm->tm_hour % 10;
	now[10] = '0' + tm->tm_min / 10;
	now[11] = '0' + tm->tm_min % 10;
	now[12] = '0' + tm->tm_sec / 10;
	now[13] = '0' + tm->tm_sec % 10;
	now[14] = 0;
	for (i = 0; i < 14; i++)
		;  /* redundant as written */

	for (i = 0; i < ncert; i++) {
		if (flags & 0x20) {
			j = 0;
			name = SSLCERT_get_subject_name(certs[i]);
			for (j = 0; j < g_https_ncert; j++)
				if (X509_NAME_cmp(X509_get_subject_name(g_https_cert[j]), name) == 0)
					break;
			if (j != g_https_ncert)
				local = 1;
		}
		if (flags & 0x10) {
			if (local > 0) {
				at = X509_get_notBefore(g_https_cert[j]);
				if (https_asn1_time_to_str(date, at->type, (char *)at->data, at->length) < 0) {
					ret = -1;
					*err = -2003;
					goto out;
				}
			} else {
				if (SSLCERT_get_notBefore(certs[i], &type, &data, &len) != 1) {
					ret = -1;
					HTTPS_SET_INTERNAL_ERR(err);
					goto out;
				}
				if (https_asn1_time_to_str(date, type, data, len) < 0) {
					ret = -1;
					HTTPS_SET_INTERNAL_ERR(err);
					goto out;
				}
			}
			fprintf(stderr, "current time: %s, %s cert's not_before: %s\n", now,
			    local > 0 ? "local" : "recved", date);
			if (memcmp(now, date, strlen(now)) < 0) {
				ret = 10;
				*err = -2005;
				goto out;
			}
		}
		if (flags & 0x08) {
			if (local > 0) {
				at = X509_get_notAfter(g_https_cert[j]);
				if (https_asn1_time_to_str(date, at->type, (char *)at->data, at->length) < 0) {
					ret = -1;
					*err = -2003;
					goto out;
				}
			} else if (SSLCERT_get_notAfter(certs[i], &type, &data, &len) != 1
			    || https_asn1_time_to_str(date, type, data, len) < 0) {
				ret = -1;
				HTTPS_SET_INTERNAL_ERR(err);
				goto out;
			}
			fprintf(stderr, "current time: %s, %s cert's not_after : %s\n", now,
			    local > 0 ? "local" : "recved", date);
			if (memcmp(now, date, strlen(now)) > 0) {
				ret = 10;
				*err = -2004;
				goto out;
			}
		}
	}
	return 0;
out:
	return ret;
}

/* SSL-C app-verify callback: checks expiry, the issuer against the loaded
   CAs (-2006), the host name (-2007) and the signature (-2001). Returns 1
   only if the signature verifies, else 0 with conn->ssl_errnum set. */
static int https_verify_chain(void *unused, void *chain, int ncert, struct sceHTTPClient *conn, int *out)
{
	X509 **certs;
	int err = 0;
	int i;
	int j;
	X509_NAME *issuer;
	EVP_PKEY *pkey;
	int r;

	if (ncert > 0) {
		certs = (X509 **)chain;
	} else {
		fprintf(stderr, "No Certificate!!\n");
		HTTPS_SET_INTERNAL_ERR(&err);
		goto out;
	}

	if (conn->ssl_flags & 0x40) {
		i = 0;
		if (ncert > 0) {
			do {
				fprintf(stderr, "recved cert %d:\n", i);
				https_dump_cert(certs[i]);
				fprintf(stderr, "\n");
			} while (++i < ncert);
		}
	}

	if (conn->ssl_flags & 0x18) {
		r = https_check_expiration(certs, ncert, conn->ssl_flags, &err);
		if (r == -1)
			goto out;
		if (r != 0) {
			fprintf(stderr, "expiration check failed.\n");
			fprintf(stderr, "ssl_errnum = %d\n", err);
			*out = r;
			goto out;
		}
	}

	for (i = 0; i < ncert; i++) {
		issuer = X509_get_issuer_name(certs[i]);
		for (j = 0; j < g_https_ncert; j++) {
			if (X509_NAME_cmp(issuer, X509_get_subject_name(g_https_cert[j])) == 0)
				goto found;
		}
	}
	fprintf(stderr, "Could not match client certificate to Server CA!!\n");
	err = -2006;
	goto out;

found:
	pkey = X509_get_pubkey(g_https_cert[j]);
	if (conn->ssl_flags & 4) {
		if (https_match_hostname(conn->uri->host, certs[0]) < 0) {
			fprintf(stderr, "Could not match cert's commonName to it's hostname.\n");
			err = -2007;
			goto out;
		}
	}
	if (X509_verify(certs[i], pkey) != 0)
		return 1;
	fprintf(stderr, "X509 verification failed.\n");
	err = -2001;
out:
	conn->ssl_errnum = err;
	return 0;
}

/* BIO callback: appends len bytes of src to accumulator 1 (type 130) or 2
   (type 131), up to 255 bytes. Does nothing if ret < 0; returns ret. */
static int https_accum(void *unused1, int type, const char *src, int len, int unused5, int ret)
{
	int n;

	if (ret < 0)
		return ret;

	if (type == 130) {
		n = g_https_acc1_len;
		if (n >= 255)
			return ret;
		if (n + len < 255) {
			memcpy(g_https_acc1 + n, src, len);
			g_https_acc1_len = g_https_acc1_len + len;
		} else {
			memcpy(g_https_acc1 + n, src, 255 - n);
			g_https_acc1_len = 255;
		}
	} else if (type == 131) {
		n = g_https_acc2_len;
		if (n >= 255)
			return ret;
		if (n + len < 255) {
			memcpy(g_https_acc2 + n, src, len);
			g_https_acc2_len = g_https_acc2_len + len;
		} else {
			memcpy(g_https_acc2 + n, src, 255 - n);
			g_https_acc2_len = 255;
		}
	}
	return ret;
}

static void https_busy_wait(void)
{
	int i;

	if (g_https_acc2_len > 0)
		for (i = 0; i < g_https_acc2_len; i++)  /* redundant as written */
			;
	if (g_https_acc1_len > 0)
		for (i = 0; i < g_https_acc1_len; i++)  /* redundant as written */
			;
}

static void https_trace(int c)
{}

static void https_dump_name(X509_NAME *name);

/* Debug dump of a certificate on stderr. */
static void https_dump_cert(X509 *x)
{
	char buf[64];
	X509_NAME *name;

	fprintf(stderr, "      Certificate version: %d\n", X509_get_version(x) + 1);
	fprintf(stderr, "      Valid from:%s - ", X509_get_notBefore_str(x));
	if (X509_get_notBefore_buf(x, buf, 64))
		fprintf(stderr, "%s\n", buf);
	fprintf(stderr, "      Valid to  :%s - ", X509_get_notAfter_str(x));
	if (X509_get_notAfter_buf(x, buf, 64))
		fprintf(stderr, "%s\n", buf);
	fprintf(stderr, "      Public key is %d bits\n", X509_get_pubkey_bits(x));
	X509_get_serialNumber_buf(x, buf, 64);
	fprintf(stderr, "      The Serial Number is:%s\n", buf);
	name = X509_get_issuer_name(x);
	fprintf(stderr, "      The issuer name is\n");
	https_dump_name(name);
	name = X509_get_subject_name(x);
	fprintf(stderr, "      The subject name is\n");
	https_dump_name(name);
	fprintf(stderr, "      Extension Count: %d\n", X509_get_ext_count(x));
}

/* Debug dump of an X509_NAME on stderr. */
static void https_dump_name(X509_NAME *name)
{
	char buf[512];
	char oid[128];
	int n;
	int i;
	X509_NAME_ENTRY *e;
	const char *v;

	X509_NAME_oneline(name, buf, 512);
	fprintf(stderr, "      %s\n", buf);
	n = X509_NAME_get_entry_count(name);
	for (i = 0; i < n; i++) {
		e = X509_NAME_get_entry(name, i);
		fprintf(stderr, "\t");
		oid[0] = 0;
		X509_NAME_ENTRY_get_oid_buf(e, oid, 128);
		v = X509_NAME_ENTRY_get_data_ptr(e);
		fprintf(stderr, "%s:%s\n", oid, v ? v : "");
	}
}
