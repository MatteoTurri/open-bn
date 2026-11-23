/*
 * /opt0/bn/script/utility/dbconvert.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The conversion of the databases (dbconvert library) with a progress dialog; when it is done the initial setup
 * (init_setup) goes on, a failure leaves its message on the screen for good.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 2560

/* The status of dbconvGetStatus. */
const CONV_RUNNING = 0;
const CONV_DONE = 2;
const CONV_TOO_MANY_RECORDS = -2; /* a database has over 100000 records */

/* The natives of the dbconvert library (dbconvert.so): the conversion of the databases. */
#pragma library "dbconvert"
native dbconvStart();
/*
 * database is the number of the database being converted, from 1; tables the number of tables of all the
 * databases; table_percent the part of the current table already converted. status is CONV_RUNNING while the
 * conversion runs, then its result (CONV_DONE, CONV_TOO_MANY_RECORDS or another negative value: failed).
 */
native dbconvGetStatus(&database, &tables, &table_percent, &status);
native dbconvDestroy();

#include "wait.inc"

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

#include "misc.inc"

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
    xmlLoadStart(url, g_common_bg_name, "", XMLLOAD_BG);
    wait_xml_load();
    fade_in_background(fade_time);
}

#include "background.inc"

#include "bgmode.inc"

#include "focus.inc"

/* Same as open_dialog. */
open_dialog3(args[])
{
    new screen_w;
    new screen_h;
    new screen_d;
    wndGetScreen(0, screen_w, screen_h, screen_d);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 0.5 * float(screen_w), 0.5 * float(screen_h), 0.0, 0);
    AttachScript(dialog, _, "dialog", args);
    SetPubVar(dialog, "manager", Self());
    return dialog;
}

#include "pfs.inc"

new g_confirm_format[] = "msg=%s;type=confirm;btn0=%s";

/* Shows msg with one button and waits until it is closed; returns 1 if it was cancelled. */
show_confirm(focus, msg[], button[], cancelable)
{
    new args[128];
    sprintf(args, g_confirm_format, msg, button);
    new dialog = open_dialog3(args);
    SetPubVar(dialog, "focus", focus);
    SetPubVar(dialog, "padhelp", cancelable ? PADHELP_CROSS : 0);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    new canceled = GetPubVar(dialog, "canceled");
    Destroy(dialog);
    return canceled;
}

/* Opens a dialog with msg and returns it once it is open. */
open_message(focus, msg[])
{
    new args[128];
    sprintf(args, "msg=%s", msg);
    new dialog = open_dialog3(args);
    SetPubVar(dialog, "focus", focus);
    while (!GetPubVar(dialog, "ready"))
    {
        sleep;
    }
    return dialog;
}

/* Opens the dialog described by the "dialog" tag id, with the button select first. */
open_id_dialog(focus, id[], cancelable, select)
{
    new args[16];
    sprintf(args, "id=%s", id);
    new dialog = open_dialog3(args);
    SetPubVar(dialog, "focus", focus);
    SetPubVar(dialog, "select", select);
    SetPubVar(dialog, "padhelp", cancelable ? PADHELP_CROSS : 0);
    return dialog;
}

main()
{
    start_top_page(0);
    load_common_bg("bg", 1000.0);
    set_background_poor(600.0, 0);
    SetPos(Camera(0), 0.0, 0.0, 300.0, 0);
    LookAt(Camera(0), Scene(0), 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
    new Float:screen_w;
    new Float:screen_h;
    get_screen_size2(screen_w, screen_h);
    new focus = create_focus_plane2("focus");
    SetOptionI(focus, TAG("ZSRT"), 1); /* acts as ZSORT_NONE */
    SetOptionF(focus, TAG("ZOFS"), -1.0);
    new window;
    window = CreatePlane(Disp2D(0));
    SetPos(window, 0.671875 * screen_w, 0.5 * screen_h, 0.0, 0);
    SetColor(window, 1.0, 1.0, 1.0, 0.0);
    SetLocalScale(window, 380.0, 380.0, 1.0);
    SetOptionI(window, TAG("ZSRT"), ZSORT_BACKGROUND);
    SetOptionI(window, TAG("ALPH"), BLEND_ADD);
    AttachScript(window, _, "window");
    new msg[64];
    new dialog = open_id_dialog(focus, "dbconvert_progress", 0, 0);
    wait_ms(500);
    SystemCommand(SYSCMD_BUSY, 1);
    dbconvStart();
    new database;
    new tables;
    new table_percent;
    new status;
    new Float:progress;
    for (; ; )
    {
        dbconvGetStatus(database, tables, table_percent, status);
        progress = 100.0 * float(database) / float(tables) + float(table_percent);
        /* NOTE: bug: progress is already a Float, which float() takes for an integer */
        SetPubVar(dialog, "progress", float(progress));
        if (CONV_RUNNING != status)
        {
            break;
        }
        sleep;
    }
    SetPubVar(dialog, "progress", 100.0);
    wait_ms(500);
    close_dialog(dialog);
    SystemCommand(SYSCMD_BUSY, 0);
    if (status == CONV_DONE)
    {
        new ok_label[32];
        get_tag_attr("TEXT", "msgutil_db_convert_complete", "value", msg, 64);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok_label, 32);
        show_confirm(focus, msg, ok_label, 0);
    }
    else if (status == CONV_TOO_MANY_RECORDS)
    {
        SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
        new label[32]; /* unused, but part of the layout */
        get_tag_attr("TEXT", "msgutil_db_convert_warn_fail_max", "value", msg, 64);
        open_message(focus, msg);
        while (1)
        {
            sleep;
        }
    }
    else if (status < 0)
    {
        SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
        get_tag_attr("TEXT", "msgutil_db_convert_warn_fail", "value", msg, 64);
        open_message(focus, msg);
        while (1)
        {
            sleep;
        }
    }
    dbconvDestroy();
    fade_background(0, 0, 600.0);
    {
        new url[64];
        get_tag_attr("LINK", "init_setup", "href", url, 64);
        xmlLoadStart(url, _, "mode=DBConvert", XMLLOAD_MAIN);
        wait_xml_load();
    }
    Destroy(Self());
}

#include "string.inc"
