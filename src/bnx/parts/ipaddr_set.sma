/*
 * /opt0/bn/script/parts/ipaddr_set.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * An IP address field. The circle starts editing: left and right pick a number, up and down change it, faster
 * while held; the circle keeps the address and the cross restores it. Not editing, it is a part like a button.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

#include "wait.inc"

#include "tag.inc"

new Float:g_pi = 3.141592;

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

new g_shadow_colors[4] = {0x5F000000, 0x5F000000, 0x5F000000, 0x5F000000}; /* the "SCL4" of the shadowed labels */
public manager; /* the object told of focus moves, changes and presses */
public focus; /* the focus object */
public padhelp; /* pad help buttons shown besides the circle while the field has the focus */
public active; /* 1 while the field has the focus */
public editting; /* 1 while the address is being edited */
public cursor; /* the number being edited, 0 to 3 */
public Float:meta_alpha = 1.0; /* overall alpha, set by "FADE" */
public ip0; /* the four numbers of the address */
public ip1; /* second number */
public ip2; /* third number */
public ip3; /* fourth number */
new g_saved_ip0; /* the address when editing started, restored by the cross */
new g_saved_ip1;
new g_saved_ip2;
new g_saved_ip3;
new g_octet_labels[4];
new g_dot_labels[3];
new g_arrows[2]; /* above and below the number being edited, while editing */
new g_arrow_blinks[2]; /* frames left of the blink of an arrow after a step */
new Float:g_octet_x[4] = {-53.0, -3.0, 47.0, 97.0};
new Float:g_dot_x[3] = {-51.0, -1.0, 49.0};

main()
{
    SetLocalScale(Self(), 180.0, 1.0, 1.0);
    for (new i; i < 4; i++)
    {
        g_octet_labels[i] = CreateLabelShadow(Self());
        SetOptionP(g_octet_labels[i], TAG("SCL4"), g_shadow_colors, 4);
        SetOptionI(g_octet_labels[i], TAG("SX64"), 2 * 64);
        SetOptionI(g_octet_labels[i], TAG("SY64"), 2 * 64);
        SetPos(g_octet_labels[i], g_octet_x[i], 0.0, 0.0, 0);
        SetColor(g_octet_labels[i], 0.215, 0.708, 0.999, 0.0);
        SetOptionI(g_octet_labels[i], TAG("ALPH"), BLEND_ADD);
        SetOptionI(g_octet_labels[i], TAG("HALN"), ALIGN_RIGHT);
        SetOptionI(g_octet_labels[i], TAG("VALN"), ALIGN_CENTER);
        SetOptionI(g_octet_labels[i], TAG("FSIZ"), 15);
        sleep;
    }
    for (new i; i < 3; i++)
    {
        g_dot_labels[i] = CreateLabelShadow(Self());
        SetOptionP(g_dot_labels[i], TAG("SCL4"), g_shadow_colors, 4);
        SetOptionI(g_dot_labels[i], TAG("SX64"), 2 * 64);
        SetOptionI(g_dot_labels[i], TAG("SY64"), 2 * 64);
        SetPos(g_dot_labels[i], g_dot_x[i], 0.0, 0.0, 0);
        SetColor(g_dot_labels[i], 0.7843, 0.7843, 0.7843, 0.0);
        SetOptionI(g_dot_labels[i], TAG("ALPH"), BLEND_ADD);
        SetOptionI(g_dot_labels[i], TAG("VALN"), ALIGN_CENTER);
        SetOptionI(g_dot_labels[i], TAG("FSIZ"), 15);
        SetOptionS(g_dot_labels[i], TAG("UTF8"), ".");
        sleep;
    }
    show_address();
    update_color(100.0, 0.0);
    wait_ms(100);
    new pushed;
    new changed;
    for (; ; )
    {
        new msg;
        new a;
        new Float:b;
        /*
         * "FOCS" (direction): the focus arrives or leaves ("FOUT"); "FADE" (alpha, duration); "PUSH" and "CHNG":
         * the manager has handled the last press or change.
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
            new held;
            new pressed;
            new repeated;
            PadState(held, pressed, _, repeated, -1);
            static last_held;
            static held_count;
            if (last_held == held)
            {
                held_count = held_count + 4;
            }
            else
            {
                held_count = 0;
            }
            last_held = held;
            /* The step grows by four each second the same buttons are held. */
            new step = held_count / FrameRate() + 1;
            if (editting)
            {
                if (repeated & PAD_LEFT && cursor - 1 >= 0)
                {
                    cursor--;
                    sndSePlay(SE_CURSOR);
                    send_label_focus2(focus, g_octet_labels[cursor], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                    show_address();
                    update_arrows();
                }
                else if (repeated & PAD_RIGHT && cursor + 1 < 4)
                {
                    cursor++;
                    sndSePlay(SE_CURSOR);
                    send_label_focus2(focus, g_octet_labels[cursor], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                    show_address();
                    update_arrows();
                }
                else if (repeated & PAD_UP)
                {
                    increase_octet(step);
                    blink_arrow(0);
                }
                else if (repeated & PAD_DOWN)
                {
                    decrease_octet(step);
                    blink_arrow(1);
                }
            }
            else
            {
                if (repeated & PAD_LEFT)
                {
                    SendMessage(manager, TAG("FOCS"), TAG("LEFT"), 0, 0, 0);
                    lose_focus();
                    goto next_frame;
                }
                if (repeated & PAD_RIGHT)
                {
                    SendMessage(manager, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
                    lose_focus();
                    goto next_frame;
                }
                if (repeated & PAD_UP)
                {
                    SendMessage(manager, TAG("FOCS"), TAG("UP"), 0, 0, 0);
                    lose_focus();
                    goto next_frame;
                }
                if (repeated & PAD_DOWN)
                {
                    SendMessage(manager, TAG("FOCS"), TAG("DOWN"), 0, 0, 0);
                    lose_focus();
                    goto next_frame;
                }
            }
            if (pressed & PAD_CIRCLE)
            {
                if (editting)
                {
                    editting = 0;
                    cursor = 0;
                    sndSePlay(SE_DECIDE);
                    show_pad_help();
                    update_arrows();
                    send_focus(focus, Self(), 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                    show_address();
                    /* A changed address is a change ("CHNG"). */
                    if (g_saved_ip0 != ip0 || g_saved_ip1 != ip1 || g_saved_ip2 != ip2 || g_saved_ip3 != ip3)
                    {
                        SendMessage(manager, TAG("CHNG"), 0, 0, 0, 0);
                        changed = 1;
                    }
                }
                else
                {
                    editting = 1;
                    cursor = 0;
                    sndSePlay(SE_OPEN);
                    show_pad_help();
                    update_arrows();
                    send_label_focus2(focus, g_octet_labels[cursor], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                    show_address();
                    g_saved_ip0 = ip0;
                    g_saved_ip1 = ip1;
                    g_saved_ip2 = ip2;
                    g_saved_ip3 = ip3;
                }
            }
            else if (pressed & PAD_CROSS)
            {
                if (editting)
                {
                    sndSePlay(SE_CANCEL);
                    ip0 = g_saved_ip0;
                    ip1 = g_saved_ip1;
                    ip2 = g_saved_ip2;
                    ip3 = g_saved_ip3;
                    editting = 0;
                    cursor = 0;
                    send_focus(focus, Self(), 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                    show_address();
                    show_pad_help();
                    update_arrows();
                }
                else
                {
                    SendMessage(manager, TAG("PUSH"), 0, TAG("RDWN"), 0, 0);
                    pushed = 1;
                }
            }
            else if (held & PAD_TRIANGLE)
            {
                SendMessage(manager, TAG("PUSH"), 0, TAG("RUP"), 0, 0);
                pushed = 1;
            }
            else if (held & PAD_SQUARE)
            {
                SendMessage(manager, TAG("PUSH"), 0, TAG("RLFT"), 0, 0);
                pushed = 1;
            }
            else if (held & PAD_SELECT)
            {
                SendMessage(manager, TAG("PUSH"), 0, TAG("SLCT"), 0, 0);
                pushed = 1;
            }
            else if (held & PAD_START)
            {
                SendMessage(manager, TAG("PUSH"), 0, TAG("STRT"), 0, 0);
                pushed = 1;
            }
        }
        next_frame:
        update_arrow_blinks();
        sleep;
    }
    return;
}

/* Fades the numbers to the alpha of the focus state and meta_alpha, the dots to half of it. */
update_color(Float:duration, Float:delay)
{
    new Float:label_alpha = 0.5;
    if (active)
    {
        label_alpha = 1.0;
    }
    new Float:red;
    new Float:green;
    new Float:blue;
    new Float:alpha;
    GetColor(g_octet_labels[0], red, green, blue, alpha);
    AnimColor(g_octet_labels[0], red, green, blue, label_alpha * meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
    AnimColor(g_octet_labels[1], red, green, blue, label_alpha * meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
    AnimColor(g_octet_labels[2], red, green, blue, label_alpha * meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
    AnimColor(g_octet_labels[3], red, green, blue, label_alpha * meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
    AnimColor(g_dot_labels[0], 0.7843, 0.7843, 0.7843, 0.5 * meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
    AnimColor(g_dot_labels[1], 0.7843, 0.7843, 0.7843, 0.5 * meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
    AnimColor(g_dot_labels[2], 0.7843, 0.7843, 0.7843, 0.5 * meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
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
        cursor = 3;
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
    if (editting)
    {
        send_label_focus2(focus, g_octet_labels[cursor], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    }
    else
    {
        send_focus(focus, Self(), 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    }
    update_color(100.0, 0.0);
    show_pad_help();
}

/* Leaves the focus and dims the colors. */
lose_focus()
{
    active = 0;
    update_color(100.0, 40.0);
}

show_address()
{
    new text[8];
    sprintf(text, "%3d", ip0);
    SetOptionS(g_octet_labels[0], TAG("UTF8"), text);
    sprintf(text, "%3d", ip1);
    SetOptionS(g_octet_labels[1], TAG("UTF8"), text);
    sprintf(text, "%3d", ip2);
    SetOptionS(g_octet_labels[2], TAG("UTF8"), text);
    sprintf(text, "%3d", ip3);
    SetOptionS(g_octet_labels[3], TAG("UTF8"), text);
}

new Float:g_arrow_y[2] = {-14.0, 14.0};

/* Adds step to the number under the cursor, up to 255. */
increase_octet(step)
{
    switch (cursor)
    {
        case 0:
        {
            ip0 = ip0 + step;
            if (ip0 > 255)
            {
                ip0 = 255;
            }
            else
            {
                sndSePlay(SE_CURSOR);
            }
        }
        case 1:
        {
            ip1 = ip1 + step;
            if (ip1 > 255)
            {
                ip1 = 255;
            }
            else
            {
                sndSePlay(SE_CURSOR);
            }
        }
        case 2:
        {
            ip2 = ip2 + step;
            if (ip2 > 255)
            {
                ip2 = 255;
            }
            else
            {
                sndSePlay(SE_CURSOR);
            }
        }
        case 3:
        {
            ip3 = ip3 + step;
            if (ip3 > 255)
            {
                ip3 = 255;
            }
            else
            {
                sndSePlay(SE_CURSOR);
            }
        }
    }
    send_label_focus2(focus, g_octet_labels[cursor], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    show_address();
}

/* Subtracts step from the number under the cursor, down to 0. */
decrease_octet(step)
{
    switch (cursor)
    {
        case 0:
        {
            ip0 = ip0 - step;
            if (ip0 < 0)
            {
                ip0 = 0;
            }
            else
            {
                sndSePlay(SE_CURSOR);
            }
        }
        case 1:
        {
            ip1 = ip1 - step;
            if (ip1 < 0)
            {
                ip1 = 0;
            }
            else
            {
                sndSePlay(SE_CURSOR);
            }
        }
        case 2:
        {
            ip2 = ip2 - step;
            if (ip2 < 0)
            {
                ip2 = 0;
            }
            else
            {
                sndSePlay(SE_CURSOR);
            }
        }
        case 3:
        {
            ip3 = ip3 - step;
            if (ip3 < 0)
            {
                ip3 = 0;
            }
            else
            {
                sndSePlay(SE_CURSOR);
            }
        }
    }
    send_label_focus2(focus, g_octet_labels[cursor], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    show_address();
}

/* Shows the pad help: the circle, and the cross while editing. */
show_pad_help()
{
    new buttons = padhelp;
    buttons = buttons | PADHELP_CIRCLE;
    if (editting)
    {
        buttons = buttons | PADHELP_CROSS;
    }
    SetSystemParam(SYS_PADHELP, buttons);
}

/* Creates the arrows and moves them to the cursor while editing, destroys them otherwise. */
update_arrows()
{
    if (editting)
    {
        if (!g_arrows[0])
        {
            g_arrows[0] = CreatePlane(Self());
            SetPos(g_arrows[0], -18.0 + g_octet_x[cursor], g_arrow_y[0], 0.0, 0);
            SetOptionI(g_arrows[0], TAG("ALPH"), BLEND_ADD);
            SetScale(g_arrows[0], 17.0, 8.0, 1.0);
            SetColor(g_arrows[0], 1.0, 1.0, 1.0, 0.0);
            SetRot(g_arrows[0], 0.0, 0.0, g_pi, 0, 0);
            SetTexture(g_arrows[0], imgGetResource(RES_SPIN), 0, -1);
        }
        if (!g_arrows[1])
        {
            g_arrows[1] = CreatePlane(Self());
            SetOptionI(g_arrows[1], TAG("ALPH"), BLEND_ADD);
            SetPos(g_arrows[1], -18.0 + g_octet_x[cursor], g_arrow_y[1], 0.0, 0);
            SetScale(g_arrows[1], 17.0, 8.0, 1.0);
            SetColor(g_arrows[1], 1.0, 1.0, 1.0, 0.0);
            SetTexture(g_arrows[1], imgGetResource(RES_SPIN), 0, -1);
        }
        AnimMoveLine(g_arrows[0], -18.0 + g_octet_x[cursor], g_arrow_y[0], 0.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
        AnimMoveLine(g_arrows[1], -18.0 + g_octet_x[cursor], g_arrow_y[1], 0.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(g_arrows[0], 1.0, 1.0, 1.0, 1.0, 1000.0, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(g_arrows[1], 1.0, 1.0, 1.0, 1.0, 1000.0, 0.0, ANIM_FUNC_LINEAR);
    }
    else
    {
        if (g_arrows[0])
        {
            Destroy(g_arrows[0]);
            g_arrows[0] = 0;
        }
        if (g_arrows[1])
        {
            Destroy(g_arrows[1]);
            g_arrows[1] = 0;
        }
    }
}

/* Starts a blink of arrow index (0 up, 1 down) unless one is running. */
blink_arrow(index)
{
    if (!g_arrow_blinks[index])
    {
        index = clamp(index, 0, 1);
        g_arrow_blinks[index] = ms_to_frames(100) * 2;
        AnimColor(g_arrows[index], 1.0, 1.0, 1.0, 0.0, float(100), 0.0, ANIM_FUNC_LINEAR);
    }
}

update_arrow_blinks()
{
    for (new i; i < 2; i++)
    {
        if (g_arrows[i] && g_arrow_blinks[i])
        {
            g_arrow_blinks[i]--;
            if (g_arrow_blinks[i] == ms_to_frames(100))
            {
                AnimColor(g_arrows[i], 1.0, 1.0, 1.0, 1.0, float(100), 0.0, ANIM_FUNC_LINEAR);
            }
        }
    }
}
