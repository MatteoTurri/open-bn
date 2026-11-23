/*
 * The natives of the netconf library (/opt0/bn/modules/netconf.so), in the order of its FuncTable: the network
 * settings, kept in /var/ps2/network/hdd/netcnf000.dat and copied to and from a memory card. netConfInit, netConfTest,
 * netConfDetect, netConfOpen, netConfClose, netConfDefault, netConfBackup, netConfRestore and netConfGetInfo start an
 * operation on the thread of the module and return 0, or -1 while another one runs; netConfSync follows it. index is
 * one of the saved configurations: there is one at most, and only netConfOpen reads index.
 */

/* The items of netConfGetValue, netConfSetValue, netConfGetString and netConfSetString. */
const NETCONF_DEVICE = 0;     /* set only: selects the network adaptor, the one device (the value is not used) */
const NETCONF_DHCP = 1;       /* 1: the address is assigned automatically */
const NETCONF_PPPOE = 2;      /* 1: a PPPoE connection, with USER_ID and PASSWORD */
const NETCONF_AUTO_DNS = 3;   /* 1: the DNS servers are assigned automatically */
const NETCONF_IP_ADDRESS = 4; /* the addresses: the first byte of the address in the lowest byte */
const NETCONF_NETMASK = 5;
const NETCONF_GATEWAY = 6;    /* the default route */
const NETCONF_DNS1 = 7;
const NETCONF_DNS2 = 8;
const NETCONF_USER_ID = 9;    /* a string, for PPPoE */
const NETCONF_PASSWORD = 10;  /* a string, for PPPoE */

/* Results of the operations (netConfSync) besides 0 and -1 (any other failure). */
const NETCONF_NOT_CONNECTED = -2;  /* the network cannot be brought up, or it is down */
const NETCONF_BAD_SETTINGS = -3;   /* the file of the settings is not valid */
const NETCONF_HOST_NOT_FOUND = -4; /* the name of the server of the test is not found */
const NETCONF_UNREACHABLE = -5;    /* the network of the server of the test is unreachable */
const NETCONF_REFUSED = -6;        /* the server of the test refuses the connection */
const NETCONF_TIMED_OUT = -7;      /* the connection to the server of the test times out */
const NETCONF_CARD_FULL = -8;      /* not enough free space on the memory card */
const NETCONF_UNFORMATTED = -9;    /* the memory card is not formatted, and format is 0 */
const NETCONF_NO_CARD = -10;       /* no PS2 memory card in the port */
const NETCONF_BACKUP_EXISTS = -11; /* the memory card holds a backup already, and overwrite is 0 */
const NETCONF_NO_SERVER = -12;     /* the address of the server of the test cannot be read */
const NETCONF_NOT_FOUND = -13;     /* there are no settings to copy */
const NETCONF_CANCELED = -14;      /* stopped by netConfCancel */

/* netConfDetect: the card type of a PS2 memory card, in the lower two bits of the nibble of its port. */
const NETCONF_CARD_PS2 = 2;

#pragma library "netconf"
/* Starts the module and counts the saved configurations. */
native netConfInit();
/* Stops the operation that runs; returns 1 if there was one. */
native netConfCancel();
/* Returns 1 while the operation runs; progress gets its percentage, then its result (0, or a negative error). */
native netConfSync(&progress);
/* Tests the connection: looks up the server of the system update and connects to its http port. */
native netConfTest(index);
/*
 * Looks at the two memory card ports; the result has a nibble per port, port 0 in bits 4-7 and port 1 in bits 0-3:
 * the card type in the lower two bits (2 for a PS2 memory card), then 4 if the card is formatted.
 */
native netConfDetect();
/* Reads the settings into the items; a negative index starts from empty ones. */
native netConfOpen(index);
/* Saves the items as the settings. */
native netConfClose();
/* Brings the network up with the settings. */
native netConfDefault(index);
/* Copies the settings to the memory card in port; format formats an unformatted card, overwrite replaces a backup. */
native netConfBackup(index, port, overwrite, format);
/* Copies the settings back from the memory card in port. */
native netConfRestore(index, port);
/* Reads the address and the DNS servers of the running connection into the items. */
native netConfGetInfo();
/* Returns 1: the network adaptor is the one device. */
native netConfGetDevCount();
/* Returns the value of item (1-8), -1 for another item. */
native netConfGetValue(item);
native netConfSetValue(item, value);
/* Copies item (NETCONF_USER_ID or NETCONF_PASSWORD) into buf as a packed string; maxlength is not used. */
native netConfGetString(item, const buf[], maxlength);
native netConfSetString(item, const value[]);
#pragma library
