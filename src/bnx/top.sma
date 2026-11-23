/*
 * /opt0/bn/script/top.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The Top Menu page: starts its camera (top_camera) and its list (top_list) and watches the disc drive; a new disc
 * that the detection mode takes leaves for the page that plays it.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

#include "topmenu.inc"
#include "musicdb.inc"

/* Bits of new_media_detect. */
const MEDIA_CDVD = 1;
const MEDIA_USB = 2;

#include "top.inc"

#include "wait.inc"

#include "tag.inc"

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

/* Sends the camera of the background window back home ("RELS"), animated over duration ms if animate is 1. */
release_bg_camera(animate, Float:duration)
{
    new bg_scene = wndGetScene(WND_BG);
    if (bg_scene)
    {
        SendMessage(bg_scene, TAG("RELS"), animate, duration, 0, 0);
    }
}

#include "disc.inc"

public enable_media_detect; /* the detection mode, DETECT_*, set by top_list; 0 while leaving the page */
public new_media_detect; /* the MEDIA_* bits of what changed in this frame */
public cdvd_media_status; /* the disc type, 0 if the detection mode does not take it */
public usb_media_status = 511; /* the USB device, above 511 when there is one; never set here */
new g_list;

main()
{
    start_top_page(0);
    topInit();
    soundBgmStop();
    new info[256];
    new frames;
    /* Waits, 600 frames at most, until the BGM player has stopped. */
    info[0] = 1;
    while (0 != info[0])
    {
        soundGetInfo(5, info, 256);
        printf("info = %d\n", info);
        sleep;
        frames++;
        if (frames > 600)
        {
            break;
        }
    }
    sleep;
    new camera = Camera(0);
    SetPos(camera, 0.0, 0.0, 0.0, 0);
    AttachScript(camera, "camera_main", "top_camera");
    new dialog_obj;
    dialog_obj = CreateNull(Self());
    AttachScript(dialog_obj, "media_dialog", "");
    {
        new list_args[256];
        new menu_id[64];
        new channel_id[64];
        new item[64];
        if (GetArg("menu_id", menu_id, 64))
        {
            sprintf(list_args, "menu_id = %s ;", menu_id);
            printf("%s !!\n", item); /* NOTE: bug: prints item, still empty, instead of list_args */
        }
        if (GetArg("channel_id", channel_id, 64))
        {
            sprintf(item, "channel_id = %s ;", channel_id);
            append_string(list_args, item, 256);
            printf("%s !!\n", item);
        }
        if (GetArg("discboot", item, 64))
        {
            printf("%s !!\n", item);
            if (0 == atoi(item))
            {
                sprintf(item, "discboot = 0 ;");
                append_string(list_args, item, 256);
            }
        }
        g_list = CreateNull(Self());
        SetPos(g_list, 0.0, 0.0, 0.0, 0);
        AttachScript(g_list, "list_main", "top_list", list_args);
    }
    {
        new first[32];
        if (GetArg("first", first, 32))
        {
            topInitStart();
            while (!topInitSync())
            {
                sleep;
            }
        }
    }
    for (; ; )
    {
        if (ScriptState(g_list) == SCRIPT_FINISHED)
        {
            new init_status = GetPubVar(g_list, "init_status");
            if (0 == init_status) /* redundant as written */
            {
            }
            if (init_status == INIT_ISP_FAILED)
            {
            }
            if (init_status == 2)
            {
            }
            if (init_status == 4)
            {
            }
            if (init_status == 8)
            {
            }
            if (init_status == INIT_NO_ITEM)
            {
            }
        }
        new_media_detect = detect_new_media();
        /* The list is inactive while a disc is being started. */
        if (enable_media_detect > 0 && (new_media_detect > 0 || GetPubVar(dialog_obj, "dialog_active")))
        {
            SetPubVar(g_list, "list_active", 0);
        }
        else if (!GetPubVar(dialog_obj, "dialog_active"))
        {
            SetPubVar(g_list, "list_active", 1);
        }
        sleep;
    }
    wait_forever();
}

/* Reads the disc type through the detection mode; returns the MEDIA_* bits of what changed. */
detect_new_media()
{
    new old_cdvd = cdvd_media_status;
    new old_usb = usb_media_status;
    cdvd_media_status = filter_disc_type(enable_media_detect, GetSystemParam(SYS_CDVD_TYPE));
    if (FindMessage(MSG_NAVIMENU_OPENED))
    {
        return 0;
    }
    new changed;
    if (old_cdvd != cdvd_media_status)
    {
        changed = changed | MEDIA_CDVD;
    }
    if (old_usb != usb_media_status)
    {
    }
    return changed;
}

/* Leaves for the page url with args, after stopping the music and the disc detection. */
load_page(url[], const args[] = "")
{
    release_bg_camera(1, 2000.0);
    avalSet("ChannelTransit", "OTHER_CHANNEL");
    avalSet("ChannelIntoID", "");
    sndBgmStop(1);
    SetPubVar(Parent(0), "enable_media_detect", 0);
    xmlLoadStart(url, "", args, XMLLOAD_MAIN);
    wait_xml_load();
}

/* Returns whether a disc of type can be started from the page. */
is_bootable_media(type)
{
    switch (type)
    {
        case CDVD_PS1_CD .. CDVD_PS2_DVD:
        {
            return 1;
        }
        case CDVD_CDDA:
        {
            return 1;
        }
        case CDVD_DVD_VR, CDVD_DVD_VIDEO:
        {
            return 1;
        }
        case CDVD_ILLEGAL:
        {
            return 0;
        }
        default:
        {
            return 0;
        }
    }
    return 1;
}

/* Leaves for the page that plays a disc of type (the disc page or the CD player) if the detection mode takes it. */
boot_media(mode, type)
{
    new args[64];
    new url[128];
    if (0 == mode)
    {
        return -1;
    }
    if (mode == DETECT_GAME)
    {
        printf("boot jump game!!\n");
        switch (type)
        {
            case CDVD_PS1_CD .. CDVD_PS2_DVD:
            {
                sprintf(args, "BOOT = GAME; MEDIA = %d;", type);
                get_tag_attr("LINK", "script_discpage", "href", url, 128);
                load_page(url, args);
            }
            default:
            {
                return -1;
            }
        }
    }
    else if (mode == DETECT_MUSIC)
    {
        printf("boot jump music!!\n");
        switch (type)
        {
            case CDVD_PS1_CDDA, CDVD_PS2_CDDA, CDVD_CDDA:
            {
                new album_mode[16];
                sprintf(album_mode, "%d", MUSIC_ALBUM_CD);
                avalSet("MusicAlbumMode", album_mode);
                get_tag_attr("LINK", "script_cdplayer", "href", url, 128);
                load_page(url);
            }
            default:
            {
                return -1;
            }
        }
    }
    else if (mode == DETECT_MOVIE)
    {
        printf("boot jump DVD!!\n");
        switch (type)
        {
            case CDVD_DVD_VR, CDVD_DVD_VIDEO:
            {
                sprintf(args, "BOOT = DVD; MEDIA = %d;", type);
                get_tag_attr("LINK", "script_discpage", "href", url, 128);
                load_page(url, args);
            }
            default:
            {
                return -1;
            }
        }
    }
    else
    {
        if (mode == DETECT_PHOTO)
        {
            return -1;
        }
        if (mode == DETECT_ALL)
        {
            switch (type)
            {
                case CDVD_PS1_CD .. CDVD_PS2_DVD:
                {
                    sprintf(args, "BOOT = GAME; MEDIA = %d;", type);
                    get_tag_attr("LINK", "script_discpage", "href", url, 128);
                    load_page(url, args);
                }
                case CDVD_CDDA:
                {
                    new album_mode[16];
                    sprintf(album_mode, "%d", MUSIC_ALBUM_CD);
                    avalSet("MusicAlbumMode", album_mode);
                    get_tag_attr("LINK", "script_cdplayer", "href", url, 128);
                    load_page(url);
                }
                case CDVD_DVD_VR, CDVD_DVD_VIDEO:
                {
                    sprintf(args, "BOOT = DVD; MEDIA = %d;", type);
                    get_tag_attr("LINK", "script_discpage", "href", url, 128);
                    load_page(url, args);
                }
                default:
                {
                    return -1;
                }
            }
        }
    }
    return -1;
}

public usb_new_device = 511; /* the usb_media_status of the page, read when it changes */
public cdvd_new_media; /* the disc type last read from the page */
public dialog_active; /* a disc is being started; read by the page */
public select; /* unused, but part of the layout */

/* Runs on a child of the page: starts a new disc when it can be started. */
public media_dialog()
{
    for (new media; ; )
    {
        new changed = GetPubVar(Parent(0), "new_media_detect");
        if (changed & MEDIA_CDVD)
        {
            cdvd_new_media = GetPubVar(Parent(0), "cdvd_media_status");
        }
        if (changed & MEDIA_USB)
        {
            usb_new_device = GetPubVar(Parent(0), "usb_media_status");
        }
        changed = 0;
        if (cdvd_new_media > CDVD_UNKNOWN && media != cdvd_new_media)
        {
            media = cdvd_new_media;
            new bootable = is_bootable_media(cdvd_new_media);
            if (bootable)
            {
                SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
                dialog_active = 1;
                printf("goto media boot sequence %d!!!!!!!\n", cdvd_new_media);
            }
            else
            {
                dialog_active = 0;
            }
        }
        else if (usb_new_device > 511 && media != usb_new_device)
        {
            dialog_active = 1;
            media = usb_new_device;
        }
        else
        {
            media = cdvd_new_media;
            dialog_active = 0;
        }
        if (dialog_active)
        {
            new mode = GetPubVar(Parent(0), "enable_media_detect");
            boot_media(mode, media);
            dialog_active = 0;
        }
        sleep;
    }
}

#include "topisp.inc"
