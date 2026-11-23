/*
 * /opt0/bn/script/help/help.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The manual viewer: loads the manual document of its argument url into the page, then shows its menus
 * (MENUGROUP, with "ITEM" entries that open a subgroup or pages) and its pages (PAGEGROUP, one image each).
 * Errors in the document are reported with a code.
 */

#include <core>
#include <bn>

#include "scroll.inc"

#include "genmenu.inc"

#pragma dynamic 1280

/* What a push in a menu leads to, as the MENU_* results of stdmenu.inc; 0 is nothing. */
const ACTION_REDRAW = 1;   /* the same menu again */
const ACTION_REOPEN = 2;
const ACTION_RETURN = 3;
const ACTION_SUBGROUP = 4; /* the subgroup of the item */
const ACTION_BACK = 5;     /* the group the menu returns to */
const ACTION_EXIT = 6;     /* out of the manual */
const ACTION_ERROR = 7;
const ACTION_PAGE = 8;     /* the pages of the item */

/* What a menu shows (kind); a negative kind is an error code. */
const KIND_MENU = 0;  /* the items of a group */
const KIND_PAGES = 1; /* the pages of an item */

/* domMoveTag: the node becomes the next sibling of ref. */
const DOM_AFTER = 3;

/* The shade behind a menu, in the flags of run_menu. */
const SHADE_MASK = 0x300;
const SHADE_FADING_EDGE = 0x200; /* with the fading edge ("FDEG") */

/* The errors in the manual document, reported with their code. */
const ERR_NO_ITEMS = -10001;     /* a menu group has no "ITEM" */
const ERR_NO_RETURN = -10002;    /* there is no menu to go back to */
const ERR_NO_TARGET = -10003;    /* an "ITEM" leads to no subgroup or page, or a PAGE has no src */
const ERR_NO_GROUP = -10004;     /* a MENUGROUP is not in the document */
const ERR_NO_ITEM = -10005;      /* an "ITEM" is not in its group */
const ERR_NO_ATTRIBUTE = -10006; /* an "ITEM" has no id or label, or a PAGE no retitem */
const ERR_NO_ID = -10007;        /* an "ITEM" or the first MENUGROUP has no id */
const ERR_NO_PAGES = -10008;     /* there is no PAGEGROUP, or no PAGE in it */
const ERR_NO_IMAGE_SRC = -10009; /* an IMG has no src */
const ERR_IMAGE = -10010;        /* an image cannot be decoded */
const ERR_TITLE = -10020;        /* the title of a menu cannot be drawn */
const ERR_VERSION = -20000;      /* the version of the document is not 1.0 or 1.1 */

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

#include "tag.inc"

#include "wait.inc"

#include "xmlsync.inc"

#include "imgsync.inc"

/* Sleeps until a message msg arrives. */
wait_message(msg, &a, &b, &c, &d)
{
    while (!FindMessage(msg, a, b, c, d))
    {
        sleep;
    }
}

#include <float>

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

#include "wndvar.inc"

#include "imgdec.inc"

/* Shows or hides the pad help of the right button RR, RD, RU or RL. */
set_pad_help_button(button, on)
{
    new buttons = GetSystemParam(SYS_PADHELP);
    new bit;
    if (button == TAG("RR"))
    {
        bit = PADHELP_CIRCLE;
    }
    else if (button == TAG("RD"))
    {
        bit = PADHELP_CROSS;
    }
    else if (button == TAG("RU"))
    {
        bit = PADHELP_TRIANGLE;
    }
    else if (button == TAG("RL"))
    {
        bit = PADHELP_SQUARE;
    }
    else
    {
        return;
    }
    if (on)
    {
        buttons = buttons | bit;
    }
    else
    {
        buttons = buttons & ~bit;
    }
    SetSystemParam(SYS_PADHELP, buttons);
}

/* Sets the label of the pad help of the right button RR, RD, RU or RL. */
set_pad_help_label(button, label[])
{
    if (button == TAG("RR"))
    {
        SetSystemParamStr(SYS_STR_PADHELP_RR, label);
    }
    else if (button == TAG("RD"))
    {
        SetSystemParamStr(SYS_STR_PADHELP_RD, label);
    }
    else if (button == TAG("RU"))
    {
        SetSystemParamStr(SYS_STR_PADHELP_RU, label);
    }
    else if (button == TAG("RL"))
    {
        SetSystemParamStr(SYS_STR_PADHELP_RL, label);
    }
    else
    {
        return;
    }
}

new Float:g_screen_width6;
new Float:g_screen_height6;
new Float:g_screen_depth6;

/* Same as get_screen_size of text.inc, with globals of its own. */
get_screen_size6(&Float:screen_w, &Float:screen_h)
{
    GetScreen(Camera(0), g_screen_width6, g_screen_height6, g_screen_depth6);
    screen_w = g_screen_width6;
    screen_h = g_screen_height6;
}

#include "disc.inc"

#include "layout.inc"

#include "focus.inc"

/* Opens the dialog script with its arguments args at the center of the screen and returns its object. */
open_dialog(args[])
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

/* Shows msg in a red dialog and waits until it is closed. */
show_error_dialog(focus, msg[])
{
    new args[128];
    sprintf(args, "msg=%s", msg);
    new dialog = open_dialog(args);
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

public stdmenu_curr_group; /* the MENUGROUP element the menu functions read */

/* Makes the MENUGROUP element group the current one; returns whether it exists. */
find_menu_group(group[])
{
    stdmenu_curr_group = domFindTag(domRoot(), "MENUGROUP", group, 0);
    if (stdmenu_curr_group)
    {
        return 1;
    }
    return 0;
}

#include "validate.inc"

/* Reads the retgroup and retitem of the menu: the menu and the item to go back to; returns 0 if there are none. */
get_menu_return(group[], item[], group_max, item_max)
{
    if (!stdmenu_curr_group)
    {
        return 0;
    }
    if (!domGetData(stdmenu_curr_group, "retgroup", group, group_max))
    {
        return 0;
    }
    if (!domGetData(stdmenu_curr_group, "retitem", item, item_max))
    {
        return 0;
    }
    return 1;
}

/* Gets the label of the TITLE of the current group. */
get_group_title(title[], maxlen)
{
    if (!stdmenu_curr_group)
    {
        return 0;
    }
    new node = domFirstChild(stdmenu_curr_group, "TITLE");
    if (!node)
    {
        return 0;
    }
    if (!domGetData(node, "label", title, maxlen))
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
        new item_id[64];
        domGetData(node, "id", item_id, 64);
        if (!strcmp(item_id, id))
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

/* Reads attribute attr of the item id of the menu; returns 0 if there is no such item. */
get_named_item_attr(id[], attr[], value[], maxlen)
{
    if (!stdmenu_curr_group)
    {
        return 0;
    }
    new node = domFindTag(stdmenu_curr_group, "ITEM", id, 0);
    if (!node)
    {
        return 0;
    }
    return domGetData(node, attr, value, maxlen);
}

/*
 * The menu of the current group, run as the script of a null object of run_menu: a title and a general_menu of
 * label_texture items. It reports the push to its manager with "TITL", "GRUP", "IDTY" and "FNSH" (action, kind, error).
 */
public std_menu_core()
{
    new error;
    new title[64];
    GetArg("title", title, 64);
    new group[64];
    GetArg("group", group, 64);
    new id[64];
    GetArg("id", id, 64);
    sleep;
    new obj = Self();
    new Float:screen_w;
    new Float:screen_h;
    get_screen_size6(screen_w, screen_h);
    new manager;
    new action;
    new side;
    new kind;
    {
        new a;
        new b;
        new c;
        new d;
        wait_message(TAG("INIT"), a, b, c, d);
        manager = a;
        side = b;
        action = c;
        kind = d;
    }
    new title_plane;
    new list;
    new scroll_bar;
    if (!find_menu_group(group))
    {
        error = ERR_NO_GROUP;
        goto done;
    }
    if (KIND_MENU == kind)
    {
        get_group_title(title, 64);
    }
    sleep;
    restart:
    new index;
    new count;
    if (KIND_MENU == kind)
    {
        count = count + count_menu_items();
        if (strcmp(id, ""))
        {
            index = find_menu_item_index(id);
        }
        if (count <= 0)
        {
            error = ERR_NO_ITEMS;
        }
        if (index < 0)
        {
            error = ERR_NO_ITEM;
        }
        if (error < 0)
        {
            goto done;
        }
    }
    sleep;
    {
        new result = get_menu_title(side, action, kind, title, group, id, index);
        if (result >= 0)
        {
            count = count + result;
        }
        else
        {
            error = result;
            goto done;
        }
    }
    title_plane = CreatePlane(obj);
    {
        new Float:x;
        new Float:y;
        get_list_title_pos(0, count, 1, x, y);
        SetPos(title_plane, 0.0 * screen_w, screen_h * y, 0.0, 0);
        SetColor(title_plane, 0.902, 0.902, 0.215, 0.0);
        SetOptionI(title_plane, TAG("ZSRT"), ZSORT_OVERLAY);
        imgDecStringStart(title, 17, 64, 64, floatround(0.3844 * screen_w / 0.6, 0));
        new status;
        while (0 == status)
        {
            sleep;
            status = imgDecSync();
        }
        if (status == -1)
        {
            error = ERR_TITLE;
            goto done;
        }
        new Float:tex_w = float(imgTexWidth());
        new Float:tex_h = float(imgTexHeight());
        if (tex_w > 0.3844 * screen_w)
        {
            tex_w = 0.3844 * screen_w;
        }
        SetTexture(title_plane, imgTexID(), 0, -1);
        SetLocalScale(title_plane, tex_w, tex_h, 1.0);
        AnimColor(title_plane, 0.902, 0.902, 0.215, 1.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    }
    sleep;
    list = CreateNull(obj);
    new columns = 1;
    new rows = 5;
    {
        new Float:top;
        get_list_top(count, 1, top);
        SetPos(list, 0.0 * screen_w, screen_h * top, 0.0, 0);
        SetOptionI(list, TAG("ZSRT"), ZSORT_OVERLAY);
        AttachScript(list, "general_menu", "general_menu");
        SetPubVar(list, "manager", Self());
        SetPubVar(list, "focus", GetPubVar(manager, "focus_glowfly"));
        SetPubVar(list, "pad_enable", PAD_CROSS);
        rows = count >= 8 ? 8 : count;
        SendMessage(list, TAG("INUM"), count, 0, 0, 0);
        SendMessage(list, TAG("CLRW"), columns, rows, 0, 0);
        SendMessage(list, TAG("PADL"), -1, 1, 0, 0);
        SendMessage(list, TAG("DIST"), 0, 0.0583 * screen_h, 0, 0);
        SendMessage(list, TAG("MGIN"), 2, 2, 0, 0);
        SendMessage(list, TAG("MODE"), MENU_TYPE_POPUP, SCROLL_LINE, 0, 0);
        SendMessage(list, TAG("ANIM"), 200.0, 0, 6, 0);
        SendMessage(list, TAG("OFFA"), -1.0, 0, 0, 0);
        SendMessage(list, TAG("IDLA"), 1.0, 0, 0, 0);
        SendMessage(list, TAG("REPT"), 100.0, 100.0, 0, 0);
        SendMessage(list, TAG("SLCT"), index, 0, 0, 0);
        SendMessage(list, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
    }
    sleep;
    if (count > rows * columns)
    {
        scroll_bar = CreateNull(obj);
        new Float:bar_x;
        new Float:y;
        new Float:list_x;
        new bar_side;
        if (side == 1)
        {
            bar_side = 1;
        }
        get_list_pos(bar_side, count, 1, list_x, y);
        get_scroll_bar_pos(bar_side, count, 1, bar_x, y);
        SetPos(scroll_bar, screen_w * (bar_x - list_x), screen_h * y, 0.0, 0);
        get_list_body_height(count, 1, y);
        SetOptionI(scroll_bar, TAG("ZSRT"), ZSORT_OVERLAY);
        AttachScript(scroll_bar, _, "scroll_bar");
        SetPubVar(scroll_bar, "manager", Self());
        SetPubVar(scroll_bar, "focus", GetPubVar(manager, "focus_glowfly"));
        SetPubVar(scroll_bar, "height", screen_h * (y + 0.0583 * 2.0));
    }
    sleep;
    for (; ; )
    {
        action = 0;
        new msg;
        new a;
        new b;
        new c;
        /* "ITEM" (list, parent, index): the list asks for the item object; "PUSH" (index, button) */
        while (RecvMessage(msg, a, b, c))
        {
            if (msg == TAG("ITEM"))
            {
                if (a == list)
                {
                    new item_index = c;
                    new label[64];
                    if (KIND_MENU == kind && item_index >= 0 && item_index < count)
                    {
                        new item_id[8];
                        if (!get_item_attr(item_index, "id", item_id, 8))
                        {
                            error = ERR_NO_ID;
                        }
                        if (!get_item_attr(item_index, "label", label, 64))
                        {
                            error = ERR_NO_ATTRIBUTE;
                        }
                    }
                    prepare_item(side, kind, item_index, group, label);
                    new item = create_item_label(b, label);
                    SendMessage(a, TAG("ITEM"), item, c, 0, 0);
                }
            }
            if (msg == TAG("PUSH"))
            {
                SendMessage(list, TAG("PUSH"), 0, 0, 0, 0);
                SendMessage(list, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                index = a;
                if (KIND_MENU == kind)
                {
                    if (!get_item_attr(index, "id", id, 64))
                    {
                        error = ERR_NO_ATTRIBUTE;
                    }
                }
                action = handle_push(manager, side, kind, index, b, title, group, id);
                if (KIND_MENU == kind)
                {
                    error = apply_action(action, index, group, id);
                }
                else if (kind < 0)
                {
                    error = kind;
                }
                if (action < ACTION_REOPEN)
                {
                    SendMessage(list, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                }
            }
        }
        if (scroll_bar)
        {
            new scroll_top = GetPubVar(list, "scroll_top");
            SetPubVar(scroll_bar, "top", float(scroll_top) / float(count));
            SetPubVar(scroll_bar, "length", float(rows * columns) / float(count));
        }
        if (action >= ACTION_REDRAW)
        {
            SetSystemParam(SYS_PADHELP, 0);
            SendMessage(list, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
            break;
        }
        if (ScriptState(list) == SCRIPT_FINISHED)
        {
            break;
        }
        if (error < 0)
        {
            break;
        }
        sleep;
    }
    done:
    if (error < 0)
    {
        action = ACTION_ERROR;
    }
    if (action >= ACTION_REOPEN)
    {
        SendMessageStr(manager, TAG("TITL"), title);
        SendMessageStr(manager, TAG("GRUP"), group);
        SendMessageStr(manager, TAG("IDTY"), id);
        SendMessage(manager, TAG("FNSH"), action, kind, error, 0);
    }
    if (list)
    {
        SendMessage(list, TAG("FADE"), 0, 300.0, 0, 0);
    }
    if (title_plane)
    {
        AnimColor(title_plane, 0.902, 0.902, 0.215, 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    }
    wait_ms(300);
    if (list)
    {
        Destroy(list);
        list = 0;
    }
    if (scroll_bar)
    {
        Destroy(scroll_bar);
        scroll_bar = 0;
    }
    if (title_plane)
    {
        Destroy(title_plane);
        title_plane = 0;
    }
    if (action == ACTION_REDRAW)
    {
        goto restart;
    }
    return index;
}

/* Runs the menus from group, starting on the item index, until the push leaves them; returns the action or an error. */
run_menu(id[], group[], flags, index)
{
    new action;
    new kind;
    new side = flags & 3;
    new error;
    new Float:screen_w;
    new Float:screen_h;
    get_screen_size6(screen_w, screen_h);
    new menu = CreateNull(Disp2D(0));
    if (0 == side || side == 2)
    {
        new Float:x;
        new Float:y;
        get_list_pos(0, 5, 1, x, y);
        SetPos(menu, screen_w * x, 0.0, 0.0, 0);
    }
    else if (side == 1)
    {
        new Float:x;
        new Float:y;
        get_list_pos(1, 5, 1, x, y);
        SetPos(menu, screen_w * x, 0.0, 0.0, 0);
    }
    new shade;
    new shade_mode = flags & SHADE_MASK;
    if (0 != shade_mode)
    {
        shade = CreatePlane(Disp2D(0));
        if (0 == side)
        {
            SetPos(shade, 0.25 * screen_w, 0.5 * screen_h, 0.0, 0);
        }
        else if (side == 1)
        {
            SetPos(shade, 0.75 * screen_w, 0.5 * screen_h, 0.0, 0);
        }
        SetLocalScale(shade, 0.5 * screen_w, 2.0 * (0.364 * screen_h), 0.0);
        SetColor(shade, 0.0, 0.0, 0.0, 0.0);
        AnimColor(shade, 0.0, 0.0, 0.0, 0.3, 500.0, 0.0, ANIM_FUNC_LINEAR);
        SetOptionI(shade, TAG("ZSRT"), ZSORT_BACKGROUND);
        if (shade_mode == SHADE_FADING_EDGE)
        {
            SetOptionI(shade, TAG("FDEG"), 1);
        }
    }
    new cur_title[64];
    new cur_group[64];
    new cur_id[64];
    copy_string(cur_group, id, 64); /* redundant as written */
    copy_string(cur_group, group, 64);
    if (!find_menu_group(cur_group))
    {
        error = ERR_NO_GROUP;
    }
    if (!get_item_attr(index, "id", cur_id, 64))
    {
        error = ERR_NO_ATTRIBUTE;
    }
    if (error < 0)
    {
        goto done;
    }
    sleep;
    for (; ; )
    {
        new args[256];
        sprintf(args, "title = %s ; group = %s ; id = %s", cur_title, cur_group, cur_id);
        AttachScript(menu, "std_menu_core", _, args);
        SendMessage(menu, TAG("INIT"), Self(), side, action, kind);
        for (; ; )
        {
            new msg;
            new a;
            new b;
            new c;
            new text[64];
            while (RecvMessageStr(msg, text, 64))
            {
                if (msg == TAG("TITL"))
                {
                    copy_string(cur_title, text, 64);
                }
                if (msg == TAG("GRUP"))
                {
                    copy_string(cur_group, text, 64);
                }
                if (msg == TAG("IDTY"))
                {
                    copy_string(cur_id, text, 64);
                }
            }
            if (FindMessage(TAG("FNSH"), a, b, c))
            {
                action = a;
                kind = b;
                error = c;
            }
            if (error < 0)
            {
                goto done;
            }
            if (ScriptState(menu) == SCRIPT_FINISHED)
            {
                break;
            }
            sleep;
        }
        if (action >= ACTION_EXIT)
        {
            break;
        }
    }
    done:
    if (0 != shade)
    {
        AnimColor(shade, 0.0, 0.0, 0.0, 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
        wait_ms(300);
        Destroy(shade);
    }
    if (menu)
    {
        Destroy(menu);
    }
    if (error < 0)
    {
        copy_string(group, cur_group, 64);
        action = error;
    }
    return action;
}

/*
 * Moves group and id on to the subgroup or back to the return group; with no return group, the action becomes
 * ACTION_EXIT.
 */
apply_action(&action, index, group[], id[])
{
    if (action == ACTION_RETURN)
    {
    }
    else if (action == ACTION_SUBGROUP)
    {
        new subgroup[64];
        if (!get_item_attr(index, "subgroup", subgroup, 64))
        {
            return ERR_NO_TARGET;
        }
        printf("%s -------------\n", subgroup);
        if (!set_menu_return(subgroup, group, id))
        {
            return ERR_NO_GROUP;
        }
        copy_string(group, subgroup, 64);
        sprintf(id, "");
    }
    else if (action == ACTION_BACK)
    {
        new ret_group[64];
        new ret_item[64];
        if (!get_menu_return(ret_group, ret_item, 64, 64))
        {
            action = ACTION_EXIT;
        }
        else if (!strcmp(ret_group, "") || !strcmp(ret_item, ""))
        {
            action = ACTION_EXIT;
        }
        copy_string(group, ret_group, 64);
        copy_string(id, ret_item, 64);
        printf(" %d, %s, %s, %s, %s \n", index, group, id, ret_group, ret_item);
    }
    return 0;
}

/* Creates a menu item: a label_texture with the label, answering circle. */
create_item_label(parent, label[])
{
    new args[128];
    new item;
    item = CreatePlane(parent);
    SetOptionI(item, TAG("ZSRT"), ZSORT_OVERLAY);
    SetColor(item, 0.215, 0.708, 0.999, 0.0);
    sprintf(args, "string = %s", label);
    AttachScript(item, "label_texture", "label_texture", args);
    SetPubVar(item, "pad_enable", PAD_CIRCLE);
    return item;
}

public camera_focus; /* the point camera_main keeps the camera on */
public focus_glowfly; /* the focus plane of the menus */
new g_bg_halves[2];
new g_black;
new Float:g_screen_w;
new Float:g_screen_h;
new g_page_count;
new g_page_group;
new g_group[64];
new g_item[64];

main()
{
    start_top_page(0);
    new result = load_manual();
    SystemCommand(SYSCMD_HIDE_WINDOW, 1);
    new obj = Self();
    new camera = Camera(0);
    camera_focus = CreateNull(obj);
    SetPos(camera_focus, 0.0, 0.0, 0.0, 0);
    SetPos(camera, 0.0, 0.0, 1.0, 0);
    LookAt(camera, camera_focus, 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
    AttachScript(camera, "camera_main");
    SetPubVar(camera, "camera_focus", camera_focus);
    focus_glowfly = create_focus_plane2("focus");
    if (result < 0)
    {
        goto done;
    }
    result = load_background();
    if (result < 0)
    {
        goto done;
    }
    new label[64];
    get_tag_attr("TEXT", "msggame_btn_channel", "value", label, 64);
    set_pad_help_label(TAG("RL"), label);
    SetSystemParam(SYS_PADHELP, 0);
    set_pad_help_button(TAG("RR"), 1);
    set_pad_help_button(TAG("RD"), 1);
    set_pad_help_button(TAG("RU"), 0);
    set_pad_help_button(TAG("RL"), 0);
    btnhelp_lr_enable(TAG("L1"), 0);
    btnhelp_lr_enable(TAG("R1"), 0);
    g_page_group = domFirstChild(domRoot(), "PAGEGROUP");
    if (!g_page_group)
    {
        result = ERR_NO_PAGES;
        goto done;
    }
    k_belt_control(1);
    if (!domGetData(domFirstChild(domRoot(), "MENUGROUP"), "id", g_group, 64))
    {
        result = ERR_NO_ID;
        goto done;
    }
    new index;
    for (; ; )
    {
        result = run_menu(g_item, g_group, 0, index);
        if (result == ACTION_EXIT || result < 0)
        {
            break;
        }
        new page_id[64];
        get_wnd_var_str2("page_id", page_id, 64);
        result = run_pages(page_id, g_group, g_item);
        if (result == ACTION_EXIT || result < 0)
        {
            break;
        }
        if (!find_menu_group(g_group))
        {
            result = ERR_NO_GROUP;
            break;
        }
        index = find_menu_item_index(g_item);
        if (index < 0)
        {
            result = ERR_NO_ITEM;
            break;
        }
    }
    done:
    if (result < 0)
    {
        new msg[128];
        sprintf(msg, "Manual page format error \n group id  '%s' \n error code %d", g_group, result);
        show_error_dialog(focus_glowfly, msg);
    }
    get_tag_attr("TEXT", "msggame_btn_display", "value", label, 64);
    set_pad_help_label(TAG("RL"), label);
    if (g_bg_halves[0])
    {
        Destroy(g_bg_halves[0]);
    }
    if (g_bg_halves[1])
    {
        Destroy(g_bg_halves[1]);
    }
    if (g_black)
    {
        Destroy(g_black);
    }
    Destroy(Self());
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

/* Shows the IMG element bg as the background, in two halves, and creates the black cover of the pages. */
load_background()
{
    g_bg_halves[0] = CreatePlane(Disp2D(0));
    SetColor(g_bg_halves[0], 1.0, 1.0, 1.0, 0.0);
    g_bg_halves[1] = CreatePlane(Disp2D(0));
    SetColor(g_bg_halves[1], 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_bg_halves[0], TAG("ZSRT"), ZSORT_BACKGROUND);
    SetOptionI(g_bg_halves[1], TAG("ZSRT"), ZSORT_BACKGROUND);
    g_black = CreatePlane(Disp2D(0));
    SetColor(g_black, 0.0, 0.0, 0.0, 0.0);
    get_screen_size6(g_screen_w, g_screen_h);
    SetPos(g_black, 0.5 * g_screen_w, 0.5 * g_screen_h, 0.0, 0);
    SetLocalScale(g_black, g_screen_w, g_screen_h, 1.0);
    get_screen_size6(g_screen_w, g_screen_h);
    new url[256];
    new id[64] = "bg";
    if (get_tag_attr("IMG", id, "src", url, 256))
    {
        imgDecStart(url, 0, 0, -1, 100, 1);
        if (IMG_DONE != wait_image_decode())
        {
            return ERR_IMAGE;
        }
        new w = imgWidth();
        new h = imgHeight();
        SetPos(g_bg_halves[0], 0.5 * g_screen_w, 0.5 * g_screen_h - 0.25 * float(h), 0.0, 0);
        SetPos(g_bg_halves[1], 0.5 * g_screen_w, 0.5 * g_screen_h + 0.25 * float(h), 0.0, 0);
        SetLocalScale(g_bg_halves[0], float(w), 0.5 * float(h), 1.0);
        SetLocalScale(g_bg_halves[1], float(w), 0.5 * float(h), 1.0);
        if (w > 640)
        {
            w = 640;
        }
        if (h > 480)
        {
            h = 480;
        }
        imgTrimStart(0, 0, w, h / 2);
        if (wait_image_trim() == IMG_DONE)
        {
            SetTexture(g_bg_halves[0], imgTexID(), 0, -1);
        }
        else
        {
            return ERR_IMAGE;
        }
        imgTrimStart(0, h / 2, w, h / 2);
        if (wait_image_trim() == IMG_DONE)
        {
            SetTexture(g_bg_halves[1], imgTexID(), 0, -1);
        }
        else
        {
            return ERR_IMAGE;
        }
        AnimColor(g_bg_halves[0], 1.0, 1.0, 1.0, 1.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(g_bg_halves[1], 1.0, 1.0, 1.0, 1.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    }
    return 0;
}

cover_background()
{
    AnimColor(g_black, 0.0, 0.0, 0.0, 1.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    SetOptionI(g_black, TAG("ZSRT"), ZSORT_OVERLAY);
}

/* Shows or hides the pad help of L1 or R1. */
public btnhelp_lr_enable(button, on)
{
    new buttons = GetSystemParam(SYS_PADHELP);
    new bit;
    if (button == TAG("L1"))
    {
        bit = PADHELP_L1;
    }
    else if (button == TAG("R1"))
    {
        bit = PADHELP_R1;
    }
    else
    {
        return;
    }
    if (on)
    {
        buttons = buttons | bit;
    }
    else
    {
        buttons = buttons & ~bit;
    }
    SetSystemParam(SYS_PADHELP, buttons);
}

/* Shows or hides the bars of the foreground. */
public k_belt_control(on)
{
    if (on)
    {
        SetSystemParam(SYS_FG_ALPHA, 255);
        SetSystemParam(SYS_FGBAR_ALPHA, 180);
    }
    else
    {
        SetSystemParam(SYS_FG_ALPHA, 0);
        SetSystemParam(SYS_FGBAR_ALPHA, 0);
    }
}

/* Gets the title of the current group, or the label of the item it returns to. */
get_menu_title(side, action, kind, title[], group[], id[], &index)
{
    if (get_group_title(title, 64))
    {
        return 0;
    }
    new ret_group[64];
    new ret_item[64];
    if (!get_menu_return(ret_group, ret_item, 64, 64))
    {
        return ERR_NO_RETURN;
    }
    new group_node = domFindTag(domRoot(), "MENUGROUP", ret_group, 0);
    if (!group_node)
    {
        return ERR_NO_RETURN;
    }
    new item_node = domFindTag(group_node, "ITEM", ret_item, 0);
    /* NOTE: bug: tests group_node again instead of item_node */
    if (!group_node)
    {
        return ERR_NO_RETURN;
    }
    if (!domGetData(item_node, "label", title, 64))
    {
        return ERR_NO_RETURN;
    }
    return 0;
}

/* Called for each item the list asks for; does nothing. */
prepare_item(side, kind, index, group[], label[])
{
}

/* Returns the action of a push of button on the item id, and sets kind and title. */
handle_push(manager, side, &kind, index, button, title[], group[], id[])
{
    new value[64];
    if (button == TAG("RRGT"))
    {
        if (get_named_item_attr(id, "subgroup", value, 64))
        {
            get_named_item_attr(id, "label", value, 64);
            copy_string(title, value, 64);
            kind = KIND_MENU;
            return ACTION_SUBGROUP;
        }
        if (get_named_item_attr(id, "page", value, 64))
        {
            set_wnd_var_str("page_id", value);
            kind = KIND_PAGES;
            return ACTION_PAGE;
        }
        kind = ERR_NO_TARGET;
        return ACTION_ERROR;
    }
    if (button == TAG("RDWN"))
    {
        kind = KIND_MENU;
        return ACTION_BACK;
    }
    return 0;
}

count_pages()
{
    new count;
    if (!g_page_group)
    {
        return -1;
    }
    for (new node = domFirstChild(g_page_group, "PAGE"); node; node = domNext(node, "PAGE"))
    {
        count++;
    }
    return count;
}

find_page_index(id[])
{
    new index;
    if (!g_page_group)
    {
        return -1;
    }
    for (new node = domFirstChild(g_page_group, "PAGE"); node; node = domNext(node, "PAGE"))
    {
        new page_id[64];
        domGetData(node, "id", page_id, 64);
        if (!strcmp(page_id, id))
        {
            return index;
        }
        index++;
    }
    return -1;
}

get_page_attr(index, attr[], value[], maxlen)
{
    new i;
    if (!g_page_group || index < 0)
    {
        return 0;
    }
    new node;
    node = domFirstChild(g_page_group, "PAGE");
    for (; node && i < index; node = domNext(node, "PAGE"))
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
 * Shows the pages from page_id, one help_page each: L1 and R1 turn the page, square shows or hides the bars, cross
 * goes back. Then sets group and item to the return of the page; returns 0 or an error.
 */
run_pages(page_id[], group[], item[])
{
    cover_background();
    new index;
    new page;
    new old_page;
    new slide = 1;
    new result;
    g_page_count = count_pages();
    if (g_page_count <= 0)
    {
        result = ERR_NO_PAGES;
        goto done;
    }
    index = find_page_index(page_id);
    if (index < 0)
    {
        result = ERR_NO_PAGES;
        goto done;
    }
    printf("num = %d, index = %d\n", g_page_count, index);
    for (; ; )
    {
        set_pad_help_button(TAG("RR"), 0);
        set_pad_help_button(TAG("RD"), 1);
        set_pad_help_button(TAG("RL"), 1);
        if (0 == index)
        {
            btnhelp_lr_enable(TAG("L1"), 0);
            btnhelp_lr_enable(TAG("R1"), 1);
        }
        else if (index == g_page_count - 1)
        {
            btnhelp_lr_enable(TAG("L1"), 1);
            btnhelp_lr_enable(TAG("R1"), 0);
        }
        else
        {
            btnhelp_lr_enable(TAG("L1"), 1);
            btnhelp_lr_enable(TAG("R1"), 1);
        }
        SetSystemParam(SYS_PAGE_MAX, g_page_count);
        SetSystemParam(SYS_PAGE_NO, index + 1);
        page = CreateNull(Disp2D(0));
        {
            new src[64];
            if (!get_page_attr(index, "src", src, 64))
            {
                result = ERR_NO_TARGET;
                goto done;
            }
            new args[128];
            sprintf(args, "image = %s", src);
            AttachScript(page, "help_page", "help_page", args);
            SetPubVar(page, "prepare", slide);
            sleep;
        }
        new belt_on = GetSystemParam(SYS_FGBAR_ALPHA);
        if (belt_on)
        {
            SetPubVar(page, "arrowStatus", 1);
        }
        else
        {
            SetPubVar(page, "arrowStatus", 0);
        }
        if (old_page)
        {
            while (SCRIPT_FINISHED != ScriptState(old_page))
            {
                sleep;
            }
            Destroy(old_page);
            old_page = 0;
        }
        do
        {
            result = GetPubVar(page, "prepare");
            sleep;
        }
        while (result > 0);
        if (result < 0)
        {
            goto done;
        }
        for (; ; )
        {
            new held;
            new pressed;
            new released;
            new repeated;
            PadState(held, pressed, released, repeated, -1);
            result = GetPubVar(page, "suicide");
            if (result < 0)
            {
                goto done;
            }
            if (index + 1 < g_page_count && held & PAD_R1)
            {
                sndSePlay(SE_OPEN);
                index++;
                slide = 3;
                SetPubVar(page, "suicide", slide);
                old_page = page;
                page = 0;
                break;
            }
            else if (index - 1 >= 0 && held & PAD_L1)
            {
                sndSePlay(SE_OPEN);
                index--;
                slide = 2;
                SetPubVar(page, "suicide", slide);
                old_page = page;
                page = 0;
                break;
            }
            else
            {
                if (held & PAD_CROSS)
                {
                    sndSePlay(SE_CANCEL);
                    SetPubVar(page, "arrowStatus", 1);
                    SetPubVar(page, "suicide", 1);
                    goto done;
                }
                if (pressed & PAD_SQUARE)
                {
                    new belt_shown = GetSystemParam(SYS_FGBAR_ALPHA);
                    if (!belt_shown)
                    {
                        k_belt_control(1);
                        SetPubVar(page, "arrowStatus", 1);
                    }
                    else
                    {
                        k_belt_control(0);
                        SetPubVar(page, "arrowStatus", 0);
                    }
                }
            }
            sleep;
        }
        sleep;
    }
    done:
    set_pad_help_button(TAG("RR"), 0);
    set_pad_help_button(TAG("RD"), 0);
    set_pad_help_button(TAG("RL"), 0);
    btnhelp_lr_enable(TAG("L1"), 0);
    btnhelp_lr_enable(TAG("R1"), 0);
    SetSystemParam(SYS_PAGE_MAX, 0);
    k_belt_control(1);
    AnimColor(g_black, 0.0, 0.0, 0.0, 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    wait_ms(300);
    if (page)
    {
        Destroy(page);
    }
    if (old_page)
    {
        Destroy(old_page);
    }
    page = 0;
    old_page = 0;
    new value[64];
    if (!get_page_attr(index, "retitem", value, 64))
    {
        result = ERR_NO_ATTRIBUTE;
    }
    copy_string(item, value, 64);
    if (!get_page_attr(index, "retgroup", value, 64))
    {
        result = ERR_NO_ATTRIBUTE;
    }
    copy_string(group, value, 64);
    if (result < 0)
    {
        sprintf(group, "PAGEGROUP");
    }
    return result;
}

/* Loads the manual document of the argument url and appends it to the page, with the src paths made full. */
load_manual()
{
    new error;
    new url[64];
    new base[64];
    GetArg("url", url, 64);
    {
        new slash = find_last_any_char(url, "/", -1);
        if (slash >= 0)
        {
            copy_substring(base, 0, url, 0, slash + 1, 64);
        }
    }
    xmlLoadSetOptionI(200, 1);
    xmlLoadStart(url, "", "", XMLLOAD_MAIN);
    wait_xml_load();
    new info[4];
    xmlLoadGetInfo(100, info, 4);
    for (new node = domFirstChild(info[0]); node; node = domNext(node))
    {
        new tag_name[10];
        domGetTagName(node, tag_name, 10);
        if (0 == strcmp(tag_name, "IMG"))
        {
            new src[64];
            new path[64];
            if (!domGetData(node, "src", src, 64))
            {
                error = ERR_NO_IMAGE_SRC;
            }
            snprintf(path, 64, "%s%s", base, src);
            domSetData(node, "src", path);
        }
    }
    {
        new page_group = domFindTag(info[0], "PAGEGROUP", _, 0);
        if (page_group)
        {
            for (new node = domFirstChild(page_group); node; node = domNext(node))
            {
                new tag_name[10];
                domGetTagName(node, tag_name, 10);
                if (0 == strcmp(tag_name, "PAGE"))
                {
                    new src[64];
                    new path[64];
                    if (!domGetData(node, "src", src, 64))
                    {
                        error = ERR_NO_TARGET;
                    }
                    snprintf(path, 64, "%s%s", base, src);
                    domSetData(node, "src", path);
                }
            }
        }
        else
        {
            error = ERR_NO_PAGES;
        }
    }
    {
        new version[32];
        new digits[32];
        domGetData(info[0], "version", version, 32);
        replace_all(digits, version, ".", "", 32);
        new version_no = atoi(digits);
        if (version_no < 10 || version_no > 11)
        {
            error = ERR_VERSION;
        }
    }
    new last = domLastChild(domRoot(), "");
    for (new node = domFirstChild(info[0]); node; )
    {
        new next = domNext(node);
        domMoveTag(node, last, DOM_AFTER);
        last = node;
        node = next;
    }
    if (error < 0)
    {
        if (error == 10009) /* NOTE: bug: the error is -10009 */
        {
            sprintf(g_group, "IMG");
        }
        else
        {
            sprintf(g_group, "PAGEGROUP");
        }
    }
    return error;
}
