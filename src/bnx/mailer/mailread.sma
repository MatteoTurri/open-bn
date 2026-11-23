/*
 * /opt0/bn/script/mailer/mailread.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The page of one mail, opened by main: its subject, its sender (the recipient out of the inbox), its date and its
 * body, scrolled with up, down, L1 and R1; the loupe doubles the text. It runs the actions of main on the mail.
 */

#include <core>
#include <bn>

#pragma dynamic 164000

#include "mailer.inc"

#include "wait.inc"

#include "tag.inc"

#include <float>

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

#include "text.inc"

#include "mailsync.inc"

new Float:g_flTextRed = 0.7843;
new Float:g_flTextGreen = 0.7843;
new Float:g_flTextBlue = 0.7843;
new Float:g_flDimAlpha = 0.2;
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
public g_focus; /* unused, but part of the layout */
public g_active; /* the page reads the pad */
public g_count; /* mails in the folder */
public g_blink; /* unused, but part of the layout */
public g_line; /* the mail is g_line + g_top of g_folder */
public g_top; /* the mail on the first row of the list */
public g_folder; /* FOLDER_* */
public g_space; /* the result of the last deletion: 0, ERR_NOSPACE or 1 */
new g_objBody;
new g_objDateBg;
new g_objFromBg;
new g_objBodyBg;
new g_objSubjectBg;
new g_objFrom;
new g_objSubject;
new g_objDate;
new g_objScrollBar;
new g_iLineCount; /* the lines of the body, with the room past the last one */
new g_iLineExtra;
new g_iScrollLine; /* the first line of the body shown */
new g_iFolderTops[4]; /* the top and the line of each folder */
new g_iFolderLines[4];
new g_iAddrIndex; /* the address book entry found by "ACHK" */
new g_iPageLines; /* the lines of the body shown at once */

main()
{
    new value[16];
    new Float:pos[3];
    new Float:color[3];
    new Float:width;
    new Float:height;
    new Float:bar_h;
    if (GetArg("folder", value, 16))
    {
        g_folder = atoi(value);
    }
    if (GetArg("line0", value, 16))
    {
        g_iFolderLines[FOLDER_INBOX] = atoi(value);
    }
    if (GetArg("top0", value, 16))
    {
        g_iFolderTops[FOLDER_INBOX] = atoi(value);
    }
    if (GetArg("line1", value, 16))
    {
        g_iFolderLines[FOLDER_OUTBOX] = atoi(value);
    }
    if (GetArg("top1", value, 16))
    {
        g_iFolderTops[FOLDER_OUTBOX] = atoi(value);
    }
    if (GetArg("line2", value, 16))
    {
        g_iFolderLines[FOLDER_DRAFT] = atoi(value);
    }
    if (GetArg("top2", value, 16))
    {
        g_iFolderTops[FOLDER_DRAFT] = atoi(value);
    }
    if (GetArg("line3", value, 16))
    {
        g_iFolderLines[FOLDER_QUEUE] = atoi(value);
    }
    if (GetArg("top3", value, 16))
    {
        g_iFolderTops[FOLDER_QUEUE] = atoi(value);
    }
    g_line = g_iFolderLines[g_folder];
    g_top = g_iFolderTops[g_folder];
    g_iPageLines = 12;
    mx_SetMsgStatus(g_folder, g_top + g_line, MSG_READ);
    new subject_tex = GetPubVar(g_manager, "g_txTitleBack");
    get_group_tag_attr2("READ", "HEAD", 0, "title_bg_x", value, 16);
    pos[0] = atof(value);
    get_group_tag_attr2("READ", "HEAD", 0, "title_bg_y", value, 16);
    pos[1] = atof(value);
    get_group_tag_attr2("READ", "HEAD", 0, "title_bg_z", value, 16);
    pos[2] = atof(value);
    width = GetPubVar(g_manager, "g_TitleBackWidth");
    height = GetPubVar(g_manager, "g_TitleBackHeight");
    g_objSubjectBg = CreatePlane(Self());
    SetPos(g_objSubjectBg, pos[0], pos[1], pos[2], 0);
    SetScale(g_objSubjectBg, width, height, 0.0);
    SetColor(g_objSubjectBg, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_objSubjectBg, TAG("ZSRT"), ZSORT_OVERLAY);
    SetTexture(g_objSubjectBg, subject_tex, 0, -1);
    new from_tex;
    if (FOLDER_INBOX == g_folder)
    {
        from_tex = GetPubVar(g_manager, "g_txFromBack");
    }
    else
    {
        from_tex = GetPubVar(g_manager, "g_txToBack");
    }
    get_group_tag_attr2("READ", "HEAD", 0, "from_bg_x", value, 16);
    pos[0] = atof(value);
    get_group_tag_attr2("READ", "HEAD", 0, "from_bg_y", value, 16);
    pos[1] = atof(value);
    get_group_tag_attr2("READ", "HEAD", 0, "from_bg_z", value, 16);
    pos[2] = atof(value);
    width = GetPubVar(g_manager, "g_FromBackWidth");
    height = GetPubVar(g_manager, "g_FromBackHeight");
    g_objFromBg = CreatePlane(Self());
    SetPos(g_objFromBg, pos[0], pos[1], pos[2], 0);
    SetScale(g_objFromBg, width, height, 0.0);
    SetColor(g_objFromBg, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_objFromBg, TAG("ZSRT"), ZSORT_OVERLAY);
    SetTexture(g_objFromBg, from_tex, 0, -1);
    new date_tex = GetPubVar(g_manager, "g_txDateBack");
    get_group_tag_attr2("READ", "HEAD", 0, "date_bg_x", value, 16);
    pos[0] = atof(value);
    get_group_tag_attr2("READ", "HEAD", 0, "date_bg_y", value, 16);
    pos[1] = atof(value);
    get_group_tag_attr2("READ", "HEAD", 0, "date_bg_z", value, 16);
    pos[2] = atof(value);
    width = GetPubVar(g_manager, "g_DateBackWidth");
    height = GetPubVar(g_manager, "g_DateBackHeight");
    g_objDateBg = CreatePlane(Self());
    SetPos(g_objDateBg, pos[0], pos[1], pos[2], 0);
    SetScale(g_objDateBg, width, height, 0.0);
    SetColor(g_objDateBg, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_objDateBg, TAG("ZSRT"), ZSORT_OVERLAY);
    SetTexture(g_objDateBg, date_tex, 0, -1);
    get_group_tag_attr2("READ", "HEAD", 0, "body_bg_x", value, 16);
    pos[0] = atof(value);
    get_group_tag_attr2("READ", "HEAD", 0, "body_bg_y", value, 16);
    pos[1] = atof(value);
    get_group_tag_attr2("READ", "HEAD", 0, "body_bg_z", value, 16);
    pos[2] = atof(value);
    get_group_tag_attr2("READ", "HEAD", 0, "body_bg_r", value, 16);
    color[0] = atof(value);
    get_group_tag_attr2("READ", "HEAD", 0, "body_bg_g", value, 16);
    color[1] = atof(value);
    get_group_tag_attr2("READ", "HEAD", 0, "body_bg_b", value, 16);
    color[2] = atof(value);
    get_group_tag_attr2("READ", "HEAD", 0, "body_bg_w", value, 16);
    width = atof(value);
    get_group_tag_attr2("READ", "HEAD", 0, "body_bg_h", value, 16);
    height = atof(value);
    g_objBodyBg = CreatePlane(Self());
    SetPos(g_objBodyBg, pos[0], pos[1], pos[2], 0);
    SetScale(g_objBodyBg, width, height, 0.0);
    SetColor(g_objBodyBg, color[0], color[1], color[2], 0.0);
    get_group_tag_attr2("READ", "HEAD", 0, "title_text_x", value, 16);
    pos[0] = atof(value);
    get_group_tag_attr2("READ", "HEAD", 0, "title_text_y", value, 16);
    pos[1] = atof(value);
    get_group_tag_attr2("READ", "HEAD", 0, "title_text_z", value, 16);
    pos[2] = atof(value);
    g_objSubject = CreateLabel(Self());
    SetPos(g_objSubject, pos[0], pos[1], pos[2], 0);
    SetColor(g_objSubject, 0.71, 0.71, 0.71, 0.0);
    SetOptionI(g_objSubject, TAG("HALN"), ALIGN_LEFT);
    SetOptionI(g_objSubject, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_objSubject, TAG("FSIZ"), 15);
    SetOptionI(g_objSubject, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(g_objSubject, TAG("W64 "), floatround(566.0 - 70.0 - 5.0, 0) * 64);
    get_group_tag_attr2("READ", "HEAD", 0, "from_text_x", value, 16);
    pos[0] = atof(value);
    get_group_tag_attr2("READ", "HEAD", 0, "from_text_y", value, 16);
    pos[1] = atof(value);
    get_group_tag_attr2("READ", "HEAD", 0, "from_text_z", value, 16);
    pos[2] = atof(value);
    g_objFrom = CreateLabel(Self());
    SetPos(g_objFrom, pos[0], pos[1], pos[2], 0);
    SetColor(g_objFrom, 0.71, 0.71, 0.71, 0.0);
    SetOptionI(g_objFrom, TAG("HALN"), ALIGN_LEFT);
    SetOptionI(g_objFrom, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_objFrom, TAG("FSIZ"), 15);
    SetOptionI(g_objFrom, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(g_objFrom, TAG("W64 "), floatround(391.0 - 70.0 - 5.0, 0) * 64);
    get_group_tag_attr2("READ", "HEAD", 0, "date_text_x", value, 16);
    pos[0] = atof(value);
    get_group_tag_attr2("READ", "HEAD", 0, "date_text_y", value, 16);
    pos[1] = atof(value);
    get_group_tag_attr2("READ", "HEAD", 0, "date_text_z", value, 16);
    pos[2] = atof(value);
    g_objDate = CreateLabel(Self());
    SetPos(g_objDate, pos[0], pos[1], pos[2], 0);
    SetColor(g_objDate, g_flTextRed, g_flTextGreen, g_flTextBlue, 0.0);
    SetOptionI(g_objDate, TAG("HALN"), ALIGN_LEFT);
    SetOptionI(g_objDate, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_objDate, TAG("FSIZ"), 14);
    SetOptionI(g_objDate, TAG("ZSRT"), ZSORT_OVERLAY);
    get_group_tag_attr2("READ", "HEAD", 0, "body_text_x", value, 16);
    pos[0] = atof(value);
    get_group_tag_attr2("READ", "HEAD", 0, "body_text_y", value, 16);
    pos[1] = atof(value);
    get_group_tag_attr2("READ", "HEAD", 0, "body_text_z", value, 16);
    pos[2] = atof(value);
    g_objBody = CreateEditML_P(Self());
    SetPos(g_objBody, pos[0], pos[1], pos[2], 0);
    SetColor(g_objBody, 0.71, 0.71, 0.71, 0.0);
    SetOptionF(g_objBody, TAG("ZOFS"), 2.0);
    SetOptionI(g_objBody, TAG("FSIZ"), 15);
    SetOptionI(g_objBody, TAG("SDWE"), 0);
    SetOptionI(g_objBody, TAG("CRSW"), 0);
    SetOptionI(g_objBody, TAG("EDTE"), 0);
    SetOptionI(g_objBody, TAG("LSPC"), 3);
    SetOptionI(g_objBody, TAG("W64 "), 548 * 64);
    SetOptionI(g_objBody, TAG("H64 "), 230 * 64);
    {
        new packed[4];
        static bg_color[16];
        pack_color_bytes(packed, bg_color);
        SetOptionP(g_objBody, TAG("BGC4"), packed, 4);
        static text_color[16] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
        pack_color_bytes(packed, text_color);
        SetOptionP(g_objBody, TAG("COLR"), packed, 4);
    }
    get_group_tag_attr2("READ", "HEAD", 0, "bar_x", value, 16);
    pos[0] = atof(value);
    get_group_tag_attr2("READ", "HEAD", 0, "bar_y", value, 16);
    pos[1] = atof(value);
    get_group_tag_attr2("READ", "HEAD", 0, "bar_z", value, 16);
    pos[2] = atof(value);
    get_group_tag_attr2("READ", "HEAD", 0, "bar_h", value, 16);
    bar_h = atof(value);
    g_objScrollBar = CreateNull(Self());
    SetPos(g_objScrollBar, pos[0], pos[1], pos[2], 0);
    SetOptionI(g_objScrollBar, TAG("ZSRT"), ZSORT_OVERLAY);
    AttachScript(g_objScrollBar, _, "scroll_bar", "");
    SetPubVar(g_objScrollBar, "manager", Self());
    SetPubVar(g_objScrollBar, "focus", Self());
    SetPubVar(g_objScrollBar, "height", bar_h);
    SendMessage(g_objScrollBar, TAG("FADE"), 0, 0, 0, 0);
    ShowMail(g_folder, g_top + g_line);
    sleep;
    SendMessage(Self(), TAG("FADE"), TAG("IN"), 0, 0, 0);
    SendMessage(g_manager, TAG("SHWT"), 0, 0, 0, 0);
    for (; ; )
    {
        if (g_iLineCount > g_iPageLines + 1)
        {
            new held;
            new pressed;
            new released;
            new repeated;
            if (g_active)
            {
                PadState(held, pressed, released, repeated, -1);
            }
            static last_tick;
            if (pressed & PAD_L1 || (repeated & PAD_L1 && tickcount() - last_tick > 70))
            {
                last_tick = tickcount();
                SetOptionI(g_objBody, TAG("SCLY"), -g_iPageLines);
                if (g_iScrollLine > 0)
                {
                    sndSePlay(SE_CURSOR);
                    g_iScrollLine = g_iScrollLine - g_iPageLines;
                    if (g_iScrollLine < 0)
                    {
                        g_iScrollLine = 0;
                    }
                    UpdateScrollBar();
                }
            }
            else if (pressed & PAD_R1 || (repeated & PAD_R1 && tickcount() - last_tick > 70))
            {
                last_tick = tickcount();
                SetOptionI(g_objBody, TAG("SCLY"), g_iPageLines);
                if (g_iScrollLine <= g_iLineCount - g_iPageLines - 1)
                {
                    sndSePlay(SE_CURSOR);
                    g_iScrollLine = g_iScrollLine + g_iPageLines;
                    if (g_iScrollLine > g_iLineCount - g_iPageLines - 1)
                    {
                        g_iScrollLine = g_iLineCount - g_iPageLines;
                    }
                    UpdateScrollBar();
                }
            }
            else if (pressed & PAD_UP || (repeated & PAD_UP && tickcount() - last_tick > 70))
            {
                last_tick = tickcount();
                SetOptionI(g_objBody, TAG("SCLY"), -1);
                if (g_iScrollLine > 0)
                {
                    sndSePlay(SE_CURSOR);
                    g_iScrollLine--;
                    UpdateScrollBar();
                }
            }
            else if (pressed & PAD_DOWN || (repeated & PAD_DOWN && tickcount() - last_tick > 70))
            {
                last_tick = tickcount();
                SetOptionI(g_objBody, TAG("SCLY"), 1);
                if (g_iScrollLine <= g_iLineCount - g_iPageLines - 1)
                {
                    sndSePlay(SE_CURSOR);
                    g_iScrollLine++;
                    UpdateScrollBar();
                }
            }
        }
        new msg;
        new what;
        new arg;
        /*
         * The actions of main on the mail ("ACT"), each answered with "ACWT": "ADDR" adds its address to the address
         * book, "AEXT" over the entry found by "ACHK" (which answers that entry and whether the address or the name is
         * too long), "LOCK" toggles its lock, "LOUP" the loupe, "DELE" deletes it, "SEND" and "SDCK" answer its line
         * and whether it has a recipient, "LINE" sends the folder and the lines of every folder first.
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
            else if (msg == TAG("BLNK"))
            {
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
            else if (msg == TAG("ACT"))
            {
                if (what == TAG("ADDR"))
                {
                    new peer[246];
                    GetMsgPeer(g_folder, g_top + g_line, peer);
                    mx_SetAddr(-1, peer);
                    SendMessage(g_manager, TAG("ACWT"), 0, 0, 0, 0);
                }
                else if (what == TAG("AEXT"))
                {
                    new peer[246];
                    GetMsgPeer(g_folder, g_top + g_line, peer);
                    mx_SetAddr(g_iAddrIndex, peer);
                    SendMessage(g_manager, TAG("ACWT"), 0, 0, 0, 0);
                }
                else if (what == TAG("ACHK"))
                {
                    new addr[246];
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
                    SendMessage(g_manager, TAG("ACWT"), 0, 0, 0, 0);
                }
                else if (what == TAG("LOUP"))
                {
                    ToggleLoupe();
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
                else if (what == TAG("DELE"))
                {
                    DeleteMsg(g_folder, g_top, g_line);
                    SendMessage(g_manager, TAG("ACWT"), 0, 0, 0, 0);
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
        sleep;
    }
    return;
}

/* Fades the page to alpha (the background of the body to alpha times its "body_bg_a"). */
FadeAll(Float:alpha, Float:fade_time, Float:delay)
{
    new Float:color[4];
    GetColor(g_objDateBg, color[0], color[1], color[2], color[3]);
    AnimColor(g_objDateBg, color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objFromBg, color[0], color[1], color[2], color[3]);
    AnimColor(g_objFromBg, color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objSubjectBg, color[0], color[1], color[2], color[3]);
    AnimColor(g_objSubjectBg, color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objFrom, color[0], color[1], color[2], color[3]);
    AnimColor(g_objFrom, color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objSubject, color[0], color[1], color[2], color[3]);
    AnimColor(g_objSubject, color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objDate, color[0], color[1], color[2], color[3]);
    AnimColor(g_objDate, color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objBody, color[0], color[1], color[2], color[3]);
    AnimColor(g_objBody, color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    if (g_iLineCount > g_iPageLines + 1)
    {
        SendMessage(g_objScrollBar, TAG("FADE"), alpha, fade_time, delay, 0);
    }
    new bg_alpha[10];
    get_group_tag_attr2("READ", "HEAD", 0, "body_bg_a", bg_alpha, 10);
    GetColor(g_objBodyBg, color[0], color[1], color[2], color[3]);
    AnimColor(g_objBodyBg, color[0], color[1], color[2], alpha * atof(bg_alpha), fade_time, delay, ANIM_FUNC_LINEAR);
}

FadeIn()
{
    FadeAll(1.0, float(400), 0.0);
}

FadeOut()
{
    FadeAll(0.0, float(400), 0.0);
}

FadeDim()
{
    FadeAll(g_flDimAlpha, float(400), 0.0);
}

/* Sets the scroll bar to the part of the body shown. */
UpdateScrollBar()
{
    if (g_iLineCount > g_iPageLines + 1)
    {
        new Float:length = float(g_iPageLines) / float(g_iLineCount + g_iLineExtra);
        length = length > 1.0 ? 1.0 : length;
        length = length < 0.0 ? 0.0 : length;
        length = length < 0.1 ? 0.1 : length;
        new Float:top = float(g_iScrollLine) / float(g_iLineCount + g_iLineExtra - g_iPageLines) * (1.0 - length);
        SetPubVar(g_objScrollBar, "top", top);
        SetPubVar(g_objScrollBar, "length", length);
    }
    else
    {
        SetPubVar(g_objScrollBar, "top", 0);
        SetPubVar(g_objScrollBar, "length", 0);
    }
}

/* Takes the pad, with the pad help of the options, or leaves it. */
SetActive(active)
{
    if (active)
    {
        new label[16];
        get_tag_attr("TEXT", "msgmailer_btn_option", "value", label, 16);
        SetSystemParamStr(SYS_STR_PADHELP_RU, label);
        SetSystemParam(SYS_PADHELP, PADHELP_CROSS | PADHELP_TRIANGLE);
        g_active = 1;
    }
    else
    {
        g_active = 0;
    }
}

new g_bLoupe;

/* Switches the body between the normal and the double size of the text, fading it out and in. */
ToggleLoupe()
{
    SetActive(1);
    new Float:color[4];
    GetColor(g_objBody, color[0], color[1], color[2], color[3]);
    AnimColor(g_objBody, color[0], color[1], color[2], 0.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
    if (g_iLineCount > g_iPageLines + 1 && g_bLoupe && g_iLineCount / 2 <= 12)
    {
        SendMessage(g_objScrollBar, TAG("FADE"), 0, 200.0, 0, 0);
    }
    for (; ; )
    {
        sleep;
        if (ANIM_DONE == AnimGetState(g_objBody, ANIM_COLOR))
        {
            break;
        }
    }
    {
        new spacing;
        if (!g_bLoupe)
        {
            GetOptionI(g_objBody, TAG("LSPC"), spacing);
            SetOptionI(g_objBody, TAG("FSIZ"), 30);
            SetOptionI(g_objBody, TAG("LSPC"), spacing * 2);
            g_iPageLines = 6;
            g_bLoupe = 1;
        }
        else
        {
            GetOptionI(g_objBody, TAG("LSPC"), spacing);
            SetOptionI(g_objBody, TAG("FSIZ"), 15);
            SetOptionI(g_objBody, TAG("LSPC"), spacing / 2);
            g_iPageLines = 12;
            g_bLoupe = 0;
        }
        CountLines();
        UpdateScrollBar();
        wait_ms(150);
    }
    AnimColor(g_objBody, color[0], color[1], color[2], 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
    if (g_iLineCount > g_iPageLines + 1 && g_bLoupe)
    {
        SendMessage(g_objScrollBar, TAG("FADE"), 1.0, 200.0, 0, 0);
    }
    for (; ; )
    {
        sleep;
        if (ANIM_DONE == AnimGetState(g_objBody, ANIM_COLOR))
        {
            break;
        }
    }
    SetActive(0);
}

/* Shows mail index of folder. */
ShowMail(folder, index)
{
    new peer[246];
    new subject[246];
    new body[131072];
    mx_CountMsg(g_folder, g_count);
    mx_GetMsgSubject(folder, index, subject, 984);
    SetOptionS(g_objSubject, TAG("UTF8"), subject);
    GetMsgPeer(folder, index, peer);
    SetOptionS(g_objFrom, TAG("UTF8"), peer);
    new date_text[25];
    mx_GetMsgDateYMDHM(folder, index, date_text, 100);
    SetOptionS(g_objDate, TAG("UTF8"), date_text);
    if (strlen(date_text) > 14)
    {
        SetScale(g_objDate, 0.823, 1.0, 1.0);
    }
    read_message_body(folder, index, body, 524288);
    SetOptionS(g_objBody, TAG("UTF8"), body);
    CountLines();
    UpdateScrollBar();
}

/* Scrolls the body to its top and counts its lines, with a page of room past the last one. */
CountLines()
{
    g_iScrollLine = g_iLineExtra = 0;
    SetOptionI(g_objBody, TAG("CPDY"), 0);
    GetOptionI(g_objBody, TAG("CPDM"), g_iLineCount);
    if (g_iLineCount > g_iPageLines + 1)
    {
        g_iLineCount = g_iLineCount + g_iPageLines - 1;
    }
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

/* Like GetMsgPeer, the address only. */
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

/* Deletes the mail at top + line of folder; the line of the folder moves to the next mail (the previous at the end). */
DeleteMsg(folder, top, line)
{
    new new_top;
    new new_line;
    if (g_count <= 1)
    {
        g_space = remove_message(folder, top + line);
        g_iFolderTops[folder] = g_top = new_top;
        g_iFolderLines[folder] = g_line = new_line;
    }
    else if (g_count - 1 <= top + line)
    {
        new msg_no = mx_Line2MsgNum(folder, top + line - 1);
        g_space = remove_message(folder, top + line);
        new index = mx_MsgNum2Line(folder, msg_no);
        get_message_line(new_top, new_line, folder, index, 1, line);
        g_iFolderTops[folder] = g_top = new_top;
        g_iFolderLines[folder] = g_line = new_line;
    }
    else
    {
        new msg_no = mx_Line2MsgNum(folder, top + line + 1);
        g_space = remove_message(folder, top + line);
        new index = mx_MsgNum2Line(folder, msg_no);
        get_message_line(new_top, new_line, folder, index, 1, line);
        g_iFolderTops[folder] = g_top = new_top;
        g_iFolderLines[folder] = g_line = new_line;
    }
}
