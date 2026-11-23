/*
 * /opt0/bn/script/game/boot_game3.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * Starts the game installed in the partition of argument "uri": shows its notice, if it has one, and asks to play,
 * then reboots into it or into its disc. Returns a negative value if it cannot.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

#include "gamedb.inc"

#include "wait.inc"

#include "tag.inc"

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

#include "focus.inc"

#include "misc.inc"

#include "dom.inc"

#include "sysconf.inc"

new g_dialog_args[128];
new g_msg[128];
/* Lines of TEXT elements joined into g_msg; g_line2 and g_line3 are also the answers of a notice. */
new g_line1[32];
new g_line2[32];
new g_line3[32];
new g_line4[32];
new g_line5[32];

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
    new plain[4][32];
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
            get_tag_attr("TEXT", "msggame_exechdd_disc", "value", g_line1, 32);
            get_tag_attr("TEXT", "msggame_execdisc_insertdisc", "value", g_line2, 32);
            sprintf(g_msg, "%s\n%s", g_line1, g_line2);
            show_message_nofocus(g_msg);
            return -1;
        }
        case CDVD_DETECTING:
        {
            get_tag_attr("TEXT", "msggame_detecting_disc1", "value", g_line1, 32);
            get_tag_attr("TEXT", "msggame_detecting_disc2", "value", g_line2, 32);
            get_tag_attr("TEXT", "msggame_detecting_disc3", "value", g_line3, 32);
            sprintf(g_msg, "%s\n%s\n%s", g_line1, g_line2, g_line3);
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
            get_tag_attr("TEXT", "msggame_execdisc_wrongdisc", "value", g_line1, 32);
            sprintf(g_msg, "%s", g_line1);
            show_message_nofocus(g_msg);
            return -1;
        }
    }
    disc_type = get_cdvd_type();
    switch (disc_type)
    {
        case CDVD_NO_DISC:
        {
            get_tag_attr("TEXT", "msggame_exechdd_disc", "value", g_line1, 32);
            get_tag_attr("TEXT", "msggame_execdisc_insertdisc", "value", g_line2, 32);
            sprintf(g_msg, "%s\n%s", g_line1, g_line2);
            show_message_nofocus(g_msg);
            return -1;
        }
        case CDVD_DETECTING:
        {
            get_tag_attr("TEXT", "msggame_detecting_disc1", "value", g_line1, 32);
            get_tag_attr("TEXT", "msggame_detecting_disc2", "value", g_line2, 32);
            get_tag_attr("TEXT", "msggame_detecting_disc3", "value", g_line3, 32);
            sprintf(g_msg, "%s\n%s\n%s", g_line1, g_line2, g_line3);
            show_message_nofocus(g_msg);
            return -1;
        }
        case CDVD_PS2_CD:
        {
            if (result < 0)
            {
                if (0 != err10034())
                {
                    get_tag_attr("TEXT", "msggame_execdisc_wrongdisc", "value", g_line1, 32);
                    sprintf(g_msg, "%s", g_line1);
                    show_message_nofocus(g_msg);
                    return -1;
                }
                get_tag_attr("TEXT", "msggame_execdisc_loadingfail", "value", g_line1, 32);
                sprintf(g_msg, "%s", g_line1);
                show_message_nofocus(g_msg);
                return -1;
            }
        }
        case CDVD_PS2_CDDA:
        {
            if (result < 0)
            {
                get_tag_attr("TEXT", "msggame_execdisc_loadingfail", "value", g_line1, 32);
                sprintf(g_msg, "%s", g_line1);
                show_message_nofocus(g_msg);
                return -1;
            }
        }
        case CDVD_PS2_DVD:
        {
            if (result < 0)
            {
                get_tag_attr("TEXT", "msggame_execdisc_loadingfail", "value", g_line1, 32);
                sprintf(g_msg, "%s", g_line1);
                show_message_nofocus(g_msg);
                return -1;
            }
        }
        default:
        {
            get_tag_attr("TEXT", "msggame_execdisc_wrongdisc", "value", g_line1, 32);
            sprintf(g_msg, "%s", g_line1);
            show_message_nofocus(g_msg);
            return -1;
        }
    }
    result = err10025(plain[2], boot);
    if (result < 0)
    {
        get_tag_attr("TEXT", "msggame_execdisc_wrongdisc", "value", g_line1, 32);
        sprintf(g_msg, "%s", g_line1);
        show_message_nofocus(g_msg);
        return -4;
    }
    SetSystemParam(SYS_PADHELP, 0);
    get_tag_attr("TEXT", "msggame_warn_wait", "value", g_line1, 32);
    get_tag_attr("TEXT", "msggame_warn_wait2", "value", g_line2, 32);
    get_tag_attr("TEXT", "msggame_warn_console_power", "value", g_line3, 32);
    get_tag_attr("TEXT", "msggame_warn_console_disctray", "value", g_line4, 32);
    sprintf(g_msg, "%s%s\n\n%s\n%s", g_line1, g_line2, g_line3, g_line4);
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
    new plain[3][32];
    new encoded[3][64];
    new buffer[64];
    new len;
    new i;
    new type;
    type = err10026(partition, plain[2], buffer);
    if (type < 0)
    {
        if (type == BOOT2_NOT_FOUND)
        {
            get_tag_attr("TEXT", "msggame_navimenu_game_notfound", "value", g_line1, 32);
            sprintf(g_msg, "%s", g_line1);
        }
        else
        {
            get_tag_attr("TEXT", "msggame_execdisc_dataerr1", "value", g_line1, 32);
            get_tag_attr("TEXT", "msggame_execdisc_dataerr2", "value", g_line2, 32);
            sprintf(g_msg, "%s\n%s", g_line1, g_line2);
        }
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
            get_tag_attr("TEXT", "msggame_execdisc_nostart", "value", g_line1, 32);
            sprintf(g_msg, "%s", g_line1);
            show_message_nofocus(g_msg);
            return -2;
        }
        default:
        {
            return -3;
        }
    }
    SetSystemParam(SYS_PADHELP, 0);
    get_tag_attr("TEXT", "msggame_warn_wait", "value", g_line1, 32);
    get_tag_attr("TEXT", "msggame_warn_wait2", "value", g_line5, 32);
    get_tag_attr("TEXT", "msggame_warn_console_power", "value", g_line2, 32);
    sprintf(g_msg, "%s%s\n\n%s", g_line1, g_line5, g_line2);
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
        get_tag_attr("TEXT", "msggame_notice_play", "value", g_line2, 32);
        get_tag_attr("TEXT", "msggame_notice_notplay", "value", g_line3, 32);
        result = ask_yes_no_notice(notice, g_line2, g_line3, focus);
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

new g_uri[128];
new g_focus;

main()
{
    new result = -255;
    g_focus = create_focus_plane2("focus");
    SetOptionI(g_focus, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_focus, TAG("ZOFS"), -1.0);
    g_uri = "";
    GetArg("uri", g_uri, 128);
    if (strlen(g_uri) > 0)
    {
        result = boot_downloaded_game(g_uri, g_focus);
    }
    Destroy(Scene(0));
    return result;
}
