/*
 * DNAS2 "pwrap" transport: plain or TLS socket to the DNAS server, with the
 * errno to DNAS error code (-6xx) translation and the TLS certificate checks.
 */

#include <sys/select.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "dnas_err.h"
#include "dnas1core.h"
#include "sslcert.h"
#include "dnas_pwrap.h"
#include "scednas1_state.h"
#include "thread/scesema.h"

/* The six root CAs of the pwrap: X.509 subject name and RSA public key of
 * each, in DER (data assets, extracted at build time). */
unsigned char _sce_dnas2_debugserver_subject_name[172] = {
#include "../../assets/dnas/dnas2_ca_debugserver_subject_name.inc"
};
unsigned char _sce_dnas2_debugserver_public_key[294] = {
#include "../../assets/dnas/dnas2_ca_debugserver_public_key.inc"
};
unsigned char _sce_dnas2_testca_subject_name[78] = {
#include "../../assets/dnas/dnas2_ca_testca_subject_name.inc"
};
unsigned char _sce_dnas2_testca_public_key[94] = {
#include "../../assets/dnas/dnas2_ca_testca_public_key.inc"
};
unsigned char _sce_dnas2_verisignclass1_subject_name[97] = {
#include "../../assets/dnas/dnas2_ca_verisignclass1_subject_name.inc"
};
unsigned char _sce_dnas2_verisignclass1_public_key[162] = {
#include "../../assets/dnas/dnas2_ca_verisignclass1_public_key.inc"
};
unsigned char _sce_dnas2_verisignclass2_subject_name[97] = {
#include "../../assets/dnas/dnas2_ca_verisignclass2_subject_name.inc"
};
unsigned char _sce_dnas2_verisignclass2_public_key[162] = {
#include "../../assets/dnas/dnas2_ca_verisignclass2_public_key.inc"
};
unsigned char _sce_dnas2_verisignclass3_subject_name[97] = {
#include "../../assets/dnas/dnas2_ca_verisignclass3_subject_name.inc"
};
unsigned char _sce_dnas2_verisignclass3_public_key[162] = {
#include "../../assets/dnas/dnas2_ca_verisignclass3_public_key.inc"
};
unsigned char _sce_dnas2_verisignss_subject_name[97] = {
#include "../../assets/dnas/dnas2_ca_verisignss_subject_name.inc"
};
unsigned char _sce_dnas2_verisignss_public_key[158] = {
#include "../../assets/dnas/dnas2_ca_verisignss_public_key.inc"
};

typedef struct {
	/* 0x00 */ unsigned char *der;
	/* 0x04 */ int derlen;
	/* 0x08 */ unsigned char *pub;
	/* 0x0C */ int publen;
	/* 0x10 */ X509_NAME *name;  /* decoded name, set at the first verification */
} PwrapRootCa;

#define PWRAP_ROOT_CA(ca) \
	{ _sce_dnas2_##ca##_subject_name, sizeof(_sce_dnas2_##ca##_subject_name), \
	  _sce_dnas2_##ca##_public_key, sizeof(_sce_dnas2_##ca##_public_key), 0 }

PwrapRootCa _sce_dnas2_CA_data[6] = {
	PWRAP_ROOT_CA(debugserver),
	PWRAP_ROOT_CA(testca),
	PWRAP_ROOT_CA(verisignclass3),
	PWRAP_ROOT_CA(verisignss),
	PWRAP_ROOT_CA(verisignclass1),
	PWRAP_ROOT_CA(verisignclass2),
};

static int g_pwrapStarted = 0;
static int g_pwrapFd = -1;
static int g_pwrapConnected = 0;
static int g_pwrapSecure = 1;
static SSL_CTX *g_pwrapCtx = 0;
static SSL *g_pwrapSsl = 0;
static int g_accALen = 0;
static int g_accBLen = 0;
static int g_pwrapNoCert = 0;
static int g_pwrapHsPending = 0;
static int g_pwrapRecPending = 0;
static int g_pwrapRootsReady = 0;
static int g_pwrapTraceMode = 2;
static BIO *g_pwrapBio = 0;

/* Proxy host name and port, negotiated TLS version, the two 255-byte message
 * accumulators, last handshake state and two trace buffers. */
static char g_pwrapHost[512];
static unsigned short g_pwrapPort;
static int g_pwrapSslVersion;
static char g_accA[256];
static char g_accB[256];
static int g_pwrapLastState;
static char g_pwrapHsBuf[52];
static char g_pwrapRecBuf[256];

/* file-local functions defined below */
static int DNAS2PwrapMapErrno(int e);
static int DNAS2PwrapSockReadAll(int fd, char *buf, int len);

static int pwrap_verify_chain(void *unused, X509 **certs, int n);

static int DNAS2PwrapCtxNew(void);
static int pwrap_connect(void);
static int DNAS2PwrapWrite(void *buf, int len);
static int DNAS2PwrapReadAll(char *buf, int len);
static int DNAS2PwrapDisconnect(void);
static int DNAS2PwrapCleanup(void);
static int DNAS2PwrapNop(void);
static int DNAS2PwrapSetProxy(char *host, unsigned short port);
static int DNAS2PwrapSetFlag(int v);
static int DNAS2PwrapMapHerrno(int code);
static void DNAS2BusyWait(void);

/* Fills the table of the nine transport operations. */
int _sce_dnas2_init_netglue_table(void **table)
{
	table[0] = (void *)DNAS2PwrapCtxNew;
	table[1] = (void *)pwrap_connect;
	table[2] = (void *)DNAS2PwrapWrite;
	table[3] = (void *)DNAS2PwrapReadAll;
	table[4] = (void *)DNAS2PwrapDisconnect;
	table[5] = (void *)DNAS2PwrapCleanup;
	table[6] = (void *)DNAS2PwrapNop;
	table[7] = (void *)DNAS2PwrapSetProxy;
	table[8] = (void *)DNAS2PwrapSetFlag;
	return 0;
}

/* Creates the TLS context. */
static int DNAS2PwrapCtxNew(void)
{
	int r;

	g_pwrapConnected = 0;
	if (g_pwrapSecure == 0)
		return 0;

	g_pwrapCtx = SSL_CTX_new(SSLv23_client_method());
	if (g_pwrapCtx == 0) {
		r = SCE_DNAS_ERR_SSL;
	} else {
		SSL_CTX_ctrl(g_pwrapCtx, SSL_CTRL_OPTIONS, 0xffff, NULL);
		SSL_CTX_set_verify(g_pwrapCtx, 5, 0);
		SSL_CTX_set_app_verify_cb(g_pwrapCtx, pwrap_verify_chain, 0);
		r = 0;
	}
	return r;
}

/* Waits for a non-blocking connect() to complete: raises in_connect, then
 * waits in select() with a 100 ms timeout until the socket is readable or
 * writable and reports SO_ERROR in errno. Between rounds checks abort_sema and
 * returns EINTR if it is signalled. in_connect is cleared on exit. */
static int pwrap_connect_wait(int fd)
{
	fd_set rfds;
	fd_set wfds;
	struct timeval tv;
	int soerr;
	int solen;
	struct timeval *tvp;
	int nfds;
	int r;

	_sce_dnas1_common.in_connect = 1;
	if (_sce_dnas1_abort_called() > 0) {
		errno = EINTR;
		r = -1;
		goto out;
	}
	nfds = fd + 1;
	tvp = &tv;
	for (;;) {
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		FD_ZERO(&wfds);
		FD_SET(fd, &wfds);
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		r = select(nfds, &rfds, &wfds, 0, tvp);
		if (r < 0) {
			perror("select()");
			goto out;
		}
		if (r > 0) {
			if (FD_ISSET(fd, &rfds) || FD_ISSET(fd, &wfds)) {
				solen = 4;
				soerr = 0;
				r = getsockopt(fd, 0xffff, 4103, &soerr, &solen);
				if (r < 0) {
					perror("getsockopt()");
					goto out;
				}
				if (soerr == 0)
					goto ok;
				perror("connect()");
				errno = soerr;
				r = -1;
				goto out;
			}
		}
		if (sce_sema_pollsema(_sce_dnas1_common.abort_sema) >= 0) {
			errno = EINTR;
			r = -1;
			goto out;
		}
	}
ok:
	r = 0;
out:
	_sce_dnas1_common.in_connect = 0;
	return r;
}

static void pwrap_trace_state(SSL *ssl, int where, int ret);
static void pwrap_info_cb(SSL *ssl, int where, int ret);
static int pwrap_accum(int unused1, int type, const void *src, int len, int unused2, int r);

/* Connect to the pwrap host: resolve the name (inet_addr, then
 * gethostbyname_r), try the addresses of h_addr_list in order with a
 * non-blocking connect(), make the socket blocking again and, for TLS, create
 * the session and run the handshake. */
static int pwrap_connect(void)
{
	struct sockaddr_in sin;
	struct hostent hent;
	char buf[2048];
	struct hostent *hp;
	int herr;
	unsigned int addr;
	int i;
	int fl;
	int r;
	int done;
	int n;

	hp = 0;
	done = 0;
	if (g_pwrapConnected != 0)
		return -1;

	addr = inet_addr(g_pwrapHost);
	if (addr != (unsigned int)-1) {
		sin.sin_family = 2;
		sin.sin_addr.s_addr = addr;
	} else {
		if (gethostbyname_r(g_pwrapHost, &hent, buf, 2048, &hp, &herr) != 0) {
			perror("gethostbyname_r");
			r = DNAS2PwrapMapHerrno(herr);
			goto out;
		}
		sin.sin_family = hp->h_addrtype;
		memcpy(&sin.sin_addr, hp->h_addr_list[0], hp->h_length);
	}
	i = 0;
	sin.sin_port = (g_pwrapPort >> 8) | ((g_pwrapPort & 0xff) << 8);
	for (;;) {
		g_pwrapFd = -1;
		g_pwrapFd = socket(AF_INET, SOCK_STREAM, 0);
		if (g_pwrapFd < 0) {
			perror("socket()");
			r = DNAS2PwrapMapErrno(errno);
			goto out;
		}
		fl = fcntl(g_pwrapFd, F_GETFL, 0);
		if (fl < 0) {
			perror("fcntl()");
			r = DNAS2PwrapMapErrno(errno);
			goto out;
		}
		if (fcntl(g_pwrapFd, F_SETFL, fl | O_NONBLOCK) < 0) {
			perror("fcntl()");
			r = DNAS2PwrapMapErrno(errno);
			goto out;
		}
		n = connect(g_pwrapFd, (struct sockaddr *)&sin, 16);
		if (n == 0)
			goto connected;
		if (n < 0) {
			if (errno == EINPROGRESS && pwrap_connect_wait(g_pwrapFd) == 0)
				goto connected;
			if (errno == EINTR || addr != (unsigned int)-1 || hp->h_addr_list[++i] == 0) {
				perror("connect");
				r = DNAS2PwrapMapErrno(errno);
				goto out;
			}
		}
		close(g_pwrapFd);
		sin.sin_family = hp->h_addrtype;
		sin.sin_port = (g_pwrapPort >> 8) | ((g_pwrapPort & 0xff) << 8);
		memcpy(&sin.sin_addr, hp->h_addr_list[i], 4);
	}
connected:
	g_pwrapConnected = 1;
	fl = fcntl(g_pwrapFd, F_GETFL, 0);
	if (fl < 0) {
		perror("fcntl()");
		r = DNAS2PwrapMapErrno(errno);
		goto out;
	}
	if (fcntl(g_pwrapFd, F_SETFL, fl & ~O_NONBLOCK) < 0) {
		perror("fcntl()");
		r = DNAS2PwrapMapErrno(errno);
		goto out;
	}
	if (g_pwrapSecure == 0)
		return 0;

	SSL_CTX_ctrl(g_pwrapCtx, SSL_CTRL_OPTIONS, 0x40000, NULL);
	g_pwrapSsl = SSL_new(g_pwrapCtx);
	if (g_pwrapSsl == 0)
		goto sslfail;
	SSL_set_info_cb(g_pwrapSsl, pwrap_trace_state);
	SSL_set_alert_info_cb(g_pwrapSsl, pwrap_info_cb, 0);
	SSL_set_connect_state(g_pwrapSsl);
	g_pwrapBio = BIO_new_socket(g_pwrapFd, 0);
	if (g_pwrapBio == 0) {
sslfail:
		DNAS2PwrapDisconnect();
		return SCE_DNAS_ERR_SSL;
	}
	BIO_set_cb(g_pwrapBio, (long (*)())pwrap_accum);
	SSL_set_bio(g_pwrapSsl, g_pwrapBio, g_pwrapBio);
	do {
		switch (SSL_get_error(g_pwrapSsl, SSL_do_handshake(g_pwrapSsl))) {
		case 0:
			done = 1;
			break;
		case 6:
			break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 7:
		default:
			r = errno;
			DNAS2PwrapDisconnect();
			if (r == 4)
				return SCE_DNAS_ERR_ABORTED;
			return SCE_DNAS_ERR_SSL;
		}
	} while (done == 0);
	return 0;

out:
	if (g_pwrapFd >= 0) {
		close(g_pwrapFd);
		g_pwrapFd = -1;
	}
	return r;
}

/* Writes once, on TLS or on the plain socket. */
static int DNAS2PwrapWrite(void *buf, int len)
{
	int r;

	if (g_pwrapSecure != 0) {
		r = SSL_write(g_pwrapSsl, buf, len);
		if (r < 0) {
			int en = errno ^ 4;
			int e = SCE_DNAS_ERR_ABORTED;

			if (en)
				e = SCE_DNAS_ERR_SSL;
			return e;
		}
	} else {
		r = write(g_pwrapFd, buf, len);
		if (r < 0)
			return DNAS2PwrapMapErrno(errno);
	}
	return r;
}

/* Reads len bytes from the TLS socket (SSL_read until len or EOF). */
static int DNAS2PwrapTlsReadAll(SSL *ssl, char *buf, int len)
{
	int left = len;
	int n;
	int eintr = SCE_DNAS_ERR_ABORTED;

	while (left > 0) {
		n = SSL_read(ssl, buf, left);
		if (n < 0) {
			int r = eintr;

			if (errno != EINTR)
				r = SCE_DNAS_ERR_SSL;
			return r;
		}
		if (n == 0)
			break;
		left -= n;
		buf += n;
	}
	return len - left;
}

/* Same on the plain socket, with the full errno translation. */
static int DNAS2PwrapSockReadAll(int fd, char *buf, int len)
{
	int left = len;
	int n;

	while (left > 0) {
		n = read(fd, buf, left);
		if (n < 0)
			return DNAS2PwrapMapErrno(errno);
		if (n == 0)
			break;
		left -= n;
		buf += n;
	}
	return len - left;
}

/* Reads everything, on TLS or on the plain socket. */
static int DNAS2PwrapReadAll(char *buf, int len)
{
	int r;

	if (g_pwrapSecure != 0) {
		r = DNAS2PwrapTlsReadAll(g_pwrapSsl, buf, len);
		if (r < 0)
			return r;
	} else {
		r = DNAS2PwrapSockReadAll(g_pwrapFd, buf, len);
		if (r < 0)
			return r;
	}
	return r;
}

/* Closes the transport. */
static int DNAS2PwrapDisconnect(void)
{
	if (g_pwrapSecure != 0) {
		if (g_pwrapConnected != 0) {
			SSL_shutdown(g_pwrapSsl);
			if (g_pwrapSsl != 0)
				SSL_free(g_pwrapSsl);
		}
	}
	if (g_pwrapFd >= 0) {
		close(g_pwrapFd);
		g_pwrapFd = -1;
	}
	g_pwrapConnected = 0;
	return 0;
}

static int DNAS2PwrapCleanup(void)
{
	if (g_pwrapSecure != 0) {
		if (g_pwrapCtx != 0)
			SSL_CTX_free(g_pwrapCtx);
	}
	return 0;
}

/* Empty entry of the netglue table (slot 6). */
static int DNAS2PwrapNop(void)
{
	return 0;
}

/* errno -> DNAS error code. */
static int DNAS2PwrapMapErrno(int e)
{
	int r;

	switch (e) {
	case EINTR:
	case ECONNABORTED:
		r = SCE_DNAS_ERR_ABORTED;
		break;
	case EAGAIN:
	case ENOMEM:
	case ENFILE:
	case ENOBUFS:
		r = SCE_DNAS_ERR_NO_RESOURCES;
		break;
	case EMFILE:
		r = SCE_DNAS_ERR_MFILE;
		break;
	case EFAULT:
	case EINVAL:
	case EPROTONOSUPPORT:
	case EAFNOSUPPORT:
		r = SCE_DNAS_ERR_INVALID;
		break;
	case EBADF:
	case ENOTSOCK:
		r = SCE_DNAS_ERR_BAD_SOCKET;
		break;
	case EISCONN:
	case ENOTCONN:
		r = SCE_DNAS_ERR_NOT_CONNECTED;
		break;
	case ECONNREFUSED:
		r = SCE_DNAS_ERR_CONNREFUSED;
		break;
	case ETIMEDOUT:
		r = SCE_DNAS_ERR_TIMEDOUT;
		break;
	case ENETUNREACH:
		r = SCE_DNAS_ERR_NETUNREACH;
		break;
	case EPERM:
	case EACCES:
	case EADDRINUSE:
		r = SCE_DNAS_ERR_NET;
		break;
	default:
		r = SCE_DNAS_ERR_NET;
		printf("unknown error value.\n");
		break;
	}
	return r;
}

/* Host-lookup error code -> DNAS error code. */
static int DNAS2PwrapMapHerrno(int code)
{
	int r;

	switch (code) {
	case -1:
		r = DNAS2PwrapMapErrno(errno);
		if (r == SCE_DNAS_ERR_NET)
			r = SCE_DNAS_ERR_RESOLVE;
		break;
	case HOST_NOT_FOUND:
		r = SCE_DNAS_ERR_HOST_NOT_FOUND;
		break;
	case TRY_AGAIN:
		r = SCE_DNAS_ERR_TRY_AGAIN;
		break;
	case NO_RECOVERY:
		r = SCE_DNAS_ERR_NO_RECOVERY;
		break;
	case NO_DATA:
		r = SCE_DNAS_ERR_NO_DATA;
		break;
	default:
		r = SCE_DNAS_ERR_RESOLVE;
		printf("unknown herror value.\n");
		break;
	}
	return r;
}

static int DNAS2PwrapSetProxy(char *host, unsigned short port)
{
	g_pwrapStarted = 1;
	strcpy(g_pwrapHost, host);
	g_pwrapPort = port;
	return 0;
}

static int DNAS2PwrapSetFlag(int v)
{
	g_pwrapSecure = v;
	return 0;
}

/* ------------------------------------------------------------------ *
 * TLS tracing: handshake state and alerts. 0x4000/8/4 are
 * SSL_CB_ALERT/_WRITE/_READ.
 * ------------------------------------------------------------------ */

static void pwrap_alert(int code);

/* Trace the handshake state. */
static void pwrap_trace_state(SSL *ssl, int where, int ret)
{
	int state;

	if (strcmp(SSL_get_version(ssl), "SSLv2") == 0)
		g_pwrapSslVersion = 2;
	else if (strcmp(SSL_get_version(ssl), "SSLv3") == 0)
		g_pwrapSslVersion = 3;
	else if (strcmp(SSL_get_version(ssl), "TLSv1") == 0)
		g_pwrapSslVersion = 1;
	else
		g_pwrapSslVersion = -1;
	if (g_pwrapHsPending > 0 && g_pwrapTraceMode == 3) {
		DNAS2BusyWait();
		memset(g_pwrapHsBuf, 0, 50);
		g_pwrapHsPending = 0;
	}
	state = SSL_state(ssl);
	if (g_pwrapLastState == state) {
		if (g_accALen != 0) {
			memset(g_accA, 0, 256);
			g_accALen = 0;
		}
		return;
	}
	g_pwrapLastState = state;
	switch (state) {
	case 0x1110:
	case 0x1111:
	case 0x1210:
	case 0x1211:
		pwrap_alert(32);
		break;
	case 0x1120:
	case 0x1121:
		pwrap_alert(33);
		break;
	case 0x1130:
	case 0x1131:
		pwrap_alert(34);
		break;
	case 0x1150:
	case 0x1151:
		pwrap_alert(35);
		break;
	case 0x1140:
	case 0x1141:
		pwrap_alert(37);
		break;
	case 0x1160:
	case 0x1161:
		pwrap_alert(36);
		break;
	case 0x1180:
	case 0x1181:
		pwrap_alert(38);
		break;
	case 0x1190:
	case 0x1191:
		pwrap_alert(39);
		break;
	case 0x11a0:
	case 0x11a1:
		pwrap_alert(16);
		break;
	case 0x11b0:
	case 0x11b1:
		pwrap_alert(40);
		break;
	case 0x11c0:
	case 0x11c1:
		pwrap_alert(17);
		break;
	case 0x11d0:
	case 0x11d1:
		pwrap_alert(41);
		break;
	case 0x1170:
	case 0x1171:
	case 0x1172:
		break;
	case 0x1010:
	case 0x1011:
		pwrap_alert(48);
		break;
	case 0x1020:
	case 0x1021:
		pwrap_alert(49);
		break;
	case 0x1030:
	case 0x1031:
		pwrap_alert(50);
		break;
	case 0x1040:
	case 0x1041:
		pwrap_alert(51);
		break;
	case 0x1050:
	case 0x1051:
	case 0x1052:
	case 0x1053:
		break;
	case 0x1060:
	case 0x1061:
		pwrap_alert(52);
		break;
	case 0x1091:
		pwrap_alert(54);
		break;
	case 0x1070:
	case 0x1071:
		pwrap_alert(53);
		break;
	case 0x1090:
		break;
	}
	if (g_accALen > 0) {
		if (g_pwrapTraceMode == 3)
			DNAS2BusyWait();
		if (g_accALen != 0) {
			memset(g_pwrapRecBuf, 0, 256);
			g_pwrapRecPending = 0;
			memset(g_accA, 0, 256);
			g_accALen = 0;
		}
	}
	if (g_accBLen > 0) {
		if (g_pwrapTraceMode == 3)
			DNAS2BusyWait();
		if (g_accBLen != 0) {
			memset(g_accB, 0, 256);
			g_accBLen = 0;
		}
	}
}

/* Info callback of the TLS session. */
static void pwrap_info_cb(SSL *ssl, int where, int ret)
{
	if (where & 0x4000) {
		if (where & 8) {
			switch (ret & 0xff) {
			case 0:
				pwrap_alert(64);
				break;
			case 10:
				pwrap_alert(65);
				break;
			case 20:
				pwrap_alert(66);
				break;
			case 30:
				pwrap_alert(67);
				break;
			case 40:
				pwrap_alert(68);
				break;
			case 41:
				pwrap_alert(69);
				g_pwrapNoCert = 1;
				break;
			case 42:
				pwrap_alert(70);
				break;
			case 43:
				pwrap_alert(71);
				break;
			case 44:
				pwrap_alert(72);
				break;
			case 45:
				pwrap_alert(73);
				break;
			case 46:
				pwrap_alert(74);
				break;
			case 47:
				pwrap_alert(75);
				break;
			case 21:
				pwrap_alert(128);
				break;
			case 22:
				pwrap_alert(129);
				break;
			case 48:
				pwrap_alert(130);
				break;
			case 49:
				pwrap_alert(131);
				break;
			case 50:
				pwrap_alert(132);
				break;
			case 51:
				pwrap_alert(133);
				break;
			case 60:
				pwrap_alert(134);
				break;
			case 70:
				pwrap_alert(135);
				break;
			case 71:
				pwrap_alert(136);
				break;
			case 80:
				pwrap_alert(137);
				break;
			case 90:
				pwrap_alert(138);
				break;
			case 100:
				pwrap_alert(139);
				break;
			case 1:
				pwrap_alert(96);
				break;
			case 2:
				pwrap_alert(97);
				g_pwrapNoCert = 1;
				break;
			case 4:
				pwrap_alert(98);
				break;
			case 6:
				pwrap_alert(99);
				break;
			}
		}
		if (where & 4) {
			switch (ret & 0xff) {
			case 0:
				pwrap_alert(80);
				break;
			case 10:
				pwrap_alert(81);
				break;
			case 20:
				pwrap_alert(82);
				break;
			case 30:
				pwrap_alert(83);
				break;
			case 40:
				pwrap_alert(84);
				break;
			case 41:
				pwrap_alert(85);
				break;
			case 42:
				pwrap_alert(86);
				break;
			case 43:
				pwrap_alert(87);
				break;
			case 44:
				pwrap_alert(88);
				break;
			case 45:
				pwrap_alert(89);
				break;
			case 46:
				pwrap_alert(90);
				break;
			case 47:
				pwrap_alert(91);
				break;
			case 21:
				pwrap_alert(144);
				break;
			case 22:
				pwrap_alert(145);
				break;
			case 48:
				pwrap_alert(146);
				break;
			case 49:
				pwrap_alert(147);
				break;
			case 50:
				pwrap_alert(148);
				break;
			case 51:
				pwrap_alert(149);
				break;
			case 60:
				pwrap_alert(150);
				break;
			case 70:
				pwrap_alert(151);
				break;
			case 71:
				pwrap_alert(152);
				break;
			case 80:
				pwrap_alert(153);
				break;
			case 90:
				pwrap_alert(154);
				break;
			case 100:
				pwrap_alert(155);
				break;
			case 1:
				pwrap_alert(112);
				break;
			case 2:
				pwrap_alert(113);
				break;
			case 4:
				pwrap_alert(114);
				break;
			case 6:
				pwrap_alert(115);
				break;
			}
		}
		if (g_pwrapSslVersion == 2) {
			if (g_pwrapHsPending > 0) {
				if (g_pwrapTraceMode == 3)
					DNAS2BusyWait();
				memset(g_pwrapHsBuf, 0, 50);
				g_pwrapHsPending = 0;
			}
		} else if (g_accALen > 0 && (where & 4)) {
			if (g_pwrapTraceMode == 3)
				DNAS2BusyWait();
			memset(g_accA, 0, 256);
			g_accALen = 0;
		} else if (g_accBLen > 0 && (where & 8)) {
			if (g_pwrapTraceMode == 3)
				DNAS2BusyWait();
			memset(g_accB, 0, 256);
			g_accBLen = 0;
		}
	}
}

/* Checks the commonName of the peer certificate against the expected host
 * name (g_pwrapHost). 0 if it matches, -1 otherwise or if there is none. */
static int pwrap_check_cert_cn(X509 *cert)
{
	char line[8192];
	unsigned char *oid;
	int oidlen;
	int val_type;
	char *val;
	int val_len;
	X509_NAME *name;
	X509_NAME_ENTRY *e;
	const char *s;
	int n;
	int i;

	name = X509_get_subject_name(cert);
	X509_NAME_oneline(name, line, 8192);
	n = SSLCERT_NAME_get_entry_count(name);
	for (i = 0; i < n; i++) {
		e = SSLCERT_NAME_get_entry(name, i);
		if (SSLCERT_NAME_ENTRY_get_info(e, 0, &oid, &oidlen, &val_type, &val, &val_len) != 0) {
			s = SSLCERT_OID_to_string(oid, oidlen, 0);
			s = (s != 0) ? s : "<UNKNOWN>";
			if (strcmp(s, "commonName") == 0)
				break;
		}
	}
	if (i == n)
		goto fail;
	return (strcmp(val, g_pwrapHost) == 0) ? 0 : -1;
fail:
	return -1;
}

static int pwrap_check_cert_cn(X509 *cert);

/* Verifies the certificate chain. At the first call decode the names of the
 * six root CAs; then walks up the chain until an issuer matches a root, checks
 * that signature with the root public key and, going back down, each
 * certificate with the key of the next one. */
static int pwrap_verify_chain(void *unused, X509 **certs, int n)
{
	EVP_PKEY *pkey;
	unsigned char *p;
	unsigned char *pk;
	X509_NAME **np;
	X509_NAME *issuer;
	X509_NAME *subj;
	PwrapRootCa *e;
	PwrapRootCa *b;
	unsigned char **pp;
	int i, k;
	unsigned int j;

	pkey = 0;
	if (n == 0)
		return 0;
	if (g_pwrapRootsReady == 0) {
		pp = &p;
		b = _sce_dnas2_CA_data;
		np = &b->name;
		e = b;
		do {
			p = e->der;
			if (SSLCERT_NAME_from_binary(np, pp, e->derlen) == 0)
				return 0;
			e++;
			np = (X509_NAME **)((char *)np + 20);
		} while (e < _sce_dnas2_CA_data + 6);
		g_pwrapRootsReady = 1;
	}
	i = 0;
	for (;;) {
		issuer = SSLCERT_get_issuer_name(certs[i]);
		for (j = 0; j < 6; j++)
			if (SSLCERT_NAME_cmp(_sce_dnas2_CA_data[j].name, issuer) == 0)
				goto found;
		if (!(i + 1 < n))
			return 0;
		subj = SSLCERT_get_subject_name(certs[i + 1]);
		if (SSLCERT_NAME_cmp(issuer, subj) != 0)
			return 0;
		i = i + 1;
	}
found:
	if (g_pwrapStarted == 0) {
		if (pwrap_check_cert_cn(certs[i]) < 0)
			goto freeout;
	}
	pk = _sce_dnas2_CA_data[j].pub;
	if (SSLCERT_PKEY_from_PUBKEY_binary(6, &pkey, &pk, _sce_dnas2_CA_data[j].publen) == 0)
		goto freeout;
	if (SSLCERT_verify(certs[i], pkey) == 0)
		goto freeout;
	SSLCERT_PKEY_free(pkey);
	pkey = 0;
	k = i;
	while (k > 0) {
		pkey = SSLCERT_get_pubkey(certs[k]);
		if (pkey == 0)
			return 0;
		if (SSLCERT_verify(certs[k - 1], pkey) == 0)
			goto freeout;
		pkey = 0;
		k--;
	}
	return 1;
freeout:
	if (pkey != 0) {
		SSLCERT_PKEY_free(pkey);
		pkey = 0;
	}
	return 0;
}

/* Appends len bytes to one of the two 255-byte accumulators (selected by type
 * 130/131), truncating at 255. Returns the sixth argument unchanged. */
static int pwrap_accum(int unused1, int type, const void *src, int len, int unused2, int r)
{
	int n;

	if (r < 0)
		return r;
	if (type == 130) {
		n = g_accALen;
		if (n >= 255)
			return r;
		if (n + len < 255) {
			memcpy(g_accA + n, src, len);
			g_accALen = g_accALen + len;
		} else {
			memcpy(g_accA + n, src, 255 - n);
			g_accALen = 255;
		}
	} else if (type == 131) {
		n = g_accBLen;
		if (n >= 255)
			return r;
		if (n + len < 255) {
			memcpy(g_accB + n, src, len);
			g_accBLen = g_accBLen + len;
		} else {
			memcpy(g_accB + n, src, 255 - n);
			g_accBLen = 255;
		}
	}
	return r;
}

/* Two empty delay loops, counting up to two global values. */
static void DNAS2BusyWait(void)
{
	int a = g_accBLen;
	int b = g_accALen;
	int i;

	if (a > 0)
		for (i = 0; i < a; i++)
			;
	if (b > 0)
		for (i = 0; i < b; i++)
			;
}

/* Trace point: does nothing. */
static void pwrap_alert(int code)
{}
