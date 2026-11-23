/*
 * /opt0/bn/script/edclient/menu_item3.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A vertical menu of an edclient page whose items are rows of several texts: the "item" children of the menu
 * element of argument "menu_id", each with "text" children placed by "x", "y" and "align_h". Circle marks the
 * row and tells the manager.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

#include "edpage.inc"

#include "tag.inc"

/* Values of g_iScroll: the scroll that runs. */
enum
{
    SCROLL_NONE,
    SCROLL_DOWN, /* a row added at the bottom: the top one goes when it ends */
    SCROLL_UP,
};

#include "text.inc"

/* Same as send_focus of dom.inc. */
send_focus2(obj, target, z_offset, Float:sharp, Float:extra_w, Float:extra_h, Float:move_time, smoothing, blinking)
{
    if (!obj)
    {
        return;
    }
    SendMessage(obj, TAG("FOC0"), target, TAG(""), z_offset, sharp);
    SendMessage(obj, TAG("FOC1"), extra_w, extra_h, move_time, smoothing);
    SendMessage(obj, TAG("FOC2"), blinking, 0, 0, 0);
}

new g_any_child3[1]; /* empty: any element */
new g_any_next3[1]; /* empty: any element */

#include "dom.inc"

#include "background.inc"

public g_objManager; /* the page: gets "PUSH" with the item and the button, "FOCS" when the focus leaves */
public g_objFocus; /* the focus object, moved onto the current row */
public g_iVisible = 1; /* rows shown, at most 10 */
public g_iFontSize = 15; /* the font size of the text, in pixels ("FSIZ") */
public Float:g_flInterval = 24.0; /* between two rows */
public Float:g_flSpeed = 200.0; /* the time of a fade and of the slowest scroll, in ms */
public g_iCurrent; /* the item selected first */
public g_iPadHelp = PADHELP_CIRCLE | PADHELP_CROSS; /* the SYS_PADHELP buttons while the menu has the focus */
new g_bActive; /* the menu has the focus and takes the pad */
new g_iScroll;
new g_iSelected;
new g_iCursor; /* the row of g_iSelected on the screen */
new g_iCount;
new Float:g_flRowWidth; /* "w" and "h" of the menu element */
new Float:g_flRowHeight;
new g_tagMenu;
new g_iScrollTick; /* the tickcount of the last scroll: a scroll lasts the time since the one before */
new g_iPrevTick;
new g_iMaxTime;
new g_iMinTime;
new g_objTimer; /* the timer of the scroll */
new g_iRepeated;
new g_iPressed;
new g_objLabels[22]; /* the texts of the rows shown, 0 in a free slot */
new g_tagTopItem; /* the "item" elements of the first and the last row shown */
new g_tagBottomItem;
new g_iPicked = -1; /* the item marked with circle, -1 for none; right is ignored until one is */
new g_objMark; /* the plane that marks g_iPicked */

main()
{
    Init();
    for (; ; )
    {
        Update();
        sleep;
    }
}

/*
 * Counts the items of the menu of argument "menu_id", reads the size "w", "h" of a row and creates the g_iVisible rows
 * around g_iCurrent.
 */
Init()
{
    new i;
    new text[64];
    new node;
    new found;
    GetArg("menu_id", text, 64);
    g_tagMenu = domFindTag(domRoot(), "menu", text, 0);
    g_iCount = 0;
    node = domFirstChild(g_tagMenu, g_any_child3);
    for (; node; node = domNext(node, g_any_next3))
    {
        domGetTagName(node, text, 64);
        if (0 != strcmp("item", text))
        {
            continue;
        }
        g_iCount++;
    }
    g_iVisible = clamp(g_iVisible, 1, 10);
    g_iVisible = g_iCount < g_iVisible ? g_iCount : g_iVisible;
    g_iSelected = clamp(g_iCurrent, 0, g_iCount - 1);
    g_iCursor = g_iSelected > g_iCount - g_iVisible ? g_iSelected - (g_iCount - g_iVisible) : 0;
    new first = g_iSelected - g_iCursor;
    g_flRowWidth = float(g_iFontSize);
    g_flRowHeight = float(g_iFontSize);
    found = domGetData(g_tagMenu, "w", text, 64);
    if (found == 1)
    {
        g_flRowWidth = atof(text);
    }
    found = domGetData(g_tagMenu, "h", text, 64);
    if (found == 1)
    {
        g_flRowHeight = atof(text);
    }
    new Float:y;
    i = 0;
    for (; i < g_iVisible; i++)
    {
        new row;
        row = CreateNull(Self());
        SetPos(row, 0.0, y, 0.0, 0);
        SetLocalScale(row, g_flRowWidth, g_flRowHeight, 0.0);
        listAddLast(row);
        y = y + g_flInterval;
        node = find_nth_child_tag2(g_tagMenu, "item", first + i);
        new text_node;
        text_node = domFirstChild(node, g_any_child3);
        for (; text_node; text_node = domNext(text_node, g_any_next3))
        {
            domGetTagName(text_node, text, 64);
            if (0 != strcmp("text", text))
            {
                continue;
            }
            new label = AddRowText(text_node, row);
            AnimColor(label, 0.215, 0.708, 0.999, 1.0 * 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
        }
    }
    g_tagTopItem = find_nth_child_tag2(g_tagMenu, "item", first);
    g_tagBottomItem = find_nth_child_tag2(g_tagMenu, "item", first + g_iVisible - 1);
    g_iMaxTime = floatround(g_flSpeed, 0);
    g_iMinTime = g_iMaxTime < 60 ? g_iMaxTime : 60;
    g_objTimer = CreateNull(Self());
}

/*
 * Handles the messages and the scroll, then the pad: circle marks the row and tells the manager; right leaves the menu
 * only once a row is marked.
 */
Update()
{
    ProcessMessages();
    UpdateScroll();
    if (0 == g_bActive)
    {
        return;
    }
    new i;
    new tag_name[64];
    PadState(_, g_iPressed, _, g_iRepeated, -1);
    if (g_iPressed & PAD_CIRCLE && 0 == is_animating(g_objFocus))
    {
        g_iPicked = g_iSelected;
        if (IsValidObject(g_objMark) == 1)
        {
            Destroy(g_objMark);
        }
        g_objMark = CreateMark(listGetAt(g_iCursor));
        AnimColor(g_objMark, 0.2, 0.8, 0.8, 0.4 * 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
        play_se(ED_SE_DECIDE);
        SendMessage(g_objManager, TAG("PUSH"), g_iSelected, TAG("RRGT"), 0, 0);
        g_bActive = 0;
    }
    else if (g_iPressed & PAD_CROSS)
    {
        SendMessage(g_objManager, TAG("PUSH"), g_iSelected, TAG("RDWN"), 0, 0);
        g_bActive = 0;
    }
    else if (g_iPressed & PAD_TRIANGLE)
    {
        SendMessage(g_objManager, TAG("PUSH"), g_iSelected, TAG("RUP"), 0, 0);
        g_bActive = 0;
    }
    else if (g_iPressed & PAD_SQUARE)
    {
        SendMessage(g_objManager, TAG("PUSH"), g_iSelected, TAG("RLFT"), 0, 0);
        g_bActive = 0;
    }
    else if (g_iPressed & PAD_RIGHT)
    {
        if (g_iPicked >= 0)
        {
            SendMessage(g_objManager, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
            g_bActive = 0;
        }
    }
    else if (g_iPressed & PAD_LEFT)
    {
        SendMessage(g_objManager, TAG("FOCS"), TAG("LEFT"), 0, 0, 0);
        g_bActive = 0;
    }
    else if (g_iRepeated & PAD_UP)
    {
        if (g_iSelected > 0)
        {
            play_se(ED_SE_CURSOR);
            g_iSelected = g_iSelected > 0 ? --g_iSelected : g_iSelected; /* redundant as written */
            if (0 == g_iCursor)
            {
                FinishScroll();
                new scroll_time = GetScrollTime();
                new Float:y;
                GetPos(listGetAt(0), _, y, _, 0);
                y = y + -1.0 * g_flInterval;
                new row = CreateNull(Self());
                SetPos(row, 0.0, y, 0.0, 0);
                SetLocalScale(row, g_flRowWidth, g_flRowHeight, 0.0);
                listInsert(0, row);
                y = 0.0;
                i = 0;
                for (; i < listSize(); i++)
                {
                    AnimMoveLine(listGetAt(i), 0.0, y, 0.0, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
                    y = y + g_flInterval;
                }
                FadeLabels(1.0, float(scroll_time), 0);
                g_tagTopItem = domPrev(g_tagTopItem);
                g_tagBottomItem = domPrev(g_tagBottomItem);
                new node;
                new label;
                node = domFirstChild(g_tagTopItem, g_any_child3);
                for (; node; node = domNext(node, g_any_next3))
                {
                    domGetTagName(node, tag_name, 64);
                    if (0 != strcmp("text", tag_name))
                    {
                        continue;
                    }
                    label = AddRowText(node, row);
                    AnimColor(label, 0.215, 0.708, 0.999, 1.0 * 1.0, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
                }
                FadeLabels(0.0, float(scroll_time), listGetLast());
                UpdateMark(float(scroll_time), listGetAt(0));
                g_iScroll = SCROLL_UP;
                AnimTimerManager(g_objTimer, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
                send_focus2(g_objFocus, listGetAt(0), 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            }
            else
            {
                g_iCursor = g_iCursor > 0 ? --g_iCursor : g_iCursor; /* redundant as written */
                send_focus2(g_objFocus, listGetAt(g_iCursor), 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            }
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
            play_se(ED_SE_CURSOR);
            g_iSelected = g_iSelected < g_iCount - 1 ? ++g_iSelected : g_iSelected; /* redundant as written */
            if (g_iCursor == g_iVisible - 1)
            {
                FinishScroll();
                new scroll_time = GetScrollTime();
                new Float:y;
                GetPos(listGetLast(), _, y, _, 0);
                y = y + g_flInterval;
                new row = CreateNull(Self());
                SetPos(row, 0.0, y, 0.0, 0);
                SetLocalScale(row, g_flRowWidth, g_flRowHeight, 0.0);
                listAddLast(row);
                y = -1.0 * g_flInterval;
                i = 0;
                for (; i < listSize(); i++)
                {
                    AnimMoveLine(listGetAt(i), 0.0, y, 0.0, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
                    y = y + g_flInterval;
                }
                FadeLabels(1.0, float(scroll_time), 0);
                g_tagTopItem = domNext(g_tagTopItem, g_any_next3);
                g_tagBottomItem = domNext(g_tagBottomItem, g_any_next3);
                new node;
                new label;
                node = domFirstChild(g_tagBottomItem, g_any_child3);
                for (; node; node = domNext(node, g_any_next3))
                {
                    domGetTagName(node, tag_name, 64);
                    if (0 != strcmp("text", tag_name))
                    {
                        continue;
                    }
                    label = AddRowText(node, row);
                    AnimColor(label, 0.215, 0.708, 0.999, 1.0 * 1.0, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
                }
                FadeLabels(0.0, float(scroll_time), listGetAt(0));
                UpdateMark(float(scroll_time), listGetLast());
                g_iScroll = SCROLL_DOWN;
                AnimTimerManager(g_objTimer, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
                send_focus2(g_objFocus, listGetLast(), 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            }
            else
            {
                g_iCursor = g_iCursor < g_iVisible - 1 ? ++g_iCursor : g_iCursor; /* redundant as written */
                send_focus2(g_objFocus, listGetAt(g_iCursor), 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            }
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
            FadeLabels(alpha, duration, 0);
            if (IsValidObject(g_objMark) == 1)
            {
                AnimColor(g_objMark, 0.2, 0.8, 0.8, 0.4 * alpha, duration, 0.0, ANIM_FUNC_LINEAR);
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

/* "FOUT" takes the focus away; any other dir gives it to the current row. */
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
        send_focus2(g_objFocus, listGetAt(g_iCursor), 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    }
    ShowPadHelp();
    g_bActive = 1;
}

/* Shows the buttons of g_iPadHelp in the pad help. */
ShowPadHelp()
{
    SetSystemParam(SYS_PADHELP, g_iPadHelp);
}

/* Removes the row scrolled out when the scroll ends. */
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
            PurgeLabels();
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

/* Removes the row scrolled out at once, before a new scroll. */
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
        PurgeLabels();
        g_iScroll = SCROLL_NONE;
    }
}

/* Returns the time of a scroll: the time since the last one, from g_iMinTime to g_iMaxTime. */
GetScrollTime()
{
    new scroll_time;
    g_iPrevTick = g_iScrollTick;
    g_iScrollTick = tickcount();
    scroll_time = g_iScrollTick - g_iPrevTick;
    scroll_time = clamp(scroll_time, g_iMinTime, g_iMaxTime);
    return scroll_time;
}

/* Creates the label of the "text" element node in row and adds it to g_objLabels. */
AddRowText(node, row)
{
    new label;
    new Float:x;
    new Float:y;
    new align_h;
    new value[64];
    new found;
    x = 0.0;
    y = 0.0;
    align_h = ALIGN_CENTER;
    found = domGetData(node, "x", value, 64);
    if (found == 1)
    {
        x = atof(value);
    }
    found = domGetData(node, "y", value, 64);
    if (found == 1)
    {
        y = atof(value);
    }
    found = domGetData(node, "align_h", value, 64);
    if (found == 1)
    {
        if (0 == strcmp("center", value))
        {
            align_h = ALIGN_CENTER;
        }
        else if (0 == strcmp("left", value))
        {
            align_h = ALIGN_LEFT;
        }
        else if (0 == strcmp("right", value))
        {
            align_h = ALIGN_RIGHT;
        }
    }
    domGetData(node, "value", value, 64);
    label = CreateTextLabel(row, x, y, value, align_h);
    found = AddLabel(label);
    return label;
}

/* Creates a text of a row at x, y, aligned as align_h, hidden. */
CreateTextLabel(parent, Float:x, Float:y, text[], align_h)
{
    new label;
    label = CreateLabel(parent);
    SetPos(label, x, y, 0.0, 0);
    SetColor(label, 0.215, 0.708, 0.999, 0.0);
    SetOptionI(label, TAG("ALPH"), BLEND_ADD);
    SetOptionI(label, TAG("HALN"), align_h);
    SetOptionI(label, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(label, TAG("FSIZ"), g_iFontSize);
    SetOptionS(label, TAG("UTF8"), text);
    return label;
}

/* Puts label in a free slot of g_objLabels; returns 0 if there is none. */
AddLabel(label)
{
    for (new i; i < 22; i++)
    {
        if (0 == g_objLabels[i])
        {
            g_objLabels[i] = label;
            return 1;
        }
    }
    return 0;
}

/* Frees the slots of the labels destroyed with their row. */
PurgeLabels()
{
    for (new i; i < 22; i++)
    {
        if (0 == g_objLabels[i])
        {
            continue;
        }
        if (0 == IsValidObject(g_objLabels[i]))
        {
            g_objLabels[i] = 0;
        }
    }
}

/* Fades the labels of row, or all of them with row 0. */
FadeLabels(Float:alpha, Float:duration, row)
{
    new i;
    i = 0;
    for (; i < 22; i++)
    {
        if (0 == g_objLabels[i])
        {
            continue;
        }
        if (0 != row && row != Parent(g_objLabels[i]))
        {
            continue;
        }
        AnimColor(g_objLabels[i], 0.215, 0.708, 0.999, 1.0 * alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    }
}

/* Creates the plane that marks row, a little larger than the row. */
CreateMark(row)
{
    new Float:mark_w;
    new Float:mark_h;
    new mark;
    mark_w = 4.0 + g_flRowWidth;
    mark_h = 4.0 + g_flRowHeight;
    mark = CreatePlane(row);
    SetPos(mark, 0.0, 0.0, 0.0, 0);
    SetScale(mark, mark_w, mark_h, 0.0);
    SetColor(mark, 0.2, 0.8, 0.8, 0.4 * 0.0);
    return mark;
}

/* After a scroll: the mark follows g_iPicked into row, or fades when the row of g_iPicked scrolls out. */
UpdateMark(Float:duration, row)
{
    if (g_iPicked == -1)
    {
    }
    else if (g_iPicked == g_iSelected)
    {
        if (IsValidObject(g_objMark) == 1)
        {
            Destroy(g_objMark);
        }
        g_objMark = CreateMark(row);
        AnimColor(g_objMark, 0.2, 0.8, 0.8, 0.4 * 1.0, duration, 0.0, ANIM_FUNC_LINEAR);
    }
    else if (g_iPicked == g_iSelected + g_iVisible)
    {
        if (IsValidObject(g_objMark) == 1)
        {
            AnimColor(g_objMark, 0.2, 0.8, 0.8, 0.4 * 0.0, duration, 0.0, ANIM_FUNC_LINEAR);
        }
    }
    else if (g_iPicked == g_iSelected - g_iVisible)
    {
        if (IsValidObject(g_objMark) == 1)
        {
            AnimColor(g_objMark, 0.2, 0.8, 0.8, 0.4 * 0.0, duration, 0.0, ANIM_FUNC_LINEAR);
        }
    }
}
