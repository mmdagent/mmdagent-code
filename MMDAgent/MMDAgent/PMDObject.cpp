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

#include "PMDObject.h"
#include "utils.h"
#include <mmsystem.h>

/* PMDObject::initialize: initialize PMDObject */
void PMDObject::initialize()
{
   m_isEnable = false;
   m_motionManager = NULL;

   m_moveSpeed = -1.0f;
   m_spinSpeed = -1.0f;
   m_isMoving = false;
   m_isRotating = false;
   m_underTurn = false;
   m_offsetPos = btVector3(0.0f, 0.0f, 0.0f);
   m_offsetRot = btQuaternion(0.0f, 0.0f, 0.0f, 1.0f);
   m_alphaAppearFrame = 0.0;
   m_alphaDisappearFrame = 0.0;
   m_displayCommentFrame = 0.0;
   m_assignTo = NULL;
   m_baseBone = NULL;
   m_needResetKinematic = false;
   m_alias = NULL;
}

/* PMDOjbect::clear: free PMDObject */
void PMDObject::clear()
{
   m_pmd.release();
   if (m_motionManager)
      delete m_motionManager;
   if(m_alias)
      free(m_alias);
   initialize();
}

/* PMDObject::PMDObject: constructor */
PMDObject::PMDObject()
{
   initialize();
}

/* PMDObject::PMDObject: destructor */
PMDObject::~PMDObject()
{
   clear();
}

/* PMDOjbect::release: free PMDObject */
void PMDObject::release()
{
   clear();
}

/* PMDObject::load: load model */
bool PMDObject::load(wchar_t *fileName, btVector3 *offsetPos, btQuaternion *offsetRot, bool forcedPosition, PMDBone *assignBone, PMDObject *assignObject, BulletPhysics *bullet, SystemTexture *systex, bool useCartoonRendering, float cartoonEdgeWidth, btVector3 *light)
{
   int i;

   if (fileName == NULL) return false;

   /* set physics engine before */
   m_pmd.setPhysicsEngine(bullet);

   /* apply given parameters */
   m_assignTo = assignObject;
   m_baseBone = assignBone;

   if (forcedPosition) {
      /* set offset by given parameters */
      if (offsetPos)
         m_offsetPos = (*offsetPos);
      if (offsetRot)
         m_offsetRot = (*offsetRot);
      m_pmd.getRootBone()->setOffset(&m_offsetPos);
      m_pmd.getRootBone()->update();
   } else {
      /* set offset by root bone */
      m_offsetPos = (*(m_pmd.getRootBone()->getOffset()));
   }

   /* copy absolute position flag */
   for (i = 0; i < 3; i++)
      m_absPosFlag[i] = false;

   /* copy toon rendering flag */
   m_allowToonShading = true;

   /* copy flag for motion file drop or all motion */
   if(assignBone || assignObject)
      m_allowMotionFileDrop = false;
   else
      m_allowMotionFileDrop = true;

   /* save position when position is fixed */
   if (m_baseBone) m_origBasePos = m_baseBone->getTransform()->getOrigin();
   /* set toon rendering flag */
   if (useCartoonRendering == true && m_allowToonShading == true)
      m_pmd.setToonFlag(true);
   else
      m_pmd.setToonFlag(false);

   /* set edge width */
   m_pmd.setEdgeThin(cartoonEdgeWidth);

   /* set alpha frame */
   m_alphaAppearFrame = PMDOBJECT_ALPHAFRAME;
   m_alphaDisappearFrame = 0.0;

   /* set comment frame */
   m_displayCommentFrame = PMDOBJECT_COMMENTFRAME;

   /* enable */
   m_isEnable = true;

   /* load model */
   if (m_pmd.load(fileName, systex) == false) {
      clear();
      return false;
   }

   /* set up lip sync */
   m_lipSync.setup(&m_pmd);
   /* set initial alias name as the same as model name */
   setAlias(m_pmd.getModelNameW());

   /* reset */
   setLightForToon(light);
   m_moveSpeed = -1.0f;
   m_spinSpeed = -1.0f;

   /* set temporarily all body to Kinematic */
   /* this is fixed at first simulation */
   m_needResetKinematic = true;
   m_pmd.setPhysicsControl(false);

   return true;
}

/* PMDObject::setMotion: start a motion */
bool PMDObject::startMotion(VMD * vmd, wchar_t * name, bool full, bool once, bool enableSmooth, bool enableRepos)
{
   if (m_motionManager == NULL || m_motionManager->startMotion(vmd, name, full, once, enableSmooth, enableRepos) == false)
      return false;
   /* update offset because motion may change center */
   m_offsetPos = (*(m_pmd.getRootBone()->getOffset()));
   return true;
}

/* PMDObject::swapMotion: swap a motion */
bool PMDObject::swapMotion(VMD * vmd, wchar_t * targetName)
{
   if (m_motionManager == NULL || m_motionManager->swapMotion(vmd, targetName) == false)
      return false;
   /* update offset because motion may change center */
   m_offsetPos = (*(m_pmd.getRootBone()->getOffset()));
   return true;
}

/* PMDObject::updateRootBone: update root bone if assigned to a base bone */
void PMDObject::updateRootBone()
{
   btVector3 pos;
   btVector3 posAbs;
   PMDBone *b;
   btTransform tr;

   if (!m_baseBone) return;

   /* relative position */
   pos = m_offsetPos;
   /* if absolute flag is true, fix relative position from root bone */
   posAbs = m_offsetPos + m_origBasePos - m_baseBone->getTransform()->getOrigin();
   if (m_absPosFlag[0]) pos.setX(posAbs.x());
   if (m_absPosFlag[1]) pos.setY(posAbs.y());
   if (m_absPosFlag[2]) pos.setZ(posAbs.z());

   /* set root bone */
   b = m_pmd.getRootBone();
   b->setCurrentPosition(&pos);
   b->setCurrentRotation(&m_offsetRot);
   b->update();
   /* update transform for base position */
   tr = (*m_baseBone->getTransform()) * (*b->getTransform());
   b->setTransform(&tr);
}

/* PMDObject::updateMotion: update motions */
bool PMDObject::updateMotion(double deltaFrame)
{
   bool ret;

   if (m_isEnable == false || m_motionManager == NULL) return false;

   /* set rotation and position to bone and face from motion */
   ret = m_motionManager->update(deltaFrame);
   m_pmd.updateBone(); /* update bone, IK, and rotation */
   m_pmd.updateFace(); /* update face */

   /* update comment frame */
   if (m_displayCommentFrame > 0.0f) {
      m_displayCommentFrame -= deltaFrame;
      if (m_displayCommentFrame < 0.0f)
         m_displayCommentFrame = 0.0f;
   }

   return ret;
}

/* PMDObject::updateAfterSimulation: update bone transforms from simulated rigid bodies */
void PMDObject::updateAfterSimulation(bool physicsEnabled)
{
   if (m_isEnable == false) return;

   /* if necessary, change state of Bullet Physics */
   if (m_needResetKinematic) {
      if (physicsEnabled) m_pmd.setPhysicsControl(true);
      m_needResetKinematic = false;
   }
   /* apply calculation result to bone */
   m_pmd.updateBoneFromSimulation();
   /* update skin */
   m_pmd.updateSkin();
   /* update toon */
   m_pmd.updateToon(&m_lightDir);
}

/* PMDObject::updateAlpha: update global model alpha */
bool PMDObject::updateAlpha(double deltaFrame)
{
   bool ended = false;

   if (m_alphaAppearFrame > 0.0f) {
      m_alphaAppearFrame -= deltaFrame;
      if (m_alphaAppearFrame < 0.0f)
         m_alphaAppearFrame = 0.0f;
      m_pmd.setGlobalAlpha((float)(1.0 - m_alphaAppearFrame / PMDOBJECT_ALPHAFRAME));
   }
   if (m_alphaDisappearFrame > 0.0f) {
      m_alphaDisappearFrame -= deltaFrame;
      if (m_alphaDisappearFrame <= 0.0f) {
         m_alphaDisappearFrame = 0.0f;
         ended = true; /* model was deleted */
      }
      m_pmd.setGlobalAlpha((float) (m_alphaDisappearFrame / PMDOBJECT_ALPHAFRAME));
   }
   return ended;
}

/* PMDObject::startDisppear: set disappear timer */
void PMDObject::startDisappear()
{
   m_alphaDisappearFrame = PMDOBJECT_ALPHAFRAME;
}

/* PMDObject::deleteModel: disable model */
void PMDObject::deleteModel()
{
   /* free and initialize values */
   clear();
   /* reset also root bone */
   m_pmd.getRootBone()->setOffset(&m_offsetPos);
   m_pmd.getRootBone()->update();
   m_pmd.getRootBone()->reset();
   m_pmd.getRootBone()->update();
}

/* PMDModel::setLightForToon: set light direction for ton shading */
void PMDObject::setLightForToon(btVector3 * v)
{
   m_lightDir = (*v);
   m_lightDir.normalize();
}

/* PMDObject::updateModel: update model position of root bone */
bool PMDObject::updateModelRootOffset(float fps)
{
   bool ret = false;
   PMDBone *b;
   btVector3 pos, pos2;
   float diff;
   float maxStep;

   if (m_isEnable == false) return false;

   /* get root bone */
   b = m_pmd.getRootBone();

   /* target position is m_offsetPos */
   /* move offset of root bone closer to m_offsetPos */
   pos = (*(b->getOffset()));
   m_isMoving = false;
   if (m_offsetPos != pos) {
      /* if there is difference then update */
      diff = pos.distance(m_offsetPos);
      if (diff > PMDOBJECT_MINMOVEDIFF) {
         if (m_moveSpeed >= 0.0f && fps != 0.0f) {
            /* max speed */
            maxStep = m_moveSpeed / fps;
            if (diff > maxStep) {
               pos2 = pos.lerp(m_offsetPos, maxStep / diff);
               m_isMoving = true;
            } else {
               pos2 = m_offsetPos;
               ret = true;
            }
         } else {
            /* current * 0.9 + target * 0.1 */
            pos2 = pos.lerp(m_offsetPos, 1.0f - PMDOBJECT_MOVESPEEDRATE);
            m_isMoving = true;
         }
      } else {
         /* set target offset directory if small difference */
         pos2 = m_offsetPos;
         ret = true;
      }
      m_pmd.getRootBone()->setOffset(&pos2);
      m_pmd.getRootBone()->update();
   }

   return ret;
}

/* PMDObject::updateModelRootRotation: update model rotation of root bone */
bool PMDObject::updateModelRootRotation(float fps)
{
   btQuaternion tmpRot;
   PMDBone *b;
   bool ret = false;
   btQuaternion r;
   float diff;
   float maxStep;

   if (m_isEnable == false) return false;

   m_isRotating = false;

   /* get root bone */
   b = m_pmd.getRootBone();
   /* target rotation is m_offsetRot */
   /* turn rotation of root bone closer to m_offsetRot */
   if (m_offsetRot != (*(b->getCurrentRotation()))) {
      /* difference calculation */
      r = (*(b->getCurrentRotation()));
      r = r - m_offsetRot;
      diff = r.length2();
      if (diff > PMDOBJECT_MINSPINDIFF) {
         if (m_spinSpeed >= 0.0f && fps != 0.0f) {
            /* max turn speed */
            maxStep = RAD(m_spinSpeed) / fps;
            if (diff > maxStep) {
               tmpRot = (*(b->getCurrentRotation()));
               tmpRot = tmpRot.slerp(m_offsetRot, maxStep / diff);
               b->setCurrentRotation(&tmpRot);
               m_isRotating = true;
            } else {
               b->setCurrentRotation(&m_offsetRot);
               ret = true;
            }
         } else {
            /* current * 0.95 + target * 0.05 */
            tmpRot = (*(b->getCurrentRotation()));
            tmpRot = tmpRot.slerp(m_offsetRot, 1.0f - PMDOBJECT_SPINSPEEDRATE);
            b->setCurrentRotation(&tmpRot);
            m_isRotating = true;
         }
      } else {
         /* set target offset directory if small difference */
         b->setCurrentRotation(&m_offsetRot);
         ret = true;
      }
      b->update();
   }

   return ret;
}

/* PMDObject::getAlias: get alias name */
wchar_t *PMDObject::getAlias()
{
   wchar_t *none = L"";

   if(m_alias)
      return m_alias;
   else
      return none;
}

/* PMDObject::setAlias: set alias name */
void PMDObject::setAlias(wchar_t *alias)
{
   if(alias && wcslen(alias) > 0) {
      if(m_alias)
         free(m_alias);
      m_alias = _wcsdup(alias);
   }
}

/* PMDObject::getPMDModel: get PMDModel */
PMDModel *PMDObject::getPMDModel()
{
   return &m_pmd;
}

/* PMDObject::getMotionManager: get MotionManager */
MotionManager *PMDObject::getMotionManager()
{
   return m_motionManager;
}

/* PMDObject::resetMotionManager: reset MotionManager */
void PMDObject::resetMotionManager()
{
   if (m_motionManager)
      delete m_motionManager;
   m_motionManager = new MotionManager(&m_pmd);
}

/* PMDObject::getLipSync: get LipSync */
LipSync *PMDObject::getLipSync()
{
   return &m_lipSync;;
}

/* PMDObject::getPosition: get root bone offset */
void PMDObject::getPosition(btVector3 & pos)
{
   pos = m_offsetPos;
}

/* PMDObject::setPosition: set root bone offset */
void PMDObject::setPosition(btVector3 & pos)
{
   m_offsetPos = pos;
}

/* PMDObject::getRotation: get root bone rotation */
void PMDObject::getRotation(btQuaternion & rot)
{
   rot = m_offsetRot;
}

/* PMDObject::setRotation: set root bone rotation */
void PMDObject::setRotation(btQuaternion & rot)
{
   m_offsetRot = rot;
}

/* PMDObject::setMoveSpeed: set move speed per second */
void PMDObject::setMoveSpeed(float speed)
{
   m_moveSpeed = speed;
}

/* PMDObject::setSpinSpeed: set spin seed per second */
void PMDObject::setSpinSpeed(float speed)
{
   m_spinSpeed = speed;
}

/* PMDObject::isMoving: return true when model move */
bool PMDObject::isMoving()
{
   return m_isMoving;
}

/* PMDObject::isRotating: return true when model spin */
bool PMDObject::isRotating()
{
   return m_isRotating;
}

/* PMDObject::isTruning: return true when model turn */
bool PMDObject::isTurning()
{
   return m_underTurn;
}

/* PMDObject::setTurnFlag: set turnning flag */
void PMDObject::setTurningFlag(bool flag)
{
   m_underTurn = flag;
}

/* PMDObject::isEnable: get enable flag */
bool PMDObject::isEnable()
{
   return m_isEnable;
}

/* PMDObject::setEnableFlag: set enable flag */
void PMDObject::setEnableFlag(bool flag)
{
   m_isEnable = flag;
}

/* PMDObject::allowMotionFileDrop: return true if motion file drop is allowed */
bool PMDObject::allowMotionFileDrop()
{
   return m_allowMotionFileDrop;
}

/* PMDObject::getAssignedModel: get parent model */
PMDObject *PMDObject::getAssignedModel()
{
   return m_assignTo;
}

/* PMDObject::renderCommand: render model comment */
void PMDObject::renderComment(TextRenderer * text)
{
   wchar_t buf[PMD_COMMENT_LEN];
   size_t len;
   btVector3 pos;
   float w, h;
   float tpos[3];
   wchar_t *p, *psave;

   if (m_displayCommentFrame == 0.0)
      return;

   if(m_pmd.getComment() == NULL)
      return;

   mbstowcs_s(&len, buf, PMD_COMMENT_LEN, m_pmd.getComment(), _TRUNCATE);

   pos = m_pmd.getCenterBone()->getTransform()->getOrigin();
   w = 13.0f;
   h = 5.0f;
   pos.setX(pos.x() - w * 0.5f);
   pos.setZ(pos.z() + 5.2f);
   glDisable(GL_LIGHTING);
   glPushMatrix();
   glTranslatef(pos.x() - 0.3f, pos.y() - 0.3f, pos.z() - 0.01f);
   glNormal3f(0.0, 0.0, 1.0);
   glColor4f(0.0f, 0.0f, 0.0f, 0.4f);
   glBegin(GL_QUADS);
   glVertex3f(0, 0, 0);
   glVertex3f(w, 0, 0);
   glVertex3f(w, h, 0);
   glVertex3f(0, h, 0);
   glEnd();
   glPopMatrix();
   glColor4f(0.7f, 0.8f, 0.5f, 1.0f);
   tpos[0] = pos.x();
   tpos[1] = pos.y() + 4.5f;
   tpos[2] = pos.z();
   for (p = wcstok_s(buf, L"\n", &psave); p; p = wcstok_s(NULL, L"\n", &psave)) {
      tpos[1] -= 0.65f;
      glPushMatrix();
      glTranslatef(tpos[0], tpos[1], tpos[2]);
      glScalef(0.8f, 0.8f, 0.8f);
      text->drawString(p);
      glPopMatrix();
   }
   glEnable(GL_LIGHTING);
}

/* PMDObject::renderDebug: render model debug */
void PMDObject::renderDebug(TextRenderer * text)
{
   btVector3 pos;

   /* render debug */
   m_pmd.renderDebug();
   /* render model name */
   glDisable(GL_LIGHTING);
   glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
   glPushMatrix();
   pos = m_pmd.getCenterBone()->getTransform()->getOrigin();
   pos.setY(m_pmd.getMaxHeight() + 1.0f);
   glTranslatef(pos.x(), pos.y(), pos.z());
   text->drawString(m_pmd.getModelNameW());
   glEnable(GL_LIGHTING);
   glPopMatrix();
}

/* PMDObject::renderError: render model error */
void PMDObject::renderError(TextRenderer * text)
{
   char buf[256];
   btVector3 pos;
   float w, h;
   float tpos[3];
   char *p, *psave;

   m_pmd.getErrorTextureList(buf, 256);
   if (strlen(buf) <= 0)
      return;

   pos = m_pmd.getCenterBone()->getTransform()->getOrigin();
   pos.setZ(pos.z() + 5.0f);
   glDisable(GL_LIGHTING);
   glPushMatrix();
   glTranslatef(pos.x() - 0.3f, pos.y() - 0.3f, pos.z() - 0.01f);
   w = 10.0f;
   h = 6.0f;
   glNormal3f(0.0, 0.0, 1.0);
   glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
   glBegin(GL_QUADS);
   glVertex3f(0, 0, 0);
   glVertex3f(w, 0, 0);
   glVertex3f(w, h, 0);
   glVertex3f(0, h, 0);
   glEnd();
   glPopMatrix();

   glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
   tpos[0] = pos.x();
   tpos[1] = pos.y() + 5.0f;
   tpos[2] = pos.z();
   glPushMatrix();
   glTranslatef(tpos[0], tpos[1], tpos[2]);
   text->drawAsciiStringOutline("[Texture Errors]");
   glPopMatrix();

   for (p = strtok_s(buf, "\n", &psave); p; p = strtok_s(NULL, "\n", &psave)) {
      tpos[1] -= 0.7f;
      glPushMatrix();
      glTranslatef(tpos[0], tpos[1], tpos[2]);
      text->drawAsciiStringOutline(p);
      glPopMatrix();
   }
   glEnable(GL_LIGHTING);
}
