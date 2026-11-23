/*
 * /opt0/bn/script/parts/album_item.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A cover item of a menu laid out along a curve: a texture its menu sends with TEX, or the image of the url it
 * sends with "DCDE", fitted in a 256-pixel square; the texture of TEX stays when the image cannot be shown.
 */

#include <core>
#include <bn>

#pragma dynamic 512

#include "tag.inc"

#include <float>

/* Values of state: the cover image. */
enum
{
    COVER_SHOWN,
    COVER_WAITING,      /* no url yet */
    COVER_DECODING,
    COVER_DECODING_TM2,
    COVER_DECODED,      /* shown once the old cover has faded out */
    COVER_FAILED        /* empty url, or the image failed: the texture of TEX */
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
public padhelp; /* the pad help buttons of pad_enable */
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

#include "string.inc"

new g_last_focused_index = -2; /* g_focused_index of the previous frame */

/* Sets the texture of the item, of width / height aspect, fitted in 256 pixels. */
set_cover(tex, Float:aspect)
{
    SetTexture(Self(), tex, 0, -1);
    if (0.0 == aspect)
    {
        aspect = 1.0;
    }
    if (aspect > 1.0)
    {
        SetLocalScale(Self(), 256.0, 256.0 / aspect, 256.0);
    }
    else
    {
        SetLocalScale(Self(), 256.0 * aspect, 256.0, 256.0);
    }
}

public album_item()
{
    new Float:red;
    new Float:green;
    new Float:blue;
    new Float:alpha;
    new obj = Self();
    new enable_pending;
    new state = COVER_WAITING;
    new Float:aspect = 1.33333;
    new given_tex;
    new tex;
    GetColor(obj, red, green, blue, alpha);
    SetColor(obj, red, green, blue, 0.0);
    new index = init_item(obj);
    for (; ; )
    {
        handle_messages(index);
        new msg;
        new url[256];
        new msg_tex;
        new Float:msg_aspect;
        new later;
        /* TEX (texture, aspect, later): a cover to keep, shown at once unless later */
        while (FindMessage(TAG("TEX"), msg_tex, msg_aspect, later))
        {
            given_tex = msg_tex;
            aspect = msg_aspect;
            if (index >= 0 && 0 == later)
            {
                tex = given_tex;
                set_cover(given_tex, aspect);
                AnimColor(obj, 1.0, 1.0, 1.0, g_alpha, g_fade_time, 0.0, ANIM_FUNC_LINEAR);
                state = COVER_SHOWN;
            }
        }
        while (RecvMessageStr(msg, url, 256))
        {
            if (msg == TAG("DCDE") && index >= 0)
            {
                if (has_extension(url, "tm2"))
                {
                    start_image_decode2(url);
                    state = COVER_DECODING_TM2;
                }
                else if (strlen(url) > 0)
                {
                    imgDecStart(url, floatround(256.0, 0), floatround(256.0, 0), 0, 100, 0);
                    state = COVER_DECODING;
                }
                else
                {
                    state = COVER_FAILED;
                    if (given_tex)
                    {
                        tex = given_tex;
                    }
                }
            }
        }
        if ((state == COVER_DECODING_TM2 || state == COVER_DECODING) && index >= 0)
        {
            new status;
            if (state == COVER_DECODING_TM2)
            {
                status = poll_image_decode();
            }
            else
            {
                status = imgDecSync();
            }
            if (status == IMG_DONE)
            {
                if (tex)
                {
                    /* the old cover fades out, and "FADE" waits for the new one */
                    AnimColor(obj, 1.0, 1.0, 1.0, 0.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
                    g_update_mask = g_update_mask & ~UPDATE_FADE;
                }
                aspect = 1.0 * imgTexOrgWidth() / imgTexOrgHeight();
                tex = imgTexID();
                state = COVER_DECODED;
            }
            else if (status == -1)
            {
                state = COVER_FAILED;
                if (given_tex)
                {
                    tex = given_tex;
                }
            }
        }
        if (tex && state >= COVER_DECODED && index >= 0 && ANIM_DONE == AnimGetState(obj, ANIM_COLOR))
        {
            set_cover(tex, aspect);
            AnimColor(obj, 1.0, 1.0, 1.0, g_alpha, g_fade_time, 0.0, ANIM_FUNC_LINEAR);
            state = COVER_SHOWN;
            g_update_mask = g_update_mask | UPDATE_FADE;
        }
        /* On the focus the buttons of padhelp are taken away and, the cover counting as ready, given back at once. */
        if (g_focus_pending & g_update_mask)
        {
            enable_pending = 1;
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
        }
        else if (g_last_focused_index != g_focused_index && index == g_last_focused_index)
        {
            enable_pending = 0;
        }
        g_last_focused_index = g_focused_index;
        if (enable_pending && state >= COVER_SHOWN) /* redundant as written */
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
        sleep;
    }
    return;
}
