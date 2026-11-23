/*
 * /opt0/bn/script/movie/thumb_item.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A thumbnail item of a menu laid out along a curve: the image of the url its menu sends with "DCDE", or the texture it
 * sends with TEX, fitted in a 64-pixel square over a grid frame, which rises towards the viewer while the item has
 * the focus.
 */

#include <core>
#include <bn>

#pragma dynamic 512

#include "tag.inc"

#include <float>

#include "imgsync.inc"

/* Values of state: the thumbnail image. */
enum
{
    THUMB_WAITING,      /* no image yet */
    THUMB_DECODING,
    THUMB_DECODING_TM2,
    THUMB_SHOWN,
    THUMB_NONE          /* the image failed */
};

#include "scroll.inc"

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

/* Sleeps until a message msg arrives. */
wait_message(msg, &a, &b, &c, &d)
{
    while (!FindMessage(msg, a, b, c, d))
    {
        sleep;
    }
}

#include "imgdec.inc"

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

#include "string.inc"

new g_last_focused_index = -1; /* g_focused_index of the previous frame */

public thumb_item()
{
    new tex_size = floatround(64.0, 0);
    new obj = Self();
    new state;
    new enable_pending;
    new index = init_item(obj);
    new frame = CreatePlane(obj);
    SetLocalScale(frame, 64.0 * 1.4, 64.0 * 1.4, 1.0);
    SetColor(frame, 1.0, 1.0, 1.0, 0.0);
    SetOptionF(frame, TAG("ZOFS"), -1.0);
    SetOptionI(frame, TAG("ALPH"), BLEND_ADD);
    SetTexture(frame, imgGetResource(RES_GRID), 0, -1);
    new thumb = CreatePlane(frame);
    SetLocalScale(thumb, 64.0, 64.0, 1.0);
    SetOptionI(thumb, TAG("FDEG"), 1);
    SetColor(thumb, 1.0, 1.0, 1.0, 0.0);
    for (; ; )
    {
        handle_messages(index);
        new msg;
        new url[256];
        new tex;
        new Float:tex_ratio;
        while (RecvMessageStr(msg, url, 256))
        {
            if (msg == TAG("DCDE") && index >= 0)
            {
                if (has_extension(url, "tm2"))
                {
                    start_image_decode2(url);
                    state = THUMB_DECODING_TM2;
                }
                else
                {
                    imgDecStart(url, tex_size, tex_size, 0, 100, 1);
                    state = THUMB_DECODING;
                }
            }
        }
        /* "TEX" (texture, width / height): a thumbnail decoded by the sender. */
        while (RecvMessage(msg, tex, tex_ratio))
        {
            if (msg == TAG("TEX") && index >= 0)
            {
                new Float:ratio = tex_ratio;
                SetTexture(thumb, tex, 0, -1);
                if (ratio > 1.0)
                {
                    SetLocalScale(thumb, 64.0, 64.0 / ratio, 64.0);
                }
                else
                {
                    SetLocalScale(thumb, 64.0 * ratio, 64.0, 64.0);
                }
                AnimColor(thumb, 1.0, 1.0, 1.0, g_alpha, g_fade_time, 0.0, g_anim_func);
                state = THUMB_SHOWN;
            }
        }
        if (index >= 0 && state <= THUMB_DECODING_TM2)
        {
            new status;
            if (state == THUMB_DECODING_TM2)
            {
                status = poll_image_decode();
            }
            else
            {
                status = imgDecSync();
            }
            if (status == IMG_DONE)
            {
                new Float:aspect = 1.0 * imgOrgHeight() / imgOrgWidth();
                new trim_w;
                new trim_h;
                new trim_x;
                new trim_y;
                if (aspect < 1.0)
                {
                    trim_w = floatround(64.0 * aspect, 0);
                    trim_h = floatround(64.0, 0);
                    trim_x = (tex_size - trim_w) / 2;
                    trim_y = 0;
                }
                else
                {
                    trim_w = floatround(64.0, 0);
                    trim_h = floatround(64.0 / aspect, 0);
                    trim_x = 0;
                    trim_y = (tex_size - trim_h) / 2;
                }
                imgTrimStart(trim_x, trim_y, trim_w, trim_h);
                if (wait_image_trim() == IMG_DONE)
                {
                    SetTexture(thumb, imgTexID(), 0, -1);
                }
                AnimColor(thumb, 1.0, 1.0, 1.0, g_alpha, g_fade_time, 0.0, g_anim_func);
                state = THUMB_SHOWN;
            }
            else if (status == IMG_FAILED)
            {
                state = THUMB_NONE;
            }
        }
        /*
         * On the focus the frame rises and the buttons of padhelp are taken away until the image is in; when the
         * focus leaves, the frame goes back.
         */
        if (g_focus_pending & g_update_mask)
        {
            if (focus)
            {
                send_focus(focus, frame, -5.0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            }
            AnimMoveLine(frame, 0.0, 0.0, 50.0, g_duration, 0.0, g_anim_func);
            if (padhelp & PADHELP_CIRCLE)
            {
                pad_enable = pad_enable & ~PAD_CIRCLE;
            }
            if (padhelp & PADHELP_SQUARE)
            {
                pad_enable = pad_enable & ~PAD_SQUARE;
            }
            if (padhelp & PADHELP_TRIANGLE)
            {
                pad_enable = pad_enable & ~PAD_TRIANGLE;
            }
            if (padhelp & PADHELP_CROSS)
            {
                pad_enable = pad_enable & ~PAD_CROSS;
            }
            SetSystemParam(SYS_PADHELP, GetSystemParam(SYS_PADHELP) & ~padhelp);
            g_focus_pending = 0;
            enable_pending = 1;
        }
        else if (g_last_focused_index != g_focused_index && index == g_last_focused_index)
        {
            enable_pending = 0;
            AnimMoveLine(frame, 0.0, 0.0, 0.0, g_duration, 0.0, g_anim_func);
        }
        g_last_focused_index = g_focused_index;
        if (enable_pending && state >= THUMB_SHOWN)
        {
            if (padhelp & PADHELP_CIRCLE)
            {
                pad_enable = pad_enable | PAD_CIRCLE;
            }
            if (padhelp & PADHELP_SQUARE)
            {
                pad_enable = pad_enable | PAD_SQUARE;
            }
            if (padhelp & PADHELP_TRIANGLE)
            {
                pad_enable = pad_enable | PAD_TRIANGLE;
            }
            if (padhelp & PADHELP_CROSS)
            {
                pad_enable = pad_enable | PAD_CROSS;
            }
            SetSystemParam(SYS_PADHELP, GetSystemParam(SYS_PADHELP) | padhelp);
            enable_pending = 0;
        }
        {
            new Float:red;
            new Float:green;
            new Float:blue;
            new Float:alpha;
            GetColor(obj, red, green, blue, alpha);
            if (alpha < 0.05)
            {
                HideChildren(Self());
            }
            else
            {
                ShowChildren(Self());
                SetColor(frame, 1.0, 1.0, 1.0, alpha);
                if (state == THUMB_SHOWN)
                {
                    AnimModColor(thumb, 1.0, 1.0, 1.0, alpha);
                }
            }
        }
        sleep;
    }
    return;
}
