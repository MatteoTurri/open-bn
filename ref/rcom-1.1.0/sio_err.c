/*
 * ERR_load_SIO_strings and the function names of the SIO error library of RCOM.
 */
#include <stdio.h>
#include "cryptlib.h"


#define SIO_F_SIO_ACCEPT                   100
#define SIO_F_SIO_CONNECT                  101
#define SIO_F_SIO_GET_ACCEPT_SOCKET        102
#define SIO_F_SIO_GET_HOST_IP              103
#define SIO_F_SIO_GET_PORT                 104
#define SIO_F_SIO_GHBA_DEFAULT             105
#define SIO_F_SIO_GHBN_CACHE               106
#define SIO_F_SIO_GHBN_DEFAULT             107
#define SIO_F_SIO_SET_TCP_NODELAY          108
#define SIO_F_SIO_SOCKET_NBIO              109
#define SIO_F_SIO_SOCK_INIT                110
#define SIO_F_UDP_CONNECT                  111
#define SIO_F_UDP_GET_ACCEPT_SOCKET        112

#define SIO_R_ACCEPT_ERROR                 100
#define SIO_R_BAD_GETHOSTBYADDR            101
#define SIO_R_BAD_HOSTNAME_LOOKUP          102
#define SIO_R_CONNECT_ERROR                103
#define SIO_R_ERROR_SETTING_NBIO           104
#define SIO_R_GETHOSTBYNAME_ADDR_IS_NOT_AF_INET 105
#define SIO_R_INVALID_IP_ADDRESS           106
#define SIO_R_NETLIBIFGET                  131
#define SIO_R_NETLIBOPEN                   132
#define SIO_R_NO_HOSTHNAME_SPECIFIED       107
#define SIO_R_NO_PORT_DEFINED              108
#define SIO_R_NO_PORT_SPECIFIED            109
#define SIO_R_SYSLIBFIND                   133
#define SIO_R_UNABLE_TO_BIND_SOCKET        110
#define SIO_R_UNABLE_TO_CREATE_SOCKET      111
#define SIO_R_UNABLE_TO_LISTEN_SOCKET      112
#define SIO_R_WSASTARTUP                   124

static ERR_STRING_DATA SIO_str_functs[]=
	{
{ERR_PACK(0,SIO_F_SIO_ACCEPT,0),	"SIO_accept"},
{ERR_PACK(0,SIO_F_SIO_CONNECT,0),	"SIO_connect"},
{ERR_PACK(0,SIO_F_SIO_GET_ACCEPT_SOCKET,0),	"SIO_get_accept_socket"},
{ERR_PACK(0,SIO_F_SIO_GET_HOST_IP,0),	"SIO_get_host_ip"},
{ERR_PACK(0,SIO_F_SIO_GET_PORT,0),	"SIO_get_port"},
{ERR_PACK(0,SIO_F_SIO_GHBA_DEFAULT,0),	"SIO_ghba_default"},
{ERR_PACK(0,SIO_F_SIO_GHBN_CACHE,0),	"SIO_ghbn_cache"},
{ERR_PACK(0,SIO_F_SIO_GHBN_DEFAULT,0),	"SIO_ghbn_default"},
{ERR_PACK(0,SIO_F_SIO_SET_TCP_NODELAY,0),	"SIO_set_tcp_nodelay"},
{ERR_PACK(0,SIO_F_SIO_SOCKET_NBIO,0),	"SIO_socket_nbio"},
{ERR_PACK(0,SIO_F_SIO_SOCK_INIT,0),	"SIO_sock_init"},
{ERR_PACK(0,SIO_F_UDP_CONNECT,0),	"UDP_CONNECT"},
{ERR_PACK(0,SIO_F_UDP_GET_ACCEPT_SOCKET,0),	"UDP_GET_ACCEPT_SOCKET"},
{0,NULL},
	};

static ERR_STRING_DATA SIO_str_reasons[]=
	{
{SIO_R_ACCEPT_ERROR                ,"accept error"},
{SIO_R_BAD_GETHOSTBYADDR           ,"bad gethostbyaddr"},
{SIO_R_BAD_HOSTNAME_LOOKUP         ,"bad hostname lookup"},
{SIO_R_CONNECT_ERROR               ,"connect error"},
{SIO_R_ERROR_SETTING_NBIO          ,"error setting nbio"},
{SIO_R_GETHOSTBYNAME_ADDR_IS_NOT_AF_INET,"gethostbyname addr is not af inet"},
{SIO_R_INVALID_IP_ADDRESS          ,"invalid ip address"},
{SIO_R_NETLIBIFGET                 ,"netlibifget"},
{SIO_R_NETLIBOPEN                  ,"netlibopen"},
{SIO_R_NO_HOSTHNAME_SPECIFIED      ,"no hosthname specified"},
{SIO_R_NO_PORT_DEFINED             ,"no port defined"},
{SIO_R_NO_PORT_SPECIFIED           ,"no port specified"},
{SIO_R_SYSLIBFIND                  ,"syslibfind"},
{SIO_R_UNABLE_TO_BIND_SOCKET       ,"unable to bind socket"},
{SIO_R_UNABLE_TO_CREATE_SOCKET     ,"unable to create socket"},
{SIO_R_UNABLE_TO_LISTEN_SOCKET     ,"unable to listen socket"},
{SIO_R_WSASTARTUP                  ,"wsastartup"},
{0,NULL},
	};

void ERR_load_SIO_strings()
	{
	static int init=1;

	if (init)
		{
		init=0;
		ERR_load_strings(ERR_LIB_SIO,SIO_str_functs);
		ERR_load_strings(ERR_LIB_SIO,SIO_str_reasons);
		}
	}
