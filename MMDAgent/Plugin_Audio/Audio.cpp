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

/* headers */

#include <stdio.h>
#include <windows.h>

#include "Audio.h"

/* Audio::initialize: initialize audio */
void Audio::initialize()
{
   int i;

   m_window = 0;
   m_needToClose = false;

   for (i = 0; i < AUDIO_MAXNSOUND; i++) {
      m_fileName[i][0] = '\0';
      m_id[i] = -1;
   }
   m_current = 0;
}

/* Audio::clear: free audio */
void Audio::clear()
{
   if (m_needToClose) {
      mciSendStringA("stop all wait", NULL, 0, NULL);
      mciSendStringA("close all wait", NULL, 0, NULL);
   }

   initialize();
}

/* Audio::Audio: constructor */
Audio::Audio()
{
   initialize();
}

/* Audio::~Audio: destructor */
Audio::~Audio()
{
   clear();
}

/* Audio::setup: initialize audio with window handle */
void Audio::setup(HWND hWnd)
{
   m_window = hWnd;
}

/* Audio::release: free audio */
void Audio::release()
{
   clear();
}

/* Audio::play: play audio */
bool Audio::play(char *alias, char *file)
{
   int i, j;
   char buf[AUDIO_MAXBUFLEN];

   /* wait */
   sprintf(buf, "open \"%s\" alias _%s wait", file, alias);
   if (mciSendStringA(buf, NULL, 0, NULL) != 0)
      return false;

   /* check */
   for (i = 0, j = -1; i < AUDIO_MAXNSOUND; i++) {
      if (m_fileName[i][0] == '\0' && j == -1)
         j = i;
      if (strcmp(m_fileName[i], alias) == 0) {
         m_id[i] = mciGetDeviceID(alias);
         break;
      }
   }
   if (i >= AUDIO_MAXNSOUND) {
      if (j == -1)
         return false;
      m_id[j] = mciGetDeviceID(alias);
      strcpy(m_fileName[j], alias);
   }

   /* enqueue */
   sprintf(buf, "cue _%s output wait", alias);
   if (mciSendStringA(buf, NULL, 0, NULL) != 0) {
      sprintf(buf, "close _%s", alias);
      mciSendStringA(buf, NULL, 0, NULL);
      return false;
   }

   /* start */
   sprintf(buf, "play _%s notify", alias);
   if (mciSendStringA(buf, NULL, 0, m_window) != 0) {
      sprintf(buf, "close _%s", alias);
      mciSendStringA(buf, NULL, 0, NULL);
      return false;
   }
   m_needToClose = true; /* set flag to close */

   return true;
}

/* Audio::stop: close audio */
void Audio::stop(char *alias)
{
   int i;
   char buf[AUDIO_MAXBUFLEN];

   /* send close command */
   sprintf(buf, "stop _%s", alias);
   mciSendStringA(buf, NULL, 0, NULL);
   sprintf(buf, "close _%s", alias);
   mciSendStringA(buf, NULL, 0, NULL);

   /* remove file name */
   for (i = 0; i < AUDIO_MAXNSOUND; i++) {
      if (strcmp(m_fileName[i], alias) == 0) {
         m_fileName[i][0] = '\0';
         break;
      }
   }
}

/* Audio::getFinishedAlias: return finished audio alias */
char *Audio::getFinishedAlias(WPARAM wParam, LPARAM lParam)
{
   int i;

   switch (wParam) {
   case MCI_NOTIFY_SUCCESSFUL: /* end of playing */
      for (i = 0; i < AUDIO_MAXNSOUND; i++) {
         if (lParam == m_id[i])
            return m_fileName[i];
      }
      break;
   }
   return NULL;
}
