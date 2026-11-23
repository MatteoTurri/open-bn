/*
 * /opt0/bn/script/parts/button.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A push button: a text label (argument "text") on a texture sized to it. While it has the focus it passes the
 * direction buttons to its manager as focus moves and the other buttons as presses.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "wait.inc"

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

public manager; /* the object told of focus moves and presses */
public focus; /* the focus object */
public padhelp; /* pad help buttons shown besides the circle while the button has the focus */
public active; /* 1 while the button has the focus */
public Float:meta_alpha = 1.0; /* overall alpha, set by "FADE" */
public Float:margin_x = 10.0; /* space between the text and the edges of the button */
public Float:margin_y = 10.0; /* space between the text and the top and bottom edges */
new g_label;
new g_planes[2]; /* the button, and its lit texture flashed when it is pressed */
new g_text[256];
new Float:g_colors[2][3] = {{0.0, 0.54, 0.78}, {0.49, 0.0, 0.25}}; /* only the first is used */

main()
{
    GetArg("text", g_text, 256);
    g_label = CreateLabel(Self());
    SetColor(g_label, 0.215, 0.708, 0.999, 0.0);
    SetOptionI(g_label, TAG("ALPH"), BLEND_ADD);
    SetOptionS(g_label, TAG("UTF8"), g_text);
    SetOptionI(g_label, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(g_label, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_label, TAG("FSIZ"), 15);
    {
        new Float:button_w = 100.0;
        new Float:button_h = 20.0;
        {
            new rect[5];
            for (; ; )
            {
                GetOptionP(g_label, TAG("RECT"), rect, 5);
                if (rect[0])
                {
                    break;
                }
                sleep;
            }
            button_w = rect[3] / 64.0 + margin_x * 2;
            button_h = rect[4] / 64.0 + margin_y * 2;
        }
        g_planes[0] = create_button_plane(button_w, button_h);
        SetTexture(g_planes[0], imgGetResource(RES_BUTTON0), 0, -1);
        g_planes[1] = create_button_plane(button_w, button_h);
        SetTexture(g_planes[1], imgGetResource(RES_BUTTON1), 0, -1);
        Hide(g_planes[1]);
    }
    update_color(100.0, 0.0);
    wait_ms(100);
    new pushed;
    for (; ; )
    {
        new msg;
        new a;
        new Float:b;
        /*
         * "FOCS" (direction): the focus arrives ("LEFT", "RIGT", "UP", "DOWN", "FRST") or leaves ("FOUT");
         * "GTXT": the text is sent back to the manager; "FADE" (alpha, duration); "PUSH": the button can be
         * pressed again.
         */
        while (RecvMessage(msg, a, b))
        {
            if (msg == TAG("FOCS"))
            {
                receive_focus(a);
            }
            else if (msg == TAG("GTXT"))
            {
                SendMessageStr(manager, TAG("GTXT"), g_text);
            }
            else if (msg == TAG("FADE"))
            {
                meta_alpha = a;
                update_color(b, 0.0);
            }
            else if (msg == TAG("PUSH"))
            {
                pushed = 0;
            }
        }
        /* A direction asks the manager to move the focus ("FOCS"); another button is a press ("PUSH"). */
        if (active && !pushed)
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
                sndSePlay(SE_DECIDE);
                Show(g_planes[1]);
                AnimColor(g_planes[0], g_colors[0][0], g_colors[0][1], g_colors[0][2], 0.0, 100.0, 0.0,
                        ANIM_FUNC_LINEAR);
                AnimColor(g_planes[1], g_colors[0][0], g_colors[0][1], g_colors[0][2], 1.0, 100.0, 0.0,
                        ANIM_FUNC_LINEAR);
                wait_ms(100);
                AnimColor(g_planes[0], g_colors[0][0], g_colors[0][1], g_colors[0][2], 1.0, 100.0, 0.0,
                        ANIM_FUNC_LINEAR);
                AnimColor(g_planes[1], g_colors[0][0], g_colors[0][1], g_colors[0][2], 0.0, 100.0, 0.0,
                        ANIM_FUNC_LINEAR);
                wait_ms(100);
                Hide(g_planes[1]);
                SendMessage(manager, TAG("PUSH"), 0, TAG("RRGT"), 0, 0);
                pushed = 1;
                goto next_frame;
            }
            if (pressed & PAD_CROSS)
            {
                SendMessage(manager, TAG("PUSH"), 0, TAG("RDWN"), 0, 0);
                pushed = 1;
                goto next_frame;
            }
            if (pressed & PAD_TRIANGLE)
            {
                SendMessage(manager, TAG("PUSH"), 0, TAG("RUP"), 0, 0);
                pushed = 1;
                goto next_frame;
            }
            if (pressed & PAD_SQUARE)
            {
                SendMessage(manager, TAG("PUSH"), 0, TAG("RLFT"), 0, 0);
                pushed = 1;
                goto next_frame;
            }
            if (pressed & PAD_SELECT)
            {
                SendMessage(manager, TAG("PUSH"), 0, TAG("SLCT"), 0, 0);
                pushed = 1;
                goto next_frame;
            }
            if (pressed & PAD_START)
            {
                SendMessage(manager, TAG("PUSH"), 0, TAG("STRT"), 0, 0);
                pushed = 1;
                goto next_frame;
            }
        }
        next_frame:
        sleep;
    }
    return;
}

/* Fades the label and the button to their colors for the focus state and meta_alpha. */
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
    AnimColor(g_planes[0], g_colors[0][0], g_colors[0][1], g_colors[0][2], meta_alpha, duration, delay,
            ANIM_FUNC_LINEAR);
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
        printf("got initial focus.\n");
    }
    send_focus(focus, g_planes[0], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
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

/* Creates a nine-slice plane for the button, with its corners kept round at any size. */
create_button_plane(Float:button_w, Float:button_h)
{
    new plane = CreatePlane3x3(Self());
    SetScale(plane, button_w, button_h, 1.0);
    SetColor(plane, g_colors[0][0], g_colors[0][1], g_colors[0][2], 0.0);
    SetTexture(plane, imgGetResource(RES_BUTTON0), 0, -1);
    SetOptionI(plane, TAG("ALPH"), BLEND_ADD);
    SetOptionI(plane, TAG("ZSRT"), ZSORT_BACKGROUND);
    {
        new Float:tex_margins[4] = {0.5, 0.5, 0.5, 0.5};
        SetOptionP(plane, TAG("MGNT"), tex_margins, 4);
        new Float:scale[3];
        GetScale(plane, scale[0], scale[1], scale[2]);
        new Float:margins[4];
        if (scale[0] > scale[1])
        {
            margins[0] = margins[1] = 0.5 * scale[1] / scale[0];
            margins[2] = margins[3] = 0.5;
        }
        else
        {
            margins[2] = margins[3] = 0.5 * scale[0] / scale[1];
            margins[0] = margins[1] = 0.5;
        }
        SetOptionP(plane, TAG("MGN"), margins, 4);
    }
    return plane;
}
