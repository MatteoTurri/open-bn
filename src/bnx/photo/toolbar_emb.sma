/*
 * /opt0/bn/script/photo/toolbar_emb.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The button bar of the slide show (back, next, start, pause, stop): embossed image buttons of three layers
 * like parts/toolbar_emb, which also show a pressed look from the press until the manager answers "PUSH" and
 * the button is released.
 */

#include <core>
#include <bn>

#pragma dynamic 512

#include "photodb.inc"

/* The layers of a button (g_layers), images btn<i>0 to btn<i>2: the first two swap their blend to look pressed. */
enum
{
    LAYER_LIGHT,    /* added */
    LAYER_SHADOW,   /* subtracted */
    LAYER_HIGHLIGHT /* blue under the focus, dimmed otherwise */
};

#include "tag.inc"

#include <float>

#include "imgsync.inc"

#include "imgdec.inc"

const PUSH_PENDING = 0x10000; /* in pushed: the manager has not answered "PUSH" yet */

public cursor;  /* the button under the focus */
public manager; /* the object told of focus moves and presses */
public focus; /* unused, but part of the layout */
public padhelp; /* pad help buttons shown besides the circle while the bar has the focus */
public alpha_blend_mode = BLEND_ALPHA; /* unused, but part of the layout */
new g_padhelp_disable;
new g_active; /* 1 while the bar has the focus */
new Float:g_meta_alpha = 1.0; /* overall alpha, set by "FADE" */
new g_layers[10][3];
new g_enabled[10]; /* a disabled button is dimmed and ignores the circle */
new g_count;
new Float:g_default_interval = 64.0;
public expand; /* 1: keep the size of a button and fit the image in it; 0: the size of the image */

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
                new layer = LAYER_HIGHLIGHT;
                for (; layer >= 0; layer--)
                {
                    new key[32];
                    new url[32];
                    sprintf(key, "btn%d%d", i, layer);
                    if (GetArg(key, url, 32))
                    {
                        g_layers[i][layer] = CreatePlane(Self());
                        SetPos(g_layers[i][layer], 0.0, 0.0, 0.0, 0);
                        SetColor(g_layers[i][layer], 1.0, 1.0, 1.0, 0.0);
                        if (LAYER_LIGHT == layer)
                        {
                            SetOptionI(g_layers[i][layer], TAG("ALPH"), BLEND_ADD);
                        }
                        else if (layer == LAYER_SHADOW)
                        {
                            SetOptionI(g_layers[i][layer], TAG("ALPH"), BLEND_SUB);
                        }
                        else if (layer == LAYER_HIGHLIGHT)
                        {
                            SetOptionI(g_layers[i][layer], TAG("ALPH"), BLEND_ALPHA);
                            SetOptionF(g_layers[i][layer], TAG("ZOFS"), -2.0);
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
                pushed = pushed & ~PUSH_PENDING;
                if (!pushed)
                {
                    release_button();
                }
            }
            else if (msg == TAG("FADE"))
            {
                g_meta_alpha = a;
                update_color(1.0, b, 0.0, -1);
            }
            else if (msg == TAG("TEX"))
            {
                SetTexture(g_layers[a][LAYER_HIGHLIGHT], b, 0, -1);
            }
            else if (msg == TAG("ENBL"))
            {
                g_enabled[a] = b;
            }
        }
        new pressed;
        new released;
        PadState(_, pressed, released, _, -1);
        if (g_active && 0 == pushed)
        {
            cursor = clamp(cursor, 0, g_count - 1);
            if (pressed & PAD_LEFT)
            {
                if (cursor - 1 >= 0)
                {
                    cursor--;
                    update_color(1.0, 100.0, 0.0, cursor);
                    sndSePlay(SE_CURSOR);
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
                    update_color(1.0, 100.0, 0.0, cursor);
                    sndSePlay(SE_CURSOR);
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
                    sndSePlay(SE_DECIDE);
                    press_button(cursor);
                    SendMessage(manager, TAG("PUSH"), cursor, TAG("RRGT"), 0, 0);
                    pushed = PUSH_PENDING | PAD_CIRCLE;
                }
                else
                {
                    sndSePlay(SE_CANCEL);
                }
            }
            else if (pressed & PAD_CROSS)
            {
                SendMessage(manager, TAG("PUSH"), cursor, TAG("RDWN"), 0, 0);
                pushed = PUSH_PENDING;
            }
            else if (pressed & PAD_TRIANGLE)
            {
                SendMessage(manager, TAG("PUSH"), cursor, TAG("RUP"), 0, 0);
                pushed = PUSH_PENDING;
            }
            else if (pressed & PAD_SQUARE)
            {
                SendMessage(manager, TAG("PUSH"), cursor, TAG("RLFT"), 0, 0);
                pushed = PUSH_PENDING;
            }
            else if (pressed & PAD_SELECT)
            {
                SendMessage(manager, TAG("PUSH"), cursor, TAG("SLCT"), 0, 0);
                pushed = PUSH_PENDING;
            }
            else if (pressed & PAD_START)
            {
                /* start shows the pause button pressed while the slide show plays, the start button otherwise */
                new mode = GetPubVar(manager, "slide_mode");
                if (mode == SLIDE_PLAY)
                {
                    press_button(BUTTON_PAUSE);
                }
                else
                {
                    press_button(BUTTON_START);
                }
                SendMessage(manager, TAG("PUSH"), cursor, TAG("STRT"), 0, 0);
                pushed = PUSH_PENDING | PAD_START;
            }
            else if (pressed & PAD_L1)
            {
                press_button(BUTTON_BACK);
                SendMessage(manager, TAG("PUSH"), cursor, TAG("L1  "), 0, 0);
                pushed = PUSH_PENDING | PAD_L1;
            }
            else if (pressed & PAD_R1)
            {
                press_button(BUTTON_NEXT);
                SendMessage(manager, TAG("PUSH"), cursor, TAG("R1  "), 0, 0);
                pushed = PUSH_PENDING | PAD_R1;
            }
            goto next_frame;
        }
        if (g_active && pushed)
        {
            if (released & pushed)
            {
                pushed = pushed & PUSH_PENDING;
                if (!pushed)
                {
                    release_button();
                }
            }
        }
        next_frame:
        sleep;
    }
    return;
}

new g_pressed_button = -1; /* the button shown pressed, -1 before the first press */

/* Shows button pressed: its added and subtracted layers swap places. */
press_button(button)
{
    g_pressed_button = button;
    if (g_pressed_button != cursor)
    {
        new Float:alpha = g_enabled[g_pressed_button] ? 0.15 : 1.0;
        AnimColor(g_layers[g_pressed_button][LAYER_HIGHLIGHT], 0.7843, 0.7843, 0.7843, alpha * g_meta_alpha, 50.0, 0.0,
                ANIM_FUNC_LINEAR);
    }
    if (!g_enabled[g_pressed_button])
    {
        return;
    }
    SetOptionI(g_layers[g_pressed_button][LAYER_LIGHT], TAG("ALPH"), BLEND_SUB);
    SetOptionI(g_layers[g_pressed_button][LAYER_SHADOW], TAG("ALPH"), BLEND_ADD);
}

/* Gives the last pressed button its raised look again. */
release_button()
{
    if (g_pressed_button < 0)
    {
        return;
    }
    if (g_pressed_button != cursor)
    {
        AnimColor(g_layers[g_pressed_button][LAYER_HIGHLIGHT], 0.7843, 0.7843, 0.7843, 0.15 * g_meta_alpha, 250.0, 0.0,
                ANIM_FUNC_LINEAR);
    }
    SetOptionI(g_layers[g_pressed_button][LAYER_LIGHT], TAG("ALPH"), BLEND_ADD);
    SetOptionI(g_layers[g_pressed_button][LAYER_SHADOW], TAG("ALPH"), BLEND_SUB);
}

/* Fades the buttons to alpha and the highlights to their resting color, the one of highlighted to blue. */
update_color(Float:alpha, Float:duration, Float:delay, highlighted)
{
    for (new i; i < g_count; i++)
    {
        new Float:button_alpha = alpha;
        AnimColor(g_layers[i][LAYER_LIGHT], 1.0, 1.0, 1.0, button_alpha * g_meta_alpha, duration, delay,
                ANIM_FUNC_LINEAR);
        AnimColor(g_layers[i][LAYER_SHADOW], 1.0, 1.0, 1.0, button_alpha * g_meta_alpha, duration, delay,
                ANIM_FUNC_LINEAR);
        if (highlighted >= 0 && i == highlighted)
        {
            AnimColor(g_layers[highlighted][LAYER_HIGHLIGHT], 0.215, 0.708, 0.999, 0.8 * g_meta_alpha, duration, delay,
                    ANIM_FUNC_LINEAR);
        }
        else if (g_enabled[i])
        {
            AnimColor(g_layers[i][LAYER_HIGHLIGHT], 0.7843, 0.7843, 0.7843, 0.15 * g_meta_alpha, duration, delay,
                    ANIM_FUNC_LINEAR);
        }
        else
        {
            AnimColor(g_layers[i][LAYER_HIGHLIGHT], 0.7843, 0.7843, 0.7843, 0.0 * g_meta_alpha, duration, delay,
                    ANIM_FUNC_LINEAR);
        }
    }
}

/* Handles "FOCS": takes the focus, or gives it up if direction is "FOUT". */
receive_focus(direction)
{
    g_active = 1;
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
    AnimColor(g_layers[cursor][LAYER_HIGHLIGHT], 0.215, 0.708, 0.999, 0.8 * g_meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
    show_pad_help();
}

/* Leaves the focus and dims the colors. */
lose_focus()
{
    g_active = 0;
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

/* Shows the pad help of padhelp and the circle, less the buttons of g_padhelp_disable. */
show_pad_help()
{
    new buttons = padhelp;
    buttons = buttons | PADHELP_CIRCLE;
    buttons = buttons & ~g_padhelp_disable;
    SetSystemParam(SYS_PADHELP, buttons);
}
