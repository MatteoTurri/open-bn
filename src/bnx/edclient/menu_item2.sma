/*
 * /opt0/bn/script/edclient/menu_item2.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A vertical menu of an edclient page: the "item" children of the menu element of argument "menu_id", each a text
 * (dimmed with enable="false") and the images of its "plane" children. Circle marks the item and tells the
 * manager; triangle only on an item with pad_ru="enable".
 */

#include <core>
#include <bn>

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

#include <float>

#include "text.inc"

#include "sysconf.inc"

#include "misc.inc"

/*
 * Creates a nine-slice plane at rect (x, y, z, width, height; with no size, the texture's), whose edges keep the
 * size in pixels given by border (left, right, top, bottom).
 */
CreateFramePlane(parent, rect[], zsort, color, texture, Float:tex_w, Float:tex_h, Float:border[])
{
    new plane;
    new Float:x;
    new Float:y;
    new Float:z;
    new Float:plane_w;
    new Float:plane_h;
    new Float:margins[4];
    new Float:red;
    new Float:green;
    new Float:blue;
    new Float:alpha;
    unpack_color(color, red, green, blue, alpha);
    plane = CreatePlane3x3(parent);
    x = rect[0];
    y = rect[1];
    z = rect[2];
    plane_w = rect[3];
    plane_h = rect[4];
    if (0.0 == plane_w && 0.0 == plane_h)
    {
        plane_w = tex_w;
        plane_h = tex_h;
    }
    SetPos(plane, x, y, z, 0);
    SetOptionI(plane, TAG("ZSRT"), zsort);
    SetColor(plane, red, green, blue, 0.0);
    if (0 != texture)
    {
        SetTexture(plane, texture, 0, -1);
    }
    SetLocalScale(plane, plane_w, plane_h, 0.0);
    margins[0] = border[0] / plane_w;
    margins[1] = border[1] / plane_w;
    margins[2] = border[2] / plane_h;
    margins[3] = border[3] / plane_h;
    SetOptionP(plane, TAG("MGN"), margins, 4);
    margins[0] = border[0] / tex_w;
    margins[1] = border[1] / tex_w;
    margins[2] = border[2] / tex_h;
    margins[3] = border[3] / tex_h;
    SetOptionP(plane, TAG("MGNT"), margins, 4);
    return plane;
}

new g_any_child3[1]; /* empty: any element */
new g_any_next3[1]; /* empty: any element */

#include "dom.inc"

#include "background.inc"

public g_objManager; /* the page: gets "PUSH" with the item and the button, "FOCS" when the focus leaves */
public g_objFocus; /* the focus object, moved onto the text of the current row */
public g_iVisible = 1; /* rows shown, at most 10 */
public g_iAlignH; /* the "HALN" of the texts */
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
new g_tagMenu;
new g_iScrollTick; /* the tickcount of the last scroll: a scroll lasts the time since the one before */
new g_iPrevTick;
new g_iMaxTime;
new g_iMinTime;
new g_objTimer; /* the timer of the scroll */
new g_iRepeated;
new g_iPressed;
new g_objLabels[11]; /* the texts of the rows shown, 0 in a free slot */
new g_iLabelColors[11]; /* the packed color of each text */
new g_objPlanes[33]; /* the images of the rows shown, 0 in a free slot */
new g_tagTopItem; /* the "item" elements of the first and the last row shown */
new g_tagBottomItem;
new g_iPicked = -1; /* the item marked with circle, -1 for none */
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

/* Counts the items of the menu of argument "menu_id" and creates the g_iVisible rows around g_iCurrent. */
Init()
{
    new i;
    new text[64];
    new node;
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
    new Float:y;
    i = 0;
    for (; i < g_iVisible; i++)
    {
        new row;
        row = CreateNull(Self());
        SetPos(row, 0.0, y, 0.0, 0);
        listAddLast(row);
        y = y + g_flInterval;
        node = find_nth_child_tag2(g_tagMenu, "item", first + i);
        AddRowLabel(row, node, g_flSpeed);
        AddRowPlanes(row, node, g_flSpeed);
    }
    g_tagTopItem = find_nth_child_tag2(g_tagMenu, "item", first);
    g_tagBottomItem = find_nth_child_tag2(g_tagMenu, "item", first + g_iVisible - 1);
    g_iMaxTime = floatround(g_flSpeed, 0);
    g_iMinTime = g_iMaxTime < 60 ? g_iMaxTime : 60;
    g_objTimer = CreateNull(Self());
}

/*
 * Handles the messages and the scroll, then the pad: circle marks an enabled item and tells the manager, up and down
 * move through the items, scrolling at the ends.
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
    new value[64];
    new item;
    new enabled;
    new found;
    PadState(_, g_iPressed, _, g_iRepeated, -1);
    if (g_iPressed & PAD_CIRCLE && 0 == is_animating(g_objFocus))
    {
        item = find_nth_child_tag2(g_tagMenu, "item", g_iSelected);
        enabled = 1;
        found = domGetData(item, "enable", value, 64);
        if (found == 1)
        {
            if (0 == strcmp("false", value))
            {
                enabled = 0;
            }
        }
        if (enabled == 1)
        {
            g_iPicked = g_iSelected;
            if (IsValidObject(g_objMark) == 1)
            {
                Destroy(g_objMark);
            }
            g_objMark = CreateMark(listGetAt(g_iCursor));
            AnimColor(g_objMark, 0.2, 0.8, 0.8, 0.4 * 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
            play_se(ED_SE_DECIDE);
            SendMessage(g_objManager, TAG("PUSH"), g_iSelected, TAG("RRGT"), 0, 0);
            g_bActive = 0;
        }
    }
    else if (g_iPressed & PAD_CROSS)
    {
        SendMessage(g_objManager, TAG("PUSH"), g_iSelected, TAG("RDWN"), 0, 0);
        g_bActive = 0;
    }
    else if (g_iPressed & PAD_TRIANGLE)
    {
        item = find_nth_child_tag2(g_tagMenu, "item", g_iSelected);
        enabled = 0;
        found = domGetData(item, "pad_ru", value, 64);
        if (found == 1)
        {
            if (0 == strcmp("enable", value))
            {
                enabled = 1;
            }
            else if (0 == strcmp("disable", value))
            {
                enabled = 0;
            }
        }
        if (enabled == 1)
        {
            SendMessage(g_objManager, TAG("PUSH"), g_iSelected, TAG("RUP"), 0, 0);
            g_bActive = 0;
        }
    }
    else if (g_iPressed & PAD_SQUARE)
    {
        SendMessage(g_objManager, TAG("PUSH"), g_iSelected, TAG("RLFT"), 0, 0);
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
    else if (g_iRepeated & PAD_UP)
    {
        if (g_iSelected > 0)
        {
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
                listInsert(0, row);
                y = 0.0;
                i = 0;
                for (; i < listSize(); i++)
                {
                    AnimMoveLine(listGetAt(i), 0.0, y, 0.0, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
                    y = y + g_flInterval;
                }
                FadeLabels(1.0, float(scroll_time), 0);
                FadePlanes(1.0, float(scroll_time), 0);
                g_tagTopItem = domPrev(g_tagTopItem);
                g_tagBottomItem = domPrev(g_tagBottomItem);
                item = g_tagTopItem;
                AddRowLabel(row, item, float(scroll_time));
                AddRowPlanes(row, item, float(scroll_time));
                FadeLabels(0.0, float(scroll_time), listGetLast());
                FadePlanes(0.0, float(scroll_time), listGetLast());
                UpdateMark(float(scroll_time), listGetAt(0));
                g_iScroll = SCROLL_UP;
                AnimTimerManager(g_objTimer, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
                FocusRow(listGetAt(0));
            }
            else
            {
                g_iCursor = g_iCursor > 0 ? --g_iCursor : g_iCursor; /* redundant as written */
                FocusRow(listGetAt(g_iCursor));
            }
            item = find_nth_child_tag2(g_tagMenu, "item", g_iSelected);
            ShowPadHelp(item);
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
                new scroll_time = GetScrollTime();
                new Float:y;
                GetPos(listGetLast(), _, y, _, 0);
                y = y + g_flInterval;
                new row = CreateNull(Self());
                SetPos(row, 0.0, y, 0.0, 0);
                listAddLast(row);
                y = -1.0 * g_flInterval;
                i = 0;
                for (; i < listSize(); i++)
                {
                    AnimMoveLine(listGetAt(i), 0.0, y, 0.0, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
                    y = y + g_flInterval;
                }
                FadeLabels(1.0, float(scroll_time), 0);
                FadePlanes(1.0, float(scroll_time), 0);
                g_tagTopItem = domNext(g_tagTopItem, g_any_next3);
                g_tagBottomItem = domNext(g_tagBottomItem, g_any_next3);
                item = g_tagBottomItem;
                AddRowLabel(row, item, float(scroll_time));
                AddRowPlanes(row, item, float(scroll_time));
                FadeLabels(0.0, float(scroll_time), listGetAt(0));
                FadePlanes(0.0, float(scroll_time), listGetAt(0));
                UpdateMark(float(scroll_time), listGetLast());
                g_iScroll = SCROLL_DOWN;
                AnimTimerManager(g_objTimer, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
                FocusRow(listGetLast());
            }
            else
            {
                g_iCursor = g_iCursor < g_iVisible - 1 ? ++g_iCursor : g_iCursor; /* redundant as written */
                FocusRow(listGetAt(g_iCursor));
            }
            item = find_nth_child_tag2(g_tagMenu, "item", g_iSelected);
            ShowPadHelp(item);
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
            FadeLabels(alpha, duration, 0);
            FadePlanes(alpha, duration, 0);
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
        FocusRow(listGetAt(g_iCursor));
    }
    new item = find_nth_child_tag2(g_tagMenu, "item", g_iSelected);
    ShowPadHelp(item);
    g_bActive = 1;
}

/* Shows g_iPadHelp, with or without triangle as the pad_ru of item says. */
ShowPadHelp(item)
{
    new buttons;
    new value[64];
    new found;
    buttons = g_iPadHelp;
    found = domGetData(item, "pad_ru", value, 64);
    if (found == 1)
    {
        if (0 == strcmp("enable", value))
        {
            buttons = buttons | PADHELP_TRIANGLE;
        }
        else if (0 == strcmp("disable", value))
        {
            buttons = buttons & ~PADHELP_TRIANGLE;
        }
    }
    SetSystemParam(SYS_PADHELP, buttons);
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
            PurgePlanes();
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
        PurgePlanes();
        g_iScroll = SCROLL_NONE;
    }
}

/* Creates the text of item in row, fading in over duration, and adds it to g_objLabels. */
AddRowLabel(row, item, Float:duration)
{
    new Float:red;
    new Float:green;
    new Float:blue;
    new Float:alpha;
    new text[64];
    new label;
    new found;
    red = 0.215;
    green = 0.708;
    blue = 0.999;
    alpha = 1.0;
    found = domGetData(item, "enable", text, 64);
    if (found == 1)
    {
        if (0 == strcmp("false", text))
        {
            red = 0.03;
            green = 0.18;
            blue = 0.36;
            alpha = 1.0;
        }
    }
    domGetData(item, "value", text, 64);
    label = CreateItemLabel(row, 0.0, 0.0, text);
    SetColor(label, red, green, blue, 0.0 * alpha);
    AnimColor(label, red, green, blue, 1.0 * alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    new red_byte;
    new green_byte;
    new blue_byte;
    new alpha_byte;
    new color;
    red_byte = floatround(255.0 * red, 0);
    green_byte = floatround(255.0 * green, 0);
    blue_byte = floatround(255.0 * blue, 0);
    alpha_byte = floatround(255.0 * alpha, 0);
    color = pack_color(red_byte, green_byte, blue_byte, alpha_byte);
    found = AddLabel(label, color);
}

/* Creates the text of an item at x, y, aligned as g_iAlignH. */
CreateItemLabel(parent, Float:x, Float:y, text[])
{
    new label;
    label = CreateLabel(parent);
    SetPos(label, x, y, 0.0, 0);
    SetOptionI(label, TAG("ALPH"), BLEND_ADD);
    SetOptionI(label, TAG("HALN"), g_iAlignH);
    SetOptionI(label, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(label, TAG("FSIZ"), g_iFontSize);
    SetOptionS(label, TAG("UTF8"), text);
    return label;
}

/* Puts label and its color in a free slot of g_objLabels; returns 0 if there is none. */
AddLabel(label, color)
{
    for (new i; i < 11; i++)
    {
        if (0 == g_objLabels[i])
        {
            g_objLabels[i] = label;
            g_iLabelColors[i] = color;
            return 1;
        }
    }
    return 0;
}

/* Frees the slots of the texts destroyed with their row. */
PurgeLabels()
{
    for (new i; i < 11; i++)
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

/* Fades the texts of row, or all of them with row 0, to alpha times their own. */
FadeLabels(Float:alpha, Float:duration, row)
{
    new i;
    i = 0;
    for (; i < 11; i++)
    {
        if (0 == g_objLabels[i])
        {
            continue;
        }
        if (0 != row && row != Parent(g_objLabels[i]))
        {
            continue;
        }
        new Float:red;
        new Float:green;
        new Float:blue;
        new Float:label_alpha;
        unpack_color(g_iLabelColors[i], red, green, blue, label_alpha);
        AnimColor(g_objLabels[i], red, green, blue, label_alpha * alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    }
}

/* Creates in row the images of the "plane" children of item (an IMG_RESOURCE and a position each). */
AddRowPlanes(row, item, Float:duration)
{
    new buf[64];
    new node;
    new res;
    new found;
    node = domFirstChild(item, g_any_child3);
    for (; node; node = domNext(node, g_any_next3))
    {
        domGetTagName(node, buf, 64);
        if (0 != strcmp("plane", buf))
        {
            continue;
        }
        new Float:x;
        new Float:y;
        new plane_w;
        new plane_h;
        new texture;
        new Float:tex_w;
        new Float:tex_h;
        new Float:rect[5];
        new Float:border[4];
        new zsort;
        new color;
        found = domGetData(node, "img_id", buf, 64);
        if (found == 1)
        {
            res = domFindTag(domRoot(), "IMG_RESOURCE", buf, 0);
            domGetData(res, "texture", buf, 64);
            texture = atoi(buf);
            domGetData(res, "w", buf, 64);
            tex_w = atof(buf);
            domGetData(res, "h", buf, 64);
            tex_h = atof(buf);
        }
        found = domGetData(node, "x", buf, 64);
        if (found == 1)
        {
            x = atof(buf);
        }
        found = domGetData(node, "y", buf, 64);
        if (found == 1)
        {
            y = atof(buf);
        }
        rect[0] = x;
        rect[1] = y;
        rect[2] = 0.0;
        rect[3] = plane_w;
        rect[4] = plane_h;
        zsort = ZSORT_NONE;
        color = pack_color(255, 255, 255, 255);
        border[0] = 0.5 * tex_w;
        border[1] = 0.5 * tex_w;
        border[2] = 0.5 * tex_h;
        border[3] = 0.5 * tex_h;
        new plane = CreateFramePlane(row, rect, zsort, color, texture, tex_w, tex_h, border);
        AnimColor(plane, 1.0, 1.0, 1.0, 1.0, duration, 0.0, ANIM_FUNC_LINEAR);
        AddPlane(plane);
    }
}

/* Puts plane in a free slot of g_objPlanes; returns 0 if there is none. */
AddPlane(plane)
{
    for (new i; i < 33; i++)
    {
        if (0 == g_objPlanes[i])
        {
            g_objPlanes[i] = plane;
            return 1;
        }
    }
    return 0;
}

/* Frees the slots of the images destroyed with their row. */
PurgePlanes()
{
    for (new i; i < 33; i++)
    {
        if (0 == g_objPlanes[i])
        {
            continue;
        }
        if (0 == IsValidObject(g_objPlanes[i]))
        {
            g_objPlanes[i] = 0;
        }
    }
}

/* Fades the images of row, or all of them with row 0. */
FadePlanes(Float:alpha, Float:duration, row)
{
    new i;
    i = 0;
    for (; i < 33; i++)
    {
        if (0 == g_objPlanes[i])
        {
            continue;
        }
        if (0 != row && row != Parent(g_objPlanes[i]))
        {
            continue;
        }
        AnimColor(g_objPlanes[i], 1.0, 1.0, 1.0, 1.0 * alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    }
}

/* Moves the focus onto the text of row. */
FocusRow(row)
{
    new label;
    label = GetRowLabel(row);
    send_label_focus(g_objFocus, label, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
}

/* Returns the text of row, 0 if it has none. */
GetRowLabel(row)
{
    for (new i; i < 11; i++)
    {
        if (0 == g_objLabels[i])
        {
            continue;
        }
        if (row == Parent(g_objLabels[i]))
        {
            return g_objLabels[i];
        }
    }
    return 0;
}

/* Creates the plane that marks row, as large as its text once laid out. */
CreateMark(row)
{
    new Float:x;
    new Float:mark_w;
    new Float:mark_h;
    new rect[5];
    new label;
    new mark;
    label = GetRowLabel(row);
    for (; ; )
    {
        GetOptionP(label, TAG("RECT"), rect, 5);
        if (rect[0])
        {
            break;
        }
        sleep;
    }
    mark_w = 4.0 + float(rect[3] >> 6);
    mark_h = 4.0 + float(rect[4] >> 6);
    mark = CreatePlane(row);
    x = x + 0.5 * mark_w;
    SetPos(mark, x, 0.0, 0.0, 0);
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
    else if (g_iPicked == g_iSelected - g_iVisible)
    {
        if (IsValidObject(g_objMark) == 1)
        {
            AnimColor(g_objMark, 0.2, 0.8, 0.8, 0.4 * 0.0, duration, 0.0, ANIM_FUNC_LINEAR);
        }
    }
    else if (g_iPicked == g_iSelected + g_iVisible)
    {
        if (IsValidObject(g_objMark) == 1)
        {
            AnimColor(g_objMark, 0.2, 0.8, 0.8, 0.4 * 0.0, duration, 0.0, ANIM_FUNC_LINEAR);
        }
    }
}
