/************************************************************************
   T h e   O p e n   W i n d o w s   P r o j e c t
 ------------------------------------------------------------------------
   Filename   : IniFile.h
   Author(s)  : Carsten Breuer
 ------------------------------------------------------------------------
 Copyright (c) 2000 by Carsten Breuer (CBreuer@openwin.de)
 ************************************************************************/

#ifndef INIFILE_H
#define INIFILE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <w32std.h>
#include <w32std.h>
#include <eikenv.h>

#ifndef OWP_DONT_DEF_FALSE
#ifndef FALSE
#define FALSE 0
#endif
#endif

#ifndef OWP_DONT_DEF_TRUE
#ifndef TRUE
#define TRUE 1
#endif
#endif


#ifndef OWP_DONT_DEF_bool
#ifndef bool
#define bool unsigned int
#endif
#endif

#ifndef OWP_DONT_DEF_CCHR
#ifndef CCHR
#define CCHR const char
#endif
#endif

#ifndef OWP_DONT_DEF_UCHR
#ifndef UCHR
#define UCHR unsigned char
#endif
#endif

#ifndef OWP_DONT_DEF_UCCHR
#ifndef UCCHR
#define UCCHR const unsigned char
#endif
#endif

#ifndef OWP_DONT_DEF_UINT
#ifndef UINT
#define UINT unsigned int
#endif
#endif

#ifndef OWP_DONT_DEF_WORD
#ifndef WORD
#define WORD unsigned short
#endif
#endif

#ifdef LINUX /* Remove CR, on unix systems. */
#define INI_REMOVE_CR
#define DONT_HAVE_STRUPR
#endif

#define DONT_HAVE_STRUPR

#define tpNULL       0
#define tpSECTION    1
#define tpKEYVALUE   2
#define tpCOMMENT    3

struct ENTRY
{
   char   Type;
   char  *pText;
   struct ENTRY *pPrev;
   struct ENTRY *pNext;
};

typedef struct
{
   struct ENTRY *pSec;
   struct ENTRY *pKey;
   char          KeyText [128];
   char          ValText [128];
   char          Comment [255];
} EFIND;

/* Macros */
#define ArePtrValid(Sec,Key,Val) ((Sec!=NULL)&&(Key!=NULL)&&(Val!=NULL))

class CIniFile
{
public:
            CIniFile    (TFileName fileName);
            ~CIniFile   (void);
    UINT    GetVersion  (void);
    bool    ReadFromFile();
    bool    WriteToFile ();

    bool    ReadBool    (CCHR *pSection, CCHR *pKey, bool   Default);
    int     ReadInt     (CCHR *pSection, CCHR *pKey, int    Default);
    double  ReadDouble  (CCHR *pSection, CCHR *pKey, double Default);
    CCHR    *ReadString (CCHR *pSection, CCHR *pKey, CCHR  *pDefault);

    void    WriteBool   (CCHR *pSection, CCHR *pKey, bool   Value);
    void    WriteInt    (CCHR *pSection, CCHR *pKey, int    Value);
    void    WriteDouble (CCHR *pSection, CCHR *pKey, double Value);
    void    WriteString (CCHR *pSection, CCHR *pKey, CCHR  *pValue);

	bool	DeleteKey (CCHR *pSection, CCHR *pKey);

protected:
	struct  ENTRY *m_pEntry;
	struct  ENTRY *m_pCurEntry;
	char    m_result [255];
	FILE    *m_pIniFile;
	void    AddKey     (struct ENTRY *pEntry, CCHR *pKey, CCHR *pValue);
	bool    AddItem    (char Type, CCHR *pText);
	bool    AddItemAt (struct ENTRY *pEntryAt, char Mode, CCHR *pText);
	void    FreeMem    (void *pPtr);
	void    FreeAllMem (void);
	bool    FindKey    (CCHR *pSection, CCHR *pKey, EFIND *pList);
	bool    AddSectionAndKey (CCHR *pSection, CCHR *pKey, CCHR *pValue);
	struct  ENTRY *MakeNewEntry (void);
	struct  ENTRY *FindSection (CCHR *pSection);

	TFileName iFileName;
private:
};

#endif


