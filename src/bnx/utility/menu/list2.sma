/*
 * /opt0/bn/script/utility/menu/list2.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A list of up to ten items of the system settings, each a title (title<n>) with its current setting (setting<n>)
 * beside it: it moves the cursor, scrolls, and tells its manager when a button is pressed or the cursor leaves it.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "tag.inc"

#include <float>

#include "text.inc"

#include "sysconf.inc"

new g_shadow_colors[4] = {0x5F000000, 0x5F000000, 0x5F000000, 0x5F000000}; /* the "SCL4" of the shadowed labels */
public manager; /* gets "FOCS" when the cursor leaves the list and "PUSH" (item, button) */
public focus; /* the focus plane */
public padhelp; /* the pad help buttons shown on every item */
public active; /* the list has the cursor */
public cursor; /* the item under the cursor */
public Float:meta_alpha; /* alpha of the whole list, set by "FADE" */
new g_titles[10];
new g_settings[10];
new g_states[10]; /* 0: enabled, 1: disabled (dim), 2: shown only (gray); only 0 takes the circle */
new Float:g_up_enabled[10]; /* the triangle is shown in the pad help */
new g_count;
new g_visible; /* the number of rows shown at a time */
public scroll_pos; /* the first visible item */
new Float:g_interval = 60.0;
new Float:g_separation = 100.0; /* between the titles, aligned right, and the settings, aligned left */
new g_hiding; /* faded out: the list hides itself g_hide_delay ms after g_hide_start */
new g_hide_start;
new g_hide_delay;

main()
{
    new font_size = 17;
    {
        new value[5];
        if (GetArg("fontsize", value, 5))
        {
            font_size = atoi(value);
        }
    }
    {
        new value[5];
        if (GetArg("separate", value, 5))
        {
            g_separation = atof(value);
        }
    }
    {
        new value[5];
        if (GetArg("interval", value, 5))
        {
            g_interval = atof(value);
        }
    }
    g_visible = 10;
    {
        new value[5];
        if (GetArg("visible", value, 5))
        {
            g_visible = atoi(value);
        }
    }
    for (new i; i < 10; i++)
    {
        new name[64];
        new title[64];
        sprintf(name, "title%d", i);
        if (GetArg(name, title, 64))
        {
            g_titles[i] = CreateLabelShadow(Self());
            SetOptionP(g_titles[i], TAG("SCL4"), g_shadow_colors, 4);
            SetOptionI(g_titles[i], TAG("SX64"), 2 * 64);
            SetOptionI(g_titles[i], TAG("SY64"), 2 * 64);
            SetPos(g_titles[i], 0.5 * -g_separation, g_interval * i, 0.0, 0);
            SetColor(g_titles[i], 0.215, 0.708, 0.999, 0.0);
            SetOptionI(g_titles[i], TAG("FONT"), FontID(0));
            SetOptionI(g_titles[i], TAG("ALPH"), BLEND_ADD);
            SetOptionS(g_titles[i], TAG("UTF8"), title);
            SetOptionI(g_titles[i], TAG("HALN"), ALIGN_RIGHT);
            SetOptionI(g_titles[i], TAG("VALN"), ALIGN_CENTER);
            SetOptionI(g_titles[i], TAG("FSIZ"), font_size);
            new setting_name[64];
            new setting[64];
            sprintf(setting_name, "setting%d", i);
            GetArg(setting_name, setting, 64);
            g_settings[i] = CreateLabelShadow(Self());
            SetOptionP(g_settings[i], TAG("SCL4"), g_shadow_colors, 4);
            SetOptionI(g_settings[i], TAG("SX64"), 2 * 64);
            SetOptionI(g_settings[i], TAG("SY64"), 2 * 64);
            SetPos(g_settings[i], 0.5 * g_separation, g_interval * i, 0.0, 0);
            SetColor(g_settings[i], 0.7843, 0.7843, 0.7843, 0.0);
            SetOptionI(g_settings[i], TAG("FONT"), FontID(0));
            SetOptionI(g_settings[i], TAG("ALPH"), BLEND_ADD);
            SetOptionS(g_settings[i], TAG("UTF8"), setting);
            SetOptionI(g_settings[i], TAG("HALN"), ALIGN_LEFT);
            SetOptionI(g_settings[i], TAG("VALN"), ALIGN_CENTER);
            SetOptionI(g_settings[i], TAG("FSIZ"), font_size);
            for (; ; )
            {
                /* a valid flag, then the x, y, width and height of the text in 1/64 pixel */
                new rect[5];
                GetOptionP(g_settings[i], TAG("RECT"), rect, 5);
                if (rect[0])
                {
                    new Float:max_w = 300.0;
                    if (rect[3] / 64.0 > max_w)
                    {
                        SetScale(g_settings[i], max_w / (rect[3] / 64.0), 1.0, 1.0);
                    }
                    break;
                }
                sleep;
            }
            g_count++;
            sleep;
        }
        else
        {
            break;
        }
    }
    if (g_count < g_visible)
    {
        g_visible = g_count;
    }
    update_colors(100.0, 0.0);
    new pushed;
    for (; ; )
    {
        sleep;
        new msg;
        new a;
        new Float:b;
        /*
         * "FOCS" (from): takes the cursor; "FADE" (alpha, time); "ENBL" (item, state); "ERUP" (item, triangle shown);
         * "PUSH": the manager is done with the last push.
         */
        if (RecvMessage(msg, a, b))
        {
            if (msg == TAG("FOCS"))
            {
                enter_focus(a);
            }
            else if (msg == TAG("FADE"))
            {
                meta_alpha = a;
                update_colors(b, 0.0);
                if (0.0 == meta_alpha)
                {
                    g_hiding = 1;
                    g_hide_delay = floatround(b, 0);
                    g_hide_start = tickcount();
                }
                else
                {
                    g_hiding = 0;
                    Show(Self());
                }
            }
            else if (msg == TAG("ENBL"))
            {
                g_states[a] = b;
                update_colors(100.0, 0.0);
            }
            else if (msg == TAG("ERUP"))
            {
                g_up_enabled[a] = b;
                update_colors(100.0, 0.0);
            }
            else if (msg == TAG("PUSH"))
            {
                pushed = 0;
            }
        }
        if (active && !pushed && !is_animating(focus))
        {
            cursor = clamp(cursor, 0, g_count - 1);
            new repeated;
            new pressed;
            PadState(_, pressed, _, repeated, -1);
            if (pressed & PAD_CIRCLE)
            {
                if (!g_states[cursor])
                {
                    SendMessage(manager, TAG("PUSH"), cursor, TAG("RRGT"), 0, 0);
                    pushed = 1;
                    continue;
                }
            }
            else if (pressed & PAD_CROSS)
            {
                SendMessage(manager, TAG("PUSH"), cursor, TAG("RDWN"), 0, 0);
                pushed = 1;
                continue;
            }
            else if (pressed & PAD_TRIANGLE)
            {
                SendMessage(manager, TAG("PUSH"), cursor, TAG("RUP"), 0, 0);
                pushed = 1;
                continue;
            }
            else if (pressed & PAD_SQUARE)
            {
                SendMessage(manager, TAG("PUSH"), cursor, TAG("RLFT"), 0, 0);
                pushed = 1;
                continue;
            }
            else if (pressed & PAD_SELECT)
            {
                SendMessage(manager, TAG("PUSH"), cursor, TAG("SLCT"), 0, 0);
                pushed = 1;
                continue;
            }
            else if (pressed & PAD_START)
            {
                SendMessage(manager, TAG("PUSH"), cursor, TAG("STRT"), 0, 0);
                pushed = 1;
                continue;
            }
            if (repeated & PAD_UP)
            {
                if (cursor > 0)
                {
                    cursor--;
                    sndSePlay(SE_CURSOR);
                    update_pad_help(!g_states[cursor], g_up_enabled[cursor]);
                    update_colors(100.0, 0.0);
                    scroll_to_cursor();
                    send_label_focus(focus, g_titles[cursor], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                }
                else
                {
                    SendMessage(manager, TAG("FOCS"), TAG("UP"), 0, 0, 0);
                    leave_focus();
                    continue;
                }
            }
            else if (repeated & PAD_DOWN)
            {
                if (cursor < g_count - 1)
                {
                    cursor++;
                    sndSePlay(SE_CURSOR);
                    update_pad_help(!g_states[cursor], g_up_enabled[cursor]);
                    update_colors(100.0, 0.0);
                    scroll_to_cursor();
                    send_label_focus(focus, g_titles[cursor], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                }
                else
                {
                    SendMessage(manager, TAG("FOCS"), TAG("DOWN"), 0, 0, 0);
                    leave_focus();
                    continue;
                }
            }
            else if (repeated & PAD_LEFT)
            {
                SendMessage(manager, TAG("FOCS"), TAG("LEFT"), 0, 0, 0);
                leave_focus();
                continue;
            }
            else if (repeated & PAD_RIGHT)
            {
                SendMessage(manager, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
                leave_focus();
                continue;
            }
        }
        if (g_hiding)
        {
            new elapsed = tickcount() - g_hide_start;
            if (elapsed >= g_hide_delay)
            {
                Hide(Self());
                printf("list hiding...\n");
                g_hiding = 0;
            }
        }
    }
    return;
}

/* Fades every label to its color (the title after its state), transparent when scrolled out of view. */
update_colors(Float:duration, Float:delay)
{
    for (new i; i < g_count; i++)
    {
        {
            new Float:alpha = 1.0;
            new Float:r = 0.215;
            new Float:g = 0.708;
            new Float:b = 0.999;
            if (active && cursor == i)
            {
                alpha = 1.0;
            }
            if (g_states[i] == 1)
            {
                alpha = 1.0;
                r = 0.03;
                g = 0.18;
                b = 0.36;
            }
            else if (g_states[i] == 2)
            {
                alpha = 1.0;
                r = 0.7843;
                g = 0.7843;
                b = 0.7843;
            }
            if (i < scroll_pos)
            {
                alpha = 0.0;
            }
            if (i >= scroll_pos + g_visible)
            {
                alpha = 0.0;
            }
            AnimColor(g_titles[i], r, g, b, alpha * meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
        }
        new Float:setting_alpha = 1.0;
        if (i < scroll_pos)
        {
            setting_alpha = 0.0;
        }
        if (i >= scroll_pos + g_visible)
        {
            setting_alpha = 0.0;
        }
        AnimColor(g_settings[i], 0.7843, 0.7843, 0.7843, setting_alpha * meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
    }
}

/* Takes the cursor coming from the direction from ("FRST", UP, "DOWN", "LEFT", "RIGT"), or gives it up ("FOUT"). */
enter_focus(from)
{
    active = 1;
    if (from == TAG("FOUT"))
    {
        leave_focus();
        return;
    }
    if (from == TAG("LEFT"))
    {
        sndSePlay(SE_SCROLL);
    }
    else if (from == TAG("RIGT"))
    {
        sndSePlay(SE_SCROLL);
    }
    else if (from == TAG("UP"))
    {
        sndSePlay(SE_SCROLL);
        cursor = 0;
    }
    else if (from == TAG("DOWN"))
    {
        sndSePlay(SE_SCROLL);
        cursor = g_count - 1;
    }
    else if (from == TAG("FRST"))
    {
        printf("got initial focus.\n");
    }
    send_label_focus(focus, g_titles[cursor], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    update_colors(100.0, 0.0);
    update_pad_help(!g_states[cursor], g_up_enabled[cursor]);
}

/* Leaves the focus and dims the colors. */
leave_focus()
{
    active = 0;
    update_colors(100.0, 40.0);
}

/* Shows padhelp, plus the circle and the triangle if they are set. */
update_pad_help(circle, triangle)
{
    new buttons = padhelp;
    if (circle)
    {
        buttons = buttons | PADHELP_CIRCLE;
    }
    if (triangle)
    {
        buttons = buttons | PADHELP_TRIANGLE;
    }
    SetSystemParam(SYS_PADHELP, buttons);
}

/* Scrolls the list by one row if the cursor has left the visible rows. */
scroll_to_cursor()
{
    new scrolled;
    if (cursor < scroll_pos)
    {
        scroll_pos--;
        scrolled = 1;
    }
    else if (cursor >= scroll_pos + g_visible)
    {
        scroll_pos++;
        scrolled = 1;
    }
    if (scrolled)
    {
        update_colors(float(150), 0.0);
        for (new i; i < g_count; i++)
        {
            new Float:y = g_interval * (i - scroll_pos);
            /* NOTE: bug: both columns end up half of g_separation to the right of where main put them */
            AnimMoveLine(g_titles[i], 0.0, y, 0.0, float(150), 0.0, ANIM_FUNC_ANGULER);
            AnimMoveLine(g_settings[i], g_separation, y, 0.0, float(150), 0.0, ANIM_FUNC_ANGULER);
        }
    }
}
