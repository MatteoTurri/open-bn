/*
 * The natives of bn that the scripts call (the natives of the libraries are in the library files), with the
 * constants of their arguments. Their implementation is in src/natives/.
 */

/* Bits of the button words of PadState. */
const PAD_SELECT = 0x0001;
const PAD_L3 = 0x0002;
const PAD_R3 = 0x0004;
const PAD_START = 0x0008;
const PAD_UP = 0x0010;
const PAD_RIGHT = 0x0020;
const PAD_DOWN = 0x0040;
const PAD_LEFT = 0x0080;
const PAD_L2 = 0x0100;
const PAD_R2 = 0x0200;
const PAD_L1 = 0x0400;
const PAD_R1 = 0x0800;
const PAD_TRIANGLE = 0x1000;
const PAD_CIRCLE = 0x2000;
const PAD_CROSS = 0x4000;
const PAD_SQUARE = 0x8000;

/* Curves of the Anim* natives: how the value goes from 0.0 to 1.0 over the duration. */
const ANIM_FUNC_LINEAR = 1;
const ANIM_FUNC_SLOWER = 2;      /* square root: fast start, slow end */
const ANIM_FUNC_FASTER = 3;      /* square: slow start, fast end */
const ANIM_FUNC_INTERPOLATE = 4; /* cubic Hermite */
const ANIM_FUNC_ANGULER = 5;     /* half a sine wave: slow at both ends */
const ANIM_FUNC_EXPONENT = 6;    /* exponential approach */

/* Animations of an object, for AnimDoTimer, AnimGetCurrTime and AnimGetState. */
const ANIM_MOVE = 0;   /* AnimMoveLine, AnimMoveHermite */
const ANIM_ROTATE = 1; /* AnimRotate, AnimQuat */
const ANIM_SCALE = 2;  /* AnimScale */
const ANIM_COLOR = 3;  /* AnimColor */
const ANIM_TIMER = 4;  /* AnimTimerManager */

/* Results of AnimGetState. */
const ANIM_DONE = 0; /* also when there is no such animation */
const ANIM_DELAY = 1;
const ANIM_RUN = 2;

/* Values of the "HALN" (left, center, right) and "VALN" (center, baseline, top, bottom) options of text. */
const ALIGN_LEFT = 0;
const ALIGN_CENTER = 1;
const ALIGN_RIGHT = 2;
const ALIGN_BASELINE = 3;
const ALIGN_TOP = 4;
const ALIGN_BOTTOM = 5;

/* Windows of wndGetName, wndGetScene and wndGetScreen. */
const WND_SELF = 0; /* the window of the calling script */
const WND_MAIN = 1;
const WND_BG = 2;

/* Where xmlLoadStart opens the page it loads. */
const XMLLOAD_MAIN = 0;
const XMLLOAD_BG = 1;
const XMLLOAD_MODAL = 2; /* not for a window from the network */

/* Ids of GetSystemParam and SetSystemParam; only the first six are open to a window from the network. */
const SYS_SCREEN_WIDTH = 0;  /* read only */
const SYS_SCREEN_HEIGHT = 1; /* read only */
const SYS_PADHELP = 500;
const SYS_FG_ALPHA = 503;
const SYS_FGBAR_ALPHA = 504;
const SYS_ENABLE_NAVIMENU = 602;
const SYS_BUSY = 100;        /* read only: any of the five below */
const SYS_NET_BUSY = 101;    /* read only */
const SYS_IMAGE_BUSY = 102;  /* read only */
const SYS_XML_BUSY = 103;    /* read only */
const SYS_DB_BUSY = 104;     /* read only */
const SYS_SCRIPT_BUSY = 105; /* read only */
const SYS_BIFF_EXIST = 200;  /* read only */
const SYS_BIFF_SOUND = 201;  /* read only */
const SYS_MAIN_HTTPS = 210;  /* read only: the main window is an https: page */
const SYS_HTTPS_BUSY = 211;  /* read only */
const SYS_CDVD_TYPE = 300;   /* read only */
const SYS_NETWORK_UP = 301;  /* read only */
const SYS_AUTH_SAVE = 400;
const SYS_AUTH_OK = 401;
const SYS_PAGE_MAX = 501;
const SYS_PAGE_NO = 502;
const SYS_ENABLE_SE = 600;
const SYS_ENABLE_BGM = 601;
const SYS_ALERT_ILLEGAL_DISC = 603;
const SYS_LANGUAGE = 700;           /* read only, as the ones below */
const SYS_KB_TYPE = 701;
const SYS_KB_REPEAT_WAIT = 702;
const SYS_KB_REPEAT_SPEED = 703;
const SYS_MOUSE_SPEED = 704;
const SYS_MOUSE_DCLICK = 705;
const SYS_MOUSE_CHANGE_LR = 706;
const SYS_ATOK_MODE = 707;
const SYS_ATOK_KEY_BIND = 708;
const SYS_SOFTKB_ON_OFF = 709;
const SYS_SOFTKB_QWERTY = 710;
const SYS_SPDIF = 711;
const SYS_ASPECT = 712;
const SYS_VIDEO = 713;
const SYS_TIME_ZONE = 714;
const SYS_TIME_ZONE_CITY = 715;
const SYS_SUMMER_TIME = 716;
const SYS_TIME_NOTATION = 717;
const SYS_DATE_NOTATION = 718;
const SYS_AUTO_DIAG = 719;

/* Disc types, the values of SYS_CDVD_TYPE (the disc types of libcdvd). */
const CDVD_NO_DISC = 0;
const CDVD_DETECTING = 1;
const CDVD_DETECTING_CD = 2;
const CDVD_DETECTING_DVD = 3;
const CDVD_DETECTING_DVD_DL = 4; /* a dual-layer DVD */
const CDVD_UNKNOWN = 5; /* the types up to this one are no disc or a disc being detected */
const CDVD_PS1_CD = 16;
const CDVD_PS1_CDDA = 17;
const CDVD_PS2_CD = 18;
const CDVD_PS2_CDDA = 19;
const CDVD_PS2_DVD = 20;
const CDVD_DVD_VR = 252;
const CDVD_CDDA = 253;
const CDVD_DVD_VIDEO = 254;
const CDVD_ILLEGAL = 255;

/*
 * Ids of GetSystemParamStr and SetSystemParamStr: the labels of the pad help for the right, down, up
 * and left buttons, open to any window, and their defaults and the authentication, set only by a
 * local window.
 */
const SYS_STR_PADHELP_RR = 500;
const SYS_STR_PADHELP_RD = 501;
const SYS_STR_PADHELP_RU = 502;
const SYS_STR_PADHELP_RL = 503;
const SYS_STR_PADHELP_DEF_RL = 600;
const SYS_STR_PADHELP_DEF_RD = 601;
const SYS_STR_PADHELP_DEF_RR = 602;
const SYS_STR_PADHELP_DEF_RU = 603;
const SYS_STR_AUTH_ID = 400;
const SYS_STR_AUTH_PASS = 401;
const SYS_STR_VERSION = 900; /* read only */

/* Messages that bn sends to the scripts of every window (RecvMessage, FindMessage). */
const MSG_NAVIMENU_OPENED = -100;
const MSG_MAIN_CREATED = -200; /* a main window was created */
const MSG_BG_CREATED = -201;   /* a background window was created */
const MSG_MAIN_FADEOUT = -300; /* the main window starts fading out */
const MSG_BG_FADEOUT = -301;   /* the background window starts fading out */

/* Values of the "ZSRT" option: the layer an object is drawn in. Any other value acts as ZSORT_NONE. */
const ZSORT_NONE = 0;       /* the layer of its parent */
const ZSORT_OVERLAY = 3;    /* a new layer drawn after the others */
const ZSORT_BACKGROUND = 4; /* a new layer drawn before the first one */

/*
 * Other options of the objects whose values are measures: those named with 64 ("W64 " the width a label wraps at,
 * "H64 ", "h64 ", "TW64" and "TH64" the frame of a text, "LM64" and "RM64" the margins, "LS64" the line spacing,
 * "SX64" and "SY64" the offset of a shadow, "AP64" and "KP64" the pitch of ASCII and kanji characters) are in 1/64
 * pixel, the 26.6 fixed point of FreeType, which lays out and draws the text (src/gfx/pltm.c); the scripts write
 * them as pixels * 64. "FSIZ" is the font size in pixels; "ZOFS" is added to the depth an object is sorted by;
 * "LEVL" is the blur level of a defocus object, 0 to 16; "FDEG" turns on the fading edge of a plane.
 */

/* Results of err10026: what the BOOT2 entry of the SYSTEM.CNF of a partition starts; below 0 an error. */
const BOOT2_NOT_FOUND = -1; /* the partition cannot be opened */
const BOOT2_CDROM = 0;      /* "cdrom0:": a game that needs its disc */
const BOOT2_PFS = 1;        /* "pfs:": an application on the hard disk */
const BOOT2_PATINFO = 2;    /* "PATINFO": the same */
const BOOT2_NOBOOT = 3;     /* "NOBOOT": nothing to start */

/* States of the import of the games into the database (err10028), besides the steps that run. */
const IMPORT_DONE = 0;
const IMPORT_CANCELLED = 4; /* by err10033 */

/* Results of imgDecSync, imgLoadTim2Sync and imgTrimSync (the last one is never IMG_BUSY). */
const IMG_BUSY = 0;    /* still decoding */
const IMG_DONE = 1;    /* done: the result is ready */
const IMG_FAILED = -1;

/* The result of fileSync once the file operation is over (0 while it runs). */
const FILESYNC_DONE = 1;

/* The option of xmlLoadSetOptionI that sets the timeout of the loads of the script (0 to 1000). */
const XMLOPT_TIMEOUT = 100;

/* Results of ScriptState. */
const SCRIPT_NONE = 0;     /* the object has no script */
const SCRIPT_RUNNING = 1;
const SCRIPT_FINISHED = 2;

/* Effects of the system sound bank (sndSePlay). */
const SE_ICON_MOVE = 0;      /* the top menu moves to another icon */
const SE_CURSOR = 5;         /* the cursor moves */
const SE_OPEN = 6;           /* triangle opens an option menu, or a field starts editing */
const SE_DECIDE = 7;         /* circle */
const SE_CANCEL = 8;         /* cross: back or cancel */
const SE_WINDOW_OPEN = 15;   /* a message box or a menu window appears */
const SE_WINDOW_CLOSE = 16;  /* an option menu closes */
const SE_DIALOG_OPEN = 17;   /* a confirmation dialog appears */
const SE_SCROLL = 20;        /* a text scrolls by a line or a page */
const SE_ZOOM_OUT = 22;
const SE_ZOOM_IN = 23;
const SE_KEYBOARD_OPEN = 30; /* the software keyboard appears */
const SE_MAIL_ARRIVED = 40;  /* new mail, when SYS_BIFF_SOUND is on */

/* Bits of SYS_PADHELP: the buttons whose help the pad help shows. */
const PADHELP_CIRCLE = 1;
const PADHELP_CROSS = 2;
const PADHELP_TRIANGLE = 4;
const PADHELP_SQUARE = 8;
const PADHELP_L1 = 16;
const PADHELP_R1 = 32;

/* Values of SYS_DATE_NOTATION: the order of year, month and day. */
const DATE_YYYYMMDD = 0;
const DATE_MMDDYYYY = 1;
const DATE_DDMMYYYY = 2;

/* Blend modes, the value of the ALPH option (the GM_BLEND_* of src/gfx/gm_graph.h). */
const BLEND_COPY = 0;
const BLEND_ALPHA = 1;
const BLEND_ADD = 2;
const BLEND_SUB = 3;
const BLEND_KEEP = 4; /* the color buffer is left as it is */
const BLEND_DARKEN = 6; /* the color buffer times (1 - source alpha) */
const BLEND_DEST_ALPHA = 0x100; /* destination alpha instead of the source alpha */
const BLEND_FIX_ALPHA = 0x200; /* alpha 1.0 instead of the source alpha */

/* The resource textures of imgGetResource (/opt0/bn/data/tex/<name>.tm2, src/app/resource.cpp); 3, 4 and 15 are
 * empty. */
const RES_HTTPS = 0;
const RES_DOT = 1;
const RES_GRID = 2;
const RES_SPIN = 5;
const RES_BTN_R = 6; /* the circle button */
const RES_BTN_D = 7; /* the cross button */
const RES_BTN_U = 8; /* the triangle button */
const RES_BTN_L = 9; /* the square button */
const RES_FOCUS = 10;
const RES_BAR1 = 11;
const RES_BAR2 = 12;
const RES_BAR3 = 13;
const RES_RADIO = 14;
const RES_EDITBOX = 16;
const RES_BUSY = 17;
const RES_BIFF = 18;
const RES_SCRBAR1 = 19;
const RES_SCRBAR2 = 20;
const RES_SCRBAR3 = 21;
const RES_WINDOW1 = 22;
const RES_CHECK0 = 23;
const RES_CHECK1 = 24;
const RES_BTN_L1 = 25;
const RES_BTN_R1 = 26;
const RES_PSBBLOGO = 27;
const RES_BUTTON0 = 28;
const RES_BUTTON1 = 29;

/* Commands of SystemCommand; all but the first only for a window that does not come from the network. */
const SYSCMD_BUSY = 100;           /* the script is busy while the argument is not 0 */
const SYSCMD_HTTPS_BUSY = 10100;   /* the script is busy on https while the argument is not 0 */
const SYSCMD_CLEAR_BIFF = 10200;
const SYSCMD_HIDE_WINDOW = 10300;  /* hides the window while the argument is not 0 */
const SYSCMD_FADEOUT_MAIN = 10400;
const SYSCMD_FADEOUT_BG = 10401;
const SYSCMD_WIPE_ON_FREE = 10500; /* clears the memory of the script when it is freed */

/* Closes the script attached to obj (within the group of the caller). */
native Abort(obj);
native AnimColor(obj, Float:r, Float:g, Float:b, Float:a, Float:duration, Float:delay, curve);
/* Advances the animation (ANIM_*) of obj by one frame times speed and returns its value. */
native Float:AnimDoTimer(obj, anim, Float:speed);
/* Returns the current value (0.0 to 1.0, as float bits) of the animation (ANIM_*) of obj. */
native AnimGetCurrTime(obj, anim);
native AnimGetState(obj, anim);
/* The AnimMod* natives change the target of the attached animation, or set the value at once without one. */
native AnimModColor(obj, Float:r, Float:g, Float:b, Float:a);
native AnimModMove(obj, Float:x, Float:y, Float:z);
native AnimModQuat(obj, Float:x, Float:y, Float:z, Float:w);
native AnimModRotate(obj, Float:x, Float:y, Float:z);
native AnimModScale(obj, Float:x, Float:y, Float:z);
/* A Hermite move; with a move already attached, its velocity times velocity_scale is the start velocity. */
native AnimMoveHermite(obj, Float:x, Float:y, Float:z, Float:duration, Float:delay, curve, Float:velocity_scale);
native AnimMoveLine(obj, Float:x, Float:y, Float:z, Float:duration, Float:delay, curve);
native AnimQuat(obj, Float:x, Float:y, Float:z, Float:w, Float:duration, Float:delay, curve);
native AnimRotate(obj, Float:x, Float:y, Float:z, Float:duration, Float:delay, curve, order);
native AnimScale(obj, Float:x, Float:y, Float:z, Float:duration, Float:delay, curve);
/* Attaches a bare timer (ANIM_TIMER) to obj. */
native AnimTimerManager(obj, Float:duration, Float:delay, curve);
native Float:atof(const str[]);
native atoi(const str[]);
/* Replaces the script of obj with function func of file (the caller's file when empty), with arguments arg. */
native AttachScript(obj, const func[] = "", const file[] = "", const arg[] = "");
native authGetStatus(&auth_result);
native authStart();
/* A variable holds a list of values: avalAdd appends one, avalSet replaces them all. */
native avalAdd(const name[], const value[]);
native avalDelete(const name[]);
native avalDeleteFile();
native avalGetNum(const name[]);
native avalGetValue(const name[], index, value[], maxlength);
native avalSet(const name[], const value[]);
native avalWriteFile();
/* Returns the camera of the scene of obj (0: the object of the calling script). */
native Camera(obj);
native cdlAbort();
native cdlCheckGetError();
native cdlCheckStart(index);
native cdlCheckSync();
native cdlExit();
native cdlGetStatus(&status, &content_length, &position);
native cdlInit();
/* Queues the download described by the attributes of the XML element tag. */
native cdlQueAdd(tag, reserved);
native cdlQueGetData(index, field, value[], maxlength);
native cdlQueRemove(index);
native cdlStart();
native cdlSuspend(suspend);
native Float:cos(Float:angle);
native CreateCloud(parent);
native CreateCrystal(parent);
native CreateDefocus(parent);
native CreateEdit(parent, max_length);
native CreateEditML(parent);
/* A multiline editor for a password. */
native CreateEditML_P(parent);
native CreateHermite(parent);
native CreateIcon(parent);
native CreateLabel(parent);
native CreateLabelShadow(parent);
native CreateModel(parent);
native CreateNull(parent);
native CreatePlane(parent);
native CreatePlane3x3(parent);
native CreatePrim(parent);
native CreateSScope0(parent);
native CreateTextbox(parent);
native datClose();
/* Local date, or UTC when utc is 1. */
native date(&date_year, &date_month, &date_day, utc);
/* Without a database, cond is a comma separated list of content types and table the URL of a folder. */
native datFind(const cond[], const table[]);
native datGetData(index, field, value[], maxlength);
native datGetNum();
/* Opens a file list (type 0, name is its URL) or a database; order sorts the file list. */
native datOpen(type, const name[], order);
/* The types of datOpen: a file list of the URL name, or the database /opt1/database/sqlite/<name>.db. */
const DAT_FILE_LIST = 0;
const DAT_DATABASE = 3;
/* The orders of a file list for datOpen (FMENTRY_ORDER_* of fmentry.h); a database ignores it. */
const DAT_ORDER_CTIME_ASC = 0;
const DAT_ORDER_CTIME_DESC = 1;
const DAT_ORDER_MTIME_ASC = 2;
const DAT_ORDER_MTIME_DESC = 3; /* the default */
const DAT_ORDER_NAME_ASC = 4;
const DAT_ORDER_NAME_DESC = 5;
const DAT_ORDER_CTCOUNT_ASC = 6;
const DAT_ORDER_CTCOUNT_DESC = 7;
const DAT_ORDER_MTCOUNT_ASC = 8;
const DAT_ORDER_MTCOUNT_DESC = 9;
native datSQLExec(const sql[]);
native datSync();
native datUpdateData(index, field, const value[], maxlength);
native Destroy(obj);
/* Returns the 2D display of the scene of obj (0: the object of the calling script). */
native Disp2D(obj);
native domAddTag(tag, const name[]);
/* Finds the first element under tag with that name and value (empty: any); deep searches the whole subtree. */
native domFindTag(tag, const name[], const value[] = "", deep);
/* domFirstChild, domLastChild, domNext and domPrev look for an element named name (empty: any). */
native domFirstChild(tag, const name[] = "");
/* With an empty name, reads the text of the element instead of an attribute. */
native domGetData(tag, const name[] = "", value[], maxlength);
native domGetTagName(tag, name[], maxlength);
/* Inserts a new element before tag. */
native domInsertTag(tag, const name[]);
native domLastChild(tag, const name[]);
/* Moves tag relative to ref: position 1 first child, 2 last child, 3 after, 4 before. */
native domMoveTag(tag, ref, position);
native domNext(tag, const name[] = "");
native domPrev(tag, const name[] = "");
/* With an empty name, removes the text of the element instead of an attribute. */
native domRemoveData(tag, const name[]);
native domRemoveTag(tag);
native domRoot();
/* With an empty name, replaces the text of the element instead of an attribute. */
native domSetData(tag, const name[], const value[]);
/* Reads the boot path of the PS1 disc, applies boot mode 0 when it is found, and gives the path. */
native err10002(boot_path[]);
/* Reads the BOOT2 path of the SYSTEM.CNF of the disc, applies boot mode 1 when it is found, and gives the path. */
native err10003(boot_path[]);
/* As err10003, with another drive setting and boot mode 2. */
native err10004(boot_path[]);
/* Generates new random keys for the boot DES and copies them (16 bytes) into keys. */
native err10019(const keys[]);
/* Resets the chaining value of the boot DES. */
native err10020();
/* Passes src through the boot DES into dest; returns the length. */
native err10021(const src[], const dest[]);
/* Writes value into download slot slot of the reboot. */
native err10022(slot, const value[]);
/* Arms count download slots and the reboot, and ends the script. */
native err10023(count);
/* Shortens a name: the last 13 characters of a longer one, then at most 11. */
native err10024(const name[], dest[]);
/* Compares two strings of at most 255 characters: 0 if equal, -3 otherwise. */
native err10025(const str1[], const str2[]);
/* Reads the BOOT2 entry of SYSTEM.CNF of a PFS partition; returns the boot type or an error. */
native err10026(const partition[], path[], boot[]);
/* Starts the scan that imports the games. */
native err10027();
/* Returns the state of the game import. */
native err10028();
/* Uninstalls the application name. */
native err10029(const name[]);
/* Reads the three uninstall messages of the icon.sys of a PFS partition. */
native err10030(const partition[], message0[], message1[], message2[]);
/* Converts a compact date "YYYYMMDDhhmmss" to local time. */
native err10031(const date_str[], &date_year, &date_month, &date_day, &time_hour, &time_minute, &time_second);
/* Returns 0. */
native err10032();
/* Cancels the game import. */
native err10033();
/* Looks for the update file on the disc. */
native err10034();
/* Permutes the bits of the 64-bit value src (two cells, low word first) by the count bit indices of map. */
native err10101(dest[], const src[], map, count);
/* Takes the root user. */
native err11012();
/* Gives the root user back. */
native err11013();
/* Turns a path into a URL ("pfs:" paths become "file://" + the path). */
native err20001(const path[], url[]);
/* Creates the table of the movies. */
native err20004();
/* Appends the file name of path to dir, as "<base>_<num>.<ext>" when num > 0. */
native err20005(const dir[], const path[], num, dest[]);
/* The directory of path: everything before the last '/'. */
native err20006(const path[], dir[]);
/* Clears the list of album names. */
native err20007();
/* Appends a name to the list of album names. */
native err20008(const name[]);
/* Returns -1 when the list of album names holds name, else 0. */
native err20009(const name[]);
/* Starts a background copy; fileSync polls it, as fileDelete, fileMove and fileRename. */
native fileCopy(const src[], const dest[]);
native fileDelete(const url[]);
native fileDeleteUSB(const url[]);
native fileGetFreeSize(const url[], &kb);
native fileLstat(const url[], &st_mode, &file_size);
native fileMkdir(const url[]);
native fileMove(const src[], const dest[]);
native fileRename(const url[], const name[]);
native fileSync(&percent, &status);
/* Takes the first queued message with id msg. */
native FindMessage(msg, &{Float,_}:a=0, &{Float,_}:b=0, &c=0, &d=0);
native FindMessageStr(msg, str[], maxlength);
native FontID(font);
native FrameRate();
native Float:FrameRateF();
/* Copies the value of the argument name of the calling script; returns 1 when it exists. */
native GetArg(const name[], value[], maxlength);
native GetColor(obj, &Float:r=0.0, &Float:g=0.0, &Float:b=0.0, &Float:a);
native GetLocalScale(obj, &Float:x, &Float:y, &Float:z);
/* The point at offset (off_x, off_y, off_z) of obj, in the frame of ref. */
native GetOffPos(obj, &Float:x, &Float:y, &Float:z, Float:off_x, Float:off_y, Float:off_z, ref);
native GetOptionF(obj, option, &Float:value);
native GetOptionI(obj, option, &value);
native GetOptionP(obj, option, const data[], cells);
native GetPos(obj, &Float:x=0.0, &Float:y=0.0, &Float:z=0.0, ref);
/* The point at t (0.0 to 1.0) along the count nodes of a Hermite object. */
native GetPosHermite(obj, &Float:x, &Float:y, &Float:z, Float:t, count);
native GetPubVar(obj, const name[]);
native GetQuat(obj, &Float:x, &Float:y, &Float:z, &Float:w, ref);
/* order: 0 rotates in the order Y, X, Z, 1 in the order Z, Y, X. */
native GetRot(obj, &Float:x, &Float:y, &Float:z, ref, order);
native GetScale(obj, &Float:x=0.0, &Float:y=0.0, &Float:z=0.0);
/* The virtual screen of the camera of the scene of obj. */
native GetScreen(obj, &Float:screen_width, &Float:screen_height, &Float:screen_depth);
native getstring(const str[], maxlength);
native GetSystemParam(id);
native GetSystemParamStr(id, value[], maxlength);
native Hide(obj);
native HideChildren(obj);
/* Clears the history of the pages. */
native hisClear();
native iconCancel(handle);
/* Starts loading the icons of a save into cache file index; each load flag passes only when it is 1. */
native iconLoad(index, const url[], &handle, load_normal, load_copy, load_delete);
/* Polls iconLoad: the end status, whether the save is damaged and its title over two lines. */
native iconSync(handle, &status, &damaged, title1[], title2[], size1, size2);
/* Starts decoding an image for the calling script; an http: or https: URL is downloaded first. */
native imgDecStart(const url[], req_width, req_height, image_mode, priority, flags);
/* Starts rendering str into a texture. */
native imgDecStringStart(const str[], char_size, half_width, full_width, width_limit);
native imgDecSync();
native imgGetResource(id);
native imgHeight();
native imgKeepTex(texture);
native imgOrgHeight();
native imgOrgWidth();
native imgReleaseTex(texture);
native imgTexHeight();
native imgTexID();
native imgTexOrgHeight();
native imgTexOrgWidth();
native imgTexWidth();
/* Cuts a rectangle out of the decoded image into a texture. */
native imgTrimStart(x, y, w, h);
native imgTrimSync();
native imgWidth();
native InitNodeArray(obj, count, loop);
native instGetStatus(&status, &inst_result, &total, &done, &trailer_total, &trailer_done);
native instStart();
native IsValidObject(obj);
native listAddLast(value);
native listGetAt(index);
native listGetLast();
native listInsert(index, value);
native listRemoveAll();
native listRemoveAt(index);
native listRemoveLast();
native listSize();
/* Turns obj towards the point (at_x, at_y, at_z) of target, with the up vector given in the frame of up_obj. */
native LookAt(obj, target, Float:at_x, Float:at_y, Float:at_z, up_obj, Float:up_x, Float:up_y, Float:up_z);
native modelGetInfo(what, value[], maxlength);
native modelLoadStart(const url[], option);
native modelLoadSync();
/* Opens a message box and puts the script to sleep until it is answered (msgboxSync). */
native msgboxStart(type, const message[], const yes[], const no[], flag);
native msgboxSync();
/* The two sticks (-100 to 100) and the pressure of twelve buttons (0 to 256) of a pad: port 0, 1 or -1 for both. */
native PadAnalog(&left_y, &left_x, &right_y=0, &right_x=0, &press_right=0, &press_left=0, &press_up=0, &press_down=0,
        &press_circle=0, &press_square=0, &press_triangle=0, &press_cross=0, &press_l1=0, &press_l2=0, &press_r1=0,
        &press_r2=0, port);
/* The buttons (PAD_*) of a pad: port 0, 1 or -1 for the active one. */
native PadState(&held=0, &pressed=0, &released=0, &repeated=0, port);
/* Returns the parent of obj (0: the object of the calling script). */
native Parent(obj);
native PeekMessage(&msg, &a, &{Float,_}:b, &c, &d=0);
/* Opens a RealPlayer URL and ends the script; resume is kept for the return. */
native play(url, resume);
native printf(const format[], {Float,_}:...);
native QuatInterp(&Float:x, &Float:y, &Float:z, &Float:w, Float:ax, Float:ay, Float:az, Float:aw, Float:bx, Float:by,
        Float:bz, Float:bw, Float:t);
native QuatMultiply(&Float:x, &Float:y, &Float:z, &Float:w, Float:ax, Float:ay, Float:az, Float:aw, Float:bx, Float:by,
        Float:bz, Float:bw);
native RecvMessage(&msg, &{Float,_}:a=0, &{Float,_}:b=0, &{Float,_}:c=0, &{Float,_}:d=0);
native RecvMessageStr(&msg, str[], maxlength);
/* Returns the scene of obj (0: the object of the calling script). */
native Scene(obj);
/* Tells whether the clock of the console is within 2000 to 2030. */
native scnfCheckDate(&ok);
native scnfDBConvCheck();
native scnfGetAspect();
native scnfGetATOKBind();
native scnfGetATOKMode();
native scnfGetAutoDiag();
/* Splits /proc/cmdline into up to five words. */
native scnfGetCmdline(const word1[], const word2[], const word3[], const word4[], const word5[], size1, size2, size3,
        size4, size5);
native scnfGetDateNotation();
native scnfGetDVDProg();
native scnfGetIniMode();
native scnfGetKBRepeatS();
native scnfGetKBRepeatW();
native scnfGetKBType();
native scnfGetLang();
native scnfGetMSDblclk();
native scnfGetMSLr();
native scnfGetMSSpeed();
native scnfGetRmGA();
native scnfGetRmMenuCMD();
native scnfGetRmSupport();
native scnfGetRmSysMenu();
native scnfGetSpdif();
native scnfGetSummerTime();
native scnfGetTimeNotation();
native scnfGetTimeZoneCity();
native scnfGetVideo();
native scnfRead();
native scnfSetAspect(aspect);
native scnfSetATOKBind(key_bind);
native scnfSetATOKMode(atok_mode);
native scnfSetAutoDiag(enable);
native scnfSetDateNotation(notation);
native scnfSetDateTime(date_year, date_month, date_day, time_hour, time_minute, time_second, is_dst);
native scnfSetDVDProg(enable);
native scnfSetIniMode(init_mode);
native scnfSetKBRepeatS(speed);
native scnfSetKBRepeatW(wait);
native scnfSetKBType(type);
native scnfSetLang(lang);
native scnfSetMSDblclk(speed);
native scnfSetMSLr(swap);
native scnfSetMSSpeed(speed);
native scnfSetRmGA(enable);
native scnfSetSpdif(enable);
native scnfSetSummerTime(enable);
native scnfSetSystemOption();
native scnfSetTimeNotation(notation);
/* Sets the city of the time zone, and its time zone. */
native scnfSetTZCityAndVal(city);
native scnfSetVideo(video);
native scnfSync();
native scnfWrite();
native scnfwriteDVDProgBit();
native ScriptState(obj);
native Self();
native SendMessage(obj, msg, {Float,_}:a, {Float,_}:b, {Float,_}:c, {Float,_}:d);
native SendMessageStr(obj, msg, const str[]);
native SetAmbient(obj, Float:r, Float:g, Float:b, a);
native SetClipRange(obj, Float:near, Float:far);
native SetColor(obj, Float:r, Float:g, Float:b, Float:a);
native SetItemOptionF(obj, item, option, Float:value, ...);
native SetItemOptionI(obj, item, option, value);
native SetItemOptionP(obj, item, option, const Float:data[], cells);
native SetLightColor(obj, light, Float:r, Float:g, Float:b, a);
native SetLightVector(obj, light, Float:x, Float:y, Float:z);
native SetLocalScale(obj, Float:x, Float:y, Float:z);
native SetModel(obj, model);
native SetNodePos(obj, Float:x, Float:y, Float:z, node);
native SetOptionF(obj, option, Float:value, {Float,_}:...);
native SetOptionI(obj, option, {Float,_}:value, {Float,_}:...);
native SetOptionP(obj, option, const {Float,_}:data[], cells);
native SetOptionS(obj, option, const str[]);
native SetPos(obj, Float:x, Float:y, Float:z, ref);
native SetPubVar(obj, const name[], {Float,_}:value);
native SetQuat(obj, Float:x, Float:y, Float:z, Float:w, ref);
/* order: 0 rotates in the order Y, X, Z, 1 in the order Z, Y, X. */
native SetRot(obj, Float:x, Float:y, Float:z, ref, order);
native SetScale(obj, Float:x, Float:y, Float:z);
native SetSystemParam(id, value);
native SetSystemParamStr(id, const value[]);
native SetTexture(obj, {Float,_}:texture, option, tex_mode);
native shortcutDel(page, const key[], const url[]);
/* Field 0 (label), 1 (key) or 2 (url) of shortcut index. */
native shortcutGet(page, index, field, value[], maxlength);
native shortcutNum(page);
native shortcutSet(page, const label[], const key[], const url[]);
native Show(obj);
native ShowChildren(obj);
native Float:sin(Float:angle);
native skbGetString(str[], maxlength);
/* Opens the software keyboard on obj until skbSync; chars are the only ones allowed (allow_only) or forbidden. */
native skbStart(obj, kind, input_mode, flags, length_unit, max_length, const chars[] = "", allow_only);
/* The kinds of skbStart: the keyboard it opens on (TextIM::Open of textim.cpp). */
const SKB_KEYBOARD_DEFAULT = 0;      /* the hiragana one, or no change with SKB_LOCK_KEYBOARD */
const SKB_KEYBOARD_HIRAGANA = 1;
const SKB_KEYBOARD_KATAKANA = 2;
const SKB_KEYBOARD_ABC = 3;          /* without SKB_LOCK_KEYBOARD, ABC or QWERTY as the system settings say */
const SKB_KEYBOARD_QWERTY = 4;       /* the same */
const SKB_KEYBOARD_NUMBERS = 5;
const SKB_KEYBOARD_SYMBOLS = 6;
const SKB_KEYBOARD_ALPHANUMERIC = 7; /* only the ABC (or QWERTY) and the number keyboards, no kana */
/* The input modes of skbStart: the mode of the ATOK conversion (fepbar.c), unchanged from 12 on. */
const SKB_INPUT_DEFAULT = 0;              /* unchanged */
const SKB_INPUT_DIRECT = 1;               /* the conversion off: the keys type their characters */
const SKB_INPUT_HIRAGANA = 2;
const SKB_INPUT_KATAKANA = 3;             /* full-width */
const SKB_INPUT_HALF_KATAKANA = 4;
const SKB_INPUT_ALNUM = 5;                /* full-width letters and digits */
const SKB_INPUT_HALF_ALNUM = 6;
const SKB_INPUT_HIRAGANA_FIXED = 7;       /* fixed input: the same five, without kanji conversion */
const SKB_INPUT_KATAKANA_FIXED = 8;
const SKB_INPUT_HALF_KATAKANA_FIXED = 9;
const SKB_INPUT_ALNUM_FIXED = 10;
const SKB_INPUT_HALF_ALNUM_FIXED = 11;
/*
 * Bits of the flags of skbStart. SKB_KEYBOARD_QWERTY with SKB_INPUT_DIRECT and exactly SKB_LOCK_KEYBOARD,
 * SKB_LOCK_INPUT_MODE and SKB_KANA_LOCK_FIXED also disables the width and the ABC/QWERTY keys.
 */
const SKB_LOCK_KEYBOARD = 0x01;   /* the user cannot change the keyboard */
const SKB_LOCK_INPUT_MODE = 0x02; /* the user cannot change the input mode */
const SKB_SHOW_KEYBOARD = 0x04;   /* shown even when the system settings hide the software keyboard */
const SKB_KANA_LOCK = 0x08;       /* the kana lock of a USB keyboard on */
const SKB_KANA_LOCK_FIXED = 0x10; /* the kana lock of a USB keyboard cannot be toggled */
const SKB_FIXED_INPUT = 0x20;     /* no kanji conversion (the SKB_INPUT_*_FIXED modes); F6 to F10 do nothing */
const SKB_HALF_WIDTH = 0x40;      /* the width key off and disabled: half-width characters */
const SKB_FULL_WIDTH = 0x80;      /* the width key on and disabled: full-width characters */
/* The units of length_unit for max_length. */
const SKB_LENGTH_BYTES = 0; /* bytes of UTF-8 */
const SKB_LENGTH_CHARS = 1; /* UCS-2 characters */
native skbSync();
native sndBgmPlay(bgm);
native sndBgmStop(bgm);
native sndSePlay(se);
native snprintf(dest[], maxlength, const format[], {Float,_}:...);
native soundBgmLoadStart(const bd[], const hd[], const sq[]);
native soundBgmLoadSync();
native soundBgmPlay();
native soundBgmSeek(position);
native soundBgmStop();
/* what 0 to 4: the path of a loaded bank (bgm bd, hd, sq, se bd, hd); 5: the state of the BGM player. */
native soundGetInfo(what, value[], maxlength);
native soundSeLoadStart(const bd[], const hd[]);
native soundSeLoadSync();
native soundSePlay(se, volume);
native sprintf(dest[], const format[], {Float,_}:...);
native Float:sqrt(Float:value);
native strcmp(const str1[], const str2[]);
native strncmp(const str1[], const str2[], count);
native svdatAddFolder(const name[], const comment[]);
/* Returns -1 for a name too long or not UTF-8, -2 for a forbidden character, -3 for "." and "..", else 0. */
native svdatChkFolderName(const name[]);
/* The svdat* natives that take a &job start a background job, polled with svdatSync. */
native svdatCopy(const src[], const dest[], overwrite, &job);
native svdatCopyFolder(const src[], const dest[], overwrite, &job);
native svdatCountData(const path[], const skip_folder[], &job);
native svdatDelete(const path[], &job);
native svdatFolderClear();
native svdatGetFolder(index, name[], comment[], name_size, comment_size);
native svdatGetFreeSize(dev_mode, port, slot, const path[], &free_kb);
/* Reads the result of svdatInformation. */
native svdatGetInfoData(job, name[], dir, &file_size, title1[], title2[], &save_kind, &no_copy, &date_year, &date_month,
        &date_day, &time_hour, &time_minute, &time_second, &damaged, name_size, dir_size, title1_size, title2_size);
native svdatGetMCInfo(port, slot, &card_type, &free_blocks, &formatted, &status);
/* The last component of path. */
native svdatGetName(const path[], name[], maxlength);
native svdatInformation(const path[], &job);
native svdatMCFormat(port, slot, &job);
native svdatMCInfo(port, slot, &job);
native svdatMkdir(const path[], &job);
native svdatMove(const src[], const dest[], overwrite, &job);
/* Cuts src to at most max_bytes bytes without splitting a UTF-8 character. */
native svdatStringCutByte(max_bytes, const src[], dest[], maxlength);
/* Strips the leading and trailing spaces of src. */
native svdatStrTrmSpc(const src[], dest[], maxlength);
native svdatSync(&percent, &status, job);
native sverGetDVDVer(version[]);
native sverGetMACAddr(address[]);
native sverGetModelName(name[]);
native sverGetPS1DrvVer(version[]);
/* Version index: 0 the installed one, 1 and 2 the two read by sysInitBNVersion. */
native sysGetBNVersion(index, version[]);
native sysInitBNVersion(const file[]);
/* Runs a command "name;key=value;key=value...". */
native System(const command[]);
native SystemCommand(command, {Float,_}:...);
/* Looks the installed version up in the list file path and writes the value it maps to back into path. */
native sysUpdateChkBnVer(path[]);
native sysUpdateDlStart(const user[], const password[], const url[], const dest[]);
native sysUpdateDlStop();
native sysUpdateDlSync(&dl_progress);
native sysUpdateEnd();
native sysUpdateExit();
/* Reads a file of the partition hdd0:__contents. */
native sysUpdateGetEura(const file[], buf[], maxlength);
native sysUpdateGetStat(&status, &sysup_result, &total_steps, &step);
/* Reads the user, the password and the URL of the update server. */
native sysUpdateGetUrl(user[], password[], url[]);
native sysUpdateInit();
native sysUpdateMkBntmp();
native sysUpdateSetup();
native sysUpdateStart();
native sysUpdateTMFinish();
native sysUpdateTMPath(path[], const prefix[], const name[]);
/* Downloads the package url to file, or with info the update information (url names the last path component). */
native sysUpdateTMStart(const url[], const file[], info);
native sysUpdateTMStop();
native sysUpdateTMSync(&tm_progress);
native sysUpdateTMUrl(index, url[]);
native tickcount();
/* Local time, or UTC when utc is 1. */
native time(&time_hour, &time_minute, &time_second, utc);
/* Destroys obj after the given number of frames. */
native TimerDie(obj, frames);
native topInitStart();
native topInitSync();
native wndGetName(window, name[], maxlength);
native wndGetScene(window);
native wndGetScreen(window, &screen_width, &screen_height, &screen_depth);
native xmlGetURI(uri[], maxlength);
native xmlLoadAbortAll();
native xmlLoadGetInfo(what, value[], maxlength);
/* option 100: fade-out time (0 to 1000); 200: load the document only; 10100: hide the window (local windows only). */
native xmlLoadSetOptionI(option, value);
/* The POST body of the next load: source 0 the string data, 1 the same as a form, 2 byte_count bytes of data. */
native xmlLoadSetPost(source, const data[], byte_count);
/* Loads the page url in the window target (XMLLOAD_*), posting post, with arguments arg. */
native xmlLoadStart(const url[], const post[] = "", const arg[] = "", target);
native xmlLoadSync();
/* Makes path absolute, relative to the window of the script. */
native xmlMakeFullPath(path[], maxlength);
