/*
 * /opt0/bn/script/feegaguide/menu.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A menu page of the FeeGa member guide: a title and a text area whose lines are links; circle opens the page of
 * the line, a level deeper, and cross goes back to the page above, both passing on the member arguments.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

/*
 * The commands of a line of create_line: the characters skipped from the letter of a command to its first value,
 * and from the closing quote of a value to the next one.
 */
const MARKUP_IMAGE = 9;     /* IMG src=" */
const MARKUP_COLOR = 8;     /* from F to the value of red */
const MARKUP_ACTION = 15;   /* from A to the action number */
const MARKUP_NEXT_VALUE = 5; /* " x=" */

forward Float:get_line_y(obj);

#include "tag.inc"

#include <float>

#include "imgsync.inc"

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
    AttachScript(Terminate, "TerminateThread");
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

new Float:g_title_pos[4] = {40.0, 87.0, 0.0, 1.0};
new Float:g_title_color[4] = {0.9686, 0.7137, 0.2352, 1.0};
new g_title_font_size = 17;
new Float:g_area_pos[4] = {80.0, 240.0, 0.0, 1.0};
new Float:g_area_height = 268.0;

/* Opens url, a level deeper, and ends the script. */
open_child_page(url[], args[])
{
    xmlLoadStart(url, _, args, XMLLOAD_MAIN);
    wait_xml_load();
    Destroy(Self());
}

/* Opens url, a level up, and ends the script. */
open_parent_page(url[], args[])
{
    xmlLoadStart(url, _, args, XMLLOAD_MAIN);
    wait_xml_load();
    Destroy(Self());
}

/* Creates the title of the page, the first "menu_item" text, at g_title_pos. */
create_page_title(parent)
{
    new text[64];
    new plane;
    text{0} = 0;
    if (!get_group_tag_attr2("TEXT", "menu_item", 0, "value", text, 64))
    {
        return;
    }
    if (!strlen(text))
    {
        return;
    }
    plane = CreatePlane(parent);
    imgDecStringStart(text, g_title_font_size, 64, 64, 640);
    if (IMG_DONE != wait_image_decode())
    {
        terminate();
    }
    SetLocalScale(plane, float(imgTexWidth()), float(imgTexHeight()), 1.0);
    SetPos(plane, g_title_pos[0] + float(imgTexWidth()) / 2.0, g_title_pos[1] + float(imgTexHeight()) / 2.0,
            g_title_pos[2], 0);
    SetColor(plane, g_title_color[0], g_title_color[1], g_title_color[2], g_title_color[3]);
    SetTexture(plane, imgTexID(), 0, -1);
}

/*
 * The text area, run by TextAreaScript on an object of its own: the children of the TEXTAREA element "id",
 * one line each, of which only the lines in view and one more have objects. With line_focus a cursor moves
 * on the lines; without it the text scrolls and the focus stops on the action lines. Circle sends "DECP" and
 * cross "RETP" (cursor, top line, index entry) to the manager.
 */
public manager; /* told "DECP" and "RETP" */
public focus;   /* the focus object */
public padhelp; /* unused, but part of the layout */
public active; /* 1 once the cursor of line_focus is on the lines */
public cursor;      /* line of the cursor */
public where;       /* without line_focus, the line to keep in view */
new g_line_count;   /* lines of the text */
public visible = 8; /* lines shown */
public Float:width = 400.0; /* width of the lines, in pixels */
public sb = 1;                /* with a scroll bar */
public line_focus;            /* a cursor on the lines; without it the text scrolls */
public Float:interval = 20.0; /* pixels from a line to the next */
public font_align;            /* ALIGN_* */
public fontsize = 15; /* font size of the lines, in pixels */
public Float:meta_alpha = 0.8; /* unused, but part of the layout */
public enable_pad = 1;   /* 0 while the page handles a message */
public scroll_speed = 1; /* duration of a scroll step, in ms */
public action_id = -1;   /* number of the action line in focus, -1 for none */
new g_list_top;          /* line of the first line object */
new g_top_line;          /* first line shown */
new g_text_node;
new g_line_node; /* the element of line g_line_node_index */
new g_line_node_index;
new g_text_id[32];
new g_scroll_bar;
new g_moved_up; /* the last scroll without line_focus went up: where is the top line in view */
new g_moved_down = 1; /* it went down: where is the bottom line in view */
new g_page_line; /* unused, but part of the layout */
new g_at_top = 1;
new g_images[4];      /* the images of the lines, by slot */
new g_image_lines[4]; /* line of each image, -1 for a free slot */
new Float:g_image_x[4];
new Float:g_image_y[4];
new Float:g_image_h[4];
new Float:g_image_shown_h[4];
new Float:g_image_trim_y[4];
new Float:g_known_image_h[32]; /* height of the image of each line that has one */
new g_known_image_lines[32];
new g_action_lines[32]; /* the lines with an action, and the number of each */
new g_action_ids[32];
new g_action_count;
new g_action;           /* index of the action line in focus, -1 for none */
new g_index_steps[128]; /* lines of each entry of the index */
public intIndexNum;     /* entries of the index, 0 for no index */
public intIndexPointer; /* the entry in focus */
public Float:voffset;   /* from the top of a line to its center */
new g_line_read[256];   /* the text of the line is in g_line_texts */
new g_line_texts[256][64];
new g_focus_setter;
new g_list_root; /* parent of the line objects, scrolled */
new g_last_tick; /* the tickcount of the frame before g_tick, for update_scroll_lag */
new g_tick;
new g_scrolling_up;
new g_scrolling_down;
new Float:g_scroll_lag; /* distance the list scrolls in this frame */

/* Resets the state of the text area. */
init_text_area()
{
    new i;
    i = 0;
    for (; i < 4; i++)
    {
        g_images[i] = 0;
        g_image_lines[i] = -1;
    }
    i = 0;
    for (; i < 32; i++)
    {
        g_known_image_h[i] = 0.0;
        g_known_image_lines[i] = -1;
    }
    i = 0;
    for (; i < 32; i++)
    {
        g_action_lines[i] = -1;
        g_action_ids[i] = 0;
    }
    g_action_count = 0;
    g_action = -1;
    action_id = -1;
    i = 0;
    for (; i < 256; i++)
    {
        g_line_read[i] = 0;
    }
    voffset = float(fontsize) / 2.0;
    i = 0;
    for (; i < 128; i++)
    {
        g_index_steps[i] = 1;
    }
    intIndexNum = 0;
    intIndexPointer = 0;
    g_focus_setter = CreateNull(Self());
    g_list_root = CreateNull(Self());
    SetPos(g_list_root, 0.0, 0.0, 0.0, 0);
}

/* Reads the arguments id, sb, visible, cursor_pos and line_focus. */
read_text_area_args()
{
    new value[64];
    GetArg("id", g_text_id, 32);
    GetArg("sb", value, 64);
    if (0 != strcmp(value, ""))
    {
        if (0 == strcmp(value, "true"))
        {
            sb = 1;
        }
        else
        {
            sb = 0;
        }
    }
    GetArg("visible", value, 64);
    if (0 != strcmp(value, ""))
    {
        visible = atoi(value);
    }
    GetArg("cursor_pos", value, 64);
    if (0 != strcmp(value, ""))
    {
        cursor = atoi(value);
    }
    else
    {
        cursor = 0;
    }
    GetArg("line_focus", value, 64);
    if (0 == strcmp(value, "true"))
    {
        line_focus = 1;
    }
    else
    {
        line_focus = 0;
    }
}

create_scroll_bar()
{
    if (sb == 1)
    {
        g_scroll_bar = CreateNull(Self());
        new Float:x;
        new Float:y;
        new Float:z;
        GetPos(Self(), x, y, z, 0);
        if (ALIGN_LEFT == font_align)
        {
            SetPos(g_scroll_bar, -25.0 - 5.0 / 2.0, (interval * float(visible) - (interval - fontsize)) / 2.0, z, 0);
        }
        else if (font_align == ALIGN_CENTER)
        {
            SetPos(g_scroll_bar, -25.0 - 5.0 / 2.0 - width / 2.0,
                    (interval * float(visible) - (interval - fontsize)) / 2.0, z, 0);
        }
        else
        {
            SetPos(g_scroll_bar, -25.0 - 5.0 / 2.0 - width, (interval * float(visible) - (interval - fontsize)) / 2.0,
                    z, 0);
        }
        SetOptionI(g_scroll_bar, TAG("ZSRT"), ZSORT_NONE);
        AttachScript(g_scroll_bar, _, "scroll_bar");
        SetPubVar(g_scroll_bar, "manager", manager);
        SetPubVar(g_scroll_bar, "focus", focus);
        SetPubVar(g_scroll_bar, "height", interval * visible - (interval - fontsize));
        SetPubVar(g_scroll_bar, "top", 0);
        SetPubVar(g_scroll_bar, "length", float(visible) / float(g_line_count));
    }
}

/*
 * Counts the lines and reads the index, the "value" of the TEXTAREA (the lines of each entry, each count
 * followed by ';'), then sets the top line from the cursor.
 */
read_lines()
{
    new value[64];
    new i;
    g_text_node = domFindTag(domRoot(), "TEXTAREA", g_text_id, 0);
    g_line_count = count_children(g_text_node);
    visible = clamp(visible, 0, g_line_count);
    if (get_tag_attr("TEXTAREA", g_text_id, "value", value, 64))
    {
        i = 0;
        for (; 0 != value{i}; i++)
        {
            g_index_steps[intIndexNum] = 0;
            while (';' != value{i})
            {
                g_index_steps[intIndexNum] = g_index_steps[intIndexNum] * 10 + value{i} - '0';
                i++;
            }
            intIndexNum++;
        }
        g_top_line = 0;
        i = 0;
        for (; i < cursor; i++)
        {
            g_top_line = g_top_line + g_index_steps[i];
            intIndexPointer++;
        }
    }
    else
    {
        g_top_line = max(0, cursor - (visible - 1));
    }
    seek_top_line(g_text_node);
    g_list_top = g_top_line;
    g_page_line = g_top_line;
    if (!line_focus)
    {
        where = g_top_line + (visible - 1);
        if (0 < g_top_line)
        {
            g_at_top = 0;
        }
    }
}

/* Creates the objects of the lines in view and of one more. */
create_lines()
{
    new i;
    SetPos(g_list_root, 0.0, -float(g_top_line) * interval, 0.0, 0);
    i = 0;
    for (; i < visible + 1; i++)
    {
        new line = i + g_list_top;
        if (g_line_count <= line)
        {
            break;
        }
        read_line_text(line, g_line_texts[line]);
        new line_obj = create_line(line, g_line_texts[line], "INIT");
        listAddLast(line_obj);
    }
}

/* Waits until every line object has rendered its text. */
wait_lines_ready()
{
    for (new i; ; )
    {
        new busy;
        i = 0;
        for (; i < listSize(); i++)
        {
            new line_obj = listGetAt(i);
            if (SCRIPT_NONE != ScriptState(line_obj))
            {
                busy = 1;
            }
        }
        if (!busy)
        {
            break;
        }
        else
        {
            sleep;
        }
    }
}

/* Moves the cursor a line up; returns whether it moved. */
cursor_up()
{
    if (cursor > 0)
    {
        cursor--;
        on_step();
        scroll_to_cursor();
        focus_cursor_line();
        return 1;
    }
    return 0;
}

/* Without a cursor, scrolls a line up: where becomes the top line in view, then goes up. */
scroll_up()
{
    if (where > 0 && !g_at_top)
    {
        g_page_line--;
        if (g_moved_down)
        {
            where = where - visible;
            g_moved_up = 1;
            g_moved_down = 0;
        }
        else
        {
            where--;
            g_moved_up = 1;
            g_moved_down = 0;
        }
        on_step();
        scroll_to_cursor();
        update_action_focus("UP");
        return 1;
    }
    update_action_focus("UP");
    return 0;
}

step_up()
{
    if (line_focus)
    {
        return cursor_up();
    }
    else
    {
        return scroll_up();
    }
    return 0;
}

/* Moves a step up, or with an index an entry up, then puts the focus there; returns whether it moved. */
move_up()
{
    new moved;
    if (!intIndexNum)
    {
        moved = step_up();
    }
    else
    {
        if (0 < intIndexPointer)
        {
            on_index_up();
            set_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        }
        for (new i; 0 < intIndexPointer && i < g_index_steps[intIndexPointer - 1]; i++)
        {
            if (step_up())
            {
                moved = 1;
            }
        }
        if (0 < intIndexPointer)
        {
            intIndexPointer--;
            set_focus_later(g_focus_setter, focus, listGetAt(g_top_line - g_list_top), 150.0, 1);
        }
    }
    return moved;
}

cursor_down()
{
    if (cursor < g_line_count - 1)
    {
        cursor++;
        on_step();
        scroll_to_cursor();
        focus_cursor_line();
        return 1;
    }
    return 0;
}

/* Without a cursor, scrolls a line down: where becomes the bottom line in view, then goes down. */
scroll_down()
{
    if (where < g_line_count - 1)
    {
        g_page_line++;
        if (g_at_top)
        {
            g_at_top = 0;
        }
        if (g_moved_up)
        {
            where = where + visible;
            g_moved_up = 0;
            g_moved_down = 1;
        }
        else
        {
            where++;
            g_moved_up = 0;
            g_moved_down = 1;
        }
        on_step();
        scroll_to_cursor();
        update_action_focus("DOWN");
        return 1;
    }
    update_action_focus("DOWN");
    return 0;
}

step_down()
{
    if (line_focus)
    {
        return cursor_down();
    }
    else
    {
        return scroll_down();
    }
    return 0;
}

/* Moves a step down, or with an index an entry down, then puts the focus there; returns whether it moved. */
move_down()
{
    new moved;
    if (!intIndexNum)
    {
        moved = step_down();
    }
    else
    {
        if (intIndexPointer < intIndexNum - 1)
        {
            on_index_down();
            set_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        }
        for (new i; intIndexPointer < intIndexNum - 1 && i < g_index_steps[intIndexPointer]; i++)
        {
            if (step_down())
            {
                moved = 1;
            }
        }
        if (intIndexPointer < intIndexNum - 1)
        {
            intIndexPointer++;
            set_focus_later(g_focus_setter, focus, listGetAt(g_top_line - g_list_top), 150.0, 1);
        }
    }
    return moved;
}

/* Moves focus_obj onto target once its text is rendered, by the script SetFocusDecWait of textarea on setter. */
set_focus_later(setter, focus_obj, target, Float:move_ms, smooth)
{
    if (SCRIPT_NONE != ScriptState(setter))
    {
        Abort(setter);
    }
    new args[64];
    new smooth_str[16];
    if (smooth)
    {
        smooth_str = "true";
    }
    else
    {
        smooth_str = "false";
    }
    sprintf(args, "focus=%d;focused=%d;move_time=%f;smooth=%s", focus_obj, target, move_ms, smooth_str);
    AttachScript(setter, "SetFocusDecWait", "textarea", args);
}

public TextAreaScript()
{
    init_text_area();
    read_text_area_args();
    read_lines();
    create_lines();
    trim_images();
    wait_lines_ready();
    create_scroll_bar();
    update_scroll_bar();
    update_line_alpha(0, 1);
    while (ANIM_DONE != AnimGetState(listGetAt(0), ANIM_COLOR))
    {
        sleep;
    }
    if (line_focus)
    {
        active = 1;
        focus_cursor_line();
    }
    else
    {
        update_action_focus("INIT");
        for (new i; i < cursor; i++)
        {
            update_action_focus("DOWN");
        }
    }
    if (intIndexNum)
    {
        set_focus_later(g_focus_setter, focus, listGetAt(g_top_line - g_list_top), 150.0, 1);
    }
    new step_tick = tickcount();
    new now = step_tick;
    new hold_tick = tickcount();
    new hold_now = hold_tick;
    new can_step;
    new held_long;
    new step_time = get_scroll_time();
    new holding_down;
    new holding_up;
    g_tick = tickcount();
    for (; ; )
    {
        g_last_tick = g_tick;
        sleep;
        g_tick = tickcount();
        new held;
        new pressed;
        new released;
        new repeated;
        PadState(held, pressed, released, repeated, -1);
        if (line_focus)
        {
            cursor = clamp(cursor, 0, g_line_count - 1);
        }
        else if (g_moved_up == 1)
        {
            where = clamp(where, 0, g_line_count - 1 - visible);
        }
        else
        {
            where = clamp(where, visible - 1, g_line_count - 1);
        }
        now = tickcount();
        if (get_step_delay() <= now - step_tick)
        {
            can_step = 1;
        }
        if (!(held & PAD_UP))
        {
            holding_up = 0;
        }
        if (!(held & PAD_DOWN))
        {
            holding_down = 0;
        }
        hold_now = tickcount();
        if (holding_up || holding_down)
        {
            if (200 <= hold_now - hold_tick)
            {
                held_long = 1;
            }
        }
        else
        {
            step_time = get_scroll_time();
            scroll_speed = step_time;
            hold_tick = hold_now;
            held_long = 0;
        }
        if (can_step)
        {
            can_step = enable_pad;
        }
        if (held & PAD_UP && holding_up == held_long && can_step)
        {
            can_step = 0;
            step_tick = now;
            if (held_long && holding_up && 40 < step_time)
            {
                if (40 < step_time)
                {
                    step_time = step_time - get_scroll_accel();
                    if (step_time <= 39)
                    {
                        step_time = 40;
                    }
                }
            }
            scroll_speed = step_time;
            holding_up = 1;
            holding_down = 0;
            g_scrolling_up = move_up();
            g_scrolling_down = 0;
        }
        else if (held & PAD_DOWN && holding_down == held_long && can_step)
        {
            can_step = 0;
            step_tick = now;
            if (held_long && holding_down && 40 < step_time)
            {
                if (40 < step_time)
                {
                    step_time = step_time - get_scroll_accel();
                    if (step_time <= 39)
                    {
                        step_time = 40;
                    }
                }
            }
            scroll_speed = step_time;
            holding_down = 1;
            holding_up = 0;
            g_scrolling_down = move_down();
            g_scrolling_up = 0;
        }
        else if (pressed & PAD_CIRCLE && can_step)
        {
            can_step = 0;
            step_tick = now;
            SendMessage(manager, TAG("DECP"), cursor, g_top_line, intIndexPointer, 0);
        }
        else if (pressed & PAD_CROSS && can_step)
        {
            can_step = 0;
            step_tick = now;
            SendMessage(manager, TAG("RETP"), cursor, g_top_line, intIndexPointer, 0);
        }
        if (ANIM_DONE == AnimGetState(g_list_root, ANIM_MOVE))
        {
            g_scrolling_up = 0;
            g_scrolling_down = 0;
            g_scroll_lag = 0.0;
        }
        if (g_scrolling_up)
        {
            drop_lines_below();
        }
        else if (g_scrolling_down)
        {
            drop_lines_above();
        }
        trim_images();
        update_scroll_bar();
        update_line_alpha(1, get_fade_lines());
        new msg;
        new a;
        new b;
        while (RecvMessage(msg, a, b))
        {
        }
    }
    return;
}

new g_child_tag[1]; /* "": any element */
new g_next_tag[1];

/* Returns the number of children of node. */
count_children(node)
{
    new count;
    new child = domFirstChild(node, g_child_tag);
    for (; child; child = domNext(child, g_next_tag))
    {
        count++;
    }
    return count;
}

/* Points g_line_node at the child of node at the top line. */
seek_top_line(node)
{
    new index;
    new child = domFirstChild(node, g_child_tag);
    g_line_node = child;
    for (; child; child = domNext(child, g_next_tag))
    {
        if (g_top_line == index)
        {
            g_line_node = child;
            g_line_node_index = index;
            break;
        }
        index++;
    }
}

/* Gets how far the list scrolls in this frame toward its place. */
update_scroll_lag()
{
    new Float:x;
    new Float:y;
    new Float:z;
    GetPos(g_list_root, x, y, z, 0);
    g_scroll_lag = float(g_top_line) * interval + y;
    g_scroll_lag = g_scroll_lag / float(scroll_speed);
    g_scroll_lag = g_scroll_lag * (g_tick - g_last_tick);
}

/* Returns the y of a line object in the area, as it will be after this frame of the scroll. */
Float:get_line_y(obj)
{
    new Float:x;
    new Float:y;
    new Float:z;
    new Float:obj_x;
    new Float:obj_y;
    new Float:obj_z;
    GetPos(obj, obj_x, obj_y, obj_z, 0);
    GetOffPos(g_list_root, x, y, z, obj_x, obj_y - g_scroll_lag, obj_z, Self());
    return y;
}

/* Lights the line of the cursor and moves the focus onto it. */
focus_cursor_line()
{
    if (line_focus)
    {
        new Float:r;
        new Float:g;
        new Float:b;
        new Float:a;
        GetColor(listGetAt(cursor - g_list_top), r, g, b, a);
        SetColor(listGetAt(cursor - g_list_top), r, g, b, 1.0);
    }
    set_focus_later(g_focus_setter, focus, listGetAt(cursor - g_list_top), 50.0, 1);
    SetScale(focus, 1.0, 1.0, 1.0);
}

/*
 * Fades each line object by its place: out above and below the area, in inside it; without animate or
 * fade the colors are set at once.
 */
update_line_alpha(animate, fade)
{
    for (new i; i < listSize(); i++)
    {
        new line = i + g_list_top;
        new Float:r;
        new Float:g;
        new Float:b;
        new Float:alpha;
        new line_obj = listGetAt(i);
        GetColor(line_obj, r, g, b, alpha);
        if (active)
        {
            if (line_focus)
            {
                if (cursor == line)
                {
                    alpha = 1.0;
                }
                else
                {
                    alpha = 1.0;
                }
            }
            else
            {
                alpha = 0.8;
            }
        }
        else if (line_focus)
        {
            alpha = 1.0;
        }
        else
        {
            alpha = 1.0;
        }
        new Float:y = get_line_y(line_obj);
        new Float:duration = float(scroll_speed) < 50.0 ? 50.0 : float(scroll_speed);
        new Float:margin = 1.0;
        if (y < 0.0 + voffset - margin - margin)
        {
            if (y < -1.0 * interval + voffset - margin - margin || !animate || !fade)
            {
                SetColor(line_obj, r, g, b, 0.0);
            }
            else
            {
                AnimColor(line_obj, r, g, b, 0.0, duration, 0.0, ANIM_FUNC_LINEAR);
            }
        }
        else if (interval * float(visible) - voffset + margin < y)
        {
            if (interval * float(visible + 1) - voffset + margin < y || !animate || !fade)
            {
                SetColor(line_obj, r, g, b, 0.0);
            }
            else
            {
                AnimColor(line_obj, r, g, b, 0.0, duration, 0.0, ANIM_FUNC_LINEAR);
            }
        }
        else if (interval + voffset - margin - margin < y && y < interval * float(visible - 1) - voffset + margin
                && animate && fade)
        {
            AnimColor(line_obj, r, g, b, 1.0, 50.0, 0.0, ANIM_FUNC_LINEAR);
        }
        else if (fade)
        {
            AnimColor(line_obj, r, g, b, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
        }
        else
        {
            SetColor(line_obj, r, g, b, alpha);
        }
    }
}

/*
 * Creates the object of line, which renders text. A text "@@<commands>@@<text>" adds commands to the line,
 * each read at fixed offsets from its letter: I an image (src, x, y, w, h: IMG src="a" x="0" ...), F a color
 * (r, g, b, a from 0 to 255) and A an action number.
 */
create_line(line, text[], how[])
{
    new plane = CreatePlane(g_list_root);
    if (0 == strcmp(how, "INIT"))
    {
        if (line_focus)
        {
            SetColor(plane, 0.215, 0.708, 0.999, 0.0);
        }
        else
        {
            SetColor(plane, 0.7843, 0.7843, 0.7843, 0.0);
        }
    }
    else if (line_focus)
    {
        SetColor(plane, 0.215, 0.708, 0.999, 0.0);
    }
    else
    {
        SetColor(plane, 0.7843, 0.7843, 0.7843, 0.0);
    }
    new Float:r;
    new Float:g;
    new Float:b;
    new Float:alpha;
    if (line_focus)
    {
        r = 0.215;
        g = 0.708;
        b = 0.999;
        if (active && 0 == strcmp(how, "INIT") && 0 == listSize())
        {
            alpha = 1.0;
        }
        else if (0 == strcmp(how, "INIT"))
        {
            alpha = 0.2;
        }
        else
        {
            alpha = 1.0;
        }
    }
    else
    {
        r = 0.7843;
        g = 0.7843;
        b = 0.7843;
        alpha = 0.8;
    }
    if (0 == strncmp(text, "@@", 2))
    {
        new body[256];
        new ch[256];
        new pos = 2;
        new body_len;
        new len;
        new stop;
        new value[256];
        new x_str[8];
        new y_str[8];
        new Float:x;
        new Float:y;
        new w_str[8];
        new h_str[8];
        new Float:w;
        new Float:h;
        new red;
        new green;
        new blue;
        new red_str[8];
        new green_str[8];
        new blue_str[8];
        new alpha_str[8];
        ch{0} = text{pos};
        while (0 != strncmp(ch, "@", 1) && 0 == stop)
        {
            if (0 == strncmp(ch, "I", 1))
            {
                len = 0;
                pos = pos + MARKUP_IMAGE;
                ch{0} = text{pos};
                while (0 != strncmp(ch, "\"", 1))
                {
                    value{len} = text{pos};
                    len++;
                    pos++;
                    ch{0} = text{pos};
                }
                len = 0;
                pos = pos + MARKUP_NEXT_VALUE;
                ch{0} = text{pos};
                while (0 != strncmp(ch, "\"", 1))
                {
                    x_str{len} = text{pos};
                    len++;
                    pos++;
                    ch{0} = text{pos};
                }
                x = atof(x_str);
                len = 0;
                pos = pos + MARKUP_NEXT_VALUE;
                ch{0} = text{pos};
                while (0 != strncmp(ch, "\"", 1))
                {
                    y_str{len} = text{pos};
                    len++;
                    pos++;
                    ch{0} = text{pos};
                }
                y = atof(y_str);
                if (y < 0.5 * -interval)
                {
                    y = 0.5 * -interval;
                }
                if (y > 0.5 * interval)
                {
                    y = 0.5 * interval;
                }
                len = 0;
                pos = pos + MARKUP_NEXT_VALUE;
                ch{0} = text{pos};
                while (0 != strncmp(ch, "\"", 1))
                {
                    w_str{len} = text{pos};
                    len++;
                    pos++;
                    ch{0} = text{pos};
                }
                w = atof(w_str);
                len = 0;
                pos = pos + MARKUP_NEXT_VALUE;
                ch{0} = text{pos};
                while (0 != strncmp(ch, "\"", 1))
                {
                    h_str{len} = text{pos};
                    len++;
                    pos++;
                    ch{0} = text{pos};
                }
                h = atof(h_str);
                new i;
                new slot = -1;
                i = 0;
                for (; i < 4; i++)
                {
                    if (g_image_lines[i] == line)
                    {
                        slot = -1;
                        break;
                    }
                    if (g_image_lines[i] < 0)
                    {
                        slot = i;
                    }
                }
                if (slot >= 0)
                {
                    g_images[slot] = CreateNull(g_list_root);
                    SetPos(g_images[slot], x, float(line) * interval + y, 0.0, 0);
                    g_image_lines[slot] = line;
                    sprintf(value, "src=%s", value);
                    AttachScript(g_images[slot], _, "trim_tex", value);
                    SetPubVar(g_images[slot], "iTexWidth", floatround(w, 0));
                    SetPubVar(g_images[slot], "iTexHeight", floatround(h, 0));
                    SetPubVar(g_images[slot], "iTrimW", floatround(w, 0));
                    SetPubVar(g_images[slot], "iTrimH", floatround(h, 0));
                    SetPubVar(g_images[slot], "iHAlign", 0);
                    SetPubVar(g_images[slot], "iVAlign", 0);
                    SetPubVar(g_images[slot], "iZType", ZSORT_OVERLAY);
                    g_image_h[slot] = h;
                    g_image_shown_h[slot] = h;
                    g_image_trim_y[slot] = 0.0;
                    g_image_x[slot] = x;
                    g_image_y[slot] = y;
                    i = 0;
                    for (; i < 32; i++)
                    {
                        if (g_known_image_lines[i] < 0 && g_known_image_lines[i] != line)
                        {
                            g_known_image_lines[i] = line;
                            g_known_image_h[i] = h;
                            break;
                        }
                        else if (g_known_image_lines[i] == line)
                        {
                            break;
                        }
                    }
                }
            }
            else if (0 == strncmp(ch, "F", 1))
            {
                len = 0;
                pos = pos + MARKUP_COLOR;
                ch{0} = text{pos};
                while (0 != strncmp(ch, "\"", 1))
                {
                    red_str{len} = text{pos};
                    len++;
                    pos++;
                    ch{0} = text{pos};
                }
                red = atoi(red_str);
                r = float(red) / 255.0;
                clamp(r, 0, 1.0); /* NOTE: bug: the result of clamp is not used */
                len = 0;
                pos = pos + MARKUP_NEXT_VALUE;
                ch{0} = text{pos};
                while (0 != strncmp(ch, "\"", 1))
                {
                    green_str{len} = text{pos};
                    len++;
                    pos++;
                    ch{0} = text{pos};
                }
                green = atoi(green_str);
                g = float(green) / 255.0;
                clamp(g, 0, 1.0);
                len = 0;
                pos = pos + MARKUP_NEXT_VALUE;
                ch{0} = text{pos};
                while (0 != strncmp(ch, "\"", 1))
                {
                    blue_str{len} = text{pos};
                    len++;
                    pos++;
                    ch{0} = text{pos};
                }
                blue = atoi(blue_str);
                b = blue / 255.0;
                clamp(b, 0, 1.0);
                len = 0;
                pos = pos + MARKUP_NEXT_VALUE;
                ch{0} = text{pos};
                while (0 != strncmp(ch, "\"", 1))
                {
                    alpha_str{len} = text{pos};
                    len++;
                    pos++;
                    ch{0} = text{pos};
                }
                /* NOTE: bug: the fourth value was read into alpha_str, but alpha is never set from it */
                clamp(alpha, 0, 1.0);
                SetColor(plane, r, g, b, 0.0);
            }
            else if (0 == strncmp(ch, "A", 1))
            {
                len = 0;
                pos = pos + MARKUP_ACTION;
                ch{0} = text{pos};
                while (0 != strncmp(ch, "\"", 1))
                {
                    value{len} = text{pos};
                    len++;
                    pos++;
                    ch{0} = text{pos};
                }
                SetColor(plane, 0.2156, 0.7058, 1.0, 0.0);
                new i;
                i = 0;
                for (; i < 32; i++)
                {
                    if (g_action_lines[i] < 0 && g_action_lines[i] != line)
                    {
                        g_action_lines[i] = line;
                        g_action_ids[i] = atoi(value);
                        g_action_count++;
                        break;
                    }
                    else if (g_action_lines[i] == line)
                    {
                        break;
                    }
                }
            }
            else
            {
                pos++;
            }
            ch{0} = text{pos};
            if (pos > 256 || 0 == strncmp(ch, "", 1))
            {
                break;
            }
        }
        if (0 == stop)
        {
            pos = pos + 2;
        }
        else
        {
            stop = 1;
        }
        ch{0} = text{pos};
        for (; 0 != strncmp(ch, "", 1); pos++, body_len++, ch{0} = text{pos})
        {
            body{body_len} = text{pos};
        }
        {
            new args[64];
            sprintf(args, "fontsize=%d;align=%d;width=%f", fontsize, font_align, width);
            AttachScript(plane, "StrTexDecWait", "textarea", args);
            SendMessageStr(plane, TAG("TEXT"), body);
            SetPos(plane, 0.0, float(line) * interval + voffset, 0.0, 0);
            SetLocalScale(plane, 0.0, 0.0, 0.0);
        }
    }
    else
    {
        new args[64];
        sprintf(args, "fontsize=%d;align=%d;width=%f", fontsize, font_align, width);
        AttachScript(plane, "StrTexDecWait", "textarea", args);
        SendMessageStr(plane, TAG("TEXT"), text);
        SetPos(plane, 0.0, float(line) * interval + voffset, 0.0, 0);
        SetLocalScale(plane, 0.0, 0.0, 0.0);
    }
    SetOptionI(plane, TAG("ALPH"), BLEND_ADD);
    return plane;
}

/* Scrolls the list a line when the cursor, or where, leaves the lines in view; creates the line that comes in. */
scroll_to_cursor()
{
    new moved;
    if ((line_focus && cursor < g_top_line) || (!line_focus && where < g_top_line))
    {
        g_top_line--;
        if (g_list_top - 1 >= 0)
        {
            if (!g_line_read[g_list_top - 1])
            {
                read_line_text(g_list_top - 1, g_line_texts[g_list_top - 1]);
                g_line_read[g_list_top - 1] = 1;
            }
            new line_obj = create_line(g_list_top - 1, g_line_texts[g_list_top - 1], "UP");
            listInsert(0, line_obj);
            g_list_top--;
            show_images_over(g_list_top - 1);
        }
        moved = 1;
    }
    else if ((line_focus && cursor >= g_top_line + visible) || (!line_focus && where >= g_top_line + visible))
    {
        g_top_line++;
        if (g_list_top + listSize() < g_line_count)
        {
            if (!g_line_read[g_list_top + listSize()])
            {
                read_line_text(g_list_top + listSize(), g_line_texts[g_list_top + listSize()]);
                g_line_read[g_list_top + listSize()] = 1;
            }
            new line_obj = create_line(g_list_top + listSize(), g_line_texts[g_list_top + listSize()], "DOWN");
            listAddLast(line_obj);
        }
        moved = 1;
    }
    if (moved)
    {
        update_scroll_lag();
        AnimMoveLine(g_list_root, 0.0, -float(g_top_line) * interval, 0.0, float(scroll_speed), 0.0, ANIM_FUNC_LINEAR);
    }
}

new g_data_attr[1]; /* "": the text of the element */
new g_prev_tag[1];

destroy_image(slot)
{
    Destroy(g_images[slot]);
    g_image_lines[slot] = -1;
}

/* Destroys the images of the lines without an object that are out of the area. */
destroy_hidden_images()
{
    new slot;
    slot = 0;
    for (; slot < 4; slot++)
    {
        if (0 <= g_image_lines[slot])
        {
            if (g_image_lines[slot] < g_list_top || g_list_top + listSize() - 1 < g_image_lines[slot])
            {
                new Float:y = get_line_y(g_images[slot]);
                if (y < 0 && g_image_h[slot] + y <= -interval)
                {
                    destroy_image(slot);
                    continue;
                }
                if (y + g_image_h[slot] > interval * visible && y > interval * visible)
                {
                    destroy_image(slot);
                    continue;
                }
            }
        }
    }
}

/* Destroys the line objects that scrolled out above the area. */
drop_lines_above()
{
    for (; ; )
    {
        new line_obj = listGetAt(0);
        new Float:y = get_line_y(line_obj);
        if (y <= -2.0 * interval + voffset)
        {
            Destroy(line_obj);
            listRemoveAt(0);
            g_list_top++;
        }
        else
        {
            break;
        }
    }
    destroy_hidden_images();
}

/* Destroys the line objects that scrolled out below the area. */
drop_lines_below()
{
    for (; ; )
    {
        new line_obj = listGetLast();
        new Float:y = get_line_y(line_obj);
        if (interval * float(visible + 1) + voffset <= y)
        {
            Destroy(line_obj);
            listRemoveLast();
        }
        else
        {
            break;
        }
    }
    destroy_hidden_images();
}

/* Reads the text of line, walking g_line_node from the line read last. */
read_line_text(line, text[])
{
    for (; ; )
    {
        if (line == g_line_node_index)
        {
            domGetData(g_line_node, g_data_attr, text, 64);
            return;
        }
        if (line > g_line_node_index)
        {
            g_line_node = domNext(g_line_node, g_next_tag);
            g_line_node_index++;
        }
        else
        {
            g_line_node = domPrev(g_line_node, g_prev_tag);
            g_line_node_index--;
        }
    }
}

/* Moves the thumb of the scroll bar to the top line. */
update_scroll_bar()
{
    if (sb == 1)
    {
        if (SCRIPT_FINISHED != ScriptState(g_scroll_bar))
        {
            new Float:top;
            if (line_focus)
            {
                top = float(g_top_line) / float(g_line_count);
            }
            else
            {
                top = float(g_top_line) / float(g_line_count);
            }
            SetPubVar(g_scroll_bar, "top", top);
        }
    }
}

/* Cuts each image to its part inside the area. */
trim_images()
{
    new slot;
    slot = 0;
    for (; slot < 4; slot++)
    {
        if (g_image_lines[slot] >= 0)
        {
            new Float:y = get_line_y(g_images[slot]);
            g_image_shown_h[slot] = g_image_h[slot];
            g_image_trim_y[slot] = 0.0;
            if (y < 0)
            {
                g_image_shown_h[slot] = g_image_h[slot] + y;
                g_image_trim_y[slot] = -y;
            }
            if (y + g_image_h[slot] > interval * visible)
            {
                g_image_shown_h[slot] = interval * visible - y - g_image_trim_y[slot];
            }
            SetPubVar(g_images[slot], "iTrimH", floatround(g_image_shown_h[slot], 0));
            SetPubVar(g_images[slot], "iTrimY", floatround(g_image_trim_y[slot], 0));
        }
    }
}

/* Shows again the images of the lines above line that reach down to it. */
show_images_over(line)
{
    new i;
    i = 0;
    for (; i < 32; i++)
    {
        if (g_known_image_lines[i] < 0)
        {
            break;
        }
        else
        {
            new span = floatround(g_known_image_h[i] / interval, 0);
            if (g_known_image_lines[i] + span > line && g_known_image_lines[i] < line)
            {
                new text[64];
                read_line_text(g_known_image_lines[i], text);
                new line_obj = create_line(g_known_image_lines[i], text, "UP");
                Destroy(line_obj);
            }
        }
    }
}

/* Lights the action line in focus and moves the focus onto it. */
focus_action()
{
    new Float:r;
    new Float:g;
    new Float:b;
    new Float:a;
    GetColor(listGetAt(g_action_lines[g_action] - g_list_top), r, g, b, a);
    SetColor(listGetAt(g_action_lines[g_action] - g_list_top), r, g, b, 1.0);
    set_focus_later(g_focus_setter, focus, listGetAt(g_action_lines[g_action] - g_list_top), 500.0, 0);
    SetScale(focus, 1.0, 1.0, 1.0);
}

/*
 * Moves the focus to the first action line in view ("INIT") or to the next one up ("UP") or down ("DOWN"),
 * or takes it away when none is in view.
 */
update_action_focus(dir[])
{
    new i;
    new found;
    if (g_action < 0)
    {
        i = 0;
        for (; i < g_action_count; i++)
        {
            if (g_top_line <= g_action_lines[i] && g_action_lines[i] <= g_top_line + visible - 1)
            {
                g_action = i;
                action_id = g_action_ids[g_action];
                focus_action();
                break;
            }
        }
    }
    else if (!strcmp(dir, "UP"))
    {
        i = g_action_count - 1;
        for (; 0 <= i; i--)
        {
            if (g_top_line <= g_action_lines[i] && g_action_lines[i] < g_action_lines[g_action])
            {
                g_action = i;
                action_id = g_action_ids[g_action];
                found = 1;
                focus_action();
                break;
            }
        }
        if (g_action_lines[g_action] <= g_top_line + visible - 1)
        {
            found = 1;
        }
        if (!found)
        {
            g_action = -1;
            action_id = -1;
            set_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        }
    }
    else if (!strcmp(dir, "DOWN"))
    {
        i = 0;
        for (; i < g_action_count; i++)
        {
            if (g_action_lines[g_action] < g_action_lines[i] && g_action_lines[i] <= g_top_line + visible - 1)
            {
                g_action = i;
                action_id = g_action_ids[g_action];
                found = 1;
                focus_action();
                break;
            }
        }
        if (g_top_line <= g_action_lines[g_action])
        {
            found = 1;
        }
        if (!found)
        {
            g_action = -1;
            action_id = -1;
            set_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        }
    }
}

/* The arguments of the FeeGa member pages, which the guide passes on from page to page. */
new g_arg_ret[] = "ret";
new g_arg_retarg[] = "retarg";
new g_arg_game_ver[] = "GameVer";
new g_arg_game_id[] = "GameID";
new g_arg_game_passwd[] = "GamePasswd";
new g_arg_given_name[] = "givenname";
new g_arg_family_name[] = "familyname";
new g_arg_birthday[] = "birthday";
new g_arg_zipcode[] = "zipcode";
new g_arg_sex[] = "sex";
new g_arg_mail_addr[] = "mailaddr";
new g_arg_mail_hope[] = "mailhope";
new g_arg_unload_bgp[] = "unloadBGP";
new g_arg_unload_bgm[] = "unloadBGM";
new g_arg_member_focus[] = "member_focus";

/* Appends to args (maxlen cells) the member arguments the page was opened with, as "name=value;". */
append_member_args(args[], maxlen)
{
    new value[64];
    new found;
    found = GetArg(g_arg_ret, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", g_arg_ret, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(g_arg_retarg, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", g_arg_retarg, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(g_arg_game_ver, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", g_arg_game_ver, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(g_arg_game_id, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", g_arg_game_id, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(g_arg_game_passwd, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", g_arg_game_passwd, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(g_arg_given_name, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", g_arg_given_name, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(g_arg_family_name, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", g_arg_family_name, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(g_arg_birthday, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", g_arg_birthday, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(g_arg_zipcode, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", g_arg_zipcode, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(g_arg_sex, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", g_arg_sex, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(g_arg_mail_addr, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", g_arg_mail_addr, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(g_arg_mail_hope, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", g_arg_mail_hope, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(g_arg_unload_bgp, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", g_arg_unload_bgp, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(g_arg_unload_bgm, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", g_arg_unload_bgm, value);
        append_string(args, value, maxlen);
    }
}

/* Appends the argument member_focus to args, like append_member_args. */
append_focus_arg(args[], maxlen)
{
    new value[64];
    if (GetArg(g_arg_member_focus, value, 64) == 1)
    {
        sprintf(value, "%s=%s;", g_arg_member_focus, value);
        append_string(args, value, maxlen);
    }
}

new g_focus;
new g_text_area;
new g_visible; /* lines of the text area */
new Float:g_area_x;
new Float:g_area_y;
new Float:g_area_z;
new g_align;
new g_valign = -1;
new Float:g_pitch = 28.0; /* pixels from a line to the next */
new g_member_args[256]; /* the member arguments, added to the arguments of the next page */

/* Called at each step of the cursor or of the scroll. */
on_step()
{
    sndSePlay(SE_CURSOR);
}

/* Called when the focus goes to the entry above. */
on_index_up()
{
}

/* Called when the focus goes to the entry below. */
on_index_down()
{
}

/* Milliseconds between two steps while a button is held. */
get_step_delay()
{
    return 75;
}

/* Milliseconds a scroll step takes until a held button speeds it up. */
get_scroll_time()
{
    return 100;
}

/* How much shorter each scroll step gets while a button is held: no value, so 0. */
get_scroll_accel()
{
}

/* Whether the lines fade at the edges of the area. */
get_fade_lines()
{
    return 1;
}

main()
{
    hisClear();
    new camera = Camera(0);
    SetPos(camera, 0.0, 0.0, 300.0, 0);
    LookAt(camera, Scene(0), 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
    g_focus = create_focus_plane2("focus");
    new value[10];
    init_guide_links2();
    create_text_area();
    SetPos(g_text_area, g_area_x, g_area_y, g_area_z, 0);
    SetPubVar(g_text_area, "scroll_speed", 100);
    SetSystemParam(SYS_PADHELP, PADHELP_CIRCLE | PADHELP_CROSS);
    for (; ; )
    {
        new msg;
        new line;
        new top;
        new entry;
        while (RecvMessage(msg, line, top, entry))
        {
            /* "DECP" (line, top line, entry): open the link ln_<line + 1>; "RETP": go back to ln_back. */
            if (msg == TAG("DECP"))
            {
                SetPubVar(g_text_area, "enable_pad", 0);
                sndSePlay(SE_OPEN);
                new link_id[256];
                sprintf(link_id, "ln_%d", line + 1);
                new args[256];
                new url[256];
                get_tag_attr("LINK", link_id, "href", url, 256);
                sprintf(args, "LnkNum=0;");
                append_string(args, g_member_args, 256);
                set_focus(g_focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                SetPos(g_text_area, g_area_x + 6400, g_area_y + 4800, g_area_z, 0);
                SetSystemParam(SYS_PADHELP, 0);
                open_child_page(url, args);
                SetPubVar(g_text_area, "enable_pad", 1);
            }
            else if (msg == TAG("RETP"))
            {
                SetPubVar(g_text_area, "enable_pad", 0);
                sndSePlay(SE_OPEN);
                new args[256];
                new url[256];
                get_tag_attr("LINK", "ln_back", "href", url, 256);
                if (get_tag_attr("LINK", "ln_back", "LnkNum", value, 10))
                {
                    new link_num = atoi(value);
                    sprintf(args, "LnkNum=%d;", link_num);
                }
                append_string(args, g_member_args, 256);
                set_focus(g_focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                SetPos(g_text_area, g_area_x + 6400, g_area_y + 4800, g_area_z, 0);
                SetSystemParam(SYS_PADHELP, 0);
                open_parent_page(url, args);
                SetPubVar(g_text_area, "enable_pad", 1);
            }
        }
        sleep;
    }
    return;
}

/*
 * Creates the text area of the TEXTAREA "textvalue", fitted in 416 by g_area_height, with the
 * cursor on the line of the argument LnkNum.
 */
create_text_area()
{
    new value[64];
    new cursor_pos;
    if (GetArg("LnkNum", value, 64))
    {
        cursor_pos = max(0, atoi(value) - 1);
    }
    g_member_args = "";
    append_member_args(g_member_args, 256);
    append_focus_arg(g_member_args, 256);
    create_page_title(Disp2D(0));
    {
        new args[256];
        new id[16] = "textvalue";
        new scrollbar[16] = "true";
        new line_focus_str[16] = "true";
        new attr[20];
        new Float:area_w = 400.0;
        new Float:font_size = 15.0;
        new found;
        found = get_tag_attr("TEXTAREA", "textvalue", "pitch", attr, 20);
        if (found)
        {
            g_pitch = float(atoi(attr));
        }
        found = get_tag_attr("TEXTAREA", "textvalue", "visible", attr, 20);
        if (found)
        {
            g_visible = atoi(attr);
        }
        found = get_tag_attr("TEXTAREA", "textvalue", "scrollbar", attr, 20);
        if (found)
        {
            copy_string(scrollbar, attr, 16);
        }
        found = get_tag_attr("TEXTAREA", "textvalue", "width", attr, 20);
        if (found)
        {
            area_w = float(atoi(attr));
        }
        found = get_tag_attr("TEXTAREA", "textvalue", "fontsize", attr, 20);
        if (found)
        {
            font_size = float(atoi(attr));
        }
        g_text_area = CreateNull(Disp2D(0));
        g_area_x = g_area_pos[0];
        g_area_y = g_area_pos[1];
        g_area_z = g_area_pos[2];
        area_w = 16.0 * 26.0;
        g_align = ALIGN_LEFT;
        g_valign = ALIGN_CENTER;
        if (g_area_height < g_pitch * float(g_visible))
        {
            new fit_lines = floatround(g_area_height, 1) / floatround(g_pitch, 1);
            if (fit_lines < g_visible)
            {
                scrollbar = "true";
            }
            g_visible = fit_lines;
        }
        if (g_align == ALIGN_CENTER)
        {
            g_area_x = g_area_x + area_w / 2;
        }
        else if (g_align == ALIGN_RIGHT)
        {
            g_area_x = g_area_x + area_w;
        }
        if (g_valign == ALIGN_CENTER)
        {
            g_area_y = 120.0 + g_area_height / 2.0 - g_pitch * float(g_visible) / 2.0;
        }
        SetOptionI(g_text_area, TAG("ZSRT"), ZSORT_OVERLAY);
        sprintf(args, "id=%s;line_focus=%s;visible=%d;cursor_pos=%d;", id, line_focus_str, g_visible, cursor_pos);
        AttachScript(g_text_area, "TextAreaScript", _, args);
        if (0 == strcmp(scrollbar, "true"))
        {
            SetPubVar(g_text_area, "sb", 1);
        }
        else
        {
            SetPubVar(g_text_area, "sb", 0);
        }
        SetPubVar(g_text_area, "font_align", g_align);
        SetPubVar(g_text_area, "manager", Self());
        SetPubVar(g_text_area, "focus", g_focus);
        SetPubVar(g_text_area, "width", area_w);
        SetPubVar(g_text_area, "fontsize", floatround(font_size, 0));
        SetPubVar(g_text_area, "interval", g_pitch);
        SetPos(g_text_area, g_area_x + 6400, g_area_y + 4800, g_area_z, 0);
    }
}
