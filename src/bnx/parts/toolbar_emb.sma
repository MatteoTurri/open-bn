/*
 * /opt0/bn/script/parts/toolbar_emb.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A row of up to ten embossed image buttons, each of three layers (arguments "btn<n>0" added, "btn<n>1"
 * subtracted, "btn<n>2" the highlight under the cursor); left and right move the cursor, the other buttons are
 * reported to the manager.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "wait.inc"

#include "tag.inc"

#include <float>

#include "imgsync.inc"

#include "imgdec.inc"

public manager; /* the object told of focus moves and presses */
public focus; /* unused, but part of the layout */
public padhelp;         /* pad help buttons shown besides the circle while the bar has the focus */
public padhelp_disable; /* pad help buttons taken away */
public active;          /* 1 while the bar has the focus */
public cursor;          /* the button under the focus */
public alpha_blend_mode = BLEND_ALPHA; /* unused, but part of the layout */
public Float:meta_alpha = 1.0;         /* overall alpha, set by "FADE" */
new g_layers[10][3];
new g_enabled[10]; /* a disabled button is dimmed and ignores the circle */
new g_count;
new Float:g_default_interval = 70.0;
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
    {
        new i;
        for (; i < 10; i++)
        {
            g_enabled[i] = 1;
            {
                new layer = 2;
                for (; layer >= 0; layer--)
                {
                    new key[64];
                    new url[64];
                    sprintf(key, "btn%d%d", i, layer);
                    if (GetArg(key, url, 64))
                    {
                        g_layers[i][layer] = CreatePlane(Self());
                        SetLocalScale(g_layers[i][layer], 70.0, 70.0, 70.0);
                        SetPos(g_layers[i][layer], 0.0, 0.0, 0.0, 0);
                        SetColor(g_layers[i][layer], 1.0, 1.0, 1.0, 0.0);
                        if (0 == layer)
                        {
                            SetOptionI(g_layers[i][layer], TAG("ALPH"), BLEND_ADD);
                        }
                        else if (layer == 1)
                        {
                            SetOptionI(g_layers[i][layer], TAG("ALPH"), BLEND_SUB);
                        }
                        else if (layer == 2)
                        {
                            SetOptionI(g_layers[i][layer], TAG("ALPH"), BLEND_ALPHA);
                        }
                        load_button_texture(g_layers[i][layer], url, tex_size, tex_size, -1);
                        g_count++;
                    }
                    else
                    {
                        goto buttons_loaded;
                    }
                }
            }
        }
    }
    buttons_loaded:
    g_count = g_count / 3;
    {
        new Float:x;
        for (new i; i < g_count - 1; i++)
        {
            x = x + get_interval(i);
        }
        x = -0.5 * x;
        for (new i; i < g_count; i++)
        {
            for (new layer; layer < 3; layer++)
            {
                SetPos(g_layers[i][layer], x, 0.0, 0.0, 0);
            }
            x = x + get_interval(i);
        }
    }
    update_color(1.0, 100.0, 0.0, -1);
    new pushed;
    for (; ; )
    {
        new msg;
        new a;
        new b;
        /*
         * "FOCS" (direction); "PUSH": the manager has handled the last press; "FADE" (alpha, duration);
         * "TEX" (index, texture): a new highlight for a button; "ENBL" (index, enabled)
         */
        while (RecvMessage(msg, a, b))
        {
            if (msg == TAG("FOCS"))
            {
                receive_focus(a);
            }
            else if (msg == TAG("PUSH"))
            {
                pushed = 0;
            }
            else if (msg == TAG("FADE"))
            {
                meta_alpha = a;
                update_color(1.0, b, 0.0, -1);
            }
            else if (msg == TAG("TEX"))
            {
                SetTexture(g_layers[a][2], b, 0, -1);
            }
            else if (msg == TAG("ENBL"))
            {
                g_enabled[a] = b;
                update_color(1.0, 100.0, 0.0, -1);
                SetOptionI(g_layers[cursor][2], TAG("ALPH"), BLEND_ALPHA);
                AnimColor(g_layers[cursor][2], 1.0, 1.0, 1.0, 0.8 * meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
            }
        }
        if (active && !pushed)
        {
            cursor = clamp(cursor, 0, g_count - 1);
            new pressed;
            PadState(_, pressed, _, _, -1);
            if (pressed & PAD_LEFT)
            {
                if (cursor - 1 >= 0)
                {
                    update_color(1.0, 100.0, 0.0, cursor);
                    cursor--;
                    sndSePlay(SE_CURSOR);
                    SetOptionI(g_layers[cursor][2], TAG("ALPH"), BLEND_ALPHA);
                    if (g_enabled[cursor])
                    {
                        AnimColor(g_layers[cursor][2], 1.0, 1.0, 1.0, 0.8 * meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
                    }
                    else
                    {
                        AnimColor(g_layers[cursor][2], 1.0, 1.0, 1.0, 0.3 * meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
                    }
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
                    update_color(1.0, 100.0, 0.0, cursor);
                    cursor++;
                    sndSePlay(SE_CURSOR);
                    SetOptionI(g_layers[cursor][2], TAG("ALPH"), BLEND_ALPHA);
                    if (g_enabled[cursor])
                    {
                        AnimColor(g_layers[cursor][2], 1.0, 1.0, 1.0, 0.8 * meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
                    }
                    else
                    {
                        AnimColor(g_layers[cursor][2], 1.0, 1.0, 1.0, 0.3 * meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
                    }
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
            if (pressed & PAD_CIRCLE && g_enabled[cursor])
            {
                sndSePlay(SE_DECIDE);
                AnimColor(g_layers[cursor][2], 1.0, 1.0, 1.0, 0.0 * meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
                wait_ms(100);
                AnimColor(g_layers[cursor][2], 1.0, 1.0, 1.0, 0.5 * meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
                wait_ms(100);
                AnimColor(g_layers[cursor][2], 1.0, 1.0, 1.0, 0.0 * meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
                wait_ms(100);
                AnimColor(g_layers[cursor][2], 1.0, 1.0, 1.0, 0.5 * meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
                wait_ms(100);
                AnimColor(g_layers[cursor][2], 1.0, 1.0, 1.0, 0.0 * meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
                wait_ms(100);
                AnimColor(g_layers[cursor][2], 1.0, 1.0, 1.0, 0.5 * meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
                wait_ms(100);
                SendMessage(manager, TAG("PUSH"), cursor, TAG("RRGT"), 0, 0);
                pushed = 1;
            }
            else if (pressed & PAD_CROSS)
            {
                SendMessage(manager, TAG("PUSH"), cursor, TAG("RDWN"), 0, 0);
                pushed = 1;
            }
            else if (pressed & PAD_TRIANGLE)
            {
                SendMessage(manager, TAG("PUSH"), cursor, TAG("RUP"), 0, 0);
                pushed = 1;
            }
            else if (pressed & PAD_SQUARE)
            {
                SendMessage(manager, TAG("PUSH"), cursor, TAG("RLFT"), 0, 0);
                pushed = 1;
            }
            else if (pressed & PAD_SELECT)
            {
                SendMessage(manager, TAG("PUSH"), cursor, TAG("SLCT"), 0, 0);
                pushed = 1;
            }
            else if (pressed & PAD_START)
            {
                SendMessage(manager, TAG("PUSH"), cursor, TAG("STRT"), 0, 0);
                pushed = 1;
            }
            else if (pressed & PAD_L1)
            {
                SendMessage(manager, TAG("PUSH"), cursor, TAG("L1  "), 0, 0);
                pushed = 1;
            }
            else if (pressed & PAD_R1)
            {
                SendMessage(manager, TAG("PUSH"), cursor, TAG("R1  "), 0, 0);
                pushed = 1;
            }
        }
        next_frame:
        sleep;
    }
    return;
}

/* Fades the buttons to alpha (a quarter of it when disabled) and hides the highlights. */
update_color(Float:alpha, Float:duration, Float:delay, old_cursor)
{
    if (old_cursor >= 0)
    {
        new Float:button_alpha = g_enabled[old_cursor] ? alpha : 0.25 * alpha;
        AnimColor(g_layers[old_cursor][0], 1.0, 1.0, 1.0, button_alpha * meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
        AnimColor(g_layers[old_cursor][1], 1.0, 1.0, 1.0, button_alpha * meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
        AnimColor(g_layers[old_cursor][2], 1.0, 1.0, 1.0, 0.0 * meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
    }
    for (new i; i < g_count; i++)
    {
        new Float:button_alpha = g_enabled[i] ? alpha : 0.25 * alpha;
        AnimColor(g_layers[i][0], 1.0, 1.0, 1.0, button_alpha * meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
        AnimColor(g_layers[i][1], 1.0, 1.0, 1.0, button_alpha * meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
        AnimColor(g_layers[i][2], 1.0, 1.0, 1.0, 0.0 * meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
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
    update_color(1.0, 100.0, 0.0, -1);
    SetOptionI(g_layers[cursor][2], TAG("ALPH"), BLEND_ALPHA);
    if (g_enabled[cursor])
    {
        AnimColor(g_layers[cursor][2], 1.0, 1.0, 1.0, 0.8 * meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
    }
    else
    {
        AnimColor(g_layers[cursor][2], 1.0, 1.0, 1.0, 0.3 * meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
    }
    show_pad_help();
}

/* Leaves the focus and dims the colors. */
lose_focus()
{
    active = 0;
    update_color(1.0, 100.0, 0.0, -1);
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
    printf("toolbar_emb load tex %s\n", url);
    if (has_extension(path, "tm2"))
    {
        start_image_decode2(path);
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
