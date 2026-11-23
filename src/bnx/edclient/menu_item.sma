/*
 * /opt0/bn/script/edclient/menu_item.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A vertical menu of an edclient page: the "item" children of the menu element of argument "menu_id", of which
 * g_iVisible lines are shown and scrolled with up and down.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

#include "edpage.inc"

#include "tag.inc"

#include "text.inc"

#include "sysconf.inc"

#include "background.inc"

/* Values of g_iScroll: the scroll that runs. */
enum
{
    SCROLL_NONE,
    SCROLL_DOWN, /* a line added at the bottom: the top one goes when it ends */
    SCROLL_UP,
};

public g_objManager; /* the page: gets "CUSR" with the item, "PUSH" for the buttons, "FOCS" when the focus leaves */
public g_objFocus; /* the focus object, moved onto the current line */
public g_iVisible = 1; /* lines shown */
public g_iAlignH; /* the "HALN" of the lines */
public g_iFontSize = 15; /* the font size of the text, in pixels ("FSIZ") */
public Float:g_flInterval = 24.0; /* between two lines */
public Float:g_flSpeed = 200.0; /* the time of a fade and of the slowest scroll, in ms */
public g_iCurrent; /* the item selected first */
public g_iPadHelp = PADHELP_CIRCLE | PADHELP_CROSS; /* the SYS_PADHELP buttons while the menu has the focus */
new g_bActive; /* the menu has the focus and takes the pad */
new g_iScroll;
new g_iSelected;
new g_iCursor; /* the line of g_iSelected on the screen */
new g_iCount;
new g_tagMenu;
new g_iScrollTick; /* the tickcount of the last scroll: a scroll lasts the time since the one before */
new g_iPrevTick;
new g_iMaxTime;
new g_iMinTime;
new g_objTimer; /* the timer of the scroll */
new g_iRepeated;
new g_iPressed;

main()
{
    Init();
    for (; ; )
    {
        Update();
        sleep;
    }
}

/* Counts the items of the menu of argument "menu_id" and creates the g_iVisible lines around g_iCurrent. */
Init()
{
    new i;
    new text[64];
    new node;
    GetArg("menu_id", text, 64);
    g_tagMenu = domFindTag(domRoot(), "menu", text, 0);
    g_iCount = 0;
    node = domFirstChild(g_tagMenu);
    for (; node; node = domNext(node))
    {
        domGetTagName(node, text, 64);
        if (0 != strcmp("item", text))
        {
            continue;
        }
        g_iCount++;
    }
    g_iVisible = g_iVisible < 1 ? 1 : g_iVisible;
    g_iVisible = g_iCount < g_iVisible ? g_iCount : g_iVisible;
    g_iSelected = clamp(g_iCurrent, 0, g_iCount - 1);
    g_iCursor = g_iSelected > g_iCount - g_iVisible ? g_iSelected - (g_iCount - g_iVisible) : 0;
    new first = g_iSelected - g_iCursor;
    new Float:y;
    i = 0;
    for (; i < g_iVisible; i++)
    {
        new label;
        GetItemText(first + i, text, 64);
        label = CreateItemLabel(Self(), 0.0, y, text);
        AnimColor(label, 0.215, 0.708, 0.999, 1.0 * 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
        listAddLast(label);
        y = y + g_flInterval;
    }
    g_iMaxTime = floatround(g_flSpeed, 0);
    g_iMinTime = g_iMaxTime < 60 ? g_iMaxTime : 60;
    g_objTimer = CreateNull(Self());
}

/* Handles the messages and the scroll, then the pad: up and down move through the items, scrolling at the ends. */
Update()
{
    new i;
    new text[64];
    ProcessMessages();
    UpdateScroll();
    if (0 == g_bActive)
    {
        return;
    }
    PadState(_, g_iPressed, _, g_iRepeated, -1);
    if (g_iPressed & PAD_CIRCLE && 0 == is_animating(g_objFocus))
    {
        play_se(ED_SE_DECIDE);
        SendMessage(g_objManager, TAG("CUSR"), g_iSelected, 0, 0, 0);
        SendMessage(g_objManager, TAG("PUSH"), 0, TAG("RRGT"), 0, 0);
        g_bActive = 0;
    }
    else if (g_iPressed & PAD_CROSS)
    {
        SendMessage(g_objManager, TAG("PUSH"), 0, TAG("RDWN"), 0, 0);
        g_bActive = 0;
    }
    else if (g_iPressed & PAD_TRIANGLE)
    {
        SendMessage(g_objManager, TAG("CUSR"), g_iSelected, 0, 0, 0);
        SendMessage(g_objManager, TAG("PUSH"), 0, TAG("RUP"), 0, 0);
        g_bActive = 0;
    }
    else if (g_iPressed & PAD_SQUARE)
    {
        SendMessage(g_objManager, TAG("PUSH"), 0, TAG("RLFT"), 0, 0);
        g_bActive = 0;
    }
    else if (g_iPressed & PAD_RIGHT)
    {
        SendMessage(g_objManager, TAG("CUSR"), g_iSelected, 0, 0, 0);
        SendMessage(g_objManager, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
        g_bActive = 0;
    }
    else if (g_iPressed & PAD_LEFT)
    {
        SendMessage(g_objManager, TAG("CUSR"), g_iSelected, 0, 0, 0);
        SendMessage(g_objManager, TAG("FOCS"), TAG("LEFT"), 0, 0, 0);
        g_bActive = 0;
    }
    else if (g_iRepeated & PAD_UP)
    {
        if (g_iSelected > 0)
        {
            g_iSelected = g_iSelected > 0 ? --g_iSelected : g_iSelected; /* redundant as written */
            if (0 == g_iCursor)
            {
                FinishScroll();
                g_iPrevTick = g_iScrollTick;
                g_iScrollTick = tickcount();
                new scroll_time = g_iScrollTick - g_iPrevTick;
                scroll_time = clamp(scroll_time, g_iMinTime, g_iMaxTime);
                GetItemText(g_iSelected, text, 64);
                new Float:y;
                GetPos(listGetAt(0), _, y, _, 0);
                y = y + -1.0 * g_flInterval;
                new label = CreateItemLabel(Self(), 0.0, y, text);
                listInsert(0, label);
                y = 0.0;
                i = 0;
                for (; i < listSize(); i++)
                {
                    AnimColor(listGetAt(i), 0.215, 0.708, 0.999, 1.0, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
                    AnimMoveLine(listGetAt(i), 0.0, y, 0.0, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
                    y = y + g_flInterval;
                }
                AnimColor(listGetLast(), 0.215, 0.708, 0.999, 1.0 * 0.0, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
                g_iScroll = SCROLL_UP;
                AnimTimerManager(g_objTimer, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
                send_label_focus(g_objFocus, listGetAt(0), 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            }
            else
            {
                g_iCursor = g_iCursor > 0 ? --g_iCursor : g_iCursor; /* redundant as written */
                send_label_focus(g_objFocus, listGetAt(g_iCursor), 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            }
            play_se(ED_SE_CURSOR);
        }
        else if (SCROLL_NONE == g_iScroll)
        {
            SendMessage(g_objManager, TAG("FOCS"), TAG("UP"), 0, 0, 0);
            g_bActive = 0;
        }
    }
    else if (g_iRepeated & PAD_DOWN)
    {
        if (g_iSelected < g_iCount - 1)
        {
            g_iSelected = g_iSelected < g_iCount - 1 ? ++g_iSelected : g_iSelected; /* redundant as written */
            if (g_iCursor == g_iVisible - 1)
            {
                FinishScroll();
                g_iPrevTick = g_iScrollTick;
                g_iScrollTick = tickcount();
                new scroll_time = g_iScrollTick - g_iPrevTick;
                scroll_time = clamp(scroll_time, g_iMinTime, g_iMaxTime);
                GetItemText(g_iSelected, text, 64);
                new Float:y;
                GetPos(listGetLast(), _, y, _, 0);
                y = y + g_flInterval;
                new label = CreateItemLabel(Self(), 0.0, y, text);
                listAddLast(label);
                y = -1.0 * g_flInterval;
                i = 0;
                for (; i < listSize(); i++)
                {
                    AnimColor(listGetAt(i), 0.215, 0.708, 0.999, 1.0, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
                    AnimMoveLine(listGetAt(i), 0.0, y, 0.0, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
                    y = y + g_flInterval;
                }
                AnimColor(listGetAt(0), 0.215, 0.708, 0.999, 1.0 * 0.0, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
                g_iScroll = SCROLL_DOWN;
                AnimTimerManager(g_objTimer, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
                send_label_focus(g_objFocus, listGetLast(), 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            }
            else
            {
                g_iCursor = g_iCursor < g_iVisible - 1 ? ++g_iCursor : g_iCursor; /* redundant as written */
                send_label_focus(g_objFocus, listGetAt(g_iCursor), 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            }
            play_se(ED_SE_CURSOR);
        }
        else if (SCROLL_NONE == g_iScroll)
        {
            SendMessage(g_objManager, TAG("FOCS"), TAG("DOWN"), 0, 0, 0);
            g_bActive = 0;
        }
    }
}

ProcessMessages()
{
    new msg;
    new a;
    new Float:b;
    /* "FOCS": a the direction; "FADE": a the alpha, b the duration; "PUSH" and "CHNG" give the menu the pad back. */
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
    }
}

/* "FOUT" takes the focus away; any other dir gives it to the current line. */
HandleFocus(dir)
{
    if (dir == TAG("FOUT"))
    {
        g_bActive = 0;
        return;
    }
    if (dir == TAG("FRST"))
    {
    }
    else if (dir == TAG("RTRN"))
    {
    }
    if (g_iCount > 0)
    {
        send_label_focus(g_objFocus, listGetAt(g_iCursor), 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    }
    ShowPadHelp();
    g_bActive = 1;
}

/* Shows the buttons of g_iPadHelp in the pad help. */
ShowPadHelp()
{
    SetSystemParam(SYS_PADHELP, g_iPadHelp);
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

/* Removes the line scrolled out at once, before a new scroll. */
FinishScroll()
{
    if (SCROLL_NONE == g_iScroll)
    {
        return;
    }
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

/* Creates the text of a line at x, y, aligned as g_iAlignH, hidden. */
CreateItemLabel(parent, Float:x, Float:y, text[])
{
    new label;
    label = CreateLabel(parent);
    SetPos(label, x, y, 0.0, 0);
    SetColor(label, 0.215, 0.708, 0.999, 0.0);
    SetOptionI(label, TAG("ALPH"), BLEND_ADD);
    SetOptionI(label, TAG("HALN"), g_iAlignH);
    SetOptionI(label, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(label, TAG("FSIZ"), g_iFontSize);
    SetOptionS(label, TAG("UTF8"), text);
    return label;
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
        if (0 != strcmp("item", tag_name))
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
