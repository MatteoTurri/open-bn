/*
 * /opt0/bn/script/guide/ground/bg.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The background of the guide, in the background window: space, a cloud, the window behind the pages and four
 * rings of planes, one for each item of the top page, that spread out when its section opens.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

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

#include "misc.inc"

#include "dom.inc"

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
    AttachScript(Terminate, "TerminateThread", "", "");
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

/* The rings, one for each item of the top page of the guide, in the order of its menu. */
enum
{
    RING_WHATSNEW, /* "WNew" */
    RING_GUIDE,    /* "Guid" */
    RING_SUPPORT,  /* "Sprt" */
    RING_README    /* "Rdme" */
};

/* Values of g_ring_move_pattern. */
enum
{
    RINGMOVE_MAIN,  /* the top page: the first plane of each ring, the focused one brighter */
    RINGMOVE_OPEN,  /* a section is open: the planes of its ring spread out, the other rings fade */
    RINGMOVE_RETURN /* back to the top page: the planes gather, then RINGMOVE_MAIN */
};

new Float:g_ring0_color[4] = {1.0, 1.0, 1.0, 0.125};
new Float:g_ring_default_color[4] = {1.0, 1.0, 1.0, 1.0};
new Float:g_default_menu_pos[5] = {130.0, 160.0, 0.0, 3.0};
new Float:g_screen_z = 500.0;   /* distance of the screen plane from the camera */
new Float:g_rot_follow = 0.4;   /* how much the camera follows the rotation of the main camera */
new Float:g_pos_follow = 0.4;   /* how much the camera follows the position of the main camera */
new g_window;
new Float:g_window_r[4][16];    /* colors of the window for each ring and depth in its section */
new Float:g_window_g[4][16];
new Float:g_window_b[4][16];
new Float:g_window_a[4][16];
new g_ring_num[4];              /* planes of each ring to show, 1 to g_menu_num */
new g_ring_num_backup[4];       /* planes of each ring laid out so far */
new g_ring_nodes[4][16];
new g_ring_planes[4][16];
new Float:g_ring_alpha_pow[4][16];
new Float:g_ring_pos[4][4];     /* x, y, z and scale */
new Float:g_ring_color[4][4];
new g_menu_pos[4][4];
new Float:g_ring_rot[4][3];
new Float:g_ring_premove[4][4]; /* where the planes go first when the section opens, if not 0, 0, 0 */
new Float:g_ring_extension[4][4]; /* the line along which the planes spread */
new Float:g_move_speed_to[4];   /* delay between the planes, in ms */
new Float:g_move_speed_ret[4];
new g_menu_num[4];              /* planes of each ring, 16 at most */
new g_ring_move_pending;
new g_premove_pending[4];
new g_ring_move_pattern;
new Float:g_ring_spin[4];       /* in radians per frame */
new Float:g_cloud_color[4];
new g_cloud;
new g_cloud_jitter[4];
new Float:g_window_w;
new Float:g_window_h;
new g_cur_ring;
new g_ring_move_tick[4];

/* Reads the colors of the window (IMG win_<ring>) and the layout of each ring (IMG img_Ring_<ring>). */
read_ring_params()
{
    new Float:value;
    new text[64];
    new index;
    get_tag_attr("IMG", "win_WNew", "color_r1", text, 64);
    g_window_r[0][index] = atof(text);
    get_tag_attr("IMG", "win_WNew", "color_g1", text, 64);
    g_window_g[0][index] = atof(text);
    get_tag_attr("IMG", "win_WNew", "color_b1", text, 64);
    g_window_b[0][index] = atof(text);
    get_tag_attr("IMG", "win_WNew", "color_a1", text, 64);
    g_window_a[0][index] = atof(text);
    index = 0;
    get_tag_attr("IMG", "win_Guid", "color_r1", text, 64);
    g_window_r[1][index] = atof(text);
    get_tag_attr("IMG", "win_Guid", "color_g1", text, 64);
    g_window_g[1][index] = atof(text);
    get_tag_attr("IMG", "win_Guid", "color_b1", text, 64);
    g_window_b[1][index] = atof(text);
    get_tag_attr("IMG", "win_Guid", "color_a1", text, 64);
    g_window_a[1][index] = atof(text);
    index++;
    get_tag_attr("IMG", "win_Guid", "color_r2", text, 64);
    g_window_r[1][index] = atof(text);
    get_tag_attr("IMG", "win_Guid", "color_g2", text, 64);
    g_window_g[1][index] = atof(text);
    get_tag_attr("IMG", "win_Guid", "color_b2", text, 64);
    g_window_b[1][index] = atof(text);
    get_tag_attr("IMG", "win_Guid", "color_a2", text, 64);
    g_window_a[1][index] = atof(text);
    index++;
    get_tag_attr("IMG", "win_Guid", "color_r3", text, 64);
    g_window_r[1][index] = atof(text);
    get_tag_attr("IMG", "win_Guid", "color_g3", text, 64);
    g_window_g[1][index] = atof(text);
    get_tag_attr("IMG", "win_Guid", "color_b3", text, 64);
    g_window_b[1][index] = atof(text);
    get_tag_attr("IMG", "win_Guid", "color_a3", text, 64);
    g_window_a[1][index] = atof(text);
    index = 0;
    get_tag_attr("IMG", "win_Sprt", "color_r1", text, 64);
    g_window_r[2][index] = atof(text);
    get_tag_attr("IMG", "win_Sprt", "color_g1", text, 64);
    g_window_g[2][index] = atof(text);
    get_tag_attr("IMG", "win_Sprt", "color_b1", text, 64);
    g_window_b[2][index] = atof(text);
    get_tag_attr("IMG", "win_Sprt", "color_a1", text, 64);
    g_window_a[2][index] = atof(text);
    index++;
    get_tag_attr("IMG", "win_Sprt", "color_r2", text, 64);
    g_window_r[2][index] = atof(text);
    get_tag_attr("IMG", "win_Sprt", "color_g2", text, 64);
    g_window_g[2][index] = atof(text);
    get_tag_attr("IMG", "win_Sprt", "color_b2", text, 64);
    g_window_b[2][index] = atof(text);
    get_tag_attr("IMG", "win_Sprt", "color_a2", text, 64);
    g_window_a[2][index] = atof(text);
    index++;
    get_tag_attr("IMG", "win_Sprt", "color_r3", text, 64);
    g_window_r[2][index] = atof(text);
    get_tag_attr("IMG", "win_Sprt", "color_g3", text, 64);
    g_window_g[2][index] = atof(text);
    get_tag_attr("IMG", "win_Sprt", "color_b3", text, 64);
    g_window_b[2][index] = atof(text);
    get_tag_attr("IMG", "win_Sprt", "color_a3", text, 64);
    g_window_a[2][index] = atof(text);
    index = 0;
    get_tag_attr("IMG", "win_Rdme", "color_r1", text, 64);
    g_window_r[3][index] = atof(text);
    get_tag_attr("IMG", "win_Rdme", "color_g1", text, 64);
    g_window_g[3][index] = atof(text);
    get_tag_attr("IMG", "win_Rdme", "color_b1", text, 64);
    g_window_b[3][index] = atof(text);
    get_tag_attr("IMG", "win_Rdme", "color_a1", text, 64);
    g_window_a[3][index] = atof(text);
    index++;
    get_tag_attr("IMG", "win_Rdme", "color_r2", text, 64);
    g_window_r[3][index] = atof(text);
    get_tag_attr("IMG", "win_Rdme", "color_g2", text, 64);
    g_window_g[3][index] = atof(text);
    get_tag_attr("IMG", "win_Rdme", "color_b2", text, 64);
    g_window_b[3][index] = atof(text);
    get_tag_attr("IMG", "win_Rdme", "color_a2", text, 64);
    g_window_a[3][index] = atof(text);
    get_tag_attr("IMG", "img_Ring_WNew", "x", text, 64);
    value = atof(text);
    g_ring_pos[0][0] = value;
    get_tag_attr("IMG", "img_Ring_WNew", "y", text, 64);
    value = atof(text);
    g_ring_pos[0][1] = value;
    get_tag_attr("IMG", "img_Ring_WNew", "z", text, 64);
    value = atof(text);
    g_ring_pos[0][2] = value;
    get_tag_attr("IMG", "img_Ring_WNew", "scale", text, 64);
    value = atof(text);
    g_ring_pos[0][3] = value;
    get_tag_attr("IMG", "img_Ring_WNew", "moveSpeedTo", text, 64);
    g_move_speed_to[0] = atof(text);
    get_tag_attr("IMG", "img_Ring_WNew", "moveSpeedRet", text, 64);
    g_move_speed_ret[0] = atof(text);
    get_tag_attr("IMG", "img_Ring_WNew", "menunum", text, 64);
    value = atof(text);
    g_menu_num[0] = floatround(value, 1);
    g_ring_num[0] = g_menu_num[0];
    g_ring_num_backup[0] = g_menu_num[0];
    /* "make the value of [IMG] [img_Ring_...] [...] in the XML smaller"; both warnings test menunum */
    if (g_menu_num[0] >= 16)
    {
        printf("XMLの[IMG]\t[img_Ring_WNew]\t[offset\t]\tの数値を小さくしてください");
    }
    if (g_menu_num[0] >= 16)
    {
        printf("XMLの[IMG]\t[img_Ring_WNew]\t[menunum]\tの数値を小さくしてください");
    }
    get_tag_attr("IMG", "img_Ring_WNew", "extension_x", text, 64);
    g_ring_extension[0][0] = atof(text) + g_ring_pos[0][0] / float(g_menu_num[0]);
    get_tag_attr("IMG", "img_Ring_WNew", "extension_y", text, 64);
    g_ring_extension[0][1] = atof(text) + g_ring_pos[0][1] / float(g_menu_num[0]);
    get_tag_attr("IMG", "img_Ring_WNew", "extension_z", text, 64);
    g_ring_extension[0][2] = atof(text) + g_ring_pos[0][2] / float(g_menu_num[0]);
    get_tag_attr("IMG", "img_Ring_WNew", "rotx", text, 64);
    g_ring_rot[0][0] = atof(text);
    get_tag_attr("IMG", "img_Ring_WNew", "roty", text, 64);
    g_ring_rot[0][1] = atof(text);
    get_tag_attr("IMG", "img_Ring_WNew", "rotz", text, 64);
    g_ring_rot[0][2] = atof(text);
    get_tag_attr("IMG", "img_Ring_Guid", "x", text, 64);
    value = atof(text);
    g_ring_pos[1][0] = value;
    get_tag_attr("IMG", "img_Ring_Guid", "y", text, 64);
    value = atof(text);
    g_ring_pos[1][1] = value;
    get_tag_attr("IMG", "img_Ring_Guid", "z", text, 64);
    value = atof(text);
    g_ring_pos[1][2] = value;
    get_tag_attr("IMG", "img_Ring_Guid", "scale", text, 64);
    value = atof(text);
    g_ring_pos[1][3] = value;
    get_tag_attr("IMG", "img_Ring_Guid", "moveSpeedTo", text, 64);
    g_move_speed_to[1] = atof(text);
    get_tag_attr("IMG", "img_Ring_Guid", "moveSpeedRet", text, 64);
    g_move_speed_ret[1] = atof(text);
    get_tag_attr("IMG", "img_Ring_Guid", "menunum", text, 64);
    value = atof(text);
    g_menu_num[1] = floatround(value, 1);
    g_ring_num[1] = g_menu_num[1];
    g_ring_num_backup[1] = g_menu_num[1];
    if (g_menu_num[1] >= 16)
    {
        printf("XMLの[IMG]\t[img_Ring_Guid]\t[offset\t]\tの数値を小さくしてください");
    }
    if (g_menu_num[1] >= 16)
    {
        printf("XMLの[IMG]\t[img_Ring_Guid]\t[menunum]\tの数値を小さくしてください");
    }
    get_tag_attr("IMG", "img_Ring_Guid", "extension_x", text, 64);
    g_ring_extension[1][0] = atof(text) + g_ring_pos[1][0] / float(g_menu_num[1]);
    get_tag_attr("IMG", "img_Ring_Guid", "extension_y", text, 64);
    g_ring_extension[1][1] = atof(text) + g_ring_pos[1][1] / float(g_menu_num[1]);
    get_tag_attr("IMG", "img_Ring_Guid", "extension_z", text, 64);
    g_ring_extension[1][2] = atof(text) + g_ring_pos[1][2] / float(g_menu_num[1]);
    get_tag_attr("IMG", "img_Ring_Guid", "rotx", text, 64);
    g_ring_rot[1][0] = atof(text);
    get_tag_attr("IMG", "img_Ring_Guid", "roty", text, 64);
    g_ring_rot[1][1] = atof(text);
    get_tag_attr("IMG", "img_Ring_Guid", "rotz", text, 64);
    g_ring_rot[1][2] = atof(text);
    get_tag_attr("IMG", "img_Ring_Sprt", "x", text, 64);
    value = atof(text);
    g_ring_pos[2][0] = value;
    get_tag_attr("IMG", "img_Ring_Sprt", "y", text, 64);
    value = atof(text);
    g_ring_pos[2][1] = value;
    get_tag_attr("IMG", "img_Ring_Sprt", "z", text, 64);
    value = atof(text);
    g_ring_pos[2][2] = value;
    get_tag_attr("IMG", "img_Ring_Sprt", "scale", text, 64);
    value = atof(text);
    g_ring_pos[2][3] = value;
    get_tag_attr("IMG", "img_Ring_Sprt", "moveSpeedTo", text, 64);
    g_move_speed_to[2] = atof(text);
    get_tag_attr("IMG", "img_Ring_Sprt", "moveSpeedRet", text, 64);
    g_move_speed_ret[1] = atof(text);
    get_tag_attr("IMG", "img_Ring_Sprt", "menunum", text, 64);
    value = atof(text);
    g_menu_num[2] = floatround(value, 1);
    g_ring_num[2] = g_menu_num[2];
    g_ring_num_backup[2] = g_menu_num[2];
    if (g_menu_num[2] >= 16)
    {
        printf("XMLの[IMG]\t[img_Ring_Sprt]\t[offset\t]\tの数値を小さくしてください");
    }
    if (g_menu_num[2] >= 16)
    {
        printf("XMLの[IMG]\t[img_Ring_Sprt]\t[menunum]\tの数値を小さくしてください");
    }
    get_tag_attr("IMG", "img_Ring_Sprt", "extension_x", text, 64);
    g_ring_extension[2][0] = atof(text) + g_ring_pos[2][0] / float(g_menu_num[2]);
    get_tag_attr("IMG", "img_Ring_Sprt", "extension_y", text, 64);
    g_ring_extension[2][1] = atof(text) + g_ring_pos[2][1] / float(g_menu_num[2]);
    get_tag_attr("IMG", "img_Ring_Sprt", "extension_z", text, 64);
    g_ring_extension[2][2] = atof(text) + g_ring_pos[2][2] / float(g_menu_num[2]);
    get_tag_attr("IMG", "img_Ring_Sprt", "rotx", text, 64);
    g_ring_rot[2][0] = atof(text);
    get_tag_attr("IMG", "img_Ring_Sprt", "roty", text, 64);
    g_ring_rot[2][1] = atof(text);
    get_tag_attr("IMG", "img_Ring_Sprt", "rotz", text, 64);
    g_ring_rot[2][2] = atof(text);
    get_tag_attr("IMG", "img_Ring_Rdme", "x", text, 64);
    value = atof(text);
    g_ring_pos[3][0] = value;
    get_tag_attr("IMG", "img_Ring_Rdme", "y", text, 64);
    value = atof(text);
    g_ring_pos[3][1] = value;
    get_tag_attr("IMG", "img_Ring_Rdme", "z", text, 64);
    value = atof(text);
    g_ring_pos[3][2] = value;
    get_tag_attr("IMG", "img_Ring_Rdme", "scale", text, 64);
    value = atof(text);
    g_ring_pos[3][3] = value;
    get_tag_attr("IMG", "img_Ring_Rdme", "moveSpeedTo", text, 64);
    g_move_speed_to[3] = atof(text);
    get_tag_attr("IMG", "img_Ring_Rdme", "moveSpeedRet", text, 64);
    g_move_speed_ret[3] = atof(text);
    get_tag_attr("IMG", "img_Ring_Rdme", "menunum", text, 64);
    value = atof(text);
    g_menu_num[3] = floatround(value, 1);
    g_ring_num[3] = g_menu_num[3];
    g_ring_num_backup[3] = g_menu_num[3];
    if (g_menu_num[3] >= 16)
    {
        printf("XMLの[IMG]\t[img_Ring_Rdme]\t[offset\t]\tの数値を小さくしてください");
    }
    if (g_menu_num[3] >= 16)
    {
        printf("XMLの[IMG]\t[img_Ring_Rdme]\t[menunum]\tの数値を小さくしてください");
    }
    get_tag_attr("IMG", "img_Ring_Rdme", "extension_x", text, 64);
    g_ring_extension[3][0] = atof(text) + g_ring_pos[3][0] / float(g_menu_num[3]);
    get_tag_attr("IMG", "img_Ring_Rdme", "extension_y", text, 64);
    g_ring_extension[3][1] = atof(text) + g_ring_pos[3][1] / float(g_menu_num[3]);
    get_tag_attr("IMG", "img_Ring_Rdme", "extension_z", text, 64);
    g_ring_extension[3][2] = atof(text) + g_ring_pos[3][2] / float(g_menu_num[3]);
    get_tag_attr("IMG", "img_Ring_Rdme", "rotx", text, 64);
    g_ring_rot[3][0] = atof(text);
    get_tag_attr("IMG", "img_Ring_Rdme", "roty", text, 64);
    g_ring_rot[3][1] = atof(text);
    get_tag_attr("IMG", "img_Ring_Rdme", "rotz", text, 64);
    g_ring_rot[3][2] = atof(text);
    get_tag_attr("IMG", "img_Ring_WNew", "premovex", text, 64);
    if (0 == strcmp(text, ""))
    {
        g_ring_premove[0][0] = 0.0;
    }
    else
    {
        g_ring_premove[0][0] = atof(text);
    }
    get_tag_attr("IMG", "img_Ring_WNew", "premovey", text, 64);
    if (0 == strcmp(text, ""))
    {
        g_ring_premove[0][1] = 0.0;
    }
    else
    {
        g_ring_premove[0][1] = atof(text);
    }
    get_tag_attr("IMG", "img_Ring_WNew", "premovez", text, 64);
    if (0 == strcmp(text, ""))
    {
        g_ring_premove[0][2] = 0.0;
    }
    else
    {
        g_ring_premove[0][2] = atof(text);
    }
    get_tag_attr("IMG", "img_Ring_Guid", "premovex", text, 64);
    if (0 == strcmp(text, ""))
    {
        g_ring_premove[1][0] = 0.0;
    }
    else
    {
        g_ring_premove[1][0] = atof(text);
    }
    get_tag_attr("IMG", "img_Ring_Guid", "premovey", text, 64);
    if (0 == strcmp(text, ""))
    {
        g_ring_premove[1][1] = 0.0;
    }
    else
    {
        g_ring_premove[1][1] = atof(text);
    }
    get_tag_attr("IMG", "img_Ring_Guid", "premovez", text, 64);
    if (0 == strcmp(text, ""))
    {
        g_ring_premove[1][2] = 0.0;
    }
    else
    {
        g_ring_premove[1][2] = atof(text);
    }
    get_tag_attr("IMG", "img_Ring_Sprt", "premovex", text, 64);
    if (0 == strcmp(text, ""))
    {
        g_ring_premove[2][0] = 0.0;
    }
    else
    {
        g_ring_premove[2][0] = atof(text);
    }
    get_tag_attr("IMG", "img_Ring_Sprt", "premovey", text, 64);
    if (0 == strcmp(text, ""))
    {
        g_ring_premove[2][1] = 0.0;
    }
    else
    {
        g_ring_premove[2][1] = atof(text);
    }
    get_tag_attr("IMG", "img_Ring_Sprt", "premovez", text, 64);
    if (0 == strcmp(text, ""))
    {
        g_ring_premove[2][2] = 0.0;
    }
    else
    {
        g_ring_premove[2][2] = atof(text);
    }
    get_tag_attr("IMG", "img_Ring_Rdme", "premovex", text, 64);
    if (0 == strcmp(text, ""))
    {
        g_ring_premove[3][0] = 0.0;
    }
    else
    {
        g_ring_premove[3][0] = atof(text);
    }
    get_tag_attr("IMG", "img_Ring_Rdme", "premovey", text, 64);
    if (0 == strcmp(text, ""))
    {
        g_ring_premove[3][1] = 0.0;
    }
    else
    {
        g_ring_premove[3][1] = atof(text);
    }
    get_tag_attr("IMG", "img_Ring_Rdme", "premovez", text, 64);
    if (0 == strcmp(text, ""))
    {
        g_ring_premove[3][2] = 0.0;
    }
    else
    {
        g_ring_premove[3][2] = atof(text);
    }
    for (new ring; ring < 4; ring++)
    {
        g_ring_num[ring] = g_menu_num[ring];
        g_ring_num_backup[ring] = g_menu_num[ring];
    }
}

main()
{
    for (new ring; ring < 4; ring++)
    {
        g_ring_alpha_pow[ring][0] = 0.7;
        g_ring_alpha_pow[ring][1] = 0.5;
        g_ring_alpha_pow[ring][2] = 0.3;
        g_ring_move_tick[ring] = 0;
        g_premove_pending[ring] = 1;
        g_ring_spin[ring] = -0.004;
    }
    g_cloud_color[0] = 0.3;
    g_cloud_color[1] = 0.5;
    g_cloud_color[2] = 1.0;
    g_cloud_color[3] = 0.3;
    g_ring_move_pending = 0;
    new Float:base_pos[3];
    base_pos[0] = 0.0;
    base_pos[1] = 0.0;
    base_pos[2] = 0.0;
    new Float:base_quat[4];
    base_quat[0] = 0.0;
    base_quat[1] = 1.0;
    base_quat[2] = 0.0;
    base_quat[3] = 0.0;
    new Float:home_quat[4];
    home_quat[0] = 0.0;
    home_quat[1] = 1.0;
    home_quat[2] = 0.0;
    home_quat[3] = 0.0;
    SetPos(Camera(0), 0.0, 0.0, 0.0, 0);
    SetQuat(Camera(0), home_quat[0], home_quat[1], home_quat[2], home_quat[3], 0);
    {
        new Float:screen_w;
        new Float:screen_h;
        new Float:x;
        new Float:y;
        new Float:z;
        get_screen_size2(screen_w, screen_h);
        map_screen_point(x, y, z, 0.5 * screen_w, 0.5 * screen_h, Camera(0));
        g_screen_z = z;
    }
    new cover;
    new cover_fading;
    /* a black cover over the screen until "STRT" */
    cover = CreatePlane(Disp2D(0));
    SetColor(cover, 0.0, 0.0, 0.0, 1.0);
    SetPos(cover, 320.0, 224.0, 0.0, 0);
    SetLocalScale(cover, 640.0, 480.0, 1.0);
    SetOptionI(cover, TAG("ZSRT"), ZSORT_OVERLAY);
    new cloud_id;
    g_cloud = CreateCloud(Scene(0));
    SetPos(g_cloud, 0.0, 0.0, -20000.0 * 1.0, 0);
    LookAt(g_cloud, Scene(0), 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
    SetLocalScale(g_cloud, 1.5 * 10000.0, 1.5 * 10000.0, 1.5 * 10000.0);
    /* NOTE: bug: "CDW\t" has a tab where the option "CDW " has a space: the cloud ignores it */
    SetOptionP(g_cloud, TAG("CDW\t"), g_cloud_jitter, 4);
    SetOptionI(g_cloud, TAG("CLID"), cloud_id);
    SetOptionI(g_cloud, TAG("ZSRT"), ZSORT_BACKGROUND);
    SetOptionI(g_cloud, TAG("NSPR"), 25);
    SetOptionI(g_cloud, TAG("NDSP"), 12);
    SetOptionI(g_cloud, TAG("RAYM"), 0);
    SetColor(g_cloud, g_cloud_color[0], g_cloud_color[1], g_cloud_color[2], g_cloud_color[3]);
    SetColor(g_cloud, g_cloud_color[0], g_cloud_color[1], g_cloud_color[2], 0.5);
    AnimColor(g_cloud, g_cloud_color[0], g_cloud_color[1], g_cloud_color[2], 0.0, 20.0, 0.0, ANIM_FUNC_LINEAR);
    new layer1;
    layer1 = CreatePlane(Scene(0));
    SetPos(layer1, 0.0, 0.0, 1.0 * (40.0 * -g_screen_z), 0);
    LookAt(layer1, Scene(0), 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
    SetColor(layer1, 1.0, 1.0, 1.0, 0.0);
    SetRot(layer1, 0.0, 0.0, 3.141592, 0, 0);
    SetLocalScale(layer1, 1.0 * (1024.0 * 40.0), 1.0 * (512.0 * 40.0), 1.0 * (1.0 * 40.0));
    imgDecStart("file://opt0/bn/script/common/texture/space_layer1.tm2", 0, 0, -1, 100, 0);
    if (IMG_DONE != wait_image_decode())
    {
        terminate();
    }
    SetTexture(layer1, imgTexID(), 0, -1);
    SetOptionI(layer1, TAG("ALPH"), BLEND_ADD);
    SetColor(layer1, 1.0, 1.0, 1.0, 1.0);
    new layer3;
    layer3 = CreatePlane(Scene(0));
    SetPos(layer3, 0.0, 0.0, 1.0 * (25.0 * -g_screen_z), 0);
    LookAt(layer3, Scene(0), 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
    SetColor(layer3, 1.0, 1.0, 1.0, 0.0);
    SetLocalScale(layer3, 1.0 * (1024.0 * 25.0), 1.0 * (512.0 * 25.0), 1.0 * (1.0 * 25.0));
    imgDecStart("file://opt0/bn/script/common/texture/space_layer3.tm2", 0, 0, -1, 100, 0);
    if (IMG_DONE != wait_image_decode())
    {
        terminate();
    }
    SetTexture(layer3, imgTexID(), 0, -1);
    SetOptionI(layer3, TAG("ALPH"), BLEND_ADD);
    SetColor(layer3, 1.0, 1.0, 1.0, 1.0);
    /* the window behind the pages, colored by the open section */
    g_window = CreatePlane(Disp2D(0));
    LookAt(g_window, Camera(0), 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
    SetColor(g_window, 1.0, 1.0, 1.0, 0.0);
    SetRot(g_window, 0.0, 0.0, 0.0, 0, 0);
    imgDecStart("../images/window.tm2", 0, 0, -1, 100, 0);
    if (IMG_DONE != wait_image_decode())
    {
        terminate();
    }
    SetTexture(g_window, imgTexID(), 0, -1);
    SetOptionI(g_window, TAG("ALPH"), BLEND_ALPHA);
    SetColor(g_window, 1.0, 1.0, 1.0, 1.0);
    SetPos(g_window, 319.99493, 223.99399, 497.28986, 0);
    g_window_h = float(imgTexHeight());
    g_window_w = float(imgTexWidth());
    SetScale(g_window, 3.28, 3.28 * g_window_h / g_window_w, 1.0);
    new linked;
    new main_scene;
    new main_camera;
    new tex;
    new tex_size[2];
    new msg;
    new a;
    read_ring_params();
    for (new i; i < 4; i++)
    {
        g_ring_color[0][i] = g_ring0_color[i];
    }
    for (new ring = 1; ring < 4; ring++)
    {
        for (new i; i < 4; i++)
        {
            g_ring_color[ring][i] = g_ring_default_color[i];
        }
    }
    for (new ring; ring < 4; ring++)
    {
        for (new i; i < 4; i++)
        {
            g_menu_pos[ring][i] = g_default_menu_pos[i];
        }
    }
    {
        new ring_ids[4][16];
        copy_string(ring_ids[0], "img_Ring_WNew", 16);
        copy_string(ring_ids[1], "img_Ring_Guid", 16);
        copy_string(ring_ids[2], "img_Ring_Sprt", 16);
        copy_string(ring_ids[3], "img_Ring_Rdme", 16);
        for (new ring; ring < 4; ring++)
        {
            for (new i; i < g_menu_num[ring]; i++)
            {
                g_ring_nodes[ring][i] = CreateNull(Disp2D(0));
                g_ring_planes[ring][i] = CreatePlane(g_ring_nodes[ring][i]);
                SetPos(g_ring_planes[ring][i], g_ring_pos[ring][0], g_ring_pos[ring][1], g_ring_pos[ring][2], 0);
                SetScale(g_ring_planes[ring][i], g_ring_pos[ring][3], g_ring_pos[ring][3], g_ring_pos[ring][3]);
                SetOptionI(g_ring_planes[ring][i], TAG("ZSRT"), ZSORT_NONE);
                SetOptionI(g_ring_planes[ring][i], TAG("ALPH"), BLEND_ALPHA);
                if (0 == ring && 0 == i)
                {
                    SetColor(g_ring_planes[ring][i], g_ring_color[ring][0], g_ring_color[ring][1],
                            g_ring_color[ring][2], g_ring_color[ring][3]);
                }
                else if (0 == ring)
                {
                    SetColor(g_ring_planes[ring][i], g_ring_color[ring][0], g_ring_color[ring][1],
                            g_ring_color[ring][2], 0.5);
                }
                SetRot(g_ring_planes[ring][i], g_ring_rot[ring][0], g_ring_rot[ring][1], g_ring_rot[ring][2], 0, 0);
                load_image(tex, ring_ids[ring], tex_size[0], tex_size[1]);
                SetTexture(g_ring_planes[ring][i], tex, 0, -1);
                SetLocalScale(g_ring_planes[ring][i], float(tex_size[0]), float(tex_size[1]), 1.0);
            }
        }
    }
    sndBgmPlay(1);
    for (; ; )
    {
        spin_rings();
        /* the main window closed while the camera followed it */
        if (!wndGetScene(WND_MAIN) && main_scene && linked)
        {
            main_scene = 0;
            main_camera = 0;
            goto release_camera;
        }
        main_scene = wndGetScene(WND_MAIN);
        main_camera = Camera(main_scene);
        new Float:b;
        new Float:c;
        while (RecvMessage(msg, a, b, c))
        {
            /* "INRG" (ring): the top page is shown with the focus on ring; the rings gather. */
            if (msg == TAG("INRG"))
            {
                g_cur_ring = RING_WHATSNEW;
                if (a == TAG("None"))
                {
                }
                else if (a == TAG("WNew"))
                {
                    g_cur_ring = RING_WHATSNEW;
                }
                else if (a == TAG("Guid"))
                {
                    g_cur_ring = RING_GUIDE;
                }
                else if (a == TAG("Sprt"))
                {
                    g_cur_ring = RING_SUPPORT;
                }
                else if (a == TAG("Rdme"))
                {
                    g_cur_ring = RING_README;
                }
                if ((RINGMOVE_RETURN != g_ring_move_pattern))
                {
                    g_ring_move_pattern = RINGMOVE_RETURN;
                    g_ring_move_pending = 1;
                }
            }
            /* "DRRG" (ring, or "None" on the top page): the section of ring opens. */
            if (msg == TAG("DRRG"))
            {
                new none;
                if (a == TAG("None"))
                {
                    none = 1;
                }
                else if (a == TAG("WNew"))
                {
                    g_cur_ring = RING_WHATSNEW;
                }
                else if (a == TAG("Guid"))
                {
                    g_cur_ring = RING_GUIDE;
                }
                else if (a == TAG("Sprt"))
                {
                    g_cur_ring = RING_SUPPORT;
                }
                else if (a == TAG("Rdme"))
                {
                    g_cur_ring = RING_README;
                }
                if (!none && RINGMOVE_OPEN != g_ring_move_pattern)
                {
                    g_ring_move_pattern = RINGMOVE_OPEN;
                    g_ring_move_pending = 1;
                }
            }
            /* "FSTR" (ring): the focus moved to ring. */
            if (msg == TAG("FSTR"))
            {
                if (a == TAG("None"))
                {
                }
                else if (a == TAG("WNew"))
                {
                    g_cur_ring = RING_WHATSNEW;
                }
                else if (a == TAG("Guid"))
                {
                    g_cur_ring = RING_GUIDE;
                }
                else if (a == TAG("Sprt"))
                {
                    g_cur_ring = RING_SUPPORT;
                }
                else if (a == TAG("Rdme"))
                {
                    g_cur_ring = RING_README;
                }
            }
            /* "STRT" (duration, 2000 ms if 0): the black cover fades out. */
            if (msg == TAG("STRT"))
            {
                if (cover)
                {
                    new Float:duration = 2000.0;
                    if (Float:a > 0.0)
                    {
                        duration = a;
                    }
                    AnimColor(cover, 0.0, 0.0, 0.0, 0.0, duration, 0.0, ANIM_FUNC_LINEAR);
                    cover_fading = 1;
                }
            }
            /* "AJST": the camera moves part of the way to the pose of the main camera. */
            if (msg == TAG("AJST"))
            {
                new Float:x;
                new Float:y;
                new Float:z;
                new Float:w;
                base_pos[0] = 0.0;
                base_pos[1] = 0.0;
                base_pos[2] = 0.0;
                base_quat[0] = 0.0;
                base_quat[1] = 1.0;
                base_quat[2] = 0.0;
                base_quat[3] = 0.0;
                GetPos(main_camera, x, y, z, main_scene);
                AnimMoveHermite(Camera(0), 1.0 * x * g_pos_follow, 1.0 * y * g_pos_follow, 1.0 * z * g_pos_follow,
                        2000.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
                GetQuat(main_camera, x, y, z, w, main_scene);
                QuatInterp(x, y, z, w, home_quat[0], home_quat[1], home_quat[2], home_quat[3], x, y, z, w,
                        g_rot_follow);
                AnimQuat(Camera(0), x, y, z, w, 2000.0, 0.0, ANIM_FUNC_EXPONENT);
            }
            /* "CRCT": the present pose of the main camera becomes the one the camera is home at. */
            if (msg == TAG("CRCT"))
            {
                GetPos(main_camera, base_pos[0], base_pos[1], base_pos[2], main_scene);
                GetQuat(main_camera, base_quat[0], base_quat[1], base_quat[2], base_quat[3], main_scene);
                AnimMoveHermite(Camera(0), 0.0, 0.0, 0.0, 2000.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
                AnimQuat(Camera(0), home_quat[0], home_quat[1], home_quat[2], home_quat[3], 2000.0, 0.0,
                        ANIM_FUNC_EXPONENT);
            }
            /* "LINK" (_, share of the position, share of the rotation): follow the main camera. */
            if (msg == TAG("LINK"))
            {
                linked = 1;
                g_pos_follow = b;
                g_rot_follow = c;
            }
            /* "RELS" (animate, duration): stop following and go back home. */
            if (msg == TAG("RELS"))
            {
                release_camera:
                g_pos_follow = 0.4;
                g_rot_follow = 0.4;
                base_pos[0] = 0.0;
                base_pos[1] = 0.0;
                base_pos[2] = 0.0;
                base_quat[0] = 0.0;
                base_quat[1] = 1.0;
                base_quat[2] = 0.0;
                base_quat[3] = 0.0;
                if (a == 1)
                {
                    AnimMoveHermite(Camera(0), 0.0, 0.0, 0.0, b, 0.0, ANIM_FUNC_EXPONENT, 1.0);
                    AnimQuat(Camera(0), home_quat[0], home_quat[1], home_quat[2], home_quat[3], b, 0.0,
                            ANIM_FUNC_EXPONENT);
                }
                else
                {
                    SetPos(Camera(0), 0.0, 0.0, 0.0, 0);
                    SetQuat(Camera(0), home_quat[0], home_quat[1], home_quat[2], home_quat[3], 0);
                }
                linked = 0;
                while (RecvMessage(msg, a, b, c))
                {
                }
            }
            /* "CPTN" (pattern): the pattern of the cloud. */
            if (msg == TAG("CPTN"))
            {
                SetOptionI(g_cloud, TAG("CLID"), a);
            }
            /*
             * "INCP" and "DECP" (sender): a page of the guide opens one a level deeper or up, and the focused ring
             * shows one plane fewer or more; the sender gets "RECV".
             */
            if (msg == TAG("INCP"))
            {
                new Float:sender = a;
                if (RING_WHATSNEW == g_cur_ring || RING_WHATSNEW == g_cur_ring) /* redundant as written */
                {
                    g_ring_num[0] -= 1;
                    if (g_ring_num[0] <= 0)
                    {
                        g_ring_num[0] = 1;
                    }
                    else if (g_ring_num[0] > g_menu_num[0])
                    {
                        g_ring_num[0] = g_menu_num[0];
                    }
                }
                else if (g_cur_ring == RING_GUIDE || g_cur_ring == RING_GUIDE)
                {
                    g_ring_num[1] -= 1;
                    if (g_ring_num[1] <= 0)
                    {
                        g_ring_num[1] = 1;
                    }
                    else if (g_ring_num[1] > g_menu_num[1])
                    {
                        g_ring_num[1] = g_menu_num[1];
                    }
                }
                else if (g_cur_ring == RING_SUPPORT || g_cur_ring == RING_SUPPORT)
                {
                    g_ring_num[2] -= 1;
                    if (g_ring_num[2] <= 0)
                    {
                        g_ring_num[2] = 1;
                    }
                    else if (g_ring_num[2] > g_menu_num[2])
                    {
                        g_ring_num[2] = g_menu_num[2];
                    }
                }
                else if (g_cur_ring == RING_README || g_cur_ring == RING_README)
                {
                    g_ring_num[3] -= 1;
                    if (g_ring_num[3] <= 0)
                    {
                        g_ring_num[3] = 1;
                    }
                    else if (g_ring_num[3] > g_menu_num[3])
                    {
                        g_ring_num[3] = g_menu_num[3];
                    }
                }
                else
                {
                    new unused; /* unused, but part of the layout */
                }
                assert IsValidObject(sender);
                SendMessage(sender, TAG("RECV"), 0, 0, 0, 0);
                update_ring_num();
            }
            if (msg == TAG("DECP"))
            {
                new Float:sender = a;
                if (RING_WHATSNEW == g_cur_ring || RING_WHATSNEW == g_cur_ring) /* redundant as written */
                {
                    g_ring_num[0] += 1;
                    if (g_ring_num[0] <= 0)
                    {
                        g_ring_num[0] = 1;
                    }
                    else if (g_ring_num[0] > g_menu_num[0])
                    {
                        g_ring_num[0] = g_menu_num[0];
                    }
                }
                else if (g_cur_ring == RING_GUIDE || g_cur_ring == RING_GUIDE)
                {
                    g_ring_num[1] += 1;
                    if (g_ring_num[1] <= 0)
                    {
                        g_ring_num[1] = 1;
                    }
                    else if (g_ring_num[1] > g_menu_num[1])
                    {
                        g_ring_num[1] = g_menu_num[1];
                    }
                }
                else if (g_cur_ring == RING_SUPPORT || g_cur_ring == RING_SUPPORT)
                {
                    g_ring_num[2] += 1;
                    if (g_ring_num[2] <= 0)
                    {
                        g_ring_num[2] = 1;
                    }
                    else if (g_ring_num[2] > g_menu_num[2])
                    {
                        g_ring_num[2] = g_menu_num[2];
                    }
                }
                else if (g_cur_ring == RING_README || g_cur_ring == RING_README)
                {
                    g_ring_num[3] += 1;
                    if (g_ring_num[3] <= 0)
                    {
                        g_ring_num[3] = 1;
                    }
                    else if (g_ring_num[3] > g_menu_num[3])
                    {
                        g_ring_num[3] = g_menu_num[3];
                    }
                }
                else
                {
                    new unused; /* unused, but part of the layout */
                }
                SendMessage(sender, TAG("RECV"), 0, 0, 0, 0);
                update_ring_num();
            }
        }
        move_rings();
        if (cover && cover_fading)
        {
            if (ANIM_DONE == AnimGetState(cover, ANIM_COLOR))
            {
                Destroy(cover);
                cover = 0;
                cover_fading = 0;
            }
        }
        /* follow the camera of the main window from its pose at "CRCT" */
        if (wndGetScene(WND_MAIN))
        {
            if (linked)
            {
                new Float:x;
                new Float:y;
                new Float:z;
                new Float:w;
                GetOffPos(main_camera, x, y, z, -base_pos[0], -base_pos[1], -base_pos[2], main_scene);
                AnimModMove(Camera(0), 1.0 * x * g_pos_follow, 1.0 * y * g_pos_follow, 1.0 * z * g_pos_follow);
                GetQuat(main_camera, x, y, z, w, main_scene);
                new Float:qx;
                new Float:qy;
                new Float:qz;
                new Float:qw;
                QuatMultiply(qx, qy, qz, qw, x, y, z, w, -base_quat[0], -base_quat[1], -base_quat[2], base_quat[3]);
                QuatMultiply(qx, qy, qz, qw, qx, qy, qz, qw, home_quat[0], home_quat[1], home_quat[2], home_quat[3]);
                QuatInterp(qx, qy, qz, qw, home_quat[0], home_quat[1], home_quat[2], home_quat[3], qx, qy, qz, qw,
                        g_rot_follow);
                AnimModQuat(Camera(0), qx, qy, qz, qw);
            }
        }
        sleep;
    }
    return;
}

/* Decodes the "src" of the IMG id into tex and gets its size; returns 0 if it has none. */
load_image(&tex, id[], &width, &height)
{
    new url[64];
    get_tag_attr("IMG", id, "src", url, 64);
    if (0 == strlen(url))
    {
        return 0;
    }
    imgDecStart(url, 0, 0, -1, 100, 0);
    if (IMG_DONE != wait_image_decode())
    {
        terminate();
    }
    tex = imgTexID();
    if (0 != tex)
    {
        width = imgTexOrgWidth();
        height = imgTexOrgHeight();
    }
    return 1;
}

/* Turns the rings, ends RINGMOVE_RETURN once its moves are over, and fades the planes to the focus. */
spin_rings()
{
    if (g_ring_move_pattern == RINGMOVE_RETURN && tickcount() > g_ring_move_tick[0] + floatround(500.0, 1)
            && tickcount() > g_ring_move_tick[1] + floatround(500.0, 1)
            && tickcount() > g_ring_move_tick[2] + floatround(500.0, 1)
            && tickcount() > g_ring_move_tick[3] + floatround(500.0, 1))
    {
        g_ring_move_pattern = RINGMOVE_MAIN;
    }
    for (new ring; ring < 4; ring++)
    {
        if (0 == ring)
        {
            continue;
        }
        for (new plane; plane < g_menu_num[ring]; plane++)
        {
            if (g_ring_rot[ring][2] > 3.141593)
            {
                g_ring_rot[ring][2] -= 3.141593 * 2.0;
            }
            SetRot(g_ring_planes[ring][plane], g_ring_rot[ring][0], g_ring_rot[ring][1], g_ring_rot[ring][2], 0, 0);
        }
        g_ring_rot[ring][2] += g_ring_spin[ring];
    }
    if (g_ring_move_pattern == RINGMOVE_OPEN)
    {
        return;
    }
    new index;
    for (new ring; ring < 4; ring++)
    {
        index = 0;
        for (; index < g_menu_num[ring]; index++)
        {
            if (0 != index && tickcount() > 500.0 + g_ring_move_tick[ring])
            {
                SetColor(g_ring_planes[ring][index], g_ring_color[ring][0], g_ring_color[ring][1],
                        g_ring_color[ring][2], 0.0);
            }
            else if (RINGMOVE_MAIN == g_ring_move_pattern)
            {
                if (RING_WHATSNEW == ring && g_cur_ring == ring)
                {
                    AnimColor(g_ring_planes[ring][index], g_ring_color[ring][0], g_ring_color[ring][1],
                            g_ring_color[ring][2], g_ring_color[ring][3], 10.0, 0.0, ANIM_FUNC_LINEAR);
                }
                else if (g_cur_ring == ring)
                {
                    AnimColor(g_ring_planes[ring][index], g_ring_color[ring][0], g_ring_color[ring][1],
                            g_ring_color[ring][2], 1.0, 10.0, 0.0, ANIM_FUNC_LINEAR);
                }
                else
                {
                    AnimColor(g_ring_planes[ring][index], g_ring_color[ring][0], g_ring_color[ring][1],
                            g_ring_color[ring][2], 0.5, 10.0, 0.0, ANIM_FUNC_LINEAR);
                }
            }
        }
    }
}

/* Spreads or gathers the planes of each ring to g_ring_num, with the window color of that depth. */
update_ring_num()
{
    new Float:x;
    new Float:y;
    new Float:z;
    new Float:end_x;
    new Float:end_y;
    new Float:end_z;
    if (g_ring_move_pattern == RINGMOVE_RETURN)
    {
        return;
    }
    for (new ring; ring < 4; ring++)
    {
        if (g_ring_num[ring] < g_ring_num_backup[ring])
        {
            AnimColor(g_window, g_window_r[ring][g_menu_num[ring] - g_ring_num[ring]],
                    g_window_g[ring][g_menu_num[ring] - g_ring_num[ring]],
                    g_window_b[ring][g_menu_num[ring] - g_ring_num[ring]],
                    g_window_a[ring][g_menu_num[ring] - g_ring_num[ring]], 500.0, 0.0, ANIM_FUNC_LINEAR);
            GetPos(g_ring_nodes[ring][0], x, y, z, Disp2D(0));
            end_x = x + g_ring_extension[ring][0] / float(g_menu_num[ring]) * float(g_ring_num[ring] - 1);
            end_y = y + g_ring_extension[ring][1] / float(g_menu_num[ring]) * float(g_ring_num[ring] - 1);
            end_z = z + g_ring_extension[ring][2] / float(g_menu_num[ring]) * float(g_ring_num[ring] - 1);
            for (new i; i < g_ring_num[ring]; i++)
            {
                AnimColor(g_ring_planes[ring][i], g_ring_color[ring][0], g_ring_color[ring][1], g_ring_color[ring][2],
                        g_ring_color[ring][3] * g_ring_alpha_pow[ring][i + (g_menu_num[ring] - g_ring_num[ring])],
                        500.0, 0.0, ANIM_FUNC_LINEAR);
            }
            for (new i = g_ring_num[ring]; i < g_menu_num[ring]; i++)
            {
                g_ring_move_tick[ring] = tickcount();
                AnimMoveLine(g_ring_nodes[ring][i], end_x, end_y, end_z, 500.0, 0.0, ANIM_FUNC_EXPONENT);
                AnimColor(g_ring_planes[ring][i], g_ring_color[ring][0], g_ring_color[ring][1], g_ring_color[ring][2],
                        0.0, 500.0, 0.0, ANIM_FUNC_LINEAR);
            }
            g_ring_num_backup[ring]--;
            printf("iRingNumStateBackup[%d] = [%d]\n", ring, g_ring_num_backup[ring]);
        }
        if (g_ring_num[ring] > g_ring_num_backup[ring])
        {
            GetPos(g_ring_nodes[ring][0], x, y, z, Disp2D(0));
            AnimColor(g_window, g_window_r[ring][g_menu_num[ring] - g_ring_num[ring]],
                    g_window_g[ring][g_menu_num[ring] - g_ring_num[ring]],
                    g_window_b[ring][g_menu_num[ring] - g_ring_num[ring]],
                    g_window_a[ring][g_menu_num[ring] - g_ring_num[ring]], 500.0, 0.0, ANIM_FUNC_LINEAR);
            new i;
            for (; i < g_ring_num[ring]; i++)
            {
                g_ring_move_tick[ring] = tickcount();
                AnimMoveLine(g_ring_nodes[ring][i], x + g_ring_extension[ring][0] / float(g_menu_num[ring]) * float(i),
                        y + g_ring_extension[ring][1] / float(g_menu_num[ring]) * float(i),
                        z + g_ring_extension[ring][2] / float(g_menu_num[ring]) * float(i), 500.0, 0.0,
                        ANIM_FUNC_EXPONENT);
                AnimColor(g_ring_planes[ring][i], g_ring_color[ring][0], g_ring_color[ring][1], g_ring_color[ring][2],
                        g_ring_color[ring][3] * g_ring_alpha_pow[ring][i + (g_menu_num[ring] - g_ring_num[ring])],
                        500.0, 0.0, ANIM_FUNC_LINEAR);
            }
            for (; i < g_menu_num[ring]; i++)
            {
                g_ring_move_tick[ring] = tickcount();
                AnimMoveLine(g_ring_nodes[ring][i],
                        x + g_ring_extension[ring][0] / float(g_menu_num[ring]) * float(g_ring_num[ring] - 1),
                        y + g_ring_extension[ring][1] / float(g_menu_num[ring]) * float(g_ring_num[ring] - 1),
                        z + g_ring_extension[ring][2] / float(g_menu_num[ring]) * float(g_ring_num[ring] - 1), 500.0,
                        0.0, ANIM_FUNC_EXPONENT);
                AnimColor(g_ring_planes[ring][i], g_ring_color[ring][0], g_ring_color[ring][1], g_ring_color[ring][2],
                        0.0, 500.0, 500.0 / 3.2, ANIM_FUNC_LINEAR);
            }
            g_ring_num_backup[ring]++;
            printf("iRingNumStateBackup[%d] = [%d]\n", ring, g_ring_num_backup[ring]);
        }
    }
}

/* Starts the moves of g_ring_move_pattern, once g_ring_move_pending is set. */
move_rings()
{
    if (0 == g_ring_move_pending)
    {
        return;
    }
    g_ring_move_pending = 0;
    new Float:x;
    new Float:y;
    new Float:z;
    new step;
    if (RINGMOVE_MAIN == g_ring_move_pattern)
    {
        printf("[BG]\tNote----------------------------------------------------\n");
        printf("[BG]\tした状態です。intRingMovePattern\t==\tDEF_RINGMOVE_MAIN\n");
        printf("[BG]\tintRingMovePattern\t==\t[%d]\n", g_ring_move_pattern);
    }
    else if (g_ring_move_pattern == RINGMOVE_OPEN)
    {
        for (new ring; ring < 4; ring++)
        {
            if (g_cur_ring == ring)
            {
                AnimColor(g_window, g_window_r[ring][0], g_window_g[ring][0], g_window_b[ring][0], g_window_a[ring][0],
                        500.0, 0.0, ANIM_FUNC_LINEAR);
                if (g_premove_pending[ring] == 1 && (0.0 != g_ring_premove[ring][0] || 0.0 != g_ring_premove[ring][1]
                        || 0.0 != g_ring_premove[ring][2]))
                {
                    for (new i; i < g_menu_num[ring]; i++)
                    {
                        g_ring_move_tick[ring] = tickcount();
                        AnimMoveLine(g_ring_nodes[ring][i], g_ring_premove[ring][0], g_ring_premove[ring][1],
                                g_ring_premove[ring][2], 500.0, 0.0, ANIM_FUNC_EXPONENT);
                    }
                    g_ring_move_tick[ring] = tickcount();
                    g_ring_move_pending = 1;
                    g_premove_pending[ring] = 0;
                    return;
                }
                if (tickcount() < g_ring_move_tick[ring] + floatround(500.0, 1))
                {
                    g_ring_move_pending = 1;
                    g_premove_pending[ring] = 0;
                    return;
                }
                for (new index; index < g_menu_num[ring]; index++)
                {
                    if (g_menu_num[ring] > 1)
                    {
                        step = index;
                    }
                    else
                    {
                        step = 1;
                    }
                    GetPos(g_ring_nodes[ring][index], x, y, z, Disp2D(0));
                    printf("fltRingCol[tmpRing][\tDEF_COLOR_A\t]\t*\tcolPow_a[tmpRing][\tindex\t] == [%f]\n",
                            g_ring_color[ring][3] * g_ring_alpha_pow[ring][index]);
                    AnimColor(g_ring_planes[ring][index], g_ring_color[ring][0], g_ring_color[ring][1],
                            g_ring_color[ring][2], g_ring_color[ring][3] * g_ring_alpha_pow[ring][index], 500.0,
                            float(index) * g_move_speed_to[ring], ANIM_FUNC_EXPONENT);
                    g_ring_move_tick[ring] = tickcount();
                    AnimMoveLine(g_ring_nodes[ring][index],
                            x + g_ring_extension[ring][0] / float(g_menu_num[ring]) * float(step),
                            y + g_ring_extension[ring][1] / float(g_menu_num[ring]) * float(step),
                            z + g_ring_extension[ring][2] / float(g_menu_num[ring]) * float(step), 500.0,
                            float(index) * g_move_speed_to[ring], ANIM_FUNC_EXPONENT);
                }
            }
            else
            {
                for (new i; i < g_menu_num[ring]; i++)
                {
                    AnimColor(g_ring_planes[ring][i], g_ring_color[ring][0], g_ring_color[ring][1],
                            g_ring_color[ring][2], 0.0, 500.0, float(i) * g_move_speed_to[ring], ANIM_FUNC_EXPONENT);
                }
            }
            g_premove_pending[ring] = 1;
        }
    }
    else if (g_ring_move_pattern == RINGMOVE_RETURN)
    {
        AnimColor(g_window, 1.0, 1.0, 1.0, 0.0, 500.0, 0.0, ANIM_FUNC_LINEAR);
        for (new ring; ring < 4; ring++)
        {
            for (new i; i < g_menu_num[ring]; i++)
            {
                GetPos(g_ring_nodes[ring][i], x, y, z, 0);
                if (0.0 != x || 0.0 != y || 0.0 != z)
                {
                    g_ring_move_tick[ring] = tickcount();
                    AnimMoveLine(g_ring_nodes[ring][i], 0.0, 0.0, 0.0, 500.0, 0.0, ANIM_FUNC_EXPONENT);
                }
                if (0 != i)
                {
                    AnimColor(g_ring_planes[ring][i], g_ring_color[ring][0], g_ring_color[ring][1],
                            g_ring_color[ring][2], 0.0, 500.0, 0.0, ANIM_FUNC_LINEAR);
                    continue;
                }
                if (RING_WHATSNEW == g_cur_ring && g_cur_ring == ring)
                {
                    AnimColor(g_ring_planes[0][i], g_ring_color[0][0], g_ring_color[0][1], g_ring_color[0][2],
                            g_ring_color[0][3], 500.0, 0.0, ANIM_FUNC_LINEAR);
                }
                else if (RING_WHATSNEW != g_cur_ring && RING_WHATSNEW == ring)
                {
                    AnimColor(g_ring_planes[0][i], g_ring_color[0][0], g_ring_color[0][1], g_ring_color[0][2],
                            g_ring_color[0][3], 500.0, 0.0, ANIM_FUNC_LINEAR);
                }
                else if (g_cur_ring == ring)
                {
                    AnimColor(g_ring_planes[ring][i], g_ring_color[ring][0], g_ring_color[ring][1],
                            g_ring_color[ring][2], 1.0, 500.0, 0.0, ANIM_FUNC_LINEAR);
                }
                else
                {
                    AnimColor(g_ring_planes[ring][i], g_ring_color[ring][0], g_ring_color[ring][1],
                            g_ring_color[ring][2], 0.5, 500.0, 0.0, ANIM_FUNC_LINEAR);
                }
            }
        }
    }
}
