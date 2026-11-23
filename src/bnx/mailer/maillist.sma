/*
 * /opt0/bn/script/mailer/maillist.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The mail list of the mailer, opened by main: seven lines of a folder (left and right change the folder), its title,
 * the unread and position counters and a scroll bar; it runs the actions of main on the selected mail.
 */

#include <core>
#include <bn>

#pragma dynamic 10000

#include "mailer.inc"

#include "tag.inc"

#include <float>

new Float:g_flPi = 3.141592;

/* The labels of g_objCounters. */
const COUNTER_UNREAD = 0;
const COUNTER_POSITION = 1;

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

/* Appends src to dest (maxlen cells), packed or unpacked like dest. */
append_string(dest[], src[], maxlen)
{
    new dest_len;
    new src_len;
    new i;
    new room;
    dest_len = strlen(dest);
    src_len = strlen(src);
    if (is_packed(dest))
    {
        room = maxlen * 4 - dest_len - 1;
        if (is_packed(src))
        {
            i = 0;
            for (; i < src_len && i < room; i++)
            {
                dest{dest_len + i} = src{i};
            }
        }
        else
        {
            i = 0;
            for (; i < src_len && i < room; i++)
            {
                dest{dest_len + i} = src[i];
            }
        }
        new pad = 4 - (dest_len + i) % 4;
        for (new j; j < pad; j++)
        {
            dest{dest_len + i + j} = 0;
        }
    }
    else
    {
        room = maxlen - dest_len - 1;
        if (is_packed(src))
        {
            i = 0;
            for (; i < src_len && i < room; i++)
            {
                dest[dest_len + i] = src{i};
            }
        }
        else
        {
            i = 0;
            for (; i < src_len && i < room; i++)
            {
                dest[dest_len + i] = src[i];
            }
        }
        dest[dest_len + i] = 0;
    }
    return;
}

#include "text.inc"

/* Copies src into dest (maxlen cells) with every occurrence of from replaced by to. */
replace_all(dest[], src[], from[], to[], maxlen)
{
    new count;
    new from_len = strlen(from);
    new to_len = strlen(to);
    new src_pos;
    new dest_pos;
    for (; ; )
    {
        new found = find_substring(src, from, src_pos);
        if (found < 0)
        {
            copy_substring(dest, dest_pos, src, src_pos, -1, maxlen);
            return count;
        }
        copy_substring(dest, dest_pos, src, src_pos, found - src_pos, maxlen);
        dest_pos = dest_pos + (found - src_pos);
        append_string(dest, to, maxlen);
        dest_pos = dest_pos + to_len;
        src_pos = found + from_len;
    }
    return 0;
}

/* Same as get_tag_attr2 of text.inc. */
get_tag_attr(tag[], id[], attr[], value[], maxlen)
{
    if (0 == tag[0] || 0 == id[0])
    {
        return 0;
    }
    new node = domFindTag(domRoot(), tag, id, 0);
    if (node)
    {
        new len = domGetData(node, attr, value, maxlen);
        if (0 == strcmp(attr, "src") || 0 == strcmp(attr, "href"))
        {
            xmlMakeFullPath(value, maxlen);
        }
        return len;
    }
    return 0;
}

/* Same as get_group_tag_attr of text.inc. */
get_group_tag_attr3(tag[], group[], index, attr[], value[], maxlen)
{
    new count;
    new dom_root = domRoot();
    for (new node = domFirstChild(dom_root, tag); node; node = domNext(node, tag))
    {
        new name[64];
        domGetData(node, "group", name, 64);
        if (0 == strcmp(name, group))
        {
            if (count == index)
            {
                new len = domGetData(node, attr, value, maxlen);
                if (0 == strcmp(attr, "src") || 0 == strcmp(attr, "href"))
                {
                    xmlMakeFullPath(value, maxlen);
                }
                return len;
            }
            count++;
        }
    }
    return 0;
}

#include "mailsync.inc"

new Float:g_flTitleRed = 0.902;
new Float:g_flTitleGreen = 0.902;
new Float:g_flTitleBlue = 0.215;
new Float:g_flDimAlpha = 0.2;
new Float:g_flBlinkOutTime = 1200.0;
new Float:g_flBlinkInTime = 600.0;
new g_iItemCount;
new g_szItemId[64];

/* Appends to ids, from g_iItemCount on, the id of every element below node with no children and no text. */
CollectItemIds(node, ids[][])
{
    new tag_name[16];
    domGetTagName(node, tag_name, 16);
    g_szItemId[0] = 0;
    domGetData(node, "id", g_szItemId, 64);
    static text[256];
    text[0] = 0;
    domGetData(node, _, text, 256);
    if (0 == domFirstChild(node))
    {
        if (0 == strlen(text))
        {
            sprintf(ids[g_iItemCount++], "%s", g_szItemId);
        }
    }
    else
    {
        for (new child = domFirstChild(node); child; child = domNext(child))
        {
            CollectItemIds(child, ids);
        }
    }
}

new g_iItemIndex;
new g_szFindId[64];

/* Counts the empty leaves below node like CollectItemIds; the one whose id is id sets g_iItemIndex. */
FindItemIndex(node, id[])
{
    new tag_name[16];
    domGetTagName(node, tag_name, 16);
    g_szFindId[0] = 0;
    domGetData(node, "id", g_szFindId, 64);
    static text[256];
    text[0] = 0;
    domGetData(node, _, text, 256);
    if (0 == domFirstChild(node))
    {
        if (0 == strlen(text))
        {
            if (!strcmp(g_szFindId, id))
            {
                g_iItemIndex = g_iItemCount;
            }
            g_iItemCount++;
        }
    }
    else
    {
        for (new child = domFirstChild(node); child; child = domNext(child))
        {
            FindItemIndex(child, id);
        }
    }
}

#include "mailline.inc"

public g_manager; /* the mailer page: holds the textures; gets "SHWT", "ACWT" after "ACT", "FOL" for "LINE" */
public g_focus; /* the focus object */
public g_active; /* the list reads the pad */
public g_count; /* mails in the folder */
public g_line; /* the row of the cursor */
public g_top; /* the mail on the first row */
public g_folder; /* FOLDER_* */
public g_blink; /* the cursor blinks */
public g_space; /* the result of the last deletion: 0, ERR_NOSPACE or 1 */
new g_objTitle;
new g_objNextArrow;
new g_objPrevArrow;
new g_objArrowGlow;
new g_objScrollBar;
new g_objFolderIcon;
new g_objCursor;
new g_objCounters[2]; /* the unread count and the position */
new g_objScrollTimer;
new g_iScrollDir;
new g_iScrollCount; /* the lines added by the scroll in progress */
new g_iFolderTops[4]; /* the top and the line of each folder */
new g_iFolderLines[4];
new g_bBlinkOn;
new g_iAddrIndex; /* the address book entry found by "ACHK" */

/* Creates a line of the list (mailline) under parent. */
CreateLine(parent)
{
    new tex = GetPubVar(g_manager, "g_txListBg");
    new Float:tex_w = GetPubVar(g_manager, "g_ListBgWidth");
    new Float:tex_h = GetPubVar(g_manager, "g_ListBgHeight");
    new line = CreatePlane(parent);
    SetColor(line, 0.0, 0.0, 0.0, 0.0);
    SetLocalScale(line, tex_w, tex_h, 0.0);
    SetOptionI(line, TAG("ALPH"), BLEND_ADD);
    SetTexture(line, tex, 0, -1);
    AttachScript(line, _, "mailline", "");
    SetPubVar(line, "g_manager", g_manager);
    SetPubVar(line, "g_focus", g_focus);
    return line;
}

main()
{
    new value[8];
    new Float:pos[3];
    new Float:width;
    new Float:height;
    new Float:list_pos[3];
    new Float:line_h;
    listRemoveAll();
    g_iScrollDir = SCROLL_NONE;
    g_iScrollCount = 0;
    if (GetArg("folder", value, 8))
    {
        g_folder = atoi(value);
    }
    if (GetArg("line0", value, 8))
    {
        g_iFolderLines[FOLDER_INBOX] = atoi(value);
    }
    if (GetArg("top0", value, 8))
    {
        g_iFolderTops[FOLDER_INBOX] = atoi(value);
    }
    if (GetArg("line1", value, 8))
    {
        g_iFolderLines[FOLDER_OUTBOX] = atoi(value);
    }
    if (GetArg("top1", value, 8))
    {
        g_iFolderTops[FOLDER_OUTBOX] = atoi(value);
    }
    if (GetArg("line2", value, 8))
    {
        g_iFolderLines[FOLDER_DRAFT] = atoi(value);
    }
    if (GetArg("top2", value, 8))
    {
        g_iFolderTops[FOLDER_DRAFT] = atoi(value);
    }
    if (GetArg("line3", value, 8))
    {
        g_iFolderLines[FOLDER_QUEUE] = atoi(value);
    }
    if (GetArg("top3", value, 8))
    {
        g_iFolderTops[FOLDER_QUEUE] = atoi(value);
    }
    g_line = g_iFolderLines[g_folder];
    g_top = g_iFolderTops[g_folder];
    new arrow_tex = GetPubVar(g_manager, "g_txArrow");
    new Float:arrow_w = GetPubVar(g_manager, "g_ArrowWidth");
    new Float:arrow_h = GetPubVar(g_manager, "g_ArrowHeight");
    new glow_tex = GetPubVar(g_manager, "g_txArrowGlow");
    new Float:glow_w = GetPubVar(g_manager, "g_ArrowGlowWidth");
    new Float:glow_h = GetPubVar(g_manager, "g_ArrowGlowHeight");
    new cursor_tex = GetPubVar(g_manager, "g_txListFg");
    get_group_tag_attr3("LIST", "HEAD", 0, "list_x", value, 8);
    list_pos[0] = atof(value);
    get_group_tag_attr3("LIST", "HEAD", 0, "list_y", value, 8);
    list_pos[1] = atof(value);
    get_group_tag_attr3("LIST", "HEAD", 0, "list_z", value, 8);
    list_pos[2] = atof(value);
    get_group_tag_attr3("LIST", "HEAD", 0, "line_height", value, 8);
    line_h = atof(value);
    for (new i; i < LIST_LINES; i++)
    {
        new line = CreateLine(Self());
        SetPos(line, list_pos[0], list_pos[1] + line_h * i, list_pos[2], 0);
        listAddLast(line);
    }
    width = GetPubVar(g_manager, "g_ListBgWidth");
    height = GetPubVar(g_manager, "g_ListBgHeight");
    g_objCursor = CreatePlane(Self());
    SetScale(g_objCursor, width, height, 0.0);
    SetColor(g_objCursor, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_objCursor, TAG("ALPH"), BLEND_ADD);
    SetOptionF(g_objCursor, TAG("ZOFS"), -1.0);
    SetOptionI(g_objCursor, TAG("ZSRT"), ZSORT_OVERLAY);
    SetTexture(g_objCursor, cursor_tex, 0, -1);
    Hide(g_objCursor);
    get_group_tag_attr3("LIST", "HEAD", 0, "bar_x", value, 8);
    pos[0] = atof(value);
    get_group_tag_attr3("LIST", "HEAD", 0, "bar_y", value, 8);
    pos[1] = atof(value);
    get_group_tag_attr3("LIST", "HEAD", 0, "bar_z", value, 8);
    pos[2] = atof(value);
    get_group_tag_attr3("LIST", "HEAD", 0, "bar_h", value, 8);
    height = atof(value);
    g_objScrollBar = CreateNull(Self());
    SetPos(g_objScrollBar, pos[0], pos[1], pos[2], 0);
    SetOptionI(g_objScrollBar, TAG("ZSRT"), ZSORT_OVERLAY);
    AttachScript(g_objScrollBar, _, "scroll_bar", "");
    SetPubVar(g_objScrollBar, "manager", Self());
    SetPubVar(g_objScrollBar, "focus", g_focus);
    SetPubVar(g_objScrollBar, "height", height);
    SendMessage(g_objScrollBar, TAG("FADE"), 0, 0, 0, 0);
    get_group_tag_attr3("LIST", "HEAD", 0, "folder_x", value, 8);
    pos[0] = atof(value);
    get_group_tag_attr3("LIST", "HEAD", 0, "folder_y", value, 8);
    pos[1] = atof(value);
    get_group_tag_attr3("LIST", "HEAD", 0, "folder_z", value, 8);
    pos[2] = atof(value);
    g_objTitle = CreateLabel(Self());
    SetPos(g_objTitle, pos[0], pos[1], pos[2], 0);
    SetColor(g_objTitle, g_flTitleRed, g_flTitleGreen, g_flTitleBlue, 0.0);
    SetOptionI(g_objTitle, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(g_objTitle, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_objTitle, TAG("FSIZ"), 17);
    SetOptionI(g_objTitle, TAG("ZSRT"), ZSORT_OVERLAY);
    get_group_tag_attr3("LIST", "HEAD", 0, "prev_x", value, 8);
    pos[0] = atof(value);
    get_group_tag_attr3("LIST", "HEAD", 0, "prev_y", value, 8);
    pos[1] = atof(value);
    get_group_tag_attr3("LIST", "HEAD", 0, "prev_z", value, 8);
    pos[2] = atof(value);
    g_objPrevArrow = CreatePlane(Self());
    SetPos(g_objPrevArrow, pos[0], pos[1], pos[2], 0);
    SetScale(g_objPrevArrow, arrow_w, arrow_h, 0.0);
    SetColor(g_objPrevArrow, 0.6, 0.6, 0.6, 0.0);
    SetTexture(g_objPrevArrow, arrow_tex, 0, -1);
    SetOptionI(g_objPrevArrow, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(g_objPrevArrow, TAG("ALPH"), BLEND_ADD);
    get_group_tag_attr3("LIST", "HEAD", 0, "next_x", value, 8);
    pos[0] = atof(value);
    get_group_tag_attr3("LIST", "HEAD", 0, "next_y", value, 8);
    pos[1] = atof(value);
    get_group_tag_attr3("LIST", "HEAD", 0, "next_z", value, 8);
    pos[2] = atof(value);
    g_objNextArrow = CreatePlane(Self());
    SetPos(g_objNextArrow, pos[0], pos[1], pos[2], 0);
    SetScale(g_objNextArrow, arrow_w, arrow_h, 0.0);
    SetRot(g_objNextArrow, 0.0, 0.0, g_flPi, 0, 0);
    SetColor(g_objNextArrow, 0.6, 0.6, 0.6, 0.0);
    SetTexture(g_objNextArrow, arrow_tex, 0, -1);
    SetOptionI(g_objNextArrow, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(g_objNextArrow, TAG("ALPH"), BLEND_ADD);
    g_objArrowGlow = CreatePlane(Self());
    SetPos(g_objArrowGlow, pos[0], pos[1], pos[2], 0);
    SetScale(g_objArrowGlow, glow_w, glow_h, 0.0);
    SetRot(g_objArrowGlow, 0.0, 0.0, g_flPi, 0, 0);
    SetColor(g_objArrowGlow, 1.0, 1.0, 1.0, 0.0);
    SetTexture(g_objArrowGlow, glow_tex, 0, -1);
    SetOptionI(g_objArrowGlow, TAG("ALPH"), BLEND_ADD);
    SetOptionI(g_objArrowGlow, TAG("ZSRT"), ZSORT_OVERLAY);
    get_group_tag_attr3("LIST", "HEAD", 0, "index_x", value, 8);
    pos[0] = atof(value);
    get_group_tag_attr3("LIST", "HEAD", 0, "index_y", value, 8);
    pos[1] = atof(value);
    get_group_tag_attr3("LIST", "HEAD", 0, "index_z", value, 8);
    pos[2] = atof(value);
    width = GetPubVar(g_manager, "g_BoxInboxWidth");
    height = GetPubVar(g_manager, "g_BoxInboxHeight");
    g_objFolderIcon = CreatePlane(Self());
    SetPos(g_objFolderIcon, pos[0], pos[1], pos[2], 0);
    SetScale(g_objFolderIcon, width, height, 0.0);
    SetColor(g_objFolderIcon, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_objFolderIcon, TAG("ZSRT"), ZSORT_OVERLAY);
    get_group_tag_attr3("LIST", "HEAD", 0, "mail1_x", value, 8);
    pos[0] = atof(value);
    get_group_tag_attr3("LIST", "HEAD", 0, "mail1_y", value, 8);
    pos[1] = atof(value);
    get_group_tag_attr3("LIST", "HEAD", 0, "mail1_z", value, 8);
    pos[2] = atof(value);
    g_objCounters[COUNTER_UNREAD] = CreateLabel(Self());
    SetPos(g_objCounters[COUNTER_UNREAD], pos[0], pos[1], pos[2], 0);
    SetColor(g_objCounters[COUNTER_UNREAD], 0.7843, 0.7843, 0.7843, 0.0);
    SetOptionI(g_objCounters[COUNTER_UNREAD], TAG("HALN"), ALIGN_RIGHT);
    SetOptionI(g_objCounters[COUNTER_UNREAD], TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_objCounters[COUNTER_UNREAD], TAG("FSIZ"), 13);
    SetOptionI(g_objCounters[COUNTER_UNREAD], TAG("ZSRT"), ZSORT_OVERLAY);
    get_group_tag_attr3("LIST", "HEAD", 0, "mail2_x", value, 8);
    pos[0] = atof(value);
    get_group_tag_attr3("LIST", "HEAD", 0, "mail2_y", value, 8);
    pos[1] = atof(value);
    get_group_tag_attr3("LIST", "HEAD", 0, "mail2_z", value, 8);
    pos[2] = atof(value);
    g_objCounters[COUNTER_POSITION] = CreateLabel(Self());
    SetPos(g_objCounters[COUNTER_POSITION], pos[0], pos[1], pos[2], 0);
    SetColor(g_objCounters[COUNTER_POSITION], 0.7843, 0.7843, 0.7843, 0.0);
    SetOptionI(g_objCounters[COUNTER_POSITION], TAG("HALN"), ALIGN_RIGHT);
    SetOptionI(g_objCounters[COUNTER_POSITION], TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_objCounters[COUNTER_POSITION], TAG("FSIZ"), 13);
    SetOptionI(g_objCounters[COUNTER_POSITION], TAG("ZSRT"), ZSORT_OVERLAY);
    g_objScrollTimer = CreateNull(Self());
    LoadFolder(g_top, g_line);
    sleep;
    SendMessage(Self(), TAG("FADE"), TAG("IN"), 0, 0, 0);
    SendMessage(g_manager, TAG("SHWT"), 0, 0, 0, 0);
    for (; ; )
    {
        new held;
        new pressed;
        new released;
        new repeated;
        if (g_active)
        {
            PadState(held, pressed, released, repeated, -1);
        }
        new old_line = g_line;
        if (held & (PAD_CROSS | PAD_CIRCLE | PAD_TRIANGLE))
        {
            sleep;
            continue;
        }
        else
        {
            static last_tick;
            if (pressed & PAD_LEFT || (repeated & PAD_LEFT && tickcount() - last_tick > 70))
            {
                last_tick = tickcount();
                if (SCROLL_NONE == g_iScrollDir)
                {
                    sndSePlay(SE_CURSOR);
                    g_folder = g_folder <= FOLDER_INBOX ? (g_folder = FOLDER_QUEUE) : g_folder - 1;
                    g_line = g_iFolderLines[g_folder];
                    g_top = g_iFolderTops[g_folder];
                    SetActive(0);
                    SetBlink(0);
                    SlideOutFolder(-1.0, 0.0, 150.0, 0.0);
                    LoadFolder(g_top, g_line);
                    sleep;
                    SlideInFolder(-1.0, 1.0, 150.0, 0.0);
                    SetBlink(1);
                    HighlightLine(listGetAt(g_line));
                    SetActive(1);
                }
            }
            else if (pressed & PAD_RIGHT || (repeated & PAD_RIGHT && tickcount() - last_tick > 70))
            {
                last_tick = tickcount();
                if (SCROLL_NONE == g_iScrollDir)
                {
                    sndSePlay(SE_CURSOR);
                    g_folder = g_folder >= FOLDER_QUEUE ? (g_folder = FOLDER_INBOX) : g_folder + 1;
                    g_line = g_iFolderLines[g_folder];
                    g_top = g_iFolderTops[g_folder];
                    SetActive(0);
                    SetBlink(0);
                    SlideOutFolder(1.0, 0.0, 150.0, 0.0);
                    LoadFolder(g_top, g_line);
                    sleep;
                    SlideInFolder(1.0, 1.0, 150.0, 0.0);
                    SetBlink(1);
                    HighlightLine(listGetAt(g_line));
                    SetActive(1);
                }
            }
            else if (pressed & PAD_L1 || (repeated & PAD_L1 && tickcount() - last_tick > 70))
            {
                last_tick = tickcount();
                if (g_top + g_line > 0 && SCROLL_NONE == g_iScrollDir)
                {
                    sndSePlay(SE_CURSOR);
                    if (g_top + g_line - LIST_LINES > LIST_LINES)
                    {
                        g_top = g_top - LIST_LINES;
                    }
                    else
                    {
                        g_top = g_line = 0;
                    }
                    LoadFolder(g_top, g_line);
                    SetBlink(1);
                    HighlightLine(listGetAt(g_line));
                }
            }
            else if (pressed & PAD_R1 || (repeated & PAD_R1 && tickcount() - last_tick > 70))
            {
                last_tick = tickcount();
                if (g_top + g_line < g_count - 1 && SCROLL_NONE == g_iScrollDir)
                {
                    sndSePlay(SE_CURSOR);
                    if (g_top + g_line + LIST_LINES < g_count - LIST_LINES)
                    {
                        g_top = g_top + LIST_LINES;
                    }
                    else if (LIST_LINES < g_count)
                    {
                        g_line = LIST_LINES - 1;
                        g_top = g_count - g_line;
                    }
                    else
                    {
                        g_line = g_count - 1;
                        g_top = 0;
                    }
                    LoadFolder(g_top, g_line);
                    SetBlink(1);
                    HighlightLine(listGetAt(g_line));
                }
            }
            else
            {
                static prev_tick;
                if (pressed & PAD_UP || (repeated & PAD_UP && tickcount() - last_tick > 70))
                {
                    prev_tick = last_tick;
                    last_tick = tickcount();
                    new move_time = last_tick - prev_tick;
                    move_time = clamp(move_time, 4, 30);
                    if (old_line > 0 && SCROLL_NONE == g_iScrollDir)
                    {
                        sndSePlay(SE_CURSOR);
                        MoveCursor((old_line - 1 + LIST_LINES) % LIST_LINES);
                        UpdateCounter(g_folder, g_top + g_line);
                        SetBlink(1);
                        HighlightLine(listGetAt(g_line));
                    }
                    else if (g_top > 0 && SCROLL_NONE == g_iScrollDir)
                    {
                        sndSePlay(SE_CURSOR);
                        g_top--;
                        {
                            new line = CreateLine(Self());
                            SetPos(line, list_pos[0], list_pos[1] - line_h, list_pos[2], 0);
                            listInsert(0, line);
                            SetLine(listGetAt(0), g_folder, g_top);
                        }
                        for (new i; i < listSize(); i++)
                        {
                            new Float:line_pos[3];
                            GetPos(listGetAt(i), line_pos[0], line_pos[1], line_pos[2], 0);
                            AnimMoveLine(listGetAt(i), line_pos[0], line_pos[1] + line_h, line_pos[2], float(move_time),
                                    0.0, ANIM_FUNC_LINEAR);
                            SendMessage(listGetAt(i), TAG("FADE"), TAG("IN2"), float(move_time), 0, 0);
                        }
                        FadeAlpha(listGetLast(), 0.0, float(move_time), 0.0);
                        SendMessage(listGetLast(), TAG("FADE"), TAG("OUT"), float(move_time), 0, 0);
                        FadeAlpha(listGetAt(0), 1.0, float(move_time), 0.0);
                        SendMessage(listGetAt(0), TAG("FADE"), TAG("IN1"), float(move_time), 0, 0);
                        g_iScrollDir = SCROLL_UP;
                        g_iScrollCount++;
                        AnimTimerManager(g_objScrollTimer, float(move_time), 0.0, ANIM_FUNC_LINEAR);
                        UpdateScrollBar();
                        UpdateCounter(g_folder, g_top + g_line);
                        SetBlink(1);
                    }
                }
                else if (pressed & PAD_DOWN || (repeated & PAD_DOWN && tickcount() - last_tick > 70))
                {
                    prev_tick = last_tick;
                    last_tick = tickcount();
                    new move_time = last_tick - prev_tick;
                    move_time = clamp(move_time, 4, 30);
                    if (old_line < LIST_LINES - 1 && old_line < g_count - 1 && SCROLL_NONE == g_iScrollDir)
                    {
                        sndSePlay(SE_CURSOR);
                        MoveCursor((old_line + 1 + LIST_LINES) % LIST_LINES);
                        UpdateCounter(g_folder, g_top + g_line);
                        SetBlink(1);
                        HighlightLine(listGetAt(g_line));
                    }
                    else if (g_top < g_count - LIST_LINES && SCROLL_NONE == g_iScrollDir)
                    {
                        sndSePlay(SE_CURSOR);
                        g_top++;
                        {
                            new line = CreateLine(Self());
                            SetPos(line, list_pos[0], list_pos[1] + LIST_LINES * line_h, list_pos[2], 0);
                            listAddLast(line);
                            SetLine(listGetLast(), g_folder, g_top + LIST_LINES - 1);
                        }
                        for (new i; i < listSize(); i++)
                        {
                            new Float:line_pos[3];
                            GetPos(listGetAt(i), line_pos[0], line_pos[1], line_pos[2], 0);
                            AnimMoveLine(listGetAt(i), line_pos[0], line_pos[1] - line_h, line_pos[2], float(move_time),
                                    0.0, ANIM_FUNC_LINEAR);
                            SendMessage(listGetAt(i), TAG("FADE"), TAG("IN2"), float(move_time), 0, 0);
                        }
                        FadeAlpha(listGetAt(0), 0.0, float(move_time), 0.0);
                        SendMessage(listGetAt(0), TAG("FADE"), TAG("OUT"), float(move_time), 0, 0);
                        FadeAlpha(listGetLast(), 1.0, float(move_time), 0.0);
                        SendMessage(listGetLast(), TAG("FADE"), TAG("IN1"), float(move_time), 0, 0);
                        g_iScrollDir = SCROLL_DOWN;
                        g_iScrollCount++;
                        AnimTimerManager(g_objScrollTimer, float(move_time), 0.0, ANIM_FUNC_LINEAR);
                        UpdateScrollBar();
                        UpdateCounter(g_folder, g_top + g_line);
                        SetBlink(1);
                    }
                }
            }
        }
        FinishScroll();
        new msg;
        new what;
        new arg;
        /*
         * The actions of main on the selected mail ("ACT"), each answered with "ACWT": "ADDR" adds its address to the
         * address book, "AEXT" over the entry found by "ACHK" (which answers that entry and whether the address or the
         * name is too long), "LOCK" toggles its lock, "DELE" deletes it, "SEND" and "SDCK" answer its line and whether
         * it has a recipient, "LINE" sends the folder and the lines of every folder first.
         */
        while (RecvMessage(msg, what, arg))
        {
            if (msg == TAG("FOCS"))
            {
                if (what == TAG("IN"))
                {
                    SetActive(1);
                }
                else if (what == TAG("OUT"))
                {
                    SetActive(0);
                }
            }
            else if (msg == TAG("FADE"))
            {
                if (what == TAG("IN"))
                {
                    FadeIn();
                }
                else if (what == TAG("OUT"))
                {
                    FadeOut();
                }
                else if (what == TAG("DIM"))
                {
                    FadeDim();
                }
            }
            else if (msg == TAG("BLNK"))
            {
                if (what == TAG("IN"))
                {
                    SetBlink(1);
                    HighlightLine(listGetAt(g_line));
                }
                else if (what == TAG("OUT"))
                {
                    SetBlink(0);
                }
            }
            else if (msg == TAG("ACT"))
            {
                new addr[246];
                if (what == TAG("ADDR"))
                {
                    GetMsgPeer(g_folder, g_top + g_line, addr);
                    mx_SetAddr(-1, addr);
                    SendMessage(g_manager, TAG("ACWT"), 0, 0, 0, 0);
                }
                else if (what == TAG("AEXT"))
                {
                    GetMsgPeer(g_folder, g_top + g_line, addr);
                    mx_SetAddr(g_iAddrIndex, addr);
                    SendMessage(g_manager, TAG("ACWT"), 0, 0, 0, 0);
                }
                else if (what == TAG("ACHK"))
                {
                    GetMsgPeerAddr(g_folder, g_top + g_line, addr);
                    g_iAddrIndex = mx_CheckAddr(-1, addr);
                    new too_long;
                    if (strlen(addr) >= 128)
                    {
                        too_long = 1;
                    }
                    GetMsgPeerName(g_folder, g_top + g_line, addr);
                    if (strlen(addr) >= 128)
                    {
                        too_long = 1;
                    }
                    SendMessage(g_manager, TAG("ACWT"), g_iAddrIndex, too_long, 0, 0);
                }
                else if (what == TAG("LOCK"))
                {
                    if (IsLocked(g_folder, g_top + g_line))
                    {
                        mx_SetMsgStatus(g_folder, g_top + g_line, MSG_UNLOCK);
                    }
                    else
                    {
                        mx_SetMsgStatus(g_folder, g_top + g_line, MSG_LOCK);
                    }
                    LoadFolder(g_top, g_line);
                    SendMessage(g_manager, TAG("ACWT"), 0, 0, 0, 0);
                }
                else if (what == TAG("DELE"))
                {
                    DeleteMsg(g_folder, g_top, g_line);
                    SendMessage(g_manager, TAG("ACWT"), 0, 0, 0, 0);
                }
                else if (what == TAG("SEND"))
                {
                    SendMessage(g_manager, TAG("ACWT"), g_top + g_line, 0, 0, 0);
                }
                else if (what == TAG("SDCK"))
                {
                    new has_to = HasRecipient(g_folder, g_top + g_line);
                    SendMessage(g_manager, TAG("ACWT"), has_to, 0, 0, 0);
                }
                else if (what == TAG("LINE"))
                {
                    SendMessage(g_manager, TAG("FOL"), g_folder, 0, 0, 0);
                    SendMessage(g_manager, TAG("L0"), g_iFolderTops[FOLDER_INBOX], g_iFolderLines[FOLDER_INBOX], 0, 0);
                    SendMessage(g_manager, TAG("L1"), g_iFolderTops[FOLDER_OUTBOX], g_iFolderLines[FOLDER_OUTBOX], 0,
                            0);
                    SendMessage(g_manager, TAG("L2"), g_iFolderTops[FOLDER_DRAFT], g_iFolderLines[FOLDER_DRAFT], 0, 0);
                    SendMessage(g_manager, TAG("L3"), g_iFolderTops[FOLDER_QUEUE], g_iFolderLines[FOLDER_QUEUE], 0, 0);
                    SendMessage(g_manager, TAG("ACWT"), 0, 0, 0, 0);
                }
            }
        }
        if (g_blink)
        {
            new Float:color[4];
            if (ANIM_DONE == AnimGetState(g_objCursor, ANIM_COLOR))
            {
                new Float:alpha = 0.7;
                if (!g_bBlinkOn)
                {
                    alpha = 0.25;
                }
                GetColor(g_objCursor, color[0], color[1], color[2], color[3]);
                AnimColor(g_objCursor, color[0], color[1], color[2], alpha, g_flBlinkOutTime, 0.0, ANIM_FUNC_LINEAR);
                g_bBlinkOn = g_bBlinkOn ^ 1;
            }
        }
        sleep;
    }
    return;
}

/* Destroys the lines scrolled out of the list once the scroll timer is over. */
FinishScroll()
{
    if (SCROLL_NONE == g_iScrollDir)
    {
        return;
    }
    if (ANIM_DONE == AnimGetState(g_objScrollTimer, ANIM_TIMER))
    {
        if (g_iScrollDir == SCROLL_DOWN)
        {
            for (new i; i < g_iScrollCount; i++)
            {
                Destroy(listGetAt(0));
                listRemoveAt(0);
            }
        }
        else if (g_iScrollDir == SCROLL_UP)
        {
            for (new i; i < g_iScrollCount; i++)
            {
                Destroy(listGetLast());
                listRemoveLast();
            }
        }
        g_iScrollDir = SCROLL_NONE;
        g_iScrollCount = 0;
    }
}

/* Fades the title, the folder icon, the arrows, the counters and the scroll bar to alpha. */
FadeFrame(Float:alpha, Float:fade_time, Float:delay)
{
    assert 0 - alpha;
    new Float:color[4];
    GetColor(g_objTitle, color[0], color[1], color[2], color[3]);
    AnimColor(g_objTitle, color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objFolderIcon, color[0], color[1], color[2], color[3]);
    AnimColor(g_objFolderIcon, color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objNextArrow, color[0], color[1], color[2], color[3]);
    AnimColor(g_objNextArrow, color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objPrevArrow, color[0], color[1], color[2], color[3]);
    AnimColor(g_objPrevArrow, color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objCounters[COUNTER_UNREAD], color[0], color[1], color[2], color[3]);
    AnimColor(g_objCounters[COUNTER_UNREAD], color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objCounters[COUNTER_POSITION], color[0], color[1], color[2], color[3]);
    AnimColor(g_objCounters[COUNTER_POSITION], color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    if (g_count > LIST_LINES)
    {
        SendMessage(g_objScrollBar, TAG("FADE"), alpha, fade_time, delay, 0);
    }
    AnimTimerManager(g_objScrollTimer, fade_time, 0.0, ANIM_FUNC_LINEAR);
}

/* Fades the alpha of obj, keeping its color. */
FadeAlpha(obj, Float:alpha, Float:fade_time, Float:delay)
{
    new Float:color[4];
    GetColor(obj, color[0], color[1], color[2], color[3]);
    AnimColor(obj, color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
}

FadeIn()
{
    FadeFrame(1.0, float(400), 0.0);
    for (new i; i < listSize(); i++)
    {
        FadeAlpha(listGetAt(i), 1.0, float(400), 0.0);
        SendMessage(listGetAt(i), TAG("FADE"), TAG("IN2"), float(400), 0, 0);
    }
}

FadeOut()
{
    FadeFrame(0.0, float(400), 0.0);
    for (new i; i < listSize(); i++)
    {
        FadeAlpha(listGetAt(i), 0.0, float(400), 0.0);
        SendMessage(listGetAt(i), TAG("FADE"), TAG("OUT"), float(400), 0, 0);
    }
}

FadeDim()
{
    FadeFrame(g_flDimAlpha, float(400), 0.0);
    for (new i; i < listSize(); i++)
    {
        FadeAlpha(listGetAt(i), g_flDimAlpha, float(400), 0.0);
        SendMessage(listGetAt(i), TAG("FADE"), TAG("DIM"), float(400), 0, 0);
    }
}

/* Fades the lines, the folder icon, the counters and the scroll bar to alpha, and waits for the end. */
FadeLines(Float:alpha, Float:fade_time, Float:delay)
{
    new Float:color[4];
    for (new i; i < listSize(); i++)
    {
        FadeAlpha(listGetAt(i), alpha, fade_time, 0.0);
        if (alpha)
        {
            SendMessage(listGetAt(i), TAG("FADE"), TAG("IN2"), fade_time, 0, 0);
        }
        else
        {
            SendMessage(listGetAt(i), TAG("FADE"), TAG("OUT"), fade_time, 0, 0);
        }
    }
    GetColor(g_objFolderIcon, color[0], color[1], color[2], color[3]);
    AnimColor(g_objFolderIcon, color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objCounters[COUNTER_UNREAD], color[0], color[1], color[2], color[3]);
    AnimColor(g_objCounters[COUNTER_UNREAD], color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objCounters[COUNTER_POSITION], color[0], color[1], color[2], color[3]);
    AnimColor(g_objCounters[COUNTER_POSITION], color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    if (g_count > LIST_LINES)
    {
        SendMessage(g_objScrollBar, TAG("FADE"), alpha, fade_time, delay, 0);
    }
    for (; ; )
    {
        sleep;
        if (ANIM_DONE == AnimGetState(g_objFolderIcon, ANIM_COLOR))
        {
            break;
        }
    }
}

/* Slides the title out towards dir (-1 left, 1 right), lights the arrow of that side and fades the lines to alpha. */
SlideOutFolder(Float:dir, Float:alpha, Float:fade_time, Float:delay)
{
    new value[5];
    new Float:pos[3];
    new Float:color[4];
    get_group_tag_attr3("LIST", "HEAD", 0, "folder_x", value, 5);
    pos[0] = atof(value);
    get_group_tag_attr3("LIST", "HEAD", 0, "folder_y", value, 5);
    pos[1] = atof(value);
    get_group_tag_attr3("LIST", "HEAD", 0, "folder_z", value, 5);
    pos[2] = atof(value);
    GetColor(g_objTitle, color[0], color[1], color[2], color[3]);
    AnimColor(g_objTitle, color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_SLOWER);
    AnimMoveLine(g_objTitle, pos[0] + 95.0 * dir, pos[1], pos[2], fade_time, 0.0, ANIM_FUNC_LINEAR);
    if (dir > 0)
    {
        AnimColor(g_objNextArrow, 1.0, 1.0, 1.0, 1.0, fade_time, delay, ANIM_FUNC_LINEAR);
        AnimColor(g_objArrowGlow, 1.0, 1.0, 1.0, 1.0, fade_time, delay, ANIM_FUNC_LINEAR);
        GetPos(g_objNextArrow, pos[0], pos[1], pos[2], 0);
        SetPos(g_objArrowGlow, pos[0], pos[1], pos[2], 0);
        SetRot(g_objArrowGlow, 0.0, 0.0, g_flPi, 0, 0);
    }
    else if (dir < 0)
    {
        AnimColor(g_objPrevArrow, 1.0, 1.0, 1.0, 1.0, fade_time, delay, ANIM_FUNC_LINEAR);
        AnimColor(g_objArrowGlow, 1.0, 1.0, 1.0, 1.0, fade_time, delay, ANIM_FUNC_LINEAR);
        GetPos(g_objPrevArrow, pos[0], pos[1], pos[2], 0);
        SetPos(g_objArrowGlow, pos[0], pos[1], pos[2], 0);
        SetRot(g_objArrowGlow, 0.0, 0.0, 0.0, 0, 0);
    }
    FadeLines(alpha, fade_time, delay);
    if (dir > 0)
    {
        GetColor(g_objNextArrow, color[0], color[1], color[2], color[3]);
        SetColor(g_objNextArrow, 0.6, 0.6, 0.6, color[3]);
    }
    if (dir < 0)
    {
        GetColor(g_objPrevArrow, color[0], color[1], color[2], color[3]);
        SetColor(g_objPrevArrow, 0.6, 0.6, 0.6, color[3]);
    }
    SetColor(g_objArrowGlow, 1.0, 1.0, 1.0, 0.0);
}

/* Slides the title in from the other side of dir and fades the lines to alpha. */
SlideInFolder(Float:dir, Float:alpha, Float:fade_time, Float:delay)
{
    new value[5];
    new Float:pos[3];
    new Float:color[4];
    get_group_tag_attr3("LIST", "HEAD", 0, "folder_x", value, 5);
    pos[0] = atof(value);
    get_group_tag_attr3("LIST", "HEAD", 0, "folder_y", value, 5);
    pos[1] = atof(value);
    get_group_tag_attr3("LIST", "HEAD", 0, "folder_z", value, 5);
    pos[2] = atof(value);
    GetColor(g_objTitle, color[0], color[1], color[2], color[3]);
    AnimColor(g_objTitle, color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_FASTER);
    SetPos(g_objTitle, pos[0] - 95.0 * dir, pos[1], pos[2], 0);
    AnimMoveLine(g_objTitle, pos[0], pos[1], pos[2], fade_time, 0.0, ANIM_FUNC_LINEAR);
    FadeLines(alpha, fade_time, delay);
}

MoveCursor(line)
{
    if (g_line != line)
    {
        g_line = line;
    }
    new Float:pos[3];
    GetPos(listGetAt(g_line), pos[0], pos[1], pos[2], 0);
    SetPos(g_objCursor, pos[0], pos[1], pos[2], 0);
}

/* Shows the cursor blinking, or hides it. */
SetBlink(on)
{
    Hide(g_objCursor);
    if (on)
    {
        g_blink = 1;
        g_bBlinkOn = 0;
        if (g_count)
        {
            Show(g_objCursor);
            new Float:color[4];
            GetColor(g_objCursor, color[0], color[1], color[2], color[3]);
            if (color[3])
            {
                SetColor(g_objCursor, color[0], color[1], color[2], 0.25);
            }
            AnimColor(g_objCursor, color[0], color[1], color[2], 0.7, g_flBlinkInTime, 0.0, ANIM_FUNC_SLOWER);
        }
    }
    else
    {
        g_blink = 0;
    }
}

/* Lights line_obj and dims the other lines, in step with a scroll in progress. */
HighlightLine(line_obj)
{
    if (g_count)
    {
        new Float:fade_time;
        if (ANIM_DONE != AnimGetState(g_objScrollTimer, ANIM_TIMER))
        {
            fade_time = float(400) * (1.0 - AnimDoTimer(g_objScrollTimer, ANIM_TIMER, 1.0));
        }
        else
        {
            fade_time = 1.0;
        }
        for (new i; i < listSize(); i++)
        {
            SendMessage(listGetAt(i), TAG("COL"), TAG("NOR"), fade_time, 0, 0);
        }
        SendMessage(line_obj, TAG("COL"), TAG("MAT"), fade_time, 0, 0);
    }
}

/* Takes the pad, with the pad help of the options when the folder has mail, or leaves it. */
SetActive(active)
{
    if (active)
    {
        if (g_count)
        {
            new label[16];
            get_tag_attr("TEXT", "msgmailer_btn_option", "value", label, 16);
            SetSystemParamStr(SYS_STR_PADHELP_RU, label);
            SetSystemParam(SYS_PADHELP, PADHELP_CIRCLE | PADHELP_CROSS | PADHELP_TRIANGLE);
        }
        else
        {
            SetSystemParam(SYS_PADHELP, PADHELP_CROSS);
        }
        g_active = 1;
    }
    else
    {
        g_active = 0;
    }
}

/* Sets the scroll bar to the visible part of the address book. */
UpdateScrollBar()
{
    if (g_count)
    {
        new Float:length = float(LIST_LINES) / float(g_count);
        length = length > 1.0 ? 1.0 : length;
        length = length < 0.0 ? 0.0 : length;
        length = length < 0.1 ? 0.1 : length;
        new Float:top = float(g_top) / float(g_count - LIST_LINES) * (1.0 - length);
        SetPubVar(g_objScrollBar, "top", top);
        if (g_count > LIST_LINES)
        {
            SendMessage(g_objScrollBar, TAG("FADE"), 1.0, float(400), 0, 0);
            SetPubVar(g_objScrollBar, "length", length);
        }
        else
        {
            SendMessage(g_objScrollBar, TAG("FADE"), 0, float(400), 0, 0);
            SetPubVar(g_objScrollBar, "length", 0);
        }
    }
    else
    {
        SetPubVar(g_objScrollBar, "top", 0);
        SetPubVar(g_objScrollBar, "length", 0);
        SendMessage(g_objScrollBar, TAG("FADE"), 0, float(400), 0, 0);
    }
}

/* Shows the unread count (inbox only) and the position of mail index, and keeps the top and the line of folder. */
UpdateCounter(folder, index)
{
    new text[8];
    if (FOLDER_INBOX == folder)
    {
        new unread_text[16];
        new unread_fmt[16];
        new unread;
        get_tag_attr("TEXT", "msgmailer_list_item_unread", "value", unread_text, 16);
        replace_all(unread_fmt, unread_text, "%MAILNO", "%d", 16);
        mx_CountUnread(folder, unread);
        sprintf(text, unread_fmt, unread);
        SetOptionS(g_objCounters[COUNTER_UNREAD], TAG("UTF8"), text);
        Show(g_objCounters[COUNTER_UNREAD]);
    }
    else
    {
        Hide(g_objCounters[COUNTER_UNREAD]);
    }
    if (!g_count)
    {
        sprintf(text, "%d/%d", index, g_count);
    }
    else
    {
        sprintf(text, "%d/%d", index + 1, g_count);
    }
    SetOptionS(g_objCounters[COUNTER_POSITION], TAG("UTF8"), text);
    g_iFolderTops[folder] = g_top;
    g_iFolderLines[folder] = g_line;
}

/* Fills the list with the current folder from top, with the cursor on line, and shows its title and icon. */
LoadFolder(top, line)
{
    g_top = top;
    update_message_list(g_folder);
    mx_CountMsg(g_folder, g_count);
    if (g_count - LIST_LINES < g_top)
    {
        if (--g_top < 0)
        {
            g_top = 0;
        }
    }
    if (g_count <= g_top + line)
    {
        if (--line < 0)
        {
            line = 0;
        }
    }
    for (new i; i < listSize(); i++)
    {
        SetLine(listGetAt(i), g_folder, g_top + i);
    }
    MoveCursor(line);
    UpdateScrollBar();
    UpdateCounter(g_folder, g_top + g_line);
    new tex;
    new title[16];
    if (FOLDER_INBOX == g_folder)
    {
        get_tag_attr("TEXT", "msgmailer_list_title_inbox", "value", title, 16);
        tex = GetPubVar(g_manager, "g_txBoxInbox");
        SetTexture(g_objFolderIcon, tex, 0, -1);
    }
    else if (g_folder == FOLDER_OUTBOX)
    {
        get_tag_attr("TEXT", "msgmailer_list_title_outbox", "value", title, 16);
        tex = GetPubVar(g_manager, "g_txBoxOutbox");
        SetTexture(g_objFolderIcon, tex, 0, -1);
    }
    else if (g_folder == FOLDER_DRAFT)
    {
        get_tag_attr("TEXT", "msgmailer_list_title_draft", "value", title, 16);
        tex = GetPubVar(g_manager, "g_txBoxDraft");
        SetTexture(g_objFolderIcon, tex, 0, -1);
    }
    else if (g_folder == FOLDER_QUEUE)
    {
        get_tag_attr("TEXT", "msgmailer_list_title_queue", "value", title, 16);
        tex = GetPubVar(g_manager, "g_txBoxQueue");
        SetTexture(g_objFolderIcon, tex, 0, -1);
    }
    SetOptionS(g_objTitle, TAG("UTF8"), title);
}

/*
 * Sends to line_obj the subject, the sender (the recipient out of the inbox), the date and the flags of its
 * icon ("R" unread, "L" locked, "C" marked) of mail index of folder; past the end of the folder, an empty line.
 */
SetLine(line_obj, folder, index)
{
    SystemCommand(SYSCMD_BUSY, 1);
    sleep;
    new text[246];
    new short_text[32];
    new exists;
    new unread;
    new partial; /* the "C" mark: a multipart mail, or a subject or sender cut to fit */
    new ret;
    if (index < g_count)
    {
        copy_string(short_text, "", 128);
        ret = mx_GetMsgSubject(folder, index, text, 984);
        SetFlagIf(partial, ret);
        copy_string(short_text, text, 32);
        short_text{127} = 0;
        SendMessageStr(line_obj, TAG("SUBJ"), short_text);
        copy_string(short_text, "", 128);
        if (FOLDER_INBOX == folder)
        {
            ret = mx_GetMsgFrom(folder, index, text, 984);
        }
        else
        {
            ret = mx_GetMsgTo(folder, index, text, 984);
        }
        SetFlagIf(partial, ret);
        copy_string(short_text, text, 32);
        short_text{127} = 0;
        SendMessageStr(line_obj, TAG("FROM"), short_text);
        mx_GetMsgDateYMD(folder, index, text, 100);
        SendMessageStr(line_obj, TAG("DATE"), text);
        {
            new unread_text[3];
            new lock_text[3];
            mx_GetMsgUnread(folder, index, unread_text, 10);
            if (FOLDER_INBOX != folder)
            {
                sprintf(unread_text, "%d", 0);
            }
            if (atoi(unread_text))
            {
                unread = 1;
            }
            mx_GetMsgLock(folder, index, lock_text, 10);
            if (!partial)
            {
                if (mx_IsMultiPart(folder, index))
                {
                    partial = 1;
                }
            }
            ret = 0;
            sprintf(text, "R%dL%dC%dT%d", atoi(unread_text), atoi(lock_text), partial, ret);
            SendMessageStr(line_obj, TAG("READ"), text);
        }
        exists = 1;
    }
    else
    {
        SendMessageStr(line_obj, TAG("SUBJ"), "");
        SendMessageStr(line_obj, TAG("FROM"), "");
        SendMessageStr(line_obj, TAG("DATE"), "");
        SendMessageStr(line_obj, TAG("READ"), !"*");
        exists = 0;
    }
    {
        new Float:color[4];
        new Float:bright = 1.0;
        if (!unread)
        {
            bright = 0.9;
        }
        if (!exists)
        {
            bright = 0.8;
        }
        GetColor(line_obj, color[0], color[1], color[2], color[3]);
        SetColor(line_obj, bright, bright, bright, color[3]);
    }
    SystemCommand(SYSCMD_BUSY, 0);
}

/* Sets flag when value is not 0. */
SetFlagIf(&flag, value)
{
    if (value)
    {
        flag = 1;
    }
}

HasRecipient(folder, index)
{
    new to[246];
    mx_GetMsgTo(folder, index, to, 984);
    if (strlen(to))
    {
        return 1;
    }
    return 0;
}

/* Gets the sender of a mail of the inbox, the recipient of a mail of the other folders. */
GetMsgPeer(folder, index, peer[])
{
    if (FOLDER_INBOX == folder)
    {
        mx_GetMsgFrom(folder, index, peer, 984);
    }
    else
    {
        mx_GetMsgTo(folder, index, peer, 984);
    }
}

/* The same, the address only. */
GetMsgPeerAddr(folder, index, addr[])
{
    if (FOLDER_INBOX == folder)
    {
        mx_GetMsgFromAddr(folder, index, addr, 984);
    }
    else
    {
        mx_GetMsgToAddr(folder, index, addr, 984);
    }
}

/* The same, the name only. */
GetMsgPeerName(folder, index, name[])
{
    if (FOLDER_INBOX == folder)
    {
        mx_GetMsgFromName(folder, index, name, 984);
    }
    else
    {
        mx_GetMsgToName(folder, index, name, 984);
    }
}

IsLocked(folder, index)
{
    new lock_text[3];
    mx_GetMsgLock(folder, index, lock_text, 10);
    return atoi(lock_text);
}

/* Deletes the mail at top + line of folder and puts the cursor on the next one (the previous one at the end). */
DeleteMsg(folder, top, line)
{
    new new_top;
    new new_line;
    if (g_count <= 1)
    {
        g_space = remove_message(folder, top + line);
        LoadFolder(top, line);
    }
    else if (g_count - 1 <= top + line)
    {
        new msg_no = mx_Line2MsgNum(folder, top + line - 1);
        g_space = remove_message(folder, top + line);
        new index = mx_MsgNum2Line(folder, msg_no);
        get_message_line(new_top, new_line, folder, index, 1, line);
        LoadFolder(new_top, new_line);
    }
    else
    {
        new msg_no = mx_Line2MsgNum(folder, top + line + 1);
        g_space = remove_message(folder, top + line);
        new index = mx_MsgNum2Line(folder, msg_no);
        get_message_line(new_top, new_line, folder, index, 1, line);
        LoadFolder(new_top, new_line);
    }
    for (new i; i < listSize(); i++)
    {
        FadeAlpha(listGetAt(i), 1.0, 1.0, 0.0);
    }
}
