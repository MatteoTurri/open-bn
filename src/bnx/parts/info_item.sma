/*
 * /opt0/bn/script/parts/info_item.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A line of an information menu laid out along a curve: its label (argument "label_string") and, to its right,
 * the value its menu sends with "INFO", squeezed into "info_width" pixels; an editable line has a blue label.
 */

#include <core>
#include <bn>

#pragma dynamic 512

#include "tag.inc"

#include <float>

new g_shadow_colors[4] = {0x5F000000, 0x5F000000, 0x5F000000, 0x5F000000}; /* the "SCL4" of the shadowed labels */
/* Values of edit_mode, from the argument "edit": how the value of the line is edited. */
enum
{
    EDIT_NONE,
    EDIT_STRING,
    EDIT_HALF,
    EDIT_ROMAN,
    EDIT_NUMERIC,
    EDIT_DATE,
    EDIT_TIME
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

/* Same as send_label_focus of sysconf.inc, with an untagged move_time. */
send_label_focus(obj, target, z_offset, Float:sharp, Float:extra_w, Float:extra_h, move_time, smoothing, blinking)
{
    if (!obj)
    {
        return;
    }
    SendMessage(obj, TAG("FOC0"), target, TAG("LABL"), z_offset, sharp);
    SendMessage(obj, TAG("FOC1"), extra_w, extra_h, move_time, smoothing);
    SendMessage(obj, TAG("FOC2"), blinking, 0, 0, 0);
}

public alpha_blend = BLEND_ADD; /* the "ALPH" option of the texts */
public text_effect; /* 1: the value has a shadow */

public info_item()
{
    new obj = Self();
    new fitted;
    new edit_mode;
    {
        new value[16];
        if (GetArg("edit", value, 16))
        {
            if (!strcmp(value, "string"))
            {
                edit_mode = EDIT_STRING;
            }
            else if (!strcmp(value, "half"))
            {
                edit_mode = EDIT_HALF;
            }
            else if (!strcmp(value, "roman"))
            {
                edit_mode = EDIT_ROMAN;
            }
            else if (!strcmp(value, "numeric"))
            {
                edit_mode = EDIT_NUMERIC;
            }
            else if (!strcmp(value, "date"))
            {
                edit_mode = EDIT_DATE;
            }
            else if (!strcmp(value, "time"))
            {
                edit_mode = EDIT_TIME;
            }
        }
    }
    new index = init_item(obj);
    if (!edit_mode)
    {
        pad_enable = 0;
    }
    {
        new label[64];
        GetArg("label_string", label, 64);
        if (edit_mode > EDIT_NONE)
        {
            SetColor(obj, 0.215, 0.708, 0.999, 0.0);
        }
        if (EDIT_NONE == edit_mode)
        {
            SetColor(obj, 0.7843, 0.7843, 0.7843, 0.0);
        }
        sprintf(label, "%s", label); /* redundant as written */
        SetOptionI(obj, TAG("ALPH"), alpha_blend);
        SetOptionI(obj, TAG("FONT"), FontID(0));
        SetOptionI(obj, TAG("FONT"), FontID(0));
        SetOptionI(obj, TAG("HALN"), ALIGN_RIGHT);
        SetOptionI(obj, TAG("VALN"), ALIGN_CENTER);
        SetOptionS(obj, TAG("UTF8"), label);
        SetOptionI(obj, TAG("FSIZ"), 15);
    }
    new info;
    new info_width;
    if (0 == text_effect)
    {
        info = CreateLabel(obj);
    }
    else
    {
        info = CreateLabelShadow(obj);
    }
    SetPos(info, 20.0, 0.0, 0.0, 0);
    SetColor(info, 0.7843, 0.7843, 0.7843, 0.0);
    SetOptionI(info, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(info, TAG("ALPH"), alpha_blend);
    {
        new value[64];
        if (GetArg("info_width", value, 64))
        {
            info_width = atoi(value);
        }
        else
        {
            info_width = 280;
        }
    }
    if (text_effect == 1)
    {
        SetOptionP(info, TAG("SCL4"), g_shadow_colors, 4);
        SetOptionI(info, TAG("SX64"), 2 * 64);
        SetOptionI(info, TAG("SY64"), 2 * 64);
    }
    new text[128];
    while (FindMessageStr(TAG("INFO"), text, 128))
    {
        if (index >= 0)
        {
            new info_text[256];
            sprintf(info_text, "%s", text);
            SetOptionI(info, TAG("FONT"), FontID(0));
            SetOptionS(info, TAG("UTF8"), info_text);
            SetOptionI(info, TAG("FSIZ"), 15);
        }
    }
    for (; ; )
    {
        handle_messages(index);
        if (FindMessageStr(TAG("INFO"), text, 128))
        {
            SetOptionI(info, TAG("W64 "), 0);
            SetColor(info, 0.7843, 0.7843, 0.7843, 0.0);
            SetOptionS(info, TAG("UTF8"), text);
            fitted = 0;
        }
        if (!fitted)
        {
            new rect[5];
            GetOptionP(info, TAG("RECT"), rect, 5);
            if (rect[0])
            {
                new text_w = rect[3] >> 6;
                if (text_w > info_width)
                {
                    if (0.6 * float(text_w) >= float(info_width))
                    {
                        SetOptionI(info, TAG("W64 "), floatround(64.0 * float(info_width) / 0.6, 0));
                        SetLocalScale(info, 0.6, 1.0, 1.0);
                    }
                    else
                    {
                        SetLocalScale(info, float(info_width) / float(text_w), 1.0, 1.0);
                    }
                }
                else
                {
                    SetLocalScale(info, 1.0, 1.0, 1.0);
                }
                fitted = 1;
            }
        }
        if (g_focus_pending)
        {
            if (focus)
            {
                send_label_focus(focus, obj, 0, 0.5, 60.0, 60.0, g_duration, 1, 1);
            }
            if (!edit_mode)
            {
                SetSystemParam(SYS_PADHELP, GetSystemParam(SYS_PADHELP) & ~padhelp);
            }
        }
        if (g_fade_pending)
        {
            AnimColor(info, 0.7843, 0.7843, 0.7843, g_alpha, g_fade_time, 0.0, ANIM_FUNC_LINEAR);
        }
        sleep;
    }
    return;
}
