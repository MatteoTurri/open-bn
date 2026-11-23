/*
 * /opt0/bn/script/utility/sysupdate.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The system update ("mode": Init from the initial setup, FromTop, Retry after the ISP settings): checks the
 * version on the server, then downloads and installs the update of the system (after its license agreement) or
 * the packages of the top menu, and records the outcome in the aval UpdateErrorFlag.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

#include "wait.inc"

#include "tag.inc"

/* The pages that start the update (g_mode). */
const MODE_MENU = 0; /* the utility menu */
const MODE_INIT = 1; /* the initial setup */
const MODE_FROM_TOP = 2;
const MODE_RETRY = 3; /* the ISP settings, after an update */

/* The results of a download: the HTTP status, or an error of the HTTP library. */
const HTTP_OK = 200;
const HTTP_ERR_WRITE = -1014; /* the file could not be written */

/* The errors of update_top_menu and update_system, each with its result code. */
const ERR_DOWNLOAD = -1;
const ERR_CANCELED = -2;
const ERR_UPDATE = -3;
const ERR_WORK_DIR = -4; /* sysUpdateMkBntmp failed */
const ERR_LICENSE = -5; /* the license agreement was not accepted */

/* The states of sysUpdateGetStat that end the update. */
const UPDATE_DONE = 3;
const UPDATE_FAILED = 4;

/* Returns whether str is a packed string. */
is_packed(str[])
{
    return str[0] <= 0 || str[0] > 255;
}

/* Copies the packed or unpacked string src into dest, at most maxlen cells. */
copy_string(dest[], src[], maxlen)
{
    new len = strlen(src) + 1;
    if (is_packed(src))
    {
        len = (len + 3) / 4;
    }
    len = min(len, maxlen);
    for (new i; i < len; i++)
    {
        dest[i] = src[i];
    }
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

#include "pfs.inc"

/* Opens the dialog described by the "dialog" tag id, with the button select first. */
open_id_dialog(focus, id[], cancelable, select)
{
    new args[16];
    sprintf(args, "id=%s", id);
    new dialog = open_dialog2(args);
    SetPubVar(dialog, "focus", focus);
    SetPubVar(dialog, "select", select);
    SetPubVar(dialog, "padhelp", cancelable ? PADHELP_CROSS : 0);
    return dialog;
}

/* Shows the dialog id and waits for the answer: the button selected, -1 if cancelled. */
ask_id_dialog(focus, id[], cancelable, select)
{
    new dialog = open_id_dialog(focus, id, cancelable, select);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    select = GetPubVar(dialog, "canceled") ? -1 : GetPubVar(dialog, "select");
    Destroy(dialog);
    return select;
}

/* Opens the dialog with the text of the TEXT element msg_id and waits until it is ready. */
open_message_dialog(focus, msg_id[], pad_help)
{
    new args[128];
    {
        new msg[125];
        get_tag_attr("TEXT", msg_id, "value", msg, 125);
        sprintf(args, "msg=%s", msg);
    }
    new dialog = open_dialog2(args);
    SetPubVar(dialog, "focus", focus);
    SetPubVar(dialog, "padhelp", pad_help);
    while (!GetPubVar(dialog, "ready"))
    {
        sleep;
    }
    return dialog;
}

new g_focus;
new g_eura[2048]; /* the text of the license agreement */
new g_user[256]; /* of the update server */
new g_password[256];
new g_url[256]; /* of the server, then of the version list, then of the update ("" for none) */
new g_result[16]; /* the outcome for UpdateErrorFlag */
new g_mode;
new g_tm_updated; /* the packages of the top menu have been installed */
/* Where the packages of the top menu go: the scripts, and the settings (with the ISP settings). */
new g_tm_dirs[2][32] = {"/opt0/bn/script/", "pfs:__contents/bn.conf/"};

/* Shows the text of the TEXT element msg_id in a dialog and waits until it is closed. */
show_message_dialog(focus, msg_id[], pad_help)
{
    new dialog = open_message_dialog(focus, msg_id, pad_help);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    Destroy(dialog);
}

#include "misc.inc"

#include "dom.inc"

/* Returns how many packages of the top menu the server has. */
count_tm_packages()
{
    new count;
    for (new i; i < 2; i++)
    {
        new url[64];
        if (!sysUpdateTMUrl(i, url))
        {
            count++;
        }
    }
    return count;
}

/* Returns 1 if the packages installed include the ISP settings. */
has_isp_package()
{
    if (g_tm_updated)
    {
        new url[64];
        return 0 == sysUpdateTMUrl(1, url);
    }
    return 0;
}

/* Downloads and installs the packages of the top menu; returns 0, ERR_CANCELED or ERR_UPDATE. */
update_top_menu()
{
    new status;
    new canceled;
    new dialog = open_id_dialog(g_focus, "sysupdate_topmenu", 1, 0);
    wait_ms(500);
    new total = count_tm_packages();
    new done;
    for (new i; i < 2; i++)
    {
        new url[64];
        if (!sysUpdateTMUrl(i, url))
        {
            if (canceled)
            {
                status = 0;
                break;
            }
            new path[64];
            sysUpdateTMPath(path, g_tm_dirs[i], url);
            if ((status = sysUpdateTMStart(url, path, 0)) >= 0)
            {
                while (sysUpdateTMSync(status))
                {
                    if (GetPubVar(dialog, "canceled"))
                    {
                        canceled = 1;
                        break;
                    }
                    else
                    {
                        SetPubVar(dialog, "progress", float(done * 100 + status) / float(total));
                    }
                    sleep;
                }
                sysUpdateTMStop();
            }
            if (HTTP_OK != status)
            {
                break;
            }
            done++;
        }
    }
    if (status == HTTP_OK)
    {
        sysUpdateInit();
        if (sysUpdateTMFinish())
        {
            status = -1;
        }
        else
        {
            SetPubVar(dialog, "progress", 100.0);
        }
        sysUpdateExit();
    }
    close_dialog(dialog);
    if (canceled)
    {
        show_message_dialog(g_focus, "msgutil_sysupdate_topmenu_cancel", PADHELP_CROSS);
        return ERR_CANCELED;
    }
    if (status == HTTP_OK)
    {
        g_tm_updated = 1;
        if (MODE_INIT != g_mode && !has_isp_package())
        {
            show_message_dialog(g_focus, "msgutil_sysupdate_complete", PADHELP_CROSS);
        }
    }
    else
    {
        show_message_dialog(g_focus, "msgutil_sysupdate_topmenu_fail", PADHELP_CROSS);
        return ERR_UPDATE;
    }
    return 0;
}

/* Downloads the version list and the information of the top menu; leaves g_url "" if it fails. */
get_version_info()
{
    new status;
    new canceled;
    new name[128];
    new dialog = open_id_dialog(g_focus, "sysupdate_checking", 1, 0);
    canceled = 0;
    sysUpdateInit();
    status = sysUpdateDlStart(g_user, g_password, g_url, "pfs0:/dlcontents/BN/bnversion.svr");
    if (status >= 0)
    {
        new progress;
        while (0 == (status = sysUpdateDlSync(progress)))
        {
            if (GetPubVar(dialog, "canceled"))
            {
                canceled = 1;
                sysUpdateDlStop();
            }
            else
            {
                SetPubVar(dialog, "progress", float(progress) / 2.0);
            }
            sleep;
        }
    }
    sysUpdateExit();
    name = "tmversion.dat";
    if (!canceled && status == HTTP_OK)
    {
        SetPubVar(dialog, "progress", 50.0);
        if ((status = sysUpdateTMStart(name, "/opt0/bn/sysconf/tmversion.dat", 1)) >= 0)
        {
            while (sysUpdateTMSync(status))
            {
                if (GetPubVar(dialog, "canceled"))
                {
                    canceled = 1;
                    break;
                }
                sleep;
            }
            sysUpdateTMStop();
        }
    }
    if (!canceled && status == HTTP_OK)
    {
        SetPubVar(dialog, "progress", 100.0);
    }
    close_dialog(dialog);
    if (canceled)
    {
        show_message_dialog(g_focus, "msgutil_sysupdate_check_cancel", PADHELP_CROSS);
        g_url = "";
        return;
    }
    if (HTTP_OK != status)
    {
        show_message_dialog(g_focus, "msgutil_sysupdate_error_notgetver", PADHELP_CROSS);
        g_url = "";
        return;
    }
    g_url = "pfs0:/dlcontents/BN/bnversion.svr";
}

/*
 * Copies url into dest with name in place of its last component (of the directory after "_" in a "nv!$2002"
 * url); returns 1, or 0 if url has none.
 */
replace_file_name(dest[], url[], name[], maxlen)
{
    new pos;
    copy_string(dest, url, maxlen);
    if (find_substring(dest, "nv!$2002", 0) > 0)
    {
        pos = strlen(dest) - 1;
        if (dest{pos} == '/')
        {
            dest{pos} = 0;
        }
        if ((pos = find_last_any_char(dest, "/", -1)) > 0)
        {
            if ((pos = find_any_char(dest, "_", pos)) > 0)
            {
                dest{pos + 1} = 0;
                append_string(dest, name, maxlen);
                append_string(dest, "/", maxlen);
                return 1;
            }
        }
    }
    else
    {
        if ((pos = find_last_any_char(dest, "/", -1)) > 0)
        {
            dest{pos + 1} = 0;
            append_string(dest, name, maxlen);
            return 1;
        }
    }
    return 0;
}

/*
 * Downloads the license agreement of the update (bneura.txt next to url) and asks it with the dialog dialog_id;
 * returns the answer (0 accepts), -2 if cancelled, -3 on failure.
 */
ask_eura(dialog_id[], url[])
{
    new status;
    new canceled;
    new eura_url[64];
    replace_file_name(eura_url, url, "bneura.txt", 64);
    new dialog = open_id_dialog(g_focus, "sysupdate_geteura", 1, 0);
    sysUpdateInit();
    if ((status = sysUpdateDlStart(g_user, g_password, eura_url, "pfs0:/dlcontents/BN/bneura.svr")) >= 0)
    {
        new progress;
        while (0 == (status = sysUpdateDlSync(progress)))
        {
            if (GetPubVar(dialog, "canceled"))
            {
                canceled = 1;
                sysUpdateDlStop();
            }
            else
            {
                SetPubVar(dialog, "progress", float(progress));
            }
            sleep;
        }
        if (status == HTTP_OK)
        {
            if ((status = sysUpdateGetEura("pfs0:/dlcontents/BN/bneura.svr", g_eura, 2048)) >= 0)
            {
                new i;
                while ((g_eura{i} >= '0' && g_eura{i} <= '9') || g_eura{i} == '-')
                {
                    i++;
                }
                if (0 != g_eura{i})
                {
                    status = HTTP_OK;
                }
                else
                {
                    status = 0;
                }
            }
        }
    }
    sysUpdateExit();
    if (!canceled && status == HTTP_OK)
    {
        SetPubVar(dialog, "progress", 100.0);
    }
    close_dialog(dialog);
    if (canceled)
    {
        show_message_dialog(g_focus, "msgutil_sysupdate_start_term_get_cancel", PADHELP_CROSS);
        return -2;
    }
    if (HTTP_OK != status)
    {
        show_message_dialog(g_focus, "msgutil_sysupdate_start_term_get_fail", PADHELP_CROSS);
        return -3;
    }
    new node = domFindTag(domRoot(), "dialog", dialog_id, 0);
    node = domAddTag(node, "msg");
    domSetData(node, "", g_eura);
    return ask_id_dialog(g_focus, dialog_id, 1, 1);
}

/*
 * Downloads the update url and installs it, then reboots; returns one of the ERR_* when it does not.
 */
update_system(url[])
{
    new status;
    new dialog;
    new canceled;
    if (ask_eura("sysupdate_eura", url))
    {
        return ERR_LICENSE;
    }
    dialog = open_id_dialog(g_focus, "sysupdate_download", 1, 0);
    canceled = 0;
    sysUpdateInit();
    status = sysUpdateDlStart(g_user, g_password, url, "pfs0:/dlcontents/BN/content.dat");
    if (status >= 0)
    {
        new progress;
        while (0 == (status = sysUpdateDlSync(progress)))
        {
            if (GetPubVar(dialog, "canceled"))
            {
                canceled = 1;
                sysUpdateDlStop();
            }
            else
            {
                SetPubVar(dialog, "progress", float(progress));
            }
            sleep;
        }
    }
    sysUpdateExit();
    if (!canceled && status == HTTP_OK)
    {
        SetPubVar(dialog, "progress", 100.0);
    }
    close_dialog(dialog);
    if (canceled)
    {
        show_message_dialog(g_focus, "msgutil_sysupdate_download_cancel", PADHELP_CROSS);
        return ERR_CANCELED;
    }
    if (HTTP_OK != status)
    {
        if (HTTP_ERR_WRITE != status)
        {
            show_message_dialog(g_focus, "msgutil_sysupdate_error_download", PADHELP_CROSS);
        }
        else
        {
            show_message_dialog(g_focus, "msgutil_sysupdate_fail_download", PADHELP_CROSS);
        }
        return ERR_DOWNLOAD;
    }
    dialog = open_id_dialog(g_focus, "sysupdate_updateing", 0, 0);
    wait_ms(500);
    status = sysUpdateMkBntmp();
    if (status < 0)
    {
        show_message_dialog(g_focus, "msgutil_sysupdate_fail_update", PADHELP_CROSS);
        return ERR_WORK_DIR;
    }
    sysUpdateInit();
    status = sysUpdateStart();
    if (0 == status)
    {
        new state;
        new result;
        new total;
        new done;
        new percent;
        for (; ; )
        {
            sysUpdateGetStat(state, result, total, done);
            if (state == UPDATE_DONE)
            {
                break;
            }
            if (state == UPDATE_FAILED)
            {
                break;
            }
            percent = clamp(done * 100 / total, 0, 99);
            SetPubVar(dialog, "progress", float(percent));
            sleep;
        }
        status = sysUpdateEnd();
        if (0 == status)
        {
            status = state;
        }
    }
    else
    {
        sysUpdateEnd();
    }
    sysUpdateExit();
    if (status == UPDATE_DONE)
    {
        SetPubVar(dialog, "progress", 100.0);
    }
    close_dialog(dialog);
    if (status == UPDATE_DONE)
    {
        ask_id_dialog(g_focus, "sysupdate_system_reboot", 0, 0);
        SetSystemParam(SYS_PADHELP, 0);
        open_message_dialog(g_focus, "msgutil_sysupdate_rebootwait", 0);
        reboot_to_linux();
    }
    else
    {
        show_message_dialog(g_focus, "msgutil_sysupdate_fail", PADHELP_CROSS);
        return ERR_UPDATE;
    }
    return 0;
}

/*
 * Reboots into Linux: download slot 0 gets the new boot DES keys, slot 1 the encrypted "BootLinux" (both in
 * Base64, the second also in ROT13).
 */
reboot_to_linux()
{
    new plain[2][32];
    new encoded[2][64];
    new len;
    new cipher[256];
    new i;
    err10019(plain[0]);
    encode_base64(plain[0], 16, encoded[0], 64);
    err10020();
    new count = 2;
    plain[1] = "BootLinux";
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

main()
{
    new ret;
    start_top_page(0);
    load_common_bg("bg", 1000.0);
    set_background_poor(600.0, 0);
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
    sysUpdateSetup();
    SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
    new mode[64];
    GetArg("mode", mode, 64);
    if (!strcmp(mode, "Init"))
    {
        g_mode = MODE_INIT;
    }
    else if (!strcmp(mode, "FromTop"))
    {
        g_mode = MODE_FROM_TOP;
    }
    else if (!strcmp(mode, "Retry"))
    {
        g_mode = MODE_RETRY;
    }
    else
    {
        g_mode = MODE_MENU;
    }
    g_result = "SysupNG001";
    SetPos(Camera(0), 0.0, 0.0, 300.0, 0);
    LookAt(Camera(0), Scene(0), 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
    g_focus = create_focus_plane2("focus");
    SetOptionI(g_focus, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_focus, TAG("ZOFS"), -1.0);
    SendMessage(g_focus, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
    if (MODE_INIT != g_mode && MODE_RETRY != g_mode)
    {
        if (ask_id_dialog(g_focus, "sysupdate_start", 1, 0))
        {
            g_result = "SysupNG001";
            goto done;
        }
    }
    if (sysUpdateGetUrl(g_user, g_password, g_url) < 0)
    {
        show_message_dialog(g_focus, "msgutil_sysupdate_error_notnetsetting", PADHELP_CROSS);
        g_result = "SysupNG002";
        goto done;
    }
    get_version_info();
    if (0 == strcmp(g_url, ""))
    {
        g_result = "SysupNG003";
        goto done;
    }
    sysUpdateInit();
    ret = sysUpdateChkBnVer(g_url);
    sysUpdateExit();
    if (ret < 0)
    {
        show_message_dialog(g_focus, "msgutil_sysupdate_cannot", PADHELP_CROSS);
        g_result = "SysupNG004";
        goto done;
    }
    if (strlen(g_url) <= 0 && !count_tm_packages())
    {
        if (g_mode == MODE_RETRY)
        {
            show_message_dialog(g_focus, "msgutil_sysupdate_complete", PADHELP_CROSS);
            g_result = "SysupOK001";
        }
        else
        {
            if (MODE_INIT != g_mode)
            {
                show_message_dialog(g_focus, "msgutil_sysupdate_latestver", PADHELP_CROSS);
            }
            g_result = "SysupOK002";
        }
        goto done;
    }
    if (ask_id_dialog(g_focus, "sysupdate_ask", 1, 1))
    {
        g_result = "SysupNG001";
        goto done;
    }
    if (strlen(g_url) <= 0)
    {
        ret = update_top_menu();
    }
    else
    {
        ret = update_system(g_url);
    }
    switch (ret)
    {
        case 0:
        {
            g_result = "SysupOK001";
        }
        case ERR_DOWNLOAD:
        {
            g_result = "SysupNG008";
        }
        case ERR_CANCELED:
        {
            g_result = "SysupNG009";
        }
        case ERR_UPDATE:
        {
            g_result = "SysupNG010";
        }
        case ERR_WORK_DIR:
        {
            g_result = "SysupNG012";
        }
        case ERR_LICENSE:
        {
            g_result = "SysupNG001";
        }
        default:
        {
            g_result = "SysupNG011";
        }
    }
    done:
    avalSet("UpdateErrorFlag", g_result);
    SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
    fade_background(0, 0, 600.0);
    new url[256];
    if (g_mode == MODE_INIT || g_mode == MODE_RETRY)
    {
        if (has_isp_package())
        {
            get_tag_attr("LINK", "ispconf", "href", url, 256);
            xmlLoadStart(url, _, _, XMLLOAD_MAIN);
        }
        else
        {
            get_tag_attr("LINK", "init_setup", "href", url, 256);
            xmlLoadStart(url, _, "mode=SystemUpdate", XMLLOAD_MAIN);
        }
    }
    else if (g_mode == MODE_FROM_TOP)
    {
        get_tag_attr("LINK", "top", "href", url, 256);
        xmlLoadStart(url, _, _, XMLLOAD_MAIN);
    }
    else if (has_isp_package())
    {
        get_tag_attr("LINK", "ispconf", "href", url, 256);
        xmlLoadStart(url, _, "resume=update", XMLLOAD_MAIN);
    }
    else
    {
        get_tag_attr("LINK", "utility_top", "href", url, 256);
        xmlLoadStart(url, _, _, XMLLOAD_MAIN);
    }
    wait_xml_load();
    Destroy(Self());
    return;
}
