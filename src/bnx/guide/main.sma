/*
 * /opt0/bn/script/guide/main.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The top page of the guide: a menu of four items (what's new, guide, support, readme) that tells the background
 * scene (ground/bg) the item in focus ("FSTR"), the one opened ("DRRG") and the one it came back from ("INRG").
 */

#include <core>
#include <bn>

#pragma dynamic 1280

/* The numbers of the pages of the four items in the guide links (init_guide_links). */
const LINK_WHATSNEW = 3;
const LINK_GUIDE = 4;
const LINK_SUPPORT = 44;
const LINK_README = 53;

/* The items of the guide, g_cursor: the first one cannot be opened. */
const ITEM_WHATSNEW = 1;
const ITEM_GUIDE = 2;
const ITEM_SUPPORT = 3;
const ITEM_README = 4;

#include "tag.inc"

#include <float>

#include "imgsync.inc"

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

#include "string.inc"

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
    xmlLoadStart(url, g_bg_name, _, XMLLOAD_BG);
    wait_xml_load();
}

#include "background.inc"

new g_guide_links2[54][64]; /* by link number, from 1; 0 is not used */

/* Same as init_guide_links of string.inc, with an array of its own. */
init_guide_links2()
{
    g_guide_links2[1] = "ln_bnguide";
    g_guide_links2[2] = "ln_back";
    g_guide_links2[3] = "ln_whatsnew";
    g_guide_links2[4] = "ln_guide";
    g_guide_links2[5] = "ln_guide_beforeuse";
    g_guide_links2[6] = "ln_guide_aboutfee";
    g_guide_links2[7] = "ln_guide_aboutcertify";
    g_guide_links2[8] = "ln_guide_aboutguide";
    g_guide_links2[9] = "ln_guide_whatcan";
    g_guide_links2[10] = "ln_guide_whatcan_guide";
    g_guide_links2[11] = "ln_guide_whatcan_channel";
    g_guide_links2[12] = "ln_guide_whatcan_isp";
    g_guide_links2[13] = "ln_guide_whatcan_internet";
    g_guide_links2[14] = "ln_guide_whatcan_setting";
    g_guide_links2[15] = "ln_guide_operate";
    g_guide_links2[19] = "ln_guide_navim";
    g_guide_links2[20] = "ln_guide_operationm";
    g_guide_links2[16] = "ln_guide_operationm_controler";
    g_guide_links2[17] = "ln_guide_operationm_screenkey";
    g_guide_links2[18] = "ln_guide_operationm_usbkey";
    g_guide_links2[21] = "ln_guide_gamech";
    g_guide_links2[22] = "ln_guide_gamech_play";
    g_guide_links2[23] = "ln_guide_gamech_end";
    g_guide_links2[24] = "ln_guide_gamech_delete";
    g_guide_links2[25] = "ln_guide_musicch";
    g_guide_links2[26] = "ln_guide_musicch_2";
    g_guide_links2[27] = "ln_guide_musicch_record";
    g_guide_links2[28] = "ln_guide_musicch_play";
    g_guide_links2[29] = "ln_guide_musicch_info";
    g_guide_links2[30] = "ln_guide_musicch_edit";
    g_guide_links2[31] = "ln_guide_photoch";
    g_guide_links2[32] = "ln_guide_photoch_save";
    g_guide_links2[33] = "ln_guide_photoch_show";
    g_guide_links2[34] = "ln_guide_photoch_info";
    g_guide_links2[35] = "ln_guide_photoch_edit";
    g_guide_links2[36] = "ln_guide_moviech";
    g_guide_links2[37] = "ln_guide_moviech_play";
    g_guide_links2[38] = "ln_guide_moviech_info";
    g_guide_links2[39] = "ln_guide_setting";
    g_guide_links2[40] = "ln_guide_setting_operateinfo";
    g_guide_links2[41] = "ln_guide_setting_setting";
    g_guide_links2[42] = "ln_guide_setting_netinfo";
    g_guide_links2[43] = "ln_guide_setting_datainfo";
    g_guide_links2[44] = "ln_support";
    g_guide_links2[45] = "ln_support_faq";
    g_guide_links2[46] = "ln_support_faq_a";
    g_guide_links2[47] = "ln_support_map";
    g_guide_links2[48] = "ln_support_map_a";
    g_guide_links2[49] = "ln_support_term";
    g_guide_links2[50] = "ln_support_term_a";
    g_guide_links2[51] = "ln_support_question";
    g_guide_links2[52] = "ln_support_question_a";
    g_guide_links2[53] = "ln_readme";
}

new Float:g_title_pos[4] = {50.0, 144.0, 0.0, 1.0};
new Float:g_title_color[4] = {0.9686, 0.7137, 0.2352, 1.0};
new g_title_font_size = 17;
new Float:g_item_pos[4] = {90.0, 190.0, 0.0, 1.0};
new Float:g_item_color[4] = {0.215, 0.708, 0.999, 1.0};
new g_item_font_size = 17;
new g_last_font_size = 15;
new Float:g_dim_alpha = 0.4;
new Float:g_update_pos[4] = {260.0, 190.0, 0.0, 1.0};
new Float:g_update_color[4] = {0.7843, 0.7843, 0.7843, 0.8};
new g_update_font_size = 15;
new Float:g_update_gap = 16.0;
public Terminate; /* the object whose thread (TerminateThread) leaves the page */
public isTerminate; /* set by CallTerminate: TerminateThread leaves the page */

/* Goes back to the top menu and closes the scene. */
public terminate()
{
    printf("******************************\n");
    printf("terminate callback\n");
    printf("******************************\n");
    xmlLoadStart("$$BUILDIN_TOPMENU$$", _, _, XMLLOAD_MAIN);
    wait_xml_load();
    Abort(Scene(0));
}

/* Stops this script and tells TerminateThread to leave the page. */
public CallTerminate()
{
    Abort(Self());
    for (; ; )
    {
        isTerminate = 1;
        sleep;
    }
}

/* Creates the Terminate object, whose thread goes back to the top menu once isTerminate is set. */
public CreateTerminate(parent)
{
    Terminate = CreateNull(parent);
    isTerminate = 0;
    AttachScript(Terminate, "TerminateThread", "");
}

/* Waits until isTerminate is set, then leaves the page like terminate. */
public TerminateThread()
{
    new msg;
    for (; ; )
    {
        if (isTerminate)
        {
            break;
        }
        while (RecvMessage(msg))
        {
        }
    }
    printf("******************************\n");
    printf("terminate callback\n");
    printf("******************************\n");
    xmlLoadStart("$$BUILDIN_TOPMENU$$", _, _, XMLLOAD_MAIN);
    wait_xml_load();
    Abort(Scene(0));
}

new g_focus;
new g_focused;
new g_items[5]; /* the title, then the four items */
new g_update_labels[4];
new g_args[256];
new g_first_item = 1; /* the item of the argument "LnkNum", when coming back from it */
new g_cursor = ITEM_WHATSNEW;
new g_bg_scene;
new g_item_tags[4][2] = {"WNew", "Guid", "Sprt", "Rdme"}; /* the rings of the background */

/* Opens the page of item and ends the script. */
open_item(item)
{
    new link;
    if (item == ITEM_WHATSNEW)
    {
        link = LINK_WHATSNEW;
    }
    if (item == ITEM_GUIDE)
    {
        link = LINK_GUIDE;
    }
    if (item == ITEM_SUPPORT)
    {
        link = LINK_SUPPORT;
    }
    if (item == ITEM_README)
    {
        link = LINK_README;
    }
    new url[64];
    get_tag_attr("LINK", g_guide_links2[link], "href", url, 64);
    sprintf(g_args, "LnkNum=0;");
    xmlLoadStart(url, _, g_args, XMLLOAD_MAIN);
    wait_xml_load();
    Destroy(Self());
}

/* Opens the section of item in the background scene, then its page. */
select_item(item)
{
    if (IsValidObject(g_bg_scene))
    {
        SendMessage(g_bg_scene, TAG("DRRG"), TAG(g_item_tags[item - 1]), 0, 0, 0);
    }
    open_item(item);
}

main()
{
    hisClear();
    load_background("bg");
    fade_in_background(2000.0);
    g_bg_scene = wndGetScene(WND_BG);
    if (IsValidObject(g_bg_scene))
    {
        SendMessage(g_bg_scene, TAG("DRRG"), TAG("None"), 0, 0, 0);
    }
    else
    {
        printf("objBg Making was Incompleted. \n");
    }
    SetPos(Camera(0), 0.0, 0.0, 300.0, 0);
    LookAt(Camera(0), Scene(0), 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
    g_focus = create_focus_plane2("focus");
    if (GetArg("LnkNum", g_args, 256))
    {
        g_cursor = g_first_item = atoi(g_args);
    }
    g_cursor = g_first_item = clamp(g_first_item, ITEM_GUIDE, ITEM_README);
    init_guide_links2();
    create_menu();
    SetSystemParam(SYS_PADHELP, PADHELP_CIRCLE | PADHELP_CROSS);
    g_focused = g_items[g_first_item];
    set_focus(g_focus, g_focused, 0, 0.5, 60.0, 60.0, 100.0, 1, 1);
    if (IsValidObject(g_bg_scene))
    {
        SendMessage(g_bg_scene, TAG("INRG"), TAG(g_item_tags[g_first_item - 1]), 0, 0, 0);
    }
    new scroll_tick;
    new scrolling;
    for (; ; )
    {
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
        /* Up and down move between the items, circle opens one (not the first), cross leaves the guide. */
        if (repeated & PAD_DOWN && 0 == scrolling)
        {
            if (g_cursor < ITEM_README)
            {
                g_cursor++;
                scrolling = 1;
                scroll_tick = tickcount();
                sndSePlay(SE_CURSOR);
                if (g_cursor == ITEM_WHATSNEW)
                {
                    SetSystemParam(SYS_PADHELP, PADHELP_CROSS);
                }
                else
                {
                    SetSystemParam(SYS_PADHELP, PADHELP_CIRCLE | PADHELP_CROSS);
                }
                g_focused = g_items[g_cursor];
                if (IsValidObject(g_bg_scene))
                {
                    SendMessage(g_bg_scene, TAG("FSTR"), TAG(g_item_tags[g_cursor - 1]), 0, 0, 0);
                }
                set_focus(g_focus, g_focused, 0, 0.5, 60.0, 60.0, 100.0, 1, 1);
            }
        }
        else if (repeated & PAD_UP && 0 == scrolling)
        {
            if (ITEM_WHATSNEW < g_cursor)
            {
                g_cursor--;
                scrolling = 1;
                scroll_tick = tickcount();
                sndSePlay(SE_CURSOR);
                if (g_cursor == ITEM_WHATSNEW)
                {
                    SetSystemParam(SYS_PADHELP, PADHELP_CROSS);
                }
                else
                {
                    SetSystemParam(SYS_PADHELP, PADHELP_CIRCLE | PADHELP_CROSS);
                }
                g_focused = g_items[g_cursor];
                if (IsValidObject(g_bg_scene))
                {
                    SendMessage(g_bg_scene, TAG("FSTR"), TAG(g_item_tags[g_cursor - 1]), 0, 0, 0);
                }
                set_focus(g_focus, g_focused, 0, 0.5, 60.0, 60.0, 100.0, 1, 1);
            }
        }
        else if (pressed & PAD_CIRCLE && ITEM_WHATSNEW != g_cursor)
        {
            sndSePlay(SE_OPEN);
            for (new i; i < 5; i++)
            {
                SetPos(g_items[i], 6400 + g_item_pos[0], 4800 + g_item_pos[1], g_item_pos[2], 0);
            }
            SetPos(g_update_labels[0], 6400 + g_update_pos[0], 4800 + g_update_pos[1], g_update_pos[2], 0);
            SetSystemParam(SYS_PADHELP, 0);
            select_item(g_cursor);
        }
        else if (pressed & PAD_CROSS)
        {
            sndSePlay(SE_CANCEL);
            sndBgmStop(1);
            SetSystemParam(SYS_PADHELP, 0);
            return xmlLoadStart("$$BUILDIN_TOPMENU$$", _, _, XMLLOAD_MAIN);
        }
        new msg;
        new a;
        new b;
        new c;
        while (RecvMessage(msg, a, b, c))
        {
        }
        sleep;
    }
    return 0;
}

/* Creates a transparent plane with text rendered at font_size, its left edge at x. */
create_text_plane(text[], font_size, Float:x, Float:y, Float:z)
{
    new plane = CreatePlane(Disp2D(0));
    imgDecStringStart(text, font_size, 64, 64, 640);
    if (IMG_DONE != wait_image_decode())
    {
        terminate();
    }
    SetTexture(plane, imgTexID(), 0, -1);
    SetLocalScale(plane, float(imgTexWidth()), float(imgTexHeight()), 1.0);
    x = x + float(imgTexWidth()) / 2.0;
    SetPos(plane, x, y, z, 0);
    SetColor(plane, 0.0, 0.0, 0.0, 0.0);
    return plane;
}

/* Creates the title, the four items and the note beside the first item from the TEXT elements of the page. */
create_menu()
{
    new text[256];
    new item;
    get_group_tag_attr2("TEXT", "menu_item", item, "value", text, 256);
    g_items[0] = create_text_plane(text, g_title_font_size, g_title_pos[0], g_title_pos[1], g_title_pos[2]);
    item = 1;
    for (; item < 5; item++)
    {
        get_group_tag_attr2("TEXT", "menu_item", item, "value", text, 256);
        g_items[item] = create_text_plane(text, g_item_font_size, g_item_pos[0],
                g_item_pos[1] + (28.0 + 16) * (item - 1), g_item_pos[2]);
    }
    /* NOTE: bug: the loop above already made item 4; that plane stays, transparent */
    get_group_tag_attr2("TEXT", "menu_item", 4, "value", text, 256);
    g_items[4] = create_text_plane(text, g_last_font_size, g_item_pos[0],
            g_item_pos[1] + 3 * (28.0 + 16) + (g_item_font_size - g_last_font_size) / 2, g_item_pos[2]);
    {
        new Float:item_w;
        new Float:item_h;
        new Float:item_d;
        GetLocalScale(g_items[1], item_w, item_h, item_d);
        get_group_tag_attr2("TEXT", "update_item", 0, "value", text, 256);
        g_update_labels[0] = create_text_plane(text, g_update_font_size, g_item_pos[0] + g_update_gap + item_w,
                g_update_pos[1] + (g_item_font_size - g_update_font_size) / 2, g_update_pos[2]);
    }
    SetColor(g_items[0], g_title_color[0], g_title_color[1], g_title_color[2], g_title_color[3]);
    for (new i = 1; i < 5; i++)
    {
        SetColor(g_items[i], g_item_color[0], g_item_color[1], g_item_color[2], g_item_color[3]);
    }
    SetColor(g_items[1], g_item_color[0], g_item_color[1], g_item_color[2], g_dim_alpha);
    SetColor(g_items[4], g_item_color[0], g_item_color[1], g_item_color[2], g_item_color[3]);
    SetColor(g_update_labels[0], g_update_color[0], g_update_color[1], g_update_color[2], g_update_color[3]);
    SetColor(g_update_labels[0], g_update_color[0], g_update_color[1], g_update_color[2], g_dim_alpha);
}
