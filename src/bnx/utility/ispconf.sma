/*
 * /opt0/bn/script/utility/ispconf.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The installation of the default ISP settings from the hard disk, after the network settings: a netconf_page
 * page with its progress, the error if it fails and a last page; then the page it came from.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "ispconf.inc"

/* The pages, "page_<n>" in the XML. */
const PAGE_NONE = -1;
const PAGE_INSTALL = 0; /* installs the settings, with its progress */
const PAGE_DONE = 1;

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

#include "imgdec.inc"

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

#include "pfs.inc"

/* Shows msg in a red dialog and waits until it is closed. */
show_error_dialog(focus, msg[])
{
    new args[128];
    sprintf(args, "msg=%s", msg);
    new dialog = open_dialog2(args);
    SetPubVar(dialog, "focus", focus);
    SetPubVar(dialog, "padhelp", PADHELP_CROSS);
    SetPubVar(dialog, "red", 0.549);
    SetPubVar(dialog, "green", 0.0784);
    SetPubVar(dialog, "blue", 0.1961);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    Destroy(dialog);
}

/* Shows msg in a dialog and waits until it is closed. */
show_message_dialog(focus, msg[])
{
    new args[128];
    sprintf(args, "msg=%s", msg);
    new dialog = open_dialog2(args);
    SetPubVar(dialog, "focus", focus);
    SetPubVar(dialog, "padhelp", PADHELP_CROSS);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    Destroy(dialog);
}

new g_focus;
new g_canceled; /* the installation was stopped before its end */
new g_status; /* of the installation: its progress, then 0 or an error */
new Float:g_center_x;
new Float:g_center_y;
new g_page_tex;
new g_cancel_shown; /* the cross of the pad help reads "cancel" */
new g_saved_cross_label[16];

/* Makes the cross of the pad help read "cancel" while the installation runs, or restores it. */
show_cancel_help(show)
{
    if (g_cancel_shown != show)
    {
        if (show)
        {
            new label[8];
            get_tag_attr("TEXT", "msgutil_common_cancel", "value", label, 8);
            GetSystemParamStr(SYS_STR_PADHELP_RD, g_saved_cross_label, 16);
            SetSystemParamStr(SYS_STR_PADHELP_RD, label);
        }
        else
        {
            SetSystemParamStr(SYS_STR_PADHELP_RD, g_saved_cross_label);
        }
        g_cancel_shown = show;
    }
}

#include "fileop.inc"

/*
 * Starts the operation of a page and slides it in (dir as for get_slide_offset); returns the page object, 0 for
 * PAGE_NONE.
 */
open_page(page, dir)
{
    switch (page)
    {
        case PAGE_INSTALL:
        {
            SystemCommand(SYSCMD_BUSY, 1);
            ispConfInstall("pfs:__contents/bn.conf/default_isp.dat");
            show_cancel_help(1);
        }
        default:
        {
        }
    }
    new obj;
    if (page >= 0)
    {
        obj = CreateNull(Disp2D(0));
        SetPos(obj, g_center_x - get_slide_offset(dir), g_center_y, 0.0, 0);
        new args[16];
        sprintf(args, "xml_item = page_%d", page);
        AttachScript(obj, _, "netconf_page", args);
        SetPubVar(obj, "manager", Self());
        SetPubVar(obj, "focus", g_focus);
        SetPubVar(obj, "texid", g_page_tex);
        AnimMoveLine(obj, g_center_x, g_center_y, 0.0, 200.0, 0.0, ANIM_FUNC_EXPONENT);
        sndSePlay(SE_DIALOG_OPEN);
    }
    else
    {
        obj = 0;
    }
    return obj;
}

/* Slides a page out and fades it. */
close_page(obj, dir)
{
    if (obj)
    {
        AnimMoveLine(obj, g_center_x + get_slide_offset(dir), g_center_y, 0.0, 200.0, 0.0, ANIM_FUNC_EXPONENT);
        SendMessage(obj, TAG("FADE"), 0, 200.0, 0, 0);
        wait_ms(250);
        show_cancel_help(0);
    }
}

/* Follows the operation of a page; returns 1 when it has ended. */
poll_page(obj, page)
{
    new done;
    switch (page)
    {
        case PAGE_INSTALL:
        {
            if (ispConfSync(g_status))
            {
                SendMessage(obj, TAG("PROG"), g_status, 0, 0, 0);
            }
            else
            {
                SystemCommand(SYSCMD_BUSY, 0);
                done = 1;
            }
        }
        default:
        {
            done = 0;
        }
    }
    return done;
}

/*
 * Shows the page and waits until it is left; returns 1 if it is left with the circle, 0 with the cross or when
 * its operation ends.
 */
run_pages(page)
{
    new current = PAGE_NONE;
    new obj;
    new dir = 3; /* how the pages slide: get_slide_offset */
    new canceled;
    new ok;
    for (; ; )
    {
        sleep;
        if (current != page)
        {
            close_page(obj, dir);
            if (canceled)
            {
                current = PAGE_NONE;
                canceled = 0;
            }
            else
            {
                current = page;
            }
            if (!(obj = open_page(current, dir)))
            {
                break;
            }
        }
        if (poll_page(obj, current))
        {
            page = PAGE_NONE;
            continue;
        }
        new msg;
        new a;
        new b;
        /* From the page: "FOCS" is given back at once; "PUSH" (index, button): circle or cross ends the pages. */
        if (RecvMessage(msg, a, b))
        {
            if (msg == TAG("FOCS"))
            {
                SendMessage(obj, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
            }
            else if (msg == TAG("PUSH"))
            {
                if (b == TAG("RRGT"))
                {
                    page = PAGE_NONE;
                    dir = 3;
                    ok = 1;
                }
                else if (b == TAG("RDWN"))
                {
                    page = PAGE_NONE;
                    dir = 3;
                    sndSePlay(SE_CANCEL);
                    canceled = 1;
                }
                SendMessage(obj, TAG("PUSH"), 0, 0, 0, 0);
            }
        }
    }
    return ok;
}

main()
{
    start_top_page(0);
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
    SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
    new first_setup; /* no "resume": the first setup of the network */
    new from_update; /* "resume=update": back to the system update afterwards */
    new url[64];
    {
        new screen_w;
        new screen_h;
        new screen_d;
        wndGetScreen(0, screen_w, screen_h, screen_d);
        g_center_x = 0.5 * float(screen_w);
        g_center_y = 0.5 * float(screen_h);
    }
    load_common_bg("bg", 1000.0);
    set_background_poor(600.0, 0);
    {
        new resume[8];
        if (GetArg("resume", resume, 8))
        {
            if (!strcmp(resume, "update"))
            {
                from_update = 1;
            }
        }
        else
        {
            first_setup = 1;
        }
    }
    {
        new tex_url[64];
        get_tag_attr("IMG", "page", "src", tex_url, 64);
        start_image_decode2(tex_url);
        finish_image_decode();
        g_page_tex = imgTexID();
        imgKeepTex(g_page_tex);
    }
    SetPos(Camera(0), 0.0, 0.0, 300.0, 0);
    LookAt(Camera(0), Scene(0), 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
    g_focus = create_focus_plane2("focus");
    SetOptionI(g_focus, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_focus, TAG("ZOFS"), -1.0);
    if (!run_pages(PAGE_INSTALL))
    {
        new cancel_wait;
        while (ispConfSync(g_status))
        {
            if (cancel_wait-- <= 0)
            {
                g_canceled = 1;
                ispConfCancel();
                cancel_wait = 30;
            }
            sleep;
        }
        SystemCommand(SYSCMD_BUSY, 0);
    }
    if (0 != g_status)
    {
        if (g_canceled)
        {
            g_status = ISPCONF_ABORTED;
        }
        show_install_error(g_status);
    }
    else if (!first_setup && !from_update)
    {
        run_pages(PAGE_DONE);
    }
    kill_object(g_focus);
    imgReleaseTex(g_page_tex);
    SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
    fade_background(0, 0, 600.0);
    if (first_setup)
    {
        get_tag_attr("LINK", "Init", "href", url, 64);
        xmlLoadStart(url, _, "mode=GetIspChannel", XMLLOAD_MAIN);
    }
    else if (0 != g_status || !from_update)
    {
        get_tag_attr("LINK", "Back", "href", url, 64);
        xmlLoadStart(url, _, _, XMLLOAD_MAIN);
    }
    else
    {
        get_tag_attr("LINK", "Update", "href", url, 64);
        xmlLoadStart(url, _, "mode=Retry", XMLLOAD_MAIN);
    }
    wait_xml_load();
    Destroy(Self());
}

/* Shows the error of the installation with its kind and code; the errors of the DNAS information center in red. */
show_install_error(code)
{
    new dnas;
    new msg[64];
    if (code == ISPCONF_DNAS_ABORTED || code == ISPCONF_ABORTED)
    {
        get_tag_attr("TEXT", "msgutil_netconf_warn_provider_infocancel", "value", msg, 64);
    }
    else
    {
        new kind[16];
        if (-20500 < code <= -20400 || code == -20880 || code == -20881)
        {
            get_tag_attr("TEXT", "msgutil_netconf_warn_dnas_infocenter", "value", msg, 64);
            get_tag_attr("TEXT", "msgutil_netconf_warn_dnas", "value", kind, 16);
            dnas = 1;
        }
        else if (-20700 < code <= -20000)
        {
            get_tag_attr("TEXT", "msgutil_netconf_warn_dnas_authen", "value", msg, 64);
            get_tag_attr("TEXT", "msgutil_netconf_warn_http", "value", kind, 16);
        }
        else if (-25000 < code <= -20700)
        {
            get_tag_attr("TEXT", "msgutil_netconf_warn_dnas_authen", "value", msg, 64);
            get_tag_attr("TEXT", "msgutil_netconf_warn_internal", "value", kind, 16);
        }
        else
        {
            get_tag_attr("TEXT", "msgutil_netconf_warn_provider", "value", msg, 64);
            if (code == 401)
            {
                get_tag_attr("TEXT", "msgutil_netconf_warn_authen", "value", kind, 16);
            }
            else if (code >= 0)
            {
                get_tag_attr("TEXT", "msgutil_netconf_warn_http", "value", kind, 16);
            }
            else if (-1000 < code < 0)
            {
                get_tag_attr("TEXT", "msgutil_netconf_warn_system", "value", kind, 16);
            }
            else if (-50000 < code <= -40000)
            {
                get_tag_attr("TEXT", "msgutil_netconf_warn_download", "value", kind, 16);
            }
            else
            {
                get_tag_attr("TEXT", "msgutil_netconf_warn_internal", "value", kind, 16);
            }
        }
        if (-1 != code)
        {
            append_string(msg, "\n(", 64);
            append_string(msg, kind, 64);
            sprintf(kind, " %d)", code);
            append_string(msg, kind, 64);
        }
    }
    if (dnas)
    {
        show_error_dialog(g_focus, msg);
    }
    else
    {
        show_message_dialog(g_focus, msg);
    }
}
