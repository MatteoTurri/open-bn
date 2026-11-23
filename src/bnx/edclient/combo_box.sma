/*
 * /opt0/bn/script/edclient/combo_box.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A drop-down list of an edclient form: the text chosen, and on circle the "text" children of the menu element of
 * argument "menu_id" below it, g_iVisible lines at a time; circle again confirms it ("CHNG" to the manager).
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "edpage.inc"

#include "tag.inc"

/* Values of g_iMode. */
enum
{
    MODE_NAVIGATE, /* the focus is on the text chosen */
    MODE_CHOOSE,   /* the list is open */
};

/* Values of g_iScroll: the scroll that runs. */
enum
{
    SCROLL_NONE,
    SCROLL_DOWN, /* a line added at the bottom: the top one goes when it ends */
    SCROLL_UP,
};

#include <float>

/* Returns whether str is a packed string. */
is_packed(str[])
{
    return str[0] <= 0 || str[0] > 255;
}

/* Copies the packed or unpacked string src into dest, at most maxlen cells. */
copy_string(dest[], src[], maxlen)
{
    new len = strlen(src) + 1;
    if (is_packed(src))
    {
        len = (len + 3) / 4;
    }
    len = min(len, maxlen);
    for (new i; i < len; i++)
    {
        dest[i] = src[i];
    }
}

#include "sysconf.inc"

#include "background.inc"

public g_objManager; /* the page: gets "PUSH", "CHNG", "FOCS" when the focus leaves, and the text for "GTXT" */
public g_objFocus; /* the focus object, moved onto the text or the line */
public g_iVisible = 1; /* lines of the open list */
public g_iFontSize = 15; /* the font size of the text, in pixels ("FSIZ") */
public Float:g_flListInterval = 24.0; /* from the text to the first line of the list */
public g_bHide; /* 1: the text is hidden while the list is open */
public Float:g_flSpeed = 200.0; /* the time of a fade and of the slowest scroll, in ms */
public g_iPadHelp; /* the SYS_PADHELP buttons while the list has the focus, besides circle */
new g_bActive; /* the list has the focus and takes the pad */
new g_iMode;
new g_iScroll;
new g_szText[64]; /* the text chosen; argument "text" first */
new g_objLabel;
new g_iSelected;
new g_iCursor; /* the line of g_iSelected in the open list */
new g_iOldSelected; /* g_iSelected and g_iCursor when the list opened, back on cross */
new g_iOldCursor;
new g_iCount;
new g_objTimer; /* the timer of the scroll */
new g_iRepeated;
new g_iPressed;
new g_iScrollTick; /* the tickcount of the last scroll: a scroll lasts the time since the one before */
new g_iPrevTick;
new g_iMaxTime;
new g_iMinTime;
new g_tagMenu;

main()
{
    Init();
    for (; ; )
    {
        Update();
        sleep;
    }
}

/* Reads the text of argument "text" and counts the "text" children of the menu of argument "menu_id". */
Init()
{
    new text[64];
    new found;
    found = GetArg("text", text, 64);
    if (found == 1)
    {
        copy_string(g_szText, text, 64);
    }
    g_iVisible = g_iVisible < 1 ? 1 : g_iVisible;
    found = GetArg("menu_id", text, 64);
    if (found == 1)
    {
        g_tagMenu = domFindTag(domRoot(), "menu", text, 0);
        new node;
        node = domFirstChild(g_tagMenu);
        for (; node; node = domNext(node))
        {
            domGetTagName(node, text, 64);
            if (0 != strcmp("text", text))
            {
                continue;
            }
            g_iCount++;
        }
    }
    g_objLabel = CreateItemLabel(Self(), 0.0, 0.0, g_szText);
    AnimColor(g_objLabel, 0.215, 0.708, 0.999, 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
    if (strlen(g_szText) > 0)
    {
        g_iSelected = FindItem(g_tagMenu, g_szText);
        g_iCursor = g_iSelected > g_iCount - g_iVisible ? g_iSelected - (g_iCount - g_iVisible) : 0;
    }
    g_iMaxTime = floatround(g_flSpeed, 0);
    g_iMinTime = g_iMaxTime < 60 ? g_iMaxTime : 60;
    g_objTimer = CreateNull(Self());
}

/* Handles the messages and the scroll, then the pad while the box has the focus, by g_iMode. */
Update()
{
    ProcessMessages();
    UpdateScroll();
    if (0 == g_bActive)
    {
        return;
    }
    PadState(_, g_iPressed, _, g_iRepeated, -1);
    switch (g_iMode)
    {
        case MODE_NAVIGATE:
        {
            UpdateNavigate();
        }
        case MODE_CHOOSE:
        {
            UpdateChoose();
        }
        default:
        {
        }
    }
}

ProcessMessages()
{
    new msg;
    new a;
    new Float:b;
    /* "FOCS": a the direction; "FADE": a the alpha, b the duration; "PUSH" and "CHNG" give the list the pad back. */
    while (RecvMessage(msg, a, b))
    {
        if (msg == TAG("FOCS"))
        {
            HandleFocus(a);
        }
        else if (msg == TAG("FADE"))
        {
            new Float:alpha = a;
            new Float:duration = b;
            AnimColor(g_objLabel, 0.215, 0.708, 0.999, 1.0 * alpha, duration, 0.0, ANIM_FUNC_LINEAR);
            for (new i; i < listSize(); i++)
            {
                AnimColor(listGetAt(i), 0.215, 0.708, 0.999, 1.0 * alpha, duration, 0.0, ANIM_FUNC_LINEAR);
            }
        }
        else if (msg == TAG("PUSH"))
        {
            g_bActive = 1;
        }
        else if (msg == TAG("CHNG"))
        {
            g_bActive = 1;
        }
        else if (msg == TAG("GTXT"))
        {
            SendMessageStr(g_objManager, TAG("GTXT"), g_szText);
        }
    }
}

/* Any dir but "FOUT" gives the focus to the text chosen. */
HandleFocus(dir)
{
    if (dir == TAG("FOUT"))
    {
        return;
    }
    if (dir == TAG("FRST"))
    {
    }
    else if (dir == TAG("RTRN"))
    {
    }
    send_label_focus(g_objFocus, g_objLabel, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    ShowPadHelp();
    g_bActive = 1;
}

/* Removes the line scrolled out when the scroll ends. */
UpdateScroll()
{
    if (SCROLL_NONE == g_iScroll)
    {
        return;
    }
    switch (AnimGetState(g_objTimer, ANIM_TIMER))
    {
        case ANIM_DONE:
        {
            if (g_iScroll == SCROLL_DOWN)
            {
                Destroy(listGetAt(0));
                listRemoveAt(0);
            }
            else if (g_iScroll == SCROLL_UP)
            {
                Destroy(listGetLast());
                listRemoveLast();
            }
            g_iScroll = SCROLL_NONE;
        }
        case ANIM_DELAY:
        {
        }
        case ANIM_RUN:
        {
        }
        default:
        {
        }
    }
}

/* Circle opens the list at the text chosen. */
UpdateNavigate()
{
    new i;
    new Float:y;
    new text[64];
    if (g_iPressed & PAD_CIRCLE)
    {
        SendMessage(g_objManager, TAG("PUSH"), 0, TAG("RRGT"), 0, 0);
        g_bActive = 0;
        g_iMode = MODE_CHOOSE;
        i = 0;
        for (; i < listSize(); i++)
        {
            Destroy(listGetAt(i));
        }
        listRemoveAll();
        {
            new first = g_iSelected - g_iCursor;
            y = g_flListInterval;
            i = 0;
            for (; i < g_iVisible; i++)
            {
                new label;
                GetItemText(first + i, text, 64);
                label = CreateItemLabel(Self(), 0.0, y, text);
                listAddLast(label);
                y = y + float(g_iFontSize);
            }
            g_iOldSelected = g_iSelected;
            g_iOldCursor = g_iCursor;
        }
        if (g_bHide == 1)
        {
            AnimColor(g_objLabel, 0.215, 0.708, 0.999, 1.0 * 0.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
        }
        i = 0;
        for (; i < listSize(); i++)
        {
            AnimColor(listGetAt(i), 0.215, 0.708, 0.999, 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
        }
        send_label_focus(g_objFocus, listGetAt(g_iCursor), 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        play_se(ED_SE_DECIDE);
    }
    else if (g_iPressed & PAD_CROSS)
    {
        SendMessage(g_objManager, TAG("PUSH"), 0, TAG("RDWN"), 0, 0);
        g_bActive = 0;
    }
    else if (g_iPressed & PAD_RIGHT)
    {
        SendMessage(g_objManager, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
        g_bActive = 0;
    }
    else if (g_iPressed & PAD_LEFT)
    {
        SendMessage(g_objManager, TAG("FOCS"), TAG("LEFT"), 0, 0, 0);
        g_bActive = 0;
    }
    else if (g_iPressed & PAD_UP)
    {
        SendMessage(g_objManager, TAG("FOCS"), TAG("UP"), 0, 0, 0);
        g_bActive = 0;
    }
    else if (g_iPressed & PAD_DOWN)
    {
        SendMessage(g_objManager, TAG("FOCS"), TAG("DOWN"), 0, 0, 0);
        g_bActive = 0;
    }
}

/* Up and down move through the list, scrolling at its ends; circle confirms the text, cross gives the list up. */
UpdateChoose()
{
    new i;
    new text[64];
    new Float:y;
    if (g_iPressed & PAD_CIRCLE)
    {
        GetItemText(g_iSelected, g_szText, 64);
        SetOptionS(g_objLabel, TAG("UTF8"), g_szText);
        g_iMode = MODE_NAVIGATE;
        if (g_bHide == 1)
        {
            AnimColor(g_objLabel, 0.215, 0.708, 0.999, 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
        }
        i = 0;
        for (; i < listSize(); i++)
        {
            AnimColor(listGetAt(i), 0.215, 0.708, 0.999, 1.0 * 0.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
        }
        send_label_focus(g_objFocus, g_objLabel, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        play_se(ED_SE_DECIDE);
        SendMessage(g_objManager, TAG("CHNG"), 0, 0, 0, 0);
        g_bActive = 0;
    }
    else if (g_iPressed & PAD_CROSS)
    {
        g_iSelected = g_iOldSelected;
        g_iCursor = g_iOldCursor;
        g_iMode = MODE_NAVIGATE;
        if (g_bHide == 1)
        {
            AnimColor(g_objLabel, 0.215, 0.708, 0.999, 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
        }
        i = 0;
        for (; i < listSize(); i++)
        {
            AnimColor(listGetAt(i), 0.215, 0.708, 0.999, 1.0 * 0.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
        }
        send_label_focus(g_objFocus, g_objLabel, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        play_se(ED_SE_CANCEL);
    }
    else if (g_iRepeated & PAD_UP)
    {
        if (g_iSelected > 0)
        {
            g_iSelected = g_iSelected > 0 ? --g_iSelected : g_iSelected; /* redundant as written */
            if (0 == g_iCursor)
            {
                if (SCROLL_NONE != g_iScroll)
                {
                    if (ANIM_DONE != AnimGetState(g_objTimer, ANIM_TIMER))
                    {
                        if (g_iScroll == SCROLL_DOWN)
                        {
                            Destroy(listGetAt(0));
                            listRemoveAt(0);
                        }
                        else if (g_iScroll == SCROLL_UP)
                        {
                            Destroy(listGetLast());
                            listRemoveLast();
                        }
                        g_iScroll = SCROLL_NONE;
                    }
                }
                g_iPrevTick = g_iScrollTick;
                g_iScrollTick = tickcount();
                new scroll_time = g_iScrollTick - g_iPrevTick;
                scroll_time = clamp(scroll_time, g_iMinTime, g_iMaxTime);
                new label;
                y = g_flListInterval - float(g_iFontSize);
                GetItemText(g_iSelected, text, 64);
                label = CreateItemLabel(Self(), 0.0, y, text);
                AnimColor(label, 0.215, 0.708, 0.999, 1.0, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
                listInsert(0, label);
                AnimColor(listGetLast(), 0.215, 0.708, 0.999, 1.0 * 0.0, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
                y = g_flListInterval;
                i = 0;
                for (; i < listSize(); i++)
                {
                    AnimMoveLine(listGetAt(i), 0.0, y, 0.0, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
                    y = y + float(g_iFontSize);
                }
                g_iScroll = SCROLL_UP;
                AnimTimerManager(g_objTimer, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
                if (0.0 == g_flSpeed && g_iVisible == 1)
                {
                    send_label_focus(g_objFocus, listGetAt(0), 0, 0.5, 60.0, 60.0, 150.0, 0, 1);
                }
                else
                {
                    send_label_focus(g_objFocus, listGetAt(0), 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                }
            }
            else
            {
                g_iCursor = g_iCursor > 0 ? --g_iCursor : g_iCursor; /* redundant as written */
                send_label_focus(g_objFocus, listGetAt(g_iCursor), 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            }
            play_se(ED_SE_CURSOR);
        }
    }
    else if (g_iRepeated & PAD_DOWN)
    {
        if (g_iSelected < g_iCount - 1)
        {
            g_iSelected = g_iSelected < g_iCount - 1 ? ++g_iSelected : g_iSelected; /* redundant as written */
            if (g_iCursor == g_iVisible - 1)
            {
                if (SCROLL_NONE != g_iScroll)
                {
                    if (ANIM_DONE != AnimGetState(g_objTimer, ANIM_TIMER))
                    {
                        if (g_iScroll == SCROLL_DOWN)
                        {
                            Destroy(listGetAt(0));
                            listRemoveAt(0);
                        }
                        else if (g_iScroll == SCROLL_UP)
                        {
                            Destroy(listGetLast());
                            listRemoveLast();
                        }
                        g_iScroll = SCROLL_NONE;
                    }
                }
                g_iPrevTick = g_iScrollTick;
                g_iScrollTick = tickcount();
                new scroll_time = g_iScrollTick - g_iPrevTick;
                scroll_time = clamp(scroll_time, g_iMinTime, g_iMaxTime);
                new label;
                y = g_flListInterval + listSize() * float(g_iFontSize);
                GetItemText(g_iSelected, text, 64);
                label = CreateItemLabel(Self(), 0.0, y, text);
                AnimColor(label, 0.215, 0.708, 0.999, 1.0, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
                listAddLast(label);
                AnimColor(listGetAt(0), 0.215, 0.708, 0.999, 1.0 * 0.0, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
                y = g_flListInterval - float(g_iFontSize);
                i = 0;
                for (; i < listSize(); i++)
                {
                    AnimMoveLine(listGetAt(i), 0.0, y, 0.0, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
                    y = y + float(g_iFontSize);
                }
                g_iScroll = SCROLL_DOWN;
                AnimTimerManager(g_objTimer, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
                if (0.0 == g_flSpeed && g_iVisible == 1)
                {
                    send_label_focus(g_objFocus, listGetLast(), 0, 0.5, 60.0, 60.0, 150.0, 0, 1);
                }
                else
                {
                    send_label_focus(g_objFocus, listGetLast(), 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                }
            }
            else
            {
                g_iCursor = g_iCursor < g_iVisible - 1 ? ++g_iCursor : g_iCursor; /* redundant as written */
                send_label_focus(g_objFocus, listGetAt(g_iCursor), 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            }
            play_se(ED_SE_CURSOR);
        }
    }
}

/* Creates a text of the list at x, y, aligned left, hidden. */
CreateItemLabel(parent, Float:x, Float:y, text[])
{
    new label;
    label = CreateLabel(parent);
    SetPos(label, x, y, 0.0, 0);
    SetColor(label, 0.215, 0.708, 0.999, 0.0);
    SetOptionI(label, TAG("ALPH"), BLEND_ADD);
    SetOptionI(label, TAG("HALN"), ALIGN_LEFT);
    SetOptionI(label, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(label, TAG("FSIZ"), g_iFontSize);
    SetOptionS(label, TAG("UTF8"), text);
    return label;
}

/* Shows the buttons of g_iPadHelp and circle in the pad help. */
ShowPadHelp()
{
    SetSystemParam(SYS_PADHELP, g_iPadHelp | PADHELP_CIRCLE);
}

/* Reads the value of item index of the menu; returns 0 if there is no such item. */
GetItemText(index, text[], maxlen)
{
    new n;
    new node;
    new tag_name[64];
    text{0} = 0;
    n = 0;
    node = domFirstChild(g_tagMenu);
    for (; node; node = domNext(node))
    {
        domGetTagName(node, tag_name, 64);
        if (0 != strcmp("text", tag_name))
        {
            continue;
        }
        if (n == index)
        {
            domGetData(node, "value", text, maxlen);
            return 1;
        }
        n++;
    }
    return 0;
}

/* Returns the index of the item of menu whose value is text, 0 if there is none. */
FindItem(menu, text[])
{
    new n;
    new node;
    new value[64];
    new len;
    n = 0;
    node = domFirstChild(menu);
    for (; node; node = domNext(node))
    {
        domGetTagName(node, value, 64);
        if (0 != strcmp("text", value))
        {
            continue;
        }
        len = domGetData(node, "value", value, 64);
        if (0 == len)
        {
            continue;
        }
        if (0 == strcmp(text, value))
        {
            return n;
        }
        n++;
    }
    return 0;
}
