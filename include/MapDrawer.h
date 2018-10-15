/**
* This file is part of ORB-SLAM2.
*
* Copyright (C) 2014-2016 Raúl Mur-Artal <raulmur at unizar dot es> (University of Zaragoza)
* For more information see <https://github.com/raulmur/ORB_SLAM2>
*
* ORB-SLAM2 is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* ORB-SLAM2 is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with ORB-SLAM2. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MAPDRAWER_H
#define MAPDRAWER_H

#include "Map.h"
#include "MapperServer.h"
#include "MapPoint.h"
#include "KeyFrame.h"
#include "SyncPrint.h"
#include<pangolin/pangolin.h>

#include<mutex>

namespace ORB_SLAM2
{

   class MapDrawer : SyncPrint
   {
   public:
      MapDrawer(MapperServer * pMapper, cv::FileStorage & fSettings);
      void Reset();

      void Follow(pangolin::OpenGlRenderState * pRenderState);
      void DrawMapPoints();
      void DrawKeyFrames(const bool bDrawKF, const bool bDrawGraph);
      void DrawCurrentCameras();
      void SetCurrentCameraPose(const cv::Mat &Tcw);
      void SetReferenceMapPoints(const std::vector<MapPoint*>& vpMPs);

      float GetViewpointX();
      float GetViewpointY();
      float GetViewpointZ();
      float GetViewpointF();

   private:
      Map * mpMap;

      MapperServer * mpMapper;

      float mKeyFrameSize;

      float mKeyFrameLineWidth;

      float mGraphLineWidth;

      float mPointSize;

      float mCameraSize;

      float mCameraLineWidth;

      cv::Mat mCameraPose;

      std::mutex mMutexCamera;

      mutex mMutexReferenceMapPoints;

      std::vector<MapPoint*> mvpReferenceMapPoints;

      float mViewpointX, mViewpointY, mViewpointZ, mViewpointF;

      void ConvertMatrixFromOpenCvToOpenGL(pangolin::OpenGlMatrix &M, cv::Mat cameraPose);

   };

} //namespace ORB_SLAM

#endif // MAPDRAWER_H
