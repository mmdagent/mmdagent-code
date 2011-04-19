/* ----------------------------------------------------------------- */
/*           Toolkit for Building Voice Interaction Systems          */
/*           MMDAgent developed by MMDAgent Project Team             */
/*           http://www.mmdagent.jp/                                 */
/* ----------------------------------------------------------------- */
/*                                                                   */
/*  Copyright (c) 2009-2011  Nagoya Institute of Technology          */
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

#include <windows.h>
#include <locale.h>
#include <shellapi.h>

#include "MMDAgent.h"

/* MMDAgent::getNewModelId: return new model ID */
int MMDAgent::getNewModelId()
{
   int i;

   for (i = 0; i < m_numModel; i++)
      if (m_model[i].isEnable() == false)
         return i; /* re-use it */

   if (m_numModel >= m_option->getMaxNumModel())
      return -1; /* no more room */

   i = m_numModel;
   m_numModel++;

   m_model[i].setEnableFlag(false); /* model is not loaded yet */
   return i;
}

/* MMDAgent::removeRelatedModels: delete a model */
void MMDAgent::removeRelatedModels(int modelId)
{
   int i;
   MotionPlayer *motionPlayer;

   /* remove assigned accessories */
   for (i = 0; i < m_numModel; i++)
      if (m_model[i].isEnable() == true && m_model[i].getAssignedModel() == &(m_model[modelId]))
         removeRelatedModels(i);

   /* remove motion */
   for (motionPlayer = m_model[modelId].getMotionManager()->getMotionPlayerList(); motionPlayer; motionPlayer = motionPlayer->next) {
      /* send event message */
      if (MMDAgent_strequal(motionPlayer->name, LIPSYNC_MOTIONNAME))
         sendEventMessage(MMDAGENT_EVENT_LIPSYNCSTOP, "%s", m_model[modelId].getAlias());
      else {
         sendEventMessage(MMDAGENT_EVENT_MOTIONDELETE, "%s|%s", m_model[modelId].getAlias(), motionPlayer->name);
      }
      /* unload from motion stocker */
      m_motion->unload(motionPlayer->vmd);
   }

   /* remove model */
   sendEventMessage(MMDAGENT_EVENT_MODELDELETE, "%s", m_model[modelId].getAlias());
   m_model[modelId].release();
}

/* MMDAgent::updateLight: update light */
void MMDAgent::updateLight()
{
   int i;
   float *f;
   btVector3 l;

   /* udpate OpenGL light */
   m_render->updateLight(m_option->getUseMMDLikeCartoon(), m_option->getUseCartoonRendering(), m_option->getLightIntensity(), m_option->getLightDirection(), m_option->getLightColor());
   /* update shadow matrix */
   f = m_option->getLightDirection();
   m_stage->updateShadowMatrix(f);
   /* update vector for cartoon */
   l = btVector3(f[0], f[1], f[2]);
   for (i = 0; i < m_numModel; i++)
      if (m_model[i].isEnable() == true)
         m_model[i].setLightForToon(&l);
}

/* MMDAgent::setHighLight: set high-light of selected model */
void MMDAgent::setHighLight(int modelId)
{
   float color[4];

   if (m_highLightingModel == modelId) return;

   if (m_highLightingModel != -1) {
      /* reset current highlighted model */
      color[0] = PMDMODEL_EDGECOLORR;
      color[1] = PMDMODEL_EDGECOLORG;
      color[2] = PMDMODEL_EDGECOLORB;
      color[3] = PMDMODEL_EDGECOLORA;
      m_model[m_highLightingModel].getPMDModel()->setEdgeColor(color);
      /* disable force edge flag */
      m_model[m_highLightingModel].getPMDModel()->setForceEdgeFlag(false);
   }
   if (modelId != -1) {
      /* set highlight to the specified model */
      m_model[modelId].getPMDModel()->setEdgeColor(m_option->getCartoonEdgeSelectedColor());
      /* enable force edge flag */
      m_model[modelId].getPMDModel()->setForceEdgeFlag(true);
   }

   m_highLightingModel = modelId;
}

/* MMDAgent::addModel: add model */
bool MMDAgent::addModel(const char *modelAlias, const char *fileName, btVector3 *pos, btQuaternion *rot, const char *baseModelAlias, const char *baseBoneName)
{
   int i;
   int id;
   int baseID;
   char *name;
   btVector3 offsetPos = btVector3(0.0f, 0.0f, 0.0f);
   btQuaternion offsetRot = btQuaternion(0.0f, 0.0f, 0.0f, 1.0f);
   bool forcedPosition = false;
   PMDBone *assignBone = NULL;
   PMDObject *assignObject = NULL;
   float *l = m_option->getLightDirection();
   btVector3 light = btVector3(l[0], l[1], l[2]);

   /* set */
   if (pos)
      offsetPos = (*pos);
   if (rot)
      offsetRot = (*rot);
   if (pos || rot)
      forcedPosition = true;
   if (baseModelAlias) {
      baseID = findModelAlias(baseModelAlias);
      if (baseID < 0) {
         m_logger->log("Error: addModel: %s is not found.", baseModelAlias);
         return false;
      }
      if (baseBoneName) {
         assignBone = m_model[baseID].getPMDModel()->getBone(baseBoneName);
      } else {
         assignBone = m_model[baseID].getPMDModel()->getCenterBone();
      }
      if (assignBone == NULL) {
         if (baseBoneName)
            m_logger->log("Error: addModel: %s is not exist on %s.", baseBoneName, baseModelAlias);
         else
            m_logger->log("Error: addModel: %s don't have center bone.", baseModelAlias);
         return false;
      }
      assignObject = &m_model[baseID];
   }

   /* ID */
   id = getNewModelId();
   if (id == -1) {
      m_logger->log("Error: addModel: number of models exceed the limit.");
      return false;
   }

   /* determine name */
   if (MMDAgent_strlen(modelAlias) > 0) {
      /* check the same alias */
      name = MMDAgent_strdup(modelAlias);
      if (findModelAlias(name) >= 0) {
         m_logger->log("Error: addModel: model alias \"%s\" is already used.", name);
         free(name);
         return false;
      }
   } else {
      /* if model alias is not specified, unused digit is used */
      for(i = 0;; i++) {
         name = MMDAgent_intdup(i);
         if (findModelAlias(name) >= 0)
            free(name);
         else
            break;
      }
   }

   /* add model */
   if (!m_model[id].load(fileName, name, &offsetPos, &offsetRot, forcedPosition, assignBone, assignObject, m_bullet, m_systex, m_lipSync, m_option->getUseCartoonRendering(), m_option->getCartoonEdgeWidth(), &light, m_option->getDisplayCommentFrame())) {
      m_logger->log("Error: addModel: %s cannot be loaded.", fileName);
      m_model[id].release();
      free(name);
      return false;
   }

   /* initialize motion manager */
   m_model[id].resetMotionManager();

   /* send event message */
   sendEventMessage(MMDAGENT_EVENT_MODELADD, "%s", name);
   free(name);
   return true;
}

/* MMDAgent::changeModel: change model */
bool MMDAgent::changeModel(const char *modelAlias, const char *fileName)
{
   int i;
   int id;
   MotionPlayer *motionPlayer;
   double currentFrame;
   double previousFrame;
   float *l = m_option->getLightDirection();
   btVector3 light = btVector3(l[0], l[1], l[2]);

   /* ID */
   id = findModelAlias(modelAlias);
   if (id < 0) {
      m_logger->log("Error: changeModel: %s is not found.", modelAlias);
      return false;
   }

   /* load model */
   if (!m_model[id].load(fileName, modelAlias, NULL, NULL, false, NULL, NULL, m_bullet, m_systex, m_lipSync, m_option->getUseCartoonRendering(), m_option->getCartoonEdgeWidth(), &light, m_option->getDisplayCommentFrame())) {
      m_logger->log("Error: changeModel: %s cannot be loaded.", fileName);
      return false;
   }

   /* update motion manager */
   if (m_model[id].getMotionManager()) {
      for (motionPlayer = m_model[id].getMotionManager()->getMotionPlayerList(); motionPlayer; motionPlayer = motionPlayer->next) {
         if (motionPlayer->active) {
            currentFrame = motionPlayer->mc.getCurrentFrame();
            previousFrame = motionPlayer->mc.getPreviousFrame();
            m_model[id].getMotionManager()->startMotionSub(motionPlayer->vmd, motionPlayer);
            motionPlayer->mc.setCurrentFrame(currentFrame);
            motionPlayer->mc.setPreviousFrame(previousFrame);
         }
      }
   }

   /* delete accessories immediately*/
   for (i = 0; i < m_numModel; i++)
      if (m_model[i].isEnable() && m_model[i].getAssignedModel() == &(m_model[id]))
         removeRelatedModels(i);

   /* send message */
   sendEventMessage(MMDAGENT_EVENT_MODELCHANGE, "%s", modelAlias);
   return true;
}

/* MMDAgent::deleteModel: delete model */
bool MMDAgent::deleteModel(const char *modelAlias)
{
   int i;
   int id;

   /* ID */
   id = findModelAlias(modelAlias);
   if (id < 0) {
      /* wrong alias */
      m_logger->log("Error: deleteModel: %s is not found.", modelAlias);
      return false;
   }

   /* delete accessories  */
   for (i = 0; i < m_numModel; i++)
      if (m_model[i].isEnable() && m_model[i].getAssignedModel() == &(m_model[id]))
         deleteModel(m_model[i].getAlias());

   /* set frame from now to disappear */
   m_model[id].startDisappear();

   /* don't send event message yet */
   return true;
}

/* MMDAgent::addMotion: add motion */
bool MMDAgent::addMotion(const char *modelAlias, const char *motionAlias, const char *fileName, bool full, bool once, bool enableSmooth, bool enableRePos, float priority)
{
   int i;
   bool find;
   int id;
   VMD *vmd;
   MotionPlayer *motionPlayer;
   char *name;

   /* motion file */
   vmd = m_motion->loadFromFile(fileName);
   if (vmd == NULL) {
      m_logger->log("Error: addMotion: %s cannot be loaded.", fileName);
      return false;
   }

   /* ID */
   id = findModelAlias(modelAlias);
   if (id < 0) {
      m_logger->log("Error: addMotion: %s is not found.", modelAlias);
      return false;
   }

   /* alias */
   if (MMDAgent_strlen(motionAlias) > 0) {
      /* check the same alias */
      name = MMDAgent_strdup(motionAlias);
      for (motionPlayer = m_model[id].getMotionManager()->getMotionPlayerList(); motionPlayer; motionPlayer = motionPlayer->next) {
         if (motionPlayer->active && MMDAgent_strequal(motionPlayer->name, name)) {
            m_logger->log("Error: addMotion: motion alias \"%s\" is already used.", name);
            free(name);
            return false;
         }
      }
   } else {
      /* if motion alias is not specified, unused digit is used */
      for(i = 0;; i++) {
         find = false;
         name = MMDAgent_intdup(i);
         for (motionPlayer = m_model[id].getMotionManager()->getMotionPlayerList(); motionPlayer; motionPlayer = motionPlayer->next) {
            if (motionPlayer->active && MMDAgent_strequal(motionPlayer->name, name)) {
               find = true;
               break;
            }
         }
         if(find == false)
            break;
         free(name);
      }
   }

   /* start motion */
   if (m_model[id].startMotion(vmd, name, full, once, enableSmooth, enableRePos, priority) == false) {
      free(name);
      return false;
   }

   sendEventMessage(MMDAGENT_EVENT_MOTIONADD, "%s|%s", modelAlias, name);
   free(name);
   return true;
}

/* MMDAgent::changeMotion: change motion */
bool MMDAgent::changeMotion(const char *modelAlias, const char *motionAlias, const char *fileName)
{
   int id;
   VMD *vmd, *old = NULL;
   MotionPlayer *motionPlayer;

   /* ID */
   id = findModelAlias(modelAlias);
   if (id < 0) {
      m_logger->log("Error: changeMotion: %s is not found.", modelAlias);
      return false;
   }

   /* check */
   if (!motionAlias) {
      m_logger->log("Error: changeMotion: not specified %s.", motionAlias);
      return false;
   }

   /* motion file */
   vmd = m_motion->loadFromFile(fileName);
   if (vmd == NULL) {
      m_logger->log("Error: changeMotion: %s cannot be loaded.", fileName);
      return false;
   }

   /* get motion before change */
   for (motionPlayer = m_model[id].getMotionManager()->getMotionPlayerList(); motionPlayer; motionPlayer = motionPlayer->next) {
      if (motionPlayer->active && MMDAgent_strequal(motionPlayer->name, motionAlias)) {
         old = motionPlayer->vmd;
         break;
      }
   }
   if(old == NULL) {
      m_logger->log("Error: changeMotion: %s is not found.", motionAlias);
      m_motion->unload(vmd);
      return false;
   }

   /* change motion */
   if (m_model[id].swapMotion(vmd, motionAlias) == false) {
      m_logger->log("Error: changeMotion: %s is not found.", motionAlias);
      m_motion->unload(vmd);
      return false;
   }

   /* unload old motion from motion stocker */
   m_motion->unload(old);

   /* send event message */
   sendEventMessage(MMDAGENT_EVENT_MOTIONCHANGE, "%s|%s", modelAlias, motionAlias);
   return true;
}

/* MMDAgent::deleteMotion: delete motion */
bool MMDAgent::deleteMotion(const char *modelAlias, const char *motionAlias)
{
   int id;

   /* ID */
   id = findModelAlias(modelAlias);
   if (id < 0) {
      m_logger->log("Error: deleteMotion: %s is not found.", modelAlias);
      return false;
   }

   /* delete motion */
   if (m_model[id].getMotionManager()->deleteMotion(motionAlias) == false) {
      m_logger->log("Error: deleteMotion: %s is not found.", motionAlias);
      return false;
   }

   /* don't send event message yet */
   return true;
}

/* MMDAgent::startMove: start moving */
bool MMDAgent::startMove(const char *modelAlias, btVector3 *pos, bool local, float speed)
{
   int id;
   btVector3 currentPos;
   btQuaternion currentRot;
   btVector3 targetPos;
   btTransform tr;

   /* ID */
   id = findModelAlias(modelAlias);
   if (id < 0) {
      m_logger->log("Error: startMove: %s is not found.", modelAlias);
      return false;
   }

   if(m_model[id].isMoving() == true)
      sendEventMessage(MMDAGENT_EVENT_MOVESTOP, "%s", modelAlias);

   /* get */
   m_model[id].getCurrentPosition(&currentPos);
   targetPos = (*pos);

   /* local or global */
   if (local) {
      m_model[id].getCurrentRotation(&currentRot);
      tr = btTransform(currentRot, currentPos);
      targetPos = tr * targetPos;
   }

   /* not need to start */
   if (currentPos == targetPos) {
      sendEventMessage(MMDAGENT_EVENT_MOVESTART, "%s", modelAlias);
      sendEventMessage(MMDAGENT_EVENT_MOVESTOP, "%s", modelAlias);
      return true;
   }

   m_model[id].setMoveSpeed(speed);
   m_model[id].setPosition(&targetPos);
   sendEventMessage(MMDAGENT_EVENT_MOVESTART, "%s", modelAlias);
   return true;
}

/* MMDAgent::stopMove: stop moving */
bool MMDAgent::stopMove(const char *modelAlias)
{
   int id;
   btVector3 currentPos;

   /* ID */
   id = findModelAlias(modelAlias);
   if (id < 0) {
      m_logger->log("Error: stopMove: %s is not found.", modelAlias);
      return false;
   }

   if(m_model[id].isMoving() == false) {
      m_logger->log("Error: stopMove: %s is not moving.", modelAlias);
      return false;
   }

   /* get */
   m_model[id].getCurrentPosition(&currentPos);

   m_model[id].setPosition(&currentPos);
   sendEventMessage(MMDAGENT_EVENT_MOVESTOP, "%s", modelAlias);
   return true;
}

/* MMDAgent::startTurn: start turn */
bool MMDAgent::startTurn(const char *modelAlias, btVector3 *pos, bool local, float speed)
{
   int id;
   btVector3 currentPos;
   btQuaternion currentRot;
   btVector3 targetPos;
   btQuaternion targetRot;

   float z, rad;
   btVector3 axis;

   /* ID */
   id = findModelAlias(modelAlias);
   if (id < 0) {
      m_logger->log("Error: startTurn: %s is not found.", modelAlias);
      return false;
   }

   if(m_model[id].isRotating() == true) {
      if(m_model[id].isTurning() == true)
         sendEventMessage(MMDAGENT_EVENT_TURNSTOP, "%s", modelAlias);
      else
         sendEventMessage(MMDAGENT_EVENT_ROTATESTOP, "%s", modelAlias);
   }

   /* get */
   m_model[id].getCurrentPosition(&currentPos);
   m_model[id].getCurrentRotation(&currentRot);

   /* get vector from current position to target position */
   if(local == true)
      targetPos = (*pos);
   else
      targetPos = (*pos) - currentPos;
   targetPos.normalize();

   /* calculate target rotation from (0,0,1) */
   z = targetPos.z();
   if (z > 1.0f) z = 1.0f;
   if (z < -1.0f) z = -1.0f;
   rad = acosf(z);
   axis = btVector3(0.0f, 0.0f, 1.0f).cross(targetPos);
   if(axis.length2() < PMDOBJECT_MINSPINDIFF) {
      targetRot = btQuaternion(0.0f, 0.0f, 0.0f, 1.0f);
   } else {
      axis.normalize();
      targetRot = btQuaternion(axis, btScalar(rad));
   }

   /* local or global */
   if (local)
      targetRot = currentRot * targetRot;
   else
      targetRot = currentRot.nearest(targetRot);

   /* not need to turn */
   if (currentRot == targetRot) {
      sendEventMessage(MMDAGENT_EVENT_TURNSTART, "%s", modelAlias);
      sendEventMessage(MMDAGENT_EVENT_TURNSTOP, "%s", modelAlias);
      return true;
   }

   m_model[id].setSpinSpeed(speed);
   m_model[id].setRotation(&targetRot);
   m_model[id].setTurningFlag(true);
   sendEventMessage(MMDAGENT_EVENT_TURNSTART, "%s", modelAlias);
   return true;
}

/* MMDAgent::stopTurn: stop turn */
bool MMDAgent::stopTurn(const char *modelAlias)
{
   int id;
   btQuaternion currentRot;

   id = findModelAlias(modelAlias);
   if (id < 0) {
      m_logger->log("Error: stopTurn: %s is not found.", modelAlias);
      return false;
   }

   /* not need to stop turn */
   if (m_model[id].isRotating() == false || m_model[id].isTurning() == false) {
      m_logger->log("Error: stopTurn: %s is not turning.", modelAlias);
      return false;
   }

   /* get */
   m_model[id].getCurrentRotation(&currentRot);

   m_model[id].setRotation(&currentRot);
   sendEventMessage(MMDAGENT_EVENT_TURNSTOP, "%s", modelAlias);
   return true;
}

/* MMDAgent::startRotation: start rotation */
bool MMDAgent::startRotation(const char *modelAlias, btQuaternion *rot, bool local, float speed)
{
   int id;
   btQuaternion targetRot;
   btQuaternion currentRot;

   id = findModelAlias(modelAlias);
   if (id < 0) {
      m_logger->log("Error: startRotation: %s is not found.", modelAlias);
      return false;
   }

   if(m_model[id].isRotating() == true) {
      if(m_model[id].isTurning() == true)
         sendEventMessage(MMDAGENT_EVENT_TURNSTOP, "%s", modelAlias);
      else
         sendEventMessage(MMDAGENT_EVENT_ROTATESTOP, "%s", modelAlias);
   }

   /* get */
   m_model[id].getCurrentRotation(&currentRot);
   targetRot = (*rot);

   /* local or global */
   if (local)
      targetRot = currentRot * targetRot;
   else
      targetRot = currentRot.nearest(targetRot);

   /* not need to start */
   if (currentRot == targetRot) {
      sendEventMessage(MMDAGENT_EVENT_ROTATESTART, "%s", modelAlias);
      sendEventMessage(MMDAGENT_EVENT_ROTATESTOP, "%s", modelAlias);
      return true;
   }

   m_model[id].setSpinSpeed(speed);
   m_model[id].setRotation(&targetRot);
   m_model[id].setTurningFlag(false);
   sendEventMessage(MMDAGENT_EVENT_ROTATESTART, "%s", modelAlias);
   return true;
}

/* MMDAgent::stopRotation: stop rotation */
bool MMDAgent::stopRotation(const char *modelAlias)
{
   int id;
   btQuaternion currentRot;

   id = findModelAlias(modelAlias);
   if (id < 0) {
      m_logger->log("Error: stopRotation: %s is not found.", modelAlias);
      return false;
   }

   /* not need to stop rotation */
   if (m_model[id].isRotating() == false || m_model[id].isTurning() == true) {
      m_logger->log("Error: stopRotation: %s is not rotating.", modelAlias);
      return false;
   }

   /* get */
   m_model[id].getCurrentRotation(&currentRot);

   m_model[id].setRotation(&currentRot);
   sendEventMessage(MMDAGENT_EVENT_ROTATESTOP, "%s", modelAlias);
   return true;
}

/* MMDAgent::setFloor: set floor image */
bool MMDAgent::setFloor(const char *fileName)
{
   /* load floor */
   if (m_stage->loadFloor(fileName, m_bullet) == false) {
      m_logger->log("Error: setFloor: %s cannot be set for floor.", fileName);
      return false;
   }

   /* don't send event message */
   return true;
}

/* MMDAgent::setBackground: set background image */
bool MMDAgent::setBackground(const char *fileName)
{
   /* load background */
   if (m_stage->loadBackground(fileName, m_bullet) == false) {
      m_logger->log("Error: setBackground: %s cannot be set for background.", fileName);
      return false;
   }

   /* don't send event message */
   return true;
}

/* MMDAgent::setStage: set stage */
bool MMDAgent::setStage(const char *fileName)
{
   if (m_stage->loadStagePMD(fileName, m_bullet, m_systex) == false) {
      m_logger->log("Error: setStage: %s cannot be set for stage.", fileName);
      return false;
   }

   /* don't send event message */
   return true;
}

/* MMDAgent::changeCamera: change camera setting */
bool MMDAgent::changeCamera(const char *pos, const char *rot, const char *scale, const char *time)
{
   float p[3], r[3];

   if(MMDAgent_str2fvec(pos, p, 3) == true && MMDAgent_str2fvec(rot, r, 3) == true) {
      m_render->resetLocation(p, r, MMDAgent_str2float(scale));
      if (time) {
         m_render->setViewMoveTimer((int)(MMDAgent_str2float(time) * 1000.0f));
         m_timer->start();
      } else
         m_render->setViewMoveTimer(-1);
      return true;
   }

   return false;
}

/* MMDAgent::changeLightColor: change light color */
bool MMDAgent::changeLightColor(float r, float g, float b)
{
   float f[3];

   f[0] = r;
   f[1] = g;
   f[2] = b;
   m_option->setLightColor(f);
   updateLight();

   /* don't send event message */
   return true;
}

/* MMDAgent::changeLightDirection: change light direction */
bool MMDAgent::changeLightDirection(float x, float y, float z)
{
   float f[4];

   f[0] = x;
   f[1] = y;
   f[2] = z;
   f[3] = 0.0f;
   m_option->setLightDirection(f);
   updateLight();

   /* don't send event message */
   return true;
}

/* MMDAgent::startLipSync: start lip sync */
bool MMDAgent::startLipSync(const char *modelAlias, const char *seq)
{
   int id;
   unsigned char *vmdData;
   unsigned long vmdSize;
   VMD *vmd;
   bool find = false;
   MotionPlayer *motionPlayer;

   /* ID */
   id = findModelAlias(modelAlias);
   if (id < 0) {
      m_logger->log("Error: startLipSync: %s is not found.", modelAlias);
      return false;
   }

   /* create motion */
   if(m_model[id].createLipSyncMotion(seq, &vmdData, &vmdSize) == false) {
      m_logger->log("Error: startLipSync: cannot create lip motion.");
      return false;
   }
   vmd = m_motion->loadFromData(vmdData, vmdSize);
   free(vmdData);

   /* search running lip motion */
   for (motionPlayer = m_model[id].getMotionManager()->getMotionPlayerList(); motionPlayer; motionPlayer = motionPlayer->next) {
      if (motionPlayer->active && MMDAgent_strequal(motionPlayer->name, LIPSYNC_MOTIONNAME)) {
         find = true;
         break;
      }
   }

   /* start lip sync */
   if(find == true) {
      if (m_model[id].swapMotion(vmd, LIPSYNC_MOTIONNAME) == false) {
         m_logger->log("Error: startLipSync: lip sync cannot be started.");
         m_motion->unload(vmd);
         return false;
      }
      sendEventMessage(MMDAGENT_EVENT_LIPSYNCSTOP, "%s", modelAlias);
   } else {
      if (m_model[id].startMotion(vmd, LIPSYNC_MOTIONNAME, false, true, true, true, MOTIONMANAGER_DEFAULTPRIORITY) == false) {
         m_logger->log("Error: startLipSync: lip sync cannot be started.");
         m_motion->unload(vmd);
         return false;
      }
   }

   /* send event message */
   sendEventMessage(MMDAGENT_EVENT_LIPSYNCSTART, "%s", modelAlias);
   return true;
}

/* MMDAgent::stopLipSync: stop lip sync */
bool MMDAgent::stopLipSync(const char *modelAlias)
{
   int id;

   /* ID */
   id = findModelAlias(modelAlias);
   if (id < 0) {
      m_logger->log("Error: stopLipSync: %s is not found.", modelAlias);
      return false;
   }

   /* stop lip sync */
   if (m_model[id].getMotionManager()->deleteMotion(LIPSYNC_MOTIONNAME) == false) {
      m_logger->log("Error: stopLipSync: lipsync motion is not found.");
      return false;
   }

   /* don't send message yet */
   return true;
}

/* MMDAgent::initialize: initialize MMDAgent */
void MMDAgent::initialize()
{
   m_configFileName = NULL;
   m_configDirName = NULL;
   m_appDirName = NULL;

   m_hWnd = 0;
   m_hInst = 0;

   m_option = NULL;
   m_bullet = NULL;
   m_plugin = NULL;
   m_screen = NULL;
   m_stage = NULL;
   m_systex = NULL;
   m_lipSync = NULL;
   m_render = NULL;
   m_timer = NULL;
   m_text = NULL;
   m_logger = NULL;

   m_model = NULL;
   m_renderOrder = NULL;
   m_numModel = 0;
   m_motion = NULL;

   m_keyCtrl = false;
   m_keyShift = false;
   m_selectedModel = -1;
   m_highLightingModel = -1;
   m_doubleClicked = false;
   m_mousepos.x = 0;
   m_mousepos.y = 0;
   m_leftButtonPressed = false;

   m_enablePhysicsSimulation = true;
   m_dispLog = false;
   m_dispBulletBodyFlag = false;
   m_dispModelDebug = false;
}

/* MMDAgent::clear: free MMDAgent */
void MMDAgent::clear()
{
   if(m_configFileName)
      free(m_configFileName);
   if(m_configDirName)
      free(m_configDirName);
   if(m_appDirName)
      free(m_appDirName);
   if(m_motion)
      delete m_motion;
   if (m_renderOrder)
      delete [] m_renderOrder;
   if (m_model)
      delete [] m_model;
   if (m_logger)
      delete m_logger;
   if (m_text)
      delete m_text;
   if (m_timer)
      delete m_timer;
   if (m_render)
      delete m_render;
   if (m_lipSync)
      delete m_lipSync;
   if (m_systex)
      delete m_systex;
   if (m_stage)
      delete m_stage;
   if (m_screen)
      delete m_screen;
   if (m_plugin)
      delete m_plugin;
   if (m_bullet)
      delete m_bullet;
   if (m_option)
      delete m_option;

   initialize();
}

/* MMDAgent::MMDAgent: constructor */
MMDAgent::MMDAgent()
{
   initialize();
}

/* MMDAgent::~MMDAgent: destructor */
MMDAgent::~MMDAgent()
{
   clear();
}

/* MMDAgent::setup: initialize and setup MMDAgent */
HWND MMDAgent::setup(HINSTANCE hInstance, const char *title, const char *windowName, int argc, char **argv)
{
   int i;
   size_t len;
   char buff[MMDAGENT_MAXBUFLEN];

   char *binaryFileName;
   char *binaryDirName;

   if(argc < 1 || MMDAgent_strlen(argv[0]) <= 0)
      return 0;

   /* get binary file name */
   binaryFileName = MMDAgent_strdup(argv[0]);

   /* get binary directory name */
   binaryDirName = MMDAgent_dirdup(argv[0]);

   m_hInst = hInstance;

   /* set local to japan */
   setlocale(LC_CTYPE, "jpn");

   /* get application directory */
   if(m_appDirName)
      free(m_appDirName);
   m_appDirName = (char *) malloc(sizeof(char) * (MMDAgent_strlen(binaryDirName) + 1 + MMDAgent_strlen(MMDAGENT_SYSDATADIR) + 1));
   sprintf(m_appDirName, "%s%c%s", binaryDirName, MMDAGENT_DIRSEPARATOR, MMDAGENT_SYSDATADIR);

   /* initialize Option */
   m_option = new Option();

   /* get default config file name */
   strcpy(buff, binaryFileName);
   len = MMDAgent_strlen(buff);
   if (len > 4) {
      buff[len-4] = '.';
      buff[len-3] = 'm';
      buff[len-2] = 'd';
      buff[len-1] = 'f';

      /* load default config file */
      if(m_option->load(buff)) {
         if(m_configFileName)
            free(m_configFileName);
         m_configFileName = MMDAgent_strdup(buff);
      }
   }

   /* load additional config file name */
   for (i = 1; i < argc; i++) {
      if (MMDAgent_strtailmatch(argv[i], ".mdf")) {
         if (m_option->load(argv[i])) {
            if(m_configFileName)
               free(m_configFileName);
            m_configFileName = MMDAgent_strdup(argv[i]);
         }
      }
   }

   /* get config directory name */
   if(m_configDirName)
      free(m_configDirName);
   if(m_configFileName == NULL) {
      m_configFileName = MMDAgent_strdup(binaryFileName);
      m_configDirName = MMDAgent_strdup(binaryDirName);
   } else {
      m_configDirName = MMDAgent_dirdup(m_configFileName);
   }

   /* initialize BulletPhysics */
   m_bullet = new BulletPhysics();
   m_bullet->setup(m_option->getBulletFps());

   /* load and start plugins */
   m_plugin = new Plugin();
   sprintf(buff, "%s%c%s", binaryDirName, MMDAGENT_DIRSEPARATOR, MMDAGENT_PLUGINDIR);
   m_plugin->load(buff);

   /* create window */
   m_screen = new Screen();
   m_hWnd = m_screen->setup(m_option->getWindowSize(), hInstance, title, windowName, m_option->getMaxMultiSampling(), m_option->getMaxMultiSamplingColor(), m_option->getTopMost());
   if (m_hWnd == 0) {
      clear();
      return 0;
   }
   DragAcceptFiles(m_hWnd, true); /* allow drag and drop */

   /* create stage */
   m_stage = new Stage();
   m_stage->setSize(m_option->getStageSize(), 1.0f, 1.0f);

   /* load toon textures from system directory */
   m_systex = new SystemTexture();
   if (m_systex->load(m_appDirName) == false) {
      clear();
      return 0;
   }

   /* setup lipsync */
   m_lipSync = new LipSync();
   sprintf(buff, "%s%c%s", m_appDirName, MMDAGENT_DIRSEPARATOR, LIPSYNC_CONFIGFILE);
   if (m_lipSync->load(buff) == false) {
      clear();
      return 0;
   }

   /* setup render */
   m_render = new Render();
   if (m_render->setup(m_option->getWindowSize(), m_option->getCampusColor(), m_option->getRenderingTransition(), m_option->getRenderingRotation(), m_option->getRenderingScale(), m_option->getUseShadowMapping(), m_option->getShadowMappingTextureSize(), m_option->getShadowMappingLightFirst(), m_option->getMaxNumModel()) == false) {
      clear();
      return 0;
   }

   /* setup timer */
   m_timer = new Timer();
   m_timer->setup(2); /* set timer precision to 2ms */
   m_timer->startAdjustment();

   /* setup text render */
   m_text = new TextRenderer();
   m_text->setup(m_screen->getDC());

   /* setup logger */
   m_logger = new LogText();
   m_logger->setup(m_text, m_option->getLogSize(), m_option->getLogPosition(), m_option->getLogScale());

   /* setup models */
   m_model = new PMDObject[m_option->getMaxNumModel()];
   m_renderOrder = new short[m_option->getMaxNumModel()];

   /* setup motions */
   m_motion = new MotionStocker();

   /* load model from arguments */
   for (i = 1; i < argc; i++)
      if (MMDAgent_strtailmatch(argv[i], ".pmd"))
         addModel(NULL, argv[i], NULL, NULL, NULL, NULL);

   /* set full screen */
   if (m_option->getFullScreen())
      m_screen->setFullScreen(m_hWnd);

   /* set mouse enable timer */
   m_screen->setMouseActiveTime(45.0f);

   /* update light */
   updateLight();

   SetCurrentDirectoryA(m_configDirName);

   m_plugin->execAppStart(this);

   free(binaryFileName);
   free(binaryDirName);

   return m_hWnd;
}

/* MMDAgent::updateScene: update the whole scene */
void MMDAgent::updateScene()
{
   int i, ite;
   double intervalFrame;
   int stepmax;
   double stepFrame;
   double restFrame;
   double procFrame;
   double adjustFrame;
   MotionPlayer *motionPlayer;

   if (!m_hWnd) return;

   /* get frame interval */
   intervalFrame = m_timer->getFrameInterval();

   stepmax = m_option->getBulletFps();
   stepFrame = 30.0 / m_option->getBulletFps();
   restFrame = intervalFrame;

   for (ite = 0; ite < stepmax; ite++) {
      if (restFrame <= stepFrame) {
         /* break */
         procFrame = restFrame;
         ite = stepmax;
      } else {
         /* add stepFrame */
         procFrame = stepFrame;
         restFrame -= stepFrame;
      }
      /* calculate adjustment time for audio */
      adjustFrame = m_timer->getAdditionalFrame(procFrame);
      /* update motion */
      for (i = 0; i < m_numModel; i++) {
         if (m_model[i].isEnable() == false) continue;
         /* update root bone */
         m_model[i].updateRootBone();
         if (m_model[i].updateMotion(procFrame + adjustFrame)) {
            /* search end of motion */
            for (motionPlayer = m_model[i].getMotionManager()->getMotionPlayerList(); motionPlayer; motionPlayer = motionPlayer->next) {
               if (motionPlayer->statusFlag == MOTION_STATUS_DELETED) {
                  /* send event message */
                  if (MMDAgent_strequal(motionPlayer->name, LIPSYNC_MOTIONNAME))
                     sendEventMessage(MMDAGENT_EVENT_LIPSYNCSTOP, "%s", m_model[i].getAlias());
                  else {
                     sendEventMessage(MMDAGENT_EVENT_MOTIONDELETE, "%s|%s", m_model[i].getAlias(), motionPlayer->name);
                  }
                  /* unload from motion stocker */
                  m_motion->unload(motionPlayer->vmd);
               }
            }
         }
         /* update alpha for appear or disappear */
         if (m_model[i].updateAlpha(procFrame + adjustFrame))
            removeRelatedModels(i); /* remove model and accessories */
      }
      /* execute plugin */
      m_plugin->execUpdate(this, procFrame + adjustFrame);
      /* update bullet physics */
      m_bullet->update((float) procFrame);
   }
   /* update after simulation */
   for (i = 0; i < m_numModel; i++)
      if (m_model[i].isEnable() == true)
         m_model[i].updateAfterSimulation(m_enablePhysicsSimulation);

   /* calculate rendering range for shadow mapping */
   if(m_option->getUseShadowMapping())
      m_render->updateDepthTextureViewParam(m_model, m_numModel);

   /* decrement mouse active time */
   m_screen->updateMouseActiveTime(intervalFrame);

   InvalidateRect(m_hWnd, NULL, false);
}

/* MMDAgent::renderScene: render the whole scene */
void MMDAgent::renderScene()
{
   int i;
   char buff[MMDAGENT_MAXBUFLEN];
   btVector3 pos;
   float fps;

   if (!m_hWnd) return;

   /* update model position and rotation */
   fps = m_timer->getFps();
   for (i = 0; i < m_numModel; i++) {
      if (m_model[i].isEnable() == true) {
         if (m_model[i].updateModelRootOffset(fps))
            sendEventMessage(MMDAGENT_EVENT_MOVESTOP, "%s", m_model[i].getAlias());
         if (m_model[i].updateModelRootRotation(fps)) {
            if (m_model[i].isTurning()) {
               sendEventMessage(MMDAGENT_EVENT_TURNSTOP, "%s", m_model[i].getAlias());
               m_model[i].setTurningFlag(false);
            } else {
               sendEventMessage(MMDAGENT_EVENT_ROTATESTOP, "%s", m_model[i].getAlias());
            }
         }
      }
   }

   /* update rendering order */
   m_render->getRenderOrder(m_renderOrder, m_model, m_numModel);

   /* render scene */
   m_render->render(m_model, m_renderOrder, m_numModel, m_stage, m_option->getUseMMDLikeCartoon(), m_option->getUseCartoonRendering(), m_option->getLightIntensity(), m_option->getLightDirection(), m_option->getLightColor(), m_option->getUseShadowMapping(), m_option->getShadowMappingTextureSize(), m_option->getShadowMappingLightFirst(), m_option->getShadowMappingSelfDensity(), m_option->getShadowMappingFloorDensity(), m_render->isViewMoving() ? m_timer->ellapsed() : 0);

   /* show debug display */
   if (m_dispModelDebug)
      for (i = 0; i < m_numModel; i++)
         if (m_model[m_renderOrder[i]].isEnable() == true)
            m_model[m_renderOrder[i]].renderDebug(m_text);

   /* show bullet body */
   if (m_dispBulletBodyFlag)
      m_bullet->debugDisplay();

   /* show log window */
   if (m_dispLog)
      m_logger->render();

   /* count fps */
   m_timer->countFrame();

   /* show fps */
   if (m_option->getShowFps()) {
      sprintf(buff, "%5.1ffps ", m_timer->getFps());
      m_screen->getInfoString(&(buff[9]));
      glDisable(GL_LIGHTING);
      glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
      glPushMatrix();
      glRasterPos3fv(m_option->getFpsPosition());
      m_text->drawAsciiStringBitmap(buff);
      glPopMatrix();
      glEnable(GL_LIGHTING);
   }

   if (m_dispLog) {
      /* show adjustment time for audio */
      if (m_option->getMotionAdjustFrame() > 0)
         sprintf(buff, "%d msec advance (current motion: %+d)", m_option->getMotionAdjustFrame(), (int)(m_timer->getCurrentAdjustmentFrame() / 0.03));
      else if (m_option->getMotionAdjustFrame() < 0)
         sprintf(buff, "%d msec delay (current motion: %+d)", m_option->getMotionAdjustFrame(), (int)(m_timer->getCurrentAdjustmentFrame() / 0.03));
      else
         sprintf(buff, "%d msec (current motion: %+d)", m_option->getMotionAdjustFrame(), (int)(m_timer->getCurrentAdjustmentFrame() / 0.03));
      glDisable(GL_LIGHTING);
      glColor3f(1.0f, 0.0f, 0.0f);
      glPushMatrix();
      glWindowPos2f(5.0f, 5.0f + 18.0f * 2);
      m_text->drawAsciiStringBitmap(buff);
      glPopMatrix();
      glEnable(GL_LIGHTING);
      /* show model position */
      strcpy(buff, "");
      for (i = 0; i < m_numModel; i++) {
         if (m_model[i].isEnable() == true) {
            m_model[i].getCurrentPosition(&pos);
            if(MMDAgent_strlen(buff) <= 0)
               sprintf(buff, "(%.2f, %.2f, %.2f)", pos.x(), pos.y(), pos.z());
            else
               sprintf(buff, "%s (%.2f, %.2f, %.2f)", buff, pos.x(), pos.y(), pos.z());
         }
      }
      if (MMDAgent_strlen(buff) > 0) {
         glDisable(GL_LIGHTING);
         glColor3f(1.0f, 0.0f, 0.0f);
         glPushMatrix();
         glWindowPos2f(5.0f, 5.0f);
         m_text->drawAsciiStringBitmap(buff);
         glPopMatrix();
         glEnable(GL_LIGHTING);
      }
      /* show camera parameters */
      m_render->getInfoString(buff);
      glDisable(GL_LIGHTING);
      glColor3f(1.0f, 1.0f, 0.0f);
      glPushMatrix();
      glWindowPos2f(5.0f, 5.0f + 18.0f);
      m_text->drawAsciiStringBitmap(buff);
      glPopMatrix();
      glEnable(GL_LIGHTING);
   }

   /* show model comments and error */
   for (i = 0; i < m_numModel; i++) {
      if (m_model[m_renderOrder[i]].isEnable() == true) {
         glPushMatrix();
         m_model[m_renderOrder[i]].renderComment(m_text);
         m_model[m_renderOrder[i]].renderError(m_text);
         glPopMatrix();
      }
   }

   /* execute plugin */
   m_plugin->execRender(this);

   /* swap buffer */
   m_screen->swapBuffers();
}

/* MMDAgent::drawString: draw string */
void MMDAgent::drawString(const char *str)
{
   if(m_text != NULL)
      m_text->drawString(str);
}

/* resetAdjustmentTimer: reset adjustment timer */
void MMDAgent::resetAdjustmentTimer()
{
   m_timer->setTargetAdjustmentFrame((double) m_option->getMotionAdjustFrame() * 0.03);
   m_timer->startAdjustment();
}

/* MMDAgent::sendCommandMessage: send command message */
void MMDAgent::sendCommandMessage(const char * type, const char * format, ...)
{
   va_list argv;
   char *buf1, *buf2;

   if (!m_hWnd) return;

   buf1 = MMDAgent_strdup(type);

   if (format == NULL) {
      ::PostMessage(m_hWnd, WM_MMDAGENT_COMMAND, (WPARAM) buf1, 0);
      return;
   }

   buf2 = (char *) malloc(sizeof(char) * MMDAGENT_MAXBUFLEN);

   va_start(argv, format);
   vsprintf(buf2, format, argv);
   va_end(argv);

   ::PostMessage(m_hWnd, WM_MMDAGENT_COMMAND, (WPARAM) buf1, (LPARAM) buf2);
}

/* MMDAgent::sendEventMessage: send event message */
void MMDAgent::sendEventMessage(const char * type, const char * format, ...)
{
   va_list argv;
   char *buf1, *buf2;

   if (!m_hWnd) return;

   buf1 = MMDAgent_strdup(type);

   if (format == NULL) {
      ::PostMessage(m_hWnd, WM_MMDAGENT_EVENT, (WPARAM) buf1, 0);
      return;
   }

   buf2 = (char *) malloc(sizeof(char) * MMDAGENT_MAXBUFLEN);

   va_start(argv, format);
   vsprintf(buf2, format, argv);
   va_end(argv);

   ::PostMessage(m_hWnd, WM_MMDAGENT_EVENT, (WPARAM) buf1, (LPARAM) buf2);
}

/* MMDAgent::showLogMessage: show log message */
void MMDAgent::showLogMessage(const char * format, ...)
{
   va_list argv;
   char *buf;

   if (m_hWnd == 0 || MMDAgent_strlen(format) <= 0)
      return;

   buf = (char *) malloc(sizeof(char) * MMDAGENT_MAXBUFLEN);

   va_start(argv, format);
   vsprintf(buf, format, argv);
   va_end(argv);

   ::PostMessage(m_hWnd, WM_MMDAGENT_LOG, (WPARAM) buf, 0);
}

/* MMDAgent::findModelAlias: find a model with the specified alias */
int MMDAgent::findModelAlias(const char * alias)
{
   int i;

   if(alias)
      for (i = 0; i < m_numModel; i++)
         if (m_model[i].isEnable() && MMDAgent_strequal(m_model[i].getAlias(), alias))
            return i;

   return -1;
}

/* MMDAgent::getMoelList: get model list */
PMDObject *MMDAgent::getModelList()
{
   return m_model;
}

/* MMDAgent::getNumModel: get number of models */
short MMDAgent::getNumModel()
{
   return m_numModel;
}

/* MMDAgent::getScreenPointPosition: convert screen position to object position */
void MMDAgent::getScreenPointPosition(btVector3 * dst, btVector3 * src)
{
   m_render->getScreenPointPosition(dst, src);
}

/* MMDAgent::getWindowHandler: get window handle */
HWND MMDAgent::getWindowHandler()
{
   return m_hWnd;
}

/* MMDAgent::getInstance: get instance */
HINSTANCE MMDAgent::getInstance()
{
   return m_hInst;
}

/* MMDAgent::getConfigFileName: get config file name for plugin */
char *MMDAgent::getConfigFileName()
{
   return m_configFileName;
}

/* MMDAgent::getConfigDirName: get directory of config file for plugin */
char *MMDAgent::getConfigDirName()
{
   return m_configDirName;
}

/* MMDAgent::getAppDirName: get application directory name for plugin */
char *MMDAgent::getAppDirName()
{
   return m_appDirName;
}

/* MMDAgent::procWindowDestroyMessage: process window destroy message */
void MMDAgent::procWindowDestroyMessage()
{
   if(m_plugin)
      m_plugin->execAppEnd(this);
   clear();
}

/* MMDAgent::procMouseLeftButtonDoubleClickMessage: process mouse left button double click message */
void MMDAgent::procMouseLeftButtonDoubleClickMessage(int x, int y)
{
   if (!m_hWnd) return;

   /* double click */
   m_mousepos.x = x;
   m_mousepos.y = y;
   /* store model ID */
   m_selectedModel = m_render->pickModel(m_model, m_numModel, x, y, NULL);
   /* make model highlight */
   setHighLight(m_selectedModel);
   m_doubleClicked = true;
}

/* MMDAgent::procMouseLeftButtonDownMessage: process mouse left button down message */
void MMDAgent::procMouseLeftButtonDownMessage(int x, int y, bool withCtrl, bool withShift)
{
   if (!m_hWnd) return;

   /* start hold */
   m_mousepos.x = x;
   m_mousepos.y = y;
   m_leftButtonPressed = true;
   m_doubleClicked = false;
   /* store model ID */
   m_selectedModel = m_render->pickModel(m_model, m_numModel, x, y, NULL);
   if (withCtrl == true && withShift == false) /* with Ctrl-key */
      setHighLight(m_selectedModel);
}

/* MMDAgent::procMouseLeftButtonUpMessage: process mouse left button up message */
void MMDAgent::procMouseLeftButtonUpMessage()
{
   if (!m_hWnd) return;

   /* if highlight, trun off */
   if (!m_doubleClicked)
      setHighLight(-1);
   /* end of hold */
   m_leftButtonPressed = false;
}

/* MMDAgent::procMouseWheelMessage: process mouse wheel message */
void MMDAgent::procMouseWheelMessage(bool zoomup, bool withCtrl, bool withShift)
{
   float tmp1, tmp2;

   if (!m_hWnd) return;

   /* zoom */
   tmp1 = m_option->getScaleStep();
   tmp2 = m_render->getScale();
   if (withCtrl) /* faster */
      tmp1 = (tmp1 - 1.0f) * 5.0f + 1.0f;
   else if (withShift) /* slower */
      tmp1 = (tmp1 - 1.0f) * 0.2f + 1.0f;
   if(tmp1 != 0.0) {
      if (zoomup)
         tmp2 *= tmp1;
      else
         tmp2 /= tmp1;
      m_render->setScale(tmp2);
   }
}

/* MMDAgent::procMouseMoveMessage: process mouse move message */
void MMDAgent::procMouseMoveMessage(int x, int y, bool withCtrl, bool withShift)
{
   float *f;
   float tmp1, tmp2, tmp3;
   LONG r1; /* should be renamed */
   LONG r2;
   btVector3 v;
   btMatrix3x3 bm;

   if (!m_hWnd) return;

   /* store Ctrl-key and Shift-key state for drag and drop */
   m_keyCtrl = withCtrl;
   m_keyShift = withShift;
   /* left-button is dragged in window */
   if (m_leftButtonPressed) {
      r1 = x;
      r2 = y;
      r1 -= m_mousepos.x;
      r2 -= m_mousepos.y;
      if (r1 > 32767) r1 -= 65536;
      if (r1 < -32768) r1 += 65536;
      if (r2 > 32767) r2 -= 65536;
      if (r2 < -32768) r2 += 65536;
      if (withShift && withCtrl && m_selectedModel == -1) {
         /* if Shift- and Ctrl-key, and no model is pointed, rotate light direction */
         f = m_option->getLightDirection();
         v = btVector3(f[0], f[1], f[2]);
         bm = btMatrix3x3(btQuaternion(0, r2 * 0.1f * m_option->getRotateStep(), 0) * btQuaternion(r1 * 0.1f * m_option->getRotateStep(), 0, 0));
         v = bm * v;
         changeLightDirection(v.x(), v.y(), v.z());
      } else if (withCtrl) {
         /* if Ctrl-key and model is pointed, move the model */
         if (m_selectedModel != -1) {
            setHighLight(m_selectedModel);
            m_model[m_selectedModel].getTargetPosition(&v);
            if (withShift) {
               /* with Shift-key, move on XY (coronal) plane */
               v.setX(v.x() + r1 * 0.1f * m_option->getTranslateStep() / m_render->getScale());
               v.setY(v.y() - r2 * 0.1f * m_option->getTranslateStep() / m_render->getScale());
            } else {
               /* else, move on XZ (axial) plane */
               v.setX(v.x() + r1 * 0.1f * m_option->getTranslateStep() / m_render->getScale());
               v.setZ(v.z() + r2 * 0.1f * m_option->getTranslateStep() / m_render->getScale());
            }
            m_model[m_selectedModel].setPosition(&v);
            m_model[m_selectedModel].setMoveSpeed(-1.0f);
         }
      } else if (withShift) {
         /* if Shift-key, translate display */
         tmp1 = r1 / (float) m_render->getWidth();
         tmp2 = - r2 / (float) m_render->getHeight();
         tmp3 = 20.0f;
         tmp1 = (float) (tmp1 * (tmp3 - RENDER_VIEWPOINTCAMERAZ) / RENDER_VIEWPOINTFRUSTUMNEAR);
         tmp2 = (float) (tmp2 * (tmp3 - RENDER_VIEWPOINTCAMERAZ) / RENDER_VIEWPOINTFRUSTUMNEAR);
         tmp1 /= m_render->getScale();
         tmp2 /= m_render->getScale();
         tmp3 = 0.0f;
         m_render->translate(tmp1, tmp2, tmp3);
      } else {
         /* if no key, rotate display */
         m_render->rotate(r1 * 0.1f * m_option->getRotateStep(), r2 * 0.1f * m_option->getRotateStep(), 0.0f);
      }
      m_mousepos.x = x;
      m_mousepos.y = y;
      /* forced update motion */
      updateScene();
   } else if (m_mousepos.x != x || m_mousepos.y != y) {
      /* set mouse enable timer */
      m_screen->setMouseActiveTime(45.0f);
   }
}

/* MMDAgent::procMouseRightButtonDownMessage: process mouse right button down message */
void MMDAgent::procMouseRightButtonDownMessage()
{
   if (!m_hWnd) return;

   m_screen->setMouseActiveTime(45.0f);
}

/* MMDAgent::procFullScreenMessage: process full screen message */
void MMDAgent::procFullScreenMessage()
{
   RECT rc;

   if (!m_hWnd) return;

   if (m_option->getFullScreen() == true) {
      m_screen->exitFullScreen(m_hWnd);
      m_option->setFullScreen(false);
   } else {
      m_screen->setFullScreen(m_hWnd);
      m_option->setFullScreen(true);
   }

   GetClientRect(m_hWnd, &rc);
   procWindowSizeMessage(rc.right - rc.left, rc.bottom - rc.top);
}

/* MMDAgent::procInfoStringMessage: process information string message */
void MMDAgent::procInfoStringMessage()
{
   if (!m_hWnd) return;

   if(m_option->getShowFps() == true)
      m_option->setShowFps(false);
   else
      m_option->setShowFps(true);
}

/* MMDAgent::procVSyncMessage: process vsync message */
void MMDAgent::procVSyncMessage()
{
   if (!m_hWnd) return;

   m_screen->toggleVSync();
}

/* MMDAgent::procShadowMappingMessage: process shadow mapping message */
void MMDAgent::procShadowMappingMessage()
{
   if (!m_hWnd) return;

   if(m_option->getUseShadowMapping() == true) {
      m_option->setUseShadowMapping(false);
   } else {
      m_option->setUseShadowMapping(true);
   }
   m_render->setShadowMapping(m_option->getUseShadowMapping(), m_option->getShadowMappingTextureSize(), m_option->getShadowMappingLightFirst());
}

/* MMDAgent::procShadowMappingOrderMessage: process shadow mapping order message */
void MMDAgent::procShadowMappingOrderMessage()
{
   if (!m_hWnd) return;

   if(m_option->getShadowMappingLightFirst() == true)
      m_option->setShadowMappingLightFirst(false);
   else
      m_option->setShadowMappingLightFirst(true);
}

/* MMDAgent::procDisplayRigidBodyMessage: process display rigid body message */
void MMDAgent::procDisplayRigidBodyMessage()
{
   if (!m_hWnd) return;

   m_dispBulletBodyFlag = !m_dispBulletBodyFlag;
}

/* MMDAnget::procDisplayWireMessage: process display wire message */
void MMDAgent::procDisplayWireMessage()
{
   GLint polygonMode[2];

   if (!m_hWnd) return;

   glGetIntegerv(GL_POLYGON_MODE, polygonMode);
   if (polygonMode[1] == GL_LINE)
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
   else
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

/* MMDAgent::procDisplayBoneMessage: process display bone message */
void MMDAgent::procDisplayBoneMessage()
{
   if (!m_hWnd) return;

   m_dispModelDebug = !m_dispModelDebug;
}

/* MMDAgent::procCartoonEdgeMessage: process cartoon edge message */
void MMDAgent::procCartoonEdgeMessage(bool plus)
{
   int i;

   if (!m_hWnd) return;

   if(plus)
      m_option->setCartoonEdgeWidth(m_option->getCartoonEdgeWidth() * m_option->getCartoonEdgeStep());
   else
      m_option->setCartoonEdgeWidth(m_option->getCartoonEdgeWidth() / m_option->getCartoonEdgeStep());
   for (i = 0; i < m_numModel; i++)
      m_model[i].getPMDModel()->setEdgeThin(m_option->getCartoonEdgeWidth());
}

/* MMDAgent::procTimeAdjustMessage: process time adjust message */
void MMDAgent::procTimeAdjustMessage(bool plus)
{
   if (!m_hWnd) return;

   if(plus)
      m_option->setMotionAdjustFrame(m_option->getMotionAdjustFrame() + 10);
   else
      m_option->setMotionAdjustFrame(m_option->getMotionAdjustFrame() - 10);
   m_timer->setTargetAdjustmentFrame((double) m_option->getMotionAdjustFrame() * 0.03);
}

/* MMDAgent::procHorizontalRotateMessage: process horizontal rotate message */
void MMDAgent::procHorizontalRotateMessage(bool right)
{
   if (!m_hWnd) return;

   if(right)
      m_render->rotate(m_option->getRotateStep(), 0.0f, 0.0f);
   else
      m_render->rotate(-m_option->getRotateStep(), 0.0f, 0.0f);
}

/* MMDAgent::procVerticalRotateMessage: process vertical rotate message */
void MMDAgent::procVerticalRotateMessage(bool up)
{
   if (!m_hWnd) return;

   if(up)
      m_render->rotate(0.0f, -m_option->getRotateStep(), 0.0f);
   else
      m_render->rotate(0.0f, m_option->getRotateStep(), 0.0f);
}

/* MMDAgent::procHorizontalMoveMessage: process horizontal move message */
void MMDAgent::procHorizontalMoveMessage(bool right)
{
   if (!m_hWnd) return;

   if(right)
      m_render->translate(m_option->getTranslateStep(), 0.0f, 0.0f);
   else
      m_render->translate(-m_option->getTranslateStep(), 0.0f, 0.0f);
}

/* MMDAgent::procVerticalMoveMessage: process vertical move message */
void MMDAgent::procVerticalMoveMessage(bool up)
{
   if (!m_hWnd) return;

   if(up)
      m_render->translate(0.0f, m_option->getTranslateStep(), 0.0f);
   else
      m_render->translate(0.0f, -m_option->getTranslateStep(), 0.0f);
}

/* MMDAgent::procDeleteModelMessage: process delete model message */
void MMDAgent::procDeleteModelMessage()
{
   if (!m_hWnd) return;

   if (m_doubleClicked) {
      deleteModel(m_model[m_selectedModel].getAlias());
      m_doubleClicked = false;
   }
}

/* MMDAgent::procPhysicsMessage: process physics message */
void MMDAgent::procPhysicsMessage()
{
   int i;

   if (!m_hWnd) return;

   m_enablePhysicsSimulation = !m_enablePhysicsSimulation;
   for (i = 0; i < m_numModel; i++)
      m_model[i].getPMDModel()->setPhysicsControl(m_enablePhysicsSimulation);
}

/* MMDAgent::procDisplayLogMessage: process display log message */
void MMDAgent::procDisplayLogMessage()
{
   if (!m_hWnd) return;

   m_dispLog = !m_dispLog;
}

/* MMDAgent::procWindowSizeMessage: process window size message */
void MMDAgent::procWindowSizeMessage(int x, int y)
{
   if (!m_hWnd) return;

   m_render->setSize(x, y);
}

/* MMDAgent::procKeyMessage: process key message */
void MMDAgent::procKeyMessage(char c)
{
   if (!m_hWnd) return;

   sendEventMessage(MMDAGENT_EVENT_KEY, "%C", c);
}

/* MMDAgent::procCommandMessage: process command message */
void MMDAgent::procCommandMessage(char * mes1, char * mes2)
{
   static char command[MMDAGENT_MAXBUFLEN];
   static char argv[MMDAGENT_MAXNCOMMAND][MMDAGENT_MAXBUFLEN]; /* static buffer */
   int num = 0;

   char *str1, *str2, *str3;
   bool bool1, bool2, bool3, bool4;
   float f;
   btVector3 pos;
   btQuaternion rot;
   float fvec[3];

   if (!m_hWnd) {
      if(mes1 != NULL)
         free(mes1);
      if(mes2 != NULL)
         free(mes2);
      return;
   }

   /* plugin */
   if(m_plugin)
      m_plugin->execProcCommand(this, mes1, mes2);

   /* command */
   strncpy(command, mes1, MMDAGENT_MAXBUFLEN - 1);
   command[MMDAGENT_MAXBUFLEN - 1] = '\0';

   /* divide string into arguments */
   if (MMDAgent_strlen(mes2) <= 0) {
      m_logger->log("<%s>", command);
   } else {
      m_logger->log("<%s|%s>", command, mes2);
      for (str1 = MMDAgent_strtok(mes2, "|", &str2); str1; str1 = MMDAgent_strtok(NULL, "|", &str2)) {
         if (num >= MMDAGENT_MAXNCOMMAND) {
            m_logger->log("Error: %s: number of arguments exceed the limit.", command);
            break;
         }
         strncpy(argv[num], str1, MMDAGENT_MAXBUFLEN - 1);
         argv[num][MMDAGENT_MAXBUFLEN - 1] = '\0';
         num++;
      }
   }

   if(mes1 != NULL)
      free(mes1);
   if(mes2 != NULL)
      free(mes2);

   if (MMDAgent_strequal(command, MMDAGENT_COMMAND_MODELADD)) {
      str1 = NULL;
      str2 = NULL;
      if (num < 2 || num > 6) {
         m_logger->log("Error: %s: number of arguments should be 2-6.", command);
         return;
      }
      if (num >= 3) {
         if (MMDAgent_str2pos(argv[2], &pos) == false) {
            m_logger->log("Error: %s: %s is not a position string.", command, argv[2]);
            return;
         }
      } else {
         pos = btVector3(0.0, 0.0, 0.0);
      }
      if (num >= 4) {
         if (MMDAgent_str2rot(argv[3], &rot) == false) {
            m_logger->log("Error: %s: %s is not a rotation string.", command, argv[3]);
            return;
         }
      } else {
         rot.setEulerZYX(0.0, 0.0, 0.0);
      }
      if (num >= 5) {
         str1 = argv[4];
      }
      if (num >= 6) {
         str2 = argv[5];
      }
      addModel(argv[0], argv[1], &pos, &rot, str1, str2);
   } else if (MMDAgent_strequal(command, MMDAGENT_COMMAND_MODELCHANGE)) {
      /* change model */
      if (num != 2) {
         m_logger->log("Error: %s: number of arguments should be 2.", command);
         return;
      }
      changeModel(argv[0], argv[1]);
   } else if (MMDAgent_strequal(command, MMDAGENT_COMMAND_MODELDELETE)) {
      /* delete model */
      if (num != 1) {
         m_logger->log("Error: %s: number of arguments should be 1.", command);
         return;
      }
      deleteModel(argv[0]);
   } else if (MMDAgent_strequal(command, MMDAGENT_COMMAND_MOTIONADD)) {
      /* add motion */
      bool1 = true; /* full */
      bool2 = true; /* once */
      bool3 = true; /* enableSmooth */
      bool4 = true; /* enableRePos */
      f = MOTIONMANAGER_DEFAULTPRIORITY; /* priority */
      if (num < 3 || num > 8) {
         m_logger->log("Error: %s: number of arguments should be 4-7.", command);
         return;
      }
      if (num >= 4) {
         if (MMDAgent_strequal(argv[3], "FULL")) {
            bool1 = true;
         } else if (MMDAgent_strequal(argv[3], "PART")) {
            bool1 = false;
         } else {
            m_logger->log("Error: %s: 4th argument should be \"FULL\" or \"PART\".", command);
            return;
         }
      }
      if (num >= 5) {
         if (MMDAgent_strequal(argv[4], "ONCE")) {
            bool2 = true;
         } else if (MMDAgent_strequal(argv[4], "LOOP")) {
            bool2 = false;
         } else {
            m_logger->log("Error: %s: 5th argument should be \"ONCE\" or \"LOOP\".", command);
            return;
         }
      }
      if (num >= 6) {
         if (MMDAgent_strequal(argv[5], "ON")) {
            bool3 = true;
         } else if (MMDAgent_strequal(argv[5], "OFF")) {
            bool3 = false;
         } else {
            m_logger->log("Error: %s: 6th argument should be \"ON\" or \"OFF\".", command);
            return;
         }
      }
      if (num >= 7) {
         if (MMDAgent_strequal(argv[6], "ON")) {
            bool4 = true;
         } else if (MMDAgent_strequal(argv[6], "OFF")) {
            bool4 = false;
         } else {
            m_logger->log("Error: %s: 7th argument should be \"ON\" or \"OFF\".", command);
            return;
         }
      }
      if (num >= 8) {
         f = MMDAgent_str2float(argv[7]);
      }
      addMotion(argv[0], argv[1], argv[2], bool1, bool2, bool3, bool4, f);
   } else if (MMDAgent_strequal(command, MMDAGENT_COMMAND_MOTIONCHANGE)) {
      /* change motion */
      if (num != 3) {
         m_logger->log("Error: %s: number of arguments should be 3.", command);
         return;
      }
      changeMotion(argv[0], argv[1], argv[2]);
   } else if (MMDAgent_strequal(command, MMDAGENT_COMMAND_MOTIONDELETE)) {
      /* delete motion */
      if (num != 2) {
         m_logger->log("Error: %s: number of arguments should be 2.", command);
         return;
      }
      deleteMotion(argv[0], argv[1]);
   } else if (MMDAgent_strequal(command, MMDAGENT_COMMAND_MOVESTART)) {
      /* start moving */
      bool1 = false;
      f = -1.0;
      if (num < 2 || num > 4) {
         m_logger->log("Error: %s: number of arguments should be 2-4.", command);
         return;
      }
      if (MMDAgent_str2pos(argv[1], &pos) == false) {
         m_logger->log("Error: %s: %s is not a position string.", command, argv[1]);
         return;
      }
      if (num >= 3) {
         if (MMDAgent_strequal(argv[2], "LOCAL")) {
            bool1 = true;
         } else if (MMDAgent_strequal(argv[2], "GLOBAL")) {
            bool1 = false;
         } else {
            m_logger->log("Error: %s: 3rd argument should be \"GLOBAL\" or \"LOCAL\".", command);
            return;
         }
      }
      if (num >= 4)
         f = MMDAgent_str2float(argv[3]);
      startMove(argv[0], &pos, bool1, f);
   } else if (MMDAgent_strequal(command, MMDAGENT_COMMAND_MOVESTOP)) {
      /* stop moving */
      if (num != 1) {
         m_logger->log("Error: %s: number of arguments should be 1.", command);
         return;
      }
      stopMove(argv[0]);
   } else if (MMDAgent_strequal(command, MMDAGENT_COMMAND_ROTATESTART)) {
      /* start rotation */
      bool1 = false;
      f = -1.0;
      if (num < 2 || num > 4) {
         m_logger->log("Error: %s: number of arguments should be 2-4.", command);
         return;
      }
      if (MMDAgent_str2rot(argv[1], &rot) == false) {
         m_logger->log("Error: %s: %s is not a rotation string.", command, argv[1]);
         return;
      }
      if (num >= 3) {
         if (MMDAgent_strequal(argv[2], "LOCAL")) {
            bool1 = true;
         } else if (MMDAgent_strequal(argv[2], "GLOBAL")) {
            bool1 = false;
         } else {
            m_logger->log("Error: %s: 3rd argument should be \"GLOBAL\" or \"LOCAL\".", command);
            return;
         }
      }
      if (num >= 4)
         f = MMDAgent_str2float(argv[3]);
      startRotation(argv[0], &rot, bool1, f);
   } else if (MMDAgent_strequal(command, MMDAGENT_COMMAND_ROTATESTOP)) {
      /* stop rotation */
      if (num != 1) {
         m_logger->log("Error: %s: number of arguments should be 1.", command);
         return;
      }
      stopRotation(argv[0]);
   } else if (MMDAgent_strequal(command, MMDAGENT_COMMAND_TURNSTART)) {
      /* turn start */
      bool1 = false;
      f = -1.0;
      if (num < 2 || num > 4) {
         m_logger->log("Error: %s: number of arguments should be 2-4.", command);
         return;
      }
      if (MMDAgent_str2pos(argv[1], &pos) == false) {
         m_logger->log("Error: %s: %s is not a position string.", command, argv[1]);
         return;
      }
      if (num >= 3) {
         if (MMDAgent_strequal(argv[2], "LOCAL")) {
            bool1 = true;
         } else if (MMDAgent_strequal(argv[2], "GLOBAL")) {
            bool1 = false;
         } else {
            m_logger->log("Error: %s: 3rd argument should be \"GLOBAL\" or \"LOCAL\".", command);
            return;
         }
      }
      if (num >= 4)
         f = MMDAgent_str2float(argv[3]);
      startTurn(argv[0], &pos, bool1, f);
   } else if (MMDAgent_strequal(command, MMDAGENT_COMMAND_TURNSTOP)) {
      /* stop turn */
      if (num != 1) {
         m_logger->log("Error: %s: number of arguments should be 1.", command);
         return;
      }
      stopTurn(argv[0]);
   } else if (MMDAgent_strequal(command, MMDAGENT_COMMAND_STAGE)) {
      /* change stage */
      if (num != 1) {
         m_logger->log("Error: %s: number of arguments should be 1.", command);
         return;
      }
      /* pmd or bitmap */
      str1 = MMDAgent_strtok(argv[0], ",", &str3);
      str2 = MMDAgent_strtok(NULL, ",", &str3);
      if (str2 == NULL) {
         setStage(str1);
      } else {
         setFloor(str1);
         setBackground(str2);
      }
   } else if (MMDAgent_strequal(command, MMDAGENT_COMMAND_CAMERA)) {
      /* camera */
      if(num < 3 || num > 4) {
         m_logger->log("Error: %s: number of arguments should be 3-4.", command);
         return;
      }
      changeCamera(argv[0], argv[1], argv[2], (num == 4) ? argv[3] : NULL);
   } else if (MMDAgent_strequal(command, MMDAGENT_COMMAND_LIGHTCOLOR)) {
      /* change light color */
      if (num != 1) {
         m_logger->log("Error: %s: number of arguments should be 1.", command);
         return;
      }
      if (MMDAgent_str2fvec(argv[0], fvec, 3) == false) {
         m_logger->log("Error: %s: \"%s\" is not RGB value.", command, argv[0]);
         return;
      }
      changeLightColor(fvec[0], fvec[1], fvec[2]);
   } else if (MMDAgent_strequal(command, MMDAGENT_COMMAND_LIGHTDIRECTION)) {
      /* change light direction */
      if (num != 1) {
         m_logger->log("Error: %s: number of arguments should be 1.", command);
         return;
      }
      if (MMDAgent_str2fvec(argv[0], fvec, 3) == false) {
         m_logger->log("Error: %s: \"%s\" is not XYZ value.", command, argv[0]);
         return;
      }
      changeLightDirection(fvec[0], fvec[1], fvec[2]);
   } else if (MMDAgent_strequal(command, MMDAGENT_COMMAND_LIPSYNCSTART)) {
      /* start lip sync */
      if (num != 2) {
         m_logger->log("Error: %s: number of arguments should be 2.", command);
         return;
      }
      startLipSync(argv[0], argv[1]);
   } else if (MMDAgent_strequal(command, MMDAGENT_COMMAND_LIPSYNCSTOP)) {
      /* stop lip sync */
      if (num != 1) {
         m_logger->log("Error: %s: number of arguments should be 1.", command);
         return;
      }
      stopLipSync(argv[0]);
   }
}

/* MMDAgent::procEventMessage: process event message */
void MMDAgent::procEventMessage(char * mes1, char * mes2)
{
   /* plugin */
   if(m_plugin)
      m_plugin->execProcEvent(this, mes1, mes2);

   /* free strings */
   if (m_hWnd && mes1 != NULL) {
      if (MMDAgent_strlen(mes2) > 0)
         m_logger->log("[%s|%s]", mes1, mes2);
      else
         m_logger->log("[%s]", mes1);
   }
   if (mes1 != NULL)
      free(mes1);
   if (mes2 != NULL)
      free(mes2);
}

/* MMDAgent::procLogMessage: process log message */
void MMDAgent::procLogMessage(char * mes)
{
   if (m_hWnd && MMDAgent_strlen(mes) > 0)
      m_logger->log("Log: %s", mes);
   if (mes != NULL)
      free(mes);
}

/* MMDAgent::procDropFileMessage: process file drops message */
void MMDAgent::procDropFileMessage(const char * file, int x, int y)
{
   int i;

   int dropAllowedModelID;
   int targetModelID;

   /* for motion */
   MotionPlayer *motionPlayer;

   if (!m_hWnd) return;

   if(file == NULL) return;
   sendEventMessage(MMDAGENT_EVENT_DRAGANDDROP, "%s|%d|%d", file, x, y);

   if (MMDAgent_strtailmatch(file, ".vmd") || MMDAgent_strtailmatch(file, ".VMD")) {
      dropAllowedModelID = -1;
      targetModelID = -1;
      if (m_keyCtrl) {
         /* if Ctrl-key, start motion on all models */
         targetModelID = m_option->getMaxNumModel();
      } else if (m_doubleClicked && m_selectedModel != -1 && m_model[m_selectedModel].allowMotionFileDrop()) {
         targetModelID = m_selectedModel;
      } else {
         targetModelID = m_render->pickModel(m_model, m_numModel, x, y, &dropAllowedModelID); /* model ID in curpor position */
         if (targetModelID == -1)
            targetModelID = dropAllowedModelID;
      }
      if (targetModelID == -1) {
         m_logger->log("Warning: procDropFileMessage: there is no model at the point.");
      } else {
         if (m_keyShift) { /* if Shift-key, insert motion */
            if (targetModelID == m_option->getMaxNumModel()) {
               /* all model */
               for (i = 0; i < m_numModel; i++) {
                  if (m_model[i].isEnable() && m_model[i].allowMotionFileDrop())
                     addMotion(m_model[i].getAlias(), NULL, file, false, true, true, true, MOTIONMANAGER_DEFAULTPRIORITY);
               }
            } else {
               /* target model */
               if (m_model[targetModelID].isEnable() && m_model[targetModelID].allowMotionFileDrop())
                  addMotion(m_model[targetModelID].getAlias(), NULL, file, false, true, true, true, MOTIONMANAGER_DEFAULTPRIORITY);
               else
                  m_logger->log("Warning: procDropFileMessage: there is no model at the point.");
            }
         } else {
            /* change base motion */
            if (targetModelID == m_option->getMaxNumModel()) {
               /* all model */
               for (i = 0; i < m_numModel; i++) {
                  if (m_model[i].isEnable() && m_model[i].allowMotionFileDrop()) {
                     for (motionPlayer = m_model[i].getMotionManager()->getMotionPlayerList(); motionPlayer; motionPlayer = motionPlayer->next) {
                        if (motionPlayer->active && MMDAgent_strequal(motionPlayer->name, "base")) {
                           changeMotion(m_model[i].getAlias(), "base", file); /* if 'base' motion is already used, change motion */
                           break;
                        }
                     }
                     if (!motionPlayer)
                        addMotion(m_model[i].getAlias(), "base", file, true, false, true, true, MOTIONMANAGER_DEFAULTPRIORITY);
                  }
               }
            } else {
               /* target model */
               if(m_model[targetModelID].isEnable() && m_model[targetModelID].allowMotionFileDrop()) {
                  for (motionPlayer = m_model[targetModelID].getMotionManager()->getMotionPlayerList(); motionPlayer; motionPlayer = motionPlayer->next) {
                     if (motionPlayer->active && MMDAgent_strequal(motionPlayer->name, "base")) {
                        changeMotion(m_model[targetModelID].getAlias(), "base", file); /* if 'base' motion is already used, change motion */
                        break;
                     }
                  }
                  if (!motionPlayer)
                     addMotion(m_model[targetModelID].getAlias(), "base", file, true, false, true, true, MOTIONMANAGER_DEFAULTPRIORITY);
               } else {
                  m_logger->log("Warning: procDropFileMessage: there is no model at the point.");
               }
            }
         }
      }
   } else if (MMDAgent_strtailmatch(file, ".xpmd") || MMDAgent_strtailmatch(file, ".XPMD")) {
      /* load stage */
      setStage(file);
   } else if (MMDAgent_strtailmatch(file, ".pmd") || MMDAgent_strtailmatch(file, ".PMD")) {
      /* drop model */
      if (m_keyCtrl) {
         /* if Ctrl-key, add model */
         addModel(NULL, file, NULL, NULL, NULL, NULL);
      } else {
         /* change model */
         if (m_doubleClicked && m_selectedModel != -1) /* already selected */
            targetModelID = m_selectedModel;
         else
            targetModelID = m_render->pickModel(m_model, m_numModel, x, y, &dropAllowedModelID);
         if (targetModelID == -1) {
            m_logger->log("Warning: procDropFileMessage: there is no model at the point.");
         } else {
            changeModel(m_model[targetModelID].getAlias(), file);
         }
      }
   } else if (MMDAgent_strtailmatch(file, ".bmp") || MMDAgent_strtailmatch(file, ".tga") || MMDAgent_strtailmatch(file, ".png") || MMDAgent_strtailmatch(file, ".jpg") || MMDAgent_strtailmatch(file, ".jpeg") ||
              MMDAgent_strtailmatch(file, ".BMP") || MMDAgent_strtailmatch(file, ".TGA") || MMDAgent_strtailmatch(file, ".PNG") || MMDAgent_strtailmatch(file, ".JPG") || MMDAgent_strtailmatch(file, ".JPEG")) {
      if (m_keyCtrl)
         setFloor(file); /* change floor with Ctrl-key */
      else
         setBackground(file); /* change background without Ctrl-key */
   }
}
