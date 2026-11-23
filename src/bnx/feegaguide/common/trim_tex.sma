/*
 * /opt0/bn/script/feegaguide/common/trim_tex.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * Shows a rectangle of an image: decodes the image "src" and shows the part iTrimX, iTrimY, iTrimW, iTrimH of it,
 * following the changes of the rectangle.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "tag.inc"

#include <float>

#include "imgsync.inc"

#include "xmlsync.inc"

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

public iTexWidth;  /* size the image is decoded to */
public iTexHeight; /* with iTexWidth */
public iTrimX;     /* the rectangle shown, in pixels of the decoded image */
public iTrimY; /* with iTrimX */
public iTrimW; /* width of the rectangle */
public iTrimH; /* height of the rectangle */
public iHAlign = 1; /* 0, 1, 2: the left edge, the center or the right edge of the whole image on the origin */
public iVAlign = 1; /* 0, 1, 2: the same along y */
public iZType;      /* the "ZSRT" option of the plane */
new g_objPlane;
new g_iWidth;
new g_iHeight;
new g_iShownX = -1;
new g_iShownY = -1;
new g_iShownW = -1;
new g_iShownH = -1;

main()
{
    new src[128];
    new status;
    new is_tm2;
    GetArg("src", src, 128);
    if (iTexWidth <= 0 || iTexHeight <= 0)
    {
        printf("warning : texture size not defined\n");
    }
    if (strlen(src) <= 0)
    {
        printf("warning : texture name empty\n");
        return;
    }
    is_tm2 = HasExtension(src, "tm2");
    if (is_tm2)
    {
        printf("warning : texture file type is tm2\n");
        return;
    }
    imgDecStart(src, iTexWidth, iTexHeight, -1, 100, 0);
    status = wait_image_decode();
    if (IMG_DONE != status)
    {
        terminate();
    }
    g_iWidth = iTexWidth;
    g_iHeight = iTexHeight;
    g_objPlane = CreatePlane3x3(Self());
    SetColor(g_objPlane, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_objPlane, TAG("ALPH"), BLEND_ALPHA);
    SetOptionI(g_objPlane, TAG("ZSRT"), iZType);
    AnimColor(g_objPlane, 1.0, 1.0, 1.0, 1.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    for (; ; )
    {
        if (iTrimX == g_iShownX && iTrimY == g_iShownY && iTrimW == g_iShownW && iTrimH == g_iShownH)
        {
            sleep;
            continue;
        }
        else
        {
            g_iShownX = iTrimX;
            g_iShownY = iTrimY;
            g_iShownW = iTrimW;
            g_iShownH = iTrimH;
        }
        if (iTrimX > g_iWidth)
        {
            iTrimX = g_iWidth;
        }
        if (iTrimY > g_iHeight)
        {
            iTrimY = g_iHeight;
        }
        if (iTrimX + iTrimW > g_iWidth)
        {
            iTrimW = g_iWidth - iTrimX;
        }
        if (iTrimY + iTrimH > g_iHeight)
        {
            iTrimH = g_iHeight - iTrimY;
        }
        if (iTrimX >= 0 && iTrimY >= 0 && iTrimW > 0 && iTrimH > 0)
        {
            new Float:x = -1.0 * (g_iWidth - iTrimW) / 2 + iTrimX;
            new Float:y = -1.0 * (g_iHeight - iTrimH) / 2 + iTrimY;
            if (0 == iHAlign)
            {
                x = x + g_iWidth / 2;
            }
            else if (iHAlign == 2)
            {
                x = x - g_iWidth / 2;
            }
            if (0 == iVAlign)
            {
                y = y + g_iHeight / 2;
            }
            else if (iVAlign == 2)
            {
                y = y - g_iHeight / 2;
            }
            {
                new Float:tex_margin[4];
                new margin[4];
                tex_margin[0] = float(iTrimX) / float(g_iWidth);
                tex_margin[1] = 1.0 - float(iTrimX + iTrimW) / float(g_iWidth);
                tex_margin[2] = float(iTrimY) / float(g_iHeight);
                tex_margin[3] = 1.0 - float(iTrimY + iTrimH) / float(g_iHeight);
                SetOptionP(g_objPlane, TAG("MGNT"), tex_margin, 4);
                SetOptionP(g_objPlane, TAG("MGN"), margin, 4);
            }
            SetTexture(g_objPlane, imgTexID(), 0, -1);
            SetPos(g_objPlane, x, y, 0.0, 0);
            SetScale(g_objPlane, float(iTrimW), float(iTrimH), 0.0);
        }
        else
        {
            SetScale(g_objPlane, 0.0, 0.0, 0.0);
        }
        sleep;
    }
    return;
}

/* Same as has_extension of string.inc. */
HasExtension(name[], ext[])
{
    new len = strlen(name);
    new ext_len = strlen(ext);
    for (new i; i < ext_len; i++)
    {
        if (toupper(name{len - 1 - i}) != toupper(ext{ext_len - 1 - i}))
        {
            return 0;
        }
    }
    if ('.' != toupper(name{len - 1 - ext_len}))
    {
        return 0;
    }
    return 1;
}
