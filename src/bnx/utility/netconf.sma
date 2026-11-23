/*
 * /opt0/bn/script/utility/netconf.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The network settings ("start": edit, backup, restore or test; without it the first setup of the network): a
 * sequence of netconf_page pages that slide left and right, with the arrows of the pages before and after, over
 * the netconf library.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "netconf.inc"

/* The steps of g_mode. */
const MODE_DONE = -1;
const MODE_EDIT = 0;
const MODE_BACKUP = 1;
const MODE_RESTORE = 2;
const MODE_TEST = 3;
const MODE_CONNECT = 4; /* brings the network up with the saved settings */
const MODE_ERROR = 5;   /* shows the error of g_status */

/* The pages, "page_<n>" in the XML. */
const PAGE_NONE = -1;
const PAGE_START = 0;          /* the first setup starts */
const PAGE_PPPOE = 1;          /* whether to use PPPoE */
const PAGE_USER_ID = 2;        /* the user id and the password of PPPoE */
const PAGE_DHCP = 3;           /* whether the address is assigned automatically */
const PAGE_ADDRESS = 4;        /* the address, the netmask and the default route */
const PAGE_AUTO_DNS = 5;       /* whether the DNS servers are assigned automatically */
const PAGE_DNS = 6;            /* the DNS servers */
const PAGE_SUMMARY = 7;        /* the list of the settings */
const PAGE_ROUTER = 8;         /* whether to set up a router next */
const PAGE_END = 9;
const PAGE_BACKUP_PORT = 10;   /* the memory card port of a backup */
const PAGE_BACKUP_START = 11;
const PAGE_RESTORE_PORT = 12;  /* the memory card port of a restore */
const PAGE_RESTORE_START = 13;
const PAGE_COPYING = 14;       /* the backup or the restore runs */
const PAGE_COPY_DONE = 15;
const PAGE_TEST_START = 16;
const PAGE_TESTING = 17;       /* the connection test runs */
const PAGE_TEST_OK = 18;       /* with the list of the addresses */
const PAGE_TEST_FAILED = 19;   /* with the list of the addresses */
const PAGE_CONNECTING = 20;    /* the network is brought up */
const PAGE_CONNECTED = 21;
const PAGE_TO_ROUTER = 22;     /* the router setup comes next */
const PAGE_OTHER_PS2 = 23;     /* the settings are of another PS2 (NETCONF_BAD_SETTINGS): the setup starts again */
const PAGE_HOST_NOT_FOUND = 24; /* the errors of the test from here on, with the list of the addresses */
const PAGE_UNREACHABLE = 25;
const PAGE_REFUSED = 26;
const PAGE_TIMED_OUT = 27;

/* The ids of the controls of the pages; a text or an address control has the netconf item it edits as its id. */
const CTRL_PPPOE = 0;
const CTRL_DHCP = 1;
const CTRL_AUTO_DNS = 2;
const CTRL_ROUTER = 3;
const CTRL_PORT = 11;

/* The errors this script shows with their code, in g_status. */
const STATUS_CONNECT_FAILED = -50000; /* bringing the network up failed */
const STATUS_TEST_FAILED = -50001;    /* the test after bringing the network up failed */

#include "wait.inc"

#include "tag.inc"

#include <float>

#include "imgsync.inc"

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

#include "focus.inc"

#include "string.inc"

/* Shows msg in a dialog and waits until it is closed. */
show_message_dialog(focus, msg[])
{
    new args[128];
    sprintf(args, "msg=%s", msg);
    new dialog = open_dialog(args);
    SetPubVar(dialog, "focus", focus);
    SetPubVar(dialog, "padhelp", PADHELP_CROSS);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    Destroy(dialog);
}

/* Opens the dialog described by the "dialog" tag id, with the button select first. */
open_id_dialog(focus, id[], cancelable, select)
{
    new args[16];
    sprintf(args, "id=%s", id);
    new dialog = open_dialog(args);
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

#include "disc.inc"

new g_focus;
new g_config; /* the configuration netConfOpen opens */
new g_device; /* the device of the settings, set with NETCONF_DEVICE */
new g_router; /* set up a router after the settings */
new g_port; /* the memory card port of a backup or a restore, -1 before the choice */
new g_detect_state; /* the memory cards netConfDetect found */
new g_mode;
new g_status; /* of the last netconf operation, negative on error */
new g_first_setup; /* the setup of the network at the first start */
new g_on_first_page; /* the cross on the first page leaves without asking */
new g_format; /* the backup formats an unformatted memory card */
new g_overwrite; /* the backup replaces the one on the memory card */
new g_backup; /* the operation is a backup, not a restore */
new g_has_isp_file; /* the ISP settings come next (ispconf) */
new g_tested; /* the connection has been tried after saving */
new Float:g_center_x;
new Float:g_center_y;
new g_page_tex;
new g_prev_arrow;
new g_next_arrow;
new g_next_arrow_hi;
new g_prev_shown;
new g_next_shown;
new g_next_plain; /* the next arrow is not lit */
new g_cancel_shown; /* the cross of the pad help reads "cancel" */
new g_saved_cross_label[16];

#include "pfs.inc"

/* Gets the text of the item index of the list element list_id; returns its length, 0 if there is none. */
get_list_text(list_id[], index, text[], maxlen)
{
    new node;
    if (0 != (node = domFindTag(domRoot(), "list", list_id, 0)))
    {
        new key[16];
        sprintf(key, "%d", index);
        if (0 != (node = domFindTag(node, "item", key, 0)))
        {
            return get_linked_text(node, text, maxlen);
        }
    }
    return 0;
}

/* Creates a plane on the 2D display with the image of the IMG element img_id, at its size. */
create_image_plane(img_id[])
{
    new plane = CreatePlane(Disp2D(0));
    new result;
    new url[64];
    get_tag_attr("IMG", img_id, "src", url, 64);
    start_image_decode2(url);
    result = finish_image_decode();
    if (result == IMG_DONE)
    {
        SetScale(plane, float(imgTexOrgWidth()), float(imgTexOrgHeight()), 1.0);
        SetColor(plane, 1.0, 1.0, 1.0, 0.0);
        SetOptionI(plane, TAG("ALPH"), BLEND_ADD);
        SetOptionI(plane, TAG("ZSRT"), ZSORT_OVERLAY);
        SetTexture(plane, imgTexID(), 0, -1);
    }
    return plane;
}

/* Shows or hides the arrow "PREV" or "NEXT". */
show_arrow(arrow, show)
{
    if (arrow == TAG("PREV"))
    {
        if (g_prev_shown != show)
        {
            AnimColor(g_prev_arrow, 1.0, 1.0, 1.0, show ? 1.0 : 0.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
            g_prev_shown = show;
        }
    }
    else if (arrow == TAG("NEXT"))
    {
        if (g_next_shown != show)
        {
            if (g_next_plain)
            {
                AnimColor(g_next_arrow, 1.0, 1.0, 1.0, show ? 1.0 : 0.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
            }
            else
            {
                AnimColor(g_next_arrow_hi, 1.0, 1.0, 1.0, show ? 1.0 : 0.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
            }
            g_next_shown = show;
        }
    }
}

/* Shows the next arrow plain or lit. */
set_next_plain(plain)
{
    if (g_next_plain != plain)
    {
        if (g_next_shown)
        {
            if (plain)
            {
                AnimColor(g_next_arrow, 1.0, 1.0, 1.0, 1.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
                AnimColor(g_next_arrow_hi, 1.0, 1.0, 1.0, 0.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
            }
            else
            {
                AnimColor(g_next_arrow, 1.0, 1.0, 1.0, 0.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
                AnimColor(g_next_arrow_hi, 1.0, 1.0, 1.0, 1.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
            }
        }
        g_next_plain = plain;
    }
}

/* Makes the cross of the pad help read "cancel" while an operation runs, or restores it. */
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
        case PAGE_CONNECTING:
        {
            SystemCommand(SYSCMD_BUSY, 1);
            netConfDefault(-1);
            show_cancel_help(1);
        }
        case PAGE_TESTING:
        {
            SystemCommand(SYSCMD_BUSY, 1);
            netConfTest(-1);
            show_cancel_help(1);
        }
        case PAGE_BACKUP_PORT, PAGE_RESTORE_PORT:
        {
            netConfDetect();
            g_detect_state = wait_netconf();
        }
        case PAGE_COPYING:
        {
            if (g_mode == MODE_BACKUP)
            {
                netConfBackup(0, g_port, g_overwrite, g_format);
            }
            else
            {
                netConfRestore(0, g_port);
            }
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
        /* The memory card ports, enabled when netConfDetect finds a PS2 memory card in them. */
        if (page == PAGE_BACKUP_PORT || page == PAGE_RESTORE_PORT)
        {
            SendMessage(obj, TAG("ENBL"), 0, g_detect_state & 0x30 == NETCONF_CARD_PS2 << 4, 0, 0);
            SendMessage(obj, TAG("ENBL"), 1, g_detect_state & 3 == NETCONF_CARD_PS2, 0, 0);
        }
    }
    else
    {
        obj = 0;
    }
    update_arrows(page);
    return obj;
}

/* Slides a page out and fades it; returns 1 if it was the first page. */
close_page(obj, dir)
{
    new first;
    if (obj)
    {
        AnimMoveLine(obj, g_center_x + get_slide_offset(dir), g_center_y, 0.0, 200.0, 0.0, ANIM_FUNC_EXPONENT);
        SendMessage(obj, TAG("FADE"), 0, 200.0, 0, 0);
        wait_ms(250);
        show_cancel_help(0);
        first = g_on_first_page;
        g_on_first_page = 0;
    }
    return first;
}

/* Follows the operation of a page; returns 1 when it has ended. */
poll_page(obj, page)
{
    new done;
    switch (page)
    {
        case PAGE_CONNECTING:
        {
            if (!netConfSync(g_status))
            {
                if (g_status >= 0 && !g_first_setup && !g_router && !g_has_isp_file && !g_tested)
                {
                    SystemCommand(SYSCMD_BUSY, 1);
                    netConfTest(-1);
                    g_tested = 1;
                }
                else
                {
                    SystemCommand(SYSCMD_BUSY, 0);
                    if (g_status < 0 && NETCONF_CANCELED != g_status)
                    {
                        if (g_tested)
                        {
                            g_status = STATUS_TEST_FAILED;
                        }
                        else
                        {
                            g_status = STATUS_CONNECT_FAILED;
                        }
                    }
                    done = 1;
                }
            }
        }
        case PAGE_TESTING:
        {
            if (!netConfSync(g_status))
            {
                SystemCommand(SYSCMD_BUSY, 0);
                done = 1;
            }
        }
        case PAGE_BACKUP_PORT, PAGE_RESTORE_PORT:
        {
            if (!netConfSync(g_status))
            {
                netConfDetect();
                if (g_detect_state != g_status)
                {
                    SendMessage(obj, TAG("ENBL"), 0, g_status & 0x30 == NETCONF_CARD_PS2 << 4, 0, 0);
                    SendMessage(obj, TAG("ENBL"), 1, g_status & 3 == NETCONF_CARD_PS2, 0, 0);
                    g_detect_state = g_status;
                }
            }
            set_next_plain(g_port < 0);
        }
        case PAGE_COPYING:
        {
            if (netConfSync(g_status))
            {
                SendMessage(obj, TAG("PROG"), g_status, 0, 0, 0);
            }
            else
            {
                done = 1;
            }
        }
        default:
        {
        }
    }
    return done;
}

/* Shows the arrows of the pages before and after page. */
update_arrows(page)
{
    switch (page)
    {
        case PAGE_PPPOE, PAGE_BACKUP_PORT, PAGE_RESTORE_PORT:
        {
            show_arrow(TAG("PREV"), 0);
            show_arrow(TAG("NEXT"), 1);
        }
        case PAGE_END, PAGE_BACKUP_START, PAGE_RESTORE_START:
        {
            show_arrow(TAG("PREV"), 1);
            show_arrow(TAG("NEXT"), 0);
        }
        case PAGE_USER_ID .. PAGE_ROUTER:
        {
            show_arrow(TAG("PREV"), 1);
            show_arrow(TAG("NEXT"), 1);
        }
        default:
        {
            show_arrow(TAG("PREV"), 0);
            show_arrow(TAG("NEXT"), 0);
        }
    }
}

/* Returns the page after page, which depends on the settings chosen; page itself if there is none. */
get_next_page(page)
{
    switch (page)
    {
        case PAGE_PPPOE:
        {
            page = netConfGetValue(NETCONF_PPPOE) ? PAGE_USER_ID : PAGE_DHCP;
        }
        case PAGE_USER_ID:
        {
            netConfSetValue(NETCONF_DHCP, 0);
            page = PAGE_AUTO_DNS;
        }
        case PAGE_DHCP:
        {
            page = netConfGetValue(NETCONF_DHCP) ? PAGE_AUTO_DNS : PAGE_ADDRESS;
        }
        case PAGE_ADDRESS:
        {
            netConfSetValue(NETCONF_AUTO_DNS, 0);
            page = PAGE_DNS;
        }
        case PAGE_AUTO_DNS:
        {
            page = netConfGetValue(NETCONF_AUTO_DNS) ? PAGE_SUMMARY : PAGE_DNS;
        }
        case PAGE_DNS:
        {
            page = PAGE_SUMMARY;
        }
        case PAGE_SUMMARY:
        {
            page = netConfGetValue(NETCONF_PPPOE) ? PAGE_END : PAGE_ROUTER;
        }
        case PAGE_ROUTER:
        {
            page = PAGE_END;
        }
        case PAGE_BACKUP_PORT:
        {
            if (g_port >= 0)
            {
                page = PAGE_BACKUP_START;
            }
        }
        case PAGE_RESTORE_PORT:
        {
            if (g_port >= 0)
            {
                page = PAGE_RESTORE_START;
            }
        }
        default:
        {
        }
    }
    return page;
}

/* Returns the page before page; page itself if there is none. */
get_prev_page(page)
{
    switch (page)
    {
        case PAGE_USER_ID:
        {
            page = PAGE_PPPOE;
        }
        case PAGE_DHCP:
        {
            page = PAGE_PPPOE;
        }
        case PAGE_ADDRESS:
        {
            page = PAGE_DHCP;
        }
        case PAGE_AUTO_DNS:
        {
            page = netConfGetValue(NETCONF_PPPOE) ? PAGE_USER_ID : PAGE_DHCP;
        }
        case PAGE_DNS:
        {
            page = netConfGetValue(NETCONF_PPPOE) || netConfGetValue(NETCONF_DHCP) ? PAGE_AUTO_DNS : PAGE_ADDRESS;
        }
        case PAGE_SUMMARY:
        {
            page = netConfGetValue(NETCONF_AUTO_DNS) ? PAGE_AUTO_DNS : PAGE_DNS;
        }
        case PAGE_ROUTER:
        {
            page = PAGE_SUMMARY;
        }
        case PAGE_END:
        {
            page = netConfGetValue(NETCONF_PPPOE) ? PAGE_SUMMARY : PAGE_ROUTER;
        }
        case PAGE_BACKUP_START:
        {
            page = PAGE_BACKUP_PORT;
        }
        case PAGE_RESTORE_START:
        {
            page = PAGE_RESTORE_PORT;
        }
        default:
        {
        }
    }
    return page;
}

/* Answers "GET" for a control of a page: the radio menus answer whether their first button is on. */
send_value(obj, id)
{
    new text[64];
    switch (id)
    {
        case CTRL_PPPOE:
        {
            SendMessage(obj, TAG("GET"), !netConfGetValue(NETCONF_PPPOE), 0, 0, 0);
        }
        case CTRL_DHCP:
        {
            SendMessage(obj, TAG("GET"), !netConfGetValue(NETCONF_DHCP), 0, 0, 0);
        }
        case CTRL_AUTO_DNS:
        {
            SendMessage(obj, TAG("GET"), !netConfGetValue(NETCONF_AUTO_DNS), 0, 0, 0);
        }
        case CTRL_ROUTER:
        {
            SendMessage(obj, TAG("GET"), !g_router, 0, 0, 0);
        }
        case NETCONF_IP_ADDRESS:
        {
            SendMessage(obj, TAG("GET"), netConfGetValue(NETCONF_IP_ADDRESS), 0, 0, 0);
        }
        case NETCONF_NETMASK:
        {
            SendMessage(obj, TAG("GET"), netConfGetValue(NETCONF_NETMASK), 0, 0, 0);
        }
        case NETCONF_GATEWAY:
        {
            SendMessage(obj, TAG("GET"), netConfGetValue(NETCONF_GATEWAY), 0, 0, 0);
        }
        case NETCONF_DNS1:
        {
            SendMessage(obj, TAG("GET"), netConfGetValue(NETCONF_DNS1), 0, 0, 0);
        }
        case NETCONF_DNS2:
        {
            SendMessage(obj, TAG("GET"), netConfGetValue(NETCONF_DNS2), 0, 0, 0);
        }
        case NETCONF_USER_ID:
        {
            netConfGetString(NETCONF_USER_ID, text, 64);
            SendMessageStr(obj, TAG("GET"), text);
        }
        case NETCONF_PASSWORD:
        {
            netConfGetString(NETCONF_PASSWORD, text, 64);
            SendMessageStr(obj, TAG("GET"), text);
        }
        case CTRL_PORT:
        {
            SendMessage(obj, TAG("GET"), g_port > 0 ? g_port : 0, 0, 0, 0);
        }
    }
}

/* Takes the new value of a control (the ids of send_value). */
set_value(id, value)
{
    switch (id)
    {
        case CTRL_PPPOE:
        {
            netConfSetValue(NETCONF_PPPOE, 0 == value);
        }
        case CTRL_DHCP:
        {
            netConfSetValue(NETCONF_DHCP, 0 == value);
        }
        case CTRL_AUTO_DNS:
        {
            netConfSetValue(NETCONF_AUTO_DNS, 0 == value);
        }
        case CTRL_ROUTER:
        {
            g_router = 0 == value;
        }
        case NETCONF_IP_ADDRESS:
        {
            netConfSetValue(NETCONF_IP_ADDRESS, value);
        }
        case NETCONF_NETMASK:
        {
            netConfSetValue(NETCONF_NETMASK, value);
        }
        case NETCONF_GATEWAY:
        {
            netConfSetValue(NETCONF_GATEWAY, value);
        }
        case NETCONF_DNS1:
        {
            netConfSetValue(NETCONF_DNS1, value);
        }
        case NETCONF_DNS2:
        {
            netConfSetValue(NETCONF_DNS2, value);
        }
        case CTRL_PORT:
        {
            g_port = value;
        }
    }
}

set_text(id, text[])
{
    switch (id)
    {
        case NETCONF_USER_ID:
        {
            netConfSetString(NETCONF_USER_ID, text);
        }
        case NETCONF_PASSWORD:
        {
            netConfSetString(NETCONF_PASSWORD, text);
        }
    }
}

/* Answers "GTXT" with the name of a setting. */
send_title(obj, id)
{
    new title[64];
    get_list_text("setting", id, title, 64);
    SendMessageStr(obj, TAG("GTXT"), title);
}

/* Sends the row of a text setting to the list, the password as asterisks. */
send_text_row(obj, item)
{
    new row[128];
    new value[64];
    get_list_text("setting", item, row, 128);
    append_string(row, "\n", 128);
    netConfGetString(item, value, 64);
    if (item == NETCONF_PASSWORD)
    {
        for (new i; i < strlen(value); i++)
        {
            append_string(row, "*", 128);
        }
    }
    else
    {
        append_string(row, value, 128);
    }
    SendMessageStr(obj, TAG("ITEM"), row);
    sleep;
}

/* Sends the row of an address setting to the list. */
send_address_row(obj, item)
{
    new row[32];
    new name[16];
    get_list_text("setting", item, name, 16);
    item = netConfGetValue(item);
    sprintf(row, "%s\n%d.%d.%d.%d", name, item & 0xFF, item >> 8 & 0xFF, item >> 16 & 0xFF, item >> 24 & 0xFF);
    SendMessageStr(obj, TAG("ITEM"), row);
    sleep;
}

/* Sends the row of a setting that is assigned automatically to the list. */
send_auto_row(obj, item)
{
    new row[32];
    new auto_text[16];
    get_list_text("setting", item, row, 32);
    append_string(row, "\n", 32);
    get_tag_attr("TEXT", "msgutil_netconf_autosetting", "value", auto_text, 16);
    append_string(row, auto_text, 32);
    SendMessageStr(obj, TAG("ITEM"), row);
    sleep;
}

/* Sends the rows of the settings to the list, following the pages that apply. */
send_settings_list(obj)
{
    for (new page = PAGE_PPPOE; PAGE_SUMMARY != page; page = get_next_page(page))
    {
        switch (page)
        {
            case PAGE_USER_ID:
            {
                send_text_row(obj, NETCONF_USER_ID);
                send_text_row(obj, NETCONF_PASSWORD);
            }
            case PAGE_DHCP:
            {
                if (netConfGetValue(NETCONF_DHCP))
                {
                    send_auto_row(obj, NETCONF_IP_ADDRESS);
                    send_auto_row(obj, NETCONF_NETMASK);
                    send_auto_row(obj, NETCONF_GATEWAY);
                }
            }
            case PAGE_ADDRESS:
            {
                send_address_row(obj, NETCONF_IP_ADDRESS);
                send_address_row(obj, NETCONF_NETMASK);
                send_address_row(obj, NETCONF_GATEWAY);
            }
            case PAGE_AUTO_DNS:
            {
                if (netConfGetValue(NETCONF_AUTO_DNS))
                {
                    send_auto_row(obj, NETCONF_DNS1);
                    send_auto_row(obj, NETCONF_DNS2);
                }
            }
            case PAGE_DNS:
            {
                send_address_row(obj, NETCONF_DNS1);
                send_address_row(obj, NETCONF_DNS2);
            }
        }
    }
    SendMessageStr(obj, TAG("ITEM"), "");
}

/* Sends the rows of the address and of the DNS servers to the list. */
send_address_list(obj)
{
    send_address_row(obj, NETCONF_IP_ADDRESS);
    send_address_row(obj, NETCONF_DNS1);
    send_address_row(obj, NETCONF_DNS2);
    SendMessageStr(obj, TAG("ITEM"), "");
}

/* Waits until the netconf operation ends and returns its status. */
wait_netconf()
{
    new status;
    while (netConfSync(status))
    {
        sleep;
    }
    return status;
}

/* Hides the arrows, asks the dialog id and returns 1 for yes. */
confirm_dialog(id[])
{
    update_arrows(PAGE_NONE);
    wait_ms(200);
    wait_netconf();
    return 0 == ask_id_dialog(g_focus, id, 1, 1);
}

/* Hides the arrows and shows the text msg_id, followed by code when it is one of the STATUS_* codes. */
show_error(msg_id[], code)
{
    update_arrows(PAGE_NONE);
    wait_ms(200);
    wait_netconf();
    new msg[64];
    get_tag_attr("TEXT", msg_id, "value", msg, 64);
    if (code <= STATUS_CONNECT_FAILED)
    {
        new text[16];
        get_tag_attr("TEXT", "msgutil_netconf_warn_setting", "value", text, 16);
        append_string(msg, "\n(", 64);
        append_string(msg, text, 64);
        sprintf(text, " %d)", code);
        append_string(msg, text, 64);
    }
    show_message_dialog(g_focus, msg);
}

/*
 * Shows the pages from page on as the user moves between them; returns 1 when one is left with the circle, 0
 * when it is left with the cross (after asking, on most pages) or when its operation ends.
 */
run_pages(page)
{
    new current = PAGE_NONE;
    new obj;
    new dir = 3; /* how the pages slide: get_slide_offset */
    new canceled;
    new ok;
    new info;
    info = CreateLabel(Disp2D(0));
    AttachScript(info, _, "netconf_info");
    for (; ; )
    {
        sleep;
        if (current != page)
        {
            SendMessageStr(info, TAG("CHNG"), "");
            new no_confirm = close_page(obj, dir);
            if (canceled)
            {
                if (current == PAGE_CONNECTING || current == PAGE_TESTING)
                {
                    new cancel = 1;
                    while (netConfSync(g_status))
                    {
                        if (cancel)
                        {
                            netConfCancel();
                            cancel = 0;
                        }
                        sleep;
                    }
                    SystemCommand(SYSCMD_BUSY, 0);
                    no_confirm = 1;
                }
                if (no_confirm || confirm_dialog("dialog_cancel"))
                {
                    current = PAGE_NONE;
                }
                else
                {
                    page = current;
                    dir = 2;
                }
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
        new text[64];
        /*
         * From the page: "INFO" (text) for the information line, "FOCS" (direction) to go to the page before or
         * after, "PUSH" (index, button), "LIST", "GET" (id) and "GTXT" (id); any other message is the new value
         * of the control id, a number or a text.
         */
        if (RecvMessageStr(msg, text, 64))
        {
            if (msg == TAG("INFO"))
            {
                SendMessageStr(info, TAG("CHNG"), text);
            }
            else
            {
                set_text(msg, text);
            }
        }
        if (RecvMessage(msg, a, b))
        {
            if (msg == TAG("FOCS"))
            {
                if (a == TAG("LEFT"))
                {
                    page = get_prev_page(current);
                    dir = page != current ? 0 : 3;
                }
                else if (a == TAG("RIGT"))
                {
                    page = get_next_page(current);
                    dir = page != current ? 1 : 3;
                }
                if (page == current)
                {
                    SendMessage(obj, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                }
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
                    if (current < PAGE_HOST_NOT_FOUND && PAGE_COPY_DONE != current && PAGE_TEST_OK != current &&
                        PAGE_TEST_FAILED != current && PAGE_CONNECTED != current)
                    {
                        canceled = 1;
                    }
                }
                else
                {
                    SendMessage(obj, TAG("PUSH"), 0, 0, 0, 0);
                }
            }
            else if (msg == TAG("LIST"))
            {
                if (current == PAGE_SUMMARY)
                {
                    send_settings_list(obj);
                }
                else if (current == PAGE_TEST_OK || current == PAGE_TEST_FAILED || current >= PAGE_HOST_NOT_FOUND)
                {
                    send_address_list(obj);
                }
                else
                {
                    SendMessageStr(obj, TAG("ITEM"), "");
                }
            }
            else if (msg == TAG("GET"))
            {
                send_value(obj, a);
            }
            else if (msg == TAG("GTXT"))
            {
                send_title(obj, a);
            }
            else
            {
                set_value(msg, a);
            }
        }
    }
    wait_ms(200);
    wait_netconf();
    Destroy(info);
    return ok;
}

/* Returns 1 if the file of the default ISP settings is on the hard disk. */
has_default_isp()
{
    new mode;
    new size;
    return 0 == fileLstat("pfs:/__contents/bn.conf/default_isp.dat", mode, size);
}

main()
{
    start_top_page(0);
    SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
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
    new args[16];
    {
        new start[4];
        if (GetArg("start", start, 4))
        {
            g_config = 0;
            if (!strcmp(start, "edit"))
            {
                g_mode = MODE_EDIT;
            }
            else if (!strcmp(start, "backup"))
            {
                g_backup = 1;
                g_mode = MODE_BACKUP;
            }
            else if (!strcmp(start, "restore"))
            {
                g_backup = 0;
                g_mode = MODE_RESTORE;
            }
            else if (!strcmp(start, "test"))
            {
                g_mode = MODE_TEST;
            }
            sprintf(args, "resume=netconf_%s", start);
        }
        else
        {
            g_config = 0;
            g_first_setup = 1;
            g_mode = MODE_EDIT;
            copy_string(args, "mode=NetworkSetup", 16);
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
    g_prev_arrow = create_image_plane("prev");
    SetPos(g_prev_arrow, 64.0, g_center_y, 0.0, 0);
    g_next_arrow = create_image_plane("next");
    SetPos(g_next_arrow, 2.0 * g_center_x - 64.0, g_center_y, 0.0, 0);
    g_next_arrow_hi = create_image_plane("next_hi");
    SetPos(g_next_arrow_hi, 2.0 * g_center_x - 64.0, g_center_y, 0.0, 0);
    SetPos(Camera(0), 0.0, 0.0, 300.0, 0);
    LookAt(Camera(0), Scene(0), 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
    g_focus = create_focus_plane2("focus");
    SetOptionI(g_focus, TAG("ZSRT"), 1); /* acts as ZSORT_NONE */
    SetOptionF(g_focus, TAG("ZOFS"), -1.0);
    if (!g_first_setup)
    {
        g_on_first_page = 1;
    }
    netConfInit();
    if ((g_status = wait_netconf()) < 0)
    {
        g_mode = MODE_ERROR;
    }
    while (g_mode >= 0)
    {
        switch (g_mode)
        {
            case MODE_EDIT:
            {
                g_router = 0;
                g_device = 0;
                netConfGetDevCount();
                netConfOpen(g_config);
                g_status = wait_netconf();
                netConfSetValue(NETCONF_DEVICE, g_device);
                if (run_pages(g_status == NETCONF_BAD_SETTINGS ? PAGE_OTHER_PS2 : PAGE_START))
                {
                    if (run_pages(PAGE_PPPOE))
                    {
                        if (netConfGetValue(NETCONF_PPPOE))
                        {
                            g_router = 0;
                            g_has_isp_file = has_default_isp();
                        }
                        else if (!g_first_setup && !g_router)
                        {
                            g_has_isp_file = has_default_isp();
                        }
                        netConfClose();
                        wait_netconf();
                        g_mode = MODE_CONNECT;
                    }
                    else
                    {
                        g_router = 0;
                        g_mode = MODE_DONE;
                    }
                }
                else
                {
                    g_mode = MODE_DONE;
                }
            }
            case MODE_BACKUP:
            {
                g_port = -1;
                if (run_pages(PAGE_BACKUP_PORT))
                {
                    backup:
                    run_pages(PAGE_COPYING);
                    if (g_status < 0)
                    {
                        if (g_status == NETCONF_UNFORMATTED)
                        {
                            if (confirm_dialog("dialog_error_unformat"))
                            {
                                g_format = 1;
                                goto backup;
                            }
                            g_mode = MODE_DONE;
                        }
                        else if (g_status == NETCONF_BACKUP_EXISTS)
                        {
                            if (confirm_dialog("dialog_error_overwrite"))
                            {
                                g_overwrite = 1;
                                goto backup;
                            }
                            g_mode = MODE_DONE;
                        }
                        else
                        {
                            g_mode = MODE_ERROR;
                        }
                    }
                    else
                    {
                        run_pages(PAGE_COPY_DONE);
                        g_mode = MODE_DONE;
                    }
                }
                else
                {
                    g_mode = MODE_DONE;
                }
            }
            case MODE_RESTORE:
            {
                g_port = -1;
                if (run_pages(PAGE_RESTORE_PORT))
                {
                    run_pages(PAGE_COPYING);
                    if (g_status < 0)
                    {
                        g_mode = MODE_ERROR;
                    }
                    else
                    {
                        g_mode = MODE_CONNECT;
                    }
                }
                else
                {
                    g_mode = MODE_DONE;
                }
            }
            case MODE_TEST:
            {
                g_mode = MODE_DONE;
                if (run_pages(PAGE_TEST_START))
                {
                    netConfGetInfo();
                    g_status = wait_netconf();
                    if (g_status >= 0)
                    {
                        run_pages(PAGE_TESTING);
                        if (g_status == -1)
                        {
                            run_pages(PAGE_TEST_FAILED);
                        }
                        else if (g_status == NETCONF_CANCELED)
                        {
                            show_error("msgutil_netconf_nettest_cancel", -1);
                        }
                        else if (g_status >= 0)
                        {
                            run_pages(PAGE_TEST_OK);
                        }
                        else
                        {
                            g_mode = MODE_ERROR;
                        }
                    }
                    else
                    {
                        g_mode = MODE_ERROR;
                    }
                }
            }
            case MODE_CONNECT:
            {
                g_mode = MODE_DONE;
                run_pages(PAGE_CONNECTING);
                if (g_status < 0)
                {
                    if (g_status == NETCONF_CANCELED)
                    {
                        show_error("msgutil_netconf_connect_cancel", -1);
                    }
                    else
                    {
                        g_mode = MODE_ERROR;
                    }
                    g_router = 0;
                    g_has_isp_file = 0;
                }
                else if (!g_first_setup && !g_router && !g_has_isp_file)
                {
                    run_pages(PAGE_CONNECTED);
                }
            }
            case MODE_ERROR:
            {
                if (g_status <= STATUS_CONNECT_FAILED)
                {
                    show_error("msgutil_netconf_connect_fail", g_status);
                }
                else
                {
                    switch (g_status)
                    {
                        case NETCONF_NOT_CONNECTED:
                        {
                            show_error("msgutil_netconf_nettest_fail_netconnect", -1);
                        }
                        case NETCONF_HOST_NOT_FOUND:
                        {
                            run_pages(PAGE_HOST_NOT_FOUND);
                        }
                        case NETCONF_UNREACHABLE:
                        {
                            run_pages(PAGE_UNREACHABLE);
                        }
                        case NETCONF_REFUSED:
                        {
                            run_pages(PAGE_REFUSED);
                        }
                        case NETCONF_TIMED_OUT:
                        {
                            run_pages(PAGE_TIMED_OUT);
                        }
                        case NETCONF_BAD_SETTINGS:
                        {
                            show_error("msgutil_netconf_mc_warn_settingdata", -1);
                        }
                        case NETCONF_CARD_FULL:
                        {
                            show_error("msgutil_netconf_mc_warn_space", -1);
                        }
                        case NETCONF_NO_CARD:
                        {
                            show_error("msgutil_netconf_mc_warn_insertmc", -1);
                        }
                        case NETCONF_NO_SERVER:
                        {
                            show_error("msgutil_netconf_nettest_warn_providerinfo", -1);
                        }
                        case NETCONF_NOT_FOUND:
                        {
                            if (g_backup)
                            {
                                show_error("msgutil_netconf_hdd_warn_setting_notfound", -1);
                            }
                            else
                            {
                                show_error("msgutil_netconf_mc_warn_setting_notfound", -1);
                            }
                        }
                        default:
                        {
                            show_error("msgutil_netconf_nettest_error", -1);
                        }
                    }
                }
                g_mode = MODE_DONE;
            }
            default:
            {
                g_mode = MODE_DONE;
            }
        }
    }
    if (g_router)
    {
        g_router = run_pages(PAGE_TO_ROUTER);
    }
    kill_object(g_focus);
    Destroy(g_prev_arrow);
    Destroy(g_next_arrow);
    Destroy(g_next_arrow_hi);
    imgReleaseTex(g_page_tex);
    SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
    fade_background(0, 0, 600.0);
    if (g_router)
    {
        System("StartX;mode=router");
    }
    else
    {
        if (g_first_setup)
        {
            get_tag_attr("LINK", "Init", "href", url, 64);
        }
        else if (g_has_isp_file)
        {
            get_tag_attr("LINK", "Ispconf", "href", url, 64);
        }
        else
        {
            get_tag_attr("LINK", "Back", "href", url, 64);
        }
        xmlLoadStart(url, "", args, XMLLOAD_MAIN);
        wait_xml_load();
    }
    Destroy(Self());
}
