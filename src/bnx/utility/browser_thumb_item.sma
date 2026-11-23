/*
 * /opt0/bn/script/utility/browser_thumb_item.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * An item of a menu of the file browser laid out along a curve that shows the icon of a save: the save its menu
 * sends with "DCDE", loaded into a cache file, over a grid frame that rises towards the viewer while the item has
 * the focus. It declares its natives itself instead of including bn.inc.
 */

#pragma dynamic 512

native swapchars(c);
native FindMessage(msg, &a, &b, &c, &d=0);
native SetOptionI(obj, option, value);
native Parent(obj);
native SetPos(obj, Float:x, Float:y, Float:z, ref);
native Self();
native Hide(obj);
native HideChildren(obj);
native PeekMessage(&msg, &a, &b, &c, &d=0);
native AnimTimerManager(obj, Float:duration, Float:delay, curve);
native Float:AnimDoTimer(obj, anim, Float:speed);
native AnimMoveLine(obj, Float:x, Float:y, Float:z, Float:duration, Float:delay, curve);
native AnimMoveHermite(obj, Float:x, Float:y, Float:z, Float:duration, Float:delay, curve, Float:velocity_scale);
native GetSystemParam(id);
native SetSystemParam(id, value);
native AnimGetCurrTime(obj, anim);
native AnimGetState(obj, anim);
native Float:floatsub(Float:oper1, Float:oper2);
native Float:floatmul(Float:oper1, Float:oper2);
native Float:floatadd(Float:dividend, Float:divisor);
native GetColor(obj, &Float:r, &Float:g, &Float:b, &Float:a);
native AnimColor(obj, Float:r, Float:g, Float:b, Float:a, Float:duration, Float:delay, curve);
native GetPosHermite(obj, &Float:x, &Float:y, &Float:z, Float:t, count);
native SendMessage(obj, msg, {Float,_}:a, {Float,_}:b, Float:c, {Float,_}:d);
native floatround(Float:value, method=0);
native CreatePlane(parent);
native SetLocalScale(obj, Float:x, Float:y, Float:z);
native SetColor(obj, Float:r, Float:g, Float:b, Float:a);
native SetOptionF(obj, option, Float:value);
native imgGetResource(id);
native SetTexture(obj, texture, option, tex_mode);
native CreateIcon(parent);
native RecvMessageStr(&msg, const str[], maxlength);
native iconLoad(index, const url[], &handle, load_normal, load_copy, load_delete);
native iconSync(handle, &status, damaged, title1, title2, size1, size2);
native printf(const format[]);
native sprintf(const dest[], const format[], ...);
native SetOptionS(obj, option, const str[]);

#pragma rational Float
native Float:operator*(Float:oper1, Float:oper2) = floatmul;
native Float:operator+(Float:oper1, Float:oper2) = floatadd;
native Float:operator-(Float:oper1, Float:oper2) = floatsub;

/* As in bn.inc. */
const PAD_TRIANGLE = 0x1000;
const PAD_CIRCLE = 0x2000;
const PAD_CROSS = 0x4000;
const PAD_SQUARE = 0x8000;
const ANIM_FUNC_LINEAR = 1;
const ANIM_FUNC_EXPONENT = 6;
const ANIM_TIMER = 4;
const ANIM_RUN = 2;
const SYS_PADHELP = 500;
const PADHELP_CIRCLE = 1;
const PADHELP_CROSS = 2;
const PADHELP_TRIANGLE = 4;
const PADHELP_SQUARE = 8;
const BLEND_ADD = 2;
const RES_GRID = 2;

/* As in scroll.inc. */
const SCROLL_ALONG_CURVE = 0;
const SCROLL_LINE = 1;
const SCROLL_HERMITE = 2;
const UPDATE_FADE = 1;
const UPDATE_SCROLL = 2;
const UPDATE_FOCUS = 4;

/* As in browser.inc. */
const ICON_CARD_ERROR = 1;

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

/* Same as TAG of tag.inc. */
TAG(fourcc[])
{
    return swapchars(fourcc[0]);
}

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

/* Same as get_curve_pos of misc.inc. */
get_curve_pos(curve, Float:t, node_count, &Float:x, &Float:y, &Float:z)
{
    GetPosHermite(curve, x, y, z, t, node_count);
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

new g_last_focused_index = -1; /* g_focused_index of the previous frame */

public thumb_item()
{
    new tex_size = floatround(64.0, 0); /* unused, but part of the layout */
    new obj = Self();
    new loaded; /* the icon of the last save is in its cache file */
    new handle;
    new enable_pending;
    new index = init_item(obj);
    new frame = CreatePlane(obj);
    SetLocalScale(frame, 64.0 * 1.4, 64.0 * 1.4, 1.0);
    SetColor(frame, 1.0, 1.0, 1.0, 0.0);
    SetOptionF(frame, TAG("ZOFS"), -1.0);
    SetOptionI(frame, TAG("ALPH"), BLEND_ADD);
    SetTexture(frame, imgGetResource(RES_GRID), 0, -1);
    new icon = CreateIcon(frame);
    SetLocalScale(icon, 70.0, 70.0, 70.0);
    for (; ; )
    {
        new msg;
        new url[256];
        while (RecvMessageStr(msg, url, 256))
        {
            if (msg == TAG("DCDE") && index >= 0)
            {
                if (index >= 0) /* redundant as written */
                {
                    iconLoad(index + 1, url, handle, 1, 0, 0);
                }
                loaded = 0;
            }
        }
        if (index >= 0 && !loaded)
        {
            new ret;
            new status;
            ret = iconSync(handle, status, 1, 1);
            if (status == ICON_CARD_ERROR)
            {
                printf("...................... MEMORY CARD CHANGE??????????????\n");
            }
            if (ret == 1)
            {
                ret = 1; /* redundant as written */
            }
            if (ret == 1)
            {
                new path[64];
                sprintf(path, "/opt0/cache/ictmp%04d", index + 1);
                SetOptionS(icon, TAG("FILE"), path);
                loaded = 1;
            }
        }
        handle_messages(index);
        /*
         * On the focus the frame rises and the buttons of padhelp are taken away until the icon is in; when the
         * focus leaves, the frame goes back.
         */
        if (g_focus_pending)
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
        if (enable_pending && loaded)
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
            /* the frame takes the alpha of the item, the icon a size in proportion to it */
            SetColor(frame, 1.0, 1.0, 1.0, alpha);
            SetLocalScale(icon, 70.0 * alpha, 70.0 * alpha, 70.0 * alpha);
        }
        sleep;
    }
    return;
}
