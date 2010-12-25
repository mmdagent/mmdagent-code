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

#include <locale.h>
#include <process.h>

#include "mecab.h"
#include "njd.h"
#include "jpcommon.h"
#include "HTS_engine.h"

#include "text2mecab.h"
#include "mecab2njd.h"
#include "njd2jpcommon.h"

#include "njd_set_pronunciation.h"
#include "njd_set_digit.h"
#include "njd_set_accent_phrase.h"
#include "njd_set_accent_type.h"
#include "njd_set_unvoiced_vowel.h"
#include "njd_set_long_vowel.h"

#include "Open_JTalk.h"
#include "Open_JTalk_Thread.h"

/* get_token_from_fp: get token from a file pointer */
static int get_token_from_fp(FILE * fp, char *str)
{
   char c;
   int i;

   str[0] = '\0';

   if (feof(fp))
      return 0;

   c = fgetc(fp);
   while (c == '\n' || c == '\r' || c == '\t' || c == ' ') {
      if (feof(fp))
         return 0;
      c = getc(fp);
   }

   for (i = 0; c != '\n' && c != '\r' && c != '\t' && c != ' ' && !feof(fp); i++) {
      str[i] = c;
      c = fgetc(fp);
   }

   str[i] = '\0';
   return i;
}

/* Open_JTalk_Event_initialize: initialize input message buffer */
void Open_JTalk_Event_initialize(Open_JTalk_Event *e, char *str)
{
   if (str != NULL)
      e->event = _strdup(str);
   else
      e->event = NULL;
   e->next = NULL;
}

/* Open_JTalk_Event_clear: free input message buffer */
void Open_JTalk_Event_clear(Open_JTalk_Event *e)
{
   if (e->event != NULL)
      free(e->event);
   Open_JTalk_Event_initialize(e, NULL);
}

/* Open_JTalk_EventQueue_initialize: initialize queue */
void Open_JTalk_EventQueue_initialize(Open_JTalk_EventQueue *q)
{
   q->head = NULL;
   q->tail = NULL;
}

/* Open_JTalk_EventQueue_clear: free queue */
void Open_JTalk_EventQueue_clear(Open_JTalk_EventQueue *q)
{
   Open_JTalk_Event *tmp1, *tmp2;

   for (tmp1 = q->head; tmp1 != NULL; tmp1 = tmp2) {
      tmp2 = tmp1->next;
      Open_JTalk_Event_clear(tmp1);
      free(tmp1);
   }
   Open_JTalk_EventQueue_initialize(q);
}

/* Open_JTalk_EventQueue_enqueue: enqueue */
void Open_JTalk_EventQueue_enqueue(Open_JTalk_EventQueue *q, char *str)
{
   if (q->tail == NULL) {
      q->tail = (Open_JTalk_Event *) calloc(1, sizeof (Open_JTalk_Event));
      Open_JTalk_Event_initialize(q->tail, str);
      q->head = q->tail;
   } else {
      q->tail->next = (Open_JTalk_Event *) calloc(1, sizeof (Open_JTalk_Event));
      Open_JTalk_Event_initialize(q->tail->next, str);
      q->tail = q->tail->next;
   }
}

/* Open_JTalk_EventQueue_dequeue: dequeue */
int Open_JTalk_EventQueue_dequeue(Open_JTalk_EventQueue *q, char *str)
{
   Open_JTalk_Event *tmp;

   if (q->head == NULL) {
      if (str != NULL)
         str[0] = '\0';
      return 0;
   }
   if (str != NULL)
      strcpy(str, q->head->event);
   tmp = q->head->next;
   Open_JTalk_Event_clear(q->head);
   free(q->head);
   q->head = tmp;
   if (tmp == NULL) q->tail = NULL;
   return 1;
}

/* main_thread: main thread */
static unsigned __stdcall main_thread(void *param)
{
   Open_JTalk_Thread *open_jtalk_thread = (Open_JTalk_Thread *) param;

   while (1)
      open_jtalk_thread->synthesis();
}

/* Open_JTalk_Thread::initialize: initialize thread */
void Open_JTalk_Thread::initialize()
{
   m_window = NULL;
   m_event = NULL;
   m_command = NULL;

   m_threadHandle = 0;
   m_bufferMutex = 0;
   m_synthEvent = 0;

   Open_JTalk_EventQueue_initialize(&m_bufferQueue);

   m_numModels = 0;
   m_modelNames = NULL;
   m_numStyles = 0;
   m_styleNames = NULL;
}

/* Open_JTalk_Thread::clear: free thread */
void Open_JTalk_Thread::clear()
{
   int i;

   m_window = NULL;
   m_event = NULL;
   m_command = NULL;

   /* stop thread & close mutex */
   if (m_threadHandle != 0)
      CloseHandle(m_threadHandle);
   m_threadHandle = 0;
   if (m_bufferMutex)
      CloseHandle(m_bufferMutex);
   m_bufferMutex = 0;
   if (m_synthEvent)
      CloseHandle(m_synthEvent);
   m_synthEvent = 0;

   Open_JTalk_EventQueue_clear(&m_bufferQueue);

   /* free model names */
   if (m_numModels > 0) {
      for (i = 0; i < m_numModels; i++)
         free(m_modelNames[i]);
      free(m_modelNames);
   }
   m_numModels = 0;
   m_modelNames = NULL;

   /* free style names */
   if (m_numStyles > 0) {
      for (i = 0; i < m_numStyles; i++)
         free(m_styleNames[i]);
      free(m_styleNames);
   }
   m_numStyles = 0;
   m_styleNames = NULL;
}

/* Open_JTalk_Thread::Open_JTalk_Thread: thread constructor */
Open_JTalk_Thread::Open_JTalk_Thread()
{
   initialize();
}

/* Open_JTalk_Thread::~Open_JTalk_Thread: thread destructor */
Open_JTalk_Thread::~Open_JTalk_Thread()
{
   clear();
}

/* Open_JTalk_Thread::loadAndStart: load models and start thread */
void Open_JTalk_Thread::loadAndStart(HWND param1, UINT param2, UINT param3, char *dicDir, char *config)
{
   int i, j, k;
   char buff[OPENJTALK_MAXBUFLEN];
   FILE *fp;

   double *weights;

   /* load config file */
   fp = fopen(config, "r");
   if (fp == NULL)
      return;
   /* number of speakers */
   i = get_token_from_fp(fp, buff);
   if (i <= 0) {
      MessageBox(NULL, L"ERROR: Cannot load the number of models in config file of Open JTalk.", L"Error", MB_OK);
      fclose(fp);
      return;
   }
   m_numModels = atoi(buff);
   if (m_numModels <= 0) {
      MessageBox(NULL, L"ERROR: The number of models must be positive value.", L"Error", MB_OK);
      m_numModels = 0;
      fclose(fp);
      return;
   }
   /* model directory names */
   m_modelNames = (char **) calloc(m_numModels, sizeof(char *));
   for (i = 0; i < m_numModels; i++)
      m_modelNames[i] = (char *) calloc(OPENJTALK_MAXBUFLEN, sizeof(char));
   for (i = 0; i < m_numModels; i++) {
      j = get_token_from_fp(fp, buff);
      if (j <= 0) {
         MessageBox(NULL, L"ERROR: Cannot load model directory names in config file of Open JTalk.", L"Error", MB_OK);
         fclose(fp);
         return;
      }
      strcpy(m_modelNames[i], buff);
   }

   /* number of speaking styles */
   i = get_token_from_fp(fp, buff);
   if (i <= 0) {
      MessageBox(NULL, L"ERROR: Cannot load the number of speaking styles in config file of Open JTalk.", L"Error", MB_OK);
      fclose(fp);
      return;
   }
   m_numStyles = atoi(buff);
   if (m_numStyles <= 0) {
      MessageBox(NULL, L"ERROR: The number of speaking styles must be positive value.", L"Error", MB_OK);
      m_numStyles = 0;
      fclose(fp);
      return;
   }
   /* style names and weights */
   m_styleNames = (char **) calloc(m_numStyles, sizeof(char *));
   for (i = 0; i < m_numStyles; i++)
      m_styleNames[i] = (char *) calloc(OPENJTALK_MAXBUFLEN, sizeof(char));
   weights = (double *) calloc((3 * m_numModels + 4) * m_numStyles, sizeof(double));
   for (i = 0; i < m_numStyles; i++) {
      j = get_token_from_fp(fp, buff);
      if (j <= 0) {
         MessageBox(NULL, L"ERROR: Cannot load style definitions in config file of Open JTalk.", L"Error", MB_OK);
         fclose(fp);
         free(weights);
         return;
      }
      strcpy(m_styleNames[i], buff);
      for (j = 0; j < 3 * m_numModels + 4; j++) {
         k = get_token_from_fp(fp, buff);
         if (k <= 0) {
            MessageBox(NULL, L"ERROR: Cannot load style definitions in config file of Open JTalk.", L"Error", MB_OK);
            fclose(fp);
            free(weights);
            return;
         }
         weights[(3 * m_numModels + 4) * i + j] = (double) atof(buff);
      }
   }
   fclose(fp);

   /* load models for TTS */
   if (m_openJTalk.load(dicDir, m_modelNames, m_numModels, weights, m_numStyles) != true) {
      MessageBox(NULL, L"ERROR: Cannot initialize Open JTalk.", L"Error", MB_OK);
      free(weights);
      return;
   }
   setlocale(LC_CTYPE, "japanese");

   free(weights);

   m_window = param1;
   m_event = param2;
   m_command = param3;

   /* create mutex */
   m_bufferMutex = CreateMutex(NULL, false, NULL);
   if (m_bufferMutex == 0) {
      MessageBox(NULL, L"ERROR: Cannot create mutex.", L"Error", MB_OK);
      return;
   }
   m_synthEvent = CreateEvent(NULL, true, false, NULL);
   if (m_synthEvent == 0) {
      MessageBox(NULL, L"ERROR: Cannot create event.", L"Error", MB_OK);
      return;
   }

   /* thread start */
   m_threadHandle = (HANDLE)_beginthreadex(NULL, 0, main_thread, this, 0, NULL);
   if (m_threadHandle == 0) {
      MessageBox(NULL, L"ERROR: Cannot start Open JTalk thread.", L"Error", MB_OK);
      return;
   }
}

/* Open_JTalk_Thread::isStarted: check running */
bool Open_JTalk_Thread::isStarted()
{
   if (m_threadHandle == 0)
      return false;
   return true;
}

/* Open_JTalk_Thread::setSynthParameter: set buffer for synthesis (chara|style|text) */
void Open_JTalk_Thread::setSynthParameter(char *str)
{
   /* wait buffer */
   if (WaitForSingleObject(m_bufferMutex, INFINITE) != WAIT_OBJECT_0)
      MessageBox(NULL, L"ERROR: Cannot wait buffer.", L"Error", MB_OK);

   /* save character name, speaking style, and text */
   Open_JTalk_EventQueue_enqueue(&m_bufferQueue, str);

   /* release buffer */
   ReleaseMutex(m_bufferMutex);

   /* start synthesis thread */
   SetEvent(m_synthEvent);
}

/* Open_JTalk_Thread::synthesis: thread loop for TTS */
void Open_JTalk_Thread::synthesis()
{
   char tmp1[OPENJTALK_MAXBUFLEN];
   char tmp2[OPENJTALK_MAXBUFLEN];
   char *p, *c, *s, *t;
   int s_index;
   int remain = 1;

   /* wait event */
   if (WaitForSingleObject(m_synthEvent, INFINITE) != WAIT_OBJECT_0) {
      MessageBox(NULL, L"ERROR: Cannot wait event.", L"Error", MB_OK);
   }
   ResetEvent(m_synthEvent);

   while (remain) {
      /* wait text */
      if (WaitForSingleObject(m_bufferMutex, INFINITE) != WAIT_OBJECT_0) {
         MessageBox(NULL, L"ERROR: Cannot wait buffer.", L"Error", MB_OK);
      }
      remain = Open_JTalk_EventQueue_dequeue(&m_bufferQueue, tmp1);
      ReleaseMutex(m_bufferMutex);

      if (remain == 0) break;

      /* get character name, style name, and text */
      p = &tmp1[0];
      c = p;
      while (*p != '\0' && *p != '|') p++;
      *(p++) = '\0';
      s = p;
      while (*p != '\0' && *p != '|') p++;
      *(p++) = '\0';
      t = p;
      /* search style index */
      for (s_index = 0; s_index < m_numStyles; s_index++)
         if (strcmp(m_styleNames[s_index], s) == 0)
            break;
      if (s_index >= m_numStyles) /* unknown style */
         s_index = 0;

      /* send SYNTH_EVENT_STOP */
      sendStartEventMessage(c);

      /* synthesize */
      if (t[0] != '\0' && c[0] != '\0') {
         m_openJTalk.setStyle(s_index);
         m_openJTalk.prepare(t);
         m_openJTalk.getPhonemeSequence(tmp2);
         if (strlen(tmp2) > 0) {
            sendLipCommandMessage(c, tmp2);
            m_openJTalk.synthesis();
         }
      }

      /* send SYNTH_EVENT_STOP */
      sendStopEventMessage(c);
   }
}

/* Open_JTalk_Thread::stop: barge-in function */
void Open_JTalk_Thread::stop()
{
   m_openJTalk.stop();
}

/* Open_JTalk_Thread::sendStartEventMessage: send start event message to MMDAgent */
void Open_JTalk_Thread::sendStartEventMessage(char *str)
{
   size_t s;
   size_t len = strlen(str);
   wchar_t *mes1 = (wchar_t *) calloc(wcslen(OPENJTALKTHREAD_EVENTSTART) + 1, sizeof(wchar_t));
   wchar_t *mes2 = (wchar_t *) calloc(len + 1, sizeof(wchar_t));

   wcscpy(mes1, OPENJTALKTHREAD_EVENTSTART);
   mbstowcs_s(&s, mes2, len + 1, str, _TRUNCATE);
   if(s > 0) {
      ::PostMessage(m_window, m_event, (WPARAM) mes1, (LPARAM) mes2);
   } else {
      free(mes1);
      free(mes2);
   }
}

/* Open_JTalk_Thread::sendStopEventMessage: send stop event message to MMDAgent */
void Open_JTalk_Thread::sendStopEventMessage(char *str)
{
   size_t s;
   size_t len = strlen(str);
   wchar_t *mes1 = (wchar_t *) calloc(wcslen(OPENJTALKTHREAD_EVENTSTOP) + 1, sizeof(wchar_t));
   wchar_t *mes2 = (wchar_t *) calloc(len + 1, sizeof(wchar_t));

   wcscpy(mes1, OPENJTALKTHREAD_EVENTSTOP);
   mbstowcs_s(&s, mes2, len + 1, str, _TRUNCATE);
   if(s > 0) {
      ::PostMessage(m_window, m_event, (WPARAM) mes1, (LPARAM) mes2);
   } else {
      free(mes1);
      free(mes2);
   }
}

/* Open_JTalk_Thread::sendLipCommandMessage: send lipsync command message to MMDAgent */
void Open_JTalk_Thread::sendLipCommandMessage(char *chara, char *lip)
{
   size_t s1, s2;
   size_t len1 = strlen(chara);
   size_t len2 = strlen(lip);
   wchar_t *buf1 = (wchar_t *) calloc(len1 + 1, sizeof(wchar_t));
   wchar_t *buf2 = (wchar_t *) calloc(len2 + 1, sizeof(wchar_t));
   wchar_t *mes1 = (wchar_t *) calloc(wcslen(OPENJTALKTHREAD_COMMANDLIP) + 1, sizeof(wchar_t));
   wchar_t *mes2 = (wchar_t *) calloc(len1 + 1 + len2 + 1, sizeof(wchar_t));

   wcscpy(mes1, OPENJTALKTHREAD_COMMANDLIP);

   mbstowcs_s(&s1, buf1, len1 + 1, chara, _TRUNCATE);
   mbstowcs_s(&s2, buf2, len2 + 1, lip, _TRUNCATE);
   wsprintf(mes2, L"%s|%s", buf1, buf2);

   if(s1 > 0 && s2 > 0) {
      ::PostMessage(m_window, m_command, (WPARAM) mes1, (LPARAM) mes2);
   } else {
      free(mes1);
      free(mes2);
   }
   free(buf1);
   free(buf2);
}
