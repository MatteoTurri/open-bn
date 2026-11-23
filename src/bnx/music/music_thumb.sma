/*
 * /opt0/bn/script/music/music_thumb.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A track item of the music player laid out along a curve: its number over the texture of the argument texture
 * while it is the track being played (the grid otherwise), blinking while the track is paused; it rises towards the
 * viewer while it has the focus.
 */

#include <core>
#include <bn>

#pragma dynamic 512

#include "tag.inc"

#include <float>

#include "scroll.inc"

/* Sleeps until a message msg arrives. */
wait_message(msg, &a, &b, &c, &d)
{
    while (!FindMessage(msg, a, b, c, d))
    {
        sleep;
    }
}

/* Same as send_focus, with a Float z_offset. */
send_focus(obj, target, Float:z_offset, Float:sharp, Float:extra_w, Float:extra_h, Float:move_time, smoothing, blinking)
{
    if (!obj)
    {
        return;
    }
    SendMessage(obj, TAG("FOC0"), target, TAG(""), z_offset, sharp);
    SendMessage(obj, TAG("FOC1"), extra_w, extra_h, move_time, smoothing);
    SendMessage(obj, TAG("FOC2"), blinking, 0, 0, 0);
}

new g_curve;
new g_obj;
new Float:g_pos;
new Float:g_from_pos;
new Float:g_to_pos;
new g_node_count;
new g_index;
new g_update_mask = UPDATE_FADE | UPDATE_SCROLL | UPDATE_FOCUS; /* the messages the item follows */
new g_fade_pending;
new g_focus_pending;
new g_timer_running;
new g_scroll_mode = SCROLL_LINE;
new Float:g_duration = 600.0;
new Float:g_delay;
new g_anim_func = ANIM_FUNC_EXPONENT;
new Float:g_alpha;
new Float:g_fade_time = 600.0;
public pad_enable; /* the buttons (PAD_*) the item answers, shown in the pad help */
public padhelp; /* the pad help buttons of pad_enable (PADHELP_* bits) */
public focus; /* the focus plane */
new g_focused_index = -2;

/* Waits for "INIT" (index, curve position, node count, focus) from the menu and places the item; returns its index. */
init_item(obj)
{
    g_obj = obj;
    SetOptionI(g_obj, TAG("FLIP"), 0);
    new index;
    new pos;
    new node_count;
    new focus_obj;
    wait_message(TAG("INIT"), index, pos, node_count, focus_obj);
    g_curve = Parent(0);
    g_index = index;
    g_to_pos = pos;
    g_pos = g_to_pos;
    g_from_pos = g_to_pos;
    g_node_count = node_count;
    focus = focus_obj;
    {
        new Float:x;
        new Float:y;
        new Float:z;
        get_curve_pos(g_curve, g_pos, g_node_count, x, y, z);
        SetPos(g_obj, x, y, z, 0);
    }
    if (pad_enable & PAD_CIRCLE)
    {
        padhelp = PADHELP_CIRCLE;
    }
    if (pad_enable & PAD_SQUARE)
    {
        padhelp = padhelp | PADHELP_SQUARE;
    }
    if (pad_enable & PAD_TRIANGLE)
    {
        padhelp = padhelp | PADHELP_TRIANGLE;
    }
    if (pad_enable & PAD_CROSS)
    {
        padhelp = padhelp | PADHELP_CROSS;
    }
    if (g_index < 0)
    {
        Hide(Self());
        HideChildren(Self());
        g_update_mask = 0;
    }
    return g_index;
}

/*
 * Handles the messages of the menu: "TIME" (duration, delay, curve) of the next move, "SCRL" (position, _, mode),
 * "FOCS" (focused index, alpha, time, active) and "FADE" (alpha, time); then runs the move along the curve.
 */
handle_messages(index)
{
    g_fade_pending = 0;
    g_focus_pending = 0;
    new msg;
    new a;
    new b;
    new c;
    new d;
    if (PeekMessage(msg, a, b, c))
    {
        while (FindMessage(TAG("TIME"), a, b, c))
        {
            g_duration = a;
            g_delay = b;
            g_anim_func = c;
        }
        while (FindMessage(TAG("SCRL"), a, b, c))
        {
            if (!(g_update_mask & UPDATE_SCROLL))
            {
                continue;
            }
            g_to_pos = a;
            g_from_pos = g_pos;
            new Float:x;
            new Float:y;
            new Float:z;
            get_curve_pos(g_curve, g_to_pos, g_node_count, x, y, z);
            switch (c)
            {
                case SCROLL_ALONG_CURVE:
                {
                    AnimTimerManager(g_obj, g_duration, g_delay, g_anim_func);
                    g_timer_running = 1;
                    AnimDoTimer(g_obj, ANIM_TIMER, 1.0);
                    g_scroll_mode = SCROLL_ALONG_CURVE;
                }
                case SCROLL_LINE:
                {
                    AnimMoveLine(g_obj, x, y, z, g_duration, g_delay, g_anim_func);
                    g_scroll_mode = SCROLL_LINE;
                }
                case SCROLL_HERMITE:
                {
                    AnimMoveHermite(g_obj, x, y, z, g_duration, g_delay, g_anim_func, 1.0);
                    g_scroll_mode = SCROLL_HERMITE;
                }
            }
        }
        while (FindMessage(TAG("ACTN"), a, b, c))
        {
        }
        while (FindMessage(TAG("FOCS"), a, b, c, d))
        {
            g_focused_index = a;
            g_alpha = b;
            if (g_index == g_focused_index && g_focused_index >= 0 && d)
            {
                g_focus_pending = UPDATE_FOCUS;
                if (pad_enable & PAD_CIRCLE)
                {
                    padhelp = PADHELP_CIRCLE;
                }
                if (pad_enable & PAD_SQUARE)
                {
                    padhelp = padhelp | PADHELP_SQUARE;
                }
                if (pad_enable & PAD_TRIANGLE)
                {
                    padhelp = padhelp | PADHELP_TRIANGLE;
                }
                if (pad_enable & PAD_CROSS)
                {
                    padhelp = padhelp | PADHELP_CROSS;
                }
                SetSystemParam(SYS_PADHELP, GetSystemParam(SYS_PADHELP) | padhelp);
            }
            else
            {
                g_focus_pending = 0;
            }
            g_duration = c;
            g_fade_time = c;
            g_fade_pending = UPDATE_FADE;
        }
        while (FindMessage(TAG("FADE"), a, b, c))
        {
            g_alpha = a;
            g_fade_time = b;
            g_fade_pending = UPDATE_FADE;
        }
    }
    if (SCROLL_ALONG_CURVE == g_scroll_mode && g_update_mask & UPDATE_SCROLL && g_timer_running)
    {
        new Float:t = AnimGetCurrTime(g_obj, ANIM_TIMER);
        if (ANIM_RUN != AnimGetState(g_obj, ANIM_TIMER))
        {
            g_timer_running = 0;
            t = 1.0;
        }
        g_pos = g_from_pos + (g_to_pos - g_from_pos) * t;
        new Float:x;
        new Float:y;
        new Float:z;
        get_curve_pos(g_curve, g_pos, g_node_count, x, y, z);
        SetPos(g_obj, x, y, z, 0);
    }
    if (g_fade_pending & g_update_mask)
    {
        if (g_index < 0)
        {
            g_alpha = 0.0;
        }
        new Float:red;
        new Float:green;
        new Float:blue;
        new Float:alpha;
        GetColor(g_obj, red, green, blue, alpha);
        AnimColor(g_obj, red, green, blue, g_alpha, g_fade_time, 0.0, ANIM_FUNC_LINEAR);
    }
    if (g_focus_pending & g_update_mask)
    {
    }
    index = index * 0; /* redundant as written */
}

#include "misc.inc"

/* Arguments: texture, active (the index of the track being played), paused. */
public main()
{
    new texture;
    new active = -1;
    new focused;
    new is_active;
    new visible = 1;
    new Float:shown_alpha = -1.0;
    new paused;
    new blink_frame;
    new arg[64];
    if (GetArg("texture", arg, 64))
    {
        texture = atoi(arg);
    }
    if (GetArg("active", arg, 64))
    {
        active = atoi(arg);
    }
    if (GetArg("paused", arg, 64))
    {
        paused = atoi(arg);
    }
    new index = init_item(Self());
    is_active = index == active && index >= 0;
    if (!is_active)
    {
        paused = 0;
    }
    new plane = CreatePlane(Self());
    SetLocalScale(plane, 64.0 * 1.4, 64.0 * 1.4, 1.0);
    SetColor(plane, 1.0, 1.0, 1.0, 1.0);
    SetOptionF(plane, TAG("ZOFS"), -1.0);
    SetOptionI(plane, TAG("ALPH"), BLEND_ADD);
    if (is_active)
    {
        SetTexture(plane, texture, 0, -1);
    }
    else
    {
        SetTexture(plane, imgGetResource(RES_GRID), 0, -1);
    }
    new number_label = CreateLabel(plane);
    SetPos(number_label, 64.0 * 0.3, -64.0 * 0.3, 1.0, 0);
    SetColor(number_label, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(number_label, TAG("FSIZ"), 13);
    SetOptionI(number_label, TAG("VALN"), 2); /* no vertical alignment: the text sits on the baseline */
    SetOptionI(number_label, TAG("HALN"), ALIGN_RIGHT);
    SetOptionI(number_label, TAG("ALPH"), BLEND_ADD);
    new number[16];
    sprintf(number, "%d", index + 1);
    SetOptionS(number_label, TAG("UTF8"), number);
    for (; ; )
    {
        handle_messages(index);
        new msg;
        new a;
        new b;
        new c;
        /* "ACTV" (index of the track being played), "PAUS" (paused). */
        while (RecvMessage(msg, a, b, c))
        {
            if (msg == TAG("ACTV"))
            {
                new now_active = index == a && index >= 0;
                if (is_active != now_active)
                {
                    is_active = now_active;
                    if (is_active)
                    {
                        SetTexture(plane, texture, 0, -1);
                    }
                    else
                    {
                        SetTexture(plane, imgGetResource(RES_GRID), 0, -1);
                    }
                    paused = 0;
                }
            }
            if (msg == TAG("PAUS"))
            {
                if (is_active)
                {
                    paused = a;
                    if (!paused)
                    {
                        SetTexture(plane, texture, 0, -1);
                    }
                    blink_frame = 0;
                }
            }
        }
        /* While paused the texture blinks, 32 frames on and 32 off. */
        if (paused)
        {
            if (0 == blink_frame & 0x1F)
            {
                if (blink_frame & 0x20)
                {
                    SetTexture(plane, texture, 0, -1);
                }
                else
                {
                    SetTexture(plane, imgGetResource(RES_GRID), 0, -1);
                }
            }
            blink_frame++;
        }
        new now_focused = index == g_focused_index;
        now_focused = index == g_focused_index;
        if (focused != now_focused)
        {
            focused = now_focused;
            if (focused)
            {
                if (focus)
                {
                    send_focus(focus, plane, -5.0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                }
                AnimMoveLine(plane, 0.0, 0.0, 35.0, g_duration, 0.0, g_anim_func);
            }
            else
            {
                AnimMoveLine(plane, 0.0, 0.0, 0.0, g_duration, 0.0, g_anim_func);
            }
        }
        new Float:red;
        new Float:green;
        new Float:blue;
        new Float:alpha;
        GetColor(Self(), red, green, blue, alpha);
        if (shown_alpha != alpha)
        {
            shown_alpha = alpha;
            new now_visible = shown_alpha > 0.01;
            if (visible != now_visible)
            {
                visible = now_visible;
                if (visible)
                {
                    ShowChildren(Self());
                }
                else
                {
                    HideChildren(Self());
                }
            }
            SetColor(plane, 1.0, 1.0, 1.0, shown_alpha);
            SetColor(number_label, 1.0, 1.0, 1.0, shown_alpha);
        }
        sleep;
    }
    return;
}
