/*
 * /opt0/bn/script/system/fatal.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The illegal disc page: a red cloud over the space background and the illegal disc message, until
 * the disc is changed.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

#include "wait.inc"

#include "tag.inc"

#include "imgsync.inc"

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

#include "imgdec.inc"

#include "misc.inc"

new g_cloud;
new g_layers[2];
new g_labels[1];

main()
{
    SetSystemParam(SYS_PADHELP, 0);
    SystemCommand(SYSCMD_HIDE_WINDOW, 1);
    g_cloud = CreateCloud(Camera(0));
    SetPos(g_cloud, 0.0, 0.0, 1.7, 0);
    SetScale(g_cloud, 1.3, 1.0, 1.0);
    SetColor(g_cloud, 1.0, 0.0, 0.0, 0.0);
    {
        new Float:color_jitter[4] = {0.1, 0.1, 0.0, 0.5};
        SetOptionP(g_cloud, TAG("CDW "), color_jitter, 4);
    }
    SetOptionI(g_cloud, TAG("CLID"), 0);
    SetOptionI(g_cloud, TAG("ZSRT"), ZSORT_BACKGROUND);
    g_layers[0] = CreatePlane(Camera(0));
    SetPos(g_layers[0], 0.0, 0.0, 1.0 * (500.0 * 40.0), 0);
    SetColor(g_layers[0], 1.0, 1.0, 1.0, 0.0);
    SetLocalScale(g_layers[0], 1.0 * (1024.0 * 40.0), 1.0 * (512.0 * 40.0), 1.0 * (1.0 * 40.0));
    {
        new url[64];
        get_tag_attr("IMG", "space_layer1", "href", url, 64);
        start_image_decode2(url);
        finish_image_decode();
        SetTexture(g_layers[0], imgTexID(), 0, -1);
        SetOptionI(g_layers[0], TAG("ALPH"), BLEND_ADD);
        SetColor(g_layers[0], 1.0, 1.0, 1.0, 1.0);
    }
    g_layers[1] = CreatePlane(Camera(0));
    SetPos(g_layers[1], 0.0, 0.0, 1.0 * (500.0 * 25.0), 0);
    SetRot(g_layers[1], 0.0, 0.0, 3.141592, 0, 0);
    SetColor(g_layers[1], 1.0, 1.0, 1.0, 0.0);
    SetLocalScale(g_layers[1], 1.0 * (1024.0 * 25.0), 1.0 * (512.0 * 25.0), 1.0 * (1.0 * 25.0));
    {
        new url[64];
        get_tag_attr("IMG", "space_layer3", "href", url, 64);
        start_image_decode2(url);
        finish_image_decode();
        SetTexture(g_layers[1], imgTexID(), 0, -1);
        SetOptionI(g_layers[1], TAG("ALPH"), BLEND_ADD);
        SetColor(g_layers[1], 1.0, 1.0, 1.0, 1.0);
    }
    new Float:screen_w;
    new Float:screen_h;
    get_screen_size2(screen_w, screen_h);
    new msg[256];
    get_tag_attr("TEXT", "msgtop_localdisk_illegal", "value", msg, 256);
    sndSePlay(SE_WINDOW_OPEN);
    g_labels[0] = CreateLabel(Disp2D(0));
    SetPos(g_labels[0], 0.5 * screen_w, 0.4 * screen_h, 0.0, 0);
    SetColor(g_labels[0], 1.0, 1.0, 1.0, 0.0);
    SetOptionS(g_labels[0], TAG("UTF8"), msg);
    SetOptionI(g_labels[0], TAG("HALN"), ALIGN_CENTER);
    SetOptionI(g_labels[0], TAG("FSIZ"), 15);
    AnimColor(g_cloud, 1.0, 0.0, 0.0, 1.0, float(1000), 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_layers[0], 1.0, 1.0, 1.0, 1.0, float(1000), 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_layers[1], 1.0, 1.0, 1.0, 1.0, float(1000), 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_labels[0], 1.0, 1.0, 1.0, 1.0, float(1000), 0.0, ANIM_FUNC_LINEAR);
    wait_ms(1000);
    /* System parameter 800 is 1 unless changed, and bn never changes it: the page stays until the power is off. */
    if (GetSystemParam(800))
    {
        wait_forever();
    }
    for (; ; )
    {
        if (CDVD_ILLEGAL != GetSystemParam(SYS_CDVD_TYPE))
        {
            break;
        }
        sleep;
    }
    Destroy(Scene(0));
}
