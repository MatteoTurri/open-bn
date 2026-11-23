/*
 * The arguments the pages of edclient/ pass to each other, the fields and requests they send to the server and the
 * answers and results of those requests, the slides of their ed_page, their sound effects and the save devices.
 */

/*
 * Fields of edsClientSetStr, edsClientSetInt and edsClientGetParam, named like the parameters of the server. Five take
 * a number (edsClientSetInt): MAIL_REQUEST_FLAG, SEX, DOMAIN_CHECK_FLAG, NEW_MAIL_REQUEST_FLAG and MEMBER_STATUS; the
 * others take a string (edsClientSetStr).
 */
const EDS_FIELD_CLIENT_NAME = 0;           /* kept, never sent */
const EDS_FIELD_CLIENT_VERSION = 1;        /* sent in the User-Agent, after the group ID */
const EDS_FIELD_GROUP_ID = 2;              /* the game, from the argument GameID */
const EDS_FIELD_GROUP_PASSWD = 3;
const EDS_FIELD_SERVICE_ID = 4;
const EDS_FIELD_AUTH_SYSTEM_ID = 5;        /* always "001" */
const EDS_FIELD_AUTH_ID = 6;               /* the PSBB ID; edsClientLoad sets it */
const EDS_FIELD_AUTH_PASSWD = 7;           /* its password; edsClientLoad sets it */
const EDS_FIELD_SERVICE_SETTLEMENT_ID = 8; /* the charge ordered */
const EDS_FIELD_SETTLEMENT_END_TERM = 9;
const EDS_FIELD_STORE_ID = 10;
const EDS_FIELD_PAYMENT_ID = 11;           /* the id of the payment, in decimal */
const EDS_FIELD_CARD_NO = 12;
const EDS_FIELD_CARD_VALID_YM = 13;        /* YYYYMM */
const EDS_FIELD_ACCOUNT_ID = 14;           /* the PSBB ID and password of a payment by PSBB ID */
const EDS_FIELD_ACCOUNT_PASSWD = 15;
const EDS_FIELD_ORDER_NO = 16;
const EDS_FIELD_SECURITY_CODE = 17;
const EDS_FIELD_BIRTH_MD = 18;             /* the month and day of birth, for a card that asks for them */
const EDS_FIELD_ZIP_CODE = 19;
const EDS_FIELD_MAIL_ADDRESS = 20;
const EDS_FIELD_MAIL_REQUEST_FLAG = 21;    /* 1: the member wants the mail of the services */
const EDS_FIELD_FAMILY_NAME_KANA = 22;
const EDS_FIELD_GIVEN_NAME_KANA = 23;
const EDS_FIELD_BIRTH_YMD = 24;            /* YYYYMMDD */
const EDS_FIELD_SEX = 25;                  /* 0 male, 1 female */
const EDS_FIELD_DOMAIN_CHECK_FLAG = 26;    /* 1: the server checks the domain of the mail address */
const EDS_FIELD_NEW_AUTH_PASSWD = 27;      /* the new values of EDS_REQ_CHANGE_MEMBER */
const EDS_FIELD_NEW_ZIP_CODE = 28;
const EDS_FIELD_NEW_MAIL_ADDRESS = 29;
const EDS_FIELD_NEW_MAIL_REQUEST_FLAG = 30;
const EDS_FIELD_NEW_CARD_NO = 31;          /* empty: the card stays; all zeros: the card is removed */
const EDS_FIELD_NEW_CARD_VALID_YM = 32;
const EDS_FIELD_EC_AUTH_ID = 33;
const EDS_FIELD_EC_AUTH_PASSWD = 34;
const EDS_FIELD_SCRATCH_NO = 35;
const EDS_FIELD_WM_SESSION_ID = 36;        /* the WebMoney payment that waits for more scratch numbers */
const EDS_FIELD_MEMBER_STATUS = 37;        /* 0 registration, 1 change, 2 end of the membership */

/*
 * Requests of edsClientRequest: each one is the command of the same name of the server and sends the fields listed.
 * The enum after it is its answer, the indexes of edsClientGetInt and edsClientGetStr, named after the server: the
 * request shortened, then the field. COUNT and LIST are the number of the entries of a list and the list, whose
 * entries edsClientEnter opens; the fields of an entry are named after the entry.
 */

/* An entry of a list of services. */
enum
{
    SERVICE_SERVICE_STATUS,    /* SERVICE_STATUS_* */
    SERVICE_GROUP_ID,
    SERVICE_PARENT_SERVICE_ID, /* its own ServiceID, or the service it depends on */
    SERVICE_SERVICE_ID,
    SERVICE_NAME,
    SERVICE_START_DATE,
    SERVICE_END_DATE,          /* "9999..." for a service without an end */
    SERVICE_TICKET,
};

/* GROUP_ID, GROUP_PASSWD */
const EDS_REQ_GET_GROUP_INFO = 0;
enum
{
    GROUP_INFO_GROUP_AUTH_STATUS,
    GROUP_INFO_SERVICE_STATUS,
    GROUP_INFO_MESSAGE,
    GROUP_INFO_SG_MESSAGE,
};
/* GROUP_ID */
const EDS_REQ_GET_GROUP = 1;
enum
{
    GROUP_COUNT,
    GROUP_LIST, /* of services */
};
/* GROUP_ID, GROUP_PASSWD, AUTH_SYSTEM_ID, AUTH_ID, AUTH_PASSWD */
const EDS_REQ_AUTH_GROUP = 2;
enum
{
    AUTH_GROUP_GROUP_AUTH_STATUS,
    AUTH_GROUP_SERVICE_STATUS,
    AUTH_GROUP_MSG_CODE,
    AUTH_GROUP_MESSAGE,
    AUTH_GROUP_CHARGE_STATUS, /* CHARGE_STATUS_* */
    AUTH_GROUP_PURCHASE_STATUS,
    AUTH_GROUP_AUTH_STATUS,
    AUTH_GROUP_COMMON_INQ_ID,
    AUTH_GROUP_COUNT,
    AUTH_GROUP_LIST,          /* of services */
};
/* SERVICE_ID, AUTH_SYSTEM_ID, AUTH_ID, AUTH_PASSWD */
const EDS_REQ_AUTH_SERVICE = 3;
enum
{
    AUTH_SERVICE_AUTH_STATUS,
    AUTH_SERVICE_CARD_REGIST_STATUS,
    AUTH_SERVICE_END_DATE,
    AUTH_SERVICE_TICKET,
};
/* GROUP_ID, GROUP_PASSWD, SERVICE_ID */
const EDS_REQ_GET_SERVICE_INFO = 4;
enum
{
    SERVICE_INFO_GROUP_AUTH_STATUS,
    SERVICE_INFO_SERVICE_STATUS,
    SERVICE_INFO_MESSAGE,
    SERVICE_INFO_AGREEMENT,
};
/* GROUP_ID, GROUP_PASSWD, SERVICE_ID, AUTH_SYSTEM_ID, AUTH_ID */
const EDS_REQ_GET_SERVICE_SETTLEMENT = 5;
enum
{
    SETTLEMENT_GROUP_AUTH_STATUS,
    SETTLEMENT_CARD_VALID_START_YM,
    SETTLEMENT_CARD_VALID_END_YM,
    SETTLEMENT_CARD_REGIST_STATUS,
    SETTLEMENT_COUNT,
    SETTLEMENT_LIST, /* of the charges */
};
/* An entry of the list of the charges. */
enum
{
    CHARGE_SERVICE_SETTLEMENT_ID,
    CHARGE_EXPLANATION,
    CHARGE_SETTLEMENT_KIND,
    CHARGE_END_TERM_SETUP,
    CHARGE_END_TERM_START_YM,
    CHARGE_END_TERM_END_YM,
    CHARGE_INITIAL_FREE_TERM,
    CHARGE_TERM_VALIDITY,
    CHARGE_PURCHASE_UNIT,
    CHARGE_PRICE,
    CHARGE_TAX_AMOUNT,
    CHARGE_TAX_PRICE,
    CHARGE_PAYMENT_COUNT,
    CHARGE_PAYMENT_LIST, /* of the payments */
};
/* An entry of the list of the payments of a charge. */
enum
{
    PAYMENT_PAYMENT_ID,
    PAYMENT_NAME,
};
/*
 * GROUP_ID, GROUP_PASSWD, AUTH_SYSTEM_ID, AUTH_ID, AUTH_PASSWD, SERVICE_SETTLEMENT_ID, SETTLEMENT_END_TERM, STORE_ID,
 * PAYMENT_ID, CARD_NO, CARD_VALID_YM, ACCOUNT_ID, ACCOUNT_PASSWD
 */
const EDS_REQ_ORDER_SERVICE = 6;
enum
{
    ORDER_GROUP_AUTH_STATUS,
    ORDER_ORDER_STATUS,       /* ORDER_STATUS_* */
    ORDER_END_DATE,
    ORDER_ORDER_NO,
    ORDER_CARD_SECURITY_FLAG, /* 1: the card needs the security code */
    ORDER_BIRTH_MD_FLAG,
};
/* CARD_NO, CARD_VALID_YM, ORDER_NO */
const EDS_REQ_CHECK_CARD = 7;
enum
{
    CHECK_CARD_CHECK_STATUS, /* CHECK_STATUS_* */
    CHECK_CARD_CARD_SECURITY_FLAG,
    CHECK_CARD_BIRTH_MD_FLAG,
};
/* AUTH_SYSTEM_ID, AUTH_ID, AUTH_PASSWD, ORDER_NO, SECURITY_CODE, BIRTH_MD */
const EDS_REQ_PURCHASE_SERVICE = 8;
enum
{
    PURCHASE_AUTH_STATUS,     /* AUTH_STATUS_* */
    PURCHASE_PURCHASE_STATUS, /* PURCHASE_STATUS_* */
    PURCHASE_COMMON_INQ_ID,
    PURCHASE_TICKET,
};
/* AUTH_SYSTEM_ID, AUTH_ID, AUTH_PASSWD, ORDER_NO */
const EDS_REQ_REGIST_CARD_INFO = 9;
enum
{
    REGIST_CARD_REGIST_STATUS,
};
/* nothing: the information of the page cancel */
const EDS_REQ_GET_QUIT_INFO = 10;
enum
{
    QUIT_INFO_SERVICE_STATUS,
    QUIT_INFO_MESSAGE,
};
/* GROUP_ID, AUTH_SYSTEM_ID, AUTH_ID, AUTH_PASSWD */
const EDS_REQ_GET_QUIT_SERVICE = 11;
enum
{
    QUIT_SERVICES_AUTH_STATUS,
    QUIT_SERVICES_COUNT,
    QUIT_SERVICES_LIST, /* of the services of the member */
};
/* SERVICE_ID, AUTH_SYSTEM_ID, AUTH_ID, AUTH_PASSWD */
const EDS_REQ_QUIT_SERVICE = 12;
enum
{
    QUIT_SERVICE_QUIT_STATUS,
};
/* nothing: the information of the pages history and service_list */
const EDS_REQ_GET_HISTORY_INFO = 13;
enum
{
    HISTORY_INFO_SERVICE_STATUS,
    HISTORY_INFO_MESSAGE,
};
/* GROUP_ID, AUTH_SYSTEM_ID, AUTH_ID, AUTH_PASSWD */
const EDS_REQ_GET_HISTORY = 14;
enum
{
    HISTORY_AUTH_STATUS,
    HISTORY_REAL_SALES_NUMBER,
    HISTORY_COUNT,
    HISTORY_LIST, /* of the purchases */
};
/* An entry of the list of the purchases. */
enum
{
    SALES_ORDER_NO,
    SALES_GROUP_ID,
    SALES_SERVICE_ID,
    SALES_NAME,
    SALES_SALES_DATE,
    SALES_PAYMENT_NAME,
    SALES_PRICE,
    SALES_TAX_AMOUNT,
    SALES_TAX_PRICE,
};
/* MEMBER_STATUS: the information of the pages regist, change and quit */
const EDS_REQ_GET_MEMBER_ADMIN_INFO = 15;
enum
{
    MEMBER_INFO_SERVICE_STATUS,
    MEMBER_INFO_MESSAGE,
};
/* AUTH_SYSTEM_ID, AUTH_ID, AUTH_PASSWD */
const EDS_REQ_GET_MEMBER = 16;
enum
{
    MEMBER_GET_STATUS,
    MEMBER_MAIL_ADDRESS,
    MEMBER_MAIL_REQUEST_FLAG,
    MEMBER_FAMILY_NAME_KANA,
    MEMBER_GIVEN_NAME_KANA,
    MEMBER_ZIP_CODE,
    MEMBER_BIRTH_YMD,
    MEMBER_SEX,
    MEMBER_CARD_NO,
    MEMBER_COMMON_INQ_ID,
};
/*
 * AUTH_SYSTEM_ID, AUTH_PASSWD, CARD_NO, CARD_VALID_YM, SECURITY_CODE, ZIP_CODE, MAIL_ADDRESS, MAIL_REQUEST_FLAG,
 * FAMILY_NAME_KANA, GIVEN_NAME_KANA, BIRTH_YMD, SEX, DOMAIN_CHECK_FLAG
 */
const EDS_REQ_REGIST_MEMBER = 17;
enum
{
    REGIST_REGIST_STATUS, /* REGIST_STATUS_* */
    REGIST_AUTH_ID,       /* the new PSBB ID */
    REGIST_COMMON_INQ_ID,
};
/* AUTH_SYSTEM_ID, AUTH_ID, AUTH_PASSWD, SECURITY_CODE, DOMAIN_CHECK_FLAG and the NEW_ fields */
const EDS_REQ_CHANGE_MEMBER = 18;
enum
{
    CHANGE_CHANGE_STATUS, /* CHANGE_STATUS_* */
};
/* AUTH_SYSTEM_ID, AUTH_ID, AUTH_PASSWD: the end of the membership */
const EDS_REQ_QUIT_MEMBER = 19;
enum
{
    QUIT_MEMBER_QUIT_STATUS, /* QUIT_STATUS_* */
};
/* nothing */
const EDS_REQ_GET_MEMBER_AGREEMENT = 20;
enum
{
    AGREEMENT_AGREEMENT,
    AGREEMENT_AGREEMENT_U12,
};
/* EC_AUTH_ID, EC_AUTH_PASSWD */
const EDS_REQ_GET_EC_MEMBER = 21;
enum
{
    EC_MEMBER_GET_STATUS,
    EC_MEMBER_MAIL_ADDRESS,
    EC_MEMBER_FAMILY_NAME_KANA,
    EC_MEMBER_GIVEN_NAME_KANA,
    EC_MEMBER_BIRTH_YMD,
    EC_MEMBER_SEX,
    EC_MEMBER_CARD_NO,
    EC_MEMBER_CARD_VALID_YM,
};
/*
 * AUTH_SYSTEM_ID, AUTH_ID, AUTH_PASSWD, ORDER_NO, SCRATCH_NO, WM_SESSION_ID. A ServerStatus 2 is not an error here: it
 * gives PurchaseStatus 30.
 */
const EDS_REQ_WM_PURCHASE_SERVICE = 22;
enum
{
    WM_PURCHASE_AUTH_STATUS,     /* AUTH_STATUS_* */
    WM_PURCHASE_PURCHASE_STATUS, /* PURCHASE_STATUS_WM_* */
    WM_PURCHASE_WM_SESSION_ID,
    WM_PURCHASE_WM_ERROR_CODE,
    WM_PURCHASE_WM_MESSAGE,
    WM_PURCHASE_COMMON_INQ_ID,
    WM_PURCHASE_TICKET,
};
/* ORDER_NO, WM_SESSION_ID; no answer: ends the WebMoney session of a payment not completed */
const EDS_REQ_STOP_WEB_MONEY = 23;
/* GROUP_ID, GROUP_PASSWD */
const EDS_REQ_GET_GROUP_CHARGE_INFO = 24;
enum
{
    GROUP_CHARGE_GROUP_AUTH_STATUS,
    GROUP_CHARGE_SERVICE_STATUS,
    GROUP_CHARGE_MSG_CODE,
    GROUP_CHARGE_MESSAGE,
    GROUP_CHARGE_CHARGE_STATUS, /* CHARGE_STATUS_* */
};

/* edsClientGetStr: the date of the server, YYYYMMDD, in place of a field of the answer. */
const EDS_SERVER_DATE = -1;

/*
 * Values of the status fields of the answers. A status is 0 when the request succeeded; for the others the pages
 * show the error dialog of the same name.
 */
/* ServiceStatus of an entry of a list of services. */
enum
{
    SERVICE_STATUS_NEW,      /* not bought */
    SERVICE_STATUS_UNLISTED, /* left out of the lists */
    SERVICE_STATUS_BOUGHT,
};
/* ChargeStatus: whether the services of the game are paid. */
enum
{
    CHARGE_STATUS_FREE,
    CHARGE_STATUS_CHARGED,
};
/* AuthStatus: the PSBB ID and password. */
enum
{
    AUTH_STATUS_OK,
    AUTH_STATUS_NG,
    AUTH_STATUS_INVALID,
    AUTH_STATUS_QUITTED, /* the membership has ended */
    AUTH_STATUS_ETC_NG,
};
/* OrderStatus */
enum
{
    ORDER_STATUS_OK,
    ORDER_STATUS_UNPURCHASABLE,
    ORDER_STATUS_INVALID_CARD,
    ORDER_STATUS_AUTH_ERROR,
    ORDER_STATUS_PERIOD_NG,
};
/* PurchaseStatus: of a payment by card from 1, with WebMoney from 30. */
enum
{
    PURCHASE_STATUS_OK,
    PURCHASE_STATUS_FAULT,             /* the pages offer to try again */
    PURCHASE_STATUS_CARD_NO_NG,
    PURCHASE_STATUS_CARD_TERM_NG,
    PURCHASE_STATUS_SECURITY_NG,
    PURCHASE_STATUS_BIRTH_MD_NG,
    PURCHASE_STATUS_SERVER_NG,
    PURCHASE_STATUS_PURCHASED_NG,      /* the service is bought already */
    PURCHASE_STATUS_SYSTEM_NG,
    PURCHASE_STATUS_WM_SERVER_NG = 30, /* WebMoney is not offered */
    PURCHASE_STATUS_WM_NG,
    PURCHASE_STATUS_WM_REENTER,        /* this scratch number cannot be used: type it again */
    PURCHASE_STATUS_WM_APPEND_CARD,    /* no scratch number that can be used: add one */
    PURCHASE_STATUS_WM_NO_BALANCE,     /* add a scratch number to the session WMSessionID */
    PURCHASE_STATUS_WM_PURCHASED_NG,
    PURCHASE_STATUS_WM_SYSTEM_NG,
    PURCHASE_STATUS_WM_CARD_OVER_NG,   /* no more scratch numbers can be added */
};
/* CheckStatus */
enum
{
    CHECK_STATUS_OK,
    CHECK_STATUS_CARD_NG,
};
/* RegistStatus of EDS_REQ_REGIST_MEMBER: the field refused. */
enum
{
    REGIST_STATUS_OK,
    REGIST_STATUS_DOMAIN_CHECK, /* the domain of the mail address is not known: sent again without the check */
    REGIST_STATUS_AUTH_PASSWORD,
    REGIST_STATUS_MAIL_ADDRESS,
    REGIST_STATUS_NAME_KANA,
    REGIST_STATUS_BIRTH_YMD,
    REGIST_STATUS_CARD_NO,
    REGIST_STATUS_CARD_VALID_YM,
    REGIST_STATUS_SYSTEM_NG,
    REGIST_STATUS_ZIP_NG,
    REGIST_STATUS_SECURITY_NG,
};
/* ChangeStatus: the field refused, as RegistStatus with CARD_INFO before ZIP_NG. */
enum
{
    CHANGE_STATUS_OK,
    CHANGE_STATUS_DOMAIN_CHECK,
    CHANGE_STATUS_AUTH_PASSWORD,
    CHANGE_STATUS_MAIL_ADDRESS,
    CHANGE_STATUS_NAME_KANA,
    CHANGE_STATUS_BIRTH_YMD,
    CHANGE_STATUS_CARD_NO,
    CHANGE_STATUS_CARD_VALID_YM,
    CHANGE_STATUS_SYSTEM_NG,
    CHANGE_STATUS_CARD_INFO,
    CHANGE_STATUS_ZIP_NG,
    CHANGE_STATUS_SECURITY_NG,
    CHANGE_STATUS_DELETE_CARD_FAIL, /* the card could not be removed */
};
/* QuitStatus */
enum
{
    QUIT_STATUS_OK,
    QUIT_STATUS_NG,
};

/* The planes of create_page_arrows, in the order update_page_arrows of layout.inc takes them. */
const PAGE_ARROW_NEXT = 0;
const PAGE_ARROW_NEXT_LIT = 1;
const PAGE_ARROW_PREV = 2;
const PAGE_ARROW_PREV_LIT = 3;

/*
 * The sound effects of the pages and their parts (play_se), which load_se loads from the SE element se_0 of the page.
 * The dialogs and the wait pages of the requests take the ones of their opening and closing.
 */
const ED_SE_CURSOR = 60;
const ED_SE_DECIDE = 61;
const ED_SE_COMPLETE = 62;   /* the page of a request completed opens */
const ED_SE_CANCEL = 63;
const ED_SE_DIALOG_OPEN = 72;
const ED_SE_DIALOG_CLOSE = 73;
const ED_SE_ERROR_OPEN = 74; /* the same, for an error dialog */
const ED_SE_ERROR_CLOSE = 75;
const ED_SE_WAIT_OPEN = 76;  /* the wait page or dialog of a request */
const ED_SE_WAIT_CLOSE = 77;
const ED_SE_PAGE_OPEN = 78;  /* the defaults of ed_page */
const ED_SE_PAGE_CLOSE = 79;

/* Results of run_eds_request besides 0, and of the requests (edsClientSync). */
const EDS_CANCELED = -1011;      /* the user canceled the request with cross */
const EDS_NETWORK_ERROR = -1000; /* any error of the connection, from -1000 to -2999 */
const EDS_TOO_LONG = -3003;      /* the answer of the server is longer than 32767 bytes */
const EDS_BAD_ANSWER = -3004;    /* an HTTP status other than 200, or an answer that is not the XML expected */
const EDS_STATUS_ERROR = -3005;  /* the answer has a ServerStatus other than 0 */

/* Values of the argument "retflag": where a page goes when it is left. */
const RETFLAG_RET = 0;    /* back to the page of the argument "ret", or to its own link */
const RETFLAG_LINK = 1;   /* to its own link, with the member arguments */
const RETFLAG_AUTHOR = 2; /* to the authentication page (author.xml), with the member arguments */

/* Values of the slide of open_ed_page: where the page comes in from. open_ed_page sets it to SLIDE_IN_NONE. */
const SLIDE_IN_NONE = 3;
const SLIDE_IN_LEFT = 4;
const SLIDE_IN_RIGHT = 5;

/* Values of the slide_dir of close_ed_page: where the page goes out. */
const SLIDE_OUT_NONE = 0;
const SLIDE_OUT_LEFT = 1;
const SLIDE_OUT_RIGHT = 2;

/*
 * Results of edsClientRequest and of the operations on the saved ID (edsClientLoad, edsClientSave, edsClientRemove,
 * edsClientFormat), at once or by edsClientSync.
 */
const EDS_NOT_INITIALIZED = -3000; /* edsClientInit was not called */
const EDS_INVALID_PARAM = -3001;   /* an unknown request or device, or a field of the other kind */
const EDS_BUSY = -3002;            /* another request or operation is running */
const EDS_INVALID_ID = -3006;      /* the saved ID cannot be encrypted or decrypted */
const EDS_NO_CARD = -3007;         /* no PS2 memory card, or one that cannot be used */
const EDS_UNFORMATTED = -3008;
const EDS_NO_SPACE = -3009;
const EDS_FILE_STATE = -3010;      /* load: there is no saved ID; save: there is one already, without bit 0x8000 */
const EDS_BAD_VERSION = -3011;     /* the saved data is not "Billing-System Data Format:00000" */
const EDS_CARD_CHANGED = -3012;    /* the memory card was changed */

/* Devices of edsClientLoad, edsClientSave, edsClientRemove and edsClientFormat. */
const EDS_DEVICE_HDD = 0;
const EDS_DEVICE_CARD1 = 1;
const EDS_DEVICE_CARD2 = 2;
const EDS_SAVE_REPLACE = 0x8000; /* edsClientSave: replace an ID saved already */
