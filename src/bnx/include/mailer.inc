/*
 * The natives of the mailer library (/opt0/bn/modules/mailer.so), in the order of its FuncTable: the mailboxes,
 * the messages, the address book and the preferences of the mailer. They are all variadic; the comments give the
 * arguments they read. A mail is named by its folder and its index, the line in the sorted list of the folder. A
 * *Start native hands a request to the folder thread, whose result mx_SyncFolder gives; a *Block one does it at once.
 */

/* The mail folders. */
const FOLDER_INBOX = 0;
const FOLDER_OUTBOX = 1; /* sent mail */
const FOLDER_DRAFT = 2;
const FOLDER_QUEUE = 3;  /* mail waiting to be sent */

/* States of mx_SetMsgStatus. */
const MSG_READ = 0;
const MSG_UNREAD = 1;
const MSG_LOCK = 2;
const MSG_UNLOCK = 3;

/* Sort keys and orders of mx_GetMsgOrder and mx_SortMsgStart. */
const SORT_KEY_UNSORTED = 0; /* the folder as it was read */
const SORT_KEY_NUMBER = 1;   /* the order the mails arrived in, shown as no sort */
const SORT_KEY_SIZE = 2;
const SORT_KEY_DATE = 3;
const SORT_KEY_FROM = 4;
const SORT_KEY_SUBJECT = 5;
const SORT_KEY_TO = 11;
const SORT_ASCEND = 0;
const SORT_DESCEND = 1;

/* Errors of the folder updates and of the transfers: the errno of the library, and two of the scripts' own. */
const ERR_NOSPACE = 28; /* ENOSPC: the disk is full */
const ERR_NOCONN = 29;  /* the network is down */
const ERR_NODATA = 30;  /* the basic settings of the account are incomplete (mx_IsReadySend, mx_IsReadyRecv) */

/* The errors of a receiving besides ERR_NOSPACE: the Pop3ErrorValue of the POP3 client of the library (Sylpheed's). */
const PS_SOCKET = 2;   /* socket I/O */
const PS_AUTHFAIL = 3; /* the server refused the user or the password */
const PS_PROTOCOL = 4; /* the server answered with an error */
const PS_IOERR = 6;    /* file I/O */
const PS_LOCKBUSY = 9; /* the mailbox is locked */

/* The screens of the mailer, as bits of the history that main keeps and passes to the pages ("hist"): the ones
 * opened on the way from the list. */
enum
{
    SCREEN_LIST = 1,
    SCREEN_WRITE,
    SCREEN_READ,
    SCREEN_INFO,
    SCREEN_ADDR,
    SCREEN_ADDR_PICK, /* the address book, to pick an address for the mail being written */
    SCREEN_ADDR_EDIT,
    SCREEN_SET,
    SCREEN_SET_EDIT,
    SCREEN_SET_PART,
    SCREEN_SORT,
    SCREEN_SORT_EDIT,
    SCREEN_REFRESH,
};

/* How the write page starts (its "send" argument; "old" is the mode it was first opened in). */
enum
{
    WRITE_NEW,
    WRITE_REPLY,
    WRITE_FORWARD,
    WRITE_ADD_ADDR,  /* back from the address book with an address for the recipients */
    WRITE_REWRITE,   /* the selected mail, edited again */
    WRITE_FROM_ARGS, /* the to, subject and body arguments of main */
    WRITE_RETURN,    /* back from the address book without an address */
};

/* Modes of the address book (the "mode" argument of mailaddr). */
const ADDR_MODE_BOOK = 0;
const ADDR_MODE_PICK = 1; /* picks an address for the mail being written */

/* States of a transfer (mx_SyncSend, mx_SyncRecv). */
const SYNC_DONE = 0;
const SYNC_CONNECT = 1;
const SYNC_TRANSFER = 2; /* the mails are being sent or received */
const SYNC_CLOSE = 3;

/* Bits of the result of reading the settings (the config argument of mx_GetInitResult). */
const INIT_BADPASS_RECV = 0x01; /* the password of the account cannot be decoded */
const INIT_BADPASS_SMTP = 0x02; /* the password of the SMTP server cannot be decoded */
const INIT_NOACCOUNT = 0x04;    /* the account cannot be read */

/* The select of the yes/no dialog (maildlgyesno): the button selected first, then the answer. */
const ANSWER_YES = 0;
const ANSWER_NO = 1;
const ANSWER_CANCEL = -1; /* closed with the cross button */

/* Modes of the address editor (the "mode" argument of mailaedit). */
const AEDIT_CHANGE = 0;
const AEDIT_NEW = 1;
const AEDIT_OVERWRITE = 2; /* the new address is already in the book: the entry is replaced */

/* Results of "ACHK" in the address editor. */
const AEDIT_CHECK_OK = 0;
const AEDIT_CHECK_EMPTY = 1;  /* a new entry without name and address: nothing to save */
const AEDIT_CHECK_NOADDR = 2;
const AEDIT_CHECK_EXISTS = 3; /* the address is already in the book */

/* Values of the preference SIG_TYPE: what the signature of a new mail holds. */
const SIGNATURE_NONE = 0;
const SIGNATURE_ADDR = 1;
const SIGNATURE_NAME_ADDR = 2; /* the name and the address */
const SIGNATURE_TEXT = 3;      /* the preference SIG_TEXT */

/* The lists of mail and of addresses: the lines shown at once, and the scroll in progress. */
const LIST_LINES = 7;
enum
{
    SCROLL_NONE,
    SCROLL_DOWN, /* a line added at the bottom: the top ones go when it ends */
    SCROLL_UP,
};

#pragma library "mailer"
/* Does nothing; returns 1. */
native mlModuleCall(...);
/* Creates the mail folders on the disk and starts the library and its threads, the first time. */
native mx_Init(...);
/* (index, text[], maxlen): the address book entry index as "name <address>". */
native mx_GetAddr2(...);
/* (index, name[], maxlen) */
native mx_GetAddrName(...);
/* (index, addr[], maxlen) */
native mx_GetAddrAddr(...);
/* (skip, addr[]): the entry other than skip with address addr, -1 if none, -2 if the book cannot be read. */
native mx_CheckAddr(...);
/* (index, text[]): stores "name <address>" in entry index, -1 for a new entry. */
native mx_SetAddr(...);
/* (index, name[], addr[]): the same with the name and the address apart. */
native mx_SetAddr2(...);
/* (index) */
native mx_RemoveAddr(...);
native mx_CountAddr(...);
/* (text[]): the text that mx_GetMsgBodyStart reads with folder -1. */
native mx_SetMsgBody(...);
/* (folder, index, body[], maxlen): mx_GetMsgBodyStart without quoting, done at once. */
native mx_GetMsgBodyBlock(...);
/* (folder, index, body[], maxlen, quote_mark[], head[], tail[]): mx_GetMsgBodyStart done at once. */
native mx_GetMsgQBodyBlock(...);
/*
 * (folder, index, body[], maxlen, quote_mark[] = "", head[] = "", tail[] = ""): starts reading the body:
 * each line prefixed by quote_mark, between head and tail. The result is 0, 1 if cut to maxlen bytes, -1.
 */
native mx_GetMsgBodyStart(...);
/* (the arguments of mx_SetMsgStart): the save done at once. */
native mx_SetMsgBlock(...);
/*
 * (folder, index, to[], cc[], bcc[], subject[], body[], mode, src_folder, src_index, &error): starts saving a
 * mail (index is not used); mode is the WRITE_* mode, src_folder and src_index the mail replied to or
 * forwarded. The result is the line of the new mail, -1 if not saved, and error gets the errno of a failure.
 */
native mx_SetMsgStart(...);
/* (folder, &count) */
native mx_CountMsg(...);
/* (folder, &count) */
native mx_CountUnread(...);
/* (folder, index): mx_RemoveMsgStart done at once. */
native mx_RemoveMsgBlock(...);
/* (folder, index): starts deleting a mail; the result is 0, ERR_NOSPACE or 1 on another error. */
native mx_RemoveMsgStart(...);
/* (line = -1): starts sending the mail at line of the queue, or the whole queue. */
native mx_RefreshSend(...);
/* Starts receiving the mail. */
native mx_RefreshRecv(...);
/*
 * The preferences by name: mx_GetPref(name[], &value), mx_GetPrefStr(name[], text[], maxlen),
 * mx_SetPref(name[], value), mx_SetPrefStr(name[], text[]).
 */
native mx_GetPref(...);
native mx_GetPrefStr(...);
native mx_SetPref(...);
native mx_SetPrefStr(...);
/* Sets the preferences to their defaults. */
native mx_DefaultPref(...);
/*
 * (&count, &mail_total, &done, &total, &left): the mail being received of mail_total, the bytes done of
 * total, the mails left on the server over the size limit. Returns the SYNC_* state.
 */
native mx_SyncRecv(...);
/* 0, ERR_NOSPACE or the PS_* error of the last receiving. */
native mx_SyncRecvError(...);
/* (&count, &mail_total, &done, &total, &last_no): as mx_SyncRecv; last_no is the number of the last mail sent. */
native mx_SyncSend(...);
/* 0, ERR_NOSPACE or the error of the last sending. */
native mx_SyncSendError(...);
/* (folder, index, status): status is a MSG_* state; returns 1 when it was set. */
native mx_SetMsgStatus(...);
/* (folder, key, order, &index): mx_SortMsgStart done at once. */
native mx_SortMsgBlock(...);
/*
 * (folder, key, order, &index): starts sorting folder, a SORT_KEY_* key; index, a mail before, becomes its
 * index after. The result is 0, ERR_NOSPACE or 1 on another error.
 */
native mx_SortMsgStart(...);
/* (folder, &key, &order) */
native mx_GetMsgOrder(...);
/* Saves the preferences and, if the account or the check of new mail changed, restarts the check. */
native mx_Restart(...);
/* 0 when the account can send, 1 when it is not set up, 2 when a basic setting is missing. */
native mx_IsReadySend(...);
/* 0 when the account can receive, 1 when it is not set up, 2 when a basic setting is missing. */
native mx_IsReadyRecv(...);
/* Stops the sending; returns 1. */
native mx_SendCancel(...);
/* Stops the receiving; returns 1. */
native mx_RecvCancel(...);
/*
 * (folder, index, text[], maxlen, flags = 0), and To, Cc, Bcc: flags 1 unfolds the header, 2 puts one
 * address per line. Returns 1 if cut to maxlen bytes, 0, -1 if unreadable; so do the Addr, Name and Subject natives.
 */
native mx_GetMsgFrom(...);
/* (folder, index, text[], maxlen): the address alone; the Name natives give the name alone. */
native mx_GetMsgFromAddr(...);
native mx_GetMsgFromName(...);
native mx_GetMsgTo(...);
native mx_GetMsgToAddr(...);
native mx_GetMsgToName(...);
native mx_GetMsgCc(...);
native mx_GetMsgCcAddr(...);
native mx_GetMsgCcName(...);
native mx_GetMsgBcc(...);
native mx_GetMsgBccAddr(...);
native mx_GetMsgBccName(...);
/* (folder, index, text[], maxlen) */
native mx_GetMsgXMailer(...);
/* (folder, index, text[], maxlen, strip = 0): strip 1 removes a leading "Re: ", 2 "Fw: " or "Fwd: ". */
native mx_GetMsgSubject(...);
/* (folder, index, text[], maxlen, format[] = ""): the Date header, or the date written with format. */
native mx_GetMsgDate(...);
/* (folder, index, text[], maxlen): the date in the date format of the system; YMDHM adds the time. */
native mx_GetMsgDateYMD(...);
native mx_GetMsgDateYMDHM(...);
/* (folder, index, text[], maxlen): the flag as the text "0" or "1"; so does mx_GetMsgLock. */
native mx_GetMsgUnread(...);
native mx_GetMsgLock(...);
/* (folder, index): returns the size in bytes, -1 on error. */
native mx_GetMsgSize(...);
/* (folder, index, text[], maxlen, no_from = 0, flags = 0): Reply-To, or From unless no_from. */
native mx_GetMsgReplyTo(...);
/* (folder, index): 1 for a multipart mail, -1 on error. */
native mx_IsMultiPart(...);
/* Returns the free space of the disk in KB less 384 KB, negative below that; -1 if unknown. */
native mx_GetFree(...);
/* (&failed, &config): failed is 0 when mx_Init worked; config has the INIT_* bits of reading the settings. */
native mx_GetInitResult(...);
/* Returns whether the network is up. */
native mx_GetNetworkUp(...);
/* (folder, index): 0 when the address of the account is in To, 1 in Cc, 2 in neither, -1 on error. */
native mx_CheckToCcBcc(...);
/* (text[], length): cuts text after length characters and appends "...". */
native mx_TrimString(...);
/* (folder, line): the number of the mail at line of the list, -1 if none. */
native mx_Line2MsgNum(...);
/* (folder, msg_no): the line of mail msg_no. */
native mx_MsgNum2Line(...);
/* (folder): starts reading the list of folder again. */
native mx_UpdateListStart(...);
/* (&result): returns 1 when the request of a *Start native has ended, with its result. */
native mx_SyncFolder(...);
#pragma library
