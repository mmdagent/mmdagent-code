/* ----------------------------------------------------------------- */
/*           The Toolkit for Building Voice Interaction Systems      */
/*           "MMDAgent" developed by MMDAgent Project Team           */
/*           http://www.mmdagent.jp/                                 */
/* ----------------------------------------------------------------- */
/*                                                                   */
/*  Copyright (c) 2009-2010  Nagoya Institute of Technology          */
/*                           Department of Computer Science          */
/*                                                                   */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/* - Redistributions of source code must retain the above copyright  */
/*   notice, this list of conditions and the following disclaimer.   */
/* - Redistributions in binary form must reproduce the above         */
/*   copyright notice, this list of conditions and the following     */
/*   disclaimer in the documentation and/or other materials provided */
/*   with the distribution.                                          */
/* - Neither the name of the MMDAgent project team nor the names of  */
/*   its contributors may be used to endorse or promote products     */
/*   derived from this software without specific prior written       */
/*   permission.                                                     */
/*                                                                   */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND            */
/* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,       */
/* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF          */
/* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE          */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS */
/* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          */
/* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED   */
/* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   */
/* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY    */
/* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           */
/* POSSIBILITY OF SUCH DAMAGE.                                       */
/* ----------------------------------------------------------------- */

#define TEXTRENDERER_MAXBUFLEN 1024

#define TEXTRENDERER_FONT     "Arial Unicode MS"
#define TEXTRENDERER_MAXNCHAR 1024               /* maxium number of character stored in display list without ASCII */

#define TEXTRENDERER_ASCIISIZE 256

/* TextRenderer: text renderer */
class TextRenderer
{
private:

   HDC m_hDC;                    /* device context */
   HFONT m_outlineFont;          /* outline font */
   unsigned int m_outlineFontID; /* first 256 ID in ASCII font (outline) */
   unsigned int m_bitmapFontID;  /* first 256 ID in ASCII font (bitmatp) */

   unsigned long m_charList[TEXTRENDERER_MAXNCHAR]; /* 32 bit (sjis) code list without ASCII */
   unsigned int m_idList[TEXTRENDERER_MAXNCHAR];    /* IDs */

   int m_idNum;   /* number of stored characters */
   int m_current; /* current pointer (point next Character) */

   /* initialize: initialize text renderer */
   void initialize();

   /* clear: free text renderer */
   void clear();

public:

   /* TextRenderer: constructor */
   TextRenderer();

   /* ~TextRender: destructor */
   ~TextRenderer();

   /* setup: initialize and setup text renderer */
   void setup(HDC hDC);

   /* drawAsciiStringBitmap: draw ascii string (bitmap) */
   void drawAsciiStringBitmap(char *str);

   /* getDisplayListArrayOfString: get array of display list indices draw any string (outline) */
   int getDisplayListArrayOfString(char *str, unsigned int *idList, int maxlen);

   /* renderSispayListArrayOfString: render the obtained array of display lists for a string */
   void renderDisplayListArrayOfString(unsigned int *idList, int n);

   /* drawString: draw any string (outline, slow) */
   void drawString(char *str);
};