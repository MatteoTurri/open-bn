/*
 * /opt0/bn/script/system/navimenu/list.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A list of up to ten labels for the navigation menu, from the arguments btn<n> (with sc<n> for a
 * shortcut): it moves the cursor and tells its manager when a button is pressed or the cursor leaves it.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "wait.inc"

#include "tag.inc"

#include <float>

#include "sysconf.inc"

new g_shadow_colors[4] = {0x5F000000, 0x5F000000, 0x5F000000, 0x5F000000}; /* the "SCL4" of the shadowed labels */
public manager; /* gets "FOCS" when the cursor leaves the list and "PUSH" (item, button) */
public focus; /* the focus plane */
public padhelp; /* the pad help buttons shown on every item */
public active; /* the list has the cursor */
public cursor; /* the item under the cursor */
public Float:meta_alpha; /* alpha of the whole list, set by "FADE" */
new g_labels[10];
new g_enabled[10] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
new g_is_shortcut[10];
new g_count;
new g_visible;
public scroll_pos; /* the first visible item */
new Float:g_interval = 60.0;

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
    g_visible = 10;
    {
        new value[20];
        if (GetArg("visible", value, 20))
        {
            g_visible = atoi(value);
        }
    }
    for (new i; i < 10; i++)
    {
        new name[64];
        new value[64];
        sprintf(name, "btn%d", i);
        if (GetArg(name, value, 64))
        {
            g_labels[i] = CreateLabelShadow(Self());
            SetOptionP(g_labels[i], TAG("SCL4"), g_shadow_colors, 4);
            SetOptionI(g_labels[i], TAG("SX64"), 2 * 64);
            SetOptionI(g_labels[i], TAG("SY64"), 2 * 64);
            SetPos(g_labels[i], 0.0, 0.0, 0.0, 0);
            SetColor(g_labels[i], 0.215, 0.708, 0.999, 0.0);
            SetOptionI(g_labels[i], TAG("ALPH"), BLEND_ADD);
            SetOptionS(g_labels[i], TAG("UTF8"), value);
            SetOptionI(g_labels[i], TAG("HALN"), ALIGN_CENTER);
            SetOptionI(g_labels[i], TAG("VALN"), ALIGN_CENTER);
            SetOptionI(g_labels[i], TAG("FSIZ"), font_size);
            SetOptionI(g_labels[i], TAG("ZSRT"), ZSORT_OVERLAY);
            SetOptionI(g_labels[i], TAG("W64 "), 320 * 64);
            g_count++;
            sprintf(name, "sc%d", i);
            if (GetArg(name, value, 64))
            {
                g_is_shortcut[i] = 1;
            }
            sleep;
        }
        else
        {
            break;
        }
    }
    if (g_count > g_visible)
    {
        scroll_pos = 0;
    }
    else
    {
        scroll_pos = 0;
        g_visible = g_count;
    }
    {
        new value[20];
        if (GetArg("interval", value, 20))
        {
            g_interval = atof(value);
        }
        for (new i; i < g_count; i++)
        {
            new Float:y = g_interval * i;
            SetPos(g_labels[i], 0.0, y, 0.0, 0);
        }
    }
    sleep;
    update_colors(100.0, 0.0);
    new pushed;
    for (; ; )
    {
        new changed;
        new msg;
        new a;
        new b;
        /*
         * "FOCS" (from): takes the cursor; "FADE" (alpha, time); "ENBL" (item, enabled); "PUSH": the manager is
         * done with the last push.
         */
        while (RecvMessage(msg, a, b))
        {
            if (msg == TAG("FOCS"))
            {
                enter_focus(a);
            }
            else if (msg == TAG("FADE"))
            {
                meta_alpha = a;
                update_colors(b, 0.0);
            }
            else if (msg == TAG("ENBL"))
            {
                g_enabled[a] = b;
                changed = 1;
            }
            else if (msg == TAG("PUSH"))
            {
                pushed = 0;
            }
        }
        if (changed)
        {
            update_colors(100.0, 0.0);
        }
        if (active && !pushed)
        {
            cursor = clamp(cursor, 0, g_count - 1);
            new pressed;
            new repeated;
            PadState(_, pressed, _, repeated, -1);
            static last_move;
            if (pressed & PAD_UP || (repeated & PAD_UP && tickcount() - last_move > 70))
            {
                last_move = tickcount();
                if (cursor - 1 >= 0)
                {
                    cursor--;
                    sndSePlay(SE_CURSOR);
                    move_focus(cursor);
                    update_colors(100.0, 0.0);
                    scroll_to_cursor();
                    update_pad_help();
                }
                else
                {
                    SendMessage(manager, TAG("FOCS"), TAG("UP"), 0, 0, 0);
                    leave_focus();
                    goto next_frame;
                }
            }
            else if (pressed & PAD_DOWN || (repeated & PAD_DOWN && tickcount() - last_move > 70))
            {
                last_move = tickcount();
                if (cursor + 1 < g_count)
                {
                    cursor++;
                    sndSePlay(SE_CURSOR);
                    move_focus(cursor);
                    update_colors(100.0, 0.0);
                    scroll_to_cursor();
                    update_pad_help();
                }
                else
                {
                    SendMessage(manager, TAG("FOCS"), TAG("DOWN"), 0, 0, 0);
                    leave_focus();
                    goto next_frame;
                }
            }
            else
            {
                if (pressed & PAD_LEFT)
                {
                    SendMessage(manager, TAG("FOCS"), TAG("LEFT"), 0, 0, 0);
                    leave_focus();
                    goto next_frame;
                }
                if (pressed & PAD_RIGHT)
                {
                    SendMessage(manager, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
                    leave_focus();
                    goto next_frame;
                }
            }
            if (pressed & PAD_CIRCLE)
            {
                if (g_enabled[cursor])
                {
                    SendMessage(manager, TAG("PUSH"), cursor, TAG("RRGT"), 0, 0);
                    sndSePlay(SE_DECIDE);
                    pushed = 1;
                    goto next_frame;
                }
            }
            else
            {
                if (pressed & PAD_CROSS)
                {
                    SendMessage(manager, TAG("PUSH"), cursor, TAG("RDWN"), 0, 0);
                    pushed = 1;
                    goto next_frame;
                }
                if (pressed & PAD_TRIANGLE)
                {
                    if (g_enabled[cursor])
                    {
                        SendMessage(manager, TAG("PUSH"), cursor, TAG("RUP"), 0, 0);
                        pushed = 1;
                        goto next_frame;
                    }
                }
                else if (pressed & PAD_SELECT)
                {
                    SendMessage(manager, TAG("PUSH"), cursor, TAG("SLCT"), 0, 0);
                    pushed = 1;
                    goto next_frame;
                }
            }
        }
        next_frame:
        sleep;
    }
    return;
}

/* Fades every label to its color: dim when disabled, transparent when scrolled out of view. */
update_colors(Float:duration, Float:delay)
{
    for (new i; i < g_count; i++)
    {
        new Float:r;
        new Float:g;
        new Float:b;
        r = 0.215;
        g = 0.708;
        b = 0.999;
        new Float:alpha = 1.0;
        if (active && cursor == i)
        {
            alpha = 1.0;
        }
        if (!g_enabled[i])
        {
            r = 0.03;
            g = 0.18;
            b = 0.36;
            alpha = 1.0;
        }
        if (i < scroll_pos)
        {
            alpha = 0.0;
        }
        if (i >= scroll_pos + g_visible)
        {
            alpha = 0.0;
        }
        AnimColor(g_labels[i], r, g, b, alpha * meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
    }
}

move_focus(index)
{
    if (g_enabled[index])
    {
        send_label_focus(focus, g_labels[index], 0, 0.5, 60.0, 60.0, 70.0, 1, 1);
    }
    else
    {
        send_label_focus(focus, g_labels[index], 0, 0.5, 60.0, 60.0, 70.0, 1, 0);
    }
}

/* Takes the cursor coming from the direction from ("FRST", UP, "DOWN", "LEFT", "RIGT"), or gives it up ("FOUT"). */
enter_focus(from)
{
    active = 1;
    if (from == TAG("FOUT"))
    {
        leave_focus();
        send_label_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        return;
    }
    if (from == TAG("LEFT"))
    {
        sndSePlay(SE_CURSOR);
    }
    else if (from == TAG("RIGT"))
    {
        sndSePlay(SE_CURSOR);
    }
    else if (from == TAG("UP"))
    {
        cursor = 0;
        sndSePlay(SE_CURSOR);
    }
    else if (from == TAG("DOWN"))
    {
        cursor = g_count - 1;
        sndSePlay(SE_CURSOR);
    }
    else if (from == TAG("FRST"))
    {
    }
    move_focus(cursor);
    update_colors(100.0, 0.0);
    update_pad_help();
}

/* Leaves the focus and dims the colors. */
leave_focus()
{
    active = 0;
    update_colors(100.0, 40.0);
}

/* Shows padhelp, plus circle for an enabled item and triangle for an enabled shortcut. */
update_pad_help()
{
    new buttons = padhelp;
    if (g_enabled[cursor])
    {
        buttons = buttons | PADHELP_CIRCLE;
        if (g_is_shortcut[cursor])
        {
            buttons = buttons | PADHELP_TRIANGLE;
        }
    }
    SetSystemParam(SYS_PADHELP, buttons);
}

scroll_to_cursor()
{
    new scrolled;
    if (cursor < scroll_pos)
    {
        scroll_pos--;
        scrolled = 1;
    }
    else if (cursor >= scroll_pos + g_visible)
    {
        scroll_pos++;
        scrolled = 1;
    }
    if (scrolled)
    {
        update_colors(float(300), 0.0);
        for (new i; i < g_count; i++)
        {
            new Float:y = g_interval * (i - scroll_pos);
            AnimMoveLine(g_labels[i], 0.0, y, 0.0, float(300), 0.0, ANIM_FUNC_ANGULER);
        }
        wait_ms(300);
    }
}
