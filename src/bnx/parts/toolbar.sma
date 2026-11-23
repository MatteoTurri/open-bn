/*
 * /opt0/bn/script/parts/toolbar.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A row of up to ten image buttons (arguments "btn0" to "btn9", image urls), spaced by "interval"; left and right
 * move the cursor, the other buttons are reported to the manager, and a "repeat<n>" button repeats while held.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "wait.inc"

#include "tag.inc"

#include <float>

#include "imgsync.inc"

/* Sleeps until a message msg arrives. */
wait_message(msg, &a=0, &b=0, &c=0, &d=0)
{
    while (!FindMessage(msg, a, b, c, d))
    {
        sleep;
    }
}

/* Same as start_image_decode of text.inc. */
start_image_decode(url[])
{
    return imgDecStart(url, 0, 0, -1, 100, 0);
}

#include "imgdec.inc"

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
public padhelp;         /* pad help buttons shown besides the circle while the bar has the focus */
public padhelp_disable; /* pad help buttons taken away */
public active;          /* 1 while the bar has the focus */
public cursor;          /* the button under the focus */
public alpha_blend_mode = BLEND_ALPHA; /* the "ALPH" option of the buttons */
public Float:meta_alpha = 1.0;         /* overall alpha, set by "FADE" */
new g_buttons[10];
new g_repeats[10]; /* the button repeats its press while the circle is held */
new g_count;
new Float:g_default_interval = 70.0;
new g_repeating;   /* the circle is held on a repeating button */
new g_blink_off;
new g_blink_timer; /* ms to the next blink of the held button */
public expand = 1; /* 1: keep the size of a button and fit the image in it; 0: the size of the image */

#include "misc.inc"

main()
{
    new tex_size = 32;
    {
        new value[32];
        if (GetArg("tex_size", value, 32))
        {
            tex_size = atoi(value);
        }
        if (GetArg("interval", value, 32))
        {
            g_default_interval = atof(value);
        }
    }
    for (new i; i < 10; i++)
    {
        new key[64];
        new url[64];
        sprintf(key, "btn%d", i);
        if (GetArg(key, url, 64))
        {
            g_buttons[i] = CreatePlane(Self());
            SetLocalScale(g_buttons[i], 70.0, 70.0, 70.0);
            SetPos(g_buttons[i], 0.0, 0.0, 0.0, 0);
            SetColor(g_buttons[i], 1.0, 1.0, 1.0, 0.0);
            SetOptionI(g_buttons[i], TAG("ALPH"), alpha_blend_mode);
            load_button_texture(g_buttons[i], url, tex_size, tex_size, -1);
            g_count++;
        }
        else
        {
            break;
        }
    }
    for (new i; i < 10; i++)
    {
        new key[64];
        new value[64];
        sprintf(key, "repeat%d", i);
        if (GetArg(key, value, 64))
        {
            g_repeats[i] = 1;
        }
    }
    {
        new Float:x;
        for (new i; i < g_count - 1; i++)
        {
            x = x + get_interval(i);
        }
        x = -0.5 * x;
        for (new i; i < g_count; i++)
        {
            SetPos(g_buttons[i], x, 0.0, 0.0, 0);
            x = x + get_interval(i);
        }
    }
    update_color(100.0, 0.0);
    for (; ; )
    {
        new msg;
        new a;
        new Float:b;
        /* "FOCS" (direction), "FADE" (alpha, duration), "TEX" (index, texture): a new image for a button */
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
            else if (msg == TAG("TEX"))
            {
                SetTexture(g_buttons[a], b, 0, -1);
            }
        }
        if (active)
        {
            cursor = clamp(cursor, 0, g_count - 1);
            new held;
            new pressed;
            PadState(held, pressed, _, _, -1);
            if (!g_repeating)
            {
                if (pressed & PAD_LEFT)
                {
                    if (cursor - 1 >= 0)
                    {
                        cursor--;
                        sndSePlay(SE_CURSOR);
                        send_focus(focus, g_buttons[cursor], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
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
                        send_focus(focus, g_buttons[cursor], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
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
            }
            /*
             * Every press is a "PUSH" (cursor, button) to the manager, answered by a "PUSH"; a repeating button
             * blinks while the circle is held and sends a last "PUSH" (cursor, button, 1) when it is let go.
             */
            if (pressed & PAD_CIRCLE)
            {
                if (g_repeats[cursor])
                {
                    SendMessage(manager, TAG("PUSH"), cursor, TAG("RRGT"), 0, 0);
                    wait_message(TAG("PUSH"));
                    g_repeating = 1;
                    g_blink_off = 0;
                    g_blink_timer = 0;
                }
                else
                {
                    SendMessage(manager, TAG("PUSH"), cursor, TAG("RRGT"), 0, 0);
                    sleep;
                    sndSePlay(SE_DECIDE);
                    AnimColor(g_buttons[cursor], 1.0, 1.0, 1.0, 0.0 * meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
                    wait_ms(100);
                    AnimColor(g_buttons[cursor], 1.0, 1.0, 1.0, 1.0 * meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
                    wait_ms(100);
                    AnimColor(g_buttons[cursor], 1.0, 1.0, 1.0, 0.0 * meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
                    wait_ms(100);
                    AnimColor(g_buttons[cursor], 1.0, 1.0, 1.0, 1.0 * meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
                    wait_ms(100);
                    AnimColor(g_buttons[cursor], 1.0, 1.0, 1.0, 0.0 * meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
                    wait_ms(100);
                    AnimColor(g_buttons[cursor], 1.0, 1.0, 1.0, 1.0 * meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
                    wait_ms(100);
                    wait_message(TAG("PUSH"));
                    wait_ms(100);
                }
            }
            else if (pressed & PAD_CROSS)
            {
                SendMessage(manager, TAG("PUSH"), cursor, TAG("RDWN"), 0, 0);
                wait_message(TAG("PUSH"));
            }
            else if (pressed & PAD_TRIANGLE)
            {
                SendMessage(manager, TAG("PUSH"), cursor, TAG("RUP"), 0, 0);
                wait_message(TAG("PUSH"));
            }
            else if (pressed & PAD_SQUARE)
            {
                SendMessage(manager, TAG("PUSH"), cursor, TAG("RLFT"), 0, 0);
                wait_message(TAG("PUSH"));
            }
            else if (pressed & PAD_SELECT)
            {
                SendMessage(manager, TAG("PUSH"), cursor, TAG("SLCT"), 0, 0);
                wait_message(TAG("PUSH"));
            }
            else if (pressed & PAD_START)
            {
                SendMessage(manager, TAG("PUSH"), cursor, TAG("STRT"), 0, 0);
                wait_message(TAG("PUSH"));
            }
            if (g_repeating)
            {
                g_blink_timer = g_blink_timer - 1000 / FrameRate();
                if (g_blink_timer < 0)
                {
                    g_blink_timer = 100;
                    if (0 == g_blink_off)
                    {
                        g_blink_off = 1;
                        AnimColor(g_buttons[cursor], 1.0, 1.0, 1.0, 0.0 * meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
                    }
                    else
                    {
                        g_blink_off = 0;
                        AnimColor(g_buttons[cursor], 1.0, 1.0, 1.0, 1.0 * meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
                    }
                }
                if (!(held & PAD_CIRCLE))
                {
                    g_repeating = 0;
                    AnimColor(g_buttons[cursor], 1.0, 1.0, 1.0, 1.0 * meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
                    SendMessage(manager, TAG("PUSH"), cursor, TAG("RRGT"), 1, 0);
                    wait_message(TAG("PUSH"));
                }
            }
        }
        next_frame:
        sleep;
    }
    return;
}

/* Shows the button under the cursor, while the bar has the focus, brighter than the others. */
update_color(Float:duration, Float:delay)
{
    for (new i; i < g_count; i++)
    {
        new Float:alpha = 0.5;
        if (active && cursor == i)
        {
            alpha = 1.0;
        }
        AnimColor(g_buttons[i], 1.0, 1.0, 1.0, alpha * meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
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
    }
    send_focus(focus, g_buttons[cursor], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    update_color(100.0, 0.0);
    show_pad_help();
}

/* Leaves the focus and dims the colors. */
lose_focus()
{
    active = 0;
    update_color(100.0, 40.0);
}

/* Same as has_extension of string.inc. */
has_extension(name[], ext[])
{
    new len = strlen(name);
    new ext_len = strlen(ext);
    for (new i; i < ext_len; i++)
    {
        if (toupper(name{len - 1 - i}) != toupper(ext{ext_len - 1 - i}))
        {
            return 0;
        }
    }
    if ('.' != toupper(name{len - 1 - ext_len}))
    {
        return 0;
    }
    return 1;
}

/* Decodes the image at url (a .tm2 at its own size) as the texture of obj, and sizes obj after expand. */
load_button_texture(obj, url[], tex_w, tex_h, dec_mode)
{
    if (0 == strlen(url))
    {
        return;
    }
    new path[256];
    sprintf(path, url);
    if (has_extension(path, "tm2"))
    {
        start_image_decode(path);
        new status = finish_image_decode();
        if (IMG_DONE != status)
        {
            return;
        }
    }
    else
    {
        imgDecStart(path, tex_w, tex_h, dec_mode, 100, 0);
        new status = wait_image_decode();
        if (IMG_DONE != status)
        {
            return;
        }
    }
    SetTexture(obj, imgTexID(), 0, -1);
    new Float:aspect = 1.0 * imgTexOrgWidth() / imgTexOrgHeight();
    if (expand)
    {
        new Float:scale[3];
        GetLocalScale(obj, scale[0], scale[1], scale[2]);
        if (aspect > 1.0)
        {
            SetLocalScale(obj, scale[0], scale[1] / aspect, scale[2]);
        }
        else
        {
            SetLocalScale(obj, scale[0] * aspect, scale[1], scale[2]);
        }
    }
    else
    {
        SetLocalScale(obj, float(imgTexOrgWidth()), float(imgTexOrgHeight()), 1.0);
    }
    return;
}

/* Shows the pad help of padhelp and the circle, less the buttons of padhelp_disable. */
show_pad_help()
{
    new buttons = padhelp;
    buttons = buttons | PADHELP_CIRCLE;
    buttons = buttons & ~padhelp_disable;
    SetSystemParam(SYS_PADHELP, buttons);
}
