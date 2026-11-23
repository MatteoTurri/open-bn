/*
 * /opt0/bn/script/parts/radio_button.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A row of up to ten radio buttons (arguments "btn0" to "btn9"), centered on the object. Left and right move
 * the cursor and the circle selects; at the ends, and with the other buttons, it talks to its manager.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "tag.inc"

#include <float>

#include "sysconf.inc"

public manager; /* the object told of focus moves, changes and presses */
public focus; /* the focus object */
public padhelp; /* pad help buttons shown besides the circle while the group has the focus */
public active; /* 1 while the group has the focus */
public select; /* the selected button */
public cursor; /* the button under the focus */
public Float:meta_alpha = 1.0; /* overall alpha, set by "FADE" */
public disable_selected = 1; /* 0: the circle on the selected button sends "CHNG" again */
new g_labels[10];
new g_marks[10]; /* the radio marks, shown on the selected button */
new g_enabled[10] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
new g_count;
new Float:g_mark_x = -15.0;
new Float:g_mark_w = 18.0;
new Float:g_mark_h = 16.0;
new Float:g_spacing = 60.0; /* between the end of a label and the start of the next */

main()
{
    new font_size = 15;
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
            g_labels[i] = CreateLabel(Self());
            SetPos(g_labels[i], 0.0, 0.0, 0.0, 0);
            SetColor(g_labels[i], 0.215, 0.708, 0.999, 0.0);
            SetOptionI(g_labels[i], TAG("ALPH"), BLEND_ADD);
            SetOptionS(g_labels[i], TAG("UTF8"), text);
            SetOptionI(g_labels[i], TAG("HALN"), ALIGN_LEFT);
            SetOptionI(g_labels[i], TAG("VALN"), ALIGN_CENTER);
            SetOptionI(g_labels[i], TAG("FSIZ"), font_size);
            g_marks[i] = CreatePlane(g_labels[i]);
            SetPos(g_marks[i], g_mark_x, 0.0, 0.0, 0);
            SetScale(g_marks[i], g_mark_w, g_mark_h, 1.0);
            SetColor(g_marks[i], 1.0, 1.0, 1.0, 0.0);
            SetTexture(g_marks[i], imgGetResource(RES_RADIO), 0, -1);
            SetOptionI(g_marks[i], TAG("ALPH"), BLEND_ADD);
            g_count++;
            sleep;
        }
        else
        {
            break;
        }
    }
    {
        new Float:widths[10];
        for (new i; i < g_count; )
        {
            new rect[5];
            GetOptionP(g_labels[i], TAG("RECT"), rect, 5);
            if (rect[0])
            {
                widths[i] = float(rect[3]) / 64.0;
                i++;
                continue;
            }
            sleep;
        }
        new Float:total_w;
        for (new i; i < g_count; i++)
        {
            total_w = total_w + (widths[i] + g_spacing);
        }
        total_w = total_w - g_spacing;
        new Float:x = -total_w / 2.0;
        for (new i; i < g_count; i++)
        {
            SetPos(g_labels[i], x, 0.0, 0.0, 0);
            x = x + (widths[i] + g_spacing);
        }
    }
    sleep;
    update_color(100.0, 0.0);
    new pushed;
    new changed;
    for (; ; )
    {
        new msg;
        new a;
        new b;
        /*
         * "FOCS" (direction): the focus arrives or leaves ("FOUT"); "FADE" (alpha, duration);
         * "ENBL" (index, enabled); "PUSH" and "CHNG": the manager has handled the last press or change.
         */
        while (RecvMessage(msg, a, b))
        {
            if (msg == TAG("FOCS"))
            {
                receive_focus(a);
            }
            else if (msg == TAG("FADE"))
            {
                meta_alpha = a;
                update_color(b, 0.0);
            }
            else if (msg == TAG("ENBL"))
            {
                g_enabled[a] = b;
                update_color(100.0, 0.0);
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
        /*
         * A direction past the ends asks the manager to move the focus ("FOCS"); the circle on an enabled button
         * selects it ("CHNG" select), another button is a press ("PUSH" select, button).
         */
        if (active && !pushed && !changed)
        {
            select = clamp(select, 0, g_count - 1);
            cursor = clamp(cursor, 0, g_count - 1);
            new pressed;
            PadState(_, pressed, _, _, -1);
            if (pressed & PAD_LEFT)
            {
                if (cursor - 1 >= 0)
                {
                    cursor--;
                    send_label_focus(focus, g_labels[cursor], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                    sndSePlay(SE_CURSOR);
                    update_color(100.0, 0.0);
                }
                else
                {
                    SendMessage(manager, TAG("FOCS"), TAG("LEFT"), 0, 0, 0);
                    lose_focus();
                    goto next_frame;
                }
            }
            else if (pressed & PAD_RIGHT)
            {
                if (cursor + 1 < g_count)
                {
                    cursor++;
                    sndSePlay(SE_CURSOR);
                    send_label_focus(focus, g_labels[cursor], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                    update_color(100.0, 0.0);
                }
                else
                {
                    SendMessage(manager, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
                    lose_focus();
                    goto next_frame;
                }
            }
            else
            {
                if (pressed & PAD_UP)
                {
                    SendMessage(manager, TAG("FOCS"), TAG("UP"), 0, 0, 0);
                    lose_focus();
                    goto next_frame;
                }
                if (pressed & PAD_DOWN)
                {
                    SendMessage(manager, TAG("FOCS"), TAG("DOWN"), 0, 0, 0);
                    lose_focus();
                    goto next_frame;
                }
            }
            if (pressed & PAD_CIRCLE)
            {
                if (g_enabled[cursor])
                {
                    if (select != cursor)
                    {
                        select = cursor;
                        sndSePlay(SE_DECIDE);
                        update_color(400.0, 0.0);
                        SendMessage(manager, TAG("CHNG"), select, 0, 0, 0);
                        changed = 1;
                        goto next_frame;
                    }
                    if (!disable_selected)
                    {
                        sndSePlay(SE_DECIDE);
                        SendMessage(manager, TAG("CHNG"), select, 0, 0, 0);
                    }
                }
                else
                {
                }
            }
            else
            {
                if (pressed & PAD_CROSS)
                {
                    SendMessage(manager, TAG("PUSH"), select, TAG("RDWN"), 0, 0);
                    pushed = 1;
                    goto next_frame;
                }
                if (pressed & PAD_TRIANGLE)
                {
                    SendMessage(manager, TAG("PUSH"), select, TAG("RUP"), 0, 0);
                    pushed = 1;
                    goto next_frame;
                }
                if (pressed & PAD_SQUARE)
                {
                    SendMessage(manager, TAG("PUSH"), select, TAG("RLFT"), 0, 0);
                    pushed = 1;
                    goto next_frame;
                }
                if (pressed & PAD_SELECT)
                {
                    SendMessage(manager, TAG("PUSH"), select, TAG("SLCT"), 0, 0);
                    pushed = 1;
                    goto next_frame;
                }
                if (pressed & PAD_START)
                {
                    SendMessage(manager, TAG("PUSH"), select, TAG("STRT"), 0, 0);
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

/* Fades the labels to their colors (dimmed when disabled) and shows the mark of the selected button. */
update_color(Float:duration, Float:delay)
{
    for (new i; i < g_count; i++)
    {
        {
            new Float:color[4];
            color[0] = 0.215;
            color[1] = 0.708;
            color[2] = 0.999;
            color[3] = 1.0;
            color[3] = 1.0; /* redundant as written */
            if (active && cursor == i)
            {
                color[3] = 1.0;
            }
            if (!g_enabled[i])
            {
                color[0] = 0.03;
                color[1] = 0.18;
                color[2] = 0.36;
                color[3] = 1.0;
            }
            AnimColor(g_labels[i], color[0], color[1], color[2], color[3] * meta_alpha, duration, delay,
                    ANIM_FUNC_LINEAR);
        }
        new Float:mark_alpha = 1.0;
        if (i != select)
        {
            mark_alpha = 0.0;
        }
        AnimColor(g_marks[i], 1.0, 1.0, 1.0, mark_alpha * meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
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
        cursor = 0;
        sndSePlay(SE_CURSOR);
    }
    else if (direction == TAG("RIGT"))
    {
        cursor = g_count - 1;
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
    send_label_focus(focus, g_labels[cursor], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    update_color(100.0, 0.0);
    show_pad_help();
}

/* Leaves the focus and dims the colors. */
lose_focus()
{
    active = 0;
    update_color(100.0, 40.0);
}

/* Shows the pad help of padhelp and the circle. */
show_pad_help()
{
    new buttons = padhelp;
    buttons = buttons | PADHELP_CIRCLE;
    SetSystemParam(SYS_PADHELP, buttons);
}
