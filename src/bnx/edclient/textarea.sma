/*
 * /opt0/bn/script/edclient/textarea.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A read-only text of an edclient page shown as g_iDispNum labels, one per line, scrolled with up and down,
 * with a scroll bar on its left. The text is the TEXT element of argument "text_id"; a LINE_INDEX element with
 * that id records where each line starts.
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
    SCROLL_DOWN, /* a line added at the bottom: the top one goes when it ends */
    SCROLL_UP,
};

#include <float>

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

new g_any_child[1]; /* empty: any element */
new g_any_next[1]; /* empty: any element */
public g_objManager; /* the page: gets "PUSH" for circle and cross, "FOCS" for right and left */
public g_objFocus; /* the focus object, moved onto the text */
public g_iFontSize = 15; /* the font size of the text, in pixels ("FSIZ") */
public g_iMargin = 6; /* between two lines */
public g_iDispNum; /* lines shown */
public Float:g_flSpeed = 200.0; /* the time of a fade and of the slowest scroll, in ms */
public g_iPadHelp = PADHELP_CIRCLE | PADHELP_CROSS; /* the SYS_PADHELP buttons while the text has the focus */
new g_bActive; /* the text has the focus and takes the pad */
new g_iLineCount;
new g_iTopLine;
new g_iScroll;
new g_iScrollTick; /* the tickcount of the last scroll: a scroll lasts the time since the one before */
new g_iPrevTick;
new g_iMaxTime;
new g_iMinTime;
new g_objTimer; /* the timer of the scroll */
new g_szText[12288];
new g_tagLineIndex;
new g_tagTopLine; /* the LINE elements of the first and the last line shown */
new g_tagBottomLine;
new g_iBoxLines; /* g_iDispNum as given, before it is limited to the lines of the text */
new g_objScrollBar;

/* Same as find_child_tag of dom.inc. */
find_child_tag(parent, tag[], attr[], value[])
{
    new name[64];
    new node;
    new len;
    node = domFirstChild(parent, g_any_child);
    for (; node; node = domNext(node, g_any_next))
    {
        domGetTagName(node, name, 64);
        if (0 != strcmp(tag, name))
        {
            continue;
        }
        if (0 == strlen(attr) && 0 == strlen(value))
        {
            return node;
        }
        len = domGetData(node, attr, name, 64);
        if (0 == len)
        {
            continue;
        }
        if (0 == strcmp(value, name))
        {
            return node;
        }
    }
    return 0;
}

#include "background.inc"

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
 * Reads the text of argument "text_id", records where its lines start in a new LINE_INDEX element and creates the
 * g_iDispNum lines and the scroll bar.
 */
Init()
{
    new i;
    new Float:x;
    new Float:y;
    new text_node;
    new line[256];
    new text_id[64];
    new len;
    len = GetArg("text_id", text_id, 64);
    if (len == 1)
    {
        text_node = find_child_tag(domRoot(), "TEXT", "id", text_id);
        domGetData(text_node, "", g_szText, 12288);
    }
    g_iBoxLines = g_iDispNum;
    g_iLineCount = CountLines(g_szText);
    g_iDispNum = g_iDispNum > g_iLineCount ? g_iLineCount : g_iDispNum;
    g_tagLineIndex = domFindTag(domRoot(), "LINE_INDEX", text_id, 0);
    if (0 != g_tagLineIndex)
    {
        domRemoveTag(g_tagLineIndex);
    }
    g_tagLineIndex = domAddTag(domRoot(), "LINE_INDEX");
    domSetData(g_tagLineIndex, "id", text_id);
    new offset;
    new line_node;
    i = 0;
    for (; i < g_iLineCount; i++)
    {
        line_node = domAddTag(g_tagLineIndex, "LINE");
        sprintf(line, "%d", i);
        domSetData(line_node, "id", line);
        sprintf(line, "%d", offset);
        domSetData(line_node, "index", line);
        offset = SkipLine(offset, g_szText);
    }
    sprintf(line, "%d", g_iTopLine);
    g_tagTopLine = domFindTag(g_tagLineIndex, "LINE", line, 0);
    sprintf(line, "%d", g_iTopLine + g_iDispNum - 1);
    g_tagBottomLine = domFindTag(g_tagLineIndex, "LINE", line, 0);
    x = 0.0;
    y = 0.0;
    line_node = g_tagTopLine;
    i = 0;
    for (; i < g_iDispNum; i++)
    {
        len = domGetData(line_node, "index", line, 256);
        offset = atoi(line);
        GetLine(offset, g_szText, line);
        new label = CreateLineLabel(Self(), x, y, line);
        listAddLast(label);
        y = y + float(g_iFontSize + g_iMargin);
        line_node = domNext(line_node, g_any_next);
    }
    i = 0;
    for (; i < listSize(); i++)
    {
        for (; ; )
        {
            new rect[5];
            GetOptionP(listGetAt(i), TAG("RECT"), rect, 5);
            if (rect[0])
            {
                break;
            }
            sleep;
        }
    }
    i = 0;
    for (; i < listSize(); i++)
    {
        AnimColor(listGetAt(i), 0.7843, 0.7843, 0.7843, 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
    }
    g_iMaxTime = floatround(g_flSpeed, 0);
    g_iMinTime = g_iMaxTime < 60 ? g_iMaxTime : 60;
    g_objTimer = CreateNull(Self());
    CreateScrollBar();
}

/* Handles the messages and the scroll, then the pad while the text has the focus: up and down scroll a line. */
Update()
{
    ProcessMessages();
    UpdateScroll();
    if (0 == g_bActive)
    {
        return;
    }
    new repeated;
    new pressed;
    PadState(_, pressed, _, repeated, -1);
    if (pressed & PAD_CIRCLE)
    {
        SendMessage(g_objManager, TAG("PUSH"), 0, TAG("RRGT"), 0, 0);
        g_bActive = 0;
    }
    else if (pressed & PAD_CROSS)
    {
        SendMessage(g_objManager, TAG("PUSH"), 0, TAG("RDWN"), 0, 0);
        g_bActive = 0;
    }
    else if (pressed & PAD_RIGHT)
    {
        SendMessage(g_objManager, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
        g_bActive = 0;
    }
    else if (pressed & PAD_LEFT)
    {
        SendMessage(g_objManager, TAG("FOCS"), TAG("LEFT"), 0, 0, 0);
        g_bActive = 0;
    }
    else if (repeated & PAD_UP)
    {
        if (g_iTopLine > 0)
        {
            g_iTopLine = g_iTopLine > 0 ? --g_iTopLine : g_iTopLine; /* redundant as written */
            FinishScroll();
            new scroll_time = GetScrollTime();
            new Float:x;
            new Float:y;
            new label;
            new line[256];
            x = 0.0;
            y = -1.0 * float(g_iFontSize + g_iMargin);
            g_tagTopLine = domPrev(g_tagTopLine);
            g_tagBottomLine = domPrev(g_tagBottomLine);
            new len = domGetData(g_tagTopLine, "index", line, 256);
            new offset = atoi(line);
            GetLine(offset, g_szText, line);
            label = CreateLineLabel(Self(), 0.0, y, line);
            AnimColor(label, 0.7843, 0.7843, 0.7843, 1.0, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
            listInsert(0, label);
            AnimColor(listGetLast(), 0.7843, 0.7843, 0.7843, 1.0 * 0.0, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
            y = 0.0;
            for (new i; i < listSize(); i++)
            {
                AnimMoveLine(listGetAt(i), x, y, 0.0, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
                y = y + float(g_iFontSize + g_iMargin);
            }
            UpdateScrollBar();
            g_iScroll = SCROLL_UP;
            AnimTimerManager(g_objTimer, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
            play_se(ED_SE_CURSOR);
        }
    }
    else if (repeated & PAD_DOWN)
    {
        if (g_iTopLine < g_iLineCount - g_iDispNum)
        {
            g_iTopLine = g_iTopLine < g_iLineCount - g_iDispNum ? ++g_iTopLine : g_iTopLine; /* redundant as written */
            FinishScroll();
            new scroll_time = GetScrollTime();
            new Float:x;
            new Float:y;
            new label;
            new line[256];
            x = 0.0;
            y = listSize() * float(g_iFontSize + g_iMargin);
            g_tagTopLine = domNext(g_tagTopLine, g_any_next);
            g_tagBottomLine = domNext(g_tagBottomLine, g_any_next);
            new len = domGetData(g_tagBottomLine, "index", line, 256);
            new offset = atoi(line);
            GetLine(offset, g_szText, line);
            label = CreateLineLabel(Self(), x, y, line);
            AnimColor(label, 0.7843, 0.7843, 0.7843, 1.0, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
            listAddLast(label);
            AnimColor(listGetAt(0), 0.7843, 0.7843, 0.7843, 1.0 * 0.0, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
            y = -1.0 * float(g_iFontSize + g_iMargin);
            for (new i; i < listSize(); i++)
            {
                AnimMoveLine(listGetAt(i), x, y, 0.0, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
                y = y + float(g_iFontSize + g_iMargin);
            }
            UpdateScrollBar();
            g_iScroll = SCROLL_DOWN;
            AnimTimerManager(g_objTimer, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
            play_se(ED_SE_CURSOR);
        }
    }
}

ProcessMessages()
{
    new msg;
    new a;
    new Float:b;
    /* "FOCS": a the direction; "FADE": a the alpha, b the duration; "PUSH" gives the text the pad back. */
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
                AnimColor(listGetAt(i), 0.7843, 0.7843, 0.7843, 1.0 * alpha, duration, 0.0, ANIM_FUNC_LINEAR);
            }
            SendMessage(g_objScrollBar, TAG("FADE"), a, b, 0, 0);
        }
        else if (msg == TAG("PUSH"))
        {
            g_bActive = 1;
        }
        else if (msg == TAG("CHNG"))
        {
        }
        else if (msg == TAG("GTXT"))
        {
        }
    }
}

/* "FOUT" takes the focus away; any other dir gives it to the text. */
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
    send_focus(g_objFocus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
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

/* Creates the scroll bar (script scroll_bar) on the left of the text, its thumb as long as the part shown. */
CreateScrollBar()
{
    new Float:bar_h;
    new Float:length;
    new Float:top;
    new bar;
    bar = CreateNull(Self());
    bar_h = float((g_iFontSize + g_iMargin) * g_iBoxLines - g_iMargin);
    SetPos(bar, -16.0, 0.5 * bar_h, 0.0, 0);
    AttachScript(bar, _, "scroll_bar");
    SetPubVar(bar, "manager", Self());
    SetPubVar(bar, "focus", g_objFocus);
    SetPubVar(bar, "height", bar_h);
    length = float(g_iBoxLines) / float(g_iLineCount);
    length = length > 1.0 ? 1.0 : length;
    length = length < 0.0 ? 0.0 : length;
    length = length < 0.1 ? 0.1 : length;
    top = float(g_iTopLine) / float(g_iLineCount - g_iBoxLines) * (1.0 - length);
    SetPubVar(bar, "top", top);
    SetPubVar(bar, "length", length);
    g_objScrollBar = bar;
}

/* Moves the thumb of the scroll bar to g_iTopLine. */
UpdateScrollBar()
{
    new Float:length;
    new Float:top;
    length = float(g_iBoxLines) / float(g_iLineCount);
    length = length > 1.0 ? 1.0 : length;
    length = length < 0.0 ? 0.0 : length;
    length = length < 0.1 ? 0.1 : length;
    top = float(g_iTopLine) / float(g_iLineCount - g_iBoxLines) * (1.0 - length);
    SetPubVar(g_objScrollBar, "top", top);
}

/* Creates the text of a line at x, y, aligned left and top, hidden. */
CreateLineLabel(parent, Float:x, Float:y, text[])
{
    new label;
    label = CreateLabel(parent);
    SetPos(label, x, y, 0.0, 0);
    SetColor(label, 0.7843, 0.7843, 0.7843, 0.0);
    SetOptionI(label, TAG("ALPH"), BLEND_ADD);
    SetOptionI(label, TAG("HALN"), ALIGN_LEFT);
    SetOptionI(label, TAG("VALN"), ALIGN_TOP);
    SetOptionI(label, TAG("FSIZ"), g_iFontSize);
    SetOptionS(label, TAG("UTF8"), text);
    return label;
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

/* Shows the buttons of g_iPadHelp in the pad help. */
ShowPadHelp()
{
    SetSystemParam(SYS_PADHELP, g_iPadHelp);
}

/* Returns the bytes of the UTF-8 character whose first byte is byte. */
GetCharLength(byte)
{
    new len;
    new mask = 0xFC;
    if (byte < 0x80)
    {
        return 1;
    }
    len = 6;
    for (; len > 0; len--)
    {
        if (byte & mask == mask & 0xFF)
        {
            break;
        }
        mask = mask << 1;
    }
    return len;
}

/* Returns the lines of the packed text, ended by a newline or by the end of the text. */
CountLines(text[])
{
    new pos;
    new chars;
    new lines;
    pos = 0;
    chars = 0;
    lines = 0;
    for (; ; )
    {
        if (0 == text{pos})
        {
            if (0 != chars)
            {
                lines++;
            }
            break;
        }
        if (text{pos} == '\n')
        {
            pos++;
            lines++;
            chars = 0;
            continue;
        }
        pos = pos + GetCharLength(text{pos});
        chars++;
    }
    return lines;
}

/* Copies into line the line of text that starts at byte start; returns where the next one starts. */
GetLine(start, text[], line[])
{
    new pos;
    new i;
    new out;
    new chars;
    new lines;
    new len;
    pos = start;
    out = 0;
    chars = 0;
    lines = 0;
    if (start < 0)
    {
        line{0} = 0;
        return -1;
    }
    for (; ; )
    {
        if (0 == text{pos})
        {
            if (0 != chars)
            {
                lines++;
            }
            break;
        }
        if (text{pos} == '\n')
        {
            pos++;
            chars = 0;
            lines++;
            break;
        }
        len = GetCharLength(text{pos});
        i = 0;
        for (; i < len; i++)
        {
            line{out++} = text{pos++};
        }
        chars++;
    }
    line{out} = 0;
    return pos;
}

/* Returns where the line after the one at byte start of text starts. */
SkipLine(start, text[])
{
    new pos = start;
    for (; ; )
    {
        if (0 == text{pos})
        {
            break;
        }
        if (text{pos} == '\n')
        {
            pos++;
            break;
        }
        pos = pos + GetCharLength(text{pos});
    }
    return pos;
}
