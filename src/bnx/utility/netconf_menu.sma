/*
 * /opt0/bn/script/utility/netconf_menu.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A vertical menu of up to ten labels (arguments "btn0" to "btn9", "interval" between them), the left column of
 * the network settings pages. Moving the cursor tells the manager the item under it ("CHNG" index, -1 for a
 * disabled one); the circle and the right button move the focus to the right, the other buttons are presses.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "tag.inc"

#include <float>

#include "sysconf.inc"

new g_shadow_colors[4] = {0x5F000000, 0x5F000000, 0x5F000000, 0x5F000000}; /* the "SCL4" of the shadowed labels */
public manager; /* the object told of focus moves, changes and presses */
public focus; /* the focus object */
public padhelp; /* pad help buttons shown besides the circle while the menu has the focus */
public active; /* 1 while the menu has the focus */
public select; /* the item under the cursor */
public cursor; /* unused, but part of the layout */
public Float:meta_alpha = 1.0; /* overall alpha, set by "FADE" */
new g_labels[10];
new g_enabled[10] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
new g_count;

main()
{
    new font_size = 17;
    {
        new value[20];
        if (GetArg("fontsize", value, 20))
        {
            font_size = atoi(value);
        }
    }
    for (new i; i < 10; i++)
    {
        new key[64];
        new text[64];
        sprintf(key, "btn%d", i);
        if (GetArg(key, text, 64))
        {
            g_labels[i] = CreateLabelShadow(Self());
            SetOptionP(g_labels[i], TAG("SCL4"), g_shadow_colors, 4);
            SetOptionI(g_labels[i], TAG("SX64"), 2 * 64);
            SetOptionI(g_labels[i], TAG("SY64"), 2 * 64);
            SetColor(g_labels[i], 0.215, 0.708, 0.999, 0.0);
            SetOptionI(g_labels[i], TAG("ALPH"), BLEND_ADD);
            SetOptionS(g_labels[i], TAG("UTF8"), text);
            SetOptionI(g_labels[i], TAG("HALN"), ALIGN_LEFT);
            SetOptionI(g_labels[i], TAG("VALN"), ALIGN_CENTER);
            SetOptionI(g_labels[i], TAG("FSIZ"), font_size);
            g_count++;
            sleep;
        }
        else
        {
            break;
        }
    }
    {
        new Float:spacing = 60.0;
        new value[20];
        if (GetArg("interval", value, 20))
        {
            spacing = atof(value);
        }
        for (new i; i < g_count; i++)
        {
            new Float:y = spacing * i;
            SetPos(g_labels[i], 0.0, y, 0.0, 0);
        }
    }
    sleep;
    update_color(100.0, 0.0);
    new pushed;
    new changed;
    for (; ; )
    {
        sleep;
        new msg;
        new a;
        new b;
        /*
         * "FOCS" (direction): the focus arrives or leaves ("FOUT"); "FADE" (alpha, duration); "ENBL" (index,
         * enabled); "PUSH" and "CHNG": the manager has handled the last press or change.
         */
        if (RecvMessage(msg, a, b))
        {
            if (msg == TAG("FOCS"))
            {
                receive_focus(a);
                if (a == TAG("FRST"))
                {
                    focus_item(select);
                    changed = 1;
                }
            }
            else if (msg == TAG("FADE"))
            {
                meta_alpha = a;
                update_color(b, 0.0);
            }
            else if (msg == TAG("ENBL"))
            {
                if (g_enabled[a] != b)
                {
                    g_enabled[a] = b;
                    update_color(100.0, 0.0);
                    if (a == select)
                    {
                        focus_item(select);
                        changed = 1;
                    }
                }
            }
            else if (msg == TAG("PUSH"))
            {
                pushed = 0;
            }
            else if (msg == TAG("CHNG"))
            {
                changed = 0;
            }
        }
        if (active && !pushed && !changed)
        {
            new pressed;
            PadState(_, pressed, _, _, -1);
            if (pressed & PAD_UP)
            {
                if (select > 0)
                {
                    select--;
                    sndSePlay(SE_CURSOR);
                    update_color(100.0, 0.0);
                    focus_item(select);
                    changed = 1;
                }
                else
                {
                    SendMessage(manager, TAG("FOCS"), TAG("UP"), 0, 0, 0);
                    lose_focus();
                    continue;
                }
            }
            else if (pressed & PAD_DOWN)
            {
                if (select < g_count - 1)
                {
                    select++;
                    sndSePlay(SE_CURSOR);
                    update_color(100.0, 0.0);
                    focus_item(select);
                    changed = 1;
                }
                else
                {
                    SendMessage(manager, TAG("FOCS"), TAG("DOWN"), 0, 0, 0);
                    lose_focus();
                    continue;
                }
            }
            else if (pressed & PAD_LEFT)
            {
                SendMessage(manager, TAG("FOCS"), TAG("LEFT"), 0, 0, 0);
                lose_focus();
                continue;
            }
            else if (pressed & PAD_RIGHT)
            {
                sndSePlay(SE_DECIDE);
                SendMessage(manager, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
                lose_focus();
                continue;
            }
            if (pressed & PAD_CIRCLE)
            {
                sndSePlay(SE_DECIDE);
                SendMessage(manager, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
                lose_focus();
            }
            else if (pressed & PAD_CROSS)
            {
                SendMessage(manager, TAG("PUSH"), select, TAG("RDWN"), 0, 0);
                pushed = 1;
            }
            else if (pressed & PAD_TRIANGLE)
            {
                SendMessage(manager, TAG("PUSH"), select, TAG("RUP"), 0, 0);
                pushed = 1;
            }
            else if (pressed & PAD_SQUARE)
            {
                SendMessage(manager, TAG("PUSH"), select, TAG("RLFT"), 0, 0);
                pushed = 1;
            }
            else if (pressed & PAD_SELECT)
            {
                SendMessage(manager, TAG("PUSH"), select, TAG("SLCT"), 0, 0);
                pushed = 1;
            }
            else if (pressed & PAD_START)
            {
                SendMessage(manager, TAG("PUSH"), select, TAG("STRT"), 0, 0);
                pushed = 1;
            }
        }
    }
}

/* Moves the focus to an item and tells the manager ("CHNG"), which answers "CHNG" when it is done. */
focus_item(index)
{
    send_label_focus(focus, g_labels[index], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    if (g_enabled[index])
    {
        show_pad_help(1);
        SendMessage(manager, TAG("CHNG"), index, 0, 0, 0);
    }
    else
    {
        show_pad_help(0);
        SendMessage(manager, TAG("CHNG"), -1, 0, 0, 0);
    }
}

/* Fades the labels to their colors, dimmed when disabled. */
update_color(Float:duration, Float:delay)
{
    for (new i; i < g_count; i++)
    {
        new Float:alpha = 1.0;
        new Float:red = 0.215;
        new Float:green = 0.708;
        new Float:blue = 0.999;
        if (active && select == i) /* redundant as written */
        {
            alpha = 1.0;
        }
        if (!g_enabled[i])
        {
            red = 0.03;
            green = 0.18;
            blue = 0.36;
            alpha = 1.0;
        }
        AnimColor(g_labels[i], red, green, blue, alpha * meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
    }
}

/* Handles "FOCS": takes the focus, or gives it up if direction is "FOUT". */
receive_focus(direction)
{
    active = 1;
    if (direction == TAG("FOUT"))
    {
        lose_focus();
        return;
    }
    if (direction == TAG("LEFT"))
    {
        sndSePlay(SE_CURSOR);
    }
    else if (direction == TAG("RIGT"))
    {
        sndSePlay(SE_CURSOR);
    }
    else if (direction == TAG("UP"))
    {
        cursor = 0;
        sndSePlay(SE_CURSOR);
    }
    else if (direction == TAG("DOWN"))
    {
        cursor = g_count - 1;
        sndSePlay(SE_CURSOR);
    }
    else if (direction == TAG("FRST"))
    {
        printf("got initial focus.\n");
    }
    update_color(100.0, 0.0);
}

/* Leaves the focus and dims the colors. */
lose_focus()
{
    active = 0;
    update_color(100.0, 40.0);
}

/* Shows padhelp in the pad help, with the circle if circle is not 0. */
show_pad_help(circle)
{
    new buttons = padhelp;
    if (circle)
    {
        buttons = buttons | PADHELP_CIRCLE;
    }
    SetSystemParam(SYS_PADHELP, buttons);
}
