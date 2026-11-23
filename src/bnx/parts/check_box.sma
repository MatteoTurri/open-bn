/*
 * /opt0/bn/script/parts/check_box.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A check box with a label (argument "text"), toggled with the circle button. While it has the focus it passes
 * the direction buttons to its manager as focus moves and the other buttons as presses.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

#include "wait.inc"

#include "tag.inc"

/* Sleeps until a message msg arrives. */
wait_message(msg, &a=0, &b=0, &c=0, &d=0)
{
    while (!FindMessage(msg, a, b, c, d))
    {
        sleep;
    }
}

#include "sysconf.inc"

public manager; /* the object told of focus moves, changes and presses */
public focus; /* the focus object */
public padhelp; /* pad help buttons shown besides the circle while the box has the focus */
public active; /* 1 while the box has the focus */
public checked; /* the state of the box, 0 or 1 */
public Float:meta_alpha = 1.0; /* overall alpha, set by "FADE" */
new g_label;
new g_box;
new Float:g_box_x = -30.0; /* the box is left of the label */
new Float:g_box_size = 20.0;

main()
{
    {
        new key[64];
        new text[64];
        sprintf(key, "text");
        if (GetArg(key, text, 64))
        {
            g_label = CreateLabel(Self());
            SetPos(g_label, 0.0, 0.0, 0.0, 0);
            SetColor(g_label, 0.215, 0.708, 0.999, 0.0);
            SetOptionS(g_label, TAG("UTF8"), text);
            SetOptionI(g_label, TAG("HALN"), ALIGN_LEFT);
            SetOptionI(g_label, TAG("VALN"), ALIGN_CENTER);
            SetOptionI(g_label, TAG("FSIZ"), 15);
            g_box = CreatePlane(g_label);
            SetPos(g_box, g_box_x, 0.0, 0.0, 0);
            SetScale(g_box, g_box_size, g_box_size, 1.0);
            SetColor(g_box, 1.0, 1.0, 1.0, 0.0);
            SetOptionI(g_box, TAG("ALPH"), BLEND_ADD);
        }
    }
    if (checked)
    {
        SetTexture(g_box, imgGetResource(RES_CHECK1), 0, -1);
    }
    else
    {
        SetTexture(g_box, imgGetResource(RES_CHECK0), 0, -1);
    }
    update_color(100.0, 0.0);
    wait_ms(100);
    for (; ; )
    {
        new msg;
        new a;
        new Float:b;
        /* "FOCS" (direction): the focus arrives or leaves ("FOUT"); "FADE" (alpha, duration). */
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
        }
        /*
         * A direction asks the manager to move the focus ("FOCS"); the circle toggles the box ("CHNG" checked),
         * another button is a press ("PUSH" checked, button). Both wait for the manager to echo the message.
         */
        if (active)
        {
            new pressed;
            PadState(_, pressed, _, _, -1);
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
            if (pressed & PAD_CIRCLE)
            {
                checked = !checked;
                sndSePlay(SE_DECIDE);
                if (checked)
                {
                    SetTexture(g_box, imgGetResource(RES_CHECK1), 0, -1);
                }
                else
                {
                    SetTexture(g_box, imgGetResource(RES_CHECK0), 0, -1);
                }
                SendMessage(manager, TAG("CHNG"), checked, 0, 0, 0);
                wait_message(TAG("CHNG"));
            }
            else if (pressed & PAD_CROSS)
            {
                SendMessage(manager, TAG("PUSH"), checked, TAG("RDWN"), 0, 0);
                wait_message(TAG("PUSH"));
            }
            else if (pressed & PAD_TRIANGLE)
            {
                SendMessage(manager, TAG("PUSH"), checked, TAG("RUP"), 0, 0);
                wait_message(TAG("PUSH"));
            }
            else if (pressed & PAD_SQUARE)
            {
                SendMessage(manager, TAG("PUSH"), checked, TAG("RLFT"), 0, 0);
                wait_message(TAG("PUSH"));
            }
            else if (pressed & PAD_SELECT)
            {
                SendMessage(manager, TAG("PUSH"), checked, TAG("SLCT"), 0, 0);
                wait_message(TAG("PUSH"));
            }
            else if (pressed & PAD_START)
            {
                SendMessage(manager, TAG("PUSH"), checked, TAG("STRT"), 0, 0);
                wait_message(TAG("PUSH"));
            }
        }
        next_frame:
        sleep;
    }
}

/* Fades the label and the box to their colors for the focus state and meta_alpha. */
update_color(Float:duration, Float:delay)
{
    {
        new Float:label_alpha = 0.5;
        if (active)
        {
            label_alpha = 1.0;
        }
        AnimColor(g_label, 0.215, 0.708, 0.999, label_alpha * meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
    }
    AnimColor(g_box, 1.0, 1.0, 1.0, meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
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
        sndSePlay(SE_CURSOR);
    }
    else if (direction == TAG("DOWN"))
    {
        sndSePlay(SE_CURSOR);
    }
    else if (direction == TAG("FRST"))
    {
    }
    send_label_focus(focus, g_label, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
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
