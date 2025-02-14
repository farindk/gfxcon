#ifndef _INLINE_MULTIPIC_H
#define _INLINE_MULTIPIC_H

#include "mpic_defs.h" 

#ifndef BASE_EXT_DECL
#ifndef C_ONLY
#define BASE_EXT_DECL 
#else
#define BASE_EXT_DECL extern struct Library * MultiPicBase;
#endif
#endif
#ifndef BASE_PAR_DECL
#define BASE_PAR_DECL
#define BASE_PAR_DECL0 void
#endif
#ifndef BASE_NAME
#ifndef C_ONLY
#define BASE_NAME MultiPicLib::MultiPicBase
#else
#define BASE_NAME MultiPicBase
#endif
#endif

static __inline unsigned long 
MP_IoErr (BASE_PAR_DECL0 )
{
  BASE_EXT_DECL
  register struct Library *a6 __asm("a6") = BASE_NAME;
  register unsigned long res __asm("d0");
  __asm __volatile ("jsr a6@(-66)"
  : "=r" (res)
  : "r" (a6)
  : "d0", "memory");
  return res;
}

static __inline void 
MP_Fault (BASE_PAR_DECL unsigned long errcode, void * buff, unsigned long buflen )
{
  BASE_EXT_DECL
  register struct Library *a6 __asm("a6") = BASE_NAME;
  /* register unsigned long res __asm("d0"); */
  register unsigned long d0 __asm("d0") = errcode;
  register unsigned long d1 __asm("d1") = buflen;
  register void * a0 __asm("a0") = buff;
  __asm __volatile ("jsr a6@(-72)"
  : /* "=r" (res) */
  : "r" (a6), "r" (d0), "r" (d1), "r" (a0)
  : "d0", "memory");
  /* return res; */
}

static __inline struct PicInfo * 
MP_Info (BASE_PAR_DECL struct PicHandle * handle )
{
  BASE_EXT_DECL
  register struct Library *a6 __asm("a6") = BASE_NAME;
  register struct PicInfo *res __asm("d0");
  register struct PicHandle * a0 __asm("a0") = handle;
  __asm __volatile ("jsr a6@(-42)"
  : "=r" (res)
  : "r" (a6), "r" (a0)
  : "d0", "memory");
  return res;
}

static __inline struct PicHandle *
MP_Open (BASE_PAR_DECL unsigned char * name, unsigned long flags )
{
  BASE_EXT_DECL
  register struct Library *a6 __asm("a6") = BASE_NAME;
  register struct PicHandle *res __asm("d0");
  register unsigned char *a0 __asm("a0") = name;
  register unsigned long d0 __asm("d0") = flags;
  __asm __volatile ("jsr a6@(-30)"
  : "=r" (res)
  : "r" (a6), "r" (a0), "r" (d0)
  : "d0", "memory");
  return res;
}

static __inline void
MP_Close (BASE_PAR_DECL struct PicHandle * phan)
{
  BASE_EXT_DECL
  register struct Library *a6 __asm("a6") = BASE_NAME;
  register struct PicHandle *a0 __asm("a0") = phan;
  __asm __volatile ("jsr a6@(-36)"
  : 
  : "r" (a6), "r" (a0)
  : "memory");
}

static __inline long
MP_SetBufferAttrs (BASE_PAR_DECL struct PicHandle * phan, struct TagItem * tags)
{
  BASE_EXT_DECL
  register struct Library *a6 __asm("a6") = BASE_NAME;
  register long res __asm("d0");
  register struct PicHandle *a0 __asm("a0") = phan;
  register struct TagItem *a1 __asm("a1") = tags;
  __asm __volatile ("jsr a6@(-60)"
  : "=r" (res)
  : "r" (a6), "r" (a0), "r" (a1)
  : "d0", "memory");
  return res;
}

static __inline long
MP_Read (BASE_PAR_DECL struct PicHandle * phan,
                       unsigned char * red,
                       unsigned char * green,
                       unsigned char * blue,
                       struct TagItem * tags,
                       unsigned long lines)
{
  BASE_EXT_DECL
  register struct Library *a6 __asm("a6") = BASE_NAME;
  register long res __asm("d0");
  register struct PicHandle *a0 __asm("a0") = phan;
  register unsigned char *a1 __asm("a1") = red;
  register unsigned char *a2 __asm("a2") = green;
  register unsigned char *a3 __asm("a3") = blue;
  register struct TagItem *a4 __asm("a4") = tags;
  register unsigned long d0 __asm("d0") = lines;
  __asm __volatile ("jsr a6@(-48)"
  : "=r" (res)
  : "r" (a6), "r" (a0), "r" (a1), "r" (a2), "r" (a3), "r" (a4), "r" (d0)
  : "d0", "memory");
  return res;
}

static __inline unsigned char *
MP_ReadPalette (BASE_PAR_DECL struct PicHandle * phan, unsigned char * buffer, unsigned long type)
{
  BASE_EXT_DECL
  register struct Library *a6 __asm("a6") = BASE_NAME;
  register unsigned char * res __asm("d0");
  register struct PicHandle *a0 __asm("a0") = phan;
  register unsigned char *a1 __asm("a1") = buffer;
  register unsigned long d0 __asm("d0") = type;
  __asm __volatile ("jsr a6@(-54)"
  : "=r" (res)
  : "r" (a6), "r" (a0), "r" (a1), "r" (d0)
  : "d0", "memory");
  return res;
}


#undef BASE_EXT_DECL
#undef BASE_PAR_DECL
#undef BASE_PAR_DECL0
#undef BASE_NAME

#endif
