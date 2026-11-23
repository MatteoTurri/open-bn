/*
 * /opt0/bn/script/game/album_item.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * An item of an album: the image named by "DCDE" fitted in 256x256 (a dot until it is decoded), that the album
 * moves along its path and fades through the messages "INIT", "TIME", "SCRL", "FOCS" and "FADE".
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "tag.inc"

#include <float>

#include "scroll.inc"

new g_path; /* the path of the album, the parent of the item */
new g_item;
new Float:g_pos; /* position on the path, 0.0 to 1.0 */
new Float:g_from_pos;
new Float:g_to_pos;
new g_node_count;
new g_index; /* index in the list, -1 for a blank item */
new g_update_mask = UPDATE_FADE | UPDATE_SCROLL | UPDATE_FOCUS;
new g_fade_pending;
new g_focus_pending;
new g_timer_running;
new g_scroll_kind = SCROLL_LINE;
new Float:g_move_time = 600.0;
new Float:g_move_delay;
new g_move_curve = ANIM_FUNC_EXPONENT;
new Float:g_alpha;
new Float:g_fade_time = 600.0;
public pad_enable; /* the pad buttons the item takes, read by the album */
public padhelp; /* the PADHELP_* bits of pad_enable */
public focus; /* unused, but part of the layout */
new g_focused = -2; /* the focused index of the album */

/* Sleeps until a message msg arrives. */
wait_message(msg, &a, &b, &c, &d)
{
    while (!FindMessage(msg, a, b, c, d))
    {
        sleep;
    }
}

/* Creates a null object under parent. */
create_child(parent)
{
    return CreateNull(parent);
}

/* Waits for "INIT" (index, curve position, node count, focus) from the menu and places the item; returns its index. */
init_item(item)
{
    g_item = item;
    SetOptionI(g_item, TAG("FLIP"), 0);
    new index;
    new pos;
    new node_count;
    new focus_obj;
    wait_message(TAG("INIT"), index, pos, node_count, focus_obj);
    g_path = Parent(0);
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
        get_curve_pos(g_path, g_pos, g_node_count, x, y, z);
        SetPos(g_item, x, y, z, 0);
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

/* Handles the messages of the album and runs the move along the path and the fade. */
update_item(index)
{
    g_fade_pending = 0;
    g_focus_pending = 0;
    new msg;
    new param1;
    new param2;
    new param3;
    new param4;
    if (PeekMessage(msg, param1, param2, param3))
    {
        while (FindMessage(TAG("TIME"), param1, param2, param3))
        {
            g_move_time = param1;
            g_move_delay = param2;
            g_move_curve = param3;
        }
        while (FindMessage(TAG("SCRL"), param1, param2, param3))
        {
            if (!(g_update_mask & UPDATE_SCROLL))
            {
                continue;
            }
            g_to_pos = param1;
            g_from_pos = g_pos;
            new Float:x;
            new Float:y;
            new Float:z;
            get_curve_pos(g_path, g_to_pos, g_node_count, x, y, z);
            switch (param3)
            {
                case SCROLL_ALONG_CURVE:
                {
                    AnimTimerManager(g_item, g_move_time, g_move_delay, g_move_curve);
                    g_timer_running = 1;
                    AnimDoTimer(g_item, ANIM_TIMER, 1.0);
                    g_scroll_kind = SCROLL_ALONG_CURVE;
                }
                case SCROLL_LINE:
                {
                    AnimMoveLine(g_item, x, y, z, g_move_time, g_move_delay, g_move_curve);
                    g_scroll_kind = SCROLL_LINE;
                }
                case SCROLL_HERMITE:
                {
                    AnimMoveHermite(g_item, x, y, z, g_move_time, g_move_delay, g_move_curve, 1.0);
                    g_scroll_kind = SCROLL_HERMITE;
                }
            }
        }
        while (FindMessage(TAG("ACTN"), param1, param2, param3))
        {
        }
        /* "FOCS" (focused index, alpha, duration, album active): the focused item shows its pad help. */
        while (FindMessage(TAG("FOCS"), param1, param2, param3, param4))
        {
            g_focused = param1;
            g_alpha = param2;
            if (g_index == g_focused && g_focused >= 0 && param4)
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
            g_move_time = param3;
            g_fade_time = param3;
            g_fade_pending = UPDATE_FADE;
        }
        while (FindMessage(TAG("FADE"), param1, param2, param3))
        {
            g_alpha = param1;
            g_fade_time = param2;
            g_fade_pending = UPDATE_FADE;
        }
    }
    if (SCROLL_ALONG_CURVE == g_scroll_kind && g_update_mask & UPDATE_SCROLL && g_timer_running)
    {
        new Float:progress = AnimGetCurrTime(g_item, ANIM_TIMER);
        if (ANIM_RUN != AnimGetState(g_item, ANIM_TIMER))
        {
            g_timer_running = 0;
            progress = 1.0;
        }
        g_pos = g_from_pos + (g_to_pos - g_from_pos) * progress;
        new Float:x;
        new Float:y;
        new Float:z;
        get_curve_pos(g_path, g_pos, g_node_count, x, y, z);
        SetPos(g_item, x, y, z, 0);
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
        GetColor(g_item, red, green, blue, alpha);
        AnimColor(g_item, red, green, blue, g_alpha, g_fade_time, 0.0, ANIM_FUNC_LINEAR);
    }
    if (g_focus_pending & g_update_mask)
    {
    }
    index = index * 0; /* redundant as written */
}

#include "misc.inc"

public album_item()
{
    new self = Self();
    new index = init_item(self);
    new dot;
    if (index >= 0)
    {
        dot = create_child(self);
        SetLocalScale(dot, 5.0, 5.0, 5.0);
        SetTexture(dot, imgGetResource(RES_DOT), 0, -1);
    }
    for (; ; )
    {
        new msg;
        new url[256];
        /* "DCDE" url decodes the image of the item. */
        while (RecvMessageStr(msg, url, 256))
        {
            if (msg == TAG("DCDE") && index >= 0)
            {
                imgDecStart(url, floatround(256.0, 0), floatround(256.0, 0), 0, 100, 0);
            }
        }
        if (index >= 0)
        {
            new status = imgDecSync();
            if (status == IMG_DONE)
            {
                SetTexture(self, imgTexID(), 0, -1);
                new Float:aspect = 1.0 * imgTexOrgWidth() / imgTexOrgHeight();
                if (aspect > 1.0)
                {
                    SetLocalScale(self, 256.0, 256.0 / aspect, 256.0);
                }
                else
                {
                    SetLocalScale(self, 256.0 * aspect, 256.0, 256.0);
                }
                Destroy(dot);
            }
        }
        update_item(index);
        sleep;
    }
    return;
}
