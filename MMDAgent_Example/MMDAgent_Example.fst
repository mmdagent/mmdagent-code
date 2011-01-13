# ----------------------------------------------------------------- #
#           MMDAgent Scenario Example                               #
#           released by MMDAgent Project Team                       #
#           http://www.mmdagent.jp/                                 #
# ----------------------------------------------------------------- #
#                                                                   #
#  Copyright (c) 2009-2010  Nagoya Institute of Technology          #
#                           Department of Computer Science          #
#                                                                   #
# Some rights reserved.                                             #
#                                                                   #
# This work is licensed under the Creative Commons Attribution 3.0  #
# license.                                                          #
#                                                                   #
# You are free:                                                     #
#  * to Share - to copy, distribute and transmit the work           #
#  * to Remix - to adapt the work                                   #
# Under the following conditions:                                   #
#  * Attribution - You must attribute the work in the manner        #
#    specified by the author or licensor (but not in any way that   #
#    suggests that they endorse you or your use of the work).       #
# With the understanding that:                                      #
#  * Waiver - Any of the above conditions can be waived if you get  #
#    permission from the copyright holder.                          #
#  * Public Domain - Where the work or any of its elements is in    #
#    the public domain under applicable law, that status is in no   #
#    way affected by the license.                                   #
#  * Other Rights - In no way are any of the following rights       #
#    affected by the license:                                       #
#     - Your fair dealing or fair use rights, or other applicable   #
#       copyright exceptions and limitations;                       #
#     - The author's moral rights;                                  #
#     - Rights other persons may have either in the work itself or  #
#       in how the work is used, such as publicity or privacy       #
#       rights.                                                     #
#  * Notice - For any reuse or distribution, you must make clear to #
#    others the license terms of this work. The best way to do this #
#    is with a link to this web page.                               #
#                                                                   #
# See http://creativecommons.org/ for details.                      #
# ----------------------------------------------------------------- #

# �}��
# �E"_EVENT_" ���t���Ȃ����̂́A�R�}���h�BFST�̑�4�t�B�[���h�Ŏw�肷��B
# �E"_EVENT_" ���t�����̂́A�C�x���g�ʒm�BFST�̑�3�t�B�[���h�Ŏw�肷��B
#
# ���f��
# MODEL_ADD|���f���G�C���A�X|���f���t�@�C��(.pmd)|���΍��W|���Ή�]|����f���G�C���A�X|bone
# MODEL_CHANGE|���f���G�C���A�X|���f���t�@�C��(.pmd)
# MODEL_DELETE|���f���G�C���A�X
# MODEL_EVENT_ADD|���f���G�C���A�X
# MODEL_EVENT_CHANGE|���f���G�C���A�X
# MODEL_EVENT_DELETE|���f���G�C���A�X
#
# �E�u����f���G�C���A�X�v�ubone�v�́A�w�胂�f���̎w��{�[���ɑ΂��Ă��̃��f��������t���\���i���Ε\���j����B�A�N�Z�T�����ɗ��p�ł���B
# �E�u���΍��W�v�u���Ή�]�v�́A����f���G�C���A�X�����bone���w�肵���ꍇ�͂��̃{�[������̑��Ύw��A
# �@�w�薳���̏ꍇ�̓��[���h���W�̌��_�ɑ΂����Ύw��ƂȂ�B
# �EMODEL_ADD�́u���΍��W�v�ȍ~���ȗ��\�B�ȗ������ꍇ�A���΍��W(0,0,0)�A���Ή�]�����A����f���G�C���A�X�����ɂȂ�B
#
# ���[�V����
# MOTION_ADD|���f���G�C���A�X|���[�V�����G�C���A�X|���[�V�����t�@�C��(.vmd)|FULL or PART|ONCE or LOOP|ON or OFF|ON or OFF
# MOTION_CHANGE|���f���G�C���A�X|���[�V�����G�C���A�X|���[�V�����t�@�C��(.vmd)
# MOTION_DELETE|���f���G�C���A�X|���[�V�����G�C���A�X
# MOTION_EVENT_ADD|���f���G�C���A�X|���[�V�����G�C���A�X
# MOTION_EVENT_CHANGE|���f���G�C���A�X|���[�V�����G�C���A�X
# MOTION_EVENT_DELETE|���f���G�C���A�X|���[�V�����G�C���A�X
#
# �E���[�V�����͔C�Ӄ^�C�~���O�ŏd�ˍ��킹�ł���B�d�˂�ꂽ���[�V�����͕��s��������A��ɒǉ����ꂽ���̂��D�悳���B
# �E�uFULL�v���uPART�v�ɂ���ƁA�L�[�t���[�����w�肳��Ă���{�[���̂݁A���̃��[�V�����Ő��䂷��B
# �@���[�V������0�t���[���ځi�����p���j�����w�肳��Ă��Ȃ��{�[���͖�������A���̃��[�V���������߂���B
# �@������g���āA�Ⴆ�΁A�S�̂̃��[�V������ۂ����܂ܘr������ʂ̃��[�V�����Ő��䂷��A�Ƃ��������Ƃ��ł���B
# �E�uONCE�v��1�񂫂�ŁA�I�[�ɒB����Ǝ����I�� DELETE �����B�uLOOP�v�ɂ���Ɩ������[�v����B
# �E��ڂ́uON or OFF�v�́A���[�V�����J�n���E�I�����ɃX���[�W���O�i�u�����f�B���O�j���邩�̐ؑցB�f�t�H���g�� ON
# �E��ڂ́uON or OFF�v�́A�X���[�W���O���Ƀ��f���̐e���W�����ւ��邩�ǂ����̐ؑցB�f�t�H���g�� ON
# �EMOTION_ADD�ł́uFULL or PART�v�ȍ~�̈������ȗ��\�B�ȗ������ꍇ�A"FULL|ONCE|ON|ON" �ƂȂ�B
#
# �ړ��E��]
# MOVE_START|���f���G�C���A�X|x���W,y���W,z���W|GLOBAL or LOCAL|�ړ����x
# MOVE_STOP|���f���G�C���A�X
# MOVE_EVENT_START|���f���G�C���A�X
# MOVE_EVENT_STOP|���f���G�C���A�X
# TURN_START|���f���G�C���A�X|x���W,y���W,z���W|GLOBAL or LOCAL|��]���x
# TURN_STOP|���f���G�C���A�X
# TURN_EVENT_START|���f���G�C���A�X
# TURN_EVENT_STOP|���f���G�C���A�X
# ROTATE_START|���f���G�C���A�X|x����]�p�Cy����]�p�Cy����]�p|GLOBAL or LOCAL|��]���x
# ROTATE_STOP|���f���G�C���A�X
# ROTATE_EVENT_START|���f���G�C���A�X
# ROTATE_EVENT_STOP|���f���G�C���A�X
#
# ���y�E�摜
# SOUND_START|�T�E���h�G�C���A�X|���y�t�@�C��(.wav|.mp3)
# SOUND_STOP|�T�E���h�G�C���A�X
# SOUND_EVENT_START|�T�E���h�G�C���A�X
# SOUND_EVENT_STOP|�T�E���h�G�C���A�X
# STAGE|�X�e�[�W�t�@�C��(.xpmd)
# STAGE|���摜�t�@�C��(.bmp, .png, .tga),�w�i�摜�t�@�C��(.bmp, .png, .tga)
#
# �J�����E�Ɩ�
# LIGHTCOLOR|R,G,B
# LIGHTDIRECTION|x���W,y���W,z���W
#
# �����F��
# RECOG_EVENT_START
# RECOG_EVENT_STOP|�P���
#
# ��������
# SYNTH_START|���f���G�C���A�X|�{�C�X|�e�L�X�g
# SYNTH_STOP|���f���G�C���A�X
# SYNTH_EVENT_START|���f���G�C���A�X
# SYNTH_EVENT_STOP|���f���G�C���A�X
# LIPSYNC_START|���f���G�C���A�X|���ԏ��t�����f��
# LIPSYNC_STOP|���f���G�C���A�X
# LIPSYNC_EVENT_START|���f���G�C���A�X
# LIPSYNC_EVENT_STOP|���f���G�C���A�X
#
# ���̑�
# KEY|�L�[
# TIMER_START|�J�E���g�_�E���G�C���A�X|�l
# TIMER_STOP|�J�E���g�_�E���G�C���A�X
# TIMER_EVENT_START|�J�E���g�_�E���G�C���A�X
# TIMER_EVENT_STOP|�J�E���g�_�E���G�C���A�X

# 0000-0010 Initialize

0   1   <eps>             MODEL_ADD|���C|Model\mei\mei.pmd|0.0,0.0,-14.0
1   3   <eps>             MODEL_ADD|���j���[|Accessory\menu\menu.pmd|0.0,-4.5,0.0|0.0,0.0,0.0|���C
3   4   <eps>             MOTION_ADD|���j���[|��]|Motion\menu_rotation\menu_rotation.vmd|FULL|LOOP|OFF
4   5   <eps>             STAGE|Stage\building2\floor.bmp,Stage\building2\background.bmp
5   2   <eps>             MOTION_ADD|���C|base|Motion\mei_wait\mei_wait.vmd|FULL|LOOP
2   2   RECOG_EVENT_START MOTION_ADD|���C|����|Expression\mei_listen\mei_listen.vmd|PART|ONCE

# 0011-0020 Hello (key 1)

2   11  RECOG_EVENT_STOP|����ɂ��� SYNTH_START|���C|���C�i���j|����ɂ��́B
2   11  RECOG_EVENT_STOP|����ɂ��� SYNTH_START|���C|���C�i���j|����ɂ��́B
2   11  KEY|1                       SYNTH_START|���C|���C�i���j|����ɂ��́B
11  12  <eps>                       MOTION_ADD|���C|���A|Motion\mei_greeting\mei_greeting.vmd|PART
12  2   SYNTH_EVENT_STOP|���C       <eps>

# 0021-0030 Self introduction (key 2)

2   21  RECOG_EVENT_STOP|���ȏЉ�  SYNTH_START|���C|���C�i���j|���C�ƌ����܂��B
2   21  RECOG_EVENT_STOP|���Ȃ�,�N SYNTH_START|���C|���C�i���j|���C�ƌ����܂��B
2   21  RECOG_EVENT_STOP|�N,�N     SYNTH_START|���C|���C�i���j|���C�ƌ����܂��B
2   21  KEY|2                      SYNTH_START|���C|���C�i���j|���C�ƌ����܂��B
21  22  <eps>                      MOTION_ADD|���C|���A|Motion\mei_self_introduction\mei_self_introduction.vmd|PART
22  23  SYNTH_EVENT_STOP|���C      SYNTH_START|���C|���C�i���j|��낵�����肢���܂��B
23  2   SYNTH_EVENT_STOP|���C      <eps>

# 0031-0040 Thank you (key 3)

2   31  RECOG_EVENT_STOP|���肪��   SYNTH_START|���C|���C�i���j|�ǂ��������܂��āB
2   31  RECOG_EVENT_STOP|���肪�Ƃ� SYNTH_START|���C|���C�i���j|�ǂ��������܂��āB
2   31  RECOG_EVENT_STOP|�L�     SYNTH_START|���C|���C�i���j|�ǂ��������܂��āB
2   31  RECOG_EVENT_STOP|�L��   SYNTH_START|���C|���C�i���j|�ǂ��������܂��āB
2   31  KEY|3                       SYNTH_START|���C|���C�i���j|�ǂ��������܂��āB
31  32  <eps>                       MOTION_ADD|���C|�\��|Expression\mei_happiness\mei_happiness.vmd|PART
32  33  SYNTH_EVENT_STOP|���C       SYNTH_START|���C|���C�i��j|���ł��A�b�������Ă��������ˁB
33  34  <eps>                       MOTION_CHANGE|���C|base|Motion\mei_guide\mei_guide_happy.vmd
34  2   SYNTH_EVENT_STOP|���C       MOTION_CHANGE|���C|base|Motion\mei_wait\mei_wait.vmd

# 0041-0050 Positive comments (key 4)

2   41  RECOG_EVENT_STOP|����   SYNTH_START|���C|���C�i�Ɓj|�p���������ł��B
2   41  RECOG_EVENT_STOP|���킢�� SYNTH_START|���C|���C�i�Ɓj|�p���������ł��B
2   41  RECOG_EVENT_STOP|�Y��     SYNTH_START|���C|���C�i�Ɓj|�p���������ł��B
2   41  RECOG_EVENT_STOP|���ꂢ   SYNTH_START|���C|���C�i�Ɓj|�p���������ł��B
2   41  KEY|4                     SYNTH_START|���C|���C�i�Ɓj|�p���������ł��B
41  42  <eps>                     MOTION_ADD|���C|�\��|Expression\mei_bashfulness\mei_bashfulness.vmd|PART
42  2   SYNTH_EVENT_STOP|���C     <eps>

# 0051-0070 Information (key 5)

2   51  RECOG_EVENT_STOP|�}���� MODEL_DELETE|���j���[
2   51  KEY|5                   MODEL_DELETE|���j���[
51  52  <eps>                   MODEL_ADD|�p�l��|Accessory\map\map_library.pmd|0.0,2.8,2.5|0.0,0.0,0.0|���C
52  53  <eps>                   MOTION_ADD|���C|�ē�|Motion\mei_panel\mei_panel_on.vmd|PART|ONCE
53  54  <eps>                   MOTION_CHANGE|���C|base|Motion\mei_guide\mei_guide_normal.vmd
54  55  <eps>                   SYNTH_START|���C|���C�i���j|�}���ق́C���ʂ��猩��ƁC�E�O�̕����ɂ���܂��D
55  56  SYNTH_EVENT_STOP|���C   MOTION_ADD|���C|����|Motion\mei_look\mei_look_down.vmd|PART|ONCE
56  57  <eps>                   SYNTH_START|���C|���C�i���j|�L�����p�X�}�b�v�ł́C������ɂȂ�܂��D
57  58  <eps>                   MOTION_ADD|���C|�ē�|Motion\mei_point\mei_point_center_buttom.vmd|PART|ONCE
58  59  SYNTH_EVENT_STOP|���C   MOTION_CHANGE|���C|base|Motion\mei_guide\mei_guide_happy.vmd
59  60  <eps>                   SYNTH_START|���C|���C�i���j|������ɂȂ�܂����H
60  61  SYNTH_EVENT_STOP|���C   MODEL_DELETE|�p�l��
61  62  <eps>                   MODEL_ADD|���j���[|Accessory\menu\menu.pmd|0.0,-4.5,0.0|0.0,0.0,0.0|���C
62  63  <eps>                   MOTION_CHANGE|���C|base|Motion\mei_wait\mei_wait.vmd
63  2   <eps>                   MOTION_ADD|���j���[|��]|Motion\menu_rotation\menu_rotation.vmd|FULL|LOOP|OFF
