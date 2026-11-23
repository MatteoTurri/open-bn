/*
 * /opt0/bn/script/game/game.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The game channel: two radio buttons choose the album of the games installed on the hard disk or the one of the
 * disc in the drive; the album shows their icons and a box with the title of the selected one. Circle starts the
 * game, triangle opens its option menu (add to the navigation menu, delete, information, manual, PS1 settings).
 */

#include <core>
#include <bn>

#pragma dynamic 1280

/* Indexes of g_option_groups besides the albums. */
const OPTIONS_MANUAL = 4; /* a game of the hard disk with a manual */
const OPTIONS_PS1 = 5;    /* the PS1 settings */
const OPTIONS_DISC_SPEED = 6;
const OPTIONS_TEXTURE_MAPPING = 7;

/* PS1DiscSpeed and PS1TextureMapping (aval), the options of the PS1 driver, in the order of their menu items. */
const DISC_SPEED_STANDARD = 0;
const DISC_SPEED_FAST = 1;
const TEXTURE_STANDARD = 0;
const TEXTURE_SMOOTH = 1;

/* The items of the option menu of a game of the hard disk (optmenu4) and of the PS1 settings (optmenu_ps1drv). */
const ITEM_ADD_TO_NAVIMENU = 0;
const ITEM_DELETE = 1;
const ITEM_INFORMATION = 2;
const ITEM_MANUAL = 3;
const ITEM_DISC_SPEED = 0;
const ITEM_TEXTURE_MAPPING = 1;

#include "gamedb.inc"

#include "devimage.inc"

#include "stdmenu.inc"
#include "genmenu.inc"
#include "scroll.inc"

forward Float:get_node_x(node);
forward Float:get_node_y(node);
forward Float:get_node_z(node);

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

#include "text.inc"

#include "imgdec.inc"

#include "misc.inc"

new g_db_name[] = "game";
new g_shadow_colors[4] = {0x5F000000, 0x5F000000, 0x5F000000, 0x5F000000}; /* the "SCL4" of the shadowed labels */

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
    get_screen_size3(screen_w, screen_h);
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
    get_screen_size3(screen_w, screen_h);
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

new g_focus_box; /* the box beside the album with the title and the number of the selected game */

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

set_box_footer(text[])
{
    if (g_focus_box)
    {
        SendMessageStr(g_focus_box, TAG("FOOT"), text);
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

/* Fades the header of the box to alpha and its color to red, green, blue. */
fade_box_header(Float:alpha, Float:duration, Float:red, Float:green, Float:blue)
{
    if (g_focus_box)
    {
        SendMessage(g_focus_box, TAG("FAD1"), alpha, duration, 0, 0);
        SendMessage(g_focus_box, TAG("FCL1"), red, green, blue, duration);
    }
}

/* Fades the footer of the box to alpha and its color to red, green, blue. */
fade_box_footer(Float:alpha, Float:duration, Float:red, Float:green, Float:blue)
{
    if (g_focus_box)
    {
        SendMessage(g_focus_box, TAG("FAD2"), alpha, duration, 0, 0);
        SendMessage(g_focus_box, TAG("FCL2"), red, green, blue, duration);
    }
}

set_box_focus(focus_obj)
{
    if (g_focus_box)
    {
        SetPubVar(g_focus_box, "focus", focus_obj);
    }
}

/* Moves the focus onto slot of the box. */
focus_box_slot(slot)
{
    if (g_focus_box)
    {
        SendMessage(g_focus_box, TAG("FOCS"), slot, 0, 0, 0);
    }
}

unfocus_box()
{
    if (g_focus_box)
    {
        SendMessage(g_focus_box, TAG("FOUT"), 0, 0, 0, 0);
    }
}

set_box_title(text[])
{
    set_box_header(0, text);
}

/* Shows text, the position of the save in the album, in the footer of the box. */
set_box_counter(text[])
{
    set_box_footer(text);
}

/* Shows the image url as the violence icon of the box; an empty url hides it. */
set_box_violence_icon(url[])
{
    if (g_focus_box)
    {
        SendMessageStr(g_focus_box, TAG("VICN"), url);
    }
}

/* Shows the image url as the copyright image of the box; an empty url hides it. */
set_box_copyright(url[])
{
    if (g_focus_box)
    {
        SendMessageStr(g_focus_box, TAG("CPYI"), url);
    }
}

new g_device_image;
new Float:g_device_x = 130.0;
new Float:g_device_y = 350.0;
new Float:g_device_w = 512.0;
new Float:g_device_h = 512.0;

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

new g_caption_labels2[2][2]; /* two pairs of lines, shown in turn */
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
        get_screen_size3(screen_w, screen_h);
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

/* The option menus: 0 of a game of the HDD album, 4 of one with a manual, 5 to 7 of the PS1 settings. */
new g_option_groups[8][] = {
    "optmenu1", "optmenu2", "optmenu3", "optmenu_sort", "optmenu4", "optmenu_ps1drv",
    "optmenu_discspd", "optmenu_texmap"
};
new g_album_device[2] = {DEVICE_HDD, DEVICE_DISC};

/* Same as set_caption_pos of misc.inc, for show_caption2. */
set_caption_pos2(Float:x, Float:y)
{
    g_caption_x2 = x;
    g_caption_y2 = y;
}

/* Loads the page of "LINK" link_id, or the built-in top menu with args for "top". */
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
        xmlLoadStart(url, _, _, XMLLOAD_MAIN);
    }
    wait_xml_load();
}

/* Same as apply_rot13 of misc.inc. */
apply_rot13_2(text[])
{
    for (new i; text{i}; i++)
    {
        if ('a' <= text{i} <= 'z')
        {
            text{i} = (text{i} - 'a' + 13) % 26 + 'a';
        }
        else if ('A' <= text{i} <= 'Z')
        {
            text{i} = (text{i} - 'A' + 13) % 26 + 'A';
        }
    }
}

/* Same as encode_base64 of misc.inc. */
encode_base64_2(data[], len, out[], out_max)
{
    out_max = out_max * 4;
    new out_len;
    for (new i; i < len; i = i + 3)
    {
        new pad;
        new chunk[1];
        chunk{0} = data{i};
        if (i + 1 < len)
        {
            chunk{1} = data{i + 1};
        }
        else
        {
            pad++;
        }
        if (i + 2 < len)
        {
            chunk{2} = data{i + 2};
        }
        else
        {
            pad++;
        }
        for (new j; j < 4 - pad; j++)
        {
            out{out_len} = get_base64_char2(chunk[0] >> 8 >> (3 - j) * 6 & 0x3F);
            out_len++;
        }
        for (new k; k < pad; k++)
        {
            out{out_len} = '=';
            out_len++;
        }
    }
    return out_len;
}

/* Same as get_base64_char of dom.inc. */
get_base64_char2(value)
{
    if (0 <= value <= 25)
    {
        return value - 0 + 'A';
    }
    if (26 <= value <= 51)
    {
        return value - 26 + 'a';
    }
    if (52 <= value <= 61)
    {
        return value - 52 + '0';
    }
    if (value == 62)
    {
        return '+';
    }
    if (value == 63)
    {
        return '/';
    }
    return 0;
}

/* Same as write_current_channel of sysconf.inc. */
write_current_channel(channel[])
{
    new tries;
    new ret;
    avalDelete("PS1DiscSpeed");
    avalDelete("PS1TextureMapping");
    if (0 == strcmp(channel, "Top"))
    {
        avalSet("ChannelIntoID", "PSBB-GUIDE");
        avalSet("ChannelCategory", "GROUP-TOPMENU");
    }
    avalSet("CurrentCh", channel);
    tries = 0;
    for (; tries < 3; tries++)
    {
        ret = avalWriteFile();
        if (ret >= 0)
        {
            break;
        }
    }
    return ret;
}

new Float:g_screen_w;
new Float:g_screen_h;
public camera_focus; /* the point the camera looks at */
public camera_uptgt; /* unused, but part of the layout */
public focus_glowfly; /* the focus object */
new g_active; /* g_mode_buttons or g_album: the one that takes the pad */
new g_mode_buttons;
new g_album;
new g_default_icon;
new g_broken_icon;
new g_next_link[64]; /* the page to leave for: a "LINK" id or "top" */
new g_next_args[64];
new g_quit;
new g_disc_type; /* CDVD_* */
new g_album_mode; /* ALBUM_* */
new g_album_count;
new g_album_idle; /* the album has stopped moving */
new g_enter_album; /* the focus goes into the album once it is idle */
new g_args[128];
new g_msg[128];
/* Lines of TEXT elements joined into g_msg; g_line3 and g_line4 are also the answers of a notice. */
new g_line1[32];
new g_line2[32];
new g_line3[32];
new g_line4[32];
new g_album_busy = 1; /* the album is moving: circle and triangle wait */
new g_sort_order; /* SORT_* */
new g_sql[128];
new g_ps1_disc_speed; /* DISC_SPEED_* */
new g_ps1_texture_mapping; /* TEXTURE_* */
new g_album_select; /* the selected game, kept across the pages in the aval file; -1 for none */
new g_import_count; /* the import of the games has run this many times */

main()
{
    init_page();
    while (1 != g_quit)
    {
        handle_messages();
        sleep;
    }
    SetSystemParam(SYS_PADHELP, 0);
    close_db();
    fade_background(0, 0, 600.0);
    load_page(g_next_link, g_next_args);
}

/* Builds the page and restores the album and the game of the last visit. */
init_page()
{
    new value[4];
    start_top_page(0);
    load_common_bg("bg", 1000.0);
    set_background_poor(600.0, 0);
    get_screen_size3(g_screen_w, g_screen_h);
    create_camera();
    link_bg_camera(2.0, 0);
    reset_bg_camera();
    g_next_link = "top";
    g_next_args = "";
    g_quit = 0;
    g_album_count = 0;
    g_import_count = 0;
    g_disc_type = CDVD_NO_DISC;
    g_album_idle = 0;
    g_enter_album = 0;
    sprintf(g_args, "owner = %d", Self());
    {
        new watcher = CreateNull(Self());
        AttachScript(watcher, "media_watcher_main", _, g_args);
    }
    g_sort_order = SORT_NEWEST;
    g_album_mode = ALBUM_HDD;
    if (0 == avalGetValue("GameAlbumMode", 0, value, 4))
    {
        g_album_mode = atoi(value);
        if (ALBUM_HDD != g_album_mode && ALBUM_DISC != g_album_mode)
        {
            g_album_mode = ALBUM_HDD;
        }
        avalDelete("GameAlbumMode");
    }
    g_album_select = -1;
    if (0 == avalGetValue("GameAlbumCurNo", 0, value, 4))
    {
        g_album_select = atoi(value);
        if (g_album_select < 0)
        {
            g_album_select = -1;
        }
        avalDelete("GameAlbumCurNo");
    }
    g_ps1_disc_speed = DISC_SPEED_STANDARD;
    if (0 == avalGetValue("PS1DiscSpeed", 0, value, 4))
    {
        g_ps1_disc_speed = atoi(value);
        if (DISC_SPEED_STANDARD != g_ps1_disc_speed && DISC_SPEED_FAST != g_ps1_disc_speed)
        {
            g_ps1_disc_speed = DISC_SPEED_STANDARD;
        }
    }
    else
    {
        sprintf(value, "%d", g_ps1_disc_speed);
        avalSet("PS1DiscSpeed", value);
    }
    g_ps1_texture_mapping = TEXTURE_STANDARD;
    if (0 == avalGetValue("PS1TextureMapping", 0, value, 4))
    {
        g_ps1_texture_mapping = atoi(value);
        if (TEXTURE_STANDARD != g_ps1_texture_mapping && TEXTURE_SMOOTH != g_ps1_texture_mapping)
        {
            g_ps1_texture_mapping = TEXTURE_STANDARD;
        }
    }
    else
    {
        sprintf(value, "%d", g_ps1_texture_mapping);
        avalSet("PS1TextureMapping", value);
    }
    g_default_icon = load_icon_texture("default_icon");
    g_broken_icon = load_icon_texture("broken_icon");
    focus_glowfly = create_focus_plane2("focus");
    create_device_image();
    show_device_image(g_album_device[g_album_mode]);
    create_focus_box(0.7625 * g_screen_w, 0.5 * g_screen_h);
    fade_box(0.0, 0.0, 1.0, 1.0, 1.0);
    set_box_focus(focus_glowfly);
    set_box_title("");
    create_captions(0);
    set_caption_pos2(0.546875 * g_screen_w, 0.5 * g_screen_h);
    g_mode_buttons = create_mode_buttons();
    SetPubVar(g_mode_buttons, "select", g_album_mode);
    SetPubVar(g_mode_buttons, "cursor", g_album_mode);
    new enter;
    if (g_album_select < 0)
    {
        enter = 0;
    }
    else
    {
        enter = 1;
    }
    g_active = g_mode_buttons;
    g_album = 0;
    new mode = g_album_mode;
    g_album_mode--; /* differs from the mode, so that change_album reloads the album */
    change_album(mode, 0, enter);
}

close_db()
{
    datClose();
}

/*
 * Handles the messages of one frame: "CHNG" (album) and "FOCS", "PUSH" (_, button) of the radio buttons or of the
 * album, whichever takes the pad; "ITEM" (menu, path, index) of the album; "DISC" (type) of the disc watcher.
 */
handle_messages()
{
    new msg;
    new a;
    new b;
    new c;
    while (RecvMessage(msg, a, b, c))
    {
        if (g_active == g_mode_buttons)
        {
            if (msg == TAG("CHNG"))
            {
                change_album(a, 1, 1);
            }
            if (msg == TAG("FOCS"))
            {
                on_buttons_focus(a);
            }
            if (msg == TAG("PUSH"))
            {
                on_buttons_push(b);
            }
        }
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
        if (msg == TAG("ITEM"))
        {
            send_album_item(a, b, c, g_album_count);
        }
        if (msg == TAG("PUSH"))
        {
            on_push(b);
        }
        if (msg == TAG("DISC"))
        {
            on_disc_change(a);
        }
    }
    update_album_info();
    update_buttons_pad_help();
    return 1;
}

/* From the album, gives the pad back to the radio buttons; from the radio buttons, leaves for the top page. */
go_back()
{
    if (g_active == g_album)
    {
        sndSePlay(SE_CANCEL);
        SendMessage(g_album, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
        unfocus_box();
        dim_album();
        sleep;
        sleep;
        g_active = g_mode_buttons;
        light_buttons();
        SendMessage(g_mode_buttons, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
        g_enter_album = 0;
    }
    else
    {
        g_quit = 1;
        g_next_link = "top";
        g_next_args = "channel_id = BNCH-GAME-COLLECTION ;";
        sndSePlay(SE_CANCEL);
        return;
    }
}

/* The cross goes back to the page before. */
on_push(button)
{
    if (button == TAG("RDWN"))
    {
        go_back();
        return;
    }
}

/* Reloads the disc album when the disc changes, leaving it if there is no disc any more. */
on_disc_change(type)
{
    g_disc_type = type;
    if (ALBUM_DISC != g_album_mode)
    {
        return;
    }
    if (g_disc_type <= CDVD_DETECTING)
    {
        if (g_active == g_album)
        {
            go_back();
        }
        g_album_mode = ALBUM_HDD;
        change_album(ALBUM_DISC, 0, 0);
    }
    else
    {
        g_album_mode = ALBUM_HDD;
        change_album(ALBUM_DISC, 1, 1);
    }
}

/* Shows album mode, loading it if it is another one; with enter the focus goes into it. */
change_album(mode, with_se, enter)
{
    switch (mode)
    {
        case ALBUM_HDD:
        {
            SetPubVar(g_mode_buttons, "padhelp", PADHELP_CIRCLE | PADHELP_CROSS | PADHELP_TRIANGLE);
        }
        case ALBUM_DISC:
        {
            SetPubVar(g_mode_buttons, "padhelp", PADHELP_CIRCLE | PADHELP_CROSS);
        }
    }
    if (with_se)
    {
        sndSePlay(SE_DECIDE);
    }
    send_focus2(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    SendMessage(g_mode_buttons, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
    dim_buttons();
    SendMessage(g_active, TAG("CHNG"), 0, 0, 0, 0);
    SetPubVar(g_mode_buttons, "disable_selected", 1);
    if (g_album_mode != mode)
    {
        g_album_mode = mode;
        show_device_image(g_album_device[g_album_mode]);
        if (g_album_select < 0)
        {
            g_album_count = load_album(g_album, g_album_mode, 0);
        }
        else
        {
            g_album_count = load_album(g_album, g_album_mode, g_album_select);
        }
        if (enter)
        {
            g_enter_album = 1;
        }
    }
    else if (g_album_count > 0 && enter)
    {
        g_active = g_album;
        focus_box_slot(0);
        SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        light_album();
        g_enter_album = 1;
    }
    if (g_album_count <= 0)
    {
        g_album_count = 0;
        g_active = g_mode_buttons;
        SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        light_buttons();
        g_enter_album = 0;
    }
    else if (g_album_count > 0 && !enter)
    {
        SendMessage(g_album, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
        unfocus_box();
        dim_album();
        g_enter_album = 0;
        sleep;
        sleep;
        g_active = g_mode_buttons;
        light_buttons();
        SendMessage(g_mode_buttons, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
    }
    SetPubVar(g_mode_buttons, "disable_selected", 0);
}

/* The focus stays on g_active whichever way (direction, a TAG) the pad leaves it. */
on_buttons_focus(direction)
{
    new value = direction; /* redundant as written */
    direction = value;
    SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
}

/* Triangle on the disc album with a PS1 disc in the drive opens the PS1 settings. */
on_buttons_push(button)
{
    new cursor = GetPubVar(g_mode_buttons, "cursor");
    if (cursor == ALBUM_DISC && button == TAG("RUP"))
    {
        new type = get_cdvd_type2();
        if (CDVD_PS1_CD != type && CDVD_PS1_CDDA != type)
        {
            SendMessage(g_active, TAG("PUSH"), 0, 0, 0, 0);
            return;
        }
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
        SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
        sndSePlay(SE_OPEN);
        sndSePlay(SE_WINDOW_OPEN);
        SendMessage(g_active, TAG("PUSH"), TAG("RUP"), 0, 0, 0);
        SendMessage(g_active, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
        send_focus2(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        if (g_active == g_album)
        {
            unfocus_box();
        }
        SendMessage(g_mode_buttons, TAG("FADE"), 0, 400.0, 0, 0);
        SetPos(g_focus_box, 0.7625 * g_screen_w, 0.5 * g_screen_h, 0.0, 0);
        AnimMoveHermite(g_focus_box, 0.7625 * g_screen_w - 300.0, 0.5 * g_screen_h, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT,
                1.0);
        fade_box_footer(0.0, 400.0, 0.7843, 0.7843, 0.7843);
        show_caption2(_, _, 0.0, 400.0, 0.0);
        slide_camera(1);
        wait_ms(floatround(750.0, 0));
        new changed;
        new result;
        for (; ; )
        {
            result = pop_std_menu("optmenu", g_option_groups[OPTIONS_PS1], MENU_SIDE_RIGHT, result - MENU_ITEM);
            if (result == MENU_CLOSED)
            {
                break;
            }
            changed = changed | run_ps1_option(result);
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
            SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
        }
        sndSePlay(SE_WINDOW_CLOSE);
        center_camera();
        SetPos(g_focus_box, 0.7625 * g_screen_w - 300.0, 0.5 * g_screen_h, 0.0, 0);
        AnimMoveHermite(g_focus_box, 0.7625 * g_screen_w, 0.5 * g_screen_h, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
        wait_ms(floatround(750.0, 0));
        fade_box_footer(1.0, 400.0, 0.7843, 0.7843, 0.7843);
        SendMessage(g_mode_buttons, TAG("FADE"), 1.0, 400.0, 0, 0);
        if (!changed)
        {
            if (g_album_count > 0)
            {
                dim_album();
            }
            else
            {
                show_album_error(g_album_count);
            }
            dim_buttons();
            if (g_active == g_album)
            {
                focus_box_slot(0);
            }
            SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        }
        else
        {
            new mode = g_album_mode;
            g_album_mode--; /* differs from the mode, so that change_album reloads the album */
            change_album(mode, 0, 1);
        }
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
        return;
    }
    SendMessage(g_active, TAG("PUSH"), 0, 0, 0, 0);
}

/* Sets the pad help of the radio buttons: triangle also for a PS1 disc. */
update_buttons_pad_help()
{
    if (g_enter_album == 1)
    {
        return;
    }
    new cursor = GetPubVar(g_mode_buttons, "cursor");
    if (g_active == g_mode_buttons)
    {
        if (ALBUM_HDD == cursor)
        {
            switch (g_album_mode)
            {
                case ALBUM_HDD:
                {
                    SetPubVar(g_mode_buttons, "padhelp", PADHELP_CIRCLE | PADHELP_CROSS);
                }
                case ALBUM_DISC:
                {
                    SetPubVar(g_mode_buttons, "padhelp", PADHELP_CIRCLE | PADHELP_CROSS);
                }
                default:
                {
                    SetPubVar(g_mode_buttons, "padhelp", PADHELP_CIRCLE | PADHELP_CROSS);
                }
            }
        }
        else if (cursor == ALBUM_DISC)
        {
            switch (g_album_mode)
            {
                case ALBUM_HDD:
                {
                    switch (get_cdvd_type2())
                    {
                        case CDVD_PS1_CD:
                        {
                            SetPubVar(g_mode_buttons, "padhelp", PADHELP_CIRCLE | PADHELP_CROSS | PADHELP_TRIANGLE);
                        }
                        case CDVD_PS1_CDDA:
                        {
                            SetPubVar(g_mode_buttons, "padhelp", PADHELP_CIRCLE | PADHELP_CROSS | PADHELP_TRIANGLE);
                        }
                        default:
                        {
                            SetPubVar(g_mode_buttons, "padhelp", PADHELP_CIRCLE | PADHELP_CROSS);
                        }
                    }
                }
                case ALBUM_DISC:
                {
                    switch (get_cdvd_type2())
                    {
                        case CDVD_PS1_CD:
                        {
                            SetPubVar(g_mode_buttons, "padhelp", PADHELP_CIRCLE | PADHELP_CROSS | PADHELP_TRIANGLE);
                        }
                        case CDVD_PS1_CDDA:
                        {
                            SetPubVar(g_mode_buttons, "padhelp", PADHELP_CIRCLE | PADHELP_CROSS | PADHELP_TRIANGLE);
                        }
                        default:
                        {
                            SetPubVar(g_mode_buttons, "padhelp", PADHELP_CIRCLE | PADHELP_CROSS);
                        }
                    }
                }
                default:
                {
                    SetPubVar(g_mode_buttons, "padhelp", PADHELP_CIRCLE | PADHELP_CROSS);
                }
            }
        }
        SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
    }
}

/* Creates the album item of index and sends it to the album ("ITEM"). */
send_album_item(menu, path, index, count)
{
    if (IsValidObject(path))
    {
        new item = create_album_item(path, index, count);
        SetOptionF(item, TAG("ZOFS"), -50.0);
        SendMessage(menu, TAG("ITEM"), item, path, 0, 0);
    }
}

/* The focus stays on g_active whichever way (direction, a TAG) the pad leaves it. */
on_album_focus(direction)
{
    new value = direction; /* redundant as written */
    direction = value;
    SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
}

/* Circle starts the selected game (after its notice) or the disc; triangle opens the option menu of the game. */
on_album_push(button)
{
    if (button == TAG("RRGT") && 0 == g_album_busy)
    {
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
        SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
        sndSePlay(SE_DECIDE);
        SendMessage(g_active, TAG("PUSH"), TAG("RR"), 0, 0, 0);
        SendMessage(g_active, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
        send_focus2(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        if (g_active == g_album)
        {
            unfocus_box();
        }
        if (g_album_mode == ALBUM_DISC)
        {
            wait_ms(200);
            err11012();
            boot_disc();
            err11013();
        }
        else
        {
            new title[64] = {0};
            new select = GetPubVar(g_album, "select");
            datGetData(select, COL_TITLE, title, 64);
            if (strlen(title) <= 0)
            {
                get_tag_attr("TEXT", "msggame_execdisc_dataerr1", "value", g_line1, 32);
                get_tag_attr("TEXT", "msggame_execdisc_dataerr2", "value", g_line3, 32);
                sprintf(g_msg, "%s\n%s", g_line1, g_line3);
                show_message(g_msg);
            }
            else
            {
                new notice[64] = {0};
                new mode;
                new size;
                new answer;
                datGetData(select, COL_RESOURCE_PATH, title, 64);
                sprintf(notice, "%s/notice.png", title);
                if (0 == fileLstat(notice, mode, size))
                {
                    get_tag_attr("TEXT", "msggame_notice_play", "value", g_line3, 32);
                    get_tag_attr("TEXT", "msggame_notice_notplay", "value", g_line4, 32);
                    answer = ask_yes_no_notice(notice, g_line3, g_line4);
                }
                if (0 == answer)
                {
                    wait_ms(200);
                    err11012();
                    boot_selected_game();
                    err11013();
                }
            }
        }
        if (g_active == g_album)
        {
            focus_box_slot(0);
        }
        SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
        return;
    }
    if (button == TAG("RUP") && 0 == g_album_busy)
    {
        if (ALBUM_HDD == g_album_mode)
        {
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
            SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
            sndSePlay(SE_OPEN);
            sndSePlay(SE_WINDOW_OPEN);
            SendMessage(g_active, TAG("PUSH"), TAG("RUP"), 0, 0, 0);
            SendMessage(g_active, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
            send_focus2(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            if (g_active == g_album)
            {
                unfocus_box();
            }
            SendMessage(g_mode_buttons, TAG("FADE"), 0, 400.0, 0, 0);
            SetPos(g_focus_box, 0.7625 * g_screen_w, 0.5 * g_screen_h, 0.0, 0);
            AnimMoveHermite(g_focus_box, 0.7625 * g_screen_w - 300.0, 0.5 * g_screen_h, 0.0, 750.0, 0.0,
                    ANIM_FUNC_EXPONENT, 1.0);
            fade_box_footer(0.0, 400.0, 0.7843, 0.7843, 0.7843);
            slide_camera(1);
            wait_ms(floatround(750.0, 0));
            new manual_offset;
            if (ALBUM_HDD == g_album_mode)
            {
                new path[256];
                new select = GetPubVar(g_album, "select");
                path = "";
                datGetData(select, COL_TITLE, path, 256);
                if (strlen(path) > 0)
                {
                    datGetData(select, COL_RESOURCE_PATH, path, 256);
                    append_string(path, "/man.xml", 256);
                    new mode;
                    new size;
                    if (0 == fileLstat(path, mode, size))
                    {
                        manual_offset = OPTIONS_MANUAL;
                    }
                }
            }
            new changed;
            new result;
            for (; ; )
            {
                result = pop_std_menu("optmenu", g_option_groups[g_album_mode + manual_offset], MENU_SIDE_RIGHT,
                        result - MENU_ITEM);
                if (result == MENU_CLOSED)
                {
                    break;
                }
                changed = changed | run_game_option(result);
                if (changed == 1)
                {
                    break;
                }
                SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
                SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
            }
            sndSePlay(SE_WINDOW_CLOSE);
            center_camera();
            SetPos(g_focus_box, 0.7625 * g_screen_w - 300.0, 0.5 * g_screen_h, 0.0, 0);
            AnimMoveHermite(g_focus_box, 0.7625 * g_screen_w, 0.5 * g_screen_h, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT,
                    1.0);
            wait_ms(floatround(750.0, 0));
            fade_box_footer(1.0, 400.0, 0.7843, 0.7843, 0.7843);
            SendMessage(g_mode_buttons, TAG("FADE"), 1.0, 400.0, 0, 0);
            if (!changed)
            {
                light_album();
                dim_buttons();
                if (g_active == g_album)
                {
                    focus_box_slot(0);
                }
                SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
            }
            else
            {
                go_back();
                new album_mode = g_album_mode;
                g_album_mode--; /* differs from the mode, so that change_album reloads the album */
                change_album(album_mode, 0, 1);
            }
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
            SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
            return;
        }
    }
    SendMessage(g_active, TAG("PUSH"), 0, 0, 0, 0);
}

/* Shows the title, the number and the icons of the selected game in the box when the album stops; sets the pad help. */
update_album_info()
{
    if (g_album_count <= 0 || 0 == g_album)
    {
        return;
    }
    new old_idle = g_album_idle;
    new select = GetPubVar(g_album, "select");
    new idle = GetPubVar(g_album, "idle");
    g_album_idle = idle == 1;
    g_album_select = select;
    if (g_album_idle != old_idle)
    {
        if (g_album_idle)
        {
            new text[256] = {0};
            if (ALBUM_HDD == g_album_mode)
            {
                datGetData(select, COL_TITLE, text, 256);
                if (strlen(text) <= 0)
                {
                    get_tag_attr("TEXT", "msggame_info_corrupted", "value", text, 256);
                    set_box_header(0, text);
                    sprintf(text, "%d/%d", select + 1, g_album_count);
                    set_box_counter(text);
                    set_box_violence_icon("");
                    set_box_copyright("");
                }
                else
                {
                    set_box_header(0, text);
                    sprintf(text, "%d/%d", select + 1, g_album_count);
                    set_box_counter(text);
                    text = "";
                    datGetData(select, COL_VIOLENCE_FLAG, text, 256);
                    if (atoi(text) == 1)
                    {
                        get_tag_attr("IMG", "violence_icon", "src", text, 256);
                        set_box_violence_icon(text);
                    }
                    else
                    {
                        set_box_violence_icon("");
                    }
                    datGetData(select, COL_COPYRIGHT_IMGCOUNT, text, 256);
                    if (atoi(text) == 1)
                    {
                        text = "";
                        datGetData(select, COL_RESOURCE_PATH, text, 256);
                        append_string(text, "/jkt_cp.png", 256);
                        set_box_copyright(text);
                    }
                    else
                    {
                        set_box_copyright("");
                    }
                }
            }
            else
            {
                if (g_disc_type == CDVD_PS1_CD)
                {
                    get_tag_attr("TEXT", "msggame_chtop_ps1disc", "value", text, 256);
                }
                else if (g_disc_type == CDVD_PS1_CDDA)
                {
                    get_tag_attr("TEXT", "msggame_chtop_ps1disc", "value", text, 256);
                }
                else if (g_disc_type == CDVD_PS2_CD)
                {
                    get_tag_attr("TEXT", "msggame_chtop_ps2disc", "value", text, 256);
                }
                else if (g_disc_type == CDVD_PS2_CDDA)
                {
                    get_tag_attr("TEXT", "msggame_chtop_ps2disc", "value", text, 256);
                }
                else if (g_disc_type == CDVD_PS2_DVD)
                {
                    get_tag_attr("TEXT", "msggame_chtop_ps2disc", "value", text, 256);
                }
                set_box_header(0, text);
                set_box_counter("1/1");
            }
            if (g_enter_album)
            {
                g_active = g_album;
                focus_box_slot(0);
                light_album();
                SendMessage(g_album, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
            }
            if (g_active == g_album)
            {
                focus_box_slot(0);
            }
            g_album_busy = 0;
        }
        else
        {
            g_album_busy = 1;
            if (g_active == g_album)
            {
                unfocus_box();
            }
            fade_box(0.0, 200.0, 1.0, 1.0, 1.0);
            set_box_violence_icon("");
            set_box_copyright("");
        }
    }
    if (0 == g_album_busy)
    {
        if (g_album_mode == ALBUM_DISC)
        {
            SetSystemParam(SYS_PADHELP, PADHELP_CIRCLE | PADHELP_CROSS);
        }
        else
        {
            SetSystemParam(SYS_PADHELP, PADHELP_CIRCLE | PADHELP_CROSS | PADHELP_TRIANGLE);
        }
    }
    else
    {
        SetSystemParam(SYS_PADHELP, PADHELP_CROSS);
    }
}

/* Runs the PS1 setting chosen in the menu; returns 0. */
run_ps1_option(result)
{
    result = result - MENU_ITEM;
    new changed;
    switch (result)
    {
        case ITEM_DISC_SPEED:
        {
            changed = choose_disc_speed();
        }
        case ITEM_TEXTURE_MAPPING:
        {
            changed = choose_texture_mapping();
        }
    }
    return changed;
}

/* Runs the option of the HDD album chosen in the menu; returns 1 when the game was deleted. */
run_game_option(result)
{
    result = result - MENU_ITEM;
    switch (g_album_mode)
    {
        case ALBUM_HDD:
        {
            switch (result)
            {
                case ITEM_ADD_TO_NAVIMENU:
                {
                    return add_to_navimenu();
                }
                case ITEM_DELETE:
                {
                    return delete_game();
                }
                case ITEM_INFORMATION:
                {
                    show_information();
                }
                case ITEM_MANUAL:
                {
                    show_manual();
                }
            }
        }
    }
    return 0;
}

/* Asks for the texture mapping of the PS1 driver and saves it in the aval file as PS1TextureMapping. */
choose_texture_mapping()
{
    new result;
    result = pop_std_menu("optmenu", g_option_groups[OPTIONS_TEXTURE_MAPPING], MENU_SIDE_RIGHT, g_ps1_texture_mapping);
    if (result == MENU_CLOSED)
    {
    }
    else
    {
        new value[4];
        switch (result - MENU_ITEM)
        {
            case TEXTURE_STANDARD:
            {
                g_ps1_texture_mapping = TEXTURE_STANDARD;
                sprintf(value, "%d", g_ps1_texture_mapping);
                avalSet("PS1TextureMapping", value);
            }
            case TEXTURE_SMOOTH:
            {
                g_ps1_texture_mapping = TEXTURE_SMOOTH;
                sprintf(value, "%d", g_ps1_texture_mapping);
                avalSet("PS1TextureMapping", value);
            }
        }
    }
    return;
}

/* Asks for the disc speed of the PS1 driver and saves it in the aval file as PS1DiscSpeed. */
choose_disc_speed()
{
    new result;
    result = pop_std_menu("optmenu", g_option_groups[OPTIONS_DISC_SPEED], MENU_SIDE_RIGHT, g_ps1_disc_speed);
    if (result == MENU_CLOSED)
    {
    }
    else
    {
        new value[4];
        switch (result - MENU_ITEM)
        {
            case DISC_SPEED_STANDARD:
            {
                g_ps1_disc_speed = DISC_SPEED_STANDARD;
                sprintf(value, "%d", g_ps1_disc_speed);
                avalSet("PS1DiscSpeed", value);
            }
            case DISC_SPEED_FAST:
            {
                g_ps1_disc_speed = DISC_SPEED_FAST;
                sprintf(value, "%d", g_ps1_disc_speed);
                avalSet("PS1DiscSpeed", value);
            }
        }
    }
    return;
}

/* Shows the manual (man.xml) of the selected game with the help page. */
show_manual()
{
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
    SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
    new path[256];
    new select = GetPubVar(g_album, "select");
    datGetData(select, COL_RESOURCE_PATH, path, 256);
    append_string(path, "/man.xml", 256);
    new mode;
    new size;
    if (0 == fileLstat(path, mode, size))
    {
        new args[260];
        snprintf(args, 260, "url=%s", path);
        xmlLoadStart("../help/help.xml", _, args, XMLLOAD_MODAL);
        wait_xml_load();
    }
    else
    {
    }
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
    SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
}

/* Shows the information page (infomenu) of the selected game. */
show_information()
{
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
    SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
    new args[128];
    new url[128];
    new select = GetPubVar(g_album, "select");
    sprintf(args, "select = %d", select);
    get_tag_attr("LINK", "infomenu", "href", url, 128);
    xmlLoadStart(url, _, args, XMLLOAD_MODAL);
    wait_xml_load();
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
    SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
}

/* Deletes the selected game with game_dialog; returns its result, 1 if it was deleted. */
delete_game()
{
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
    SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    new select = GetPubVar(g_album, "select");
    new title[64];
    datGetData(select, COL_TITLE, title, 64);
    new partition[64];
    datGetData(select, COL_URI, partition, 64);
    sprintf(g_args, "url = %s ; title = %s", partition, title);
    AttachScript(dialog, "delete_main", "game_dialog", g_args);
    SetPubVar(dialog, "focus", focus_glowfly);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    new result = GetPubVar(dialog, "result");
    Destroy(dialog);
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
    SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
    return result;
}

/* Adds the selected game to the navigation menu with game_dialog, if it can be started from there. */
add_to_navimenu()
{
    new title[64];
    new partition[64];
    new boot[64];
    new type;
    new result;
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
    SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
    new select = GetPubVar(g_album, "select");
    datGetData(select, COL_TITLE, title, 64);
    datGetData(select, COL_URI, partition, 64);
    if (strlen(title) <= 0)
    {
        get_tag_attr("TEXT", "msggame_navimenu_adddata_prohibit", "value", g_line1, 32);
        sprintf(g_msg, "%s", g_line1);
        show_message(g_msg);
    }
    else
    {
        type = err10026(partition, boot, boot);
        if (type < 0)
        {
            get_tag_attr("TEXT", "msggame_execdisc_dataerr1", "value", g_line1, 32);
            get_tag_attr("TEXT", "msggame_execdisc_dataerr2", "value", g_line3, 32);
            sprintf(g_msg, "%s\n%s", g_line1, g_line3);
            show_message(g_msg);
        }
        else if (type >= BOOT2_CDROM && type <= BOOT2_PATINFO)
        {
            sprintf(g_args, "url = %s ; title = %s", partition, title);
            new dialog = CreateNull(Disp2D(0));
            SetPos(dialog, 320.0, 224.0, 0.0, 0);
            AttachScript(dialog, "add_navimenu_main", "game_dialog", g_args);
            SetPubVar(dialog, "focus", focus_glowfly);
            while (SCRIPT_FINISHED != ScriptState(dialog))
            {
                sleep;
            }
            result = GetPubVar(dialog, "result");
            Destroy(dialog);
        }
        else if (type == BOOT2_NOBOOT)
        {
            get_tag_attr("TEXT", "msggame_navimenu_add_prohibit", "value", g_line1, 32);
            sprintf(g_msg, "%s", g_line1);
            show_message(g_msg);
        }
        else
        {
            get_tag_attr("TEXT", "msggame_navimenu_add_fail", "value", g_line1, 32);
            sprintf(g_msg, "%s", g_line1);
            show_message(g_msg);
        }
    }
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
    SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
    return result;
}

create_mode_buttons()
{
    new buttons;
    buttons = CreateNull(Disp2D(0));
    new label0[64];
    new label1[64];
    get_group_tag_attr2("TEXT", "albummenu", 0, "value", label0, 64);
    get_group_tag_attr2("TEXT", "albummenu", 1, "value", label1, 64);
    new count = 2;
    sprintf(g_args, "btn0 = %s ; btn1 = %s; interval = %d", label0, label1, floatround(0.08 * g_screen_h, 0));
    AttachScript(buttons, _, "radio_button_v", g_args);
    new Float:y = 0.5 - 0.5 * (0.08 * float(count - 1));
    SetPos(buttons, 0.14 * g_screen_w, g_screen_h * y, 0.0, 0);
    SetColor(buttons, 0.215, 0.708, 0.999, 0.0);
    SetOptionI(buttons, TAG("ZSRT"), ZSORT_OVERLAY);
    SetPubVar(buttons, "manager", Self());
    SetPubVar(buttons, "focus", focus_glowfly);
    SetPubVar(buttons, "padhelp", PADHELP_CIRCLE | PADHELP_CROSS);
    SetPubVar(buttons, "disable_selected", 0);
    SetPubVar(buttons, "se_decide", -1);
    return buttons;
}

/* Creates the album: a general_menu of count items on the curve of get_node_x/y/z, item select in front. */
create_album(count, select)
{
    new album;
    album = CreateNull(Self());
    new Float:x;
    new Float:y;
    new Float:z;
    map_screen_point(x, y, z, 0.7625 * g_screen_w, 0.5 * g_screen_h, Parent(0));
    SetPos(album, x, y, z, 0);
    AttachScript(album, "general_menu", "general_menu");
    SetPubVar(album, "manager", Self());
    SetPubVar(album, "focus", focus_glowfly);
    SetPubVar(album, "pad_enable", PAD_CROSS);
    SendMessage(album, TAG("INUM"), count, 0, 0, 0);
    SendMessage(album, TAG("CLRW"), 1, 1, 0, 0);
    SendMessage(album, TAG("PADL"), 0, 0, 1, -1);
    SendMessage(album, TAG("MGIN"), 1, 7, 0, 0);
    SendMessage(album, TAG("MODE"), MENU_TYPE_PAGE, SCROLL_ALONG_CURVE, 0, 0);
    SendMessage(album, TAG("ANIM"), 250.0, 0, ANIM_FUNC_EXPONENT, 0);
    SendMessage(album, TAG("REPT"), 50.01, 83.34, 0, 0);
    SendMessage(album, TAG("SLCT"), select, 0, 0, 0);
    for (new node; node < 9; node++)
    {
        SendMessage(album, TAG("NPOS"), node, get_node_x(node - 1), get_node_y(node - 1), get_node_z(node - 1));
    }
    if (0 == count)
    {
        SetSystemParam(SYS_PADHELP, PADHELP_CROSS);
    }
    return album;
}

/* Replaces album with the one of mode, item select selected; returns its number of items, negative on an error. */
load_album(&album, mode, select)
{
    SetSystemParam(SYS_PADHELP, 0);
    show_caption2(_, _, 0.0, 400.0, 0.0);
    if (g_active == album)
    {
        unfocus_box();
    }
    fade_box(0.0, 1000.0, 1.0, 1.0, 1.0);
    if (album)
    {
        SendMessage(album, TAG("FADE"), 0, 1000.0, 0, 0);
    }
    if (album)
    {
        SendMessage(album, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
    }
    if (ALBUM_DISC != g_album_mode)
    {
        AnimTimerManager(Self(), 1000.0, 0.0, ANIM_FUNC_LINEAR);
        wait_ms(200);
        new text[64];
        get_tag_attr("TEXT", "msggame_reading_data", "value", text, 64);
        show_caption2(text, _, 1.0, 400.0, 0.0);
        SetSystemParam(SYS_PADHELP, 0);
        wait_ms(400.0);
    }
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
    SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
    new count = query_album(mode);
    SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
    while (ANIM_DONE != AnimGetState(Self(), ANIM_TIMER))
    {
        sleep;
    }
    if (album)
    {
        Destroy(album);
    }
    album = 0;
    if (count > 0)
    {
        if (select >= count)
        {
            select = count - 1;
        }
        album = create_album(count, select);
        show_caption2(_, _, 0.0, 400.0, 0.0);
    }
    else if (0 == count)
    {
        show_caption2(_, _, 0.0, 400.0, 0.0);
        wait_ms(200);
        show_album_error(count);
    }
    else
    {
        show_caption2(_, _, 0.0, 400.0, 0.0);
        wait_ms(200);
    }
    return count;
}

/* Creates the album item of game index, with its jacket image (jkt_001.png) or the default or broken icon. */
create_album_item(path, index, count)
{
    new item = CreatePlane(path);
    AttachScript(item, "album_item", "album_item");
    if (ALBUM_HDD == g_album_mode)
    {
        SetPubVar(item, "pad_enable", PAD_CIRCLE | PAD_TRIANGLE);
    }
    else
    {
        SetPubVar(item, "pad_enable", PAD_CIRCLE);
    }
    SetOptionI(item, TAG("FDEG"), 1);
    SetColor(item, 1.0, 1.0, 1.0, 0.0);
    if (index >= 0 && index < count)
    {
        if (ALBUM_HDD == g_album_mode)
        {
            new icon_path[256];
            new title[256];
            new jackets[16];
            datGetData(index, COL_IMAGE_COUNT, jackets, 16);
            jackets = "1";
            datGetData(index, COL_RESOURCE_PATH, icon_path, 256);
            datGetData(index, COL_TITLE, title, 256);
            if (strlen(title) <= 0)
            {
                SendMessage(item, TAG("TEX"), g_broken_icon, 1.0, 0, 0);
            }
            else if (atoi(jackets) >= 1 && atoi(jackets) <= 999)
            {
                append_string(icon_path, "/jkt_001.png", 256);
                new mode;
                new size;
                if (0 == fileLstat(icon_path, mode, size))
                {
                    SendMessageStr(item, TAG("DCDE"), icon_path);
                }
                else
                {
                    SendMessage(item, TAG("TEX"), g_default_icon, 1.0, 0, 0);
                }
            }
            else
            {
                SendMessage(item, TAG("TEX"), g_default_icon, 1.0, 0, 0);
            }
        }
        else
        {
            SendMessage(item, TAG("TEX"), g_default_icon, 1.0, 0, 0);
        }
    }
    return item;
}

/* Shows why the album is empty (count 0) or could not be read. */
show_album_error(count)
{
    new line1[32] = {0};
    new line2[32] = {0};
    if (ALBUM_HDD == g_album_mode)
    {
        if (0 == count)
        {
            get_tag_attr("TEXT", "msggame_error_nodata", "value", line1, 32);
        }
        else if (count < 0)
        {
            get_tag_attr("TEXT", "msggame_exechdd_fail_broken", "value", line1, 32);
        }
    }
    else if (CDVD_NO_DISC == g_disc_type)
    {
        get_tag_attr("TEXT", "msggame_error_nodisc", "value", line1, 32);
    }
    else if (g_disc_type == CDVD_DETECTING)
    {
        get_tag_attr("TEXT", "msggame_reading_disc", "value", line1, 32);
    }
    else
    {
        get_tag_attr("TEXT", "msggame_error_notplaydisc", "value", line1, 32);
    }
    set_caption_pos2(0.546875 * g_screen_w, 0.5 * g_screen_h);
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

/*
 * Lists the games of album mode: for the HDD album runs the import of the games (cross cancels it the first time)
 * and queries the game database, adding the row of the sort order if it is missing; for the disc album checks for a
 * PlayStation disc. Returns the number of games, -1 if the import was cancelled.
 */
query_album(mode)
{
    new status;
    switch (mode)
    {
        case ALBUM_HDD:
        {
            if (0 == g_import_count)
            {
                SetSystemParam(SYS_PADHELP, PADHELP_CROSS);
                sleep;
                datClose();
                err10027();
                while (IMPORT_DONE != err10028())
                {
                    new held;
                    new pressed;
                    PadState(held, pressed, _, _, -1);
                    if (held & PAD_CROSS)
                    {
                        SendMessage(Self(), TAG("PUSH"), 0, TAG("RDWN"), 0, 0);
                        err10033();
                        for (; ; )
                        {
                            new state;
                            state = err10028();
                            if (IMPORT_DONE == state || state == IMPORT_CANCELLED)
                            {
                                break;
                            }
                            sleep;
                        }
                        return -1;
                    }
                    sleep;
                }
                g_import_count++;
                SetSystemParam(SYS_PADHELP, 0);
                sleep;
            }
            else
            {
                datClose();
                err10027();
                while (IMPORT_DONE != err10028())
                {
                    sleep;
                }
            }
            status = datOpen(DAT_DATABASE, g_db_name, DAT_ORDER_MTIME_DESC);
            if (0 != status)
            {
                return 0;
            }
            status = wait_database();
            if (status < 0)
            {
                return 0;
            }
            sprintf(g_sql, "select * from sce_game where content_attr = %d", ATTR_SETTINGS);
            status = datSQLExec(g_sql);
            if (0 != status)
            {
                return 0;
            }
            status = wait_database();
            if (status < 0)
            {
                return 0;
            }
            if (datGetNum() <= 0)
            {
                sprintf(g_sql,
                        "insert into sce_game values(%d,%d,%d,'%s','%s','%s','%s','%s','%s','%s','%s','%s',%d,%d,'%s','%s',%d,%d,%d,%d,%d,%d,'%s',%d,'%s','%s','%s',%d,%d,%d,%d)",
                        0, 0, 0, "", "", "", "", "", "", "", "", "", 512, 0, "", "", 0, 0, 0, 0, 0, 0, "", 0, "", "",
                        "", 0, g_sort_order, 0, ATTR_SETTINGS);
                status = datSQLExec(g_sql);
                if (0 != status)
                {
                    return 0;
                }
                status = wait_database();
                if (status < 0)
                {
                    return 0;
                }
            }
            else
            {
                datGetData(0, COL_CONTENT_TYPE, g_sql, 128);
                g_sort_order = atoi(g_sql);
                if (g_sort_order < 0 || g_sort_order > 1)
                {
                    g_sort_order = SORT_NEWEST;
                }
            }
            if (SORT_NEWEST == g_sort_order)
            {
                g_sql = "select * from sce_game where content_attr = 0 order by install_date desc";
            }
            else
            {
                g_sql = "select * from sce_game where content_attr = 0 order by install_date asc";
            }
            status = datSQLExec(g_sql);
            if (0 != status)
            {
                return 0;
            }
            status = wait_database();
            if (status < 0)
            {
                return 0;
            }
            status = datGetNum();
            if (status < 0)
            {
                status = 0;
            }
            return status;
        }
        case ALBUM_DISC:
        {
            g_disc_type = get_cdvd_type2();
            if (g_disc_type == CDVD_PS1_CD)
            {
                return 1;
            }
            if (g_disc_type == CDVD_PS1_CDDA)
            {
                return 1;
            }
            if (g_disc_type == CDVD_PS2_CD)
            {
                return 1;
            }
            if (g_disc_type == CDVD_PS2_CDDA)
            {
                return 1;
            }
            if (g_disc_type == CDVD_PS2_DVD)
            {
                return 1;
            }
            return 0;
        }
    }
    return 0;
}

/* Lights the radio buttons: blue and opaque. */
light_buttons()
{
    if (!g_mode_buttons)
    {
        return;
    }
    SendMessage(g_mode_buttons, TAG("FCOL"), 0.215, 0.708, 0.999, 0);
    SendMessage(g_mode_buttons, TAG("FADE"), 1.0, 400.0, 0, 0);
}

/* Dims the radio buttons: grey and half transparent. */
dim_buttons()
{
    if (!g_mode_buttons)
    {
        return;
    }
    SendMessage(g_mode_buttons, TAG("FCOL"), 0.7843, 0.7843, 0.7843, 0);
    SendMessage(g_mode_buttons, TAG("FADE"), 0.5, 400.0, 0, 0);
}

/* Lights the album and the box. */
light_album()
{
    fade_box(1.0, 400.0, 1.0, 1.0, 1.0);
    fade_box_header(1.0, 400.0, 0.215, 0.708, 0.999);
    if (!g_album)
    {
        return;
    }
    SendMessage(g_album, TAG("FADE"), 1.0, 400.0, 0, 0);
}

/* Dims the album and the box to a quarter of their alpha. */
dim_album()
{
    fade_box(0.25, 400.0, 1.0, 1.0, 1.0);
    fade_box_header(1.0, 400.0, 1.0, 1.0, 1.0);
    if (!g_album)
    {
        return;
    }
    SendMessage(g_album, TAG("FADE"), 0.25, 400.0, 0, 0);
}

/* Moves the camera to the left (dir 0) or to the right for a menu. */
slide_camera(dir)
{
    if (0 == dir)
    {
        AnimMoveHermite(Camera(0), -280.0, 0.0, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
        AnimMoveHermite(camera_focus, -280.0, 0.0, -100.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
    }
    else
    {
        AnimMoveHermite(Camera(0), 300.0, 0.0, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
        AnimMoveHermite(camera_focus, 300.0, 0.0, -100.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
    }
}

/* Moves the camera back to the center. */
center_camera()
{
    AnimMoveHermite(Camera(0), 0.0, 0.0, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
    AnimMoveHermite(camera_focus, 0.0, 0.0, -100.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
}

/* Points the camera at a new camera_focus and attaches camera_main, which keeps it there. */
create_camera()
{
    new camera = Camera(0);
    camera_focus = CreateNull(Self());
    camera_uptgt = CreateNull(Self());
    SetPos(camera_focus, 0.0, 0.0, -100.0, 0);
    SetPos(camera_uptgt, 0.0, 0.0 + 1000.0, 0.0, 0);
    SetPos(camera, 0.0, 0.0, 0.0, 0);
    LookAt(camera, camera_focus, 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
    AttachScript(Camera(0), "camera_main");
    SetPubVar(Camera(0), "camera_focus", camera_focus);
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

/* Same as get_cdvd_type of string.inc. */
get_cdvd_type2()
{
    new type = GetSystemParam(SYS_CDVD_TYPE);
    switch (type)
    {
        case CDVD_NO_DISC:
        {
            return type;
        }
        case CDVD_DETECTING:
        {
            return type;
        }
        case CDVD_DETECTING_CD:
        {
            return CDVD_DETECTING;
        }
        case CDVD_DETECTING_DVD:
        {
            return CDVD_DETECTING;
        }
        case CDVD_DETECTING_DVD_DL:
        {
            return CDVD_DETECTING;
        }
        case CDVD_PS1_CD:
        {
            return type;
        }
        case CDVD_PS1_CDDA:
        {
            return type;
        }
        case CDVD_PS2_CD:
        {
            return type;
        }
        case CDVD_PS2_CDDA:
        {
            return type;
        }
        case CDVD_PS2_DVD:
        {
            return type;
        }
    }
    return CDVD_ILLEGAL;
}

/* Runs on a child of the page (argument "owner"): sends "DISC" (type) to it when the disc type changes. */
public media_watcher_main()
{
    new owner;
    new arg[16];
    if (GetArg("owner", arg, 16))
    {
        owner = atoi(arg);
    }
    new old_type = get_cdvd_type2();
    for (; ; )
    {
        new type = get_cdvd_type2();
        if (type != old_type)
        {
            SendMessage(owner, TAG("DISC"), type, 0, 0, 0);
            old_type = type;
        }
        wait_ms(500);
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

/* Returns the x of node of the album curve, the selected item being node 0: a parabola, -80 * (node / 2)^2. */
Float:get_node_x(node)
{
    new Float:t = 0.5 * float(node);
    return 80.0 * (-t * t);
}

/* Returns the y of node of the album curve: 0. */
Float:get_node_y(node)
{
    new n = node;
    return 0.0 * n;
}

/* Returns the z of node of the album curve: -400 per node, -380 per node before the selected one. */
Float:get_node_z(node)
{
    new Float:n = float(node);
    if (n < 0)
    {
        return 380.0 * -n;
    }
    return 400.0 * -n;
}

/* Returns the result of a push of item index in an option menu: circle chooses it, cross goes back. */
handle_menu_push(manager, side, &mode, index, button, title[], group[], id[])
{
    new unused; /* takes the parameters the function does not use */
    new buffer[64];
    unused = manager;
    unused = side;
    unused = mode;
    unused = index;
    unused = 64;
    copy_string(buffer, group, unused);
    copy_string(buffer, title, unused);
    copy_string(buffer, id, unused);
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
    new unused; /* takes the parameters the function does not use */
    unused = side;
    unused = result;
    unused = mode;
    unused = selected;
    unused = 64;
    copy_string(g_msg, title, unused);
    copy_string(g_msg, group, unused);
    copy_string(g_msg, id, unused);
    return;
}

/* Leaves the label of an option menu item as it is. */
fill_menu_item(side, mode, index, group[], label[])
{
    new unused; /* takes the parameters the function does not use */
    new buffer[64];
    unused = side;
    unused = mode;
    unused = index;
    unused = 64;
    copy_string(buffer, group, unused);
    copy_string(buffer, label, unused);
}

/* Reboots into the PlayStation or PlayStation 2 disc in the drive; returns -1 if there is none. */
boot_disc()
{
    g_disc_type = get_cdvd_type2();
    switch (g_disc_type)
    {
        case CDVD_PS1_CD:
        {
            boot_ps1_disc();
        }
        case CDVD_PS1_CDDA:
        {
            boot_ps1_disc();
        }
        case CDVD_PS2_CD:
        {
            boot_ps2_disc();
        }
        case CDVD_PS2_CDDA:
        {
            boot_ps2_disc();
        }
        case CDVD_PS2_DVD:
        {
            boot_ps2_disc();
        }
        default:
        {
            return -1;
        }
    }
    return 0;
}

/* Reboots into the PlayStation disc in the drive, with the disc speed and texture mapping settings. */
boot_ps1_disc()
{
    new plain[4][32];
    new encoded[4][64];
    new texture_mapping;
    new disc_speed;
    new len;
    new cipher[256];
    new i;
    new value[4];
    err10032();
    disc_speed = DISC_SPEED_STANDARD;
    if (0 == avalGetValue("PS1DiscSpeed", 0, value, 4))
    {
        disc_speed = atoi(value);
        if (DISC_SPEED_STANDARD != disc_speed && DISC_SPEED_FAST != disc_speed)
        {
            disc_speed = DISC_SPEED_STANDARD;
        }
    }
    texture_mapping = TEXTURE_STANDARD;
    if (0 == avalGetValue("PS1TextureMapping", 0, value, 4))
    {
        texture_mapping = atoi(value);
        if (TEXTURE_STANDARD != texture_mapping && TEXTURE_SMOOTH != texture_mapping)
        {
            texture_mapping = TEXTURE_STANDARD;
        }
    }
    if (disc_speed == DISC_SPEED_FAST)
    {
        disc_speed = 1;
    }
    else
    {
        disc_speed = 0;
    }
    if (texture_mapping == TEXTURE_SMOOTH)
    {
        texture_mapping = 0x10;
    }
    else
    {
        texture_mapping = 0;
    }
    sprintf(plain[1], "Opt%08x", texture_mapping | disc_speed);
    plain[2] = "BootPs1Cd";
    if (0 != err10002(plain[3]))
    {
        g_disc_type = get_cdvd_type2();
        switch (g_disc_type)
        {
            case CDVD_NO_DISC:
            {
                get_tag_attr("TEXT", "msggame_error_nodisc", "value", g_line1, 32);
                sprintf(g_msg, "%s", g_line1);
                show_message(g_msg);
            }
        }
        return -1;
    }
    SetSystemParam(SYS_PADHELP, 0);
    get_tag_attr("TEXT", "msggame_warn_wait", "value", g_line1, 32);
    get_tag_attr("TEXT", "msggame_warn_wait2", "value", g_line2, 32);
    get_tag_attr("TEXT", "msggame_warn_console_power", "value", g_line3, 32);
    get_tag_attr("TEXT", "msggame_warn_console_disctray", "value", g_line4, 32);
    sprintf(g_msg, "%s%s\n\n%s\n%s", g_line1, g_line2, g_line3, g_line4);
    show_moment(g_msg, 5000);
    err10019(plain[0]);
    encode_base64_2(plain[0], 16, encoded[0], 64);
    err10020();
    new count;
    if (strlen(plain[3]) <= 0)
    {
        count = 3;
    }
    else
    {
        count = 4;
    }
    encoded[1] = "";
    encoded[2] = "";
    encoded[3] = "";
    i = 1;
    for (; i < count; i++)
    {
        len = err10021(plain[i], cipher);
        encode_base64_2(cipher, len, encoded[i], 64);
        apply_rot13_2(encoded[i]);
    }
    switch (count)
    {
        case 3:
        {
            err10022(0, encoded[0]);
            err10022(1, encoded[1]);
            err10022(2, encoded[2]);
        }
        case 4:
        {
            err10022(0, encoded[0]);
            err10022(1, encoded[1]);
            err10022(2, encoded[2]);
            err10022(3, encoded[3]);
        }
    }
    save_album_state();
    write_current_channel("Game");
    err10023(count);
    return 0;
}

/* Reboots into the PlayStation 2 disc in the drive. */
boot_ps2_disc()
{
    new plain[4][32];
    new encoded[4][64];
    new option;
    new len;
    new cipher[256];
    new i;
    new result = -1;
    err10032();
    switch (GetSystemParam(SYS_AUTO_DIAG))
    {
        case 0:
        {
            option = 0x500;
        }
        case 1:
        {
            option = 0x400;
        }
    }
    sprintf(plain[1], "Opt%08x", option);
    g_disc_type = get_cdvd_type2();
    switch (g_disc_type)
    {
        case CDVD_NO_DISC:
        {
            get_tag_attr("TEXT", "msggame_error_nodisc", "value", g_line1, 32);
            sprintf(g_msg, "%s", g_line1);
            show_message(g_msg);
            return -1;
        }
        case CDVD_DETECTING:
        {
            get_tag_attr("TEXT", "msggame_detecting_disc1", "value", g_line1, 32);
            get_tag_attr("TEXT", "msggame_detecting_disc2", "value", g_line3, 32);
            get_tag_attr("TEXT", "msggame_detecting_disc3", "value", g_line4, 32);
            sprintf(g_msg, "%s\n%s\n%s", g_line1, g_line3, g_line4);
            show_message(g_msg);
            return -1;
        }
        case CDVD_PS2_CD:
        {
            plain[2] = "BootPs2Cd";
            result = err10003(plain[3]);
        }
        case CDVD_PS2_CDDA:
        {
            plain[2] = "BootPs2Cd";
            result = err10003(plain[3]);
        }
        case CDVD_PS2_DVD:
        {
            plain[2] = "BootPs2Dvd";
            result = err10004(plain[3]);
        }
        default:
        {
            get_tag_attr("TEXT", "msggame_error_notplaydisc", "value", g_line1, 32);
            sprintf(g_msg, "%s", g_line1);
            show_message(g_msg);
            return -1;
        }
    }
    if (result < 0)
    {
        g_disc_type = get_cdvd_type2();
        switch (g_disc_type)
        {
            case CDVD_NO_DISC:
            {
                get_tag_attr("TEXT", "msggame_error_nodisc", "value", g_line1, 32);
                sprintf(g_msg, "%s", g_line1);
                show_message(g_msg);
                return -1;
            }
            case CDVD_DETECTING:
            {
                get_tag_attr("TEXT", "msggame_detecting_disc1", "value", g_line1, 32);
                get_tag_attr("TEXT", "msggame_detecting_disc2", "value", g_line3, 32);
                get_tag_attr("TEXT", "msggame_detecting_disc3", "value", g_line4, 32);
                sprintf(g_msg, "%s\n%s\n%s", g_line1, g_line3, g_line4);
                show_message(g_msg);
                return -1;
            }
            case CDVD_PS2_CD:
            {
                if (0 != err10034())
                {
                    get_tag_attr("TEXT", "msggame_error_notplaydisc", "value", g_line1, 32);
                    sprintf(g_msg, "%s", g_line1);
                    show_message(g_msg);
                    return -1;
                }
            }
            case CDVD_PS2_CDDA:
            {
            }
            case CDVD_PS2_DVD:
            {
            }
            default:
            {
                get_tag_attr("TEXT", "msggame_error_notplaydisc", "value", g_line1, 32);
                sprintf(g_msg, "%s", g_line1);
                show_message(g_msg);
                return -1;
            }
        }
        get_tag_attr("TEXT", "msggame_execdisc_loadingfail", "value", g_line1, 32);
        sprintf(g_msg, "%s", g_line1);
        show_message(g_msg);
        return -1;
    }
    SetSystemParam(SYS_PADHELP, 0);
    get_tag_attr("TEXT", "msggame_warn_wait", "value", g_line1, 32);
    get_tag_attr("TEXT", "msggame_warn_wait2", "value", g_line2, 32);
    get_tag_attr("TEXT", "msggame_warn_console_power", "value", g_line3, 32);
    get_tag_attr("TEXT", "msggame_warn_console_disctray", "value", g_line4, 32);
    sprintf(g_msg, "%s%s\n\n%s\n%s", g_line1, g_line2, g_line3, g_line4);
    show_moment(g_msg, 5000);
    err10024(plain[3], plain[3]);
    err10019(plain[0]);
    encode_base64_2(plain[0], 16, encoded[0], 64);
    err10020();
    new count = 4;
    i = 1;
    for (; i < count; i++)
    {
        len = err10021(plain[i], cipher);
        encode_base64_2(cipher, len, encoded[i], 64);
        apply_rot13_2(encoded[i]);
    }
    err10022(0, encoded[0]);
    err10022(1, encoded[1]);
    err10022(2, encoded[2]);
    err10022(3, encoded[3]);
    save_album_state();
    write_current_channel("Game");
    err10023(count);
    return 0;
}

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
    sprintf(plain[3], path);
    g_disc_type = get_cdvd_type2();
    switch (g_disc_type)
    {
        case CDVD_NO_DISC:
        {
            get_tag_attr("TEXT", "msggame_exechdd_disc", "value", g_line1, 32);
            get_tag_attr("TEXT", "msggame_execdisc_insertdisc", "value", g_line3, 32);
            sprintf(g_msg, "%s\n%s", g_line1, g_line3);
            show_message(g_msg);
            return -1;
        }
        case CDVD_DETECTING:
        {
            get_tag_attr("TEXT", "msggame_detecting_disc1", "value", g_line1, 32);
            get_tag_attr("TEXT", "msggame_detecting_disc2", "value", g_line3, 32);
            get_tag_attr("TEXT", "msggame_detecting_disc3", "value", g_line4, 32);
            sprintf(g_msg, "%s\n%s\n%s", g_line1, g_line3, g_line4);
            show_message(g_msg);
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
            show_message(g_msg);
            return -1;
        }
    }
    g_disc_type = get_cdvd_type2();
    switch (g_disc_type)
    {
        case CDVD_NO_DISC:
        {
            get_tag_attr("TEXT", "msggame_exechdd_disc", "value", g_line1, 32);
            get_tag_attr("TEXT", "msggame_execdisc_insertdisc", "value", g_line3, 32);
            sprintf(g_msg, "%s\n%s", g_line1, g_line3);
            show_message(g_msg);
            return -1;
        }
        case CDVD_DETECTING:
        {
            get_tag_attr("TEXT", "msggame_detecting_disc1", "value", g_line1, 32);
            get_tag_attr("TEXT", "msggame_detecting_disc2", "value", g_line3, 32);
            get_tag_attr("TEXT", "msggame_detecting_disc3", "value", g_line4, 32);
            sprintf(g_msg, "%s\n%s\n%s", g_line1, g_line3, g_line4);
            show_message(g_msg);
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
                    show_message(g_msg);
                    return -1;
                }
                get_tag_attr("TEXT", "msggame_execdisc_loadingfail", "value", g_line1, 32);
                sprintf(g_msg, "%s", g_line1);
                show_message(g_msg);
                return -1;
            }
        }
        case CDVD_PS2_CDDA:
        {
            if (result < 0)
            {
                get_tag_attr("TEXT", "msggame_execdisc_loadingfail", "value", g_line1, 32);
                sprintf(g_msg, "%s", g_line1);
                show_message(g_msg);
                return -1;
            }
        }
        case CDVD_PS2_DVD:
        {
            if (result < 0)
            {
                get_tag_attr("TEXT", "msggame_execdisc_loadingfail", "value", g_line1, 32);
                sprintf(g_msg, "%s", g_line1);
                show_message(g_msg);
                return -1;
            }
        }
        default:
        {
            get_tag_attr("TEXT", "msggame_execdisc_wrongdisc", "value", g_line1, 32);
            sprintf(g_msg, "%s", g_line1);
            show_message(g_msg);
            return -1;
        }
    }
    result = err10025(plain[2], boot);
    if (result < 0)
    {
        get_tag_attr("TEXT", "msggame_execdisc_wrongdisc", "value", g_line1, 32);
        sprintf(g_msg, "%s", g_line1);
        show_message(g_msg);
        return -4;
    }
    SetSystemParam(SYS_PADHELP, 0);
    get_tag_attr("TEXT", "msggame_warn_wait", "value", g_line1, 32);
    get_tag_attr("TEXT", "msggame_warn_wait2", "value", g_line2, 32);
    get_tag_attr("TEXT", "msggame_warn_console_power", "value", g_line3, 32);
    get_tag_attr("TEXT", "msggame_warn_console_disctray", "value", g_line4, 32);
    sprintf(g_msg, "%s%s\n\n%s\n%s", g_line1, g_line2, g_line3, g_line4);
    show_moment(g_msg, 5000);
    err10024(plain[2], plain[2]);
    err10019(plain[0]);
    encode_base64_2(plain[0], 16, encoded[0], 64);
    err10020();
    new count = 4;
    i = 1;
    for (; i < count; i++)
    {
        len = err10021(plain[i], cipher);
        encode_base64_2(cipher, len, encoded[i], 64);
        apply_rot13_2(encoded[i]);
    }
    err10022(0, encoded[0]);
    err10022(1, encoded[1]);
    err10022(2, encoded[2]);
    err10022(3, encoded[3]);
    save_album_state();
    write_current_channel("Game");
    err10023(count);
    return 0;
}

/* Starts what the SYSTEM.CNF of the partition of the selected game boots; returns a negative value if it cannot. */
boot_selected_game()
{
    new plain[3][32];
    new encoded[3][64];
    new partition[64];
    new buffer[64];
    new len;
    new i;
    new type;
    new select = GetPubVar(g_album, "select");
    datGetData(select, COL_INSTALL_DATE, buffer, 64);
    datGetData(select, COL_URI, partition, 64);
    type = err10026(partition, plain[2], buffer);
    if (type < 0)
    {
        get_tag_attr("TEXT", "msggame_execdisc_dataerr1", "value", g_line1, 32);
        get_tag_attr("TEXT", "msggame_execdisc_dataerr2", "value", g_line3, 32);
        sprintf(g_msg, "%s\n%s", g_line1, g_line3);
        show_message(g_msg);
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
            show_message(g_msg);
            return -1;
        }
        default:
        {
            return -2;
        }
    }
    SetSystemParam(SYS_PADHELP, 0);
    get_tag_attr("TEXT", "msggame_warn_wait", "value", g_line1, 32);
    get_tag_attr("TEXT", "msggame_warn_wait2", "value", g_line2, 32);
    get_tag_attr("TEXT", "msggame_warn_console_power", "value", g_line3, 32);
    sprintf(g_msg, "%s%s\n\n%s", g_line1, g_line2, g_line3);
    show_moment(g_msg, 5000);
    err10019(plain[0]);
    encode_base64_2(plain[0], 16, encoded[0], 64);
    err10020();
    new count = 3;
    i = 1;
    for (; i < count; i++)
    {
        len = err10021(plain[i], buffer);
        encode_base64_2(buffer, len, encoded[i], 64);
        apply_rot13_2(encoded[i]);
    }
    err10022(0, encoded[0]);
    err10022(1, encoded[1]);
    err10022(2, encoded[2]);
    save_album_state();
    write_current_channel("Game");
    err10023(count);
    return 0;
}

/* Shows msg in a dialog and waits until it is closed. */
show_message(msg[])
{
    sprintf(g_args, "msg=%s", msg);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    SetOptionI(dialog, TAG("ZSRT"), ZSORT_OVERLAY);
    AttachScript(dialog, _, "confirm_d", g_args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus_glowfly);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    Destroy(dialog);
}

/* Shows msg for life ms. */
show_moment(msg[], life)
{
    sprintf(g_args, "msg=%s", msg);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    AttachScript(dialog, _, "confirm_moment", g_args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus_glowfly);
    SetPubVar(dialog, "life", life);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
}

/* Asks with the dialog confirm_yesno2, msg being the image of the notice; returns 0 for yes, 1 for no or cross. */
ask_yes_no_notice(msg[], yes[], no[])
{
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    sprintf(g_args, "msg=%s;yes=%s;no=%s", msg, yes, no);
    AttachScript(dialog, _, "confirm_yesno2", g_args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus_glowfly);
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

/* Records the album and the selected game in the aval file for the next visit. */
save_album_state()
{
    new value[4];
    sprintf(value, "%d", g_album_mode);
    avalSet("GameAlbumMode", value);
    sprintf(value, "%d", g_album_select);
    avalSet("GameAlbumCurNo", value);
}
