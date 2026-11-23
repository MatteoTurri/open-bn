/*
 * /opt0/bn/script/guide/text_tex.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A text page of the guide: the text areas "textvalue" and "info" of the page, scrolled with up and down, whose
 * lines are links to other pages of the guide.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "tag.inc"

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

#include "xmlsync.inc"

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

#include "focus.inc"

new g_bg_name[] = "BB_GUIDE:CommonBG";

/* Loads the page of the LINK link_id into the background window, unless the guide background is there. */
load_background(link_id[])
{
    {
        new name[256];
        if (wndGetName(WND_BG, name, 256) && 0 == strcmp(name, g_bg_name))
        {
            return;
        }
    }
    new url[256];
    get_tag_attr("LINK", link_id, "href", url, 256);
    xmlLoadStart(url, g_bg_name, "", XMLLOAD_BG);
    wait_xml_load();
}

#include "string.inc"

new Float:g_ring_rot0[3] = {-0.54615, -0.602854, -3.124533};
new Float:g_ring_rot1[3] = {-0.369934, 0.122705, -1.96844};
new Float:g_ring_rot2[3] = {-0.583142, -0.583142, 1.954143};
new Float:g_default_scale[4] = {1.0, 1.0, 1.0, 1.0};
new Float:g_default_color[4] = {1.0, 1.0, 1.0, 1.0};
new Float:g_default_menu_pos[4] = {130.0, 160.0, 0.0, 3.0};
new Float:g_bar_width = 10.0;
new g_focus;
new g_text_area;
new g_menu_labels[3];
new g_root;
new g_text_areas[2];
new g_ring_id;
new g_color[4];
new Float:g_menu_pos[4];
new Float:g_ring_rot[3];
new g_scale[4];
new g_arg[64];
new Float:g_rot;
new g_visible;
new Float:g_area_x;
new Float:g_area_y;
new Float:g_area_z;
new g_align;
new g_valign = -1;
new g_line_focus[16];
new g_skip_lines;
new g_max_line;
new g_line;

main()
{
    hisClear();
    load_background("bg_water");
    new self = Self();
    new value[20];
    new area_ids[2][20];
    g_root = CreateNull(self);
    SetPos(g_root, 0.0, 0.0, 0.0, 0);
    SetScale(g_root, 1.0, 1.0, 1.0);
    new camera = Camera(0);
    SetPos(camera, 0.0, 0.0, 300.0, 0);
    LookAt(camera, Scene(0), 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
    g_focus = create_focus_plane2("focus");
    if (GetArg("Rot", g_arg, 64))
    {
        g_rot = atof(g_arg);
    }
    init_guide_links();
    init_layout();
    create_menu_labels();
    copy_string(area_ids[0], "textvalue", 20);
    copy_string(area_ids[1], "info", 20);
    create_text_areas(area_ids, 2);
    SetSystemParam(SYS_PADHELP, PADHELP_CIRCLE | PADHELP_CROSS);
    g_text_area = g_text_areas[0];
    SendMessage(g_text_area, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
    get_tag_attr("TEXTAREA", area_ids[0], "skipline", value, 20);
    g_skip_lines = atoi(value);
    new link_num;
    if (GetArg("LnkNum", g_arg, 64))
    {
        link_num = atoi(g_arg);
        g_max_line = link_num - 1;
        SendMessage(g_text_area, TAG("NMES"), link_num * g_skip_lines, 0, 0, 0);
        for (new i; i < g_skip_lines * link_num; i++)
        {
            SendMessage(g_text_area, TAG("FOCS"), TAG("LDWN"), 0, 0, 0);
        }
    }
    for (; ; )
    {
        g_max_line = GetPubVar(g_text_areas[0], "max_line_number");
        if (0 != g_max_line)
        {
            break;
        }
        sleep;
    }
    finish_init();
    new scroll_tick;
    new scrolling;
    for (; ; )
    {
        new msg;
        new a;
        new b;
        new c;
        new i;
        new pressed;
        new repeated;
        PadState(_, pressed, _, repeated, -1);
        if (scrolling == 1)
        {
            if (tickcount() - scroll_tick >= 100)
            {
                scrolling = 0;
            }
        }
        /* Up and down scroll the text by skipline lines, at most once every 100 ms. */
        if (repeated & PAD_DOWN && 0 == scrolling)
        {
            if (g_line != g_max_line - 1)
            {
                sndSePlay(SE_CURSOR);
                g_line = g_line + g_skip_lines;
                scrolling = 1;
                scroll_tick = tickcount();
            }
            SendMessage(g_text_area, TAG("NMES"), g_skip_lines, TAG("NOSD"), 0, 0);
            i = 0;
            for (; i < g_skip_lines; i++)
            {
                SendMessage(g_text_area, TAG("FOCS"), TAG("LDWN"), 0, 0, 0);
            }
        }
        else if (repeated & PAD_UP && 0 == scrolling)
        {
            if (0 != g_line)
            {
                sndSePlay(SE_CURSOR);
                g_line = g_line - g_skip_lines;
                scrolling = 1;
                scroll_tick = tickcount();
            }
            SendMessage(g_text_area, TAG("NMES"), g_skip_lines, TAG("NOSD"), 0, 0);
            i = 0;
            for (; i < g_skip_lines; i++)
            {
                SendMessage(g_text_area, TAG("FOCS"), TAG("LUP"), 0, 0, 0);
            }
        }
        /* Circle follows the link ln_<n> of the line under the cursor; cross goes back to ln_back. */
        else if (pressed & PAD_CIRCLE)
        {
            sndSePlay(SE_OPEN);
            new cursor;
            new link_id[256];
            cursor = GetPubVar(g_text_area, "cursor");
            cursor++;
            sprintf(link_id, "ln_%d", cursor);
            new args[64];
            new url[64];
            get_tag_attr("LINK", link_id, "href", url, 64);
            sprintf(args, "Rot=%f;LnkNum=0", g_ring_rot[2]);
            SetPos(g_text_areas[0], g_area_x + 1000, g_area_y, g_area_z, 0);
            SetPos(g_text_areas[1], g_area_x + 1000, g_area_y, g_area_z, 0);
            SetSystemParam(SYS_PADHELP, 0);
            xmlLoadStart(url, _, args, XMLLOAD_MAIN);
            wait_xml_load();
            Destroy(Self());
        }
        else if (pressed & PAD_CROSS)
        {
            sndSePlay(SE_CANCEL);
            new args[64];
            new url[64];
            new back_line;
            get_tag_attr("LINK", "ln_back", "href", url, 64);
            get_tag_attr("LINK", "ln_back", "LnkNum", value, 20);
            back_line = atoi(value);
            sprintf(args, "Rot=%f;LnkNum=%d", g_ring_rot[2], back_line);
            SetPos(g_text_areas[0], g_area_x + 1000, g_area_y, g_area_z, 0);
            SetPos(g_text_areas[1], g_area_x + 1000, g_area_y, g_area_z, 0);
            SetSystemParam(SYS_PADHELP, 0);
            xmlLoadStart(url, _, args, XMLLOAD_MAIN);
            wait_xml_load();
            Destroy(Self());
        }
        while (RecvMessage(msg, a, b, c))
        {
        }
        sleep;
    }
    return;
}

/* Takes the rotation of the ring of the page ("ring_id" of the image "img_RingID") and the place of the menu. */
init_layout()
{
    new value[4];
    get_tag_attr("IMG", "img_RingID", "ring_id", value, 4);
    g_ring_id = atoi(value);
    switch (g_ring_id)
    {
        case 0:
        {
            for (new i; i < 2; i++)
            {
                g_ring_rot[i] = g_ring_rot0[i];
            }
        }
        case 1:
        {
            for (new i; i < 2; i++)
            {
                g_ring_rot[i] = g_ring_rot1[i];
            }
        }
        case 2:
        {
            for (new i; i < 2; i++)
            {
                g_ring_rot[i] = g_ring_rot2[i];
            }
        }
    }
    g_ring_rot[2] = g_rot;
    for (new i; i < 4; i++)
    {
        g_color[i] = g_default_color[i];
        g_menu_pos[i] = g_default_menu_pos[i];
    }
    for (new i; i < 3; i++)
    {
        g_scale[i] = g_default_scale[i];
    }
}

/* Creates the three labels of the menu (the "menu_item" texts) on the right. */
create_menu_labels()
{
    new parent = CreateNull(Disp2D(0));
    {
        new text[256];
        new args[256];
        for (new i; i < 3; i++)
        {
            g_menu_labels[i] = CreateLabel(parent);
            get_group_tag_attr2("TEXT", "menu_item", i, "value", text, 256);
            SetPos(g_menu_labels[i], 70.0 + g_menu_pos[0], -(g_menu_pos[1] + (25.0 + 17) * (i - 1)), g_menu_pos[2], 0);
            SetScale(g_menu_labels[i], 1.0, 1.0, 1.0);
            SetColor(g_menu_labels[i], 0.9019, 0.9019, 0.2156, 1.0);
            SetOptionI(g_menu_labels[i], TAG("FONT"), FontID(0));
            SetOptionS(g_menu_labels[i], TAG("UTF8"), text);
            SetOptionI(g_menu_labels[i], TAG("FSIZ"), 17);
            SetOptionI(g_menu_labels[i], TAG("HALN"), ALIGN_RIGHT);
            sprintf(args, "label_string = %s ; index = %d", text, i);
            AttachScript(g_menu_labels[i], "label_item", "label_item", args);
            SetPubVar(g_menu_labels[i], "focus", g_focus);
        }
    }
}

/* Creates a text area for each TEXTAREA element of ids, laid out by its attributes. */
create_text_areas(ids[][], count)
{
    new args[256];
    new scrollbar[16] = "true";
    new bar_width = g_bar_width;
    new value[20];
    new Float:width;
    new i;
    i = 0;
    for (; i < count; i++)
    {
        get_tag_attr("TEXTAREA", ids[i], "skipline", value, 20);
        g_skip_lines = atoi(value);
        get_tag_attr("TEXTAREA", ids[i], "visible", value, 20);
        g_visible = atoi(value);
        get_tag_attr("TEXTAREA", ids[i], "x", value, 20);
        g_area_x = float(atoi(value));
        get_tag_attr("TEXTAREA", ids[i], "y", value, 20);
        g_area_y = float(atoi(value));
        get_tag_attr("TEXTAREA", ids[i], "z", value, 20);
        g_area_z = float(atoi(value));
        get_tag_attr("TEXTAREA", ids[i], "align", value, 20);
        if (0 == strcmp(value, "left"))
        {
            g_align = ALIGN_LEFT;
        }
        else if (0 == strcmp(value, "center"))
        {
            g_align = ALIGN_CENTER;
        }
        else if (0 == strcmp(value, "right"))
        {
            g_align = ALIGN_RIGHT;
        }
        get_tag_attr("TEXTAREA", ids[i], "valign", value, 20);
        if (0 == strcmp(value, "center"))
        {
            g_valign = ALIGN_CENTER;
        }
        get_tag_attr("TEXTAREA", ids[i], "focus", g_line_focus, 16);
        if (0 == strcmp(g_line_focus, ""))
        {
            copy_string(g_line_focus, "false", 16);
        }
        get_tag_attr("TEXTAREA", ids[i], "scrollbar", scrollbar, 16);
        get_tag_attr("TEXTAREA", ids[i], "width", value, 20);
        width = float(atoi(value));
        g_text_areas[i] = CreateNull(Disp2D(0));
        if (g_align == ALIGN_CENTER)
        {
            g_area_x = g_area_x + width / 2;
        }
        else if (g_align == ALIGN_RIGHT)
        {
            g_area_x = g_area_x + width;
        }
        if (g_valign == ALIGN_CENTER)
        {
            g_area_y = float(224 - g_visible * 20 / 2);
        }
        SetPos(g_text_areas[i], g_area_x, g_area_y, g_area_z, 0);
        SetOptionI(g_text_areas[i], TAG("ZSRT"), ZSORT_OVERLAY);
        sprintf(args, "id=%s;line_focus=%s;visible=%d", ids[i], g_line_focus, g_visible);
        AttachScript(g_text_areas[i], _, "textarea", args);
        if (0 == strcmp(scrollbar, "true"))
        {
            SetPubVar(g_text_areas[i], "sb", 1);
        }
        else
        {
            SetPubVar(g_text_areas[i], "sb", 0);
        }
        SetPubVar(g_text_areas[i], "font_align", g_align);
        SetPubVar(g_text_areas[i], "bar_width", bar_width);
        SetPubVar(g_text_areas[i], "manager", Self());
        SetPubVar(g_text_areas[i], "focus", g_focus);
        SetPubVar(g_text_areas[i], "width", width);
    }
}

finish_init()
{
}
