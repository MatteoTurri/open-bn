/*
 * /opt0/bn/script/music/music_radio.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A column of up to ten buttons (arguments "btn0" to "btn9"), each label with a color mark of its own. Up and down
 * move the cursor and the circle selects; at the ends, and with the other buttons, it talks to its manager.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "player.inc"

#include "tag.inc"

#include <float>

#include "imgsync.inc"

#include "imgdec.inc"

#include "sysconf.inc"

/* The color of the mark of each button. */
new Float:g_mark_colors[7][3] = {
    {0.0, 0.0, 0.75}, {0.15, 0.6, 0.11}, {0.75, 0.75}, {0.75, 0.39}, {0.59, 0.1, 0.12},
    {0.75, 0.0, 0.75}, {0.39, 0.05, 0.7}
};
public manager; /* the object told of focus moves, changes and presses */
public focus; /* the focus object */
public padhelp; /* pad help buttons shown besides the circle and the cross while the group has the focus */
public active; /* 1 while the group has the focus */
public select; /* the selected button */
public cursor; /* the button under the focus */
public Float:meta_alpha = 1.0; /* overall alpha, set by "FADE" */
new g_labels[10];
new g_marks[10]; /* the color marks, left of the labels */
new g_enabled[10] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
new g_count;
new Float:g_mark_x = -30.0;
new Float:g_mark_size = 40.0;

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
    new mark_tex;
    start_image_decode2("texture/color_label.tm2");
    if (finish_image_decode() == IMG_DONE)
    {
        mark_tex = imgTexID();
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
            SetScale(g_marks[i], g_mark_size, g_mark_size, 1.0);
            /* NOTE: bug: 7 colors for up to 10 buttons: the eighth to the tenth read past the table */
            SetColor(g_marks[i], g_mark_colors[i][0], g_mark_colors[i][1], g_mark_colors[i][2], 0.0);
            SetTexture(g_marks[i], mark_tex, 0, -1);
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
        new Float:interval = 60.0;
        new value[20];
        if (GetArg("interval", value, 20))
        {
            interval = atof(value);
        }
        for (new i; i < g_count; i++)
        {
            new Float:y = interval * i;
            SetPos(g_labels[i], 0.0, y, 0.0, 0);
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
         * "FOCS" (direction): the focus arrives or leaves ("FOUT"); "FADE" (alpha, duration); "ENBL" (index,
         * enabled); "PUSH" and "CHNG": the manager has handled the last press or change.
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
            if (pressed & PAD_UP)
            {
                if (cursor - 1 >= 0)
                {
                    cursor--;
                    sndSePlay(SE_CURSOR);
                    send_label_focus(focus, g_labels[cursor], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                    update_color(100.0, 0.0);
                }
                else
                {
                    SendMessage(manager, TAG("FOCS"), TAG("UP"), 0, 0, 0);
                    lose_focus();
                    goto next_frame;
                }
            }
            else if (pressed & PAD_DOWN)
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
                    SendMessage(manager, TAG("FOCS"), TAG("DOWN"), 0, 0, 0);
                    lose_focus();
                    goto next_frame;
                }
            }
            else
            {
                if (pressed & PAD_LEFT)
                {
                    SendMessage(manager, TAG("FOCS"), TAG("LEFT"), 0, 0, 0);
                    lose_focus();
                    goto next_frame;
                }
                if (pressed & PAD_RIGHT)
                {
                    SendMessage(manager, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
                    lose_focus();
                    goto next_frame;
                }
            }
            if (pressed & PAD_CIRCLE)
            {
                if (g_enabled[cursor])
                {
                    select = cursor;
                    sndSePlay(SE_DECIDE);
                    update_color(100.0, 0.0);
                    SendMessage(manager, TAG("CHNG"), select, 0, 0, 0);
                    changed = 1;
                    goto next_frame;
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

/* Fades the labels and the marks to their alpha; the mark of a disabled button is dimmed. */
update_color(Float:duration, Float:delay)
{
    for (new i; i < g_count; i++)
    {
        {
            new Float:label_alpha = 1.0;
            if (active && cursor == i) /* redundant as written */
            {
                label_alpha = 1.0;
            }
            if (!g_enabled[i])
            {
                label_alpha = 1.0;
            }
            AnimColor(g_labels[i], 0.215, 0.708, 0.999, label_alpha * meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
        }
        new Float:mark_alpha = 1.0;
        if (!g_enabled[i])
        {
            mark_alpha = 0.3;
        }
        /* NOTE: bug: 7 colors for up to 10 buttons: the eighth to the tenth read past the table */
        AnimColor(g_marks[i], g_mark_colors[i][0], g_mark_colors[i][1], g_mark_colors[i][2], mark_alpha * meta_alpha,
                duration, delay, ANIM_FUNC_LINEAR);
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

/* Shows the circle, the cross and the buttons of padhelp in the pad help. */
show_pad_help()
{
    new buttons = padhelp;
    buttons = buttons | (PADHELP_CIRCLE | PADHELP_CROSS);
    SetSystemParam(SYS_PADHELP, buttons);
}

#include "playerctl.inc"
