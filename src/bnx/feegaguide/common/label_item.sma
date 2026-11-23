/*
 * /opt0/bn/script/feegaguide/common/label_item.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * An item of a guide list laid out on a curve (its parent): the list scrolls it along the curve, dims it unless it
 * has the focus and fades it; when it gets the focus it adds its buttons to the pad help and takes the focus object.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

#include "tag.inc"

#include "scroll.inc"

new g_curve;
new g_obj;
new Float:g_pos; /* place on the curve, 0.0 to 1.0 */
new Float:g_from;
new Float:g_to;
new g_node_count;
new g_index; /* in the list, negative for a hidden item */
new g_color_dirty;
new g_got_focus;
new g_move_mode = SCROLL_LINE;
new Float:g_time = 600.0;
new Float:g_delay;
new g_anim_func = ANIM_FUNC_EXPONENT;
new g_look;
new Float:g_look_target;
new Float:g_fade_time = 500.0;
new Float:g_fade_alpha = 1.0;
new Float:g_alpha;
new Float:g_dim_alpha = 0.75;
new Float:g_focus_alpha = 1.0;
new Float:g_color_time = 600.0;
public pad_enable; /* PAD_* bits of the buttons the item reacts to */
public padhelp;    /* the buttons of pad_enable as PADHELP_* bits */
public focus;      /* the focus object */
new g_focus_index = -2;

/* Sleeps until a message msg arrives. */
wait_message(msg, &a, &b, &c, &d)
{
    while (!FindMessage(msg, a, b, c, d))
    {
        sleep;
    }
}

/* Waits for the "INIT" message (index, place on the curve, node count, focus object) and puts the item there. */
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
    g_to = pos;
    g_pos = g_to;
    g_from = g_to;
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
    return g_index;
}

/*
 * Handles the messages of the list and runs the item for one frame. "MODE" (move mode); "TIME" (duration, delay,
 * curve) of the moves; "SCRL" (place, _, move mode); "LOOK" (on, target); "FOCS" (focused index, alpha);
 * "FADE" (alpha, duration).
 */
update_item(index)
{
    new msg;
    new a;
    new Float:b;
    new c;
    if (PeekMessage(msg, a, b, c))
    {
        while (FindMessage(TAG("MODE"), a, b, c))
        {
            g_move_mode = a;
        }
        while (FindMessage(TAG("TIME"), a, b, c))
        {
            g_time = a;
            g_delay = b;
            g_anim_func = c;
        }
        while (FindMessage(TAG("SCRL"), a, b, c))
        {
            g_to = a;
            g_from = g_pos;
            new Float:x;
            new Float:y;
            new Float:z;
            get_curve_pos(g_curve, g_to, g_node_count, x, y, z);
            switch (c)
            {
                case SCROLL_ALONG_CURVE:
                {
                    AnimTimerManager(g_obj, g_time, g_delay, g_anim_func);
                    AnimDoTimer(g_obj, ANIM_TIMER, 1.0);
                    g_move_mode = SCROLL_ALONG_CURVE;
                }
                case SCROLL_LINE:
                {
                    AnimMoveLine(g_obj, x, y, z, g_time, g_delay, g_anim_func);
                    g_move_mode = SCROLL_LINE;
                }
                case SCROLL_HERMITE:
                {
                    AnimMoveHermite(g_obj, x, y, z, g_time, g_delay, g_anim_func, 1.0);
                    g_move_mode = SCROLL_HERMITE;
                }
            }
        }
        while (FindMessage(TAG("LOOK"), a, b, c))
        {
            g_look = a;
            g_look_target = b;
        }
        while (FindMessage(TAG("ACTN"), a, b, c))
        {
        }
        while (FindMessage(TAG("FOCS"), a, b, c))
        {
            g_focus_index = a;
            g_alpha = b * g_dim_alpha;
            if (g_index == g_focus_index && g_focus_index >= 0)
            {
                g_alpha = g_focus_alpha;
                g_got_focus = 1;
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
                g_got_focus = 0;
            }
            g_color_time = g_time;
            g_color_dirty = 1;
        }
        while (FindMessage(TAG("FADE"), a, b, c))
        {
            g_fade_alpha = a;
            g_fade_time = b;
            g_color_time = g_fade_time;
            g_color_dirty = 1;
        }
    }
    {
        new Float:t = AnimGetCurrTime(g_obj, ANIM_TIMER);
        g_pos = g_from + (g_to - g_from) * t;
        if (SCROLL_ALONG_CURVE == g_move_mode)
        {
            new Float:x;
            new Float:y;
            new Float:z;
            get_curve_pos(g_curve, g_pos, g_node_count, x, y, z);
            SetPos(g_obj, x, y, z, 0);
        }
        if (g_look)
        {
            LookAt(g_obj, g_look_target, 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
        }
        if (g_color_dirty)
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
            AnimColor(g_obj, red, green, blue, g_alpha * g_fade_alpha, g_color_time, 0.0, ANIM_FUNC_LINEAR);
            g_color_dirty = 0;
        }
        if (g_got_focus) /* redundant as written */
        {
        }
    }
    index = index * 0; /* redundant as written */
}

#include "misc.inc"

public label_item()
{
    new obj = Self();
    new index;
    index = init_item(obj);
    for (; ; )
    {
        update_item(index);
        if (g_got_focus)
        {
            if (focus)
            {
                set_label_focus(focus, obj, 0, 0.5, 60.0, 60.0, 100.0, 1, 1);
            }
            g_got_focus = 0;
        }
        sleep;
    }
    return;
}
