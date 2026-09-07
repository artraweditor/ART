---
title: Edit Current Image in External Editor
contributors:
  - DrSlony
  - Fherb
tags:
  - 'General Information'
toc: true
---

The "*Edit current image in external editor*" feature allows you to have
RawTherapee fully process the current image and immediately open it in
any external application. You can use this feature to easily send the
image to an image editor such as GIMP or Photoshop for further
processing, or to preview the processed image in an image viewer.

The button to send the image to an external application
![](rt510_edit_in_external_editor_gimp.png)
is located at the bottom-left of the preview panel. When the button is
clicked, the image will be processed and sent to the currently selected
external application. The button's icon and tooltip reflect the current
external application. To select a different application, click on the
drop-down arrow and select an item from the list (this list can be
configured via [Preferences \> External Editor](/preferences#external-editor)). Note that the last
entry in the list is "Other". If this option is selected, clicking the
button will open a list of installed applications. RawTherapee will then
send the processed image to the chosen application.

When using this feature, RawTherapee processes your image and saves it
as a gamma-encoded 16-bit integer TIFF to the temporary folder as
specified in [Preferences \> External Editor \> Output Directory](/preferences#external-editor).
