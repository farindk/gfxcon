# Amiga GfxCon - Image Format Converter

This is the source code of the GfxCon image format converter that I started writing in 1993.
The latest version on AmiNet is v1.8c from 1997, but this code appears to be v1.8f with
a few minor fixes and adding the strange CVP image format, which I completely forgot about.

At that time, I used the SAS 6.50 C compiler. I have not tried to compile it since then.

---

````
Short:    Image format converter (V1.8f) for most formats.
Uploader: farindk@trick.informatik.uni-stuttgart.de (Dirk Farin)
Author:   farindk@trick.informatik.uni-stuttgart.de (Dirk Farin)
Type:     gfx/conv


GfxCon V1.8f
============

Image format converter that can load and save most formats:

Load: ILBM, LBM, RGB8, RGBN, PCX, IMG, BMP, RLE4, RLE8, DIB,
      GIF, TIFF, JPEG, RGB-Raw, Targa, CVP

Save: ILBM, PCX, GIF, JPEG, RGB-Raw, CVP, Postscript


Features:
 - virtual memory built in (even with a plain 68000)
 - color-effects and simple transformations are possible
 - shows most information stored in images
 - runs from both shell and WB
 - has a nice GUI
 - font sensitiv
 - 68020-version included
 - runs on all amiga systems


New:
  V1.8f - BOXFIT bugfix: sometimes the image size exceeded the BOXFIT
          parameters
  V1.8e - CVP-format support

  V1.8d - JPEG streams not preceded by a JFIF header are recognized as JPEG
        - a bit more JPEG file information

  V1.8c - GIF-loader doesn't get into an endless loop on corrupt files
        - BMP-24bit (odd-picture-width-error) corrected

  V1.8b - BMP-24bit loader
        - minor bugfix in BOXFIT/BOXFITALL

  V1.8  - More input format supported in the TIFF loader (LZW, ...)
        - Can now load version5-PCX images (24bit)
        - Several bugs fixed in the TIFF,PCX and BMP loaders
        - New shell commands: CROP, CENTERBOX, OFFSET, SORTDIR,
                              UNUSED, RESIZEH,RESIZEV

  V1.7  - RGB-Raw input and output with shell now possible
        - Changing brightness and contrast with shell possible
        - GIF89a transparent color should work now
        - progress infos in Shell may be switched off
        - small bug fix (bad input files produced guru)

  V1.6  - Two ways to proportionally scale an image
        - Show info in shell

  V1.5  - Now runs with shell-parameters too
        - Now runs even with Kick 1.2
        - image background-color may be copied and changed
        - bug fix in GIF save-module

  V1.4  - bug fixes in PCX-load and PCX-save
        - 68020 doesn't need a 68881 any more

  V1.3  - HAM6 working again
        - TIFF-RGB images and bug fixes
        - improved virtual memory

  V1.2  - HAM8
        - Targa-CLUT-images

Requirements:
  Kickstart 1.2
  68020 and Kickstart 2.0 recommended!

Have fun!
````