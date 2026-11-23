/*
 * /opt0/bn/script/utility/netconf_list.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The list of the current settings on the network settings pages, six rows at a time with a scroll bar: each
 * "ITEM" (text) adds a row, the name before the first newline and the value after it. Up and down move the focus
 * along the rows, scrolling them; the other buttons go to the manager.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "tag.inc"

#include <float>

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

#include "misc.inc"

public manager; /* the object told of focus moves and presses */
public focus; /* the focus object */
public padhelp; /* pad help buttons shown besides the circle while the list has the focus */
new g_scroll_bar;
new g_active;
new g_cursor;
new g_top; /* the first row shown */
new g_rows; /* the rows shown at a time */

/* Sets the length of the scroll bar to the part of the list shown. */
update_scroll_length()
{
    new Float:length;
    if (listSize())
    {
        length = 144.0 / (24.0 * listSize());
        if (length > 1.0)
        {
            length = 1.0;
        }
    }
    else
    {
        length = 1.0;
    }
    SetPubVar(g_scroll_bar, "length", length);
}

/* Creates the row of a "name\nvalue" text at the end of the list and returns it; it is shown if it is in view. */
create_item(text[])
{
    new i;
    new j;
    new line[128];
    j = 0;
    for (; text{i}; i++, j++)
    {
        if (text{i} == '\n')
        {
            i++;
            break;
        }
        line{j} = text{i};
    }
    line{j} = 0;
    new item = CreateLabel(Self());
    SetColor(item, 0.7843, 0.7843, 0.7843, 0.0);
    SetOptionI(item, TAG("ALPH"), BLEND_ADD);
    SetOptionI(item, TAG("FONT"), FontID(0));
    SetOptionI(item, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(item, TAG("HALN"), ALIGN_RIGHT);
    SetOptionS(item, TAG("UTF8"), line);
    SetOptionI(item, TAG("FSIZ"), 15);
    SetOptionI(item, TAG("ZSRT"), ZSORT_OVERLAY);
    SetPos(item, -8.0, 24.0 * listSize() - 58.0, 0.0, 0);
    j = 0;
    for (; text{i}; i++, j++)
    {
        line{j} = text{i};
    }
    line{j} = 0;
    AttachScript(item, _, "netconf_item");
    SendMessageStr(item, TAG("TEXT"), line);
    if (listSize() >= g_top && listSize() < g_top + g_rows)
    {
        SendMessage(item, TAG("FADE"), 1.0, 200.0, 0, 0);
    }
    return item;
}

/* Moves the rows so that the row top is the first shown; returns top. */
scroll_to(top)
{
    for (new i; i < listSize(); i++)
    {
        AnimMoveLine(listGetAt(i), 12.0, 24.0 * (i - top) - 58.0, 0.0, 200.0, 0.0, ANIM_FUNC_EXPONENT);
    }
    if (listSize())
    {
        SetPubVar(g_scroll_bar, "top", float(top) / listSize());
    }
    return top;
}

main()
{
    g_scroll_bar = CreateNull(Self());
    SetOptionI(g_scroll_bar, TAG("ZSRT"), ZSORT_OVERLAY);
    AttachScript(g_scroll_bar, _, "scroll_bar");
    SetPubVar(g_scroll_bar, "manager", Self());
    SetPubVar(g_scroll_bar, "focus", focus);
    SetPubVar(g_scroll_bar, "height", 144.0);
    SetPubVar(g_scroll_bar, "top", 0);
    g_top = 0;
    g_rows = floatround(144.0 / 24.0, 0);
    update_scroll_length();
    SendMessage(g_scroll_bar, TAG("FADE"), 0, 0, 0, 0);
    for (new pushed; ; )
    {
        sleep;
        new msg;
        new a;
        new b;
        /* "FOCS" (direction), "FADE" (alpha, duration), "PUSH": the manager has handled the last press. */
        if (RecvMessage(msg, a, b))
        {
            if (msg == TAG("FOCS"))
            {
                receive_focus(a);
            }
            else if (msg == TAG("FADE"))
            {
                fade_items(a, b);
            }
            else if (msg == TAG("PUSH"))
            {
                pushed = 0;
            }
        }
        new text[128];
        if (RecvMessageStr(msg, text, 128))
        {
            if (msg == TAG("ITEM"))
            {
                listAddLast(create_item(text));
                update_scroll_length();
            }
        }
        if (g_active && !pushed)
        {
            new pressed;
            PadState(_, pressed, _, _, -1);
            if (pressed & PAD_UP)
            {
                if (g_cursor > 0)
                {
                    g_cursor--;
                    if (g_cursor < g_top)
                    {
                        SendMessage(listGetAt(g_cursor), TAG("FADE"), 1.0, 200.0, 0, 0);
                        SendMessage(listGetAt(g_top + g_rows - 1), TAG("FADE"), 0, 200.0, 0, 0);
                        g_top = scroll_to(g_top - 1);
                    }
                    sndSePlay(SE_CURSOR);
                    send_label_focus2(focus, listGetAt(g_cursor), 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                }
                else
                {
                    SendMessage(manager, TAG("FOCS"), TAG("UP"), 0, 0, 0);
                    lose_focus();
                }
            }
            else if (pressed & PAD_DOWN)
            {
                if (g_cursor < listSize() - 1)
                {
                    g_cursor++;
                    if (g_cursor >= g_top + g_rows)
                    {
                        SendMessage(listGetAt(g_cursor), TAG("FADE"), 1.0, 200.0, 0, 0);
                        SendMessage(listGetAt(g_top), TAG("FADE"), 0, 200.0, 0, 0);
                        g_top = scroll_to(g_top + 1);
                    }
                    sndSePlay(SE_CURSOR);
                    send_label_focus2(focus, listGetAt(g_cursor), 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                }
                else
                {
                    SendMessage(manager, TAG("FOCS"), TAG("DOWN"), 0, 0, 0);
                    lose_focus();
                }
            }
            else if (pressed & PAD_LEFT)
            {
                SendMessage(manager, TAG("FOCS"), TAG("LEFT"), 0, 0, 0);
                lose_focus();
            }
            else if (pressed & PAD_RIGHT)
            {
                SendMessage(manager, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
                lose_focus();
            }
            else if (pressed & PAD_CIRCLE)
            {
                SendMessage(manager, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
                lose_focus();
            }
            else if (pressed & PAD_CROSS)
            {
                SendMessage(manager, TAG("PUSH"), g_cursor, TAG("RDWN"), 0, 0);
                pushed = 1;
            }
            else if (pressed & PAD_TRIANGLE)
            {
                SendMessage(manager, TAG("PUSH"), g_cursor, TAG("RUP"), 0, 0);
                pushed = 1;
            }
            else if (pressed & PAD_SQUARE)
            {
                SendMessage(manager, TAG("PUSH"), g_cursor, TAG("RLFT"), 0, 0);
                pushed = 1;
            }
            else if (pressed & PAD_SELECT)
            {
                SendMessage(manager, TAG("PUSH"), g_cursor, TAG("SLCT"), 0, 0);
                pushed = 1;
            }
            else if (pressed & PAD_START)
            {
                SendMessage(manager, TAG("PUSH"), g_cursor, TAG("STRT"), 0, 0);
                pushed = 1;
            }
        }
    }
}

/* Fades every row and the scroll bar. */
fade_items(alpha, duration)
{
    for (new i; i < listSize(); i++)
    {
        SendMessage(listGetAt(i), TAG("FADE"), alpha, duration, 0, 0);
    }
    SendMessage(g_scroll_bar, TAG("FADE"), alpha, duration, 0, 0);
}

/* Handles "FOCS": takes the focus, or gives it up if direction is "FOUT". */
receive_focus(direction)
{
    g_active = 1;
    if (direction == TAG("FOUT"))
    {
        lose_focus();
        return;
    }
    if (direction == TAG("LEFT"))
    {
        g_cursor = 0;
        sndSePlay(SE_CURSOR);
    }
    else if (direction == TAG("RIGT"))
    {
        if (listSize())
        {
            g_cursor = listSize() - 1;
        }
        sndSePlay(SE_CURSOR);
    }
    else if (direction == TAG("UP"))
    {
        sndSePlay(SE_CURSOR);
    }
    else if (direction == TAG("DOWN"))
    {
        sndSePlay(SE_CURSOR);
    }
    else if (direction == TAG("FRST"))
    {
        printf("got initial focus.\n");
    }
    if (listSize())
    {
        send_label_focus2(focus, listGetAt(g_cursor), 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    }
    else
    {
        send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    }
    show_pad_help();
}

/* Leaves the focus. */
lose_focus()
{
    g_active = 0;
}

/* Shows the pad help of padhelp and the circle. */
show_pad_help()
{
    new buttons = padhelp;
    buttons = buttons | PADHELP_CIRCLE;
    SetSystemParam(SYS_PADHELP, buttons);
}
