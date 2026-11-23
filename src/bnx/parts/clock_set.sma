/*
 * /opt0/bn/script/parts/clock_set.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The date and time setting, in the date and time notation of the system: left and right pick a field, up and
 * down change it. The circle ends; the cross, except at the first setup (argument "ps2init"), cancels.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

#include "wait.inc"

#include "tag.inc"

/* The years the clock can be set to. */
const YEAR_MIN = 2000;
const YEAR_MAX = 2030;

/* The change normalize_date_time follows: an out of range year is clamped with none, wrapped with a step. */
const STEP_NONE = 0;
const STEP_UP = 1;
const STEP_DOWN = 2;

/* The fields of cursor and g_field_labels: the three of the date in display order, then the time. */
const FIELD_DATE1 = 0;
const FIELD_DATE2 = 1;
const FIELD_DATE3 = 2;
const FIELD_HOUR = 3;
const FIELD_MINUTE = 4;
const FIELD_SECOND = 5;
const FIELD_AM_PM = 6; /* a label only, after the last field of the cursor */

new Float:g_pi = 3.141592;

/* Same as send_focus of dom.inc. */
send_focus(obj, target, z_offset, Float:sharp, Float:extra_w, Float:extra_h, Float:move_time, smoothing, blinking)
{
    if (!obj)
    {
        return;
    }
    SendMessage(obj, TAG("FOC0"), target, TAG(""), z_offset, sharp);
    SendMessage(obj, TAG("FOC1"), extra_w, extra_h, move_time, smoothing);
    SendMessage(obj, TAG("FOC2"), blinking, 0, 0, 0);
}

#include "misc.inc"

new g_shadow_colors[4] = {0x5F000000, 0x5F000000, 0x5F000000, 0x5F000000}; /* the "SCL4" of the shadowed labels */
public manager; /* unused, but part of the layout */
public focus; /* the focus object */
public cursor; /* the field under the cursor (FIELD_*) */
/* The date and time being set; all -1 when the cross cancelled. */
public year; /* the date and time; all -1 when the cross cancels */
public month; /* 1 to 12 */
public day; /* 1 to 31 */
public hour; /* 0 to 23 */
public minute; /* 0 to 59 */
public second; /* 0 to 59 */
new g_field_labels[7];
new g_separator_labels[4];
new g_arrows[2];
new g_arrow_blinks[2];
new g_first_setup; /* the setting cannot be cancelled */
new Float:g_field_x[3][7] = {
    {-80.0, -25.0, 10.0, 50.0, 85.0, 120.0, 150.0}, {-90.0, -50.0, 0.0, 50.0, 85.0, 120.0, 150.0},
    {-90.0, -50.0, 0.0, 50.0, 85.0, 120.0, 150.0}
};
new Float:g_separator_x[3][4] = {{-45.0, -7.5, 67.5, 102.5}, {-68.0, -29.0, 67.5, 102.5}, {-68.0, -29.0, 67.5, 102.5}};
new g_date_order[3] = {FIELD_DATE1, FIELD_DATE2, FIELD_DATE3}; /* the field of the year, the month and the day */
new g_date_notation; /* the order of year, month and day (DATE_*) */
new g_time_12h;

main()
{
    g_first_setup = 0;
    new value[4];
    if (GetArg("ps2init", value, 4))
    {
        if (atoi(value) == 1)
        {
            g_first_setup = 1;
        }
    }
    g_date_notation = GetSystemParam(SYS_DATE_NOTATION);
    g_time_12h = GetSystemParam(SYS_TIME_NOTATION);
    switch (g_date_notation)
    {
        case DATE_YYYYMMDD:
        {
            g_date_order = {FIELD_DATE1, FIELD_DATE2, FIELD_DATE3};
        }
        case DATE_MMDDYYYY:
        {
            g_date_order = {FIELD_DATE3, FIELD_DATE1, FIELD_DATE2};
        }
        case DATE_DDMMYYYY:
        {
            g_date_order = {FIELD_DATE3, FIELD_DATE2, FIELD_DATE1};
        }
    }
    for (new i; i < 7; i++)
    {
        g_field_labels[i] = CreateLabelShadow(Self());
        SetOptionP(g_field_labels[i], TAG("SCL4"), g_shadow_colors, 4);
        SetOptionI(g_field_labels[i], TAG("SX64"), 2 * 64);
        SetOptionI(g_field_labels[i], TAG("SY64"), 2 * 64);
        SetPos(g_field_labels[i], g_field_x[g_date_notation][i], 0.0, 0.0, 0);
        SetColor(g_field_labels[i], 0.215, 0.708, 0.999, 0.0);
        SetOptionI(g_field_labels[i], TAG("ALPH"), BLEND_ADD);
        SetOptionI(g_field_labels[i], TAG("HALN"), ALIGN_CENTER);
        SetOptionI(g_field_labels[i], TAG("VALN"), ALIGN_CENTER);
        SetOptionI(g_field_labels[i], TAG("FSIZ"), i == FIELD_AM_PM ? 12 : 15);
    }
    for (new i; i < 4; i++)
    {
        g_separator_labels[i] = CreateLabelShadow(Self());
        static separators[4][1] = {"/", "/", ":", ":"};
        SetOptionP(g_separator_labels[i], TAG("SCL4"), g_shadow_colors, 4);
        SetOptionI(g_separator_labels[i], TAG("SX64"), 2 * 64);
        SetOptionI(g_separator_labels[i], TAG("SY64"), 2 * 64);
        SetPos(g_separator_labels[i], g_separator_x[g_date_notation][i], 0.0, 0.0, 0);
        SetColor(g_separator_labels[i], 0.7843, 0.7843, 0.7843, 0.0);
        SetOptionI(g_separator_labels[i], TAG("ALPH"), BLEND_ADD);
        SetOptionI(g_separator_labels[i], TAG("HALN"), ALIGN_CENTER);
        SetOptionI(g_separator_labels[i], TAG("VALN"), ALIGN_CENTER);
        SetOptionI(g_separator_labels[i], TAG("FSIZ"), 15);
        SetOptionS(g_separator_labels[i], TAG("UTF8"), separators[i]);
    }
    normalize_date_time(STEP_NONE);
    send_label_focus2(focus, g_field_labels[cursor], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    show_date_time();
    show_pad_help();
    update_arrows();
    fade_fields(1.0, 400.0, 0.0);
    wait_ms(400);
    for (; ; )
    {
        {
            new repeated;
            new pressed;
            PadState(_, pressed, _, repeated, -1);
            if (repeated & PAD_LEFT && cursor - 1 >= 0)
            {
                cursor--;
                sndSePlay(SE_CURSOR);
                send_label_focus2(focus, g_field_labels[cursor], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                show_date_time();
                update_arrows();
            }
            else if (repeated & PAD_RIGHT && cursor + 1 < FIELD_AM_PM)
            {
                cursor++;
                sndSePlay(SE_CURSOR);
                send_label_focus2(focus, g_field_labels[cursor], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                show_date_time();
                update_arrows();
            }
            else if (repeated & PAD_UP)
            {
                increase_field();
                blink_arrow(0);
            }
            else if (repeated & PAD_DOWN)
            {
                decrease_field();
                blink_arrow(1);
            }
            if (pressed & PAD_CIRCLE)
            {
                sndSePlay(SE_DECIDE);
                send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                break;
            }
            if (!g_first_setup)
            {
                if (pressed & PAD_CROSS)
                {
                    year = -1;
                    month = -1;
                    day = -1;
                    hour = -1;
                    minute = -1;
                    second = -1;
                    sndSePlay(SE_CANCEL);
                    send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                    break;
                }
            }
        }
        update_arrow_blinks();
        sleep;
    }
    fade_fields(0.0, 300.0, 0.0);
    fade_out_arrows();
    return;
}

/* Fades the fields and the separators to alpha. */
fade_fields(Float:alpha, Float:duration, Float:delay)
{
    for (new i; i < 7; i++)
    {
        AnimColor(g_field_labels[i], 0.215, 0.708, 0.999, alpha, duration, delay, ANIM_FUNC_LINEAR);
    }
    for (new i; i < 4; i++)
    {
        AnimColor(g_separator_labels[i], 0.7843, 0.7843, 0.7843, alpha, duration, delay, ANIM_FUNC_LINEAR);
    }
}

show_date_time()
{
    new text[8];
    sprintf(text, "%04d", year);
    SetOptionS(g_field_labels[g_date_order[0]], TAG("UTF8"), text);
    sprintf(text, "%02d", month);
    SetOptionS(g_field_labels[g_date_order[1]], TAG("UTF8"), text);
    sprintf(text, "%02d", day);
    SetOptionS(g_field_labels[g_date_order[2]], TAG("UTF8"), text);
    if (g_time_12h)
    {
        sprintf(text, "%02d", (hour - 1) % 12 + 1);
    }
    else
    {
        sprintf(text, "%02d", hour);
    }
    SetOptionS(g_field_labels[FIELD_HOUR], TAG("UTF8"), text);
    sprintf(text, "%02d", minute);
    SetOptionS(g_field_labels[FIELD_MINUTE], TAG("UTF8"), text);
    sprintf(text, "%02d", second);
    SetOptionS(g_field_labels[FIELD_SECOND], TAG("UTF8"), text);
    if (g_time_12h)
    {
        if (hour >= 12 && hour < 24)
        {
            sprintf(text, "PM");
        }
        else
        {
            sprintf(text, "AM");
        }
    }
    else
    {
        sprintf(text, "");
    }
    SetOptionS(g_field_labels[FIELD_AM_PM], TAG("UTF8"), text);
}

new Float:g_arrow_dx[6];
new Float:g_arrow_y[2] = {-14.0, 14.0};

/*
 * Carries the seconds into the minutes and the minutes into the hour, wraps the hour, the day and the month, keeps
 * the year between YEAR_MIN and YEAR_MAX, and the date and time not earlier than 1 January 2000, 0:00 in Japan.
 */
normalize_date_time(step)
{
    if (second > 59)
    {
        minute++;
        second = 0;
    }
    else if (second < 0)
    {
        minute--;
        second = 59;
    }
    if (minute > 59)
    {
        hour++;
        minute = 0;
    }
    else if (minute < 0)
    {
        hour--;
        minute = 59;
    }
    if (hour > 23)
    {
        hour = 0;
    }
    else if (hour < 0)
    {
        hour = 23;
    }
    if (day > get_days_in_month())
    {
        if (cursor != g_date_order[2])
        {
            day = get_days_in_month();
        }
        else
        {
            day = 1;
        }
    }
    else if (day < 1)
    {
        day = get_days_in_month();
    }
    if (month > 12)
    {
        month = 1;
    }
    else if (month < 1)
    {
        month = 12;
    }
    switch (step)
    {
        case STEP_NONE:
        {
            if (year > YEAR_MAX)
            {
                year = YEAR_MAX;
            }
            else if (year < YEAR_MIN)
            {
                year = YEAR_MIN;
            }
        }
        case STEP_UP, STEP_DOWN:
        {
            if (year > YEAR_MAX)
            {
                year = YEAR_MIN;
            }
            else if (year < YEAR_MIN)
            {
                year = YEAR_MAX;
            }
        }
    }
    if (year == YEAR_MIN && month == 1 && day == 1)
    {
        new minutes = hour * 60 + minute;
        new earliest = GetSystemParam(SYS_TIME_ZONE) - 540; /* the time zone is in minutes; Japan is 540 */
        if (minutes - earliest < 0)
        {
            switch (step)
            {
                case STEP_NONE, STEP_UP:
                {
                    hour = earliest / 60;
                    if (earliest % 60)
                    {
                        minute = earliest % 60;
                    }
                }
                case STEP_DOWN:
                {
                    hour = 23;
                }
            }
        }
    }
    if (day > get_days_in_month())
    {
        day = get_days_in_month();
    }
}

is_leap_year()
{
    if (0 == year % 400)
    {
        return 1;
    }
    if (0 == year % 4 && 0 != year % 100)
    {
        return 1;
    }
    return 0;
}

get_days_in_month()
{
    if (month == 2)
    {
        if (is_leap_year())
        {
            return 29;
        }
        else
        {
            return 28;
        }
    }
    else if (month <= 7)
    {
        if (0 == month % 2)
        {
            return 30;
        }
        else
        {
            return 31;
        }
    }
    else
    {
        if (0 == month % 2)
        {
            return 31;
        }
        return 30;
    }
    return 0;
}

/* Adds delta to the date field in display position field. */
add_to_date(field, delta)
{
    switch (g_date_notation)
    {
        case DATE_YYYYMMDD:
        {
            switch (field)
            {
                case FIELD_DATE1:
                {
                    year = year + delta;
                }
                case FIELD_DATE2:
                {
                    month = month + delta;
                }
                case FIELD_DATE3:
                {
                    day = day + delta;
                }
            }
        }
        case DATE_MMDDYYYY:
        {
            switch (field)
            {
                case FIELD_DATE1:
                {
                    month = month + delta;
                }
                case FIELD_DATE2:
                {
                    day = day + delta;
                }
                case FIELD_DATE3:
                {
                    year = year + delta;
                }
            }
        }
        case DATE_DDMMYYYY:
        {
            switch (field)
            {
                case FIELD_DATE1:
                {
                    day = day + delta;
                }
                case FIELD_DATE2:
                {
                    month = month + delta;
                }
                case FIELD_DATE3:
                {
                    year = year + delta;
                }
            }
        }
    }
}

/* Steps the field under the cursor up, with a sound if anything changed. */
increase_field()
{
    new old_year = year;
    new old_month = month;
    new old_day = day;
    new old_hour = hour;
    new old_minute = minute;
    new old_second = second;
    switch (cursor)
    {
        case FIELD_DATE1 .. FIELD_DATE3:
        {
            add_to_date(cursor, 1);
        }
        case FIELD_HOUR:
        {
            hour++;
        }
        case FIELD_MINUTE:
        {
            minute++;
        }
        case FIELD_SECOND:
        {
            second++;
        }
    }
    normalize_date_time(STEP_UP);
    show_date_time();
    if (old_year != year || old_month != month || old_day != day || old_hour != hour || old_minute != minute
            || old_second != second)
    {
        sndSePlay(SE_CURSOR);
    }
}

/* Steps the field under the cursor down, with a sound if anything changed. */
decrease_field()
{
    new old_year = year;
    new old_month = month;
    new old_day = day;
    new old_hour = hour;
    new old_minute = minute;
    new old_second = second;
    switch (cursor)
    {
        case FIELD_DATE1 .. FIELD_DATE3:
        {
            add_to_date(cursor, -1);
        }
        case FIELD_HOUR:
        {
            hour--;
        }
        case FIELD_MINUTE:
        {
            minute--;
        }
        case FIELD_SECOND:
        {
            second--;
        }
    }
    normalize_date_time(STEP_DOWN);
    show_date_time();
    if (old_year != year || old_month != month || old_day != day || old_hour != hour || old_minute != minute
            || old_second != second)
    {
        sndSePlay(SE_CURSOR);
    }
}

/* Shows the pad help: the circle, and the cross unless at the first setup. */
show_pad_help()
{
    new buttons;
    buttons = buttons | PADHELP_CIRCLE;
    if (!g_first_setup)
    {
        buttons = buttons | PADHELP_CROSS;
    }
    SetSystemParam(SYS_PADHELP, buttons);
}

/* Creates the arrows above and below the field under the cursor, and moves them to it. */
update_arrows()
{
    if (!g_arrows[0])
    {
        g_arrows[0] = CreatePlane(Self());
        SetPos(g_arrows[0], g_field_x[g_date_notation][cursor] + g_arrow_dx[cursor], g_arrow_y[0], 0.0, 0);
        SetScale(g_arrows[0], 17.0, 8.0, 1.0);
        SetColor(g_arrows[0], 1.0, 1.0, 1.0, 0.0);
        SetRot(g_arrows[0], 0.0, 0.0, g_pi, 0, 0);
        SetTexture(g_arrows[0], imgGetResource(RES_SPIN), 0, -1);
    }
    if (!g_arrows[1])
    {
        g_arrows[1] = CreatePlane(Self());
        SetPos(g_arrows[1], g_field_x[g_date_notation][cursor] + g_arrow_dx[cursor], g_arrow_y[1], 0.0, 0);
        SetScale(g_arrows[1], 17.0, 8.0, 1.0);
        SetColor(g_arrows[1], 1.0, 1.0, 1.0, 0.0);
        SetTexture(g_arrows[1], imgGetResource(RES_SPIN), 0, -1);
    }
    AnimMoveLine(g_arrows[0], g_field_x[g_date_notation][cursor] + g_arrow_dx[cursor], g_arrow_y[0], 0.0, 100.0, 0.0,
            ANIM_FUNC_LINEAR);
    AnimMoveLine(g_arrows[1], g_field_x[g_date_notation][cursor] + g_arrow_dx[cursor], g_arrow_y[1], 0.0, 100.0, 0.0,
            ANIM_FUNC_LINEAR);
    AnimColor(g_arrows[0], 1.0, 1.0, 1.0, 1.0, 1000.0, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_arrows[1], 1.0, 1.0, 1.0, 1.0, 1000.0, 0.0, ANIM_FUNC_LINEAR);
}

fade_out_arrows()
{
    if (g_arrows[0])
    {
        AnimColor(g_arrows[0], 1.0, 1.0, 1.0, 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    }
    if (g_arrows[1])
    {
        AnimColor(g_arrows[1], 1.0, 1.0, 1.0, 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    }
}

/* Starts a blink of arrow index (0 up, 1 down) unless one is running. */
blink_arrow(index)
{
    if (!g_arrow_blinks[index])
    {
        index = clamp(index, 0, 1);
        g_arrow_blinks[index] = ms_to_frames(100) * 2;
        AnimColor(g_arrows[index], 1.0, 1.0, 1.0, 0.0, float(100), 0.0, ANIM_FUNC_LINEAR);
    }
}

update_arrow_blinks()
{
    for (new i; i < 2; i++)
    {
        if (g_arrows[i] && g_arrow_blinks[i])
        {
            g_arrow_blinks[i]--;
            if (g_arrow_blinks[i] == ms_to_frames(100))
            {
                AnimColor(g_arrows[i], 1.0, 1.0, 1.0, 1.0, float(100), 0.0, ANIM_FUNC_LINEAR);
            }
        }
    }
}
