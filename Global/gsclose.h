/* $Id$ */

#ifndef __GSCLOSE_H__
#define __GSCLOSE_H__

#include "IPAsupp.h"
#include <stdio.h>

// Constants

// Worm algorithm flags
#define TRACK_USE_MAXIMUM           0x0001 // ��������� �� ���ᨬ���� ���祭��
#define TRACK_USE_MINIMUM           0x0000 // ��������� �� ��������� ���祭��
#define TRACK_REACH_END_POINT       0x0002 // ��������� �� ���⨦���� ��������
                                           // ����筮� �窨
#define TRACK_CLOSE_CONTOUR         0x0000 // ���������, ���� �� ��������� ������
#define TRACK_CLOSE_ON_FIRST        0x0004 // ����뢠�� ⮫쪮 �� ���⨦���� ���⮢��
                                           // �窨
#define TRACK_CLOSE_ON_ANY          0x0000 // ����뢠�� �� ���⨦���� �� �窨 ������
#define TRACK_SLOPPY_DIRECTIONS     0x0008 // �ᯮ������� ���� ���ࠢ����� �� 9
#define TRACK_STRICT_DIRECTIONS     0x0000 // �ᯮ������� �� ���ࠢ����� �� 9
#define TRACK_NO_CIRCLES            0x0010 // "��१���" �����

extern PImage gs_close_edges(
                      PImage edges,
                      PImage gradient,
                      int maxlen,      // ���ᨬ��쭮 �����⨬�� ����� ����� ᮧ������� ���⪠ �p�����
                      int minedgelen,  // �������쭠� ����� "�������" �࠭���
                      int mingradient  // �������쭮� ���祭�� �p������, ���p�� �㤥� ���뢠����
                     );
extern PImage gs_track(
                       PImage img,
                       int startpos,
                       int endpos,
                       int treshold,
                       unsigned long flags
                      );

#endif /* __GSCLOSE_H__ */

