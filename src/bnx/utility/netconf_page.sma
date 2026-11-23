/*
 * /opt0/bn/script/utility/netconf_page.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A page of the network settings (netconf, ispconf), built from the items of the page element named by the
 * argument "xml_item": texts, a window, a progress bar, the list of the settings and the controls (edit boxes, IP
 * addresses, menus, buttons) that read and write the settings through the manager.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "wait.inc"

#include "tag.inc"

#include <float>

/* Sleeps until a message msg arrives. */
wait_message(msg, &a, &b=0, &c=0, &d=0)
{
    while (!FindMessage(msg, a, b, c, d))
    {
        sleep;
    }
}

#include "sound.inc"

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

/* Same as send_focus of dom.inc. */
send_focus(obj, target, z_offset, Float:sharp, Float:extra_w, Float:extra_h, Float:move_time, smoothing, blinking)
{
    if (!obj)
    {
        return;
    }
    SendMessage(obj, TAG("FOC0"), target, TAG(""), z_offset, sharp);
    SendMessage(obj, TAG("FOC1"), extra_w, extra_h, move_time, smoothing);
    SendMessage(obj, TAG("FOC2"), blinking, 0, 0, 0);
}

public select; /* the control with the focus */
public manager; /* the script of the settings: it answers "GET", "GTXT" and "LIST" and takes the values */
public focus; /* the focus object */
public texid; /* the texture of the window */
public active; /* 1 while the page takes the pad */
new g_focused;
new g_base;
new g_window;
new g_glow; /* the light on the window under the focus */
new g_progress_bar;
new g_list;
new g_menu; /* the first menu: "ENBL" goes to it */
new g_object_count; /* the controls first, then the text labels */
new g_control_count;
new g_padhelp; /* the buttons of the window, PADHELP_CIRCLE and PADHELP_CROSS */

#include "pfs.inc"

/* Asks the manager for the value of a setting ("GET" with its id in value) and returns it. */
get_value(value)
{
    SendMessage(manager, TAG("GET"), value, 0, 0, 0);
    wait_message(TAG("GET"), value);
    return value;
}

/* Gets from the manager ("GTXT") the title of a setting, as the argument of an edit box. */
get_title_arg(id, args[])
{
    new title[64];
    SendMessage(manager, TAG("GTXT"), id, 0, 0, 0);
    wait_message_str(TAG("GTXT"), title, 64);
    sprintf(args, "title=%s", title);
}

/* Gets from the manager ("GET") the text of a setting. */
get_text(id, text[])
{
    SendMessage(manager, TAG("GET"), id, 0, 0, 0);
    wait_message_str(TAG("GET"), text, 64);
}

/* Returns the first item of the page element with that id. */
find_page_items(page_id[])
{
    new page = domFindTag(domRoot(), "page", page_id, 0);
    return page ? domFirstChild(page, "item") : page;
}

/* Returns the item of g_focused: the controls are the items with a type other than window, progress and list. */
find_focused_node(items)
{
    for (new i; i < g_control_count; i++)
    {
        if (g_focused == listGetAt(i))
        {
            new index;
            for (new node = items; node; node = domNext(node, "item"))
            {
                new type[8];
                if (domGetData(node, "type", type, 8))
                {
                    if (strcmp(type, "window") && strcmp(type, "progress") && strcmp(type, "list"))
                    {
                        if (index == i)
                        {
                            return node;
                        }
                        index++;
                    }
                }
            }
        }
    }
    return 0;
}

/* Gets the rows of the list from the manager ("LIST", then "ITEM" until an empty one). */
fill_list(list)
{
    new text[64];
    SendMessage(manager, TAG("LIST"), 0, 0, 0, 0);
    for (; ; )
    {
        wait_message_str(TAG("ITEM"), text, 64);
        if (!strlen(text))
        {
            break;
        }
        SendMessageStr(list, TAG("ITEM"), text);
    }
}

/* Builds the arguments of netconf_menu from the data children of an item. */
get_menu_args(args[], node)
{
    new text[16];
    copy_string(args, "interval = 48.0", 64);
    node = domFirstChild(node, "data");
    for (new i; node; node = domNext(node, "data"), i++)
    {
        sprintf(text, ";btn%d=", i);
        append_string(args, text, 64);
        get_linked_text(node, text, 16);
        append_string(args, text, 64);
    }
}

/* Creates the window of the page, width by height, and the light that follows the focus on it. */
create_window(parent, Float:width, Float:height)
{
    g_base = CreatePlane(parent);
    SetLocalScale(g_base, 2.0 * width, 2.0 * height, 1.0);
    SetColor(g_base, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_base, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_base, TAG("ZOFS"), -2.0);
    SetOptionI(g_base, TAG("ALPH"), BLEND_KEEP);
    g_window = CreatePlane(g_base);
    SetScale(g_window, width, height, 1.0);
    SetColor(g_window, 1.0, 1.0, 1.0, 0.0);
    SetTexture(g_window, texid, 0, -1);
    SetOptionI(g_window, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_window, TAG("ZOFS"), -1.0);
    SetOptionI(g_window, TAG("ALPH"), BLEND_ADD);
    AnimColor(g_window, 1.0, 1.0, 1.0, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
    {
        new Float:tex_margins[4] = {0.5, 0.5, 0.5, 0.5};
        SetOptionP(g_window, TAG("MGNT"), tex_margins, 4);
        new Float:margins[4];
        margins[0] = margins[1] = 0.5 * 128.0 / width;
        margins[2] = margins[3] = 0.5 * 128.0 / height;
        SetOptionP(g_window, TAG("MGN"), margins, 4);
    }
    g_glow = CreatePlane(g_window);
    SetPos(g_glow, 0.3, -0.2, 0.0, 0);
    SetScale(g_glow, 2.0, 2.0, 1.0);
    SetColor(g_glow, 0.0, 0.0, 0.0, 0.0);
    SetTexture(g_glow, imgGetResource(RES_DOT), 0, -1);
    SetOptionI(g_glow, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(g_glow, TAG("ALPH"), BLEND_ADD | BLEND_DEST_ALPHA);
}

create_label(parent)
{
    new label = CreateLabel(parent);
    SetColor(label, 0.7843, 0.7843, 0.7843, 0.0);
    SetOptionI(label, TAG("ALPH"), BLEND_ADD);
    SetOptionI(label, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(label, TAG("FSIZ"), 15);
    SetOptionI(label, TAG("ZSRT"), ZSORT_OVERLAY);
    return label;
}

#include "fileop.inc"

main()
{
    new items;
    {
        new page_id[8];
        GetArg("xml_item", page_id, 8);
        items = find_page_items(page_id);
    }
    for (new node = items; node; node = domNext(node, "item"))
    {
        sleep;
        new value[64];
        new Float:x;
        if (domGetData(node, "x", value, 64))
        {
            x = atof(value);
        }
        new Float:y;
        if (domGetData(node, "y", value, 64))
        {
            y = atof(value);
        }
        new obj;
        if (!domGetData(node, "type", value, 64))
        {
            new align = ALIGN_CENTER;
            if (!domGetData(node, "align", value, 64))
            {
            }
            else if (!strcmp(value, "left"))
            {
                align = ALIGN_LEFT;
            }
            else if (!strcmp(value, "right"))
            {
                align = ALIGN_RIGHT;
            }
            new Float:line_h = 20.0;
            if (domGetData(node, "line", value, 64))
            {
                line_h = atof(value);
            }
            if (get_linked_text(node, value, 64))
            {
                new text[64];
                replace_all(text, value, "\\n", "\n", 64);
                for (new pos; (pos = get_line(value, text, pos, 64)) > 0; )
                {
                    obj = create_label(Self());
                    SetOptionI(obj, TAG("HALN"), align);
                    SetPos(obj, x, y, 0.0, 0);
                    SetOptionS(obj, TAG("UTF8"), value);
                    AnimColor(obj, 0.7843, 0.7843, 0.7843, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
                    listAddLast(obj);
                    g_object_count++;
                    y = y + line_h;
                }
            }
            continue;
        }
        else if (!strcmp(value, "window"))
        {
            new Float:width;
            new Float:height;
            if (domGetData(node, "width", value, 64))
            {
                width = atof(value);
            }
            else
            {
                width = 400.0;
            }
            if (domGetData(node, "height", value, 64))
            {
                height = atof(value);
            }
            else
            {
                height = 260.0;
            }
            create_window(Self(), width, height);
            if (domGetData(node, "pad", value, 64))
            {
                for (new i; i < strlen(value); i++)
                {
                    if (value{i} == 'o')
                    {
                        g_padhelp = g_padhelp | PADHELP_CIRCLE;
                    }
                    else if (value{i} == 'x')
                    {
                        g_padhelp = g_padhelp | PADHELP_CROSS;
                    }
                }
            }
            if (domGetData(node, "info", value, 64))
            {
                SendMessageStr(manager, TAG("INFO"), value);
            }
            continue;
        }
        else if (!strcmp(value, "progress"))
        {
            g_progress_bar = CreateNull(Self());
            SetOptionI(g_progress_bar, TAG("ZSRT"), ZSORT_OVERLAY);
            SetPos(g_progress_bar, x, y, 0.0, 0);
            AttachScript(g_progress_bar, _, "progress_bar");
            SetPubVar(g_progress_bar, "manager", Self());
            SetPubVar(g_progress_bar, "focus", focus);
            continue;
        }
        else if (!strcmp(value, "list"))
        {
            g_list = CreateNull(Self());
            SetOptionI(g_list, TAG("ZSRT"), ZSORT_OVERLAY);
            SetPos(g_list, x, y, 0.0, 0);
            AttachScript(g_list, _, "netconf_list");
            SetPubVar(g_list, "manager", Self());
            SetPubVar(g_list, "focus", focus);
            fill_list(g_list);
            continue;
        }
        else if (!strcmp(value, "edit"))
        {
            new id;
            new args[64];
            obj = CreateNull(Self());
            domGetData(node, "id", value, 64);
            id = atoi(value);
            get_text(id, value);
            get_title_arg(id, args);
            AttachScript(obj, _, "edit_box", args);
            SendMessageStr(obj, TAG("TEXT"), value);
            if (domGetData(node, "mode", value, 64))
            {
                if (!strcmp(value, "userid"))
                {
                    SetPubVar(obj, "mode", EDIT_MODE_USER_ID);
                    SetPubVar(obj, "max_byte", 256);
                }
                else if (!strcmp(value, "secret"))
                {
                    SetPubVar(obj, "mode", EDIT_MODE_PASSWORD);
                    SetPubVar(obj, "max_byte", 256);
                }
            }
        }
        else if (!strcmp(value, "ip"))
        {
            obj = CreateNull(Self());
            domGetData(node, "id", value, 64);
            new address = get_value(atoi(value));
            AttachScript(obj, _, "ipaddr_set");
            for (new i; i < 4; i++)
            {
                sprintf(value, "ip%d", i);
                SetPubVar(obj, value, address & 0xFF);
                address = address >> 8;
            }
        }
        else if (!strcmp(value, "radio"))
        {
            obj = CreateNull(Self());
            domGetData(node, "id", value, 64);
            new selected = get_value(atoi(value));
            get_menu_args(value, node);
            AttachScript(obj, _, "netconf_menu", value);
            SetPubVar(obj, "select", selected);
            SetPubVar(obj, "cursor", selected);
            if (!g_menu)
            {
                g_menu = obj;
            }
        }
        else if (!strcmp(value, "button"))
        {
            obj = CreateNull(Self());
            get_linked_text(node, value, 64);
            new args[64];
            sprintf(args, "text = %s", value);
            AttachScript(obj, _, "button", args);
            SetPubVar(obj, "margin_x", float(15));
        }
        else
        {
            continue;
        }
        SetOptionI(obj, TAG("ZSRT"), ZSORT_OVERLAY);
        SetPos(obj, x, y, 0.0, 0);
        SetPubVar(obj, "manager", Self());
        SetPubVar(obj, "focus", focus);
        SetPubVar(obj, "padhelp", g_padhelp);
        listInsert(g_control_count++, obj);
        g_object_count++;
    }
    new msg;
    new a;
    new b;
    new opening = 1; /* until the window has faded in */
    if (g_control_count)
    {
        select = 0;
        g_focused = listGetAt(select);
        new pushed;
        new focus_left;
        for (; ; )
        {
            sleep;
            if (opening)
            {
                if (ANIM_DONE == AnimGetState(g_window, ANIM_COLOR))
                {
                    SendMessage(g_focused, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
                    active = 1;
                    opening = 0;
                }
            }
            /*
             * "FOCS" (direction): the control with the focus leaves it (up and down move between the controls,
             * left and right go to the manager), or the manager gives it back; "CHNG": a control has a new value,
             * sent to the manager with the id of the setting as the message; "PUSH" (index, button): a press,
             * which goes to the manager only for the buttons of the window; "ENBL" (index, enabled): for the
             * menu; "FADE" (alpha, duration): the page goes away.
             */
            if (RecvMessage(msg, a, b))
            {
                if (msg == TAG("FOCS"))
                {
                    if (focus_left)
                    {
                        SendMessage(g_focused, TAG("FOCS"), a, 0, 0, 0);
                        focus_left = 0;
                    }
                    else if (active)
                    {
                        if (a == TAG("LEFT"))
                        {
                            SendMessage(manager, msg, a, 0, 0, 0);
                            focus_left = 1;
                        }
                        else if (a == TAG("RIGT"))
                        {
                            SendMessage(manager, msg, a, 0, 0, 0);
                            focus_left = 1;
                        }
                        else if (a == TAG("UP"))
                        {
                            if (select > 0)
                            {
                                select--;
                                g_focused = listGetAt(select);
                                SendMessage(g_focused, TAG("FOCS"), TAG("DOWN"), 0, 0, 0);
                            }
                            else
                            {
                                SendMessage(g_focused, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                            }
                        }
                        else if (a == TAG("DOWN"))
                        {
                            if (select < g_control_count - 1)
                            {
                                select++;
                                g_focused = listGetAt(select);
                                SendMessage(g_focused, TAG("FOCS"), TAG("UP"), 0, 0, 0);
                            }
                            else
                            {
                                SendMessage(g_focused, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                            }
                        }
                    }
                    else
                    {
                        SendMessage(g_focused, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                    }
                }
                else if (msg == TAG("CHNG"))
                {
                    if (active)
                    {
                        new node = find_focused_node(items);
                        new type[64];
                        if (domGetData(node, "type", type, 64))
                        {
                            new id[4];
                            domGetData(node, "id", id, 4);
                            if (!strcmp(type, "edit"))
                            {
                                SendMessage(g_focused, TAG("GTXT"), 0, 0, 0, 0);
                                wait_message_str(TAG("GTXT"), type, 64);
                                SendMessageStr(manager, atoi(id), type);
                            }
                            else
                            {
                                if (!strcmp(type, "ip"))
                                {
                                    a = GetPubVar(g_focused, "ip3");
                                    a = a << 8;
                                    a = a + GetPubVar(g_focused, "ip2");
                                    a = a << 8;
                                    a = a + GetPubVar(g_focused, "ip1");
                                    a = a << 8;
                                    a = a + GetPubVar(g_focused, "ip0");
                                }
                                SendMessage(manager, atoi(id), a, 0, 0, 0);
                            }
                        }
                    }
                    SendMessage(g_focused, TAG("CHNG"), 0, 0, 0, 0);
                }
                else if (msg == TAG("PUSH"))
                {
                    if (pushed)
                    {
                        pushed = 0;
                    }
                    else
                    {
                        if (active)
                        {
                            if ((b == TAG("RRGT") && g_padhelp & PADHELP_CIRCLE)
                                    || (b == TAG("RDWN") && g_padhelp & PADHELP_CROSS))
                            {
                                active = 0;
                                SendMessage(manager, msg, a, b, 0, 0);
                                pushed = 1;
                            }
                        }
                        if (!pushed)
                        {
                            SendMessage(g_focused, TAG("PUSH"), 0, 0, 0, 0);
                        }
                    }
                }
                else if (msg == TAG("ENBL"))
                {
                    if (g_menu)
                    {
                        SendMessage(g_menu, msg, a, b, 0, 0);
                    }
                }
                else if (msg == TAG("FADE"))
                {
                    SendMessage(g_focused, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                    break;
                }
            }
            if (focus)
            {
                new Float:pos[3];
                GetPos(focus, pos[0], pos[1], pos[2], g_window);
                SetPos(g_glow, -pos[0], pos[1], 0.0, 0);
                new Float:alpha;
                GetColor(focus, _, _, _, alpha);
                AnimColor(g_glow, 0.5 * alpha, 0.5 * alpha, 0.5 * alpha, 1.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
            }
        }
    }
    else
    {
        show_pad_help();
        send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        new pushed;
        new focus_left;
        for (; ; )
        {
            sleep;
            if (opening)
            {
                if (ANIM_DONE == AnimGetState(g_window, ANIM_COLOR))
                {
                    active = 1;
                    opening = 0;
                }
            }
            /* "PROG" (progress), "FADE" (alpha, duration); "FOCS" and "PUSH": the manager has handled them. */
            if (RecvMessage(msg, a, b))
            {
                if (msg == TAG("PROG"))
                {
                    if (g_progress_bar)
                    {
                        SetPubVar(g_progress_bar, "progress", a);
                    }
                }
                else if (msg == TAG("FADE"))
                {
                    break;
                }
                else if (msg == TAG("FOCS"))
                {
                    focus_left = 0;
                }
                else if (msg == TAG("PUSH"))
                {
                    pushed = 0;
                }
            }
            if (active && !pushed && !focus_left)
            {
                new pressed;
                PadState(_, pressed, _, _, -1);
                if (pressed & PAD_LEFT)
                {
                    SendMessage(manager, TAG("FOCS"), TAG("LEFT"), 0, 0, 0);
                    focus_left = 1;
                }
                else if (pressed & PAD_RIGHT)
                {
                    SendMessage(manager, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
                    focus_left = 1;
                }
                else if (pressed & PAD_CIRCLE)
                {
                    if (pressed & PAD_CIRCLE && g_padhelp & PADHELP_CIRCLE)
                    {
                        sndSePlay(SE_DECIDE);
                        SendMessage(manager, TAG("PUSH"), 0, TAG("RRGT"), 0, 0);
                        pushed = 1;
                    }
                }
                else if (pressed & PAD_CROSS)
                {
                    if (pressed & PAD_CROSS && g_padhelp & PADHELP_CROSS)
                    {
                        SendMessage(manager, TAG("PUSH"), 0, TAG("RDWN"), 0, 0);
                        pushed = 1;
                    }
                }
            }
        }
    }
    fade_page(a, b);
    Destroy(Self());
}

/* Gives up the focus and fades out the page in duration ms. */
fade_page(Float:alpha, Float:duration)
{
    send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    g_padhelp = 0;
    show_pad_help();
    new i;
    i = 0;
    for (; i < g_control_count; i++)
    {
        SendMessage(listGetAt(i), TAG("FADE"), alpha, duration, 0, 0);
    }
    for (; i < g_object_count; i++)
    {
        AnimColor(listGetAt(i), 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    }
    if (g_window)
    {
        AnimColor(g_window, 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    }
    if (g_progress_bar)
    {
        SendMessage(g_progress_bar, TAG("FADE"), alpha, duration, 0, 0);
    }
    if (g_list)
    {
        SendMessage(g_list, TAG("FADE"), alpha, duration, 0, 0);
    }
    wait_ms(floatround(duration, 0));
}

show_pad_help()
{
    SetSystemParam(SYS_PADHELP, g_padhelp);
}
