/*
 * /opt0/bn/script/utility/hddclear.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The erasure of the hard disk (maintenance): after a notice and two confirmations, makes the disk filler the boot
 * program of the hard disk and reboots.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

#include "tag.inc"

#include "xmlsync.inc"

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

#include "disc.inc"

new g_common_bg_name[] = "SCEI:CommonBG";

/* Loads the page of "LINK" link_id as the common background, unless the background window already shows it. */
load_common_bg(link_id[], Float:fade_time)
{
    {
        new name[256];
        if (wndGetName(WND_BG, name, 256) && 0 == strcmp(name, g_common_bg_name))
        {
            return;
        }
    }
    new url[256];
    get_tag_attr("LINK", link_id, "href", url, 256);
    xmlLoadStart(url, g_common_bg_name, _, XMLLOAD_BG);
    wait_xml_load();
    fade_in_background(fade_time);
}

#include "background.inc"

#include "bgmode.inc"

#include "focus.inc"

#include "misc.inc"

#include "dom.inc"

new g_focus;
new g_msg[128]; /* the message of the dialogs, from g_line1 and g_line2 */
new g_line1[64];
new g_line2[64];
new g_yes_label[16];
new g_no_label[16];

/* Asks msg with the buttons yes and no, select first; returns 0 for yes, 1 for no, -1 if cancelled. */
ask_yes_no(msg[], yes[], no[], select)
{
    new args[128];
    sprintf(args, "msg=%s;yes=%s;no=%s", msg, yes, no);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
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
    return answer;
}

/* Shows msg with the button ok; returns 0 for ok, -1 if cancelled. */
show_ok(msg[], ok[])
{
    new args[128];
    sprintf(args, "msg=%s;ok=%s", msg, ok);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    AttachScript(dialog, _, "confirm_ok", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", g_focus);
    SetPubVar(dialog, "padhelp", PADHELP_CROSS);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    new answer = GetPubVar(dialog, "select");
    Destroy(dialog);
    return answer;
}

/* Shows msg in a dialog and waits until it is closed. */
show_message(msg[])
{
    new args[128];
    sprintf(args, "msg=%s", msg);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    AttachScript(dialog, _, "confirm_d", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", g_focus);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    Destroy(dialog);
    return;
}

/* Shows msg until its dialog closes by itself. */
show_moment(msg[], life)
{
    new args[128];
    sprintf(args, "msg=%s", msg);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    AttachScript(dialog, _, "confirm_moment", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", g_focus);
    SetPubVar(dialog, "life", life);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
}

/*
 * Reboots into the browser: download slot 0 gets the new boot DES keys, slot 1 the encrypted
 * "BootBrowser" (both in Base64, the second also in ROT13).
 */
reboot_to_browser()
{
    new plain[2][32];
    new encoded[2][64];
    new len;
    new cipher[64];
    new i;
    err10019(plain[0]);
    encode_base64(plain[0], 16, encoded[0], 64);
    err10020();
    new count = 2;
    plain[1] = "BootBrowser";
    i = 1;
    for (; i < count; i++)
    {
        len = err10021(plain[i], cipher);
        encode_base64(cipher, len, encoded[i], 64);
        apply_rot13(encoded[i]);
    }
    err10022(0, encoded[0]);
    err10022(1, encoded[1]);
    err10023(count);
    return;
}

#include "fileop.inc"

/*
 * Copies the disk filler over the boot program of the hard disk; returns 0, -1 if it is missing or the copy does
 * not start, -2 or -3 if the copy fails.
 */
install_hdfill()
{
    new status;
    new error;
    new mode;
    new size;
    if (0 != fileLstat("pfs:/__system/hdfill/hdfill.elf", mode, size))
    {
        return -1;
    }
    if (0 != fileLstat("pfs:/__system/hdfill/HDFILLER.PAK", mode, size))
    {
        return -1;
    }
    status = fileCopy("pfs:/__system/hdfill/hdfill.elf", "pfs:/__system/p2lboot/osdboot.elf");
    if (status < 0)
    {
        return -1;
    }
    if (wait_file_op(status, error, 0, 0, 100) < 0)
    {
        if (status < 0)
        {
            return -2;
        }
        if (error == 2)
        {
            return -3;
        }
    }
    return 0;
}

main()
{
    new answer;
    start_top_page(0);
    load_common_bg("bg", 1000.0);
    set_background_poor(600.0, 0);
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
    SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
    SetPos(Camera(0), 0.0, 0.0, 300.0, 0);
    LookAt(Camera(0), Scene(0), 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
    g_focus = create_focus_plane2("focus");
    SetOptionI(g_focus, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_focus, TAG("ZOFS"), -1.0);
    SendMessage(g_focus, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
    get_tag_attr("TEXT", "msgutil_maintanance_hdd_erase_start_1", "value", g_line1, 64);
    get_tag_attr("TEXT", "msgutil_maintanance_hdd_erase_start_2", "value", g_line2, 64);
    get_tag_attr("TEXT", "msgutil_common_start", "value", g_yes_label, 16);
    sprintf(g_msg, "%s\n%s", g_line1, g_line2);
    answer = show_ok(g_msg, g_yes_label);
    if (0 != answer)
    {
        goto done;
    }
    for (; ; )
    {
        get_tag_attr("TEXT", "msgutil_maintanance_hdd_erase_notice_1", "value", g_line1, 64);
        get_tag_attr("TEXT", "msgutil_maintanance_hdd_erase_notice_2", "value", g_line2, 64);
        get_tag_attr("TEXT", "msgutil_common_yes", "value", g_yes_label, 16);
        get_tag_attr("TEXT", "msgutil_common_no", "value", g_no_label, 16);
        sprintf(g_msg, "%s\n\n%s", g_line1, g_line2);
        answer = ask_yes_no(g_msg, g_yes_label, g_no_label, 1);
        if (0 == answer)
        {
            break;
        }
        else
        {
            get_tag_attr("TEXT", "msgutil_common_cancel_ask", "value", g_msg, 128);
            get_tag_attr("TEXT", "msgutil_common_yes", "value", g_yes_label, 16);
            get_tag_attr("TEXT", "msgutil_common_no", "value", g_no_label, 16);
            answer = ask_yes_no(g_msg, g_yes_label, g_no_label, 1);
            if (0 == answer)
            {
                goto done;
            }
        }
    }
    for (; ; )
    {
        get_tag_attr("TEXT", "msgutil_maintanance_hdd_erase_ask", "value", g_line1, 64);
        sprintf(g_msg, "%s", g_line1);
        answer = ask_yes_no(g_msg, g_yes_label, g_no_label, 1);
        if (0 == answer)
        {
            break;
        }
        else
        {
            get_tag_attr("TEXT", "msgutil_common_cancel_ask", "value", g_msg, 128);
            get_tag_attr("TEXT", "msgutil_common_yes", "value", g_yes_label, 16);
            get_tag_attr("TEXT", "msgutil_common_no", "value", g_no_label, 16);
            answer = ask_yes_no(g_msg, g_yes_label, g_no_label, 1);
            if (0 == answer)
            {
                goto done;
            }
        }
    }
    if (0 == install_hdfill())
    {
        SetSystemParam(SYS_PADHELP, 0);
        get_tag_attr("TEXT", "msgutil_maintanance_hdd_erase_wait1", "value", g_line1, 64);
        get_tag_attr("TEXT", "msgutil_maintanance_hdd_erase_wait2", "value", g_line2, 64);
        sprintf(g_msg, "%s\n\n%s", g_line1, g_line2);
        show_moment(g_msg, 5000);
        reboot_to_browser();
        goto done;
    }
    get_tag_attr("TEXT", "msgutil_maintanance_hdd_erase_fail_1", "value", g_line1, 64);
    get_tag_attr("TEXT", "msgutil_maintanance_hdd_erase_fail_2", "value", g_line2, 64);
    sprintf(g_msg, "%s\n%s", g_line1, g_line2);
    show_message(g_msg);
    done:
    SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
    fade_background(0, 0, 600.0);
    new url[64];
    get_tag_attr("LINK", "utility_top", "href", url, 64);
    xmlLoadStart(url, "", _, XMLLOAD_MAIN);
    wait_xml_load();
    Destroy(Self());
    return;
}
