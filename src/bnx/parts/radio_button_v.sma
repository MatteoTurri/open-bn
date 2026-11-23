/*
 * /opt0/bn/script/parts/radio_button_v.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A column of up to ten radio buttons (arguments "btn0" to "btn9"), their labels drawn as images. Up and down
 * move the cursor and the circle selects; at the ends, and with the other buttons, it talks to its manager.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "tag.inc"

#include <float>

#include "imgsync.inc"

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

public manager; /* the object told of focus moves, changes and presses */
public focus; /* the focus object */
public padhelp; /* pad help buttons shown besides the circle while the group has the focus */
public active; /* 1 while the group has the focus */
public select; /* the selected button */
public cursor; /* the button under the focus */
public Float:meta_alpha = 1.0; /* overall alpha, set by "FADE" */
new g_labels[10];
new g_marks[10]; /* the radio marks, shown on the selected button */
new g_enabled[10] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
new g_count;
new Float:g_label_red = 0.215; /* the color of the labels, set by "FCOL" */
new Float:g_label_green = 0.708;
new Float:g_label_blue = 0.999;
public disable_selected = 1; /* 0: the circle on the selected button sends "CHNG" again */
public se_decide = SE_DECIDE; /* sound effects */
public se_focusin = SE_CURSOR; /* sound effect when the focus arrives */
/* The pad buttons each button takes, in the bits of PADHELP_*. */
new g_button_masks[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
new g_fontsize_key[] = "fontsize";

main()
{
    new font_size = 17;
    {
        new value[20];
        if (GetArg(g_fontsize_key, value, 20))
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
            g_labels[i] = CreatePlane(Self());
            SetColor(g_labels[i], 0.0, 0.0, 0.0, 0.0);
            imgDecStringStart(text, font_size, 64, 64, 0);
            if (IMG_DONE != wait_image_decode())
            {
                continue;
            }
            new Float:tex_w = float(imgTexWidth());
            new Float:tex_h = float(imgTexHeight());
            new Float:clip[4] = {2.0, 2.0, 1.0, 1.0};
            clip[0] /= tex_w;
            clip[1] /= tex_h;
            SetPos(g_labels[i], tex_w / 2.0, 0.0, 0.0, 0);
            SetOptionI(g_labels[i], TAG("ZSRT"), ZSORT_OVERLAY);
            SetTexture(g_labels[i], imgTexID(), 0, -1);
            SetOptionI(g_labels[i], TAG("NUM"), 2);
            SetLocalScale(g_labels[i], tex_w, tex_h, 1.0);
            /* Item 1 draws the text, item 0 a dark copy of it as a shadow. */
            new Float:color[4] = {1.0, 1.0, 1.0, 1.0};
            SetItemOptionP(g_labels[i], 1, TAG("COLR"), color, 4);
            SetItemOptionI(g_labels[i], 1, TAG("ALPH"), BLEND_ADD);
            color[0] = 0.0;
            color[1] = 0.0;
            color[2] = 0.0;
            color[3] = 0.371;
            SetItemOptionP(g_labels[i], 0, TAG("CLIP"), clip, 4);
            SetItemOptionP(g_labels[i], 0, TAG("COLR"), color, 4);
            SetItemOptionI(g_labels[i], 0, TAG("ALPH"), BLEND_ALPHA);
            g_marks[i] = CreatePlane(g_labels[i]);
            SetPos(g_marks[i], -15.0 + -tex_w / 2.0, 0.0, 0.0, 0);
            SetScale(g_marks[i], 18.0, 16.0, 1.0);
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
        new Float:spacing = 60.0;
        new value[20];
        if (GetArg("interval", value, 20))
        {
            spacing = atof(value);
        }
        for (new i; i < g_count; i++)
        {
            new Float:y = spacing * i;
            new Float:pos[3];
            GetPos(g_labels[i], pos[0], pos[1], pos[2], 0);
            SetPos(g_labels[i], pos[0], y, pos[2], 0);
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
        new c;
        /*
         * "FOCS" (direction): the focus arrives or leaves ("FOUT"); "FCOL" (r, g, b): the color of the labels;
         * "FADE" (alpha, duration); "ENBL" (index, enabled); "PUSH" and "CHNG": the manager has handled the last
         * press or change; "SLCT" (index): selects a button; "MASK" (index, mask): sets g_button_masks;
         * "DSEL" (flag): sets disable_selected.
         */
        while (RecvMessage(msg, a, b, c))
        {
            if (msg == TAG("FOCS"))
            {
                receive_focus(a);
            }
            else if (msg == TAG("FCOL"))
            {
                g_label_red = a;
                g_label_green = b;
                g_label_blue = c;
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
            else if (msg == TAG("SLCT"))
            {
                select_button(a);
            }
            else if (msg == TAG("MASK"))
            {
                g_button_masks[a] = b;
                if (active)
                {
                    show_pad_help();
                }
            }
            else if (msg == TAG("DSEL"))
            {
                disable_selected = a;
                if (active)
                {
                    show_pad_help();
                }
            }
        }
        /*
         * A direction past the ends asks the manager to move the focus ("FOCS"); the circle on an enabled button
         * selects it ("CHNG" select), another button the button takes is a press ("PUSH" select, button).
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
                    send_focus(focus, g_labels[cursor], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                    update_color(100.0, 0.0);
                    show_pad_help();
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
                    send_focus(focus, g_labels[cursor], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                    update_color(100.0, 0.0);
                    show_pad_help();
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
                if (g_enabled[cursor] && g_button_masks[cursor] & PADHELP_CIRCLE)
                {
                    if (select != cursor)
                    {
                        select = cursor;
                        sndSePlay(se_decide);
                        update_color(400.0, 0.0);
                        SendMessage(manager, TAG("CHNG"), select, 0, 0, 0);
                        changed = 1;
                        goto next_frame;
                    }
                    if (!disable_selected)
                    {
                        sndSePlay(se_decide);
                        SendMessage(manager, TAG("CHNG"), select, 0, 0, 0);
                    }
                }
                else
                {
                }
            }
            else if (pressed & PAD_CROSS)
            {
                if (g_button_masks[cursor] & PADHELP_CROSS)
                {
                    SendMessage(manager, TAG("PUSH"), select, TAG("RDWN"), 0, 0);
                    pushed = 1;
                    goto next_frame;
                }
            }
            else if (pressed & PAD_TRIANGLE)
            {
                if (g_button_masks[cursor] & PADHELP_TRIANGLE)
                {
                    SendMessage(manager, TAG("PUSH"), select, TAG("RUP"), 0, 0);
                    pushed = 1;
                    goto next_frame;
                }
            }
            else if (pressed & PAD_SQUARE)
            {
                if (g_button_masks[cursor] & PADHELP_SQUARE)
                {
                    SendMessage(manager, TAG("PUSH"), select, TAG("RLFT"), 0, 0);
                    pushed = 1;
                    goto next_frame;
                }
            }
            else
            {
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
            color[0] = g_label_red;
            color[1] = g_label_green;
            color[2] = g_label_blue;
            color[3] = 1.0;
            if (active && cursor == i) /* redundant as written */
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
        sndSePlay(se_focusin);
    }
    else if (direction == TAG("RIGT"))
    {
        sndSePlay(se_focusin);
    }
    else if (direction == TAG("UP"))
    {
        cursor = 0;
        sndSePlay(se_focusin);
    }
    else if (direction == TAG("DOWN"))
    {
        cursor = g_count - 1;
        sndSePlay(se_focusin);
    }
    else if (direction == TAG("FRST"))
    {
        printf("got initial focus.\n");
    }
    send_focus(focus, g_labels[cursor], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    update_color(100.0, 0.0);
    show_pad_help();
}

/* Leaves the focus and dims the colors. */
lose_focus()
{
    active = 0;
    update_color(100.0, 40.0);
}

/* Shows the pad help for the buttons the cursor button takes; with disable_selected, not the circle on select. */
show_pad_help()
{
    new buttons = (padhelp | PADHELP_CIRCLE) & g_button_masks[cursor];
    if (disable_selected && cursor == select)
    {
        buttons = buttons & ~PADHELP_CIRCLE;
    }
    SetSystemParam(SYS_PADHELP, buttons);
}

select_button(index)
{
    index = clamp(index, 0, g_count - 1);
    if (!g_enabled[index])
    {
        return;
    }
    if (index == select)
    {
        return;
    }
    select = index;
    update_color(100.0, 0.0);
}
