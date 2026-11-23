/*
 * /opt0/bn/script/mailer/mailstedit.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The sort editor of the mailer: the sort key and the order of one folder (chosen by "mline" in the sort
 * menu), changed with up and down in two columns; "folder" and "line0" to "top3" are the list position
 * of each folder, which the sort updates.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "mailer.inc"

#include "wait.inc"

#include "tag.inc"

#include <float>

new Float:g_flPi = 3.141592;

/* The arrows of g_objArrows and g_iArrowBlinks. */
const ARROW_UP = 0;   /* above the selected column */
const ARROW_DOWN = 1; /* below the selected column */

/* The columns of g_iColumn, g_iChoices and g_objLabels. */
const COLUMN_KEY = 0;
const COLUMN_ORDER = 1;

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

#include "text.inc"

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

#include "mailsync.inc"

new Float:g_flTitleRed = 0.902;
new Float:g_flTitleGreen = 0.902;
new Float:g_flTitleBlue = 0.215;
new Float:g_flLabelRed = 0.215;
new Float:g_flLabelGreen = 0.708;
new Float:g_flLabelBlue = 0.999;
new Float:g_flDimAlpha = 0.2;
new g_iItemCount;
new g_szAnyId[1]; /* an empty id: the first element with the tag */
new g_szItemId[64];

/* Fills ids with the ids of the empty leaves of the first tag element; returns their number. */
GetItemIds(tag[], ids[][])
{
    g_iItemCount = 0;
    new node = domFindTag(domRoot(), tag, g_szAnyId, 0);
    CollectItemIds(node, ids);
    return g_iItemCount;
}

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

/* Returns the index of item id among the empty leaves of the first tag element, 0 when it is not there. */
GetItemIndex(tag[], id[])
{
    g_iItemCount = 0;
    g_iItemIndex = 0;
    new node = domFindTag(domRoot(), tag, g_szAnyId, 0);
    FindItemIndex(node, id);
    return g_iItemIndex;
}

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

public g_manager; /* the sort page: gets "SHWT" when ready and "ACWT" after "ACT" */
public g_focus; /* the focus object, moved onto the selected column */
public g_active; /* 1 while the editor has the focus and takes the pad */
public g_blink; /* unused, but part of the layout */
public g_space; /* the result of the last sort: 0, ERR_NOSPACE or 1 */
new g_objTitle;
new g_objLabels[2];
new g_objArrows[2]; /* above and below the selected column */
new g_szKeyIds[3][16];
new g_szOrderIds[2][16];
new g_iFolder; /* the folder sorted */
new g_iColumn; /* 0 the key, 1 the order */
new g_iChoices[2]; /* the choice of each column, an index into g_szKeyIds and g_szOrderIds */
new g_iTops[4]; /* the first line shown of each folder */
new g_iRows[4]; /* the cursor row of each folder */
new g_iArrowBlinks[2]; /* frames left of the blink of an arrow after a step */

main()
{
    new Float:pos[3];
    new Float:line_w;
    new key;
    new order;
    new choice_count;
    new menu_line;
    new list_folder;
    new text[32];
    if (GetArg("mline", text, 32))
    {
        menu_line = atoi(text);
    }
    if (GetArg("folder", text, 32))
    {
        list_folder = atoi(text);
    }
    if (GetArg("line0", text, 32))
    {
        g_iRows[FOLDER_INBOX] = atoi(text);
    }
    if (GetArg("top0", text, 32))
    {
        g_iTops[FOLDER_INBOX] = atoi(text);
    }
    if (GetArg("line1", text, 32))
    {
        g_iRows[FOLDER_OUTBOX] = atoi(text);
    }
    if (GetArg("top1", text, 32))
    {
        g_iTops[FOLDER_OUTBOX] = atoi(text);
    }
    if (GetArg("line2", text, 32))
    {
        g_iRows[FOLDER_DRAFT] = atoi(text);
    }
    if (GetArg("top2", text, 32))
    {
        g_iTops[FOLDER_DRAFT] = atoi(text);
    }
    if (GetArg("line3", text, 32))
    {
        g_iRows[FOLDER_QUEUE] = atoi(text);
    }
    if (GetArg("top3", text, 32))
    {
        g_iTops[FOLDER_QUEUE] = atoi(text);
    }
    g_iColumn = COLUMN_KEY;
    choice_count = 3;
    get_group_tag_attr2("STEDIT", "HEAD", 0, "folder_x", text, 32);
    pos[0] = atof(text);
    get_group_tag_attr2("STEDIT", "HEAD", 0, "folder_y", text, 32);
    pos[1] = atof(text);
    get_group_tag_attr2("STEDIT", "HEAD", 0, "folder_z", text, 32);
    pos[2] = atof(text);
    {
        new key_tag[8];
        new folder_ids[4][16];
        GetItemIds("SORT_TOP_ITEM", folder_ids);
        if (!strcmp(folder_ids[menu_line], "MAILER_SORT_INBOX"))
        {
            mx_GetMsgOrder(FOLDER_INBOX, key, order);
            g_iFolder = FOLDER_INBOX;
            get_tag_attr2("TEXT", "msgmailer_sort_item_inbox", "value", text, 32);
            copy_string(key_tag, "SORT_EDIT_INBOX_ITEM", 8);
        }
        else if (!strcmp(folder_ids[menu_line], "MAILER_SORT_OUTBOX"))
        {
            mx_GetMsgOrder(FOLDER_OUTBOX, key, order);
            g_iFolder = FOLDER_OUTBOX;
            get_tag_attr2("TEXT", "msgmailer_sort_item_outbox", "value", text, 32);
            copy_string(key_tag, "SORT_EDIT_OUTBOX_ITEM", 8);
        }
        else if (!strcmp(folder_ids[menu_line], "MAILER_SORT_DRAFT"))
        {
            mx_GetMsgOrder(FOLDER_DRAFT, key, order);
            g_iFolder = FOLDER_DRAFT;
            get_tag_attr2("TEXT", "msgmailer_sort_item_draft", "value", text, 32);
            copy_string(key_tag, "SORT_EDIT_DRAFT_ITEM", 8);
        }
        else if (!strcmp(folder_ids[menu_line], "MAILER_SORT_QUEUE"))
        {
            mx_GetMsgOrder(FOLDER_QUEUE, key, order);
            g_iFolder = FOLDER_QUEUE;
            get_tag_attr2("TEXT", "msgmailer_sort_item_queue", "value", text, 32);
            copy_string(key_tag, "SORT_EDIT_QUEUE_ITEM", 8);
        }
        InitChoices(key_tag, key, order);
        GetItemIds(key_tag, g_szKeyIds);
        GetItemIds("SORT_UPDOWN_ITEM", g_szOrderIds);
    }
    g_objTitle = CreateLabel(Self());
    SetColor(g_objTitle, g_flTitleRed, g_flTitleGreen, g_flTitleBlue, 0.0);
    SetPos(g_objTitle, pos[0], pos[1], pos[2], 0);
    SetOptionS(g_objTitle, TAG("UTF8"), text);
    SetOptionI(g_objTitle, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(g_objTitle, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_objTitle, TAG("FSIZ"), 17);
    SetOptionI(g_objTitle, TAG("ZSRT"), ZSORT_OVERLAY);
    get_group_tag_attr2("STEDIT", "HEAD", 0, "info_x", text, 32);
    pos[0] = atof(text);
    get_group_tag_attr2("STEDIT", "HEAD", 0, "info_y", text, 32);
    pos[1] = atof(text);
    get_group_tag_attr2("STEDIT", "HEAD", 0, "info_z", text, 32);
    pos[2] = atof(text);
    get_group_tag_attr2("STEDIT", "HEAD", 0, "line_width", text, 32);
    line_w = atof(text);
    for (new i; i < 2; i++)
    {
        g_objLabels[i] = CreateLabel(Self());
        SetPos(g_objLabels[i], pos[0] + line_w * i - line_w / 2, pos[1], pos[2], 0);
        SetColor(g_objLabels[i], g_flLabelRed, g_flLabelGreen, g_flLabelBlue, 0.0);
        SetOptionI(g_objLabels[i], TAG("HALN"), ALIGN_CENTER);
        SetOptionI(g_objLabels[i], TAG("VALN"), ALIGN_CENTER);
        SetOptionI(g_objLabels[i], TAG("FSIZ"), 15);
        SetOptionI(g_objLabels[i], TAG("ZSRT"), ZSORT_OVERLAY);
    }
    get_group_tag_attr2("STEDIT", "HEAD", 0, "info_x", text, 32);
    pos[0] = atof(text);
    get_group_tag_attr2("STEDIT", "HEAD", 0, "info_y", text, 32);
    pos[1] = atof(text);
    get_group_tag_attr2("STEDIT", "HEAD", 0, "info_z", text, 32);
    pos[2] = atof(text);
    get_group_tag_attr2("STEDIT", "HEAD", 0, "line_width", text, 32);
    line_w = atof(text);
    g_objArrows[ARROW_UP] = CreatePlane(Self());
    SetPos(g_objArrows[ARROW_UP], pos[0] + line_w * g_iColumn - line_w / 2, pos[1] - 14.0, pos[2], 0);
    SetScale(g_objArrows[ARROW_UP], 17.0, 8.0, 1.0);
    SetColor(g_objArrows[ARROW_UP], 1.0, 1.0, 1.0, 0.0);
    SetRot(g_objArrows[ARROW_UP], 0.0, 0.0, g_flPi, 0, 0);
    SetTexture(g_objArrows[ARROW_UP], imgGetResource(RES_SPIN), 0, -1);
    SetOptionI(g_objArrows[ARROW_UP], TAG("ZSRT"), ZSORT_OVERLAY);
    g_objArrows[ARROW_DOWN] = CreatePlane(Self());
    SetPos(g_objArrows[ARROW_DOWN], pos[0] + line_w * g_iColumn - line_w / 2, 14.0 + pos[1], pos[2], 0);
    SetScale(g_objArrows[ARROW_DOWN], 17.0, 8.0, 1.0);
    SetColor(g_objArrows[ARROW_DOWN], 1.0, 1.0, 1.0, 0.0);
    SetTexture(g_objArrows[ARROW_DOWN], imgGetResource(RES_SPIN), 0, -1);
    SetOptionI(g_objArrows[ARROW_DOWN], TAG("ZSRT"), ZSORT_OVERLAY);
    ShowChoices();
    MoveArrows();
    sleep;
    SendMessage(Self(), TAG("FADE"), TAG("IN"), 0, 0, 0);
    SendMessage(Self(), TAG("FOCS"), TAG("IN"), 0, 0, 0);
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
        /* the focus must move to the selected column */
        static refocus;
        if (repeated & PAD_UP && g_iChoices[g_iColumn] - 1 >= 0)
        {
            sndSePlay(SE_CURSOR);
            g_iChoices[g_iColumn]--;
            ShowChoices();
            BlinkArrow(ARROW_UP);
        }
        else if (repeated & PAD_DOWN && g_iChoices[g_iColumn] + 1 < choice_count)
        {
            sndSePlay(SE_CURSOR);
            g_iChoices[g_iColumn]++;
            ShowChoices();
            BlinkArrow(ARROW_DOWN);
        }
        else if (pressed & PAD_LEFT && g_iColumn)
        {
            sndSePlay(SE_CURSOR);
            g_iColumn = COLUMN_KEY;
            choice_count = 3;
            MoveArrows();
            refocus = 1;
        }
        else if (pressed & PAD_RIGHT && !g_iColumn)
        {
            sndSePlay(SE_CURSOR);
            g_iColumn = COLUMN_ORDER;
            choice_count = 2;
            MoveArrows();
            refocus = 1;
        }
        if (g_active)
        {
            UpdateArrowBlinks();
            if (refocus)
            {
                send_label_focus2(g_focus, g_objLabels[g_iColumn], 0, 0.5, 60.0, 60.0, 70.0, 1, 1);
                refocus = 0;
            }
        }
        new msg;
        new what;
        new arg;
        /*
         * "FOCS" IN/OUT, "FADE" IN/OUT/DIM, and "ACT", answered with "ACWT": "SSET" sorts the folder, "LINE" reports
         * the folder ("FOL") and the first line and cursor row of each folder ("L0" to "L3").
         */
        while (RecvMessage(msg, what, arg))
        {
            if (msg == TAG("FOCS"))
            {
                if (what == TAG("IN"))
                {
                    SetSystemParam(SYS_PADHELP, PADHELP_CIRCLE | PADHELP_CROSS);
                    g_active = 1;
                    refocus = 1;
                }
                else if (what == TAG("OUT"))
                {
                    g_active = 0;
                    send_focus(g_focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
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
            if (msg == TAG("ACT"))
            {
                if (what == TAG("SSET"))
                {
                    ApplySort();
                    SendMessage(g_manager, TAG("ACWT"), 0, 0, 0, 0);
                }
                else if (what == TAG("LINE"))
                {
                    SendMessage(g_manager, TAG("FOL"), list_folder, 0, 0, 0);
                    SendMessage(g_manager, TAG("L0"), g_iTops[FOLDER_INBOX], g_iRows[FOLDER_INBOX], 0, 0);
                    SendMessage(g_manager, TAG("L1"), g_iTops[FOLDER_OUTBOX], g_iRows[FOLDER_OUTBOX], 0, 0);
                    SendMessage(g_manager, TAG("L2"), g_iTops[FOLDER_DRAFT], g_iRows[FOLDER_DRAFT], 0, 0);
                    SendMessage(g_manager, TAG("L3"), g_iTops[FOLDER_QUEUE], g_iRows[FOLDER_QUEUE], 0, 0);
                    SendMessage(g_manager, TAG("ACWT"), 0, 0, 0, 0);
                }
            }
        }
        sleep;
    }
    return;
}

/* Fades the title and the columns to alpha. */
FadeLabels(Float:alpha, Float:duration, Float:delay)
{
    new Float:color[4];
    GetColor(g_objTitle, color[0], color[1], color[2], color[3]);
    AnimColor(g_objTitle, color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
    for (new i; i < 2; i++)
    {
        GetColor(g_objLabels[i], color[0], color[1], color[2], color[3]);
        AnimColor(g_objLabels[i], color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
    }
}

FadeIn()
{
    FadeLabels(1.0, float(400), 0.0);
}

/* Fades the editor out and removes the arrows. */
FadeOut()
{
    FadeLabels(0.0, float(400), 0.0);
    for (new i; i < 2; i++)
    {
        Destroy(g_objArrows[i]);
        g_objArrows[i] = 0;
    }
}

FadeDim()
{
    FadeLabels(g_flDimAlpha, float(400), 0.0);
}

/* Moves the arrows to the selected column and fades them in. */
MoveArrows()
{
    new Float:pos[3];
    new Float:line_w;
    new value[8];
    get_group_tag_attr2("STEDIT", "HEAD", 0, "info_x", value, 8);
    pos[0] = atof(value);
    get_group_tag_attr2("STEDIT", "HEAD", 0, "info_y", value, 8);
    pos[1] = atof(value);
    get_group_tag_attr2("STEDIT", "HEAD", 0, "info_z", value, 8);
    pos[2] = atof(value);
    get_group_tag_attr2("STEDIT", "HEAD", 0, "line_width", value, 8);
    line_w = atof(value);
    AnimMoveLine(g_objArrows[ARROW_UP], pos[0] + line_w * g_iColumn - line_w / 2, pos[1] - 14.0, pos[2], 100.0, 0.0,
            ANIM_FUNC_LINEAR);
    AnimMoveLine(g_objArrows[ARROW_DOWN], pos[0] + line_w * g_iColumn - line_w / 2, 14.0 + pos[1], pos[2], 100.0, 0.0,
            ANIM_FUNC_LINEAR);
    AnimColor(g_objArrows[ARROW_UP], 1.0, 1.0, 1.0, 1.0, 1000.0, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_objArrows[ARROW_DOWN], 1.0, 1.0, 1.0, 1.0, 1000.0, 0.0, ANIM_FUNC_LINEAR);
}

/* Starts the blink of arrow index (ARROW_UP or ARROW_DOWN) after a step. */
BlinkArrow(index)
{
    if (!g_iArrowBlinks[index])
    {
        index = clamp(index, 0, 1);
        g_iArrowBlinks[index] = ms_to_frames(100) * 2;
        AnimColor(g_objArrows[index], 1.0, 1.0, 1.0, 0.0, float(100), 0.0, ANIM_FUNC_LINEAR);
    }
}

/* Advances the blinks of the arrows by a frame. */
UpdateArrowBlinks()
{
    for (new i; i < 2; i++)
    {
        if (g_objArrows[i] && g_iArrowBlinks[i])
        {
            g_iArrowBlinks[i]--;
            if (g_iArrowBlinks[i] == ms_to_frames(100))
            {
                AnimColor(g_objArrows[i], 1.0, 1.0, 1.0, 1.0, float(100), 0.0, ANIM_FUNC_LINEAR);
            }
        }
    }
}

/* Shows the texts of the key and of the order chosen. */
ShowChoices()
{
    new text[32];
    if (!strcmp(g_szKeyIds[g_iChoices[COLUMN_KEY]], "MAILER_SORT_FROM"))
    {
        get_tag_attr2("TEXT", "msgmailer_sort_item_key_from", "value", text, 32);
    }
    else if (!strcmp(g_szKeyIds[g_iChoices[COLUMN_KEY]], "MAILER_SORT_TO"))
    {
        get_tag_attr2("TEXT", "msgmailer_sort_item_key_to", "value", text, 32);
    }
    else if (!strcmp(g_szKeyIds[g_iChoices[COLUMN_KEY]], "MAILER_SORT_SUBJECT"))
    {
        get_tag_attr2("TEXT", "msgmailer_sort_item_key_subject", "value", text, 32);
    }
    else if (!strcmp(g_szKeyIds[g_iChoices[COLUMN_KEY]], "MAILER_SORT_DATE"))
    {
        get_tag_attr2("TEXT", "msgmailer_sort_item_key_date", "value", text, 32);
    }
    else if (!strcmp(g_szKeyIds[g_iChoices[COLUMN_KEY]], "MAILER_SORT_NONE"))
    {
        get_tag_attr2("TEXT", "msgmailer_sort_item_key_none", "value", text, 32);
    }
    SetOptionS(g_objLabels[COLUMN_KEY], TAG("UTF8"), text);
    if (!strcmp(g_szOrderIds[g_iChoices[COLUMN_ORDER]], "MAILER_SORT_ASCENDING"))
    {
        get_tag_attr2("TEXT", "msgmailer_sort_item_order_ascend", "value", text, 32);
    }
    else if (!strcmp(g_szOrderIds[g_iChoices[COLUMN_ORDER]], "MAILER_SORT_DESCENDING"))
    {
        get_tag_attr2("TEXT", "msgmailer_sort_item_order_descend", "value", text, 32);
    }
    SetOptionS(g_objLabels[COLUMN_ORDER], TAG("UTF8"), text);
}

/* Selects the choices of the sort key and order of the folder, the keys listed in key_tag. */
InitChoices(key_tag[], key, order)
{
    if (key == SORT_KEY_FROM)
    {
        g_iChoices[COLUMN_KEY] = GetItemIndex(key_tag, "MAILER_SORT_FROM");
    }
    else if (key == SORT_KEY_TO)
    {
        g_iChoices[COLUMN_KEY] = GetItemIndex(key_tag, "MAILER_SORT_TO");
    }
    else if (key == SORT_KEY_SUBJECT)
    {
        g_iChoices[COLUMN_KEY] = GetItemIndex(key_tag, "MAILER_SORT_SUBJECT");
    }
    else if (key == SORT_KEY_DATE)
    {
        g_iChoices[COLUMN_KEY] = GetItemIndex(key_tag, "MAILER_SORT_DATE");
    }
    else if (key == SORT_KEY_SIZE) /* SIZE and NONE are not in the key lists of the page */
    {
        g_iChoices[COLUMN_KEY] = GetItemIndex(key_tag, "MAILER_SORT_SIZE");
    }
    else if (key == SORT_KEY_NUMBER)
    {
        g_iChoices[COLUMN_KEY] = GetItemIndex(key_tag, "MAILER_SORT_NONE");
    }
    if (SORT_ASCEND == order)
    {
        g_iChoices[COLUMN_ORDER] = GetItemIndex("SORT_UPDOWN_ITEM", "MAILER_SORT_ASCENDING");
    }
    else
    {
        g_iChoices[COLUMN_ORDER] = GetItemIndex("SORT_UPDOWN_ITEM", "MAILER_SORT_DESCENDING");
    }
}

/* Sorts the folder with the key and order chosen and keeps the cursor on the same message. */
ApplySort()
{
    new key;
    new order;
    if (!strcmp(g_szKeyIds[g_iChoices[COLUMN_KEY]], "MAILER_SORT_FROM"))
    {
        key = SORT_KEY_FROM;
    }
    else if (!strcmp(g_szKeyIds[g_iChoices[COLUMN_KEY]], "MAILER_SORT_TO"))
    {
        key = SORT_KEY_TO;
    }
    else if (!strcmp(g_szKeyIds[g_iChoices[COLUMN_KEY]], "MAILER_SORT_SUBJECT"))
    {
        key = SORT_KEY_SUBJECT;
    }
    else if (!strcmp(g_szKeyIds[g_iChoices[COLUMN_KEY]], "MAILER_SORT_DATE"))
    {
        key = SORT_KEY_DATE;
    }
    else if (!strcmp(g_szKeyIds[g_iChoices[COLUMN_KEY]], "MAILER_SORT_NONE"))
    {
        key = SORT_KEY_UNSORTED; /* not SORT_KEY_NUMBER, the key InitChoices shows as NONE */
    }
    if (!strcmp(g_szOrderIds[g_iChoices[COLUMN_ORDER]], "MAILER_SORT_ASCENDING"))
    {
        order = SORT_ASCEND;
    }
    else if (!strcmp(g_szOrderIds[g_iChoices[COLUMN_ORDER]], "MAILER_SORT_DESCENDING"))
    {
        order = SORT_DESCEND;
    }
    new index = g_iRows[g_iFolder] + g_iTops[g_iFolder];
    g_space = sort_messages(g_iFolder, key, order, index);
    get_message_line(g_iTops[g_iFolder], g_iRows[g_iFolder], g_iFolder, index, 0, 0);
}
