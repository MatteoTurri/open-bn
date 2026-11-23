/*
 * The title of the edclient pages, TEXT elements and the default labels of the pad help.
 */

/* Draws the TEXT element text_id at the top left of the page and returns its plane. */
stock create_title(text_id[])
{
    new font_size;
    new Float:x;
    new Float:y;
    new Float:tex_w;
    new Float:tex_h;
    new text[64];
    new plane;
    new tex;
    new node = domFindTag(domRoot(), "TEXT", text_id, 0);
    domGetData(node, "value", text, 64);
    font_size = 16;
    imgDecStringStart(text, font_size, 64, 64, 0);
    while (IMG_DONE != wait_image_decode())
    {
        imgDecStringStart(text, font_size, 64, 64, 0);
    }
    x = 64.0;
    y = 88.0 + 20.0;
    tex = imgTexID();
    tex_w = float(imgTexWidth());
    tex_h = float(imgTexHeight());
    x = x + 0.5 * tex_w;
    plane = CreatePlane(Disp2D(0));
    SetTexture(plane, tex, 0, -1);
    SetPos(plane, x, y, 0.0, 0);
    SetColor(plane, 1.0, 1.0, 1.0, 0.0);
    SetScale(plane, tex_w, tex_h, 1.0);
    AnimColor(plane, 1.0, 1.0, 1.0, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
    return plane;
}

/* Sets the text of the TEXT element with that id, creating it if missing. */
stock set_text_value(id[], value[])
{
    new node;
    node = domFindTag(domRoot(), "TEXT", id, 0);
    if (0 == node)
    {
        node = domAddTag(domRoot(), "TEXT");
        domSetData(node, "id", id);
    }
    domSetData(node, "", value);
}

/*
 * Sets the default labels of the pad help buttons in mask (PADHELP_*): circle "Enter", cross "Back", triangle
 * "Option", square "Channel".
 */
stock set_pad_help_labels(mask)
{
    new label[16];
    if (mask & PADHELP_CIRCLE)
    {
        label{0} = 0xE6;
        label{1} = 0xB1;
        label{2} = 0xBA;
        label{3} = 0xE5;
        label{4} = 0xAE;
        label{5} = 0x9A;
        label{6} = 0;
        SetSystemParamStr(SYS_STR_PADHELP_RR, label);
    }
    if (mask & PADHELP_CROSS)
    {
        label{0} = 0xE6;
        label{1} = 0x88;
        label{2} = 0xBB;
        label{3} = 0xE3;
        label{4} = 0x82;
        label{5} = 0x8B;
        label{6} = 0;
        SetSystemParamStr(SYS_STR_PADHELP_RD, label);
    }
    if (mask & PADHELP_TRIANGLE)
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
        SetSystemParamStr(SYS_STR_PADHELP_RU, label);
    }
    if (mask & PADHELP_SQUARE)
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
        SetSystemParamStr(SYS_STR_PADHELP_RL, label);
    }
}

/* Same as set_pad_help_labels. */
stock set_pad_help_labels2(mask)
{
    new label[16];
    if (mask & PADHELP_CIRCLE)
    {
        label{0} = 0xE6;
        label{1} = 0xB1;
        label{2} = 0xBA;
        label{3} = 0xE5;
        label{4} = 0xAE;
        label{5} = 0x9A;
        label{6} = 0;
        SetSystemParamStr(SYS_STR_PADHELP_RR, label);
    }
    if (mask & PADHELP_CROSS)
    {
        label{0} = 0xE6;
        label{1} = 0x88;
        label{2} = 0xBB;
        label{3} = 0xE3;
        label{4} = 0x82;
        label{5} = 0x8B;
        label{6} = 0;
        SetSystemParamStr(SYS_STR_PADHELP_RD, label);
    }
    if (mask & PADHELP_TRIANGLE)
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
        SetSystemParamStr(SYS_STR_PADHELP_RU, label);
    }
    if (mask & PADHELP_SQUARE)
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
        SetSystemParamStr(SYS_STR_PADHELP_RL, label);
    }
}

