/*
 * The message text and the button label of a dialog, drawn with alpha blending.
 */

/* Creates the textbox of a dialog message, text_w by text_h pixels, drawn additively. */
stock create_message_text_add(parent, text_w, text_h)
{
    new textbox = CreateTextbox(parent);
    SetOptionI(textbox, TAG("ALPH"), BLEND_ADD);
    SetOptionI(textbox, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(textbox, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(textbox, TAG("FSIZ"), 15);
    SetOptionI(textbox, TAG("TW64"), text_w << 6);
    SetOptionI(textbox, TAG("TH64"), text_h << 6);
    SetOptionI(textbox, TAG("LS64"), 4 * 64);
    return textbox;
}

/* Creates a centered button label at (x, y), drawn additively. */
stock create_button_label_add(parent, Float:x, Float:y)
{
    new label = CreateLabel(parent);
    SetPos(label, x, y, 0.0, 0);
    SetOptionI(label, TAG("ALPH"), BLEND_ADD);
    SetOptionI(label, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(label, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(label, TAG("FSIZ"), 15);
    SetOptionI(label, TAG("AP64"), 1 * 64); /* 1 pixel */
    SetOptionI(label, TAG("KP64"), 1 * 64); /* 1 pixel */
    SetOptionI(label, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(label, TAG("ZOFS"), 2.0);
    return label;
}

