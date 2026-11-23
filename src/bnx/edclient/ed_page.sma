/*
 * /opt0/bn/script/edclient/ed_page.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A page of the edclient pages: builds the "page" element page_id (a window, planes, texts and items, the parts
 * of edclient/), moves the focus among its items and tells the page script (g_objManager) what the user does.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "edpage.inc"

/* Directions of GetNextItem: the attributes of the "relation" element of an item. */
const DIR_UP = 0;
const DIR_DOWN = 1;
const DIR_LEFT = 2;
const DIR_RIGHT = 3;

/* Values of g_iValueTypes: how the value of an item goes to the manager on "CHNG". */
const VALUE_NONE = 0;
const VALUE_STR = 1; /* "PARM" with its param, then "STR" with its text */
const VALUE_VAL = 2; /* "PARM" with its param, then "VAL" with the value of the "CHNG" */

#include "wait.inc"

#include "tag.inc"

#include <float>

#include "sound.inc"

/* Returns whether str is a packed string. */
is_packed(str[])
{
    return str[0] <= 0 || str[0] > 255;
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

#include "misc.inc"

#include "pfs.inc"

/*
 * Creates a nine-slice plane at rect (x, y, z, width, height; with no size, the texture's), whose edges keep the size
 * in pixels given by border (left, right, top, bottom).
 */
create_frame_plane(parent, rect[], zsort, color, texture, Float:tex_w, Float:tex_h, Float:border[] = {0, 0, 0, 0})
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

new g_any_child[1]; /* empty: any element */
new g_any_next[1]; /* empty: any element */
public g_objManager; /* the page script, which gets the messages of the page */
public g_objFocus;   /* the focus plane, given to the items */
public g_iOpenSE = ED_SE_PAGE_OPEN; /* the sound played when the page opens */
public g_iCloseSE = ED_SE_PAGE_CLOSE; /* the sound played when the page closes */
new g_objCurrent; /* the item with the focus */
new g_bActive = 1; /* a page without items: 1 while it has the focus */
new g_iCurrent;   /* the index of g_objCurrent */
new g_iItemCount; /* the items: the parts that take the focus */
new g_objItems[10];
new g_tagItems[10];     /* their "item" elements */
new g_tagRelations[10]; /* their "relation" elements: the items up, down, left and right */
new g_iValueTypes[10];
new g_iElementCount; /* the elements without focus: windows, planes, text labels and text boxes */
new g_objElements[15];
new g_tagElements[15];
new Float:g_flElementColors[15][4]; /* their colors, faded by "FADE" */
new g_tagPadHelp; /* the "pad_help" element of the page */

/*
 * Returns the first tag child of parent whose attribute attr is value (any, when both are empty), 0 if there is
 * none.
 */
find_child_tag(parent, tag[], const attr[] = "", const value[] = "")
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
        if (0 == g_iItemCount)
        {
            UpdateStatic();
        }
        else
        {
            UpdateItems();
        }
        sleep;
    }
}

/* Builds the page of the argument page_id; its first item gets the focus. */
Init()
{
    new page_id[64];
    SystemCommand(SYSCMD_WIPE_ON_FREE);
    GetArg("page_id", page_id, 64);
    BuildPage(page_id, g_objFocus);
    if (g_iItemCount > 0)
    {
        g_objCurrent = g_objItems[0];
    }
    play_se(g_iOpenSE);
}

/*
 * A page without items: "FOCS" ("FRST", "RTRN", "FOUT") gives and takes the focus, "FADE" closes the page.
 * While it has the focus, circle and cross go to the manager as "PUSH", the directions as "FOCS".
 */
UpdateStatic()
{
    new msg;
    new a;
    new b;
    new c;
    /* "FOCS": a "FRST", "RTRN" or "FOUT"; "FADE": a the alpha, b the duration. */
    while (RecvMessage(msg, a, b, c))
    {
        if (msg == TAG("FOCS"))
        {
            if (a == TAG("FRST"))
            {
                SetPadHelp(g_tagPadHelp);
                g_bActive = 1;
            }
            else if (a == TAG("RTRN"))
            {
                g_bActive = 1;
            }
            else if (a == TAG("FOUT"))
            {
                g_bActive = 0;
            }
        }
        else if (msg == TAG("FADE"))
        {
            new Float:alpha = float(a);
            new Float:duration = float(b);
            Close(alpha, duration);
        }
    }
    if (g_bActive == 1)
    {
        new pressed;
        PadState(_, pressed, _, _, -1);
        if (pressed & PAD_CIRCLE)
        {
            SendMessage(g_objManager, TAG("PUSH"), 0, TAG("RRGT"), 0, 0);
        }
        else if (pressed & PAD_CROSS)
        {
            SendMessage(g_objManager, TAG("PUSH"), 0, TAG("RDWN"), 0, 0);
        }
        else if (pressed & PAD_UP)
        {
            SendMessage(g_objManager, TAG("FOCS"), TAG("UP"), 0, 0, 0);
        }
        else if (pressed & PAD_DOWN)
        {
            SendMessage(g_objManager, TAG("FOCS"), TAG("DOWN"), 0, 0, 0);
        }
        else if (pressed & PAD_LEFT)
        {
            SendMessage(g_objManager, TAG("FOCS"), TAG("LEFT"), 0, 0, 0);
        }
        else if (pressed & PAD_RIGHT)
        {
            SendMessage(g_objManager, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
        }
    }
}

/*
 * A page with items. "FOCS" from an item leaving in a direction moves the focus to the item there (or, with
 * none, passes the direction to the manager); "FOCS" from the manager gives or takes the focus. "PUSH" goes to
 * the manager, "CHNG" sends it the new value of the item, "CUSR" ("PARM", "ID" and "VAL") its cursor, and
 * "FADE" closes the page.
 */
UpdateItems()
{
    new msg;
    new a;
    new b;
    new c;
    new next;
    new text[64];
    new param[64];
    new len;
    new node;
    /*
     * "FOCS": a the direction, or "FRST", "RTRN" or "FOUT" from the manager; "PUSH": a the row and b the button, passed
     * on; "CHNG": a the value; "CUSR": a the cursor; "FADE": a the alpha, b the duration.
     */
    while (RecvMessage(msg, a, b, c))
    {
        if (msg == TAG("FOCS"))
        {
            if (a == TAG("LEFT"))
            {
                next = GetNextItem(g_tagRelations[g_iCurrent], DIR_LEFT);
                if (next == -1)
                {
                    SendMessage(g_objManager, TAG("FOCS"), TAG("LEFT"), 0, 0, 0);
                }
                else
                {
                    g_iCurrent = next;
                    g_objCurrent = g_objItems[g_iCurrent];
                    SendMessage(g_objCurrent, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                    play_se(ED_SE_CURSOR);
                }
            }
            else if (a == TAG("RIGT"))
            {
                next = GetNextItem(g_tagRelations[g_iCurrent], DIR_RIGHT);
                if (next == -1)
                {
                    SendMessage(g_objManager, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
                }
                else
                {
                    g_iCurrent = next;
                    g_objCurrent = g_objItems[g_iCurrent];
                    SendMessage(g_objCurrent, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                    play_se(ED_SE_CURSOR);
                }
            }
            else if (a == TAG("UP"))
            {
                next = GetNextItem(g_tagRelations[g_iCurrent], DIR_UP);
                if (next == -1)
                {
                    SendMessage(g_objManager, TAG("FOCS"), TAG("UP"), 0, 0, 0);
                }
                else
                {
                    g_iCurrent = next;
                    g_objCurrent = g_objItems[g_iCurrent];
                    SendMessage(g_objCurrent, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                    play_se(ED_SE_CURSOR);
                }
            }
            else if (a == TAG("DOWN"))
            {
                next = GetNextItem(g_tagRelations[g_iCurrent], DIR_DOWN);
                if (next == -1)
                {
                    SendMessage(g_objManager, TAG("FOCS"), TAG("DOWN"), 0, 0, 0);
                }
                else
                {
                    g_iCurrent = next;
                    g_objCurrent = g_objItems[g_iCurrent];
                    SendMessage(g_objCurrent, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                    play_se(ED_SE_CURSOR);
                }
            }
            else if (a == TAG("FRST"))
            {
                SetPadHelp(g_tagPadHelp);
                SendMessage(g_objCurrent, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
            }
            else if (a == TAG("RTRN"))
            {
                SendMessage(g_objCurrent, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                SendMessage(g_objCurrent, TAG("PUSH"), 0, 0, 0, 0);
            }
            else if (a == TAG("FOUT"))
            {
                SendMessage(g_objCurrent, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
            }
        }
        else if (msg == TAG("PUSH"))
        {
            SendMessage(g_objManager, TAG("PUSH"), a, b, 0, 0);
        }
        else if (msg == TAG("CHNG"))
        {
            SendMessage(g_objCurrent, TAG("CHNG"), 0, 0, 0, 0);
            node = g_tagItems[g_iCurrent];
            len = domGetData(node, "param", param, 64);
            switch (g_iValueTypes[g_iCurrent])
            {
                case VALUE_NONE:
                {
                }
                case VALUE_STR:
                {
                    SendMessage(g_objCurrent, TAG("GTXT"), 0, 0, 0, 0);
                    wait_message_str(TAG("GTXT"), text, 64);
                    SendMessageStr(g_objManager, TAG("PARM"), param);
                    SendMessageStr(g_objManager, TAG("STR"), text);
                }
                case VALUE_VAL:
                {
                    SendMessageStr(g_objManager, TAG("PARM"), param);
                    SendMessage(g_objManager, TAG("VAL"), a, 0, 0, 0);
                }
                default:
                {
                }
            }
        }
        else if (msg == TAG("FADE"))
        {
            new Float:alpha = float(a);
            new Float:duration = float(b);
            Close(alpha, duration);
        }
        else if (msg == TAG("CUSR"))
        {
            node = g_tagItems[g_iCurrent];
            len = domGetData(node, "id", param, 64);
            SendMessage(g_objManager, TAG("PARM"), 0, 0, 0, 0);
            SendMessageStr(g_objManager, TAG("ID"), param);
            SendMessage(g_objManager, TAG("VAL"), a, 0, 0, 0);
        }
    }
}

/*
 * Creates the children of the "page" element page_id: its pad help, its elements and its items; focus is the
 * focus plane of the items.
 */
BuildPage(page_id[], focus)
{
    new name[64];
    new page;
    new node;
    new parent;
    new len;
    page = domFindTag(domRoot(), "page", page_id, 0);
    node = domFirstChild(page, g_any_child);
    for (; node; node = domNext(node, g_any_next))
    {
        parent = FindParent(node);
        domGetTagName(node, name, 64);
        if (0 == strcmp("pad_help", name))
        {
            g_tagPadHelp = node;
        }
        else if (0 == strcmp("window", name))
        {
            AddWindow(node, parent, g_flElementColors[g_iElementCount]);
        }
        else if (0 == strcmp("plane", name))
        {
            g_objElements[g_iElementCount] = AddPlane(node, parent, g_flElementColors[g_iElementCount]);
            g_tagElements[g_iElementCount] = node;
            g_iElementCount++;
        }
        else if (0 == strcmp("text_label", name))
        {
            g_objElements[g_iElementCount] = AddTextLabel(node, parent, g_flElementColors[g_iElementCount]);
            g_tagElements[g_iElementCount] = node;
            g_iElementCount++;
        }
        else if (0 == strcmp("text_box", name))
        {
            g_objElements[g_iElementCount] = AddTextBox(node, parent, g_flElementColors[g_iElementCount]);
            g_tagElements[g_iElementCount] = node;
            g_iElementCount++;
        }
        else if (0 == strcmp("item", name))
        {
            len = domGetData(node, "type", name, 64);
            if (0 == len)
            {
                continue;
            }
            if (0 == strcmp("edit_box", name))
            {
                g_objItems[g_iItemCount] = AddEditBox(node, parent, focus);
                g_iValueTypes[g_iItemCount] = VALUE_STR;
            }
            else if (0 == strcmp("radio_h", name))
            {
                g_objItems[g_iItemCount] = AddRadioButton(node, parent, focus);
                g_iValueTypes[g_iItemCount] = VALUE_VAL;
            }
            else if (0 == strcmp("radio_ex_h", name))
            {
                g_objItems[g_iItemCount] = AddRadioButtonEx(node, parent, focus);
                g_iValueTypes[g_iItemCount] = VALUE_VAL;
            }
            else if (0 == strcmp("menu_item", name))
            {
                g_objItems[g_iItemCount] = AddMenuItem(node, parent, focus);
                g_iValueTypes[g_iItemCount] = VALUE_STR;
            }
            else if (0 == strcmp("menu_item2", name))
            {
                g_objItems[g_iItemCount] = AddMenuItem2(node, parent, focus);
                g_iValueTypes[g_iItemCount] = VALUE_STR;
            }
            else if (0 == strcmp("menu_item3", name))
            {
                g_objItems[g_iItemCount] = AddMenuItem3(node, parent, focus);
                g_iValueTypes[g_iItemCount] = VALUE_STR;
            }
            else if (0 == strcmp("combo_box", name))
            {
                g_objItems[g_iItemCount] = AddComboBox(node, parent, focus);
                g_iValueTypes[g_iItemCount] = VALUE_STR;
            }
            else if (0 == strcmp("spin", name))
            {
                g_objItems[g_iItemCount] = AddSpin(node, parent, focus);
                g_iValueTypes[g_iItemCount] = VALUE_STR;
            }
            else if (0 == strcmp("spin_label", name))
            {
                g_objItems[g_iItemCount] = AddSpinLabel(node, parent, focus);
                g_iValueTypes[g_iItemCount] = VALUE_VAL;
            }
            else if (0 == strcmp("zip_set", name))
            {
                g_objItems[g_iItemCount] = AddZipSet(node, parent, focus);
                g_iValueTypes[g_iItemCount] = VALUE_STR;
            }
            else if (0 == strcmp("textarea2", name))
            {
                g_objItems[g_iItemCount] = AddTextArea2(node, parent, focus);
                g_iValueTypes[g_iItemCount] = VALUE_NONE;
            }
            else if (0 == strcmp("textarea", name))
            {
                g_objItems[g_iItemCount] = AddTextArea(node, parent, focus);
                g_iValueTypes[g_iItemCount] = VALUE_NONE;
            }
            else if (0 == strcmp("birthday_set", name))
            {
                g_objItems[g_iItemCount] = AddBirthdaySet(node, parent, focus);
                g_iValueTypes[g_iItemCount] = VALUE_STR;
            }
            else if (0 == strcmp("period_set", name))
            {
                g_objItems[g_iItemCount] = AddPeriodSet(node, parent, focus);
                g_iValueTypes[g_iItemCount] = VALUE_STR;
            }
            else if (0 == strcmp("button", name))
            {
                g_objItems[g_iItemCount] = AddButton(node, parent, focus);
                g_iValueTypes[g_iItemCount] = VALUE_VAL;
            }
            else
            {
                printf("Error : Unknown item type : [%s]\n", name);
                continue;
            }
            g_tagItems[g_iItemCount] = node;
            g_tagRelations[g_iItemCount] = find_child_tag(node, "relation");
            g_iItemCount++;
        }
        else
        {
            printf("Error : Unknown tag name : [%s]\n", name);
        }
    }
}

/*
 * Returns the object the element node goes under: its attribute parent is "self", "disp2d" or the id of an
 * element or item made before; the page without it.
 */
FindParent(node)
{
    new index;
    new name[64];
    new parent;
    new len;
    parent = Self();
    len = domGetData(node, "parent", name, 64);
    if (len == 1)
    {
        if (0 == strcmp("self", name))
        {
            parent = Self();
        }
        else if (0 == strcmp("disp2d", name))
        {
            parent = Disp2D(0);
        }
        else
        {
            index = FindElement(name);
            if (-1 != index)
            {
                return g_objElements[index];
            }
            index = FindItem(name);
            if (-1 != index)
            {
                return g_objItems[index];
            }
        }
    }
    return parent;
}

/* Returns the index of the element with that id, -1 if there is none. */
FindElement(id[])
{
    new i;
    new buf[64];
    new len;
    i = 0;
    for (; i < g_iElementCount; i++)
    {
        len = domGetData(g_tagElements[i], "id", buf, 64);
        if (0 == len)
        {
            continue;
        }
        if (0 == strcmp(id, buf))
        {
            return i;
        }
    }
    return -1;
}

/*
 * Adds the window: a nine-slice plane of the image img_id behind the page, at x, y (320, 260) and of w, h
 * (560, 280); rgba gets its color.
 */
AddWindow(node, parent, rgba[])
{
    new img_id[64];
    new buf[64];
    new Float:x;
    new Float:y;
    new Float:w;
    new h;
    new texture;
    new Float:tex_w;
    new Float:tex_h;
    new zsort;
    new color;
    new Float:rect[5];
    new Float:border[4];
    new plane;
    new len;
    len = domGetData(node, "img_id", img_id, 64);
    if (len == 1)
    {
        new res = domFindTag(domRoot(), "IMG_RESOURCE", img_id, 0);
        domGetData(res, "texture", buf, 64);
        texture = atoi(buf);
        domGetData(res, "w", buf, 64);
        tex_w = atof(buf);
        domGetData(res, "h", buf, 64);
        tex_h = atof(buf);
    }
    x = 320.0;
    y = 260.0;
    w = 560.0;
    h = 280.0;
    len = domGetData(node, "x", buf, 64);
    if (len == 1)
    {
        x = atof(buf);
    }
    len = domGetData(node, "y", buf, 64);
    if (len == 1)
    {
        y = atof(buf);
    }
    len = domGetData(node, "w", buf, 64);
    if (len == 1)
    {
        w = atof(buf);
    }
    len = domGetData(node, "h", buf, 64);
    if (len == 1)
    {
        /* NOTE: bug: the attribute h sets the width; the height stays 280. */
        w = atof(buf);
    }
    rect[0] = x;
    rect[1] = y;
    rect[2] = -1.0;
    rect[3] = w;
    rect[4] = h;
    zsort = ZSORT_NONE;
    color = pack_color(255, 255, 255, 255);
    rgba[0] = 1.0;
    rgba[1] = 1.0;
    rgba[2] = 1.0;
    rgba[3] = 1.0;
    border[0] = 0.5 * tex_w;
    border[1] = 0.5 * tex_w;
    border[2] = 0.5 * tex_h;
    border[3] = 0.5 * tex_h;
    plane = create_frame_plane(parent, rect, zsort, color, texture, tex_w, tex_h, border);
    AnimColor(plane, 1.0, 1.0, 1.0, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
    g_objElements[g_iElementCount] = plane;
    g_tagElements[g_iElementCount] = node;
    g_iElementCount++;
}

/* Adds a plane of the image img_id at x, y and of w, h (the size of the image without them). */
AddPlane(node, parent, rgba[])
{
    new img_id[64];
    new buf[64];
    new Float:x;
    new Float:y;
    new Float:w;
    new Float:h;
    new texture;
    new Float:tex_w;
    new Float:tex_h;
    new zsort;
    new color;
    new Float:rect[5];
    new plane;
    new len;
    x = 0.0;
    y = 0.0;
    w = 0.0;
    h = 0.0;
    texture = 0;
    tex_w = 0.0;
    tex_h = 0.0;
    len = domGetData(node, "img_id", img_id, 64);
    if (len == 1)
    {
        new res = domFindTag(domRoot(), "IMG_RESOURCE", img_id, 0);
        domGetData(res, "texture", buf, 64);
        texture = atoi(buf);
        domGetData(res, "w", buf, 64);
        tex_w = atof(buf);
        domGetData(res, "h", buf, 64);
        tex_h = atof(buf);
    }
    len = domGetData(node, "x", buf, 64);
    if (len == 1)
    {
        x = atof(buf);
    }
    len = domGetData(node, "y", buf, 64);
    if (len == 1)
    {
        y = atof(buf);
    }
    len = domGetData(node, "w", buf, 64);
    if (len == 1)
    {
        w = atof(buf);
    }
    len = domGetData(node, "h", buf, 64);
    if (len == 1)
    {
        h = atof(buf);
    }
    rect[0] = x;
    rect[1] = y;
    rect[2] = 0.0;
    rect[3] = w;
    rect[4] = h;
    zsort = ZSORT_NONE;
    color = pack_color(255, 255, 255, 255);
    rgba[0] = 1.0;
    rgba[1] = 1.0;
    rgba[2] = 1.0;
    rgba[3] = 1.0;
    plane = create_frame_plane(parent, rect, zsort, color, texture, tex_w, tex_h);
    AnimColor(plane, 1.0, 1.0, 1.0, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
    return plane;
}

/*
 * Adds a text label at x, y, w wide, aligned by align_v and align_h: the text of text_id, or the value of the
 * argument named by param.
 */
AddTextLabel(node, parent, rgba[])
{
    new font_size;
    new text[64];
    new id[64];
    new label;
    new len;
    new Float:x;
    new Float:y;
    new w;
    new v_align;
    new h_align;
    x = 0.0;
    y = 0.0;
    w = 0;
    v_align = ALIGN_CENTER;
    h_align = ALIGN_CENTER;
    len = domGetData(node, "x", text, 64);
    if (len == 1)
    {
        x = atof(text);
    }
    len = domGetData(node, "y", text, 64);
    if (len == 1)
    {
        y = atof(text);
    }
    len = domGetData(node, "w", text, 64);
    if (len == 1)
    {
        w = atoi(text);
    }
    font_size = 15;
    rgba[0] = 0.7843;
    rgba[1] = 0.7843;
    rgba[2] = 0.7843;
    rgba[3] = 1.0;
    len = domGetData(node, "align_v", text, 64);
    if (len == 1)
    {
        if (0 == strcmp("center", text))
        {
            v_align = ALIGN_CENTER;
        }
        else if (0 == strcmp("top", text))
        {
            v_align = ALIGN_TOP;
        }
        else if (0 == strcmp("bottom", text))
        {
            v_align = ALIGN_BOTTOM;
        }
    }
    len = domGetData(node, "align_h", text, 64);
    if (len == 1)
    {
        if (0 == strcmp("center", text))
        {
            h_align = ALIGN_CENTER;
        }
        else if (0 == strcmp("left", text))
        {
            h_align = ALIGN_LEFT;
        }
        else if (0 == strcmp("right", text))
        {
            h_align = ALIGN_RIGHT;
        }
    }
    text{0} = 0;
    len = domGetData(node, "text_id", id, 64);
    if (len == 1)
    {
        get_tag_attr("TEXT", id, "value", text, 64);
    }
    len = domGetData(node, "param", id, 64);
    if (len == 1)
    {
        GetArg(id, text, 64);
    }
    label = create_text_label(parent, x, y, rgba, font_size, text, w, v_align, h_align, BLEND_ADD);
    return label;
}

/*
 * Adds a text box at x, y and of w, h, with the text of text_id or of the argument param, in which the two
 * characters \n break the line; waits until it is laid out.
 */
AddTextBox(node, parent, Float:rgba[])
{
    new id[64];
    new buf[192];
    new text[192];
    new Float:x;
    new Float:y;
    new font_size;
    new w;
    new h;
    new line_space;
    new box;
    new len;
    len = domGetData(node, "x", buf, 192);
    if (len == 1)
    {
        x = atof(buf);
    }
    len = domGetData(node, "y", buf, 192);
    if (len == 1)
    {
        y = atof(buf);
    }
    len = domGetData(node, "w", buf, 192);
    if (len == 1)
    {
        w = atoi(buf);
    }
    len = domGetData(node, "h", buf, 192);
    if (len == 1)
    {
        h = atoi(buf);
    }
    buf{0} = 0;
    len = domGetData(node, "text_id", id, 64);
    if (len == 1)
    {
        get_tag_attr("TEXT", id, "value", buf, 192);
    }
    len = domGetData(node, "param", id, 64);
    if (len == 1)
    {
        GetArg(id, buf, 192);
    }
    font_size = 15;
    line_space = 4;
    rgba[0] = 0.7843;
    rgba[1] = 0.7843;
    rgba[2] = 0.7843;
    rgba[3] = 1.0;
    box = CreateTextbox(parent);
    y = y + float(font_size);
    SetPos(box, x, y, 0.0, 0);
    SetColor(box, rgba[0], rgba[1], rgba[2], 0.0);
    replace_all(text, buf, "\\n", "\n", 192);
    SetOptionS(box, TAG("UTF8"), text);
    SetOptionI(box, TAG("FSIZ"), font_size);
    SetOptionI(box, TAG("TW64"), w << 6);
    SetOptionI(box, TAG("TH64"), h << 6);
    SetOptionI(box, TAG("LS64"), line_space << 6);
    SetOptionI(box, TAG("ALPH"), BLEND_ADD);
    for (; ; )
    {
        new rect[5];
        new count;
        count = GetOptionP(box, TAG("RECT"), rect, 5);
        if (rect[0])
        {
            break;
        }
        sleep;
    }
    AnimColor(box, rgba[0], rgba[1], rgba[2], rgba[3], 200.0, 0.0, ANIM_FUNC_LINEAR);
    return box;
}

/*
 * The Add functions of the items attach the part of the same name to a new object under parent, at x, y, and
 * return it; most take their initial value from the argument named by their attribute param.
 */
AddEditBox(node, parent, focus)
{
    new buf[64];
    new attr[64];
    new args[64];
    new Float:x;
    new Float:y;
    new pad_help;
    new obj;
    new len;
    len = domGetData(node, "x", buf, 64);
    if (len == 1)
    {
        x = atof(buf);
    }
    len = domGetData(node, "y", buf, 64);
    if (len == 1)
    {
        y = atof(buf);
    }
    args{0} = 0;
    len = domGetData(node, "param", attr, 64);
    if (len == 1)
    {
        len = GetArg(attr, buf, 64);
        if (len == 1)
        {
            sprintf(args, "text=%s;", buf);
        }
    }
    len = domGetData(node, "title", attr, 64);
    if (len == 1)
    {
        len = get_tag_attr("TEXT", attr, "value", buf, 64);
        if (len == 1)
        {
            sprintf(buf, "title=%s;", buf);
            append_string(args, buf, 64);
        }
    }
    obj = CreateNull(parent);
    SetPos(obj, x, y, 0.0, 0);
    SetOptionI(obj, TAG("ZSRT"), ZSORT_OVERLAY);
    AttachScript(obj, _, "edit_box", args);
    SetPubVar(obj, "manager", Self());
    SetPubVar(obj, "focus", focus);
    SetPubVar(obj, "padhelp", PADHELP_CIRCLE | PADHELP_CROSS);
    len = domGetData(node, "w", buf, 64);
    if (len == 1)
    {
        SetPubVar(obj, "width", atof(buf));
    }
    len = domGetData(node, "mode", buf, 64);
    if (len == 1)
    {
        if (0 == strcmp("userid", buf))
        {
            SetPubVar(obj, "mode", 1);
        }
        else if (0 == strcmp("password", buf))
        {
            SetPubVar(obj, "mode", EDIT_MODE_PASSWORD);
        }
        else if (0 == strcmp("katakana", buf))
        {
            SetPubVar(obj, "mode", 3);
        }
        else if (0 == strcmp("katakana_fixed", buf))
        {
            SetPubVar(obj, "mode", 7);
        }
        else if (0 == strcmp("number", buf))
        {
            SetPubVar(obj, "mode", 4);
        }
        else if (0 == strcmp("qwerty", buf))
        {
            SetPubVar(obj, "mode", 9);
        }
    }
    len = domGetData(node, "size", buf, 64);
    if (len == 1)
    {
        SetPubVar(obj, "max_byte", atoi(buf) + 1);
    }
    len = GetPadHelp(node, pad_help);
    if (len == 1)
    {
        SetPubVar(obj, "padhelp", pad_help);
    }
    return obj;
}

AddRadioButton(node, parent, focus)
{
    new n;
    new buf[64];
    new text_id[64];
    new args[64];
    new param[64];
    new obj;
    new child;
    new len;
    new Float:x;
    new Float:y;
    len = domGetData(node, "x", buf, 64);
    if (len == 1)
    {
        x = atof(buf);
    }
    len = domGetData(node, "y", buf, 64);
    if (len == 1)
    {
        y = atof(buf);
    }
    n = 0;
    child = domFirstChild(node, g_any_child);
    for (; child; child = domNext(child, g_any_next))
    {
        domGetTagName(child, buf, 64);
        if (0 != strcmp("data", buf))
        {
            continue;
        }
        len = domGetData(child, "text_id", text_id, 64);
        if (0 == len)
        {
            continue;
        }
        buf{0} = 0;
        get_tag_attr("TEXT", text_id, "value", buf, 64);
        sprintf(buf, "btn%d=%s;", n, buf);
        append_string(args, buf, 64);
        n++;
    }
    sprintf(buf, "interval=%f;", 30.0);
    append_string(args, buf, 64);
    sprintf(buf, "fontsize=%d;", 16);
    append_string(args, buf, 64);
    obj = CreateNull(parent);
    SetPos(obj, x, y, 0.0, 0);
    AttachScript(obj, _, "radio_button", args);
    SetPubVar(obj, "manager", Self());
    SetPubVar(obj, "focus", focus);
    SetPubVar(obj, "padhelp", PADHELP_CIRCLE | PADHELP_CROSS);
    len = domGetData(node, "param", param, 64);
    len = GetArg(param, buf, 64);
    if (len == 1)
    {
        SetPubVar(obj, "select", atoi(buf));
    }
    return obj;
}

AddRadioButtonEx(node, parent, focus)
{
    new Float:x;
    new Float:y;
    new buf[64];
    new args[64];
    new param[64];
    new obj;
    new len;
    len = domGetData(node, "x", buf, 64);
    if (len == 1)
    {
        x = atof(buf);
    }
    len = domGetData(node, "y", buf, 64);
    if (len == 1)
    {
        y = atof(buf);
    }
    obj = CreateNull(parent);
    SetPos(obj, x, y, 0.0, 0);
    AttachScript(obj, _, "radio_button_ex", args);
    SetPubVar(obj, "g_objManager", Self());
    SetPubVar(obj, "g_objFocus", focus);
    SetPubVar(obj, "g_tagParts", node);
    len = domGetData(node, "param", param, 64);
    len = GetArg(param, buf, 64);
    if (len == 1)
    {
        SetPubVar(obj, "g_iCurrent", atoi(buf));
    }
    return obj;
}

AddMenuItem(node, parent, focus)
{
    new buf[64];
    new args[64];
    new Float:x;
    new Float:y;
    new pad_help;
    new obj;
    new len;
    len = domGetData(node, "x", buf, 64);
    if (len == 1)
    {
        x = atof(buf);
    }
    len = domGetData(node, "y", buf, 64);
    if (len == 1)
    {
        y = atof(buf);
    }
    len = domGetData(node, "menu_id", buf, 64);
    if (len == 1)
    {
        sprintf(args, "menu_id=%s;", buf);
    }
    obj = CreateNull(parent);
    SetPos(obj, x, y, 0.0, 0);
    AttachScript(obj, _, "menu_item", args);
    SetPubVar(obj, "g_objManager", Self());
    SetPubVar(obj, "g_objFocus", focus);
    len = domGetData(node, "fontsize", buf, 64);
    if (len == 1)
    {
        SetPubVar(obj, "g_iFontSize", atoi(buf));
    }
    len = domGetData(node, "interval", buf, 64);
    if (len == 1)
    {
        SetPubVar(obj, "g_flInterval", atof(buf));
    }
    len = domGetData(node, "align_h", buf, 64);
    if (len == 1)
    {
        new align;
        if (0 == strcmp(buf, "left"))
        {
            align = ALIGN_LEFT;
        }
        else if (0 == strcmp(buf, "center"))
        {
            align = ALIGN_CENTER;
        }
        else if (0 == strcmp(buf, "right"))
        {
            align = ALIGN_RIGHT;
        }
        SetPubVar(obj, "g_iAlignH", align);
    }
    len = domGetData(node, "visible", buf, 64);
    if (len == 1)
    {
        SetPubVar(obj, "g_iVisible", atoi(buf));
    }
    len = domGetData(node, "current", buf, 64);
    if (len == 1)
    {
        SetPubVar(obj, "g_iCurrent", atoi(buf));
    }
    len = GetPadHelp(node, pad_help);
    if (len == 1)
    {
        SetPubVar(obj, "g_iPadHelp", pad_help);
    }
    return obj;
}

AddMenuItem2(node, parent, focus)
{
    new buf[64];
    new args[64];
    new Float:x;
    new Float:y;
    new pad_help;
    new obj;
    new len;
    len = domGetData(node, "x", buf, 64);
    if (len == 1)
    {
        x = atof(buf);
    }
    len = domGetData(node, "y", buf, 64);
    if (len == 1)
    {
        y = atof(buf);
    }
    len = domGetData(node, "menu_id", buf, 64);
    if (len == 1)
    {
        sprintf(args, "menu_id=%s;", buf);
    }
    obj = CreateNull(parent);
    SetPos(obj, x, y, 0.0, 0);
    AttachScript(obj, _, "menu_item2", args);
    SetPubVar(obj, "g_objManager", Self());
    SetPubVar(obj, "g_objFocus", focus);
    len = domGetData(node, "fontsize", buf, 64);
    if (len == 1)
    {
        SetPubVar(obj, "g_iFontSize", atoi(buf));
    }
    len = domGetData(node, "interval", buf, 64);
    if (len == 1)
    {
        SetPubVar(obj, "g_flInterval", atof(buf));
    }
    len = domGetData(node, "align_h", buf, 64);
    if (len == 1)
    {
        new align;
        if (0 == strcmp(buf, "left"))
        {
            align = ALIGN_LEFT;
        }
        else if (0 == strcmp(buf, "center"))
        {
            align = ALIGN_CENTER;
        }
        else if (0 == strcmp(buf, "right"))
        {
            align = ALIGN_RIGHT;
        }
        SetPubVar(obj, "g_iAlignH", align);
    }
    len = domGetData(node, "visible", buf, 64);
    if (len == 1)
    {
        SetPubVar(obj, "g_iVisible", atoi(buf));
    }
    len = domGetData(node, "current", buf, 64);
    if (len == 1)
    {
        SetPubVar(obj, "g_iCurrent", atoi(buf));
    }
    len = GetPadHelp(node, pad_help);
    if (len == 1)
    {
        SetPubVar(obj, "g_iPadHelp", pad_help);
    }
    return obj;
}

AddMenuItem3(node, parent, focus)
{
    new buf[64];
    new args[64];
    new Float:x;
    new Float:y;
    new pad_help;
    new obj;
    new len;
    len = domGetData(node, "x", buf, 64);
    if (len == 1)
    {
        x = atof(buf);
    }
    len = domGetData(node, "y", buf, 64);
    if (len == 1)
    {
        y = atof(buf);
    }
    len = domGetData(node, "menu_id", buf, 64);
    if (len == 1)
    {
        sprintf(args, "menu_id=%s;", buf);
    }
    obj = CreateNull(parent);
    SetPos(obj, x, y, 0.0, 0);
    AttachScript(obj, _, "menu_item3", args);
    SetPubVar(obj, "g_objManager", Self());
    SetPubVar(obj, "g_objFocus", focus);
    len = domGetData(node, "fontsize", buf, 64);
    if (len == 1)
    {
        SetPubVar(obj, "g_iFontSize", atoi(buf));
    }
    len = domGetData(node, "interval", buf, 64);
    if (len == 1)
    {
        SetPubVar(obj, "g_flInterval", atof(buf));
    }
    len = domGetData(node, "visible", buf, 64);
    if (len == 1)
    {
        SetPubVar(obj, "g_iVisible", atoi(buf));
    }
    len = domGetData(node, "current", buf, 64);
    if (len == 1)
    {
        SetPubVar(obj, "g_iCurrent", atoi(buf));
    }
    len = GetPadHelp(node, pad_help);
    if (len == 1)
    {
        SetPubVar(obj, "g_iPadHelp", pad_help);
    }
    return obj;
}

AddComboBox(node, parent, focus)
{
    new buf[64];
    new param[64];
    new args[64];
    new Float:x;
    new Float:y;
    new obj;
    new len;
    len = domGetData(node, "x", buf, 64);
    if (len == 1)
    {
        x = atof(buf);
    }
    len = domGetData(node, "y", buf, 64);
    if (len == 1)
    {
        y = atof(buf);
    }
    len = domGetData(node, "param", param, 64);
    if (len == 1)
    {
        len = GetArg(param, buf, 64);
        if (len == 1)
        {
            sprintf(buf, "text=%s;", buf);
            append_string(args, buf, 64);
        }
    }
    len = domGetData(node, "id", buf, 64);
    if (len == 1)
    {
        sprintf(buf, "menu_id=%s;", buf);
        append_string(args, buf, 64);
    }
    obj = CreateNull(parent);
    SetPos(obj, x, y, 0.0, 0);
    AttachScript(obj, _, "combo_box", args);
    SetPubVar(obj, "g_objManager", Self());
    SetPubVar(obj, "g_objFocus", focus);
    SetPubVar(obj, "g_iPadHelp", PADHELP_CIRCLE | PADHELP_CROSS);
    len = domGetData(node, "visible", buf, 64);
    if (len == 1)
    {
        SetPubVar(obj, "g_iVisible", atoi(buf));
    }
    len = domGetData(node, "fontsize", buf, 64);
    if (len == 1)
    {
        SetPubVar(obj, "g_iFontSize", atoi(buf));
    }
    len = domGetData(node, "list_interval", buf, 64);
    if (len == 1)
    {
        SetPubVar(obj, "g_flListInterval", atof(buf));
    }
    len = domGetData(node, "hide", buf, 64);
    if (len == 1)
    {
        if (0 == strcmp("on", buf))
        {
            SetPubVar(obj, "g_bHide", 1);
        }
        else if (0 == strcmp("off", buf))
        {
            SetPubVar(obj, "g_bHide", 0);
        }
    }
    len = domGetData(node, "speed", buf, 64);
    if (len == 1)
    {
        SetPubVar(obj, "g_flSpeed", atof(buf));
    }
    return obj;
}

AddSpin(node, parent, focus)
{
    new Float:x;
    new Float:y;
    new args[64];
    new buf[64];
    new param[64];
    new obj;
    new len;
    len = domGetData(node, "x", buf, 64);
    if (len == 1)
    {
        x = atof(buf);
    }
    len = domGetData(node, "y", buf, 64);
    if (len == 1)
    {
        y = atof(buf);
    }
    len = domGetData(node, "param", param, 64);
    if (len == 1)
    {
        len = GetArg(param, buf, 64);
        if (len == 1)
        {
            sprintf(buf, "text=%s;", buf);
            append_string(args, buf, 64);
        }
    }
    obj = CreateNull(parent);
    SetPos(obj, x, y, 0.0, 0);
    AttachScript(obj, _, "spin", args);
    SetPubVar(obj, "g_objManager", Self());
    SetPubVar(obj, "g_objFocus", focus);
    SetPubVar(obj, "g_iPadHelp", PADHELP_CIRCLE | PADHELP_CROSS);
    len = domGetData(node, "min", buf, 64);
    if (len == 1)
    {
        SetPubVar(obj, "g_iMin", atoi(buf));
    }
    len = domGetData(node, "max", buf, 64);
    if (len == 1)
    {
        SetPubVar(obj, "g_iMax", atoi(buf));
    }
    len = domGetData(node, "figure", buf, 64);
    if (len == 1)
    {
        SetPubVar(obj, "g_iFigure", atoi(buf));
    }
    return obj;
}

AddSpinLabel(node, parent, focus)
{
    new Float:x;
    new Float:y;
    new buf[64];
    new param[64];
    new obj;
    new len;
    len = domGetData(node, "x", buf, 64);
    if (len == 1)
    {
        x = atof(buf);
    }
    len = domGetData(node, "y", buf, 64);
    if (len == 1)
    {
        y = atof(buf);
    }
    obj = CreateNull(parent);
    SetPos(obj, x, y, 0.0, 0);
    AttachScript(obj, _, "spin_label");
    SetPubVar(obj, "g_objManager", Self());
    SetPubVar(obj, "g_objFocus", focus);
    SetPubVar(obj, "g_tagParts", node);
    len = domGetData(node, "param", param, 64);
    len = GetArg(param, buf, 64);
    if (len == 1)
    {
        SetPubVar(obj, "g_iCurrent", atoi(buf));
    }
    return obj;
}

AddZipSet(node, parent, focus)
{
    new Float:x;
    new Float:y;
    new buf[64];
    new args[64];
    new param[64];
    new obj;
    new len;
    len = domGetData(node, "x", buf, 64);
    if (len == 1)
    {
        x = atof(buf);
    }
    len = domGetData(node, "y", buf, 64);
    if (len == 1)
    {
        y = atof(buf);
    }
    len = domGetData(node, "param", param, 64);
    if (len == 1)
    {
        len = GetArg(param, buf, 64);
        if (len == 1)
        {
            sprintf(buf, "text=%s;", buf);
            append_string(args, buf, 64);
        }
    }
    obj = CreateNull(parent);
    SetPos(obj, x, y, 0.0, 0);
    AttachScript(obj, _, "zip_set", args);
    SetPubVar(obj, "g_objManager", Self());
    SetPubVar(obj, "g_objFocus", focus);
    SetPubVar(obj, "g_iPadHelp", PADHELP_CIRCLE | PADHELP_CROSS);
    return obj;
}

AddTextArea(node, parent, focus)
{
    new Float:x;
    new Float:y;
    new obj;
    new buf[64];
    new args[64];
    new pad_help;
    new len;
    len = domGetData(node, "x", buf, 64);
    if (len == 1)
    {
        x = atof(buf);
    }
    len = domGetData(node, "y", buf, 64);
    if (len == 1)
    {
        y = atof(buf);
    }
    len = domGetData(node, "text_id", buf, 64);
    if (len == 1)
    {
        sprintf(args, "text_id=%s;", buf);
    }
    obj = CreateNull(parent);
    SetPos(obj, x, y, 0.0, 0);
    AttachScript(obj, _, "textarea", args);
    SetPubVar(obj, "g_objManager", Self());
    SetPubVar(obj, "g_objFocus", focus);
    len = domGetData(node, "line", buf, 64);
    if (len == 1)
    {
        SetPubVar(obj, "g_iDispNum", atoi(buf));
    }
    len = domGetData(node, "margin", buf, 64);
    if (len == 1)
    {
        SetPubVar(obj, "g_iMargin", atoi(buf));
    }
    len = GetPadHelp(node, pad_help);
    if (len == 1)
    {
        SetPubVar(obj, "g_iPadHelp", pad_help);
    }
    return obj;
}

AddTextArea2(node, parent, focus)
{
    new Float:x;
    new Float:y;
    new obj;
    new buf[64];
    new args[64];
    new pad_help;
    new len;
    len = domGetData(node, "x", buf, 64);
    if (len == 1)
    {
        x = atof(buf);
    }
    len = domGetData(node, "y", buf, 64);
    if (len == 1)
    {
        y = atof(buf);
    }
    len = domGetData(node, "text_id", buf, 64);
    if (len == 1)
    {
        sprintf(args, "text_id=%s;", buf);
    }
    obj = CreateNull(parent);
    SetPos(obj, x, y, 0.0, 0);
    AttachScript(obj, _, "textarea2", args);
    SetPubVar(obj, "g_objManager", Self());
    SetPubVar(obj, "g_objFocus", focus);
    len = domGetData(node, "w", buf, 64);
    if (len == 1)
    {
        SetPubVar(obj, "g_iWidth", atoi(buf));
    }
    len = domGetData(node, "h", buf, 64);
    if (len == 1)
    {
        SetPubVar(obj, "g_iHeight", atoi(buf));
    }
    len = GetPadHelp(node, pad_help);
    if (len == 1)
    {
        SetPubVar(obj, "g_iPadHelp", pad_help);
    }
    return obj;
}

AddBirthdaySet(node, parent, focus)
{
    new Float:x;
    new Float:y;
    new buf[64];
    new args[64];
    new param[64];
    new obj;
    new len;
    len = domGetData(node, "x", buf, 64);
    if (len == 1)
    {
        x = atof(buf);
    }
    len = domGetData(node, "y", buf, 64);
    if (len == 1)
    {
        y = atof(buf);
    }
    len = domGetData(node, "param", param, 64);
    if (len == 1)
    {
        len = GetArg(param, buf, 64);
        if (len == 1)
        {
            sprintf(buf, "text=%s;", buf);
            append_string(args, buf, 64);
        }
    }
    obj = CreateNull(parent);
    SetPos(obj, x, y, 0.0, 0);
    AttachScript(obj, _, "birthday_set", args);
    SetPubVar(obj, "g_objManager", Self());
    SetPubVar(obj, "g_objFocus", focus);
    SetPubVar(obj, "g_tagParts", node);
    return obj;
}

AddPeriodSet(node, parent, focus)
{
    new Float:x;
    new Float:y;
    new buf[64];
    new args[64];
    new param[64];
    new obj;
    new len;
    len = domGetData(node, "x", buf, 64);
    if (len == 1)
    {
        x = atof(buf);
    }
    len = domGetData(node, "y", buf, 64);
    if (len == 1)
    {
        y = atof(buf);
    }
    len = domGetData(node, "param", param, 64);
    if (len == 1)
    {
        len = GetArg(param, buf, 64);
        if (len == 1)
        {
            sprintf(buf, "text=%s;", buf);
            append_string(args, buf, 64);
        }
    }
    obj = CreateNull(parent);
    SetPos(obj, x, y, 0.0, 0);
    AttachScript(obj, _, "period_set", args);
    SetPubVar(obj, "g_objManager", Self());
    SetPubVar(obj, "g_objFocus", focus);
    SetPubVar(obj, "g_tagParts", node);
    return obj;
}

AddButton(node, parent, focus)
{
    new Float:x;
    new Float:y;
    new buf[64];
    new obj;
    new len;
    len = domGetData(node, "x", buf, 64);
    if (len == 1)
    {
        x = atof(buf);
    }
    len = domGetData(node, "y", buf, 64);
    if (len == 1)
    {
        y = atof(buf);
    }
    obj = CreateNull(parent);
    SetPos(obj, x, y, 0.0, 0);
    AttachScript(obj, _, "button");
    SetPubVar(obj, "g_objManager", Self());
    SetPubVar(obj, "g_objFocus", focus);
    SetPubVar(obj, "g_tagParts", node);
    return obj;
}

/*
 * Shows the pad help buttons of the "pad_help" element node, each with its label or the default one: circle "Enter",
 * cross "Back", triangle "Option", square "Channel".
 */
SetPadHelp(node)
{
    new mask;
    new label[64];
    new len;
    mask = 0;
    if (0 == node)
    {
        return;
    }
    len = domGetData(node, "circle", label, 64);
    if (len == 1)
    {
        mask = mask | PADHELP_CIRCLE;
        if (0 == strlen(label))
        {
            label{0} = 0xE6;
            label{1} = 0xB1;
            label{2} = 0xBA;
            label{3} = 0xE5;
            label{4} = 0xAE;
            label{5} = 0x9A;
            label{6} = 0;
        }
        SetSystemParamStr(SYS_STR_PADHELP_RR, label);
    }
    len = domGetData(node, "cross", label, 64);
    if (len == 1)
    {
        mask = mask | PADHELP_CROSS;
        if (0 == strlen(label))
        {
            label{0} = 0xE6;
            label{1} = 0x88;
            label{2} = 0xBB;
            label{3} = 0xE3;
            label{4} = 0x82;
            label{5} = 0x8B;
            label{6} = 0;
        }
        SetSystemParamStr(SYS_STR_PADHELP_RD, label);
    }
    len = domGetData(node, "triangle", label, 64);
    if (len == 1)
    {
        mask = mask | PADHELP_TRIANGLE;
        if (0 == strlen(label))
        {
            label{0} = 0xE3;
            label{1} = 0x82;
            label{2} = 0xAA;
            label{3} = 0xE3;
            label{4} = 0x83;
            label{5} = 0x97;
            label{6} = 0xE3;
            label{7} = 0x82;
            label{8} = 0xB7;
            label{9} = 0xE3;
            label{10} = 0x83;
            label{11} = 0xA7;
            label{12} = 0xE3;
            label{13} = 0x83;
            label{14} = 0xB3;
            label{15} = 0;
        }
        SetSystemParamStr(SYS_STR_PADHELP_RU, label);
    }
    len = domGetData(node, "square", label, 64);
    if (len == 1)
    {
        mask = mask | PADHELP_SQUARE;
        if (0 == strlen(label))
        {
            label{0} = 0xE3;
            label{1} = 0x83;
            label{2} = 0x81;
            label{3} = 0xE3;
            label{4} = 0x83;
            label{5} = 0xA3;
            label{6} = 0xE3;
            label{7} = 0x83;
            label{8} = 0xB3;
            label{9} = 0xE3;
            label{10} = 0x83;
            label{11} = 0x8D;
            label{12} = 0xE3;
            label{13} = 0x83;
            label{14} = 0xAB;
            label{15} = 0;
        }
        SetSystemParamStr(SYS_STR_PADHELP_RL, label);
    }
    len = domGetData(node, "l1", label, 64);
    if (len == 1)
    {
        mask = mask | PADHELP_L1;
    }
    len = domGetData(node, "r1", label, 64);
    if (len == 1)
    {
        mask = mask | PADHELP_R1;
    }
    SetSystemParam(SYS_PADHELP, mask);
}

/* Reads into mask the pad help buttons of the "pad_help" child of node; returns 0 if there is none. */
GetPadHelp(node, &mask)
{
    new buf[64];
    new pad_help;
    new len;
    pad_help = find_child_tag(node, "pad_help");
    if (0 == pad_help)
    {
        return 0;
    }
    mask = 0;
    len = domGetData(pad_help, "circle", buf, 64);
    if (len == 1)
    {
        mask = mask | PADHELP_CIRCLE;
    }
    len = domGetData(pad_help, "cross", buf, 64);
    if (len == 1)
    {
        mask = mask | PADHELP_CROSS;
    }
    len = domGetData(pad_help, "triangle", buf, 64);
    if (len == 1)
    {
        mask = mask | PADHELP_TRIANGLE;
    }
    len = domGetData(pad_help, "square", buf, 64);
    if (len == 1)
    {
        mask = mask | PADHELP_SQUARE;
    }
    len = domGetData(pad_help, "l1", buf, 64);
    if (len == 1)
    {
        mask = mask | PADHELP_L1;
    }
    len = domGetData(pad_help, "r1", buf, 64);
    if (len == 1)
    {
        mask = mask | PADHELP_R1;
    }
    return 1;
}

/* Fades the page to alpha over duration ms, then tells the manager "END" and destroys the page. */
Close(Float:alpha, Float:duration)
{
    for (new i; i < g_iElementCount; i++)
    {
        AnimColor(g_objElements[i], g_flElementColors[i][0], g_flElementColors[i][1], g_flElementColors[i][2],
                g_flElementColors[i][3] * alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    }
    for (new i; i < g_iItemCount; i++)
    {
        SendMessage(g_objItems[i], TAG("FADE"), alpha, duration, 0, 0);
    }
    play_se(g_iCloseSE);
    wait_ms(floatround(duration, 0));
    SendMessage(g_objManager, TAG("END"), 0, 0, 0, 0);
    Destroy(Self());
}

/* Returns the index of the item with that id, -1 if there is none. */
FindItem(id[])
{
    new i;
    new buf[64];
    new len;
    i = 0;
    for (; i < g_iItemCount; i++)
    {
        len = domGetData(g_tagItems[i], "id", buf, 64);
        if (0 == len)
        {
            continue;
        }
        if (0 == strcmp(id, buf))
        {
            return i;
        }
    }
    return -1;
}

/* Returns the index of the item named by the "relation" element relation in the direction dir, -1 if none. */
GetNextItem(relation, dir)
{
    new id[64];
    new attr[64];
    new len;
    if (0 == relation)
    {
        return -1;
    }
    switch (dir)
    {
        case DIR_UP:
        {
            attr = "up";
        }
        case DIR_DOWN:
        {
            attr = "down";
        }
        case DIR_LEFT:
        {
            attr = "left";
        }
        case DIR_RIGHT:
        {
            attr = "right";
        }
        default:
        {
            return -1;
        }
    }
    len = domGetData(relation, attr, id, 64);
    if (0 == len)
    {
        return -1;
    }
    if (0 == strlen(id))
    {
        return -1;
    }
    return FindItem(id);
}
