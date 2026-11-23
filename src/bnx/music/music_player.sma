/*
 * /opt0/bn/script/music/music_player.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The music player page: the tracks of an album (of the hard disk, a favorite album, the audio CD or the portable
 * device) as a grid of music_thumb items beside the jacket panel, the box with the song, artist and album names, the
 * toolbar of the player buttons, shuffle and repeat, the time bar and the sound scopes.
 */

#include <core>
#include <bn>

#pragma dynamic 2048

#include "musicdb.inc"

#include "devimage.inc"

#include "stdmenu.inc"
#include "genmenu.inc"
#include "scroll.inc"

/* Values of g_state: what the player is doing. */
enum
{
    STATE_STOPPED,
    STATE_RIPPING, /* the ripping dialog is open */
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_FORWARD,
    STATE_REWIND
};

/* The buttons of the player toolbar (g_buttons), in its order; BUTTON_RIP only for a CD. */
enum
{
    BUTTON_PREV,
    BUTTON_NEXT,
    BUTTON_REWIND,
    BUTTON_FORWARD,
    BUTTON_PLAY,
    BUTTON_PAUSE,
    BUTTON_STOP,
    BUTTON_RIP
};

/* The buttons of the toolbar below it (g_mode_buttons). */
enum
{
    BUTTON_SHUFFLE,
    BUTTON_REPEAT
};

/* The toolbars of create_toolbar and update_toolbar. */
enum
{
    TOOLBAR_PLAYER, /* g_buttons */
    TOOLBAR_MODES   /* g_mode_buttons */
};

/* Results of run_track_option besides the 0 or 1 of run_favorite (whether the album changed). */
const OPTION_CLOSE = -1;
const OPTION_ALBUM_CHANGED = 2; /* and the menu closes */
const OPTION_RENAMED = 3;

/* The items of the option menu of a track, by album: optmenu0 and optmenu1 (favorite), optmenu2 (CD), optmenu3 (PD). */
const ITEM_ADD_TO_FAVORITE = 0;
const ITEM_CHECKOUT = 1;
const ITEM_DELETE = 2;
const ITEM_INFORMATION = 3;
const ITEM_CD_INFORMATION = 0;
const ITEM_PD_CHECKIN = 0;
const ITEM_PD_DELETE = 1;
const ITEM_PD_INFORMATION = 2;

const TRACKS_UNKNOWN = -999; /* g_track_count before the album is read */
const TRACK_LAST = 1000;     /* past the last track of any album: play_track plays the last one */

#include "player.inc"

forward Float:get_node_x(node, columns, count);
forward Float:get_node_y(node, columns, count);
forward Float:get_node_z(node, columns, count);

#include "wait.inc"

#include "tag.inc"

#include <float>

#include "imgsync.inc"

/* Sleeps until a message msg arrives. */
wait_message(msg, &a, &b, &c, &d)
{
    while (!FindMessage(msg, a, b, c, d))
    {
        sleep;
    }
}

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

#include "imgdec.inc"

#include "misc.inc"

new g_db_name[] = "music";
new g_shadow_colors[4] = {0x5F000000, 0x5F000000, 0x5F000000, 0x5F000000}; /* the "SCL4" of the shadowed labels */

/* Decodes the URL-encoded src ("+" and "%XX") into the packed dest of maxlen cells; stops at a bad escape. */
decode_url(src[], dest[], maxlen)
{
    new out;
    for (new i; src{i}; i++)
    {
        new c = src{i};
        new value;
        if (c == '+')
        {
            value = ' ';
        }
        else if (c == '%')
        {
            new digits[2];
            for (new k; k < 2; k++)
            {
                i++;
                c = src{i};
                if ('0' <= c <= '9')
                {
                    digits[k] = c - '0';
                }
                else if ('A' <= c <= 'F')
                {
                    digits[k] = c - 'A' + 10;
                }
                else if ('a' <= c <= 'f')
                {
                    digits[k] = c - 'a' + 10;
                }
                else
                {
                    return;
                }
            }
            value = (digits[0] << 4) + digits[1];
        }
        else if ('a' <= c <= 'z' || 'A' <= c <= 'Z' || '0' <= c <= '9' || c == '*' || c == '-' || c == '.' || c == '@'
                || c == '_')
        {
            value = c;
        }
        else
        {
            value = c;
        }
        if (out >= maxlen * 4 - 1)
        {
            return;
        }
        dest{out} = value;
        out++;
    }
}

#include "string.inc"

#include "dom.inc"

#include "disc.inc"

#include "layout.inc"

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

/* Makes the camera of the background window follow this camera, by these shares of its position and rotation. */
link_bg_camera(Float:pos_follow, rot_follow)
{
    new bg_scene = wndGetScene(WND_BG);
    if (bg_scene)
    {
        SendMessage(bg_scene, TAG("LINK"), Camera(0), pos_follow, rot_follow, 0);
    }
}

#include "fileop.inc"

#include "focus.inc"

/* Same as send_focus of dom.inc. */
send_focus2(obj, target, z_offset, Float:sharp, Float:extra_w, Float:extra_h, Float:move_time, smoothing, blinking)
{
    if (!obj)
    {
        return;
    }
    SendMessage(obj, TAG("FOC0"), target, TAG(""), z_offset, sharp);
    SendMessage(obj, TAG("FOC1"), extra_w, extra_h, move_time, smoothing);
    SendMessage(obj, TAG("FOC2"), blinking, 0, 0, 0);
}

public stdmenu_curr_group; /* the MENUGROUP element of the option menu shown */

/* Makes the MENUGROUP element id the current one; returns whether it exists. */
find_menu_group(id[])
{
    stdmenu_curr_group = domFindTag(domRoot(), "MENUGROUP", id, 0);
    if (stdmenu_curr_group)
    {
        return 1;
    }
    return 0;
}

#include "validate.inc"

/* Reads the retgroup and retitem of the menu: the menu and the item to go back to; returns 0 if there are none. */
get_menu_return(ret_group[], ret_item[], group_len, item_len)
{
    if (!stdmenu_curr_group)
    {
        return 0;
    }
    if (!domGetData(stdmenu_curr_group, "retgroup", ret_group, group_len))
    {
        return 0;
    }
    if (!domGetData(stdmenu_curr_group, "retitem", ret_item, item_len))
    {
        return 0;
    }
    return 1;
}

/* Reads the label of the TITLE element of the menu; returns 0 if there is none. */
get_menu_title(label[], maxlen)
{
    if (!stdmenu_curr_group)
    {
        return 0;
    }
    new title = domFirstChild(stdmenu_curr_group, "TITLE");
    if (!title)
    {
        return 0;
    }
    if (!domGetData(title, "label", label, maxlen))
    {
        return 0;
    }
    return 1;
}

/* Returns the number of items of the current menu group, -1 without one. */
count_menu_items()
{
    new count;
    if (!stdmenu_curr_group)
    {
        return -1;
    }
    for (new node = domFirstChild(stdmenu_curr_group, "ITEM"); node; node = domNext(node, "ITEM"))
    {
        count++;
    }
    return count;
}

/* Returns the index of the item with that id in the current menu group, -1 if there is none. */
find_menu_item_index(id[])
{
    new index;
    if (!stdmenu_curr_group)
    {
        return -1;
    }
    for (new node = domFirstChild(stdmenu_curr_group, "ITEM"); node; node = domNext(node, "ITEM"))
    {
        new value[64];
        domGetData(node, "id", value, 64);
        if (!strcmp(value, id))
        {
            return index;
        }
        index++;
    }
    return -1;
}

/* Reads attribute attr of item index of the menu; returns 0 if there is no such item. */
get_item_attr(index, attr[], value[], maxlen)
{
    new i;
    if (!stdmenu_curr_group || index < 0)
    {
        return 0;
    }
    new node;
    node = domFirstChild(stdmenu_curr_group, "ITEM");
    for (; node && i < index; node = domNext(node, "ITEM"))
    {
        i++;
    }
    if (!node)
    {
        return 0;
    }
    return domGetData(node, attr, value, maxlen);
}

/*
 * Runs on the object of pop_std_menu: waits for "INIT" (manager, side, result, mode), shows the title and a
 * general_menu of the items of the MENUGROUP group (MENU_MODE_GROUP) and of those the page adds (count_extra_items),
 * and, when a push leads to another menu, sends "TITL", "GRUP", "IDTY" and "FNSH" (result, mode, selected
 * index) to the manager.
 */
public std_menu_core()
{
    new title[64];
    GetArg("title", title, 64);
    new group[64];
    GetArg("group", group, 64);
    new id[64];
    GetArg("id", id, 64);
    sleep;
    new self = Self();
    new Float:screen_w;
    new Float:screen_h;
    get_screen_size5(screen_w, screen_h);
    new manager;
    new result;
    new side;
    new mode;
    {
        new init_manager;
        new init_side;
        new init_result;
        new init_mode;
        wait_message(TAG("INIT"), init_manager, init_side, init_result, init_mode);
        manager = init_manager;
        side = init_side;
        result = init_result;
        mode = init_mode;
    }
    find_menu_group(group);
    if (MENU_MODE_GROUP == mode)
    {
        get_menu_title(title, 64);
    }
    sleep;
    new title_label;
    new menu;
    new scroll_bar;
    rebuild:
    new selected;
    new item_count;
    if (MENU_MODE_GROUP == mode)
    {
        item_count = item_count + count_menu_items();
        selected = find_menu_item_index(id);
        if (selected < 0)
        {
            selected = 0;
        }
    }
    sleep;
    item_count = item_count + count_extra_items(side, result, mode, title, group, id, selected);
    if (item_count <= 0 || selected < 0)
    {
        goto finish;
    }
    title_label = CreateLabel(self);
    {
        new Float:x;
        new Float:y;
        get_list_title_pos(0, item_count, 1, x, y);
        SetPos(title_label, 0.0 * screen_w, screen_h * y, 0.0, 0);
        SetColor(title_label, 0.902, 0.902, 0.215, 0.0);
        AnimColor(title_label, 0.902, 0.902, 0.215, 1.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
        SetOptionI(title_label, TAG("FONT"), FontID(0));
        SetOptionS(title_label, TAG("UTF8"), title);
        SetOptionI(title_label, TAG("FSIZ"), 17);
        SetOptionI(title_label, TAG("ZSRT"), ZSORT_OVERLAY);
        SetOptionI(title_label, TAG("HALN"), ALIGN_CENTER);
        SetOptionI(title_label, TAG("VALN"), ALIGN_CENTER);
    }
    sleep;
    menu = CreateNull(self);
    new columns = 1;
    new rows = 5;
    {
        new Float:y;
        get_list_top(item_count, 1, y);
        SetPos(menu, 0.0 * screen_w, screen_h * y, 0.0, 0);
        SetOptionI(menu, TAG("ZSRT"), ZSORT_OVERLAY);
        AttachScript(menu, "general_menu", "general_menu");
        SetPubVar(menu, "manager", Self());
        SetPubVar(menu, "focus", GetPubVar(manager, "focus_glowfly"));
        SetPubVar(menu, "pad_enable", PAD_CROSS);
        rows = item_count >= 8 ? 8 : item_count;
        SendMessage(menu, TAG("INUM"), item_count, 0, 0, 0);
        SendMessage(menu, TAG("CLRW"), columns, rows, 0, 0);
        SendMessage(menu, TAG("PADL"), -1, 1, 0, 0);
        SendMessage(menu, TAG("DIST"), 0, 0.0583 * screen_h, 0, 0);
        SendMessage(menu, TAG("MGIN"), 2, 2, 0, 0);
        SendMessage(menu, TAG("MODE"), MENU_TYPE_POPUP, SCROLL_LINE, 0, 0);
        SendMessage(menu, TAG("ANIM"), 200.0, 0, ANIM_FUNC_EXPONENT, 0);
        SendMessage(menu, TAG("OFFA"), -1.0, 0, 0, 0);
        SendMessage(menu, TAG("IDLA"), 1.0, 0, 0, 0);
        SendMessage(menu, TAG("REPT"), 100.0, 100.0, 0, 0);
        SendMessage(menu, TAG("SLCT"), selected, 0, 0, 0);
        SendMessage(menu, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
    }
    sleep;
    if (item_count > rows * columns)
    {
        scroll_bar = CreateNull(self);
        new Float:bar_x;
        new Float:bar_y;
        new Float:list_x;
        new bar_side;
        if (side == MENU_SIDE_RIGHT)
        {
            bar_side = MENU_SIDE_RIGHT;
        }
        get_list_pos(bar_side, item_count, 1, list_x, bar_y);
        get_scroll_bar_pos(bar_side, item_count, 1, bar_x, bar_y);
        SetPos(scroll_bar, screen_w * (bar_x - list_x), screen_h * bar_y, 0.0, 0);
        get_list_body_height(item_count, 1, bar_y);
        SetOptionI(scroll_bar, TAG("ZSRT"), ZSORT_OVERLAY);
        AttachScript(scroll_bar, _, "scroll_bar");
        SetPubVar(scroll_bar, "manager", Self());
        SetPubVar(scroll_bar, "focus", GetPubVar(manager, "focus_glowfly"));
        SetPubVar(scroll_bar, "height", screen_h * (bar_y + 0.0583 * 2.0));
    }
    sleep;
    for (; ; )
    {
        result = 0;
        new msg;
        new a;
        new b;
        new c;
        /* "ITEM" (menu, path, index): create the item of index; "PUSH" (index, button, item). */
        while (RecvMessage(msg, a, b, c))
        {
            if (msg == TAG("ITEM"))
            {
                if (a == menu)
                {
                    new index = c;
                    new label[64];
                    if (MENU_MODE_GROUP == mode)
                    {
                        get_item_attr(index, "label", label, 64);
                    }
                    fill_menu_item(side, mode, index, group, label);
                    new item = create_label_item(b, label);
                    SendMessage(a, TAG("ITEM"), item, c, 0, 0);
                }
            }
            if (msg == TAG("PUSH"))
            {
                SendMessage(menu, TAG("PUSH"), 0, 0, 0, 0);
                SendMessage(menu, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                selected = a;
                if (MENU_MODE_GROUP == mode)
                {
                    get_item_attr(selected, "id", id, 64);
                }
                result = handle_menu_push(manager, side, mode, selected, b, title, group, id);
                if (MENU_MODE_GROUP == mode)
                {
                    apply_result(result, selected, group, id);
                }
                if (result < MENU_REOPEN)
                {
                    SendMessage(menu, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                }
            }
        }
        if (scroll_bar)
        {
            new scroll_top = GetPubVar(menu, "scroll_top");
            SetPubVar(scroll_bar, "top", float(scroll_top) / float(item_count));
            SetPubVar(scroll_bar, "length", float(rows * columns) / float(item_count));
        }
        if (result >= MENU_REDRAW)
        {
            SetSystemParam(SYS_PADHELP, 0);
            SendMessage(menu, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
            break;
        }
        if (ScriptState(menu) == SCRIPT_FINISHED)
        {
            break;
        }
        sleep;
    }
    finish:
    if (result >= MENU_REOPEN)
    {
        SendMessageStr(manager, TAG("TITL"), title);
        SendMessageStr(manager, TAG("GRUP"), group);
        SendMessageStr(manager, TAG("IDTY"), id);
        SendMessage(manager, TAG("FNSH"), result, mode, selected, 0);
    }
    if (menu)
    {
        SendMessage(menu, TAG("FADE"), 0, 300.0, 0, 0);
    }
    if (title_label)
    {
        AnimColor(title_label, 0.902, 0.902, 0.215, 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    }
    wait_ms(300);
    if (menu)
    {
        Destroy(menu);
        menu = 0;
    }
    if (scroll_bar)
    {
        Destroy(scroll_bar);
        scroll_bar = 0;
    }
    if (title_label)
    {
        Destroy(title_label);
        title_label = 0;
    }
    if (result == MENU_REDRAW)
    {
        goto rebuild;
    }
    return selected;
}

/*
 * Shows the option menu of the MENUGROUP group (prefix is not used) with item select selected, on the left of
 * the screen (flags & 0x3 is 0 or 2) or on the right (1), over a dark backdrop if flags & 0x300, and the menus
 * it leads to; returns the result, MENU_CLOSED or MENU_ITEM + the chosen index.
 */
pop_std_menu(prefix[], group[], flags, select)
{
    new result;
    new mode;
    new side = flags & 0x3;
    new Float:screen_w;
    new Float:screen_h;
    get_screen_size5(screen_w, screen_h);
    new menu_obj = CreateNull(Disp2D(0));
    if (MENU_SIDE_LEFT == side || side == 2)
    {
        new Float:x;
        new Float:y;
        get_list_pos(MENU_SIDE_LEFT, 5, 1, x, y);
        SetPos(menu_obj, screen_w * x, 0.0, 0.0, 0);
    }
    else if (side == MENU_SIDE_RIGHT)
    {
        new Float:x;
        new Float:y;
        get_list_pos(MENU_SIDE_RIGHT, 5, 1, x, y);
        SetPos(menu_obj, screen_w * x, 0.0, 0.0, 0);
    }
    new backdrop;
    new backdrop_kind = flags & 0x300;
    if (0 != backdrop_kind)
    {
        backdrop = CreatePlane(Disp2D(0));
        if (MENU_SIDE_LEFT == side)
        {
            SetPos(backdrop, 0.25 * screen_w, 0.5 * screen_h, 0.0, 0);
        }
        else if (side == MENU_SIDE_RIGHT)
        {
            SetPos(backdrop, 0.75 * screen_w, 0.5 * screen_h, 0.0, 0);
        }
        SetLocalScale(backdrop, 0.5 * screen_w, 2.0 * (0.364 * screen_h), 0.0);
        SetColor(backdrop, 0.0, 0.0, 0.0, 0.0);
        AnimColor(backdrop, 0.0, 0.0, 0.0, 0.3, 500.0, 0.0, ANIM_FUNC_LINEAR);
        SetOptionI(backdrop, TAG("ZSRT"), ZSORT_BACKGROUND);
        if (backdrop_kind == MENU_BACKDROP_FADED)
        {
            SetOptionI(backdrop, TAG("FDEG"), 1);
        }
    }
    new cur_title[64];
    new cur_group[64];
    new cur_id[64];
    copy_string(cur_group, prefix, 64);
    copy_string(cur_group, group, 64);
    find_menu_group(cur_group);
    if (!get_item_attr(select, "id", cur_id, 64))
    {
        cur_id = "";
    }
    sleep;
    for (; ; )
    {
        new args[256];
        sprintf(args, "title = %s ; group = %s ; id = %s", cur_title, cur_group, cur_id);
        AttachScript(menu_obj, "std_menu_core", _, args);
        SendMessage(menu_obj, TAG("INIT"), Self(), side, result, mode);
        for (; ; )
        {
            new msg;
            new a;
            new b;
            new c;
            new str[64];
            while (RecvMessageStr(msg, str, 64))
            {
                if (msg == TAG("TITL"))
                {
                    copy_string(cur_title, str, 64);
                }
                if (msg == TAG("GRUP"))
                {
                    copy_string(cur_group, str, 64);
                }
                if (msg == TAG("IDTY"))
                {
                    copy_string(cur_id, str, 64);
                }
            }
            if (FindMessage(TAG("FNSH"), a, b, c))
            {
                result = a;
                mode = b;
            }
            if (ScriptState(menu_obj) == SCRIPT_FINISHED)
            {
                break;
            }
            sleep;
        }
        if (result >= MENU_CANCEL)
        {
            break;
        }
    }
    if (0 != backdrop)
    {
        AnimColor(backdrop, 0.0, 0.0, 0.0, 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
        wait_ms(300);
        Destroy(backdrop);
    }
    if (menu_obj)
    {
        Destroy(menu_obj);
    }
    return result;
}

/* Sets the group and id of the menu that result leads to from item index; MENU_CLOSED if there is none. */
apply_result(&result, index, group[], id[])
{
    if (result == MENU_RETURN)
    {
    }
    else if (result == MENU_SUBMENU)
    {
        new sub_group[64];
        get_item_attr(index, "subgroup", sub_group, 64);
        printf("%s -------------\n", sub_group);
        if (!strcmp(sub_group, ""))
        {
            printf("WARNING: nothing sub item !!\n");
        }
        else
        {
            set_menu_return(sub_group, group, id);
            copy_string(group, sub_group, 64);
        }
    }
    else if (result == MENU_BACK)
    {
        new ret_group[64];
        new ret_item[64];
        get_menu_return(ret_group, ret_item, 64, 64);
        printf(" %d, %s, %s, %s, %s \n", index, group, id, ret_group, ret_item);
        if (!strcmp(ret_group, "") || !strcmp(ret_item, ""))
        {
            printf("WARNING: nothing return menu !!\n");
            result = MENU_CLOSED;
        }
        else
        {
            copy_string(group, ret_group, 64);
            copy_string(id, ret_item, 64);
        }
    }
}

/* Creates a shadowed label item of an option menu. */
create_label_item(parent, label[])
{
    new item;
    item = CreateLabelShadow(parent);
    SetOptionP(item, TAG("SCL4"), g_shadow_colors, 4);
    SetOptionI(item, TAG("SX64"), 2 * 64);
    SetOptionI(item, TAG("SY64"), 2 * 64);
    SetColor(item, 0.215, 0.708, 0.999, 0.0);
    SetOptionI(item, TAG("ALPH"), BLEND_ADD);
    SetOptionI(item, TAG("FONT"), FontID(0));
    SetOptionS(item, TAG("UTF8"), label);
    SetOptionI(item, TAG("FSIZ"), 15);
    SetOptionI(item, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(item, TAG("VALN"), ALIGN_CENTER);
    AttachScript(item, "label_item", "label_item");
    SetPubVar(item, "pad_enable", PAD_CIRCLE);
    return item;
}

new g_focus_box; /* the box with the names of the song, the artist and the album */

/* Creates the box at x, y (a focus_box part) unless it exists. */
create_focus_box(Float:x, Float:y)
{
    if (!g_focus_box)
    {
        g_focus_box = CreateNull(Disp2D(0));
        SetPos(g_focus_box, x, y, 0.0, 0);
        SetOptionI(g_focus_box, TAG("ZSRT"), ZSORT_OVERLAY);
        AttachScript(g_focus_box, _, "focus_box");
    }
}

/* Sets header line 0 to 2 of the box. */
set_box_header(line, text[])
{
    if (g_focus_box)
    {
        switch (line)
        {
            case 0:
            {
                SendMessageStr(g_focus_box, TAG("HED0"), text);
            }
            case 1:
            {
                SendMessageStr(g_focus_box, TAG("HED1"), text);
            }
            case 2:
            {
                SendMessageStr(g_focus_box, TAG("HED2"), text);
            }
        }
    }
}

/* Sets the icon of header line 0 to 2 of the box to the image of the url text. */
set_box_icon(line, text[])
{
    if (g_focus_box)
    {
        switch (line)
        {
            case 0:
            {
                SendMessageStr(g_focus_box, TAG("ICN0"), text);
            }
            case 1:
            {
                SendMessageStr(g_focus_box, TAG("ICN1"), text);
            }
            case 2:
            {
                SendMessageStr(g_focus_box, TAG("ICN2"), text);
            }
        }
    }
}

/* Fades the whole box to alpha and its color to red, green, blue. */
fade_box(Float:alpha, Float:duration, Float:red, Float:green, Float:blue)
{
    if (g_focus_box)
    {
        SendMessage(g_focus_box, TAG("FADE"), alpha, duration, 0, 0);
        SendMessage(g_focus_box, TAG("FCOL"), red, green, blue, duration);
    }
}

new g_device_image;
new Float:g_device_x = 130.0;
new Float:g_device_y = 350.0;
new Float:g_device_w = 512.0;
new Float:g_device_h = 512.0;

/* Does nothing. */
setup_box(x, y)
{
}

/* Creates the plane of the device image (a device_image part), transparent until shown. */
create_device_image()
{
    g_device_image = CreatePlane(Camera(0));
    SetPos(g_device_image, g_device_x, g_device_y, 0.0, Disp2D(0));
    SetRot(g_device_image, 0.0, 0.0, 0.0, Disp2D(0), 0);
    SetColor(g_device_image, 1.0, 1.0, 1.0, 0.0);
    SetScale(g_device_image, g_device_w, g_device_h, 1.0);
    SetOptionI(g_device_image, TAG("ZSRT"), ZSORT_BACKGROUND);
    SetOptionI(g_device_image, TAG("ALPH"), BLEND_ADD);
    SetOptionF(g_device_image, TAG("ZOFS"), -100.0);
    AttachScript(g_device_image, _, "device_image");
}

/* Shows the image of device, DEVICE_*, or none. */
show_device_image(device)
{
    if (!g_device_image)
    {
        return;
    }
    if (device == DEVICE_DISC)
    {
        SendMessageStr(g_device_image, TAG("IMAG"), "file:/opt0/bn/data/tex/device_image/disc.tm2");
        SendMessage(g_device_image, TAG("POS"), g_device_x, g_device_y - 15.0, 0, 0);
    }
    else if (device == DEVICE_HDD)
    {
        SendMessageStr(g_device_image, TAG("IMAG"), "file:/opt0/bn/data/tex/device_image/hdd.tm2");
        SendMessage(g_device_image, TAG("POS"), g_device_x, g_device_y, 0, 0);
    }
    else if (device == DEVICE_MC)
    {
        SendMessageStr(g_device_image, TAG("IMAG"), "file:/opt0/bn/data/tex/device_image/mc.tm2");
        SendMessage(g_device_image, TAG("POS"), g_device_x - 30.0, 5.0 + g_device_y, 0, 0);
    }
    else if (device == DEVICE_USB)
    {
        SendMessageStr(g_device_image, TAG("IMAG"), "file:/opt0/bn/data/tex/device_image/usb.tm2");
        SendMessage(g_device_image, TAG("POS"), 30.0 + g_device_x, g_device_y, 0, 0);
    }
    else
    {
        SendMessageStr(g_device_image, TAG("IMAG"), "");
    }
}

public pad_enable; /* unused, but part of the layout */
public padhelp; /* unused, but part of the layout */
public focus; /* unused, but part of the layout */
new g_caption_labels2[2][2];
new g_caption_index2;
new Float:g_caption_x2;
new Float:g_caption_y2;

/* Creates the labels of show_caption2, with a shadow if shadow. */
create_captions(shadow)
{
    new i;
    new j;
    {
        new Float:screen_w;
        new Float:screen_h;
        get_screen_size5(screen_w, screen_h);
        g_caption_x2 = 0.546875 * screen_w;
        g_caption_y2 = 0.5 * screen_h;
    }
    i = 0;
    for (; i < 2; i++)
    {
        j = 0;
        for (; j < 2; j++)
        {
            if (!shadow)
            {
                g_caption_labels2[i][j] = CreateLabel(Disp2D(0));
            }
            else
            {
                g_caption_labels2[i][j] = CreateLabelShadow(Disp2D(0));
            }
            SetPos(g_caption_labels2[i][j], g_caption_x2, g_caption_y2, 0.0, 0);
            SetOptionI(g_caption_labels2[i][j], TAG("FONT"), FontID(0));
            SetOptionI(g_caption_labels2[i][j], TAG("HALN"), ALIGN_LEFT);
            SetOptionI(g_caption_labels2[i][j], TAG("VALN"), ALIGN_CENTER);
            SetOptionI(g_caption_labels2[i][j], TAG("FSIZ"), 17);
            if (shadow)
            {
                SetOptionP(g_caption_labels2[i][j], TAG("SCL4"), g_shadow_colors, 4);
                SetOptionI(g_caption_labels2[i][j], TAG("SX64"), 2 * 64);
                SetOptionI(g_caption_labels2[i][j], TAG("SY64"), 2 * 64);
            }
            SetColor(g_caption_labels2[i][j], 0.7843, 0.7843, 0.7843, 0.0);
        }
    }
}

/* Same as show_caption of string.inc, with globals of its own. */
show_caption2(line1[] = "", line2[] = "", Float:target_a, Float:duration, Float:delay)
{
    new incoming = g_caption_index2;
    g_caption_index2 = ++g_caption_index2 & 1;
    new outgoing = g_caption_index2;
    SetOptionS(g_caption_labels2[incoming][0], TAG("UTF8"), line1);
    SetOptionS(g_caption_labels2[incoming][1], TAG("UTF8"), line2);
    if (strlen(line2) > 0)
    {
        SetPos(g_caption_labels2[incoming][0], g_caption_x2, g_caption_y2 - 12.0, 0.0, 0);
        SetPos(g_caption_labels2[incoming][1], g_caption_x2, 12.0 + g_caption_y2, 0.0, 0);
    }
    else
    {
        SetPos(g_caption_labels2[incoming][0], g_caption_x2, g_caption_y2, 0.0, 0);
        SetPos(g_caption_labels2[incoming][1], g_caption_x2, g_caption_y2, 0.0, 0);
    }
    AnimColor(g_caption_labels2[incoming][0], 0.7843, 0.7843, 0.7843, target_a, duration, delay, ANIM_FUNC_LINEAR);
    AnimColor(g_caption_labels2[incoming][1], 0.7843, 0.7843, 0.7843, target_a, duration, delay, ANIM_FUNC_LINEAR);
    AnimColor(g_caption_labels2[outgoing][0], 0.7843, 0.7843, 0.7843, 0.0, 0.5 * duration, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_caption_labels2[outgoing][1], 0.7843, 0.7843, 0.7843, 0.0, 0.5 * duration, 0.0, ANIM_FUNC_LINEAR);
}

/* The option menu of a track and the image of the device, by MUSIC_ALBUM_*; the colors of the favorite albums. */
new g_option_groups[4][3] = {"optmenu1", "optmenu0", "optmenu2", "optmenu3"};
new g_album_device[4] = {DEVICE_HDD, DEVICE_HDD, DEVICE_DISC, DEVICE_USB};
new Float:g_favorite_colors[7][3] = {
    {0.56, 0.71, 1.0}, {0.56, 1.0, 0.69}, {0.84, 0.89, 0.42}, {0.89, 0.67, 0.39}, {0.88, 0.46, 0.46},
    {0.89, 0.58, 0.75}, {0.46, 0.36, 0.55}
};

/* Same as set_caption_pos of misc.inc, for show_caption2. */
set_caption_pos2(Float:x, Float:y)
{
    g_caption_x2 = x;
    g_caption_y2 = y;
}

/* Loads the page of "LINK" link_id, or the built-in top menu, with args. */
load_page(link_id[], args[])
{
    if (!strcmp(link_id, "top"))
    {
        xmlLoadStart("$$BUILDIN_TOPMENU$$", _, args, XMLLOAD_MAIN);
    }
    else
    {
        new url[256];
        get_tag_attr("LINK", link_id, "href", url, 256);
        xmlLoadStart(url, _, args, XMLLOAD_MAIN);
    }
    wait_xml_load();
}

/* Opens the information page ("LINK" link_id) as a modal window, with its editmode and media_watcher arguments. */
open_info_page(link_id[], edit_mode, watch)
{
    new url[256];
    new args[16];
    sprintf(args, "editmode=%d;media_watcher=%d", edit_mode, watch);
    get_tag_attr("LINK", link_id, "href", url, 256);
    xmlLoadStart(url, _, args, XMLLOAD_MODAL);
    wait_xml_load();
}

/* Sets the variable name to the number value. */
set_aval_int(name[], value)
{
    new text[16];
    sprintf(text, "%d", value);
    avalSet(name, text);
}

/* Returns the number in the variable name, 0 if there is none. */
get_aval_int(name[])
{
    new text[16];
    if (0 != avalGetValue(name, 0, text, 16))
    {
        return 0;
    }
    return atoi(text);
}

/* Sets the variable name to the number value if it is not set yet. */
init_aval_int(name[], value)
{
    new text[16];
    if (0 != avalGetValue(name, 0, text, 16))
    {
        set_aval_int(name, value);
    }
}

/* Same as reset_player of playerctl.inc. */
reset_player()
{
    playerExecCommand(PLAYER_RESET);
    return 1;
}

/* Same as stop_player of playerctl.inc. */
stop_player()
{
    printf("---------------- stopping \n");
    playerExecCommand(PLAYER_STOP, PLAYER_UNIT_CD, 0, 0);
    new state = PLAYER_STATE_STOPPED;
    new info;
    do
    {
        playerExecCommand(PLAYER_CD_GET_STATE, state, info, 0);
        sleep;
    }
    while (PLAYER_STATE_STOPPED != state);
    return;
}

new g_box_icons[3][3] = {"song_icon", "artist_icon", "album_icon"};
new g_shuffle_labels[2][32] = {"OFF", "ON"};
new g_repeat_labels[3][32] = {"OFF", "ALL", "1"};
public idx; /* the player, from reset_player */
new g_camera;
new Float:g_screen_w;
new Float:g_screen_h;
public camera_focus; /* the point the camera looks at */
public camera_uptgt; /* unused, but part of the layout */
public focus_glowfly; /* the focus object */
new g_active; /* g_album, g_buttons or g_mode_buttons: the one that takes the pad */
new g_album; /* the grid of the tracks */
new g_buttons; /* the toolbar of the player buttons, or the update button of the portable device */
new g_mode_buttons; /* the toolbar of shuffle and repeat */
new g_jacket;
new g_time_panel;
new g_time_label;
new g_meter;
new g_mode_labels[2]; /* the values of shuffle and repeat */
new g_scopes[2]; /* the sound scopes of the two channels */
new g_scope_bases[2];
new g_play_icons[2]; /* unused, but part of the layout */
new g_next_link[256]; /* the page to leave for: a "LINK" id, "top" or "music" */
new g_next_args[256];
new g_quit;
new g_disc_type; /* CDVD_* */
new g_frame_count;
new g_padhelp; /* the pad help of the toolbars and of the album */
new g_list_only; /* the tracks of the portable device are only listed: no player */
new g_album_name[256];
new g_album_mode; /* MUSIC_ALBUM_* */
new g_album_no;
new g_artist[256];
new g_track_count; /* the tracks of the album, below 0 on an error */
new g_state; /* STATE_* */
new g_track; /* the track being played, -1 for none */
new g_last_track = -1; /* g_track of the last change */
new g_wait_track = -1; /* the track asked for, until the player reaches it */
new g_start_track; /* unused, but part of the layout */
new g_seek_button = -1; /* always -1 */
new g_time[3]; /* the time played: minutes, seconds and frames, in BCD */
new g_last_time[3];
new g_percent; /* the part of the track played */
new g_scope_left[32];
new g_scope_right[32];
new g_select; /* the selected track of the album */
new g_select_idle; /* the album has stopped on g_select */
new g_song_name[256];
new g_shuffle; /* 1: on */
new g_repeat; /* REPEAT_* */
new g_quality; /* the recording quality, QUALITY_* */
new g_active_tex; /* the texture of the track being played */
new g_pd_tracks; /* the tracks of the portable device, -1 without one */
new g_checkout_times; /* the check-out count of the selected track (PLAYER_GET_TRACK_INFO) */
new g_bitrate; /* the bitrate of its compressed copy, BITRATE_* */
new Float:g_jacket_pos[3];
new Float:g_jacket_menu_pos[3];
new g_shuffle_order[999];
new g_shuffle_list[999]; /* the order of the tracks with shuffle on */
new g_shuffle_pos; /* the position in g_shuffle_list */
new g_page_mode; /* the album mode to give back to the music page */
new g_db_open;
new g_seconds; /* the time played, in seconds */
new g_track_offset; /* added to the track numbers of the CD */
new g_favorites_cached; /* the favorite dialog has run once */
new g_time_resets; /* the times the time of the CD with shuffle was reset to 0 */

/* Copies src into dest doubling each single quote, for a quoted SQL string. */
escape_sql(dest[], src[])
{
    new len = strlen(src);
    new out;
    for (new i; i < len; i++)
    {
        if (src{i} == '\'')
        {
            dest{out++} = '\'';
            dest{out++} = '\'';
        }
        else if (src{i} == '\\')
        {
            dest{out++} = '\\';
        }
        else
        {
            dest{out++} = src{i};
        }
    }
    dest{out} = 0;
}

/* Builds the page, runs it until it leaves, and loads the next page (argument bref, if any, first). */
public main()
{
    g_next_link = "top";
    g_next_args = "";
    idx = reset_player();
    init_page();
    while (1 != g_quit)
    {
        handle_messages();
        sleep;
    }
    close_page();
    stop_track();
    if (0 != strcmp(g_next_link, "collection") && 0 != strcmp(g_next_link, "favorite")
            && 0 != strcmp(g_next_link, "music"))
    {
        stop_player();
    }
    if (g_album_mode == MUSIC_ALBUM_CD || g_album_mode == MUSIC_ALBUM_PD)
    {
        set_aval_int("MusicAlbumMode", g_page_mode);
    }
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
    playerExecCommand(PLAYER_SET_DIN_THRU, 0);
    fade_background(0, 0, 600.0);
    new bref[256];
    if (GetArg("bref", bref, 256))
    {
        xmlLoadStart(bref, _, _, XMLLOAD_MAIN);
        wait_xml_load();
    }
    load_page(g_next_link, g_next_args);
}

/*
 * Builds the page on the album of the aval file (MusicAlbumMode, MusicAlbumNo, MusicAlbumName), or on the folder of
 * the argument folder; starts the watcher of the disc or of the portable device, and plays the track of the argument
 * autostart.
 */
init_page()
{
    g_camera = Camera(0);
    get_screen_size5(g_screen_w, g_screen_h);
    g_quit = 0;
    g_disc_type = CDVD_NO_DISC;
    g_frame_count = 0;
    g_padhelp = PADHELP_CIRCLE | PADHELP_CROSS;
    g_list_only = 0;
    g_album_name = "";
    g_album_no = 0;
    g_artist = "";
    g_track_count = TRACKS_UNKNOWN;
    g_state = STATE_STOPPED;
    g_track = -1;
    g_time[0] = 0;
    g_time[1] = 0;
    g_time[2] = 0;
    g_select = -1;
    g_select_idle = 0;
    g_song_name = "";
    g_shuffle = 0;
    g_repeat = REPEAT_OFF;
    new autostart = -1;
    get_tag_attr("TEXT", "shuffle_off", "value", g_shuffle_labels[0], 32);
    get_tag_attr("TEXT", "shuffle_on", "value", g_shuffle_labels[1], 32);
    get_tag_attr("TEXT", "repeat_off", "value", g_repeat_labels[0], 32);
    get_tag_attr("TEXT", "repeat_all", "value", g_repeat_labels[1], 32);
    get_tag_attr("TEXT", "repeat_1", "value", g_repeat_labels[2], 32);
    create_captions(0);
    set_aval_int("MusicMediaChanged", 0);
    set_aval_int("MusicChannelMode", CHANNEL_TRACK);
    init_aval_int("MusicAlbumMode", MUSIC_ALBUM_CD);
    init_aval_int("MusicPrefNewDisc", 0);
    playerExecCommand(PLAYER_GET_PREFS, g_quality, g_repeat, g_shuffle);
    init_aval_int("MusicPrefQuality", g_quality);
    set_aval_int("MusicRepeatMode", g_repeat);
    set_aval_int("MusicShuffleMode", g_shuffle);
    g_album_mode = get_aval_int("MusicAlbumMode");
    g_page_mode = g_album_mode;
    g_album_no = get_aval_int("MusicAlbumNo");
    avalGetValue("MusicAlbumName", 0, g_album_name, 256);
    new arg[256];
    if (GetArg("folder", arg, 256))
    {
        new name[256];
        new slash = find_last_any_char(arg, "/", -1);
        if (slash < 0)
        {
            slash = -1;
        }
        copy_substring(name, 0, arg, slash + 1, -1, 256);
        decode_url(name, g_album_name, 256);
        avalSet("MusicAlbumName", g_album_name);
        for (new i; i < strlen(arg) - 5; i++)
        {
            name{i} = arg{i + 5};
        }
        avalSet("ContentPath", name);
        append_string(name, "   ", 256);
        playerExecCommand(PLAYER_SET_ALBUM_PATH, name);
        g_album_mode = MUSIC_ALBUM_COLLECTION;
        g_album_no = 0;
    }
    if (GetArg("autostart", arg, 256))
    {
        autostart = atoi(arg);
        if (autostart < 0)
        {
            autostart = TRACK_LAST;
        }
    }
    if (g_album_mode == MUSIC_ALBUM_COLLECTION)
    {
        if (g_album_no == ALBUM_NO_CD)
        {
            g_album_mode = MUSIC_ALBUM_CD;
        }
        else if (g_album_no == ALBUM_NO_PD)
        {
            g_album_mode = MUSIC_ALBUM_PD;
        }
    }
    else if (g_album_mode == MUSIC_ALBUM_CD)
    {
        g_page_mode = MUSIC_ALBUM_COLLECTION;
        set_aval_int("MusicAlbumMode", MUSIC_ALBUM_COLLECTION);
        g_album_no = ALBUM_NO_CD;
        set_aval_int("MusicAlbumNo", g_album_no);
    }
    else if (g_album_mode == MUSIC_ALBUM_PD)
    {
        g_page_mode = MUSIC_ALBUM_COLLECTION;
        set_aval_int("MusicAlbumMode", MUSIC_ALBUM_COLLECTION);
    }
    printf("============ ALBUM_MODE %d ALBUM_NO %d\n", g_album_mode, g_album_no);
    if (g_album_mode == MUSIC_ALBUM_CD)
    {
        get_tag_attr("TEXT", "msgmusic_ainfo_audiocd", "value", g_album_name, 256);
        avalSet("MusicAlbumName", g_album_name);
    }
    if (g_album_mode == MUSIC_ALBUM_PD)
    {
        get_tag_attr("TEXT", "msgmusic_menu_pd", "value", g_album_name, 256);
    }
    if (g_album_mode == MUSIC_ALBUM_CD)
    {
        playerExecCommand(PLAYER_SET_DIN_THRU, 1);
        playerExecCommand(PLAYER_CD_READ_TOC, 1);
    }
    else
    {
        playerExecCommand(PLAYER_SET_DIN_THRU, 0);
    }
    if (g_album_mode == MUSIC_ALBUM_PD)
    {
        g_padhelp = PADHELP_CROSS;
        g_list_only = 1;
    }
    else
    {
    }
    set_aval_int("MediaWatcherFlag", 1);
    if (g_album_mode == MUSIC_ALBUM_CD)
    {
        new args[256];
        sprintf(args, "owner = %d", Self());
        new watcher = CreateNull(Self());
        AttachScript(watcher, "media_watcher_main", _, args);
        SetPubVar(watcher, "idx", idx);
    }
    else if (g_album_mode == MUSIC_ALBUM_PD)
    {
        new args[256];
        sprintf(args, "owner = %d", Self());
        new watcher = CreateNull(Self());
        AttachScript(watcher, "pd_watcher_main", _, args);
        SetPubVar(watcher, "idx", idx);
    }
    g_track_count = query_album(g_album_mode);
    shuffle_tracks(g_track_count);
    g_play_icons[0] = load_icon_texture("play");
    g_play_icons[1] = load_icon_texture("pause");
    start_top_page(0);
    load_common_bg("bg", 1000.0);
    set_background_poor(600.0, 0);
    camera_focus = CreateNull(Self());
    camera_uptgt = CreateNull(Self());
    SetPos(camera_focus, 0.0, 0.0, -100.0, 0);
    SetPos(camera_uptgt, 0.0, 0.0 + 1000.0, 0.0, 0);
    SetPos(g_camera, 0.0, 0.0, 0.0, 0);
    LookAt(g_camera, camera_focus, 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
    AttachScript(g_camera, "camera_main");
    SetPubVar(g_camera, "camera_focus", camera_focus);
    link_bg_camera(4.0, 0);
    reset_bg_camera();
    focus_glowfly = create_focus_plane2("focus");
    create_device_image();
    g_album = create_album(g_track_count < 0 ? 0 : g_track_count, autostart);
    g_buttons = create_toolbar(TOOLBAR_PLAYER);
    g_mode_buttons = create_toolbar(TOOLBAR_MODES);
    g_jacket = create_jacket_panel();
    g_time_panel = create_time_panel();
    fade_time_panel(0.0, 0.0);
    create_focus_box(0.7625 * g_screen_w, -25.0 + 0.4911 * g_screen_h);
    fade_box(0.0, 0.0, 1.0, 1.0, 1.0);
    setup_box(0, 0);
    {
        new url[256];
        for (new i; i < 3; i++)
        {
            get_tag_attr("IMG", g_box_icons[i], "src", url, 256);
            set_box_icon(i, url);
        }
    }
    set_box_header(0, "0123456789");
    show_device_image(g_album_device[g_album_mode]);
    show_album_error(0);
    g_active = g_track_count > 0 ? g_album : g_buttons;
    if (g_album_mode == MUSIC_ALBUM_PD)
    {
        if (g_track_count <= 0)
        {
            SetSystemParam(SYS_PADHELP, g_padhelp);
            SetPubVar(g_album, "padhelp", g_padhelp);
            SetPubVar(g_album, "pad_enable", PAD_CROSS);
        }
        else
        {
            SetPubVar(g_album, "pad_enable", PAD_TRIANGLE | PAD_CROSS);
        }
    }
    if (g_track_count <= 0)
    {
        update_track_info();
    }
    if (g_album_mode == MUSIC_ALBUM_PD && g_track_count <= 0)
    {
        SendMessage(g_active, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
    }
    else
    {
        SendMessage(g_active, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
    }
    new default_url[256];
    get_tag_attr("IMG", "default_icon", "src", default_url, 256);
    SendMessageStr(g_jacket, TAG("DCDE"), default_url);
    set_aval_int("MediaWatcherFlag", 0);
    if (autostart >= 0)
    {
        play_track(autostart);
    }
}

/* Destroys the level meters, takes the focus from the parts and closes the database. */
close_page()
{
    Destroy(g_scopes[0]);
    Destroy(g_scopes[1]);
    g_scopes[0] = 0;
    g_scopes[1] = 0;
    SendMessage(g_album, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
    SendMessage(g_buttons, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
    SendMessage(g_mode_buttons, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
    datClose();
}

new g_init_status = -2; /* the initialization of the player: below 0 until it is known, 0 on an error */

/*
 * Handles the messages of one frame: "FOCS" and "PUSH" of the album or of the toolbar that takes the pad, the push of
 * the forward and rewind buttons (index, button, released) as they are held; "ITEM" (menu, path, index) of the album;
 * "DISC" and "PDEV" of the watchers; then the pad, the album, the change of track and the position of the player.
 */
handle_messages()
{
    if (g_init_status < 0)
    {
        playerExecCommand(PLAYER_GET_INIT, g_init_status);
        if (0 == g_init_status)
        {
            printf("INIT ERR\n");
            show_unit_warning();
            return 0;
        }
    }
    new msg;
    new a;
    new b;
    new c;
    new d;
    while (RecvMessage(msg, a, b, c, d))
    {
        if (g_active == g_album)
        {
            if (msg == TAG("FOCS"))
            {
                on_album_focus(a);
            }
            if (msg == TAG("PUSH"))
            {
                on_album_push(b);
            }
        }
        if (g_active == g_buttons)
        {
            if (msg == TAG("FOCS"))
            {
                on_buttons_focus(a);
            }
            if (msg == TAG("PUSH"))
            {
                new index = a;
                if ((index == BUTTON_FORWARD || index == BUTTON_REWIND) && b == TAG("RRGT"))
                {
                    on_seek_push(a, c);
                }
                else
                {
                    on_buttons_push(b);
                }
            }
        }
        if (g_active == g_mode_buttons)
        {
            if (msg == TAG("FOCS"))
            {
                on_mode_buttons_focus(a);
            }
            if (msg == TAG("PUSH"))
            {
                on_mode_buttons_push(b);
            }
        }
        if (msg == TAG("ITEM"))
        {
            send_album_item(a, b, c, g_track_count);
        }
        if (msg == TAG("PUSH"))
        {
            on_push(b);
        }
        if (msg == TAG("DISC"))
        {
            on_disc_change(a);
        }
        if (msg == TAG("PDEV"))
        {
            on_pd_change(a);
        }
        if (msg == MSG_NAVIMENU_OPENED && g_album_mode == MUSIC_ALBUM_CD)
        {
            g_track_count = 0;
            g_disc_type = CDVD_DETECTING;
            show_album_error(0);
            playerExecCommand(PLAYER_CD_READ_TOC, 1);
            on_disc_change(CDVD_DETECTING);
        }
    }
    if (STATE_STOPPED == g_state)
    {
        SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
    }
    else
    {
        SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
    }
    handle_pad();
    update_album();
    on_track_change();
    update_position();
    g_frame_count++;
    return 1;
}

/* The forward or rewind button held starts the seek; released, it plays on. */
on_seek_push(index, released)
{
    new offset = index - BUTTON_REWIND;
    if (0 == released)
    {
        if (g_state == STATE_PLAYING && g_seek_button == -1)
        {
            if (index == BUTTON_REWIND)
            {
                rewind(0);
            }
            else if (index == BUTTON_FORWARD)
            {
                fast_forward(0);
            }
        }
    }
    else
    {
        new held;
        PadState(held, _, _, _, -1);
        held = held & (PAD_L2 | PAD_R2);
        if ((g_state == STATE_FORWARD || g_state == STATE_REWIND) && !held)
        {
            resume_track();
            g_seek_button = -1;
        }
    }
    SendMessage(g_active, TAG("PUSH"), index, 0, 0, 0);
}

/* The cross leaves for the music page when the player is stopped, else stops it. */
on_push(button)
{
    if (button == TAG("RDWN"))
    {
        if (g_state == STATE_RIPPING)
        {
            g_state = STATE_STOPPED;
            sndSePlay(SE_CANCEL);
        }
        else
        {
            if (STATE_STOPPED == g_state)
            {
                g_quit = 1;
                g_next_link = "music";
                if (g_album_mode == MUSIC_ALBUM_CD)
                {
                    g_album_mode = MUSIC_ALBUM_COLLECTION;
                }
                sndSePlay(SE_CANCEL);
                return;
            }
            stop_track();
        }
        return;
    }
    if (button == TAG("SLCT"))
    {
        printf("-================== SELECT\n");
    }
}

new g_hold_frames; /* the frames L2 or R2 has been held */

/* L1 and R1 go to the previous and next track; L2 and R2 held 15 frames rewind and fast forward. */
handle_pad()
{
    new held;
    new pressed;
    new released;
    PadState(held, pressed, released, _, -1);
    if (0 == pressed && 0 == released && 0 == g_hold_frames)
    {
        return;
    }
    if (g_list_only)
    {
        if (pressed & PAD_CROSS)
        {
            sndSePlay(SE_CANCEL);
            g_quit = 1;
            g_next_link = "music";
            return;
        }
    }
    if (pressed & PAD_L1 && (g_state == STATE_PLAYING || g_state == STATE_PAUSED))
    {
        prev_track();
        return;
    }
    if (pressed & PAD_R1 && (g_state == STATE_PLAYING || g_state == STATE_PAUSED))
    {
        next_track();
        return;
    }
    new cursor = GetPubVar(g_buttons, "cursor");
    if (held & PAD_CIRCLE && g_active == g_buttons && (cursor == BUTTON_REWIND || cursor == BUTTON_FORWARD))
    {
        printf("ignored:L2/R2 %d\n", g_seek_button);
        return;
    }
    held = held & (PAD_L2 | PAD_R2);
    if (held == PAD_L2 || held == PAD_R2)
    {
        g_hold_frames++;
        if (g_hold_frames < 15)
        {
            held = 0;
        }
    }
    else
    {
        g_hold_frames = 0;
        held = 0;
    }
    switch (held)
    {
        case 0:
        {
            if (g_state == STATE_FORWARD || g_state == STATE_REWIND)
            {
                resume_track();
            }
            g_seek_button = -1;
        }
        case PAD_L2:
        {
            if (g_state == STATE_PLAYING && g_seek_button == -1)
            {
                printf("player_fr:on_any_pad\n");
                rewind(0);
            }
            else if (g_state == STATE_FORWARD)
            {
                resume_track();
                g_seek_button = -1;
            }
        }
        case PAD_R2:
        {
            if (g_state == STATE_PLAYING && g_seek_button == -1)
            {
                printf("player_ff:on_any_pad\n");
                fast_forward(0);
            }
            else if (g_state == STATE_REWIND)
            {
                resume_track();
                g_seek_button = -1;
            }
        }
    }
}

/* Reloads the album when the tracks of the portable device change. */
on_pd_change(tracks)
{
    reload_album();
}

/* Stops an audio CD that was playing and reloads the album when the disc changes. */
on_disc_change(type)
{
    if (g_disc_type == CDVD_CDDA)
    {
        stop_track();
    }
    g_disc_type = type;
    reload_album();
}

/* Loads the album again and gives the pad to it, or to the toolbar if it is empty. */
reload_album()
{
    send_focus2(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    SendMessage(g_album, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
    SendMessage(g_buttons, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
    SendMessage(g_mode_buttons, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
    if (g_active == g_album)
    {
        g_active = g_buttons;
    }
    g_track_count = load_album(g_album);
    show_album_error(0);
    if (g_track_count > 0)
    {
        SendMessage(g_album, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        g_active = g_album;
        printf("====================== changed focus = thumb_list\n");
        if (g_album_mode == MUSIC_ALBUM_PD)
        {
            SetPubVar(g_album, "pad_enable", PAD_TRIANGLE | PAD_CROSS);
        }
    }
    else
    {
        printf("====================== no data\n");
        if (g_disc_type == CDVD_DETECTING)
        {
            send_focus2(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        }
        else if (g_album_mode == MUSIC_ALBUM_PD)
        {
            SetSystemParam(SYS_PADHELP, g_padhelp);
            SetPubVar(g_album, "padhelp", g_padhelp);
            SetPubVar(g_album, "pad_enable", PAD_CROSS);
            SendMessage(g_active, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
        }
        else
        {
            SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        }
    }
    update_track_info();
}

/* Reads the track, the time played and the sound buffers from the player; stops when it reports no track. */
update_position()
{
    if (STATE_STOPPED == g_state || g_state == STATE_RIPPING)
    {
        return;
    }
    new unused; /* unused, but part of the layout */
    new unused2;
    g_track = -1;
    new unused3; /* unused, but part of the layout */
    if (g_album_mode == MUSIC_ALBUM_CD)
    {
        new track = -1;
        playerExecCommand(PLAYER_CD_GET_TRACK, track, 0, 0);
        g_track = track - g_track_offset;
        playerExecCommand(PLAYER_CD_GET_TIME, g_time, g_time[1], g_time[2]);
        playerExecCommand(PLAYER_CD_GET_PERCENT, g_percent, unused, unused2);
        playerExecCommand(PLAYER_GET_WAVE, g_scope_left, g_scope_right);
    }
    else
    {
        playerExecCommand(PLAYER_HDD_GET_TRACK, g_track, 0, 0);
        playerExecCommand(PLAYER_HDD_GET_TIME, g_time, g_time[1], g_time[2]);
        playerExecCommand(PLAYER_HDD_GET_PERCENT, g_percent, unused, unused2);
        playerExecCommand(PLAYER_GET_WAVE, g_scope_left, g_scope_right);
    }
    if (g_track > 0)
    {
        g_track = g_track - 1;
    }
    else if (g_track < -1)
    {
        stop_track();
        return;
    }
    update_time();
}

/*
 * When the player has gone to another track, follows repeat and shuffle: plays the track they lead to, or stops at
 * the end of the album.
 */
on_track_change()
{
    if (g_track == g_last_track)
    {
        return;
    }
    printf("track %d track_old %d track_wait %d shuffle_trk %d\n", g_track, g_last_track, g_wait_track, g_shuffle_pos);
    if (g_track >= g_track_count && g_repeat == REPEAT_ONE)
    {
        g_wait_track = -1;
    }
    if (g_track < 0)
    {
        goto show_track;
    }
    if (g_wait_track >= 0)
    {
        if (g_track == g_wait_track || g_last_track == g_wait_track)
        {
            g_wait_track = -1;
            goto show_track;
        }
        g_last_track = g_track;
        return;
    }
    new held;
    PadState(held, _, _, _, -1);
    held = held & (PAD_L2 | PAD_R2);
    new cursor = GetPubVar(g_buttons, "cursor");
    if (g_repeat == REPEAT_ONE)
    {
        g_select = g_last_track;
        g_seek_button = -1;
        goto play_select;
    }
    if (g_repeat == REPEAT_ALL)
    {
        if (0 == g_shuffle)
        {
            if (g_track > g_last_track && g_last_track == g_track_count - 1 && STATE_REWIND != g_state)
            {
                g_select = 0;
                g_seek_button = -1;
                goto play_select;
            }
            if (g_track >= g_track_count && g_state == STATE_REWIND)
            {
                stop_track();
                goto show_track;
            }
        }
        else
        {
            if (g_album_mode == MUSIC_ALBUM_CD)
            {
                if (g_track > g_last_track)
                {
                    if (!g_last_track && g_track == g_track_count && g_state == STATE_REWIND)
                    {
                        g_shuffle_pos--;
                    }
                    else
                    {
                        g_shuffle_pos++;
                    }
                }
                else if (g_track < g_last_track)
                {
                    g_shuffle_pos--;
                }
                else if (g_last_track == g_wait_track)
                {
                    g_last_track = -1;
                }
            }
            else
            {
                g_shuffle_pos++;
            }
            if (g_shuffle_pos >= g_track_count)
            {
                g_shuffle_pos = 0;
            }
            if (g_shuffle_pos < 0)
            {
                g_shuffle_pos = 0;
            }
            g_select = g_shuffle_list[g_shuffle_pos];
            if (g_track == g_select)
            {
                g_track = -1;
            }
            goto play_select;
        }
        goto show_track;
    }
    if (0 == g_shuffle)
    {
        if (g_track >= g_track_count)
        {
            stop_track();
            goto show_track;
        }
        goto show_track;
    }
    if (g_album_mode == MUSIC_ALBUM_CD)
    {
        if (g_track > g_last_track)
        {
            if (!g_last_track && g_track == g_track_count && g_state == STATE_REWIND)
            {
                g_shuffle_pos--;
            }
            else
            {
                g_shuffle_pos++;
            }
        }
        else if (g_track < g_last_track)
        {
            g_shuffle_pos--;
        }
        else if (g_last_track == g_wait_track)
        {
            g_last_track = -1;
        }
    }
    else
    {
        g_shuffle_pos++;
    }
    if (g_shuffle_pos < 0)
    {
        g_shuffle_pos = 0;
    }
    if (g_shuffle_pos < g_track_count)
    {
        g_select = g_shuffle_list[g_shuffle_pos];
        if (g_track == g_select)
        {
            g_track = -1;
        }
        goto play_select;
    }
    g_shuffle_pos = -1;
    stop_track();
    goto show_track;
    show_track:
    SendMessage(g_album, TAG("THRU"), -1, TAG("ACTV"), g_track, 0);
    update_track_info();
    g_last_track = g_track;
    return;
    play_select:
    if (g_select == g_track)
    {
        SendMessage(g_album, TAG("THRU"), -1, TAG("ACTV"), g_track, 0);
    }
    else
    {
        play_track(-1);
    }
    g_last_track = g_track;
}

/* Creates the album item of index and sends it to the album ("ITEM"). */
send_album_item(menu, path, index, count)
{
    if (IsValidObject(path))
    {
        new item = create_album_item(path);
        SendMessage(menu, TAG("ITEM"), item, path, 0, 0);
    }
    else
    {
        printf("invalid parent %d\n", path);
    }
}

/* Right from the album gives the pad to the toolbar. */
on_album_focus(direction)
{
    if (!g_list_only)
    {
        if (direction == TAG("RIGT"))
        {
            g_active = g_buttons;
            SendMessage(g_active, TAG("FOCS"), TAG("LEFT"), 0, 0, 0);
            return;
        }
    }
    SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
}

/*
 * Circle or start on a track plays it, pauses it or resumes it; triangle while stopped opens the option menu of the
 * track and reloads the album if an option changed it.
 */
on_album_push(button)
{
    if (button == TAG("RRGT") || button == TAG("STRT"))
    {
        SendMessage(g_active, TAG("PUSH"), 0, 0, 0, 0);
        if (!g_list_only)
        {
            if (STATE_STOPPED == g_state || g_select != g_track)
            {
                sndSePlay(SE_DECIDE);
                play_track(-1);
                g_start_track = g_select;
            }
            else if (g_state == STATE_PLAYING)
            {
                pause_track();
            }
            else
            {
                resume_track();
            }
        }
        return;
    }
    new has_song = 1;
    if (0 == strlen(g_song_name))
    {
        has_song = 0;
    }
    if (button == TAG("RUP") && STATE_STOPPED == g_state && has_song)
    {
        update_track_info();
        sndSePlay(SE_OPEN);
        sndSePlay(SE_WINDOW_OPEN);
        SendMessage(g_active, TAG("PUSH"), TAG("RUP"), 0, 0, 0);
        SendMessage(g_active, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
        send_focus2(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        fade_menus(0.0, 400.0);
        slide_camera(1);
        SendMessage(g_album, TAG("FADE"), 0, 1000.0, 0, 0);
        wait_ms(1000);
        new result;
        new action;
        new redraw;
        new media_changed;
        set_aval_int("MusicMediaChanged", 0);
        while (OPTION_CLOSE != action)
        {
            SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
            media_changed = get_aval_int("MusicMediaChanged");
            if (!(media_changed & 1))
            {
                set_aval_int("MusicMediaChanged", 0);
            }
            result = pop_std_menu("optmenu", g_option_groups[g_album_mode], MENU_SIDE_RIGHT, result - MENU_ITEM);
            action = run_track_option(result);
            if (action >= 1)
            {
                redraw = action;
            }
            if (action == OPTION_ALBUM_CHANGED)
            {
                action = OPTION_CLOSE;
            }
            printf("redraw = %d\n", redraw);
            if (action == OPTION_RENAMED)
            {
                new name[128];
                avalGetValue("SongName", 0, name, 128);
                set_box_header(0, name);
                avalGetValue("MusicAlbumName", 0, name, 128);
                set_box_header(2, g_album_name);
                avalGetValue("ArtistName", 0, name, 128);
                set_box_header(1, g_artist);
            }
            media_changed = get_aval_int("MusicMediaChanged");
            if (g_album_mode == MUSIC_ALBUM_CD && media_changed & 1)
            {
                break;
            }
        }
        SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        media_changed = get_aval_int("MusicMediaChanged");
        if (media_changed)
        {
            if (media_changed == 2)
            {
                g_track_count = -2;
            }
            else
            {
                g_track_count = 0;
                g_disc_type = CDVD_DETECTING;
                playerExecCommand(PLAYER_CD_READ_TOC, 1);
            }
            show_album_error(0);
            redraw = 1;
        }
        sndSePlay(SE_WINDOW_CLOSE);
        center_camera(1);
        SendMessage(g_album, TAG("FADE"), 1.0, 1000.0, 0, 0);
        wait_ms(1000);
        SendMessage(g_album, TAG("FADE"), 1.0, 500.0, 0, 0);
        fade_menus(1.0, 400.0);
        if (!redraw)
        {
            SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        }
        else if (redraw == OPTION_RENAMED)
        {
            query_album(g_album_mode);
            update_track_info();
            SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        }
        else
        {
            reload_album();
        }
        SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        return;
    }
    SendMessage(g_active, TAG("PUSH"), 0, 0, 0, 0);
}

/* Sets the pad help of the album, triangle while stopped, and shows the selected track once the album stops. */
update_album()
{
    if (g_track_count <= 0)
    {
        return;
    }
    if (MUSIC_ALBUM_PD != g_album_mode)
    {
        if (STATE_STOPPED == g_state && g_active == g_album)
        {
            SetSystemParam(SYS_PADHELP, g_padhelp | PADHELP_TRIANGLE);
            SetPubVar(g_album, "padhelp", g_padhelp | PADHELP_TRIANGLE);
            SetPubVar(g_album, "pad_enable", PAD_START | PAD_TRIANGLE | PAD_CIRCLE | PAD_CROSS);
        }
        else
        {
            SetSystemParam(SYS_PADHELP, g_padhelp);
            SetPubVar(g_album, "padhelp", g_padhelp);
            SetPubVar(g_album, "pad_enable", PAD_START | PAD_CIRCLE | PAD_CROSS);
        }
    }
    new old_select = g_select;
    new idle = GetPubVar(g_album, "idle");
    g_select = GetPubVar(g_album, "select");
    new was_idle = g_select_idle;
    if (old_select != g_select)
    {
        g_select_idle = 0;
    }
    else if (0 != idle)
    {
        g_select_idle = 1;
    }
    if (g_select_idle != was_idle)
    {
        if (g_select_idle && STATE_STOPPED == g_state)
        {
            update_track_info();
        }
        else
        {
        }
    }
}

new g_buttons_cursor = -1; /* the cursor of g_buttons when the pad went down to g_mode_buttons */

/* Left gives the pad to the album, down to the toolbar of shuffle and repeat. */
on_buttons_focus(direction)
{
    if (direction == TAG("LEFT"))
    {
        if (g_track_count > 0)
        {
            g_active = g_album;
            SendMessage(g_active, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
            return;
        }
    }
    else if (direction == TAG("DOWN"))
    {
        if (!g_list_only)
        {
            new cursor = GetPubVar(g_buttons, "cursor");
            g_buttons_cursor = cursor;
            if (cursor >= BUTTON_PLAY)
            {
                cursor = BUTTON_REPEAT;
            }
            else
            {
                cursor = BUTTON_SHUFFLE;
            }
            SetPubVar(g_mode_buttons, "cursor", cursor);
            g_active = g_mode_buttons;
            SendMessage(g_active, TAG("FOCS"), TAG("UP"), 0, 0, 0);
            return;
        }
    }
    SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
}

/* Left gives the pad to the album, up back to the player buttons. */
on_mode_buttons_focus(direction)
{
    if (direction == TAG("LEFT"))
    {
        if (g_track_count > 0)
        {
            g_active = g_album;
            SendMessage(g_active, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
            return;
        }
    }
    else if (direction == TAG("UP"))
    {
        if (!g_list_only)
        {
            if (g_buttons_cursor >= 0)
            {
                SetPubVar(g_buttons, "cursor", g_buttons_cursor);
            }
            else
            {
                SetPubVar(g_buttons, "cursor", BUTTON_PREV);
            }
            g_buttons_cursor = -1;
            g_active = g_buttons;
            SendMessage(g_active, TAG("FOCS"), TAG("DOWN"), 0, 0, 0);
            return;
        }
    }
    SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
}

/*
 * Runs the player button under the cursor: 0 previous, 1 next, 2 rewind, 3 fast forward, 4 play, 5 pause, 6 stop,
 * 7 rip the CD; start plays or pauses. For the portable device, the update button checks it again.
 */
on_buttons_push(button)
{
    if (button == TAG("RRGT"))
    {
        if (g_list_only)
        {
            SendMessage(g_active, TAG("PUSH"), 0, 0, 0, 0);
            check_pd(0);
            reload_album();
            return;
        }
        if (g_track_count <= 0)
        {
            printf("======== do nothing\n");
            SendMessage(g_active, TAG("PUSH"), 0, 0, 0, 0);
            return;
        }
        new cursor = GetPubVar(g_buttons, "cursor");
        switch (cursor)
        {
            case BUTTON_STOP:
            {
                stop_track();
            }
            case BUTTON_REWIND:
            {
                rewind(0);
            }
            case BUTTON_FORWARD:
            {
                fast_forward(0);
            }
            case BUTTON_PREV:
            {
                if (g_state == STATE_PLAYING || g_state == STATE_PAUSED)
                {
                    prev_track();
                }
            }
            case BUTTON_PLAY:
            {
                if (STATE_STOPPED == g_state)
                {
                    if (g_shuffle == 1)
                    {
                        g_shuffle_pos = 0;
                        g_select = g_shuffle_list[g_shuffle_pos];
                    }
                    else
                    {
                        g_select = 0;
                    }
                    play_track(-1);
                    g_start_track = g_select;
                }
                else if (g_state == STATE_PLAYING)
                {
                    if (g_track > g_track_count - 1)
                    {
                        g_track = 0;
                    }
                    g_select = g_track;
                    stop_track();
                    play_track(-1);
                }
                else if (g_state == STATE_PAUSED)
                {
                    resume_track();
                }
            }
            case BUTTON_NEXT:
            {
                if (g_state == STATE_PLAYING || g_state == STATE_PAUSED)
                {
                    next_track();
                }
            }
            case BUTTON_PAUSE:
            {
                if (g_state == STATE_PLAYING)
                {
                    pause_track();
                }
                else if (g_state == STATE_PAUSED)
                {
                    resume_track();
                }
            }
            case BUTTON_RIP:
            {
                run_ripping();
                SendMessage(g_active, TAG("PUSH"), 0, 0, 0, 0);
                return;
            }
        }
        SendMessage(g_active, TAG("PUSH"), 0, 0, 0, 0);
        return;
    }
    if (button == TAG("STRT"))
    {
        if (STATE_STOPPED == g_state)
        {
            if (g_shuffle == 1)
            {
                g_shuffle_pos = 0;
                g_select = g_shuffle_list[g_shuffle_pos];
            }
            else
            {
                g_select = 0;
            }
            play_track(-1);
            g_start_track = g_select;
        }
        else if (g_state == STATE_PLAYING)
        {
            if (g_track > g_track_count - 1)
            {
                g_track = 0;
                g_select = g_track;
                stop_track();
                play_track(-1);
            }
            else
            {
                pause_track();
            }
        }
        else
        {
            resume_track();
        }
        SendMessage(g_active, TAG("PUSH"), 0, 0, 0, 0);
        return;
    }
    SendMessage(g_active, TAG("PUSH"), 0, 0, 0, 0);
}

/* Circle switches shuffle or repeat; start plays or pauses. */
on_mode_buttons_push(button)
{
    if (button == TAG("RRGT"))
    {
        new cursor = GetPubVar(g_mode_buttons, "cursor");
        switch (cursor)
        {
            case BUTTON_SHUFFLE:
            {
                toggle_shuffle();
            }
            case BUTTON_REPEAT:
            {
                cycle_repeat();
            }
        }
        SendMessage(g_active, TAG("PUSH"), 0, 0, 0, 0);
        return;
    }
    if (button == TAG("STRT"))
    {
        if (STATE_STOPPED == g_state)
        {
            if (g_shuffle == 1)
            {
                g_shuffle_pos = 0;
                g_select = g_shuffle_list[g_shuffle_pos];
            }
            else
            {
                g_select = 0;
            }
            play_track(-1);
            g_start_track = g_select;
        }
        else if (g_state == STATE_PLAYING)
        {
            if (g_track > g_track_count - 1)
            {
                g_track = 0;
                g_select = g_track;
                stop_track();
                play_track(-1);
            }
            else
            {
                pause_track();
            }
        }
        else
        {
            resume_track();
        }
        SendMessage(g_active, TAG("PUSH"), 0, 0, 0, 0);
        return;
    }
    SendMessage(g_active, TAG("PUSH"), 0, 0, 0, 0);
}

/*
 * Runs the option result of the menu of a track, by album: add to a favorite album, check out or in, delete,
 * information; returns an OPTION_* or the result of run_favorite.
 */
run_track_option(result)
{
    new action;
    SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
    result = result - MENU_ITEM;
    switch (g_album_mode)
    {
        case MUSIC_ALBUM_COLLECTION:
        {
            switch (result)
            {
                case ITEM_ADD_TO_FAVORITE:
                {
                    return run_favorite();
                }
                case ITEM_CHECKOUT:
                {
                    run_checkout();
                }
                case ITEM_DELETE:
                {
                    action = run_delete();
                    if (action)
                    {
                        action = OPTION_ALBUM_CHANGED;
                    }
                }
                case ITEM_INFORMATION:
                {
                    action = show_information();
                    if (action)
                    {
                        action = OPTION_RENAMED;
                    }
                }
                default:
                {
                    action = OPTION_CLOSE;
                }
            }
        }
        case MUSIC_ALBUM_FAVORITE:
        {
            switch (result)
            {
                case ITEM_ADD_TO_FAVORITE:
                {
                    return run_favorite();
                }
                case ITEM_CHECKOUT:
                {
                    run_checkout();
                }
                case ITEM_DELETE:
                {
                    action = run_delete();
                    if (action)
                    {
                        action = OPTION_ALBUM_CHANGED;
                    }
                }
                case ITEM_INFORMATION:
                {
                    action = show_information();
                    if (action)
                    {
                        action = OPTION_RENAMED;
                    }
                }
                default:
                {
                    action = OPTION_CLOSE;
                }
            }
        }
        case MUSIC_ALBUM_CD:
        {
            switch (result)
            {
                case ITEM_CD_INFORMATION:
                {
                    show_information();
                }
                default:
                {
                    action = OPTION_CLOSE;
                }
            }
        }
        case MUSIC_ALBUM_PD:
        {
            switch (result)
            {
                case ITEM_PD_CHECKIN:
                {
                    action = run_checkin();
                    if (action)
                    {
                        action = OPTION_ALBUM_CHANGED;
                    }
                }
                case ITEM_PD_DELETE:
                {
                    action = run_delete();
                    if (action)
                    {
                        action = OPTION_ALBUM_CHANGED;
                    }
                }
                case ITEM_PD_INFORMATION:
                {
                    action = show_information();
                }
                default:
                {
                    action = OPTION_CLOSE;
                }
            }
        }
    }
    SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
    return action;
}

/* Runs the dialog of music_dialog that adds the selected track to a favorite album; returns its result. */
run_favorite()
{
    new args[64] = {0};
    sprintf(args, "channel_mode=%d;album_mode=%d;cache=%d", CHANNEL_TRACK, g_album_mode, g_favorites_cached);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 0.5 * g_screen_w, 0.5 * g_screen_h, 0.0, 0);
    set_aval_int("CheckoutTrack", g_select + 1);
    AttachScript(dialog, "favorite_main", "music_dialog", args);
    SetPubVar(dialog, "focus", focus_glowfly);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    new result = GetPubVar(dialog, "result");
    Destroy(dialog);
    g_favorites_cached = 1;
    return result;
}

/* Runs the check-in dialog of music_dialog for the selected track; returns its result. */
run_checkin()
{
    set_aval_int("MediaWatcherFlag", 1);
    new args[64] = {0};
    sprintf(args, "channel_mode=%d;album_mode=%d", CHANNEL_TRACK, g_album_mode);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 0.5 * g_screen_w, 0.5 * g_screen_h, 0.0, 0);
    set_aval_int("CheckoutTrack", g_select + 1);
    AttachScript(dialog, "checkin_main", "music_dialog", args);
    SetPubVar(dialog, "focus", focus_glowfly);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    new result = GetPubVar(dialog, "result");
    Destroy(dialog);
    set_aval_int("MediaWatcherFlag", 0);
    return result;
}

/* Runs the check-out dialog of music_dialog for the selected track; returns its result. */
run_checkout()
{
    new args[64] = {0};
    sprintf(args, "channel_mode=%d;album_mode=%d", CHANNEL_TRACK, g_album_mode);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 0.5 * g_screen_w, 0.5 * g_screen_h, 0.0, 0);
    set_aval_int("CheckoutTrack", g_select + 1);
    AttachScript(dialog, "checkout_main", "music_dialog", args);
    SetPubVar(dialog, "focus", focus_glowfly);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    new result = GetPubVar(dialog, "result");
    Destroy(dialog);
    return result;
}

/* Runs the delete dialog of music_dialog for the selected track; returns its result. */
run_delete()
{
    new args[64] = {0};
    sprintf(args, "channel_mode=%d;album_mode=%d", CHANNEL_TRACK, g_album_mode);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 0.5 * g_screen_w, 0.5 * g_screen_h, 0.0, 0);
    set_aval_int("CheckoutTrack", g_select + 1);
    AttachScript(dialog, "delete_main", "music_dialog", args);
    SetPubVar(dialog, "focus", focus_glowfly);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    new result = GetPubVar(dialog, "result");
    Destroy(dialog);
    return result;
}

/*
 * Keeps the values of the selected track in the aval file and opens the information page; if a name was changed
 * there, writes the song, album and artist names to the database. Returns 1 after a rename, -1 on an error.
 */
show_information()
{
    if (g_album_mode == MUSIC_ALBUM_COLLECTION || MUSIC_ALBUM_FAVORITE == g_album_mode)
    {
        playerExecCommand(PLAYER_READ_TRACK_INFO, 0, g_select + 1);
        new status = -1;
        new tries;
        while (status < 0)
        {
            if (0 == tries % 10)
            {
                playerExecCommand(PLAYER_GET_STATUS, status);
            }
            tries++;
            sleep;
            if (tries > 1800)
            {
                printf("============ break %d\n", tries);
                break;
            }
        }
        new mqt_size;
        playerExecCommand(PLAYER_GET_TRACK_INFO, g_checkout_times, g_bitrate, mqt_size);
        if (mqt_size < 1024)
        {
            mqt_size = 1024;
        }
        mqt_size = mqt_size / 1024;
        printf("checkouttimes = %d bitrate = %d\n", g_checkout_times, g_bitrate);
        set_aval_int("CheckoutTimes", g_checkout_times);
        set_aval_int("Bitrate", g_bitrate);
        set_aval_int("MQTDataSize", mqt_size);
    }
    else
    {
        set_aval_int("MusicAlbumMode", g_album_mode);
        set_aval_int("CheckoutTimes", -1);
        set_aval_int("Bitrate", g_bitrate);
    }
    if (g_album_mode == MUSIC_ALBUM_CD)
    {
        open_info_page("info", 1, 1);
        set_aval_int("MusicAlbumMode", g_page_mode);
        return 0;
    }
    if (g_album_mode == MUSIC_ALBUM_PD)
    {
        open_info_page("info", 1, 0);
        set_aval_int("MusicAlbumMode", g_page_mode);
        return 0;
    }
    if (MUSIC_ALBUM_FAVORITE == g_album_mode)
    {
        open_info_page("info", 2, 0);
    }
    else
    {
        open_info_page("info", 0, 0);
    }
    new edited = get_aval_int("MusicInfoEdit");
    if (0 == edited)
    {
        return 0;
    }
    new status2;
    new url[128];
    new escaped[128];
    new sql[384];
    open_db(0);
    if (g_album_mode == MUSIC_ALBUM_COLLECTION)
    {
        avalGetValue("ContentURI", 0, url, 128);
        printf("url = %s\n", url);
        printf("trackno = %d\n", g_select);
        avalGetValue("SongName", 0, g_song_name, 128);
        printf("songname = %s\n", g_song_name);
        if (0 == strlen(g_song_name))
        {
            get_tag_attr("TEXT", "msgmusic_tinfo_track_default", "value", escaped, 128);
            sprintf(g_song_name, "%s%d", escaped, g_select + 1);
            avalSet("SongName", g_song_name);
        }
        escape_sql(escaped, g_song_name);
        sprintf(sql, "UPDATE scei_jukebox SET songname = '%s', songname2 = '' WHERE url = '%s'", escaped, url);
        status2 = datSQLExec(sql);
        if (0 != status2)
        {
            return -1;
        }
        status2 = wait_database();
        if (status2 < 0)
        {
            return -1;
        }
        avalGetValue("ContentPath", 0, url, 128);
        printf("url = %s\n", url);
        avalGetValue("MusicAlbumName", 0, g_album_name, 128);
        printf("albumname = %s\n", g_album_name);
        if (0 == strlen(g_album_name))
        {
            get_tag_attr("TEXT", "msgmusic_ainfo_album_default", "value", escaped, 128);
            sprintf(g_album_name, "%s%d", escaped, g_album_no + 1);
            avalSet("MusicAlbumName", g_album_name);
        }
        escape_sql(escaped, g_album_name);
        sprintf(sql, "UPDATE scei_jukebox SET albumname = '%s', albumname2 = '' WHERE url LIKE '%s%s'", escaped, url,
                "%");
        status2 = datSQLExec(sql);
        if (0 != status2)
        {
            return -1;
        }
        status2 = wait_database();
        if (status2 < 0)
        {
            return -1;
        }
        avalGetValue("ArtistName", 0, g_artist, 128);
        printf("artistname = %s\n", g_artist);
        if (0 == strlen(g_artist))
        {
            get_tag_attr("TEXT", "msgmusic_ainfo_artist_default", "value", g_artist, 256);
            avalSet("ArtistName", g_artist);
        }
        escape_sql(escaped, g_artist);
        sprintf(sql, "UPDATE scei_jukebox SET artistname = '%s', artistname2 = '' WHERE url LIKE '%s%s'", escaped, url,
                "%");
        status2 = datSQLExec(sql);
        if (0 != status2)
        {
            return -1;
        }
        status2 = wait_database();
        if (status2 < 0)
        {
            return -1;
        }
    }
    else
    {
        printf("favorite = %d\n", g_album_no);
        avalGetValue("MusicAlbumName", 0, g_album_name, 128);
        printf("albumname = %s\n", g_album_name);
        if (0 == strlen(g_album_name))
        {
            get_tag_attr("TEXT", "msgmusic_optmenut_library_favorite", "value", escaped, 128);
            sprintf(g_album_name, "%s%d", escaped, g_album_no + 1);
            avalSet("MusicAlbumName", g_album_name);
        }
        escape_sql(escaped, g_album_name);
        sprintf(sql, "UPDATE scei_jukebox SET albumname = '%s', albumname2 = '' WHERE url = '%d'", escaped, g_album_no);
        status2 = datSQLExec(sql);
        if (0 != status2)
        {
            return -1;
        }
        status2 = wait_database();
        if (status2 < 0)
        {
            return -1;
        }
        status2 = datSQLExec("COMMIT");
        if (0 != status2)
        {
            return -1;
        }
        status2 = wait_database();
        if (status2 < 0)
        {
            return -1;
        }
        return 1;
    }
    new url_len = strlen(url);
    new pad_count = url_len % 4;
    for (new i; i < pad_count; i++)
    {
        append_string(url, " ", 128);
    }
    playerExecCommand(PLAYER_SET_ALBUM_PATH, url);
    return 1;
}

/* Plays track, or g_select if below 0, once the player has stopped. */
play_track(track)
{
    if (g_track_count <= 0 || g_list_only)
    {
        return;
    }
    new first;
    if (track < 0)
    {
        if (g_select < 0)
        {
            g_select = 0;
        }
        if (g_select > g_track_count - 1)
        {
            g_select = g_track_count - 1;
        }
        first = g_select;
    }
    else
    {
        if (track > g_track_count - 1)
        {
            track = g_track_count - 1;
        }
        first = track;
    }
    g_state = STATE_PLAYING;
    g_wait_track = first;
    printf("player_play():track %d track_old %d track_wait %d\n", g_track, g_last_track, g_wait_track);
    new unit;
    unit = g_album_mode == MUSIC_ALBUM_CD ? PLAYER_UNIT_CD : PLAYER_UNIT_HDD;
    playerExecCommand(PLAYER_STOP, unit, 0, 0);
    new state = 1;
    new tries;
    while (PLAYER_STATE_STOPPED != state)
    {
        playerExecCommand(PLAYER_GET_UNIT_STATE, unit, state, 0, 0);
        printf("WAIT %d\n", state);
        tries++;
        if (tries > 300)
        {
            return;
        }
        sleep;
    }
    playerExecCommand(PLAYER_PLAY, unit, first + 1, 0);
    fade_time_panel(1.0, 500.0);
    update_toolbar(TOOLBAR_PLAYER);
    SetSystemParam(SYS_ENABLE_SE, 0);
    SetSystemParam(SYS_ENABLE_BGM, 0);
    SetSystemParam(SYS_PADHELP, g_padhelp);
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
    SetPubVar(g_album, "padhelp", g_padhelp);
    SetPubVar(g_buttons, "padhelp", g_padhelp);
    SetPubVar(g_mode_buttons, "padhelp", g_padhelp);
}

/* Stops the player. */
stop_track()
{
    if (g_track_count <= 0 || g_list_only)
    {
        return;
    }
    g_state = STATE_STOPPED;
    g_track = -1;
    g_wait_track = -1;
    new unit;
    unit = g_album_mode == MUSIC_ALBUM_CD ? PLAYER_UNIT_CD : PLAYER_UNIT_HDD;
    playerExecCommand(PLAYER_STOP, unit, 0, 0);
    SendMessage(g_meter, TAG("PROG"), -1, 0, 0, 0);
    fade_time_panel(0.0, 1000.0);
    update_toolbar(TOOLBAR_PLAYER);
    SetSystemParam(SYS_ENABLE_SE, 1);
    SetSystemParam(SYS_ENABLE_BGM, 1);
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
    if (g_active == g_album)
    {
        SetSystemParam(SYS_PADHELP, g_padhelp | PADHELP_TRIANGLE);
    }
    else
    {
        SetSystemParam(SYS_PADHELP, g_padhelp);
    }
    SetPubVar(g_album, "padhelp", g_padhelp | PADHELP_TRIANGLE);
    SetPubVar(g_buttons, "padhelp", g_padhelp);
    SetPubVar(g_mode_buttons, "padhelp", g_padhelp);
    g_percent = 0;
}

/* Plays the next track, in the shuffle order if shuffle is on. */
next_track()
{
    if (g_track_count <= 0 || g_list_only)
    {
        return;
    }
    if (g_shuffle == 1)
    {
        g_shuffle_pos++;
        if (g_shuffle_pos >= g_track_count)
        {
            g_shuffle_pos--;
            printf("shuffle=on:ignored\n");
            return;
        }
        g_select = g_shuffle_list[g_shuffle_pos];
    }
    else
    {
        g_select = g_track + 1;
        if (g_select >= g_track_count)
        {
            printf("shuffle=off:ignored\n");
            return;
        }
    }
    play_track(-1);
    new unit;
    unit = g_album_mode == MUSIC_ALBUM_CD ? PLAYER_UNIT_CD : PLAYER_UNIT_HDD;
    update_toolbar(TOOLBAR_PLAYER);
    SendMessage(g_album, TAG("THRU"), -1, TAG("PAUS"), 0, 0);
}

/* Plays the previous track, or the current one again after its first two seconds. */
prev_track()
{
    if (g_track_count <= 0 || g_list_only)
    {
        return;
    }
    if (g_seconds <= 2)
    {
        if (g_shuffle == 1)
        {
            if (--g_shuffle_pos < 0)
            {
                g_shuffle_pos = 0;
            }
            g_select = g_shuffle_list[g_shuffle_pos];
            if (g_album_mode == MUSIC_ALBUM_CD)
            {
                g_last_track = -1;
            }
        }
        else
        {
            g_select = g_track - 1;
            if (g_album_mode == MUSIC_ALBUM_CD)
            {
                g_last_track = -1;
            }
        }
    }
    else
    {
        g_select = g_track;
        g_last_track = -1;
    }
    play_track(-1);
    new unit;
    unit = g_album_mode == MUSIC_ALBUM_CD ? PLAYER_UNIT_CD : PLAYER_UNIT_HDD;
    update_toolbar(TOOLBAR_PLAYER);
    SendMessage(g_album, TAG("THRU"), -1, TAG("PAUS"), 0, 0);
}

/* Starts fast forward; with brief, for half a second only. */
fast_forward(brief)
{
    if (g_track_count <= 0 || g_list_only)
    {
        return;
    }
    if (STATE_STOPPED == g_state)
    {
        play_track(-1);
    }
    g_state = STATE_FORWARD;
    new unit;
    unit = g_album_mode == MUSIC_ALBUM_CD ? PLAYER_UNIT_CD : PLAYER_UNIT_HDD;
    playerExecCommand(PLAYER_FORWARD, unit, 0, 0);
    if (brief)
    {
        wait_ms(500);
        resume_track();
    }
    update_toolbar(TOOLBAR_PLAYER);
}

/* Starts rewinding; with brief, for half a second only. */
rewind(brief)
{
    if (g_track_count <= 0 || g_list_only)
    {
        return;
    }
    if (STATE_STOPPED == g_state)
    {
        play_track(-1);
    }
    g_state = STATE_REWIND;
    new unit;
    unit = g_album_mode == MUSIC_ALBUM_CD ? PLAYER_UNIT_CD : PLAYER_UNIT_HDD;
    playerExecCommand(PLAYER_REWIND, unit, 0, 0);
    if (brief)
    {
        wait_ms(500);
        resume_track();
    }
    update_toolbar(TOOLBAR_PLAYER);
}

/* Pauses the track being played. */
pause_track()
{
    if (g_track_count <= 0 || g_list_only)
    {
        return;
    }
    g_state = STATE_PAUSED;
    new unit;
    unit = g_album_mode == MUSIC_ALBUM_CD ? PLAYER_UNIT_CD : PLAYER_UNIT_HDD;
    playerExecCommand(PLAYER_PAUSE, unit, 0, 0);
    update_toolbar(TOOLBAR_PLAYER);
    SendMessage(g_album, TAG("THRU"), -1, TAG("PAUS"), 1, 0);
}

/* Goes on with the track after a pause, a forward or a rewind; plays it when stopped. */
resume_track()
{
    if (g_track_count <= 0 || g_list_only)
    {
        return;
    }
    if (STATE_STOPPED == g_state)
    {
        play_track(-1);
    }
    new unit;
    unit = g_album_mode == MUSIC_ALBUM_CD ? PLAYER_UNIT_CD : PLAYER_UNIT_HDD;
    if (g_state == STATE_FORWARD || g_state == STATE_REWIND)
    {
        playerExecCommand(PLAYER_RESUME, unit, 0, 0);
    }
    else
    {
        playerExecCommand(PLAYER_PAUSE, unit, 0, 0);
    }
    g_state = STATE_PLAYING;
    update_toolbar(TOOLBAR_PLAYER);
    SendMessage(g_album, TAG("THRU"), -1, TAG("PAUS"), 0, 0);
}

/* Switches shuffle, with a new order of the tracks. */
toggle_shuffle()
{
    g_shuffle++;
    if (g_shuffle >= 2)
    {
        g_shuffle = 0;
    }
    if (g_shuffle > 0)
    {
        shuffle_tracks(g_track_count);
    }
    playerExecCommand(PLAYER_SET_PREFS, g_quality, g_repeat, g_shuffle);
    update_toolbar(TOOLBAR_MODES);
}

/* Repeat off, all the tracks, one track. */
cycle_repeat()
{
    g_repeat++;
    if (g_repeat >= 3)
    {
        g_repeat = REPEAT_OFF;
    }
    playerExecCommand(PLAYER_SET_PREFS, g_quality, g_repeat, g_shuffle);
    update_toolbar(TOOLBAR_MODES);
}

/* Stops the player and runs the ripping dialog of music_dialog; returns its result. */
run_ripping()
{
    if (g_track_count <= 0)
    {
        return 1;
    }
    if (STATE_STOPPED != g_state)
    {
        stop_track();
    }
    SendMessage(g_active, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
    g_state = STATE_RIPPING;
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 0.5 * g_screen_w, 0.5 * g_screen_h, 0.0, 0);
    AttachScript(dialog, "ripping_main", "music_dialog");
    SetPubVar(dialog, "focus", focus_glowfly);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    new result = GetPubVar(dialog, "result");
    Destroy(dialog);
    SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
    g_state = STATE_STOPPED;
    return result;
}

/* Creates the album: a general_menu of count tracks on a grid of four columns, item select selected. */
create_album(count, select)
{
    new album;
    album = CreateNull(Self());
    {
        new Float:x;
        new Float:y;
        new Float:z;
        map_screen_point(x, y, z, 0.25 * g_screen_w, 0.5 * g_screen_h, Self());
        SetPos(album, x, 10.0 + y, z - 75.0, 0);
        SetRot(album, -0.3, 0.7, 0.0, 0, 0);
        AttachScript(album, "general_menu", "general_menu");
        SetPubVar(album, "manager", Self());
        SetPubVar(album, "focus", focus_glowfly);
        SetPubVar(album, "padhelp", g_padhelp);
        SendMessage(album, TAG("INUM"), count, 0, 0, 0);
        SendMessage(album, TAG("CLRW"), 4, 4, 0, 0);
        SendMessage(album, TAG("MGIN"), 4, 4, 0, 0);
        SendMessage(album, TAG("MODE"), MENU_TYPE_PAGE, SCROLL_LINE, EXIT_RIGHT, 0);
        SendMessage(album, TAG("ANIM"), 200.0, 0, ANIM_FUNC_EXPONENT, 0);
        if (select >= 0)
        {
            if (select >= count)
            {
                select = count - 1;
            }
            if (select >= 0)
            {
                SendMessage(album, TAG("SLCT"), select, 0, 0, 0);
            }
        }
        SendMessage(album, TAG("OFFA"), 0, 0, 0, 0);
        new node_count = 24;
        for (new node; node < node_count; node++)
        {
            SendMessage(album, TAG("NPOS"), node, get_node_x(node, 4, node_count), get_node_y(node, 4, node_count),
                    get_node_z(node, 4, node_count));
        }
    }
    new url[256];
    get_tag_attr("IMG", "active", "src", url, 256);
    start_image_decode2(url);
    if (finish_image_decode() == IMG_DONE)
    {
        g_active_tex = imgTexID();
        imgKeepTex(g_active_tex);
    }
    return album;
}

/* Replaces album with the tracks of the album mode; returns their number, negative on an error. */
load_album(&album)
{
    SendMessage(album, TAG("FADE"), 0, 500.0, 0, 0);
    wait_ms(500);
    Destroy(album);
    album = 0;
    new count = query_album(g_album_mode);
    printf("new db opened %d = %d\n", g_album_mode, count);
    shuffle_tracks(count);
    album = create_album(count < 0 ? 0 : count, -1);
    return count;
}

/* Creates a music_thumb item of the album. */
create_album_item(path)
{
    new args[256];
    sprintf(args, "texture = %d ; active = %d ; paused = %d", g_active_tex, g_track, g_state == STATE_PAUSED);
    new item = CreateNull(path);
    AttachScript(item, _, "music_thumb", args);
    if (g_list_only)
    {
        SetPubVar(item, "pad_enable", PAD_CROSS);
    }
    else
    {
    }
    SetColor(item, 1.0, 1.0, 1.0, 0.0);
    return item;
}

/*
 * Creates a toolbar: TOOLBAR_PLAYER the player buttons (the rip button only on a CD), TOOLBAR_MODES shuffle and
 * repeat with their values. For the portable device, the update button instead.
 */
create_toolbar(which)
{
    if (g_list_only)
    {
        return create_update_button(which);
    }
    new count = 8;
    if (g_album_mode == MUSIC_ALBUM_COLLECTION || MUSIC_ALBUM_FAVORITE == g_album_mode)
    {
        count = 7;
    }
    new toolbar;
    toolbar = CreateNull(Disp2D(0));
    {
        new prev_url[64];
        new play_url[64];
        new next_url[64];
        new stop_url[64];
        new ff_url[64];
        new fr_url[64];
        new pause_url[64];
        new rec_url[64];
        new args[256];
        if (TOOLBAR_PLAYER == which)
        {
            get_tag_attr("IMG", "prev", "src", prev_url, 64);
            get_tag_attr("IMG", "play", "src", play_url, 64);
            get_tag_attr("IMG", "next", "src", next_url, 64);
            get_tag_attr("IMG", "stop", "src", stop_url, 64);
            get_tag_attr("IMG", "ff", "src", ff_url, 64);
            get_tag_attr("IMG", "fr", "src", fr_url, 64);
            get_tag_attr("IMG", "rec", "src", rec_url, 64);
            get_tag_attr("IMG", "pause", "src", pause_url, 64);
            SetPos(toolbar, 0.7625 * g_screen_w, 0.7458 * g_screen_h, 0.0, 0);
        }
        else
        {
            get_tag_attr("IMG", "shuffle", "src", play_url, 64);
            get_tag_attr("IMG", "repeat", "src", next_url, 64);
            SetPos(toolbar, 0.7625 * g_screen_w, 0.805 * g_screen_h, 0.0, 0);
        }
        SetColor(toolbar, 0.215, 0.708, 0.999, 0.0);
        SetOptionI(toolbar, TAG("ZSRT"), ZSORT_OVERLAY);
        if (which == TOOLBAR_MODES)
        {
            sprintf(args, "btn0=%s ; btn1=%s ; interval=65", play_url, next_url);
        }
        else if (count == 8)
        {
            sprintf(args,
                    "btn0=%s ; btn1=%s ; btn2=%s ; btn3=%s ; btn4=%s ; btn5=%s ; btn6=%s ; btn7=%s ; interval=%d ; interval3=32 ; interval4=27 ; interval5=22; interval6=24; repeat%d=1; repeat%d=1",
                    prev_url, next_url, fr_url, ff_url, play_url, pause_url, stop_url, rec_url, 39, BUTTON_REWIND,
                    BUTTON_FORWARD);
        }
        else
        {
            sprintf(args,
                    "btn0=%s ; btn1=%s ; btn2=%s ; btn3=%s ; btn4=%s ; btn5=%s ; btn6=%s ; interval=%d ; interval3=34 ; interval4=29 ; interval5=24; repeat%d=1; repeat%d=1",
                    prev_url, next_url, fr_url, ff_url, play_url, pause_url, stop_url, 39, BUTTON_REWIND,
                    BUTTON_FORWARD);
        }
        AttachScript(toolbar, _, "toolbar", args);
        SetPubVar(toolbar, "manager", Self());
        SetPubVar(toolbar, "focus", focus_glowfly);
        SetPubVar(toolbar, "alpha_blend_mode", BLEND_ADD);
        SetPubVar(toolbar, "padhelp", g_padhelp);
        SetPubVar(toolbar, "expand", 0);
    }
    if (which == TOOLBAR_MODES)
    {
        new Float:x = count == 3 ? 0.0 : float(-32); /* redundant as written */
        for (new i; i < 2; i++)
        {
            new label = CreateLabel(toolbar);
            g_mode_labels[i] = label;
            SetPos(label, x, 20.0, 0.0, 0);
            SetColor(label, 1.0, 1.0, 1.0, 0.0);
            AnimColor(label, 1.0, 1.0, 1.0, 1.0, 1000.0, 0.0, ANIM_FUNC_LINEAR);
            SetOptionI(label, TAG("FONT"), FontID(0));
            SetOptionI(label, TAG("VALN"), 0); /* no vertical alignment: the text sits on the baseline */
            SetOptionI(label, TAG("HALN"), ALIGN_CENTER);
            SetOptionI(label, TAG("FSIZ"), 10);
            x = 64 + x;
        }
    }
    update_toolbar(which);
    return toolbar;
}

/* Shows the values of shuffle and repeat (TOOLBAR_MODES); nothing for the player buttons. */
update_toolbar(which)
{
    if (g_list_only)
    {
        return;
    }
    if (TOOLBAR_PLAYER == which)
    {
        if (0 == g_buttons)
        {
            return;
        }
        new unused = 1; /* unused, but part of the layout */
        new playing = g_state == STATE_PLAYING ? 1 : 0; /* unused, but part of the layout */
    }
    else
    {
        SetOptionS(g_mode_labels[0], TAG("UTF8"), g_shuffle_labels[g_shuffle]);
        SetOptionS(g_mode_labels[1], TAG("UTF8"), g_repeat_labels[g_repeat]);
    }
}

/* Creates the update button of the portable device, hidden. */
create_update_button(which)
{
    new toolbar;
    toolbar = CreateNull(Disp2D(0));
    SetPos(toolbar, 0.65 * g_screen_w, 0.74 * g_screen_h, 0.0, 0);
    SetColor(toolbar, 0.215, 0.708, 0.999, 0.0);
    SetOptionI(toolbar, TAG("ZSRT"), ZSORT_OVERLAY);
    {
        new url[128];
        new args[256];
        get_tag_attr("IMG", "update", "src", url, 128);
        sprintf(args, "interval = 70 ; btn0 = %s", url);
        AttachScript(toolbar, _, "toolbar", args);
        SetPubVar(toolbar, "manager", Self());
        SetPubVar(toolbar, "focus", focus_glowfly);
        SetPubVar(toolbar, "alpha_blend_mode", BLEND_ADD);
        SetPubVar(toolbar, "padhelp", g_padhelp);
        SetPubVar(toolbar, "expand", 0);
    }
    HideChildren(toolbar);
    return toolbar;
}

/* Creates the jacket panel, tinted with the color of the favorite album. */
create_jacket_panel()
{
    new panel;
    {
        new Float:x;
        new Float:y;
        new Float:z;
        map_screen_point(x, y, z, 0.7625 * g_screen_w, -25.0 + 0.5 * g_screen_h, Scene(0));
        g_jacket_pos[0] = x;
        g_jacket_pos[1] = y;
        g_jacket_pos[2] = z;
        map_screen_point(x, y, z, 0.7625 * g_screen_w, 0.5 * g_screen_h, Scene(0));
        g_jacket_menu_pos[0] = x;
        g_jacket_menu_pos[1] = y;
        g_jacket_menu_pos[2] = z;
        panel = CreatePlane(Scene(0));
        SetPos(panel, g_jacket_pos[0], g_jacket_pos[1], g_jacket_pos[2], 0);
        SetColor(panel, 1.0, 1.0, 1.0, 0.0);
        SetOptionI(panel, TAG("FDEG"), 1);
        new color = -1;
        if (MUSIC_ALBUM_FAVORITE == g_album_mode)
        {
            color = g_album_no;
            if (color < 0)
            {
                color = 0;
            }
            if (color > 6)
            {
                color = 6;
            }
        }
        new args[64];
        sprintf(args, "color = %d", color);
        AttachScript(panel, "jacket_panel_main", _, args);
    }
    return panel;
}

/*
 * Runs on the jacket panel (argument color: a favorite color, -1 for none): shows the image of the url of "DCDE",
 * fitted in 256 pixels; "FADE" (alpha) fades it once shown.
 */
public jacket_panel_main()
{
    new color = -1;
    new is_tm2;
    new arg[64];
    if (GetArg("color", arg, 64))
    {
        color = atoi(arg);
    }
    new Float:red = color < 0 ? 1.0 : g_favorite_colors[color][0];
    new Float:green = color < 0 ? 1.0 : g_favorite_colors[color][1];
    new Float:blue = color < 0 ? 1.0 : g_favorite_colors[color][2];
    new self = Self();
    new shown;
    new Float:width = 1.0;
    new Float:height = 1.0;
    new Float:depth = 1.0;
    SetColor(self, red, green, blue, 0.0);
    for (; ; )
    {
        new msg;
        new Float:alpha;
        new b;
        new c;
        new url[256];
        while (RecvMessage(msg, alpha, b, c))
        {
            if (msg == TAG("FADE"))
            {
                if (shown)
                {
                    AnimColor(self, red, green, blue, alpha, 300.0, 0.0, ANIM_FUNC_LINEAR);
                }
            }
        }
        while (RecvMessageStr(msg, url, 256))
        {
            if (msg == TAG("DCDE"))
            {
                if (has_extension2(url, "tm2"))
                {
                    is_tm2 = 1;
                    start_image_decode2(url);
                }
                else
                {
                    is_tm2 = 0;
                    imgDecStart(url, floatround(256.0, 0), floatround(256.0, 0), 0, 100, 0);
                }
                shown = 0;
            }
        }
        new status = is_tm2 ? poll_image_decode() : imgDecSync();
        if (status == IMG_DONE)
        {
            SetTexture(self, imgTexID(), 0, -1);
            new Float:aspect = 1.0 * imgTexOrgWidth() / imgTexOrgHeight();
            if (aspect > 1.0)
            {
                width = 256.0;
                height = 256.0 / aspect;
            }
            else
            {
                width = 256.0 * aspect;
                height = 256.0;
            }
            depth = 256.0;
            shown = 1;
            AnimColor(self, red, green, blue, 1.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
            SetLocalScale(self, width, height, depth);
        }
        sleep;
    }
    return;
}

/*
 * Shows the names of the selected track, or of the one being played, in the box and keeps its values in the aval
 * file for the information page.
 */
update_track_info()
{
    g_song_name = "";
    printf("==================== jacket_panel_update %d trk = %d\n", g_select, g_track);
    if (g_track_count <= 0)
    {
        copy_string(g_song_name, "-", 256);
        copy_string(g_artist, "-", 256);
        goto show;
    }
    new value[128];
    new track = STATE_STOPPED == g_state || g_state == STATE_RIPPING ? g_select : g_track;
    printf("selected = %d\n", track);
    if (track >= g_track_count)
    {
        printf("not updated, state = %d\n", g_state);
        stop_track();
        return;
    }
    if (track < 0)
    {
        return;
    }
    if (MUSIC_ALBUM_FAVORITE == g_album_mode || g_album_mode == MUSIC_ALBUM_COLLECTION)
    {
        datGetData(track, TRACK_COL_NAME, g_song_name, 256);
        g_artist = "";
        datGetData(track, TRACK_COL_ARTIST, g_artist, 256);
        if (MUSIC_ALBUM_FAVORITE == g_album_mode)
        {
            new title[256];
            new album_name[256];
            datGetData(track, TRACK_COL_ALBUM, album_name, 256);
            sprintf(title, "%s / %s", g_song_name, album_name);
            avalSet("SongName", title);
            copy_string(g_song_name, title, 256);
        }
        else
        {
            avalSet("SongName", g_song_name);
        }
        avalSet("ArtistName", g_artist);
        datGetData(track, TRACK_COL_LENGTH, value, 128);
        avalSet("Length", value);
        datGetData(track, TRACK_COL_SIZE, value, 128);
        avalSet("DataSize", value);
        datGetData(track, TRACK_COL_FORMAT, value, 128);
        avalSet("Format", value);
        datGetData(track, TRACK_COL_URL, value, 128);
        avalSet("ContentURI", value);
    }
    else if (g_album_mode == MUSIC_ALBUM_CD)
    {
        get_tag_attr("TEXT", "msgmusic_tinfo_track_default", "value", value, 128);
        sprintf(g_song_name, "%s%d", value, track + 1 + g_track_offset);
        get_tag_attr("TEXT", "msgmusic_ainfo_artist_default", "value", g_artist, 256);
        avalSet("SongName", g_song_name);
        avalSet("ArtistName", g_artist);
        datGetData(track, TRACK_COL_SIZE, value, 128);
        new minutes;
        new seconds;
        new frames;
        playerExecCommand(PLAYER_CD_GET_TOC_TIME, track + g_track_offset, minutes, seconds, frames);
        new size_kb;
        size_kb = ((minutes * 60 + seconds) * 75 + frames) * 2352;
        size_kb = size_kb / 1024;
        if (minutes > 60)
        {
            frames = minutes / 60;
            minutes = minutes - frames * 60;
        }
        else
        {
            frames = 0;
        }
        sprintf(value, "%02d:%02d:%02d", frames, minutes, seconds);
        avalSet("Length", value);
        set_aval_int("DataSize", size_kb);
        avalSet("Format", "0");
        value = "    ";
        avalSet("ContentURI", value);
    }
    else
    {
        g_song_name = "";
        datGetData(track, TRACK_COL_NAME, g_song_name, 256);
        g_artist = "";
        datGetData(track, TRACK_COL_ARTIST, g_artist, 256);
        avalSet("SongName", g_song_name);
        avalSet("ArtistName", g_artist);
        datGetData(track, TRACK_COL_FORMAT, value, 128);
        avalSet("Format", value);
        datGetData(track, TRACK_COL_LENGTH, value, 128);
        avalSet("Length", value);
    }
    if (MUSIC_ALBUM_FAVORITE == g_album_mode)
    {
        datGetData(track, TRACK_COL_ENTRY_DATE, value, 128);
        avalSet("EntryDate", value);
    }
    show:
    set_box_header(0, g_song_name);
    set_box_header(2, g_album_name);
    set_box_header(1, g_artist);
    fade_box(1.0, 1000.0, 1.0, 1.0, 1.0);
}

/* Creates the time label and the time bar (music_meter) and the sound scopes. */
create_time_panel()
{
    new panel = CreateNull(Disp2D(0));
    SetPos(panel, 0.7625 * g_screen_w, 0.703125 * g_screen_h, 0.0, 0);
    new label = CreateLabel(panel);
    SetPos(label, -52.0, -18.0, 0.0, 0);
    SetColor(label, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(label, TAG("FONT"), FontID(0));
    SetOptionI(label, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(label, TAG("HALN"), ALIGN_LEFT);
    SetOptionI(label, TAG("FSIZ"), 12);
    new text[32] = "min1234567890sec";
    SetOptionS(label, TAG("UTF8"), text);
    sleep;
    text = "";
    SetOptionS(label, TAG("UTF8"), text);
    new meter = CreateNull(panel);
    SetPos(meter, -5.0, -4.0, 0.0, 0);
    SetOptionI(meter, TAG("ZSRT"), ZSORT_OVERLAY);
    AttachScript(meter, _, "music_meter");
    SetPubVar(meter, "length", 240.0);
    create_scopes();
    g_time_label = label;
    g_meter = meter;
    return panel;
}

/* Creates the sound scopes of the two channels, which draw g_scope_left and g_scope_right. */
create_scopes()
{
    new line_tex = load_icon_texture("wave_line");
    new base_tex = load_icon_texture("wave_base");
    for (new i; i < 2; i++)
    {
        new base = CreatePlane(Disp2D(0));
        g_scope_bases[i] = base;
        SetColor(base, 1.0, 1.0, 1.0, 0.0);
        SetTexture(base, base_tex, 0, -1);
        SetPos(base, 272.0 + i * 90, 388.0, 0.0, 0);
        SetScale(base, 88.0, 20.0, 1.0);
        SetOptionI(base, TAG("ZSRT"), ZSORT_OVERLAY);
        new scope = CreateSScope0(base);
        g_scopes[i] = scope;
        SetColor(scope, 1.0, 1.0, 1.0, 0.0);
        SetTexture(scope, line_tex, 0, -1);
        SetOptionI(scope, TAG("ALPH"), BLEND_ADD);
    }
    SetOptionP(g_scopes[0], TAG("SBF "), g_scope_left, 32);
    SetOptionP(g_scopes[1], TAG("SBF "), g_scope_right, 32);
}

new g_shown_min = -1;
new g_shown_sec = -1;
new g_shown_seconds;

/*
 * Shows the time played when it changes; a jump of more than two seconds shows the time closer to the last one shown.
 * Sends "PROG" (track, percent) to the time bar.
 */
update_time()
{
    if (g_time[1] != g_shown_sec || g_time[0] != g_shown_min)
    {
        g_shown_min = g_time[0];
        g_shown_sec = g_time[1];
        new text[32];
        new seconds = (g_time[0] / 16 * 10 + g_time[0] % 16) * 60 + (g_time[1] / 16 * 10 + g_time[1] % 16);
        new last_seconds = (g_last_time[0] / 16 * 10 + g_last_time[0] % 16) * 60
                + (g_last_time[1] / 16 * 10 + g_last_time[1] % 16);
        if (g_album_mode == MUSIC_ALBUM_CD && g_shuffle == 1)
        {
            if (seconds - last_seconds > 10)
            {
                if (g_time_resets < 20)
                {
                    g_time[0] = 0;
                    g_time[1] = 0;
                    g_time_resets++;
                }
            }
            else
            {
                g_time_resets = 0;
            }
        }
        if (abs(seconds - last_seconds) > 2 && seconds)
        {
            if (abs(seconds - g_shown_seconds) > abs(last_seconds - g_shown_seconds))
            {
                sprintf(text, "%02x min %02x sec", g_last_time, g_last_time[1]);
                g_shown_seconds = last_seconds;
            }
            else
            {
                sprintf(text, "%02x min %02x sec", g_time, g_time[1]);
                g_shown_seconds = seconds;
            }
        }
        else
        {
            sprintf(text, "%02x min %02x sec", g_time, g_time[1]);
            g_shown_seconds = seconds;
        }
        g_last_time[0] = g_time[0];
        g_last_time[1] = g_time[1];
        g_last_time[2] = g_time[2];
        g_seconds = (g_time[0] / 16 * 10 + g_time[0] % 16) * 60 + (g_time[1] / 16 * 10 + g_time[1] % 16);
        SetOptionS(g_time_label, TAG("UTF8"), text);
    }
    SendMessage(g_meter, TAG("PROG"), g_track, g_percent, 0, 0);
}

/* Lists the tracks of the album of mode: rows of the database, the tracks of the CD or of the portable device. */
query_album(mode)
{
    new count;
    new sql[512];
    new path[256];
    new status;
    switch (mode)
    {
        case MUSIC_ALBUM_COLLECTION:
        {
            open_db(0);
            avalGetValue("ContentPath", 0, path, 256);
            sprintf(sql,
                    "SELECT songname,artistname,trackno,length,datasize/1024,format,url FROM scei_jukebox WHERE format > -1 AND url LIKE '%s%s' ORDER BY trackno ASC",
                    path, "%");
            status = datSQLExec(sql);
            if (0 != status)
            {
                return -1;
            }
            status = wait_database();
            if (status < 0)
            {
                return -1;
            }
            count = datGetNum();
        }
        case MUSIC_ALBUM_FAVORITE:
        {
            playerExecCommand(PLAYER_SET_FAVORITE, g_album_no + 1);
            open_db(0);
            count = get_aval_int("MusicAlbumNo");
            sprintf(sql,
                    "SELECT songname,artistname,trackno,length,datasize/1024,format,scei_jukebox.url,scei_jukebox.albumname,entrydate FROM scei_jukebox,scei_jukebox_favorite_color WHERE scei_jukebox.url = scei_jukebox_favorite_color.url AND favorite = %d ORDER BY entrydate ASC",
                    count + 1);
            status = datSQLExec(sql);
            status = wait_database();
            count = datGetNum();
        }
        case MUSIC_ALBUM_CD:
        {
            g_track_offset = 0;
            g_disc_type = get_disc_type();
            if (CDVD_NO_DISC == g_disc_type)
            {
                count = 0;
            }
            else
            {
                count = -1;
                new tries;
                new last = -1;
                while (count == -1)
                {
                    playerExecCommand(PLAYER_CD_GET_TOC, count, last);
                    tries++;
                    if (tries >= 600)
                    {
                        break;
                    }
                    sleep;
                }
                if (count != last)
                {
                    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! %d %d %d\n", count, last, g_track_offset);
                    g_track_offset = 1;
                    count = last;
                }
            }
        }
        case MUSIC_ALBUM_PD:
        {
            show_album_error(-2);
            SystemCommand(SYSCMD_BUSY, 1);
            check_pd(0);
            SystemCommand(SYSCMD_BUSY, 0);
            count = get_aval_int("PDTracks");
            open_db(0);
            status = datSQLExec(
                    "SELECT songname,artistname,trackno,length,datasize/1024,format,url FROM scei_jukebox WHERE format <= -3 ORDER BY trackno ASC");
            status = wait_database();
        }
    }
    return count;
}

/* Shows why the album has no tracks, or hides the caption if it has. */
show_album_error(count)
{
    if (g_track_count > 0)
    {
        show_caption2(_, _, 0.0, 400.0, 0.0);
        return 0;
    }
    new line1[64] = {0};
    new line2[64] = {0};
    if (g_album_mode == MUSIC_ALBUM_COLLECTION || MUSIC_ALBUM_FAVORITE == g_album_mode)
    {
        if (0 == g_track_count)
        {
            get_tag_attr("TEXT", "msgmusic_track_none", "value", line1, 64);
        }
        else if (g_track_count < 0)
        {
            get_tag_attr("TEXT", "msgmusic_track_corrupt", "value", line1, 64);
        }
    }
    else if (g_album_mode == MUSIC_ALBUM_CD)
    {
        if (CDVD_NO_DISC == g_disc_type)
        {
            get_tag_attr("TEXT", "msgmusic_disc_none", "value", line1, 64);
        }
        else if (g_disc_type == CDVD_DETECTING)
        {
            get_tag_attr("TEXT", "msgmusic_disc_reading", "value", line1, 64);
        }
        else
        {
            get_tag_attr("TEXT", "msgmusic_disc_notreading", "value", line1, 64);
        }
    }
    else
    {
        new pd_tracks = get_aval_int("PDTracks");
        if (g_track_count == PD_NO_MEDIA)
        {
            get_tag_attr("TEXT", "msgmusic_usb_insert1", "value", line1, 64);
            get_tag_attr("TEXT", "msgmusic_usb_insert2", "value", line2, 64);
        }
        else if (0 == g_track_count)
        {
            get_tag_attr("TEXT", "msgmusic_track_none", "value", line1, 64);
        }
        else if (g_track_count == -2 || g_track_count == PD_CONNECTING || count == -2)
        {
            get_tag_attr("TEXT", "msgmusic_usb_connect_confirm", "value", line1, 64);
        }
        else
        {
            get_tag_attr("TEXT", "msgmusic_usb_nodevice", "value", line1, 64);
            get_tag_attr("TEXT", "msgmusic_usb_nodevice2", "value", line2, 64);
        }
    }
    set_caption_pos2(0.08125 * g_screen_w, 0.5 * g_screen_h);
    if (strlen(line2) > 0)
    {
        show_caption2(line1, line2, 1.0, 400.0, 0.0);
    }
    else
    {
        show_caption2(line1, _, 1.0, 400.0, 0.0);
    }
    return 1;
}

/* Fades the toolbars, and the time panel while playing, to alpha; hides the caption. */
fade_menus(Float:alpha, Float:duration)
{
    fade_toolbars(alpha, duration);
    new Float:box_alpha = g_track_count > 0 ? 0.0 : alpha;
    show_caption2(_, _, 0.0, 400.0, 0.0);
    if (g_state == STATE_PLAYING)
    {
        fade_time_panel(alpha, duration);
    }
}

/* Fades the toolbars and the values of shuffle and repeat to alpha. */
fade_toolbars(Float:alpha, Float:duration)
{
    SendMessage(g_buttons, TAG("FADE"), alpha, duration, 0, 0);
    SendMessage(g_mode_buttons, TAG("FADE"), alpha, duration, 0, 0);
    if (!g_list_only)
    {
        AnimColor(g_mode_labels[0], 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(g_mode_labels[1], 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    }
}

/* Fades the time label, the time bar and the sound scopes to alpha. */
fade_time_panel(Float:alpha, Float:duration)
{
    AnimColor(g_time_label, 1.0, 1.0, 1.0, 0.75 * alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    SendMessage(g_meter, TAG("FADE"), alpha, duration, 0, 0);
    for (new i; i < 2; i++)
    {
        if (g_scope_bases[i])
        {
            AnimColor(g_scope_bases[i], 1.0, 1.0, 1.0, 0.8 * alpha, duration, 0.0, ANIM_FUNC_LINEAR);
        }
        if (g_scopes[i])
        {
            AnimColor(g_scopes[i], 1.0, 1.0, 1.0, 0.2 * alpha, duration, 0.0, ANIM_FUNC_LINEAR);
        }
    }
}

/* Moves the camera to the left (dir 0, fading the box) or to the right with the box and the jacket panel. */
slide_camera(dir)
{
    if (0 == dir)
    {
        AnimMoveHermite(Camera(0), -280.0, 0.0, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
        AnimMoveHermite(camera_focus, -280.0, 0.0, -100.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
        fade_box(0.0, 500.0, 1.0, 1.0, 1.0);
    }
    else
    {
        AnimMoveHermite(Camera(0), 300.0, 0.0, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
        AnimMoveHermite(camera_focus, 300.0, 0.0, -100.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
        SetPos(g_focus_box, 0.7625 * g_screen_w, -25.0 + 0.4911 * g_screen_h, 0.0, 0);
        AnimMoveHermite(g_focus_box, 0.30625 * g_screen_w, 0.4911 * g_screen_h, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT,
                1.0);
        AnimMoveHermite(g_jacket, g_jacket_menu_pos[0], g_jacket_menu_pos[1], g_jacket_menu_pos[2], 750.0, 0.0,
                ANIM_FUNC_EXPONENT, 1.0);
    }
}

/* Moves the camera back to the center, and the box and the jacket panel back to the right if from_menu. */
center_camera(from_menu)
{
    AnimMoveHermite(Camera(0), 0.0, 0.0, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
    AnimMoveHermite(camera_focus, 0.0, 0.0, -100.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
    if (0 == from_menu)
    {
        fade_box(1.0, 2000.0, 1.0, 1.0, 1.0);
    }
    else
    {
        SetPos(g_focus_box, 0.30625 * g_screen_w, 0.4911 * g_screen_h, 0.0, 0);
        AnimMoveHermite(g_focus_box, 0.7625 * g_screen_w, -25.0 + 0.4911 * g_screen_h, 0.0, 750.0, 0.0,
                ANIM_FUNC_EXPONENT, 1.0);
        AnimMoveHermite(g_jacket, g_jacket_pos[0], g_jacket_pos[1], g_jacket_pos[2], 750.0, 0.0, ANIM_FUNC_EXPONENT,
                1.0);
    }
}

/* The script of the camera of the page: it keeps looking at camera_focus. */
public camera_main()
{
    for (; ; )
    {
        LookAt(Self(), camera_focus, 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
        sleep;
    }
}

/* Returns the type of the disc in the drive: no disc, detecting, CDVD_CDDA for an audio CD, else CDVD_ILLEGAL. */
get_disc_type()
{
    new type = GetSystemParam(SYS_CDVD_TYPE);
    if (CDVD_NO_DISC == type)
    {
        return CDVD_NO_DISC;
    }
    if (type == CDVD_UNKNOWN)
    {
        return CDVD_ILLEGAL;
    }
    if (type < CDVD_PS1_CD)
    {
        return CDVD_DETECTING;
    }
    /* NOTE: bug: "|| 19" is always true: any other disc is taken for an audio CD. */
    if (type == CDVD_CDDA || type == CDVD_PS1_CDDA || 19)
    {
        return CDVD_CDDA;
    }
    return CDVD_ILLEGAL;
}

/* Runs on a child of the page (argument "owner"): sends "DISC" (type) to it when the disc type changes. */
public media_watcher_main()
{
    new owner;
    new arg[64];
    if (GetArg("owner", arg, 64))
    {
        owner = atoi(arg);
    }
    new old_type = get_disc_type();
    playerExecCommand(PLAYER_CD_READ_TOC, 1);
    for (; ; )
    {
        new type = get_disc_type();
        if (type != old_type)
        {
            g_track_offset = 0;
            playerExecCommand(PLAYER_CD_READ_TOC, 1);
            SendMessage(owner, TAG("DISC"), type, 0, 0, 0);
            old_type = type;
        }
        wait_ms(500);
    }
    return;
}

/*
 * Asks the player for the state of the portable device (mode is passed on) and keeps PDTracks and PDType in the aval
 * file; returns the number of tracks, -1 if there is no device, the check times out or MusicCheckoutCancel is set.
 */
check_pd(mode)
{
    if (idx < 0)
    {
        g_pd_tracks = -1;
        return -1;
    }
    playerExecCommand(PLAYER_PD_CHECK, mode);
    new status = -1;
    new tries;
    while (status < 0)
    {
        if (0 == tries % 6)
        {
            playerExecCommand(PLAYER_GET_STATUS, status);
        }
        tries++;
        if (tries >= 3000)
        {
            break;
        }
        sleep;
    }
    new pd_type;
    if (tries < 3000)
    {
        playerExecCommand(PLAYER_PD_GET_INFO, g_pd_tracks, pd_type, 0);
        set_aval_int("PDTracks", g_pd_tracks);
        set_aval_int("PDType", pd_type);
        if (pd_type >= 0)
        {
            set_aval_int("PrevPDType", pd_type);
        }
    }
    else
    {
        g_pd_tracks = -1;
        set_aval_int("PDTracks", g_pd_tracks);
    }
    return g_pd_tracks;
}

/*
 * Runs on a child of the page (argument "owner"): while MediaWatcherFlag is 0, checks the portable device every second
 * and, when its tracks change, sends "PDEV" (tracks) to the owner and sets MusicMediaChanged to 2.
 */
public pd_watcher_main()
{
    new owner;
    new arg[64];
    if (GetArg("owner", arg, 64))
    {
        owner = atoi(arg);
    }
    new prev_tracks = check_pd(1);
    new count;
    new unused = -1; /* unused, but part of the layout */
    new paused;
    for (; ; )
    {
        paused = get_aval_int("MediaWatcherFlag");
        if (!paused)
        {
            check_pd(1);
            g_pd_tracks = get_aval_int("PDTracks");
            if (prev_tracks != g_pd_tracks)
            {
                printf("=== count = %d PD found %d trks\n", count, g_pd_tracks);
                printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! STATUS change\n");
                SendMessage(owner, TAG("PDEV"), g_pd_tracks, 0, 0, 0);
                set_aval_int("MusicMediaChanged", 2);
                prev_tracks = g_pd_tracks;
            }
        }
        wait_ms(1000);
    }
    return;
}

/* Decodes the image of IMG id and keeps its texture; returns it, 0 on failure. */
load_icon_texture(id[])
{
    new url[256];
    get_tag_attr("IMG", id, "src", url, 256);
    start_image_decode2(url);
    if (IMG_DONE != finish_image_decode())
    {
        return 0;
    }
    new texture = imgTexID();
    imgKeepTex(texture);
    return texture;
}

/* Makes a new random order of count tracks for shuffle (not for the portable device). */
shuffle_tracks(count)
{
    if (g_album_mode == MUSIC_ALBUM_PD)
    {
        return;
    }
    if (count <= 0)
    {
        count = 1;
    }
    for (new i; i < count; i++)
    {
        g_shuffle_order[i] = i;
    }
    for (new i; i < count - 1; i++)
    {
        new j = i + 1 + random(count - i - 1);
        new a = g_shuffle_order[i];
        new b = g_shuffle_order[j];
        g_shuffle_order[i] = b;
        g_shuffle_order[j] = a;
    }
    for (new i; i < count; i++)
    {
        new track = g_shuffle_order[i];
        new next = g_shuffle_order[(i + 1) % count];
        g_shuffle_list[track] = next;
    }
    g_shuffle_pos = -1;
}

/* Returns the x of node of the album grid: its column, the columns centered, 70.4 apart. */
Float:get_node_x(node, columns, count)
{
    new n = count;
    n = n * n;
    new Float:left = 1.1 * -(64.0 * float(columns - 1) / 2.0);
    return 1.1 * (64.0 * float(node % columns)) + left;
}

/*
 * Returns the y of node of the album grid: its row, the rows centered, 70.4 apart; the first and the last row are
 * pushed out by two rows more.
 */
Float:get_node_y(node, columns, count)
{
    new Float:top = 1.1 * (64.0 * float(count / columns - 1) / 2.0);
    new Float:offset;
    if (node < columns)
    {
        offset = 2.0 * (64.0 * 1.1);
    }
    if (node >= count - columns)
    {
        offset = 2.0 * (-64.0 * 1.1);
    }
    return top + offset - 1.1 * (64.0 * float(node / columns));
}

/* Returns the z of node of the album grid: 0. */
Float:get_node_z(node, columns, count)
{
    new n = node;
    n = columns;
    n = count;
    return 0.0 * n;
}

/* Same as has_extension of string.inc. */
has_extension2(name[], ext[])
{
    new len = strlen(name);
    new ext_len = strlen(ext);
    for (new i; i < ext_len; i++)
    {
        if (toupper(name{len - 1 - i}) != toupper(ext{ext_len - 1 - i}))
        {
            return 0;
        }
    }
    if ('.' != toupper(name{len - 1 - ext_len}))
    {
        return 0;
    }
    return 1;
}

/* Returns the result of a push of item index in an option menu: circle chooses it, cross goes back. */
handle_menu_push(manager, side, &mode, index, button, title[], group[], id[])
{
    if (button == TAG("RRGT"))
    {
        return index + MENU_ITEM;
    }
    if (button == TAG("RDWN"))
    {
        return MENU_BACK;
    }
    return MENU_REOPEN;
}

/* Returns the number of items an option menu has besides those of its MENUGROUP: none. */
count_extra_items(side, result, mode, title[], group[], id[], &selected)
{
}

/* Leaves the label of an option menu item as it is. */
fill_menu_item(side, mode, index, group[], label[])
{
}

/* Opens the music database once; with no_wait it does not wait for it. Returns its status, -1 on an error. */
open_db(no_wait)
{
    new status;
    if (0 == g_db_open)
    {
        status = datOpen(DAT_DATABASE, g_db_name, DAT_ORDER_MTIME_DESC);
        if (0 != status)
        {
            return -1;
        }
        if (!no_wait)
        {
            status = wait_database();
            if (status < 0)
            {
                return -1;
            }
        }
        g_db_open = 1;
    }
    return status;
}

/* Shows the warning that the player could not be initialized; the page then leaves for the top menu. */
show_unit_warning()
{
    new args[513];
    new line1[256];
    new line2[256];
    g_quit = 1;
    g_next_link = "top";
    if (MUSIC_ALBUM_FAVORITE == g_page_mode)
    {
        g_next_args = "channel_id = BNCH-MUSIC-FAVORITE ;";
    }
    else if (g_page_mode == MUSIC_ALBUM_COLLECTION)
    {
        g_next_args = "channel_id = BNCH-MUSIC-COLLECTION ;";
    }
    get_tag_attr("TEXT", "msgmusic_warn_unitwrong1", "value", line1, 256);
    get_tag_attr("TEXT", "msgmusic_warn_unitwrong2", "value", line2, 256);
    sprintf(args, "msg=%s\n%s", line1, line2);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 0.5 * g_screen_w, 0.5 * g_screen_h, 0.0, 0);
    AttachScript(dialog, _, "confirm_d", args);
    SetPubVar(dialog, "manager", Self());
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    Destroy(dialog);
}
