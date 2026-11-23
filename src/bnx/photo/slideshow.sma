/*
 * /opt0/bn/script/photo/slideshow.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The slide show of the photo page: the photos of the photo list on the whole screen, one at a time, by hand or every
 * five seconds, with a toolbar to go back and forward, start, pause and stop. At the end it gives the photo page the
 * index of the last photo shown ("RTRN").
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 2560

#include "photodb.inc"

const BUTTON_TEX_SIZE = 50; /* the width and height of the textures of the buttons, in pixels */
const MAX_VIEWS = 2;        /* the photo shown and the next one */

#include "photo.inc"

/* Same as wait_ms of wait.inc. */
wait_ms(ms)
{
    new frames = ms * FrameRate() / 1000;
    for (new i; i < frames; i++)
    {
        sleep;
    }
}

/* Same as TAG of tag.inc. */
TAG(fourcc[])
{
    return swapchars(fourcc[0]);
}

/* Same as wait_xml_load of xmlsync.inc. */
wait_xml_load()
{
    new status;
    for (; ; )
    {
        status = xmlLoadSync();
        if (0 != status)
        {
            return status;
        }
        sleep;
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

new g_shadow_colors[4] = {0x5F000000, 0x5F000000, 0x5F000000, 0x5F000000}; /* the "SCL4" of the shadowed labels */
new Float:g_screen_width;
new Float:g_screen_height;
new Float:g_screen_depth;

/* Same as get_screen_size of text.inc. */
get_screen_size(&Float:screen_w, &Float:screen_h)
{
    GetScreen(Camera(0), g_screen_width, g_screen_height, g_screen_depth);
    screen_w = g_screen_width;
    screen_h = g_screen_height;
}

/* Same as create_focus_plane2 of focus.inc. */
create_focus_plane(script[])
{
    new plane = CreatePlane3x3(Scene(0));
    SetTexture(plane, imgGetResource(RES_FOCUS), 0, -1);
    SetOptionI(plane, TAG("ALPH"), BLEND_ADD);
    AttachScript(plane, _, script);
    return plane;
}

/* Same as kill_object of focus.inc. */
kill_object(obj)
{
    SendMessage(obj, TAG("KILL"), 0, 0, 0, 0);
}

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

new g_caption_labels[2][2];
new g_caption_index;
new Float:g_caption_x;
new Float:g_caption_y;

/* Creates the labels of show_caption, with a shadow if shadow. */
create_captions(shadow)
{
    new i;
    new j;
    {
        new Float:screen_w;
        new Float:screen_h;
        get_screen_size(screen_w, screen_h);
        g_caption_x = 0.546875 * screen_w;
        g_caption_y = 0.5 * screen_h;
    }
    i = 0;
    for (; i < 2; i++)
    {
        j = 0;
        for (; j < 2; j++)
        {
            if (!shadow)
            {
                g_caption_labels[i][j] = CreateLabel(Disp2D(0));
            }
            else
            {
                g_caption_labels[i][j] = CreateLabelShadow(Disp2D(0));
            }
            SetPos(g_caption_labels[i][j], g_caption_x, g_caption_y, 0.0, 0);
            SetOptionI(g_caption_labels[i][j], TAG("FONT"), FontID(0));
            SetOptionI(g_caption_labels[i][j], TAG("HALN"), ALIGN_LEFT);
            SetOptionI(g_caption_labels[i][j], TAG("VALN"), ALIGN_CENTER);
            SetOptionI(g_caption_labels[i][j], TAG("FSIZ"), 17);
            if (shadow)
            {
                SetOptionP(g_caption_labels[i][j], TAG("SCL4"), g_shadow_colors, 4);
                SetOptionI(g_caption_labels[i][j], TAG("SX64"), 2 * 64);
                SetOptionI(g_caption_labels[i][j], TAG("SY64"), 2 * 64);
            }
            SetColor(g_caption_labels[i][j], 0.7843, 0.7843, 0.7843, 0.0);
        }
    }
}

/* Same as show_caption of string.inc. */
show_caption(line1[] = "", line2[] = "", Float:target_a, Float:duration, Float:delay)
{
    new incoming = g_caption_index;
    g_caption_index = ++g_caption_index & 1;
    new outgoing = g_caption_index;
    SetOptionS(g_caption_labels[incoming][0], TAG("UTF8"), line1);
    SetOptionS(g_caption_labels[incoming][1], TAG("UTF8"), line2);
    if (strlen(line2) > 0)
    {
        SetPos(g_caption_labels[incoming][0], g_caption_x, g_caption_y - 12.0, 0.0, 0);
        SetPos(g_caption_labels[incoming][1], g_caption_x, 12.0 + g_caption_y, 0.0, 0);
    }
    else
    {
        SetPos(g_caption_labels[incoming][0], g_caption_x, g_caption_y, 0.0, 0);
        SetPos(g_caption_labels[incoming][1], g_caption_x, g_caption_y, 0.0, 0);
    }
    AnimColor(g_caption_labels[incoming][0], 0.7843, 0.7843, 0.7843, target_a, duration, delay, ANIM_FUNC_LINEAR);
    AnimColor(g_caption_labels[incoming][1], 0.7843, 0.7843, 0.7843, target_a, duration, delay, ANIM_FUNC_LINEAR);
    AnimColor(g_caption_labels[outgoing][0], 0.7843, 0.7843, 0.7843, 0.0, 0.5 * duration, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_caption_labels[outgoing][1], 0.7843, 0.7843, 0.7843, 0.0, 0.5 * duration, 0.0, ANIM_FUNC_LINEAR);
}

public slide_mode = SLIDE_STEP; /* SLIDE_*; the "slideshow" argument "on" starts it at SLIDE_PLAY */

/* Same as set_caption_pos of misc.inc. */
set_caption_pos(Float:x, Float:y)
{
    g_caption_x = x;
    g_caption_y = y;
}

/* Keeps the alpha of the task bar (SYS_FG_ALPHA) on the alpha of its object, which "FADE" (alpha, time) fades. */
public taskbar_manager()
{
    for (; ; )
    {
        sleep;
        new msg;
        new Float:alpha;
        new Float:duration;
        while (RecvMessage(msg, alpha, duration))
        {
            if (msg == TAG("FADE"))
            {
                AnimColor(Self(), 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
            }
        }
        new Float:r;
        new Float:g;
        new Float:b;
        new Float:a;
        GetColor(Self(), r, g, b, a);
        SetSystemParam(SYS_FG_ALPHA, floatround(255.0 * a, 0));
    }
}

/* Shows the caption that the photo is being read. */
show_reading()
{
    new text[16];
    get_tag_attr("TEXT", "msg_reading", "value", text, 16);
    show_caption(text, _, 1.0, 400.0, 0.0);
}

/* Shows the caption that the photo cannot be decoded. */
show_decode_error()
{
    new text[16];
    get_tag_attr("TEXT", "msg_decode_error", "value", text, 16);
    show_caption(text, _, 1.0, 400.0, 0.0);
}

/* Returns whether a USB mass storage device is connected. */
is_usb_storage()
{
    new storage;
    new count = avalGetNum("BB_USB_MEDIA");
    if (count == 1)
    {
        new media[64];
        avalGetValue("BB_USB_MEDIA", 0, media, 64);
        if (!strcmp(media, "STORAGE"))
        {
            storage = 1;
        }
    }
    return storage;
}

main()
{
    new index = -1; /* the photo of the list shown; -1: the photo of the "file" argument alone */
    {
        new arg[8];
        if (GetArg("select", arg, 8))
        {
            index = atoi(arg);
        }
    }
    new photo_mode = PHOTOS_LIBRARY;
    {
        new arg[8];
        if (GetArg("mode", arg, 8))
        {
            if (!strcmp(arg, "LIBRARY"))
            {
                photo_mode = PHOTOS_LIBRARY;
            }
            else if (!strcmp(arg, "FAVORITE"))
            {
                photo_mode = PHOTOS_FAVORITE;
            }
            else if (!strcmp(arg, "USB"))
            {
                photo_mode = PHOTOS_USB;
            }
            else
            {
                printf("album mode take miss !!\n");
            }
        }
    }
    new count = photoListNum();
    new Float:screen_w;
    new Float:screen_h;
    get_screen_size(screen_w, screen_h);
    SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
    new focus = create_focus_plane("focus");
    SetOptionI(focus, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(focus, TAG("ZOFS"), -1.0);
    new curtain = CreatePlane(Disp2D(0));
    SetColor(curtain, 0.0, 0.0, 0.0, 0.0);
    SetPos(curtain, 0.5 * screen_w, 0.5 * screen_h, 0.0, 0);
    SetOptionI(curtain, TAG("ZSRT"), ZSORT_BACKGROUND);
    SetLocalScale(curtain, screen_w, screen_h, 1.0);
    new toolbar = CreateNull(curtain);
    SetOptionI(toolbar, TAG("ZSRT"), ZSORT_OVERLAY);
    SetPos(toolbar, 0.0, 0.35 * screen_h, 0.0, 0);
    {
        new back_images[3][64];
        new start_images[3][64];
        new next_images[3][64];
        new pause_images[3][64];
        new stop_images[3][64];
        get_tag_attr("IMG", "full_back0", "src", back_images[0], 64);
        get_tag_attr("IMG", "full_back1", "src", back_images[1], 64);
        get_tag_attr("IMG", "full_back2", "src", back_images[2], 64);
        get_tag_attr("IMG", "full_next0", "src", next_images[0], 64);
        get_tag_attr("IMG", "full_next1", "src", next_images[1], 64);
        get_tag_attr("IMG", "full_next2", "src", next_images[2], 64);
        get_tag_attr("IMG", "full_start0", "src", start_images[0], 64);
        get_tag_attr("IMG", "full_start1", "src", start_images[1], 64);
        get_tag_attr("IMG", "full_start2", "src", start_images[2], 64);
        get_tag_attr("IMG", "full_pause0", "src", pause_images[0], 64);
        get_tag_attr("IMG", "full_pause1", "src", pause_images[1], 64);
        get_tag_attr("IMG", "full_pause2", "src", pause_images[2], 64);
        get_tag_attr("IMG", "full_stop0", "src", stop_images[0], 64);
        get_tag_attr("IMG", "full_stop1", "src", stop_images[1], 64);
        get_tag_attr("IMG", "full_stop2", "src", stop_images[2], 64);
        new args[512];
        sprintf(args,
                "btn00=%s;btn01=%s;btn02=%s;btn10=%s;btn11=%s;btn12=%s;btn20=%s;btn21=%s;btn22=%s;btn30=%s;btn31=%s;btn32=%s;btn40=%s;btn41=%s;btn42=%s;tex_size=%d",
                back_images[0], back_images[1], back_images[2], next_images[0], next_images[1], next_images[2],
                start_images[0], start_images[1], start_images[2], pause_images[0], pause_images[1], pause_images[2],
                stop_images[0], stop_images[1], stop_images[2], BUTTON_TEX_SIZE);
        AttachScript(toolbar, _, "toolbar_emb", args);
        SetPubVar(toolbar, "manager", Self());
        SetPubVar(toolbar, "focus", focus);
        SetPubVar(toolbar, "padhelp", PADHELP_CROSS);
        SetPubVar(toolbar, "alpha_blend_mode", BLEND_ALPHA);
        SetPubVar(toolbar, "expand", 0);
        SetPubVar(toolbar, "cursor", BUTTON_START);
        SendMessage(toolbar, TAG("FADE"), 0, 0, 0, 0);
    }
    new taskbar = CreateNull(curtain);
    AttachScript(taskbar, "taskbar_manager", "");
    AnimColor(curtain, 0.0, 0.0, 0.0, 1.0, 1000.0, 0.0, ANIM_FUNC_LINEAR);
    SendMessage(taskbar, TAG("FADE"), 0, 1000.0, 0, 0);
    create_captions(1);
    set_caption_pos(0.675 * screen_w, 0.9 * screen_h);
    new next_ready = 1;
    new next_broken;
    new due = 1;
    new at_end;
    new jumped;
    new load_next;
    new view;
    new next_view;
    new focused;
    new next_index = index;
    new views;
    new value[64]; /* the "slideshow" argument, then the next photo and the "bref" page */
    if (GetArg("slideshow", value, 64))
    {
        if (!strcmp(value, "on"))
        {
            slide_mode = SLIDE_PLAY;
        }
    }
    if (SLIDE_PLAY != slide_mode)
    {
        SendMessage(toolbar, TAG("ENBL"), BUTTON_PAUSE, 0, 0, 0);
    }
    /*
     * Each photo is a full_screen object (a view): it answers "REDY" (a: 1 if broken) when decoded and "FNSH" when it
     * ends. The next view is made while one is shown and takes its place when it is due. The views and the toolbar
     * send "PUSH" (a: the button index, b: the button); select moves the focus between the view and the toolbar.
     */
    for (; ; )
    {
        sleep;
        new msg;
        new a;
        new b;
        if (RecvMessage(msg, a, b))
        {
            if (msg == TAG("FOCS"))
            {
                SendMessage(focused, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
            }
            else if (msg == TAG("PUSH"))
            {
                new pusher = focused;
                if (b == TAG("RDWN"))
                {
                    if (focused != toolbar)
                    {
                        sndSePlay(SE_CANCEL);
                        break;
                    }
                    goto toggle_toolbar;
                }
                if (b == TAG("RRGT"))
                {
                    switch (a)
                    {
                        case BUTTON_BACK:
                        {
                            prev_photo:
                            if (next_index >= 0 && index > 0)
                            {
                                if (next_view)
                                {
                                    SendMessage(next_view, TAG("FNSH"), 0, 0, 0, 0);
                                    next_view = 0;
                                }
                                next_index = index - 1;
                                at_end = 0;
                                next_ready = 1;
                                next_broken = 0;
                                due = 1;
                                slide_mode = SLIDE_STEP;
                                SendMessage(toolbar, TAG("ENBL"), BUTTON_PAUSE, 0, 0, 0);
                                jumped = 1;
                                continue;
                            }
                        }
                        case BUTTON_NEXT:
                        {
                            next_photo:
                            if (next_index >= 0 && index < count - 1)
                            {
                                if (next_view)
                                {
                                    SendMessage(next_view, TAG("FNSH"), 0, 0, 0, 0);
                                    next_view = 0;
                                }
                                next_index = index + 1;
                                at_end = 0;
                                next_ready = 1;
                                next_broken = 0;
                                due = 1;
                                slide_mode = SLIDE_STEP;
                                SendMessage(toolbar, TAG("ENBL"), BUTTON_PAUSE, 0, 0, 0);
                                jumped = 1;
                                continue;
                            }
                        }
                        case BUTTON_START:
                        {
                            slide_mode = SLIDE_PLAY;
                            SendMessage(toolbar, TAG("ENBL"), BUTTON_PAUSE, 1, 0, 0);
                        }
                        case BUTTON_PAUSE:
                        {
                            toggle_pause:
                            switch (slide_mode)
                            {
                                case SLIDE_PAUSED, SLIDE_STEP:
                                {
                                    slide_mode = SLIDE_PLAY;
                                    SendMessage(toolbar, TAG("ENBL"), BUTTON_PAUSE, 1, 0, 0);
                                }
                                case SLIDE_PLAY:
                                {
                                    slide_mode = view ? SLIDE_PAUSED : SLIDE_STEP;
                                }
                            }
                        }
                        case BUTTON_STOP:
                        {
                            break;
                        }
                    }
                }
                else if (b == TAG("SLCT"))
                {
                    toggle_toolbar:
                    if (focused == toolbar)
                    {
                        SendMessage(focused, TAG("FADE"), 0, 500.0, 0, 0);
                        SendMessage(focused, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                        send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                        focused = view ? view : next_view;
                        SendMessage(focused, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
                    }
                    else if (next_index >= 0 && count >= 2)
                    {
                        SendMessage(focused, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                        focused = toolbar;
                        SendMessage(focused, TAG("FADE"), 1.0, 500.0, 0, 0);
                        SendMessage(focused, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
                    }
                }
                else
                {
                    if (b == TAG("STRT") && count >= 2)
                    {
                        sndSePlay(SE_DECIDE);
                        goto toggle_pause;
                    }
                    if (b == TAG("L1  "))
                    {
                        if (index > 0)
                        {
                            sndSePlay(SE_DECIDE);
                            goto prev_photo;
                        }
                        sndSePlay(SE_CANCEL);
                    }
                    else if (b == TAG("R1  "))
                    {
                        if (index < count - 1)
                        {
                            sndSePlay(SE_DECIDE);
                            goto next_photo;
                        }
                        sndSePlay(SE_CANCEL);
                    }
                }
                SendMessage(pusher, TAG("PUSH"), 0, 0, 0, 0);
            }
            else if (msg == TAG("REDY"))
            {
                next_ready = 1;
                next_broken = a;
                show_caption(_, _, 0.0, 400.0, 0.0);
            }
            else if (msg == TAG("FNSH"))
            {
                views--;
            }
        }
        if (photo_mode == PHOTOS_USB)
        {
            if (!is_usb_storage())
            {
                break;
            }
        }
        if (SLIDE_PAUSED == slide_mode)
        {
            continue;
        }
        if (due)
        {
            if (at_end)
            {
                break;
            }
            if (next_ready)
            {
                if (next_view)
                {
                    if (view)
                    {
                        SendMessage(view, TAG("FNSH"), 0, 0, 0, 0);
                    }
                    view = next_view;
                    next_view = 0;
                    if (next_broken)
                    {
                        show_decode_error();
                        next_broken = 0;
                    }
                    else
                    {
                        SendMessage(view, TAG("STRT"), 0, 0, 0, 0);
                    }
                    index = next_index;
                    if (0 == index)
                    {
                        SendMessage(toolbar, TAG("ENBL"), BUTTON_BACK, 0, 0, 0);
                    }
                    else
                    {
                        SendMessage(toolbar, TAG("ENBL"), BUTTON_BACK, 1, 0, 0);
                    }
                    if (index == count - 1)
                    {
                        SendMessage(toolbar, TAG("ENBL"), BUTTON_NEXT, 0, 0, 0);
                    }
                    else
                    {
                        SendMessage(toolbar, TAG("ENBL"), BUTTON_NEXT, 1, 0, 0);
                    }
                    if (next_index >= 0)
                    {
                        next_index++;
                    }
                    if (focused != toolbar)
                    {
                        focused = view;
                        SendMessage(focused, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
                    }
                    else if (jumped)
                    {
                        SendMessage(focused, TAG("PUSH"), 0, 0, 0, 0);
                    }
                    jumped = 0;
                    if (slide_mode == SLIDE_STEP)
                    {
                        slide_mode = SLIDE_PAUSED;
                        continue;
                    }
                    AnimTimerManager(Self(), 5000.0, 0.0, ANIM_FUNC_LINEAR);
                    due = 0;
                }
                if (next_index < 0)
                {
                    GetArg("file", value, 64);
                    load_next = 1;
                }
                else if (next_index < count)
                {
                    photoListGet(next_index, value, 64);
                    load_next = 1;
                }
                else
                {
                    at_end = 1;
                }
                next_ready = 0;
            }
        }
        else if (ANIM_DONE == AnimGetState(Self(), ANIM_TIMER))
        {
            due = 1;
        }
        if (load_next && views < MAX_VIEWS)
        {
            next_view = CreateNull(curtain);
            {
                new args[64];
                sprintf(args, "image=%s;mode=FIT_TO_SCREEN", value);
                AttachScript(next_view, _, "full_screen", args);
                SetPubVar(next_view, "manager", Self());
            }
            views++;
            if (due)
            {
                show_reading();
            }
            if (focused != toolbar)
            {
                if (!view)
                {
                    focused = next_view;
                }
                if (!jumped)
                {
                    SendMessage(focused, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
                }
            }
            load_next = 0;
        }
    }
    send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    if (view)
    {
        SendMessage(view, TAG("FNSH"), 0, 0, 0, 0);
    }
    if (next_view)
    {
        SendMessage(next_view, TAG("FNSH"), 0, 0, 0, 0);
    }
    AnimColor(curtain, 0.0, 0.0, 0.0, 0.0, 1000.0, 0.0, ANIM_FUNC_LINEAR);
    SendMessage(toolbar, TAG("FADE"), 0, 1000.0, 0, 0);
    SendMessage(toolbar, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
    SendMessage(taskbar, TAG("FADE"), 1.0, 1000.0, 0, 0);
    wait_ms(1000);
    Destroy(curtain);
    kill_object(focus);
    SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
    if (index < 0)
    {
        if (GetArg("bref", value, 64))
        {
            xmlLoadStart(value, "", "", XMLLOAD_MAIN);
            wait_xml_load();
        }
    }
    else
    {
        new scene = wndGetScene(WND_MAIN);
        if (scene)
        {
            SendMessage(scene, TAG("RTRN"), index, 0, 0, 0);
        }
    }
    Destroy(Self());
}

#include "photodate.inc"
