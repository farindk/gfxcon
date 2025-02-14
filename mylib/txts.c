/*******************************************************************************
 *
 * Copyright (C) 1993  Dirk Farin  <dirk.farin@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 ********************************************************************************/

#include "glib:incs.h"
#include "glib:txtsys.h"
#include "glib:txtstructs.h"
#include "glib:errs.h"
#include "glib:txts.h"

#define DOSERR(x) (DOS_ERROR | ((x)<<16))

static struct MsgData errormsgs_german[] =
{
  NO_MEM                   , "Kein Speicher mehr",
  NO_INTUI204              , "Intuition-Library V2.04 nicht vorhanden",
  NO_GFX204                , "Graphics-Library V2.04 nicht vorhanden",
  NO_DISKFONT204           , "DiskFont-Library V2.04 nicht vorhanden",
  NO_ASL204                , "Asl-Library V2.04 nicht vorhanden",
  NO_GADTOOLS204           , "GadTools-Library V2.04 nicht vorhanden",
  NO_WORKBENCH204          , "Workbench-Library V2.04 nicht vorhanden",
  NO_IFFPARSE204           , "IFF-Parse-Library V2.04 nicht vorhanden",
  NO_LAYERS204             , "Layers-Library V2.04 nicht vorhanden",
  NO_UTILITY204            , "Utility-Library V2.04 nicht vorhanden",
  FONT_NOT_AVAILABLE       , "Der Zeichensatz ist nicht vorhanden",
  ERRWARN_NOFONT_FALLBACK  , "Zeichensatz nicht vorhanden, anderer wird benutzt",
  COULDNT_GET_VISUALINFO   , "VisualInfo konnte nicht bestimmt werden",
  COULDNT_GET_DRAWINFO     , "DrawInfo kann nicht erfragt werden",
  COULDNT_OPEN_WINDOW      , "Konnte das Fenster nicht �ffnen",
  CANT_LOCK_WBSCREEN       , "Workbench konnte nicht verriegelt werden",
  CANT_CREATE_GADGET       , "Konnte Gadget  nicht erzeugen",
  COULDNT_OPEN_FILEREQ     , "Filerequester konnte nicht ge�ffnet werden",
  CANT_CREATE_MSGPORT      , "Kann Message-Port nicht erzeugen",
  CANT_CREATE_APPWIN       , "Kann Application-Window nicht erzeugen",
  CANT_GET_PATH_OF_LOCK    , "Kann den Pfadnamen des Locks nicht bestimmen",
  FILENAME_BUFFER_FULL     , "Speicher f�r Filename zu klein (BUG !!!)",
  NO_IFFHANDLE             , "IFF-Handle konnte nicht erzeugt werden",
  NO_PRINTER_PREFS         , "Printer (Text) - Prefs nicht in ENV:sys gefunden",
  ERR_CLOSE_PRINTERPREFS_FILE , "Printer-Prefs-File konnte nicht geschlossen werden",
  ERR_IFFHANDLE_ERR        , "Fehler mit dem IFF-Handle",
  WIN_DOESNT_FIT_ON_SCREEN , "Bildaufbau pa�t nicht auf den Screen",
  ERROR_DOESNT_EXIST       , "Fehlermeldung nicht gefunden",
  INTERNAL_ERROR           , "Interner Fehler",

#  include "myerrs_ger.h"

  DOSERR(103),"Kein Speicherplatz mehr",
  DOSERR(104),"Prozesstabelle ist voll",
  DOSERR(114),"Befehlszeile nicht korrekt",
  DOSERR(115),"numerisches Argument erwartet",
  DOSERR(116),"Ben�tiges Argument fehlt",
  DOSERR(117),"Argument hinter '=' fehlt",
  DOSERR(118),"Zu viele Argumente",
  DOSERR(119),"Anf�hrungszeichen nicht korrekt",
  DOSERR(120),"Argumentzeile fehlerhaft oder zu lang",
  DOSERR(121),"File ist nicht ausf�hrbar",
  DOSERR(122),"alte Library f�r die DOS-Befehle",
  DOSERR(202),"Objekt wird gerade benutzt",
  DOSERR(203),"Das Objekt existiert schon",
  DOSERR(204),"Verzeichnis nicht vorhanden",
  DOSERR(205),"Objekt nicht gefunden",
  DOSERR(206),"Fehlerhafte Fensterbeschreibung",
  DOSERR(209),"??? Packet request Typ unbekannt ???",
  DOSERR(210),"Objektname ung�ltig",
  DOSERR(211),"??? Ung�ltiger Objekt-Lock ???",
  DOSERR(212),"Typ des Objekt nicht korrekt",
  DOSERR(213),"Disk nicht validiert",
  DOSERR(214),"Disk ist schreibgesch�tzt",
  DOSERR(215),"Umbenennen �ber Devicegrenzen nicht m�glich",
  DOSERR(216),"Verzeichnis ist nicht leer",
  DOSERR(217),"Zu viele Verbindungen",
  DOSERR(218),"Device (oder Volume) nicht gemountet",
  DOSERR(219),"??? Seek-Error ???",
  DOSERR(220),"Kommentar ist zu lang",
  DOSERR(221),"Disk ist voll",
  DOSERR(222),"Objekt ist l�schgesch�tzt",
  DOSERR(223),"File ist schreibgesch�tzt",
  DOSERR(224),"File ist lesegesch�tzt",
  DOSERR(225),"Keine g�ltige DOS-Disk",
  DOSERR(226),"Keine Diskette im Laufwerk",
  DOSERR(232),"??? Keine Eintr�ge mehr im Verzeichnis ???",
  DOSERR(233),"??? Objekt ist nur ein Soft-Link ???",

  DOS_ERROR  ,"DOS-Fehler",

  0,0
};

static struct MsgData errormsgs_english[] =
{
  INTERNAL_ERROR           , "Internal error",
  ERROR_DOESNT_EXIST       , "Could not find error message",
  NO_MEM                   , "Not enough memory",
  NO_INTUI204              , "intuition-library V2.04 missing",
  NO_GFX204                , "Graphics-library V2.04 missing",
  NO_DISKFONT204           , "diskfont-library V2.04 missing",
  NO_ASL204                , "asl-library V2.04 missing",
  NO_GADTOOLS204           , "gadtools-library V2.04 missing",
  NO_WORKBENCH204          , "workbench-library V2.04 missing",
  NO_IFFPARSE204           , "IFF-Parse-library V2.04 missing",
  NO_LAYERS204             , "layers-library V2.04 missing",
  NO_UTILITY204            , "utility-library V2.04 missing",
  FONT_NOT_AVAILABLE       , "font not available",
  ERRWARN_NOFONT_FALLBACK  , "font not available, default font will be used",
  COULDNT_GET_VISUALINFO   , "could not get VisualInfo",
  COULDNT_GET_DRAWINFO     , "could not get DrawInfo",
  COULDNT_OPEN_WINDOW      , "could not open window",
  CANT_LOCK_WBSCREEN       , "can't lock Workbench",
  CANT_CREATE_GADGET       , "could not create gadget",
  COULDNT_OPEN_FILEREQ     , "could not open filerequester",
  CANT_CREATE_MSGPORT      , "could not create message port",
  CANT_CREATE_APPWIN       , "could not create application window",
  CANT_GET_PATH_OF_LOCK    , "could not get the name of the lock's path",
  FILENAME_BUFFER_FULL     , "??? buffer for filename too small ???",
  NO_IFFHANDLE             , "could not create IFF handle",
  NO_PRINTER_PREFS         , "did not find printer prefs in ENV:sys",
  ERR_CLOSE_PRINTERPREFS_FILE , "could not close printer prefs",
  ERR_IFFHANDLE_ERR        , "error using the IFF handle",
  WIN_DOESNT_FIT_ON_SCREEN , "window does not fit on the screen",

#  include "myerrs_eng.h"

  DOSERR(103),"Not enough memory",
  DOSERR(104),"Process table full",
  DOSERR(114),"Bad template",
  DOSERR(115),"Bad number",
  DOSERR(116),"Required argument missing",
  DOSERR(117),"Argument after '=' missing",
  DOSERR(118),"Too many arguments",
  DOSERR(119),"Unmatched quotes",
  DOSERR(120),"argument line invalid or too long",
  DOSERR(121),"File is not executable",
  DOSERR(122),"Invalid resident library",
  DOSERR(202),"Object in use",
  DOSERR(203),"Object already exists",
  DOSERR(204),"Directory not found",
  DOSERR(205),"Object not found",
  DOSERR(206),"Invalid window description",
  DOSERR(209),"??? Packet request type unknown ???",
  DOSERR(210),"Object name invalid",
  DOSERR(211),"??? Invalid object lock ???",
  DOSERR(212),"Object not of required type",
  DOSERR(213),"Disk not validated",
  DOSERR(214),"Disk write protected",
  DOSERR(215),"Rename across devices attempted",
  DOSERR(216),"Directory not empty",
  DOSERR(217),"Too many levels",
  DOSERR(218),"Device (or volume) not mounted",
  DOSERR(219),"??? Seek error ???",
  DOSERR(220),"Comment is too big",
  DOSERR(221),"Disk full",
  DOSERR(222),"Object is protected from deletion",
  DOSERR(223),"File is write protected",
  DOSERR(224),"File is read protected",
  DOSERR(225),"Not a valid DOS disk",
  DOSERR(226),"No disk in drive",
  DOSERR(232),"??? No more entries in directory ???",
  DOSERR(233),"??? Object is soft link ???",

  DOS_ERROR  ,"DOS error",
  0,0
};

static struct MsgData msgs_german[] = /************ GERMAN ***********/
{
#  include "mytxts_ger.h"

  TXT_ERRORREQUEST_TITLE  , "Fehler !!!",
  TXT_ERRORREQUEST_FORMAT , "OK",

  TXT_GERMAN_NAME    , "deutsch",
  TXT_ENGLISH_NAME   , "englisch",
  TXT_FRENCH_NAME    , "franz�sisch",
  TXT_FINNISH_NAME   , "finnisch",
  TXT_ITALIAN_NAME   , "italienisch",
  TXT_SPANISH_NAME   , "spanisch",
  0,0
};

static struct MsgData msgs_english[]=
{
#  include "mytxts_eng.h"

  TXT_ERRORREQUEST_TITLE  , "Error !!!",
  TXT_ERRORREQUEST_FORMAT , "OK",

  TXT_GERMAN_NAME    , "german",
  TXT_ENGLISH_NAME   , "english",
  TXT_FRENCH_NAME    , "french",
  TXT_FINNISH_NAME   , "finnish",
  TXT_ITALIAN_NAME   , "italian",
  TXT_SPANISH_NAME   , "spanish",
  0,0
};

/* !!!! Remember to change NLANGUAGES in txts.h when adding new languages !!!! */

struct LanguageInfo alltxts[NLANGUAGES+1] =
{
  LANGUAGE_GERMAN ,errormsgs_german,  msgs_german,
  LANGUAGE_ENGLISH,errormsgs_english, msgs_english,
  LANGUAGE_ILLEGAL,0                , 0
};

