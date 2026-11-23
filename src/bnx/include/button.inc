/*
 * The plane of a dialog button.
 */

/* Creates an 80x32 nine-slice plane at (x, y), blue and transparent; the caller sets its texture. */
stock create_button(parent, Float:x, Float:y)
{
    new button = CreatePlane3x3(parent);
    SetPos(button, x, y, 0.0, 0);
    SetScale(button, 80.0, 32.0, 1.0);
    SetColor(button, 0.0, 0.54, 0.78, 0.0);
    SetOptionI(button, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(button, TAG("ZOFS"), 1.0);
    SetOptionI(button, TAG("ALPH"), BLEND_ADD);
    new Float:tex_margins[4] = {0.5, 0.5, 0.5, 0.5};
    SetOptionP(button, TAG("MGNT"), tex_margins, 4);
    new Float:scale[3];
    GetScale(button, scale[0], scale[1], scale[2]);
    new Float:margins[4];
    if (scale[0] > scale[1])
    {
        margins[0] = margins[1] = 0.5 * scale[1] / scale[0];
        margins[2] = margins[3] = 0.5;
    }
    else
    {
        margins[2] = margins[3] = 0.5 * scale[0] / scale[1];
        margins[0] = margins[1] = 0.5;
    }
    SetOptionP(button, TAG("MGN"), margins, 4);
    return button;
}

