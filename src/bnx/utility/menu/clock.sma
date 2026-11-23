/*
 * /opt0/bn/script/utility/menu/clock.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The clock of the utility menu: the date and the time in labels laid out as the system settings say, updated
 * every second; it declares its natives itself instead of including bn.inc.
 */

#pragma dynamic 1280

native swapchars(c);
native GetSystemParam(id);
native date(&date_year, &date_month, &date_day, const utc[]);
native time(&time_hour, &time_minute, &time_second, const utc[]);
native Self();
native CreateLabelShadow(parent);
native SetOptionP(obj, option, const data[], cells);
native SetOptionI(obj, option, value);
native SetPos(obj, Float:x, Float:y, Float:z, ref);
native SetColor(obj, Float:r, Float:g, Float:b, Float:a);
native FontID(font);
native SetOptionS(obj, option, const str[]);
native RecvMessage(&msg, &Float:a, &Float:b, &c=0, &d=0);
native Float:floatmul(Float:oper1, Float:oper2);
native AnimColor(obj, Float:r, Float:g, Float:b, Float:a, Float:duration, Float:delay, curve);
native sprintf(const dest[], const format[], ...);

#pragma rational Float
native Float:operator*(Float:oper1, Float:oper2) = floatmul;

/* As in bn.inc. */
const ANIM_FUNC_LINEAR = 1;
const ALIGN_CENTER = 1;
const SYS_TIME_NOTATION = 717;
const SYS_DATE_NOTATION = 718;
const DATE_YYYYMMDD = 0;
const DATE_MMDDYYYY = 1;
const DATE_DDMMYYYY = 2;
const BLEND_ADD = 2;

new g_shadow_colors[4] = {0x5F000000, 0x5F000000, 0x5F000000, 0x5F000000}; /* the "SCL4" of the shadowed labels */
public Float:meta_alpha; /* alpha of the whole clock, set by "FADE" */
public year; /* the date and time shown */
public month; /* 1 to 12 */
public day; /* 1 to 31 */
public hour; /* 0 to 23 */
public minute; /* 0 to 59 */
public second; /* 0 to 59 */
new g_fields[7]; /* year, month and day (in the order of g_date_fields), hour, minute, second, AM/PM */
new g_separators[4]; /* the two slashes of the date and the two colons of the time */
/* The x of the fields and of the separators, for each date notation. */
new Float:g_field_x[3][7] = {
    {-69.0, -23.0, 6.7, 38.0, 67.0, 96.0, 120.0}, {-82.0, -50.0, -5.0, 38.0, 67.0, 96.0, 120.0},
    {-82.0, -50.0, -5.0, 38.0, 67.0, 96.0, 120.0}
};
new Float:g_separator_x[3][4] = {{-40.0, -7.7, 53.0, 82.0}, {-66.0, -33.0, 53.0, 82.0}, {-66.0, -33.0, 53.0, 82.0}};
new g_separator_text[4][1] = {"/", "/", ":", ":"};
new g_date_fields[3] = {0, 1, 2}; /* the fields of the year, the month and the day */
new g_date_notation;
new g_time_notation; /* 0: 24 hours, else 12 hours with AM/PM */

/* Same as TAG of tag.inc. */
TAG(fourcc[])
{
    return swapchars(fourcc[0]);
}

main()
{
    g_date_notation = GetSystemParam(SYS_DATE_NOTATION);
    g_time_notation = GetSystemParam(SYS_TIME_NOTATION);
    switch (g_date_notation)
    {
        case DATE_YYYYMMDD:
        {
            g_date_fields = {0, 1, 2};
        }
        case DATE_MMDDYYYY:
        {
            g_date_fields = {2, 0, 1};
        }
        case DATE_DDMMYYYY:
        {
            g_date_fields = {2, 1, 0};
        }
    }
    /* g_shadow_colors is at address 0: these ask for the local time, like the ones below */
    date(year, month, day, g_shadow_colors);
    time(hour, minute, second, g_shadow_colors);
    for (new i; i < 7; i++)
    {
        g_fields[i] = CreateLabelShadow(Self());
        SetOptionP(g_fields[i], TAG("SCL4"), g_shadow_colors, 4);
        SetOptionI(g_fields[i], TAG("SX64"), 2 * 64);
        SetOptionI(g_fields[i], TAG("SY64"), 2 * 64);
        SetPos(g_fields[i], g_field_x[g_date_notation][i], i == 6 ? 1.5 : 0.0, 0.0, 0);
        SetColor(g_fields[i], 0.215, 0.708, 0.999, 0.0);
        SetOptionI(g_fields[i], TAG("FONT"), FontID(0));
        SetOptionI(g_fields[i], TAG("ALPH"), BLEND_ADD);
        SetOptionI(g_fields[i], TAG("HALN"), ALIGN_CENTER);
        SetOptionI(g_fields[i], TAG("VALN"), ALIGN_CENTER);
        SetOptionI(g_fields[i], TAG("FSIZ"), i == 6 ? 10 : 15);
    }
    for (new i; i < 4; i++)
    {
        g_separators[i] = CreateLabelShadow(Self());
        SetOptionP(g_separators[i], TAG("SCL4"), g_shadow_colors, 4);
        SetOptionI(g_separators[i], TAG("SX64"), 2 * 64);
        SetOptionI(g_separators[i], TAG("SY64"), 2 * 64);
        SetPos(g_separators[i], g_separator_x[g_date_notation][i], 0.0, 0.0, 0);
        SetColor(g_separators[i], 0.7843, 0.7843, 0.7843, 0.0);
        SetOptionI(g_separators[i], TAG("FONT"), FontID(0));
        SetOptionI(g_separators[i], TAG("ALPH"), BLEND_ADD);
        SetOptionI(g_separators[i], TAG("HALN"), ALIGN_CENTER);
        SetOptionI(g_separators[i], TAG("VALN"), ALIGN_CENTER);
        SetOptionI(g_separators[i], TAG("FSIZ"), 15);
        SetOptionS(g_separators[i], TAG("UTF8"), g_separator_text[i]);
    }
    fade_labels(100.0, 0.0);
    for (; ; )
    {
        date(year, month, day, g_shadow_colors);
        time(hour, minute, second, g_shadow_colors);
        static shown_second = -1;
        if (shown_second != second)
        {
            shown_second = second;
            show_time();
        }
        new msg;
        new Float:alpha;
        new Float:duration;
        /* "FADE" (alpha, duration) */
        while (RecvMessage(msg, alpha, duration))
        {
            if (msg == TAG("FADE"))
            {
                meta_alpha = alpha;
                fade_labels(duration, 0.0);
            }
        }
        sleep;
    }
}

/* Fades every label to meta_alpha. */
fade_labels(Float:duration, Float:delay)
{
    for (new i; i < 7; i++)
    {
        AnimColor(g_fields[i], 0.7843, 0.7843, 0.7843, 1.0 * meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
    }
    for (new i; i < 4; i++)
    {
        AnimColor(g_separators[i], 0.7843, 0.7843, 0.7843, 1.0 * meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
    }
}

show_time()
{
    new text[8];
    sprintf(text, "%04d", year);
    SetOptionS(g_fields[g_date_fields[0]], TAG("UTF8"), text);
    sprintf(text, "%02d", month);
    SetOptionS(g_fields[g_date_fields[1]], TAG("UTF8"), text);
    sprintf(text, "%02d", day);
    SetOptionS(g_fields[g_date_fields[2]], TAG("UTF8"), text);
    if (g_time_notation)
    {
        sprintf(text, "%02d", (hour - 1) % 12 + 1);
    }
    else
    {
        sprintf(text, "%02d", hour);
    }
    SetOptionS(g_fields[3], TAG("UTF8"), text);
    sprintf(text, "%02d", minute);
    SetOptionS(g_fields[4], TAG("UTF8"), text);
    sprintf(text, "%02d", second);
    SetOptionS(g_fields[5], TAG("UTF8"), text);
    if (g_time_notation)
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
    SetOptionS(g_fields[6], TAG("UTF8"), text);
}
