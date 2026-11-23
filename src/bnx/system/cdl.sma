/*
 * /opt0/bn/script/system/cdl.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The DNAS content download page: checks, downloads and installs the first entry of the download queue,
 * then authenticates it and offers to start it, as its jump flag says; at the end it goes to its nextref.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

/* Fields of a queue entry for cdlQueGetData. */
const CDL_TITLE = 3;
const CDL_NEXTREF = 14;
const CDL_PAT_NAME = 20; /* the partition it is installed to */
const CDL_THUMBNAIL = 22;
const CDL_JUMP_FLAG = 24;

/* Values of the jump flag: what follows the install. */
const JUMP_GAME_CHANNEL = 0;
const JUMP_MOVIE_CHANNEL = 1;
const JUMP_BOOT = 2; /* start the game */
const JUMP_NONE = 3;

/* States of instGetStatus. */
const INST_DONE = 3;
const INST_ERROR = 4;

/* Results of cdlCheckSync (0 while the check runs). */
const CHECK_OK = 1;
const CHECK_SAME_VERSION = -100;  /* the same version is installed */
const CHECK_LOWER_VERSION = -101; /* a newer version is installed */
const CHECK_SAME_INSTALL = -200;  /* as CHECK_SAME_VERSION and CHECK_LOWER_VERSION, from the second check */
const CHECK_LOWER_INSTALL = -201;
const CHECK_NO_SPACE_DL = -300;   /* no room for the download */
const CHECK_NO_SPACE_PAT = -301;  /* no room for the partition */
const CHECK_NO_SPACE_INST = -302; /* no room for the installation */

/* States of cdlGetStatus: 1 while it downloads, a negative state is an error code. */
const DL_DONE = 5;
const DL_ABORTED = 6;
const DL_ERR_LENGTH = -10004;     /* the body ended before its Content-Length */
const DL_ERR_HTTP_LENGTH = -1015; /* the same, from the HTTP library */

const AUTH_NO_DATA = -27951; /* authGetStatus: the title has no DNAS data to authenticate */

/* The units of format_size. */
const UNIT_KB = 0;
const UNIT_MB = 1;

forward Float:get_install_progress(size, done_size, trailer, trailer_done);

#include "wait.inc"

#include "tag.inc"

#include <float>

#include "imgsync.inc"

/* Returns whether str is a packed string. */
is_packed(str[])
{
    return str[0] <= 0 || str[0] > 255;
}

/* Appends src to dest (maxlen cells), packed or unpacked like dest. */
append_string(dest[], src[], maxlen)
{
    new dest_len;
    new src_len;
    new i;
    new room;
    dest_len = strlen(dest);
    src_len = strlen(src);
    if (is_packed(dest))
    {
        room = maxlen * 4 - dest_len - 1;
        if (is_packed(src))
        {
            i = 0;
            for (; i < src_len && i < room; i++)
            {
                dest{dest_len + i} = src{i};
            }
        }
        else
        {
            i = 0;
            for (; i < src_len && i < room; i++)
            {
                dest{dest_len + i} = src[i];
            }
        }
        new pad = 4 - (dest_len + i) % 4;
        for (new j; j < pad; j++)
        {
            dest{dest_len + i + j} = 0;
        }
    }
    else
    {
        room = maxlen - dest_len - 1;
        if (is_packed(src))
        {
            i = 0;
            for (; i < src_len && i < room; i++)
            {
                dest[dest_len + i] = src{i};
            }
        }
        else
        {
            i = 0;
            for (; i < src_len && i < room; i++)
            {
                dest[dest_len + i] = src[i];
            }
        }
        dest[dest_len + i] = 0;
    }
    return;
}

#include "xmlsync.inc"

#include "text.inc"

/* Copies src into dest (maxlen cells) with every occurrence of from replaced by to. */
replace_all(dest[], src[], from[], to[], maxlen)
{
    new count;
    new from_len = strlen(from);
    new to_len = strlen(to);
    new src_pos;
    new dest_pos;
    for (; ; )
    {
        new found = find_substring(src, from, src_pos);
        if (found < 0)
        {
            copy_substring(dest, dest_pos, src, src_pos, -1, maxlen);
            return count;
        }
        copy_substring(dest, dest_pos, src, src_pos, found - src_pos, maxlen);
        dest_pos = dest_pos + (found - src_pos);
        append_string(dest, to, maxlen);
        dest_pos = dest_pos + to_len;
        src_pos = found + from_len;
    }
    return 0;
}

/* Same as get_tag_attr2 of text.inc. */
get_tag_attr(tag[], id[], attr[], value[], maxlen)
{
    if (0 == tag[0] || 0 == id[0])
    {
        return 0;
    }
    new node = domFindTag(domRoot(), tag, id, 0);
    if (node)
    {
        new len = domGetData(node, attr, value, maxlen);
        if (0 == strcmp(attr, "src") || 0 == strcmp(attr, "href"))
        {
            xmlMakeFullPath(value, maxlen);
        }
        return len;
    }
    return 0;
}

/* Writes a size of bytes in KB (TEXT kb_id) or, from 10000 KB, in MB (TEXT mb_id). */
format_size(dest[], bytes, kb_id[], mb_id[])
{
    new kb = bytes / 1024;
    new mb = bytes / 1048576;
    new unit;
    new amount;
    if (0 == bytes)
    {
        unit = UNIT_KB;
        amount = 0;
    }
    else if (0 < bytes && kb < 1)
    {
        unit = UNIT_KB;
        amount = 1;
    }
    else if (kb < 10000)
    {
        unit = UNIT_KB;
        amount = kb;
    }
    else
    {
        unit = UNIT_MB;
        amount = mb;
    }
    new unit_text[32];
    switch (unit)
    {
        case UNIT_KB:
        {
            get_tag_attr("TEXT", kb_id, "value", unit_text, 32);
        }
        case UNIT_MB:
        {
            get_tag_attr("TEXT", mb_id, "value", unit_text, 32);
        }
    }
    sprintf(dest, "%d %s", amount, unit_text);
}

#include "focus.inc"

#include "misc.inc"

#include "dom.inc"

#include "sysconf.inc"

new g_dialog_args[128];
new g_msg[128];
/* TEXT values for g_msg or for the buttons of a dialog. */
new g_text1[32];
new g_text2[32];
new g_text3[32];
new g_text4[32];
new g_text5[32];

/* Like show_message, without the focus and the pad help. */
show_message_nofocus(msg[])
{
    sprintf(g_dialog_args, "msg=%s", msg);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    SetOptionI(dialog, TAG("ZSRT"), ZSORT_OVERLAY);
    AttachScript(dialog, _, "confirm_d", g_dialog_args);
    SetPubVar(dialog, "manager", Self());
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    Destroy(dialog);
}

/* Shows msg until its dialog closes by itself. */
show_moment(msg[])
{
    sprintf(g_dialog_args, "msg=%s", msg);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    AttachScript(dialog, _, "confirm_moment", g_dialog_args);
    SetPubVar(dialog, "manager", Self());
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
}

/* Asks with the dialog confirm_yesno2, msg being the image of the notice; returns 0 for yes, 1 for no or cross. */
ask_yes_no_notice(msg[], yes[], no[], focus)
{
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    sprintf(g_dialog_args, "msg=%s;yes=%s;no=%s", msg, yes, no);
    AttachScript(dialog, _, "confirm_yesno2", g_dialog_args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus);
    SetPubVar(dialog, "select", 0);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    new answer = GetPubVar(dialog, "select");
    Destroy(dialog);
    switch (answer)
    {
        case -1:
        {
            answer = 1;
        }
        case 0:
        {
            answer = 0;
        }
        case 1:
        {
            answer = 1;
        }
    }
    return answer;
}

#include "string.inc"

/*
 * Reboots into the PlayStation 2 disc in the drive for the game installed with the boot path path, after
 * checking that the disc boots boot; returns a negative value if it cannot.
 */
boot_game_disc(path[], boot[])
{
    new plain[4][32]; /* the arguments of the reboot: the DES keys, the boot command, the boot path, path */
    new encoded[4][64];
    new len;
    new cipher[256];
    new i;
    new result = -1;
    new disc_type;
    sprintf(plain[3], path);
    disc_type = get_cdvd_type();
    switch (disc_type)
    {
        case CDVD_NO_DISC:
        {
            get_tag_attr("TEXT", "msggame_exechdd_disc", "value", g_text1, 32);
            get_tag_attr("TEXT", "msggame_execdisc_insertdisc", "value", g_text2, 32);
            sprintf(g_msg, "%s\n%s", g_text1, g_text2);
            show_message_nofocus(g_msg);
            return -1;
        }
        case CDVD_DETECTING:
        {
            get_tag_attr("TEXT", "msggame_detecting_disc1", "value", g_text1, 32);
            get_tag_attr("TEXT", "msggame_detecting_disc2", "value", g_text2, 32);
            get_tag_attr("TEXT", "msggame_detecting_disc3", "value", g_text3, 32);
            sprintf(g_msg, "%s\n%s\n%s", g_text1, g_text2, g_text3);
            show_message_nofocus(g_msg);
            return -1;
        }
        case CDVD_PS2_CD:
        {
            plain[1] = "BootPs2Cd";
            result = err10003(plain[2]);
        }
        case CDVD_PS2_CDDA:
        {
            plain[1] = "BootPs2Cd";
            result = err10003(plain[2]);
        }
        case CDVD_PS2_DVD:
        {
            plain[1] = "BootPs2Dvd";
            result = err10004(plain[2]);
        }
        default:
        {
            get_tag_attr("TEXT", "msggame_execdisc_wrongdisc", "value", g_text1, 32);
            sprintf(g_msg, "%s", g_text1);
            show_message_nofocus(g_msg);
            return -1;
        }
    }
    disc_type = get_cdvd_type();
    switch (disc_type)
    {
        case CDVD_NO_DISC:
        {
            get_tag_attr("TEXT", "msggame_exechdd_disc", "value", g_text1, 32);
            get_tag_attr("TEXT", "msggame_execdisc_insertdisc", "value", g_text2, 32);
            sprintf(g_msg, "%s\n%s", g_text1, g_text2);
            show_message_nofocus(g_msg);
            return -1;
        }
        case CDVD_DETECTING:
        {
            get_tag_attr("TEXT", "msggame_detecting_disc1", "value", g_text1, 32);
            get_tag_attr("TEXT", "msggame_detecting_disc2", "value", g_text2, 32);
            get_tag_attr("TEXT", "msggame_detecting_disc3", "value", g_text3, 32);
            sprintf(g_msg, "%s\n%s\n%s", g_text1, g_text2, g_text3);
            show_message_nofocus(g_msg);
            return -1;
        }
        case CDVD_PS2_CD:
        {
            if (result < 0)
            {
                if (0 != err10034())
                {
                    get_tag_attr("TEXT", "msggame_execdisc_wrongdisc", "value", g_text1, 32);
                    sprintf(g_msg, "%s", g_text1);
                    show_message_nofocus(g_msg);
                    return -1;
                }
                get_tag_attr("TEXT", "msggame_execdisc_loadingfail", "value", g_text1, 32);
                sprintf(g_msg, "%s", g_text1);
                show_message_nofocus(g_msg);
                return -1;
            }
        }
        case CDVD_PS2_CDDA:
        {
            if (result < 0)
            {
                get_tag_attr("TEXT", "msggame_execdisc_loadingfail", "value", g_text1, 32);
                sprintf(g_msg, "%s", g_text1);
                show_message_nofocus(g_msg);
                return -1;
            }
        }
        case CDVD_PS2_DVD:
        {
            if (result < 0)
            {
                get_tag_attr("TEXT", "msggame_execdisc_loadingfail", "value", g_text1, 32);
                sprintf(g_msg, "%s", g_text1);
                show_message_nofocus(g_msg);
                return -1;
            }
        }
        default:
        {
            get_tag_attr("TEXT", "msggame_execdisc_wrongdisc", "value", g_text1, 32);
            sprintf(g_msg, "%s", g_text1);
            show_message_nofocus(g_msg);
            return -1;
        }
    }
    result = err10025(plain[2], boot);
    if (result < 0)
    {
        get_tag_attr("TEXT", "msggame_execdisc_wrongdisc", "value", g_text1, 32);
        sprintf(g_msg, "%s", g_text1);
        show_message_nofocus(g_msg);
        return -4;
    }
    SetSystemParam(SYS_PADHELP, 0);
    get_tag_attr("TEXT", "msggame_warn_wait", "value", g_text1, 32);
    get_tag_attr("TEXT", "msggame_warn_wait2", "value", g_text2, 32);
    get_tag_attr("TEXT", "msggame_warn_console_power", "value", g_text3, 32);
    get_tag_attr("TEXT", "msggame_warn_console_disctray", "value", g_text4, 32);
    sprintf(g_msg, "%s%s\n\n%s\n%s", g_text1, g_text2, g_text3, g_text4);
    show_moment(g_msg);
    err10024(plain[2], plain[2]);
    err10019(plain[0]);
    encode_base64(plain[0], 16, encoded[0], 64);
    err10020();
    new count = 4;
    i = 1;
    for (; i < count; i++)
    {
        len = err10021(plain[i], cipher);
        encode_base64(cipher, len, encoded[i], 64);
        apply_rot13(encoded[i]);
    }
    /* Slot 0 has the keys in Base64, the others the arguments encrypted with them, in Base64 and ROT13. */
    err10022(0, encoded[0]);
    err10022(1, encoded[1]);
    err10022(2, encoded[2]);
    err10022(3, encoded[3]);
    write_current_channel("Top");
    err10023(count);
    return 0;
}

/* Starts what the SYSTEM.CNF of the partition boots; returns a negative value if it cannot. */
boot_partition(partition[])
{
    new plain[3][32]; /* the arguments of the reboot: the DES keys, the boot command, the boot path */
    new encoded[3][64];
    new buffer[64];
    new len;
    new i;
    new type;
    type = err10026(partition, plain[2], buffer);
    if (type < 0)
    {
        get_tag_attr("TEXT", "msggame_execdisc_dataerr1", "value", g_text1, 32);
        get_tag_attr("TEXT", "msggame_execdisc_dataerr2", "value", g_text2, 32);
        sprintf(g_msg, "%s\n%s", g_text1, g_text2);
        show_message_nofocus(g_msg);
        return -1;
    }
    switch (type)
    {
        case BOOT2_CDROM:
        {
            return boot_game_disc(plain[2], buffer);
        }
        case BOOT2_PFS:
        {
            plain[1] = "BootHddApp";
        }
        case BOOT2_PATINFO:
        {
            plain[1] = "BootHddApp";
        }
        case BOOT2_NOBOOT:
        {
            get_tag_attr("TEXT", "msggame_execdisc_nostart", "value", g_text1, 32);
            sprintf(g_msg, "%s", g_text1);
            show_message_nofocus(g_msg);
            return -2;
        }
        default:
        {
            return -3;
        }
    }
    SetSystemParam(SYS_PADHELP, 0);
    get_tag_attr("TEXT", "msggame_warn_wait", "value", g_text1, 32);
    get_tag_attr("TEXT", "msggame_warn_wait2", "value", g_text5, 32);
    get_tag_attr("TEXT", "msggame_warn_console_power", "value", g_text2, 32);
    sprintf(g_msg, "%s%s\n\n%s", g_text1, g_text5, g_text2);
    show_moment(g_msg);
    err10019(plain[0]);
    encode_base64(plain[0], 16, encoded[0], 64);
    err10020();
    new count = 3;
    i = 1;
    for (; i < count; i++)
    {
        len = err10021(plain[i], buffer);
        encode_base64(buffer, len, encoded[i], 64);
        apply_rot13(encoded[i]);
    }
    err10022(0, encoded[0]);
    err10022(1, encoded[1]);
    err10022(2, encoded[2]);
    write_current_channel("Top");
    err10023(count);
    return 0;
}

/* Shows the notice of the game in the partition, if it has one, and starts it if the user agrees. */
boot_downloaded_game(partition[], focus)
{
    new notice[64] = {0};
    new mode;
    new size;
    new result;
    sprintf(notice, "%s/res/notice.png", partition);
    if (0 == fileLstat(notice, mode, size))
    {
        get_tag_attr("TEXT", "msggame_notice_play", "value", g_text2, 32);
        get_tag_attr("TEXT", "msggame_notice_notplay", "value", g_text3, 32);
        result = ask_yes_no_notice(notice, g_text2, g_text3, focus);
    }
    if (0 == result)
    {
        wait_ms(200);
        err11012();
        result = boot_partition(partition);
        err11013();
    }
    return result;
}

/* Returns whether state is a state of cdlGetStatus rather than an error code. */
is_valid_status(state)
{
    return 0 <= state <= DL_ABORTED;
}

/* Aborts the download and waits for it to stop. */
abort_download()
{
    cdlAbort();
    for (; ; )
    {
        new state;
        new length;
        new pos;
        new error = cdlGetStatus(state, length, pos);
        if (0 != error)
        {
            new msg[512];
            get_tag_attr("TEXT", "text_dlabort_error", "value", msg, 512);
            show_error(msg, error);
            go_to_next();
        }
        if (0 == error && (state == DL_ABORTED || state == DL_DONE || !is_valid_status(state)))
        {
            break;
        }
        sleep;
    }
}

new g_focus;
new g_nextref[256];
new g_jump_flag;
new g_saved_cross_label[64]; /* the pad help label of cross before the page set it to g_quit_label */
new g_quit_label[64];
/* The labels of the page, by index. */
enum
{
    LABEL_TITLE,        /* three lines, 0 to 2 */
    LABEL_SIZE = 3,     /* the size of the download */
    LABEL_ACTION,       /* "download" or "install" */
    LABEL_CAUTION0,
    LABEL_CAUTION1,
    LABEL_PERCENT,
    LABEL_TIME_LEFT,
    LABEL_COUNT
};

new Float:g_label_x[LABEL_COUNT] = {100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0};
new Float:g_label_y[LABEL_COUNT] = {-100.0, -80.0, -60.0, -40.0, -20.0, 20.0, 40.0, 80.0, 100.0};
new g_label_size[LABEL_COUNT] = {17, 17, 17, 15, 15, 15, 15, 17, 15};
new g_percent_text[4];
new g_time_left_text[10];
new g_hour_text[4];
new g_minute_text[4];
new g_second_text[4];

/* Waits for the check started by cdlCheckStart; returns 1 if the entry can be installed, else an error. */
wait_check()
{
    for (; ; )
    {
        new result = cdlCheckSync();
        if (0 != result)
        {
            return result;
        }
        sleep;
    }
    return 0;
}

main()
{
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
    SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
    SystemCommand(SYSCMD_HIDE_WINDOW, 1);
    cdlInit();
    cdlQueGetData(0, CDL_NEXTREF, g_nextref, 256);
    {
        new data[1];
        cdlQueGetData(0, CDL_JUMP_FLAG, data, 1);
        g_jump_flag = data[0];
    }
    GetSystemParamStr(SYS_STR_PADHELP_RD, g_saved_cross_label, 64);
    get_tag_attr("TEXT", "text_quit", "value", g_quit_label, 64);
    g_focus = create_focus_plane2("focus");
    cdlCheckStart(0);
    {
        new result = wait_check();
        if (CHECK_OK != result)
        {
            new msg_id[64];
            new with_code;
            switch (result)
            {
                case CHECK_SAME_INSTALL, CHECK_SAME_VERSION:
                {
                    msg_id = "text_inst_check_samever";
                }
                case CHECK_LOWER_INSTALL, CHECK_LOWER_VERSION:
                {
                    msg_id = "text_inst_check_lowver";
                }
                case CHECK_NO_SPACE_DL:
                {
                    msg_id = "text_inst_check_nospc_dl";
                }
                case CHECK_NO_SPACE_PAT:
                {
                    msg_id = "text_inst_check_nospc_pat";
                }
                case CHECK_NO_SPACE_INST:
                {
                    msg_id = "text_inst_check_nospc_inst";
                }
                default:
                {
                    msg_id = "text_inst_check_unknown";
                    with_code = 1;
                }
            }
            new msg[512];
            get_tag_attr("TEXT", msg_id, "value", msg, 512);
            if (with_code)
            {
                show_error(msg, cdlCheckGetError());
            }
            else
            {
                show_message(msg);
            }
            cdlQueRemove(0);
            go_to_next();
        }
    }
    new defocus;
    defocus = CreateDefocus(Disp2D(0));
    SetPos(defocus, 320.0, 240.0, 0.0, 0);
    SetColor(defocus, 1.0, 1.0, 1.0, 0.0);
    AnimColor(defocus, 0.5, 0.5, 0.5, 1.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    SetOptionI(defocus, TAG("ZSRT"), ZSORT_BACKGROUND);
    SetOptionF(defocus, TAG("ZOFS"), -1.0);
    SetOptionI(defocus, TAG("LEVL"), 3);
    new window;
    window = CreateNull(Disp2D(0));
    SetPos(window, 320.0, 240.0, 0.0, 0);
    SetOptionI(window, TAG("ZSRT"), ZSORT_BACKGROUND);
    AttachScript(window, _, "window");
    SetPubVar(window, "manager", Self());
    SetPubVar(window, "focus", g_focus);
    SetPubVar(window, "width", 580.0);
    SetPubVar(window, "height", 320.0);
    new thumb;
    thumb = CreatePlane(window);
    SetLocalScale(thumb, 150.0, 150.0, 1.0);
    SetPos(thumb, -150.0, 0.0, 0.0, 0);
    SetColor(thumb, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(thumb, TAG("FDEG"), 1);
    AttachScript(thumb, "decode");
    {
        new url[256];
        cdlQueGetData(0, CDL_THUMBNAIL, url, 256);
        SendMessageStr(thumb, TAG("IMG"), url);
    }
    new labels[LABEL_COUNT];
    for (new i; i < LABEL_COUNT; i++)
    {
        labels[i] = create_string_label(window);
        SetPos(labels[i], g_label_x[i], g_label_y[i], 0.0, 0);
        SetOptionI(labels[i], TAG("ZSRT"), ZSORT_OVERLAY);
        send_value(labels[i], TAG("FSIZ"), g_label_size[i]);
    }
    SetSystemParamStr(SYS_STR_PADHELP_RD, g_quit_label);
    SetSystemParam(SYS_PADHELP, PADHELP_CROSS);
    new progress_bar;
    progress_bar = CreateNull(window);
    SetPos(progress_bar, 100.0, 60.0, 0.0, 0);
    SetOptionI(progress_bar, TAG("ZSRT"), ZSORT_OVERLAY);
    AttachScript(progress_bar, _, "progress_bar");
    SetPubVar(progress_bar, "manager", Self());
    SetPubVar(progress_bar, "focus", g_focus);
    {
        new text[256];
        cdlQueGetData(0, CDL_TITLE, text, 256);
        {
            new title_len = strlen(text);
            new line_start;
            {
                new line;
                for (; line < 3; line++)
                {
                    new line_text[64];
                    {
                        new j;
                        for (; ; j++)
                        {
                            if (line_start + j >= title_len)
                            {
                                send_text(labels[line], TAG("UTF8"), line_text);
                                goto title_done;
                            }
                            if (text{line_start + j} == '\n')
                            {
                                line_start = line_start + j + 1;
                                break;
                            }
                            line_text{j} = text{line_start + j};
                        }
                    }
                    send_text(labels[line], TAG("UTF8"), line_text);
                }
            }
        }
        title_done:
        get_tag_attr("TEXT", "text_download", "value", text, 256);
        send_text(labels[LABEL_ACTION], TAG("UTF8"), text);
        get_tag_attr("TEXT", "text_download_caution0", "value", text, 256);
        send_text(labels[LABEL_CAUTION0], TAG("UTF8"), text);
        get_tag_attr("TEXT", "text_download_caution1", "value", text, 256);
        send_text(labels[LABEL_CAUTION1], TAG("UTF8"), text);
    }
    get_tag_attr("TEXT", "text_percent", "value", g_percent_text, 4);
    get_tag_attr("TEXT", "text_lefttime", "value", g_time_left_text, 10);
    get_tag_attr("TEXT", "text_hour", "value", g_hour_text, 4);
    get_tag_attr("TEXT", "text_minute", "value", g_minute_text, 4);
    get_tag_attr("TEXT", "text_second", "value", g_second_text, 4);
    cdlStart();
    {
        new last_length;
        new last_pos;
        new progress_tick = tickcount();
        for (; ; )
        {
            new state;
            new length;
            new pos;
            new error = cdlGetStatus(state, length, pos);
            if (0 != error)
            {
                printf("error = %d\n", error);
                new msg[512];
                get_tag_attr("TEXT", "text_download_error", "value", msg, 512);
                show_error(msg, error);
                go_to_next();
            }
            switch (state)
            {
                case 0 .. DL_ABORTED:
                {
                }
                case DL_ERR_LENGTH, DL_ERR_HTTP_LENGTH:
                {
                    printf("code = %d\n", state);
                    new msg[256];
                    new text[256];
                    get_tag_attr("TEXT", "text_download_not_completed", "value", msg, 256);
                    replace_all(text, msg, "\\n", "\n", 256);
                    msg = text;
                    new code[8];
                    sprintf(code, "%d", state);
                    replace_all(text, msg, "%ERRORNO", code, 256);
                    msg = text;
                    show_message(msg);
                    go_to_next();
                }
                default:
                {
                    printf("code = %d\n", state);
                    new msg[512];
                    get_tag_attr("TEXT", "text_download_error", "value", msg, 512);
                    show_error(msg, state);
                    go_to_next();
                }
            }
            if (length > 0 && length > pos)
            {
                SetPubVar(progress_bar, "progress", floatround(100.0 * float(pos) / float(length), 1));
            }
            if (length > 0 && length != last_length)
            {
                new size[64];
                format_size(size, length, "text_kilobyte", "text_megabyte");
                send_text(labels[LABEL_SIZE], TAG("UTF8"), size);
                last_length = length;
            }
            new Float:percent;
            if (length > 0)
            {
                percent = 100.0 * float(pos) / float(length);
                new text[64];
                sprintf(text, "%2.2f %s", percent, g_percent_text);
                send_text(labels[LABEL_PERCENT], TAG("UTF8"), text);
            }
            if (length > 0)
            {
                static frames;
                frames = frames + 1;
                new seconds = frames_to_seconds(frames);
                static last_seconds;
                if (seconds != last_seconds)
                {
                    if (seconds > 5 && percent > 1.0)
                    {
                        new left = floatround(seconds * (100.0 - percent) / percent, 0);
                        new left_s = left % 60;
                        new left_m = left / 60 % 60;
                        new left_h = left / 3600;
                        new text[64];
                        append_string(text, g_time_left_text, 64);
                        if (left_h > 0)
                        {
                            sprintf(text, "%s %d %s %d %s", g_time_left_text, left_h, g_hour_text, left_m,
                                    g_minute_text);
                        }
                        else if (left_m > 0)
                        {
                            if (left_m < 10)
                            {
                                sprintf(text, "%s %d %s %d %s", g_time_left_text, left_m, g_minute_text, left_s,
                                        g_second_text);
                            }
                            else
                            {
                                sprintf(text, "%s %d %s", g_time_left_text, left_m, g_minute_text);
                            }
                        }
                        else
                        {
                            sprintf(text, "%s %d %s", g_time_left_text, left_s, g_second_text);
                        }
                        send_text(labels[LABEL_TIME_LEFT], TAG("UTF8"), text);
                    }
                }
                last_seconds = seconds;
            }
            if (state == DL_DONE || state == DL_ABORTED)
            {
                break;
            }
            if (tickcount() - progress_tick > 30000)
            {
                abort_download();
                new msg[256];
                get_tag_attr("TEXT", "msgcom_error_network_timeout", "value", msg, 256);
                show_message_nofocus(msg);
                go_to_next();
            }
            if (pos != last_pos)
            {
                progress_tick = tickcount();
                last_pos = pos;
            }
            new pressed;
            PadState(_, pressed, _, _, -1);
            if (pressed & PAD_CROSS)
            {
                cdlSuspend(1);
                new question[256];
                new text[256];
                get_tag_attr("TEXT", "text_confirm_quit", "value", question, 256);
                new title[32];
                cdlQueGetData(0, CDL_TITLE, title, 32);
                replace_all(text, question, "%TITLE", title, 256);
                question = text;
                replace_all(text, question, "\\n", "\n", 256);
                question = text;
                new yes[32];
                get_tag_attr("TEXT", "text_yes", "value", yes, 32);
                new no[32];
                get_tag_attr("TEXT", "text_no", "value", no, 32);
                new answer = ask_yes_no(question, yes, no, 1);
                cdlSuspend(0);
                progress_tick = tickcount();
                if (0 == answer)
                {
                    error = cdlGetStatus(state, length, pos);
                    if (state == DL_DONE || state == DL_ABORTED)
                    {
                        goto resume;
                    }
                    abort_download();
                    go_to_next();
                }
                resume:
            }
            sleep;
        }
        SetPubVar(progress_bar, "progress", 100);
        while (SCRIPT_FINISHED != ScriptState(progress_bar))
        {
            sleep;
        }
        Destroy(progress_bar);
        progress_bar = 0;
    }
    progress_bar = CreateNull(window);
    SetPos(progress_bar, 100.0, 60.0, 0.0, 0);
    SetOptionI(progress_bar, TAG("ZSRT"), ZSORT_OVERLAY);
    AttachScript(progress_bar, _, "progress_bar");
    SetPubVar(progress_bar, "manager", Self());
    SetPubVar(progress_bar, "focus", g_focus);
    {
        new text[256];
        get_tag_attr("TEXT", "text_install", "value", text, 256);
        send_text(labels[LABEL_ACTION], TAG("UTF8"), text);
        get_tag_attr("TEXT", "text_install_caution0", "value", text, 256);
        send_text(labels[LABEL_CAUTION0], TAG("UTF8"), text);
        get_tag_attr("TEXT", "text_install_caution1", "value", text, 256);
        send_text(labels[LABEL_CAUTION1], TAG("UTF8"), text);
    }
    SetSystemParam(SYS_PADHELP, 0);
    {
        new error;
        error = instStart();
        if (0 != error)
        {
            new msg[512];
            get_tag_attr("TEXT", "text_install_error", "value", msg, 512);
            show_error(msg, error);
            go_to_next();
        }
        for (; ; )
        {
            new inst_state;
            new inst_error;
            new inst_total;
            new inst_done;
            new trailer_size;
            new trailer_done_size;
            error = instGetStatus(inst_state, inst_error, inst_total, inst_done, trailer_size, trailer_done_size);
            if (0 != error)
            {
                new msg[512];
                get_tag_attr("TEXT", "text_install_error", "value", msg, 512);
                show_error(msg, error);
                go_to_next();
            }
            if (inst_state == INST_ERROR)
            {
                new msg[512];
                get_tag_attr("TEXT", "text_install_error", "value", msg, 512);
                show_error(msg, inst_error);
                go_to_next();
            }
            new Float:progress = get_install_progress(inst_total, inst_done, trailer_size, trailer_done_size);
            if (0.0 <= progress <= 100.0)
            {
                SetPubVar(progress_bar, "progress", floatround(progress, 1));
                {
                    new text[64];
                    sprintf(text, "%2.2f %s", progress, g_percent_text);
                    send_text(labels[LABEL_PERCENT], TAG("UTF8"), text);
                }
                new left = get_install_time_left(inst_total, inst_done, trailer_size, trailer_done_size);
                static last_left;
                if (left != last_left)
                {
                    new left_s = left % 60;
                    new left_m = left / 60 % 60;
                    new left_h = left / 3600;
                    new text[64];
                    append_string(text, g_time_left_text, 64);
                    if (left_h > 0)
                    {
                        sprintf(text, "%s %d %s %d %s", g_time_left_text, left_h, g_hour_text, left_m, g_minute_text);
                    }
                    else if (left_m > 0)
                    {
                        if (left_m < 10)
                        {
                            sprintf(text, "%s %d %s %d %s", g_time_left_text, left_m, g_minute_text, left_s,
                                    g_second_text);
                        }
                        else
                        {
                            sprintf(text, "%s %d %s", g_time_left_text, left_m, g_minute_text);
                        }
                    }
                    else
                    {
                        sprintf(text, "%s %d %s", g_time_left_text, left_s, g_second_text);
                    }
                    send_text(labels[LABEL_TIME_LEFT], TAG("UTF8"), text);
                }
                last_left = left;
            }
            if (inst_state == INST_DONE)
            {
                break;
            }
            sleep;
        }
        SetPubVar(progress_bar, "progress", 100);
        while (SCRIPT_FINISHED != ScriptState(progress_bar))
        {
            sleep;
        }
        Destroy(progress_bar);
        progress_bar = 0;
    }
    {
        new auth;
        new auth_error;
        authStart();
        while (0 == (auth = authGetStatus(auth_error)))
        {
            sleep;
        }
        printf("auth result = %d\n", auth);
        if (auth > 0)
        {
        }
        else
        {
            switch (auth_error)
            {
                case AUTH_NO_DATA:
                {
                }
                default:
                {
                    printf("dnas auth error - %d\n", auth_error);
                }
            }
        }
    }
    for (new i; i < LABEL_COUNT; i++)
    {
        send_text(labels[i], TAG("UTF8"), "");
    }
    {
        new Float:color[4];
        GetColor(defocus, color[0], color[1], color[2], color[3]);
        AnimColor(defocus, color[0], color[1], color[2], 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
        GetColor(window, color[0], color[1], color[2], color[3]);
        AnimColor(window, color[0], color[1], color[2], 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
        GetColor(thumb, color[0], color[1], color[2], color[3]);
        AnimColor(thumb, color[0], color[1], color[2], 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
        wait_ms(300);
    }
    soundBgmStop();
    switch (g_jump_flag)
    {
        case JUMP_GAME_CHANNEL:
        {
            new msg[256];
            new text[256];
            get_tag_attr("TEXT", "text_install_finished_game_playnow", "value", msg, 256);
            replace_all(text, msg, "\\n", "\n", 256);
            msg = text;
            new yes[32];
            get_tag_attr("TEXT", "text_yes", "value", yes, 32);
            new no[32];
            get_tag_attr("TEXT", "text_no", "value", no, 32);
            if (0 == ask_yes_no(msg, yes, no, 0))
            {
                cdlExit();
                hisClear();
                xmlLoadStart("file:/opt0/bn/script/game/game.xml", _, _, XMLLOAD_MAIN);
                wait_xml_load();
            }
            go_to_next();
        }
        case JUMP_MOVIE_CHANNEL:
        {
            new msg[256];
            new text[256];
            get_tag_attr("TEXT", "text_install_finished_movie_playnow", "value", msg, 256);
            replace_all(text, msg, "\\n", "\n", 256);
            msg = text;
            new yes[32];
            get_tag_attr("TEXT", "text_yes", "value", yes, 32);
            new no[32];
            get_tag_attr("TEXT", "text_no", "value", no, 32);
            if (0 == ask_yes_no(msg, yes, no, 0))
            {
                cdlExit();
                hisClear();
                xmlLoadStart("file:/opt0/bn/script/movie/movie.xml", _, _, XMLLOAD_MAIN);
                wait_xml_load();
            }
            go_to_next();
        }
        case JUMP_BOOT:
        {
            new msg[256];
            new text[256];
            get_tag_attr("TEXT", "text_install_finished_game_playnow", "value", msg, 256);
            replace_all(text, msg, "\\n", "\n", 256);
            msg = text;
            new yes[32];
            get_tag_attr("TEXT", "text_yes", "value", yes, 32);
            new no[32];
            get_tag_attr("TEXT", "text_no", "value", no, 32);
            if (0 == ask_yes_no(msg, yes, no, 0))
            {
                cdlExit();
                hisClear();
                new pat_name[32];
                new partition[32];
                cdlQueGetData(0, CDL_PAT_NAME, pat_name, 32);
                sprintf(partition, "pfs:/%s", pat_name);
                printf("BootDownloadGame('%s')\n", partition);
                boot_downloaded_game(partition, g_focus);
            }
            go_to_next();
        }
        case JUMP_NONE:
        {
            new msg[256];
            new text[256];
            get_tag_attr("TEXT", "text_install_finished", "value", msg, 256);
            replace_all(text, msg, "\\n", "\n", 256);
            msg = text;
            show_message_nofocus(msg);
            go_to_next();
        }
    }
}

/* Leaves the page for the nextref of the entry. */
go_to_next()
{
    cdlExit();
    hisClear();
    xmlLoadStart(g_nextref, _, _, XMLLOAD_MAIN);
    wait_xml_load();
}

/* Asks msg with the buttons yes and no, select first; returns 0 for yes, 1 for no or cross. */
ask_yes_no(msg[], yes[], no[], select)
{
    new pad_help = GetSystemParam(SYS_PADHELP);
    SetSystemParamStr(SYS_STR_PADHELP_RD, g_saved_cross_label);
    new args[512];
    sprintf(args, "msg=%s;yes=%s;no=%s", msg, yes, no);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 240.0, 0.0, 0);
    AttachScript(dialog, _, "confirm_yesno", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", g_focus);
    SetPubVar(dialog, "select", select);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    new answer = GetPubVar(dialog, "select");
    Destroy(dialog);
    if (answer == -1)
    {
        answer = 1;
    }
    SetSystemParam(SYS_PADHELP, pad_help);
    SetSystemParamStr(SYS_STR_PADHELP_RD, g_quit_label);
    return answer;
}

/* Shows msg in a dialog and waits until it is closed. */
show_message(msg[])
{
    new pad_help = GetSystemParam(SYS_PADHELP);
    SetSystemParamStr(SYS_STR_PADHELP_RD, g_saved_cross_label);
    new args[512];
    sprintf(args, "msg=%s", msg);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 240.0, 0.0, 0);
    AttachScript(dialog, _, "confirm_d", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", g_focus);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    Destroy(dialog);
    SetSystemParam(SYS_PADHELP, pad_help);
    SetSystemParamStr(SYS_STR_PADHELP_RD, g_quit_label);
}

/* Like show_message, as an error, with the error code and its kind if not 0. */
show_error(msg[], code)
{
    new pad_help = GetSystemParam(SYS_PADHELP);
    SetSystemParamStr(SYS_STR_PADHELP_RD, g_saved_cross_label);
    new args[512];
    sprintf(args, "msg=%s", msg);
    if (code)
    {
        new prefix[64];
        if (0 < code)
        {
            get_tag_attr("TEXT", "msgcom_error_http", "value", prefix, 64);
        }
        else if (-25999 <= code <= -25000)
        {
            get_tag_attr("TEXT", "text_error_type_download", "value", prefix, 64);
        }
        else if (-27999 <= code <= -26000)
        {
            get_tag_attr("TEXT", "text_error_type_install", "value", prefix, 64);
        }
        else if (-24999 <= code <= -20000)
        {
            get_tag_attr("TEXT", "msgcom_error_dnas", "value", prefix, 64);
        }
        else
        {
            get_tag_attr("TEXT", "msgcom_error_internal", "value", prefix, 64);
        }
        new number[64];
        sprintf(number, "%d", code);
        append_string(args, "\n", 512);
        append_string(args, "(", 512);
        append_string(args, prefix, 512);
        append_string(args, number, 512);
        append_string(args, ")", 512);
    }
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 240.0, 0.0, 0);
    AttachScript(dialog, _, "confirm_err", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", g_focus);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    Destroy(dialog);
    SetSystemParam(SYS_PADHELP, pad_help);
    SetSystemParamStr(SYS_STR_PADHELP_RD, g_quit_label);
}

/* The script of the thumbnail: decodes the image of IMG (url) into its texture, then ends. */
public decode()
{
    for (; ; )
    {
        new msg;
        new url[256];
        while (RecvMessageStr(msg, url, 256))
        {
            if (msg == TAG("IMG"))
            {
                imgDecStart(url, 256, 256, -1, 100, 0);
                new status = wait_image_decode();
                if (status == IMG_DONE)
                {
                    SetTexture(Self(), imgTexID(), 0, -1);
                    new Float:aspect = 1.0 * imgTexOrgWidth() / imgTexOrgHeight();
                    if (aspect > 1.0)
                    {
                        SetScale(Self(), 1.0, 1.0 / aspect, 1.0);
                    }
                    else
                    {
                        SetScale(Self(), 1.0 * aspect, 1.0, 1.0);
                    }
                    AnimColor(Self(), 1.0, 1.0, 1.0, 1.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
                }
                Abort(Self());
            }
        }
        sleep;
    }
}

frames_to_seconds(frames)
{
    return frames / FrameRate();
}

create_string_label(parent)
{
    new label = CreatePlane(parent);
    SetColor(label, 1.0, 1.0, 1.0, 0.0);
    AttachScript(label, "lw_label_self");
    return label;
}

send_value(obj, msg, number)
{
    SendMessage(obj, msg, number, 0, 0, 0);
}

send_text(obj, msg, text[])
{
    SendMessageStr(obj, msg, text);
}

/* The script of a text label: draws the text of "UTF8" (text) with the size of "FSIZ" (size). */
public lw_label_self()
{
    new dirty;
    new font_size;
    new text[64];
    for (; ; )
    {
        new msg;
        new number;
        new str[64];
        while (RecvMessage(msg, number))
        {
            if (msg == TAG("FSIZ"))
            {
                font_size = number;
            }
        }
        while (RecvMessageStr(msg, str, 64))
        {
            if (msg == TAG("UTF8"))
            {
                text = str;
                dirty = 1;
            }
        }
        if (dirty)
        {
            imgDecStringStart(text, font_size, 64, 64, 0);
            if (wait_image_decode() == IMG_DONE)
            {
                SetTexture(Self(), imgTexID(), 0, -1);
                SetLocalScale(Self(), float(imgTexWidth()), float(imgTexHeight()), 1.0);
                SetColor(Self(), 1.0, 1.0, 1.0, 1.0);
            }
            dirty = 0;
        }
        sleep;
    }
}

/* Returns the percentage installed, the trailer weighing about 40 times its size; -1.0 before the sizes are known. */
Float:get_install_progress(size, done_size, trailer, trailer_done)
{
    new Float:weighted = size + 40.069027 * trailer;
    new Float:weighted_done = done_size + 40.069027 * trailer_done;
    if (weighted > 0)
    {
        return 100.0 * weighted_done / weighted;
    }
    return -1.0;
}

/* Returns an estimate of the seconds left to install. */
get_install_time_left(size, done_size, trailer, trailer_done)
{
    return floatround(0.0000015044585 * (size - done_size) + 0.00006028219 * (trailer - trailer_done), 0);
}
