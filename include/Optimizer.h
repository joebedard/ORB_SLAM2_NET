/**
* This file is part of ORB-SLAM2-NET.
*
* Copyright (C) 2014-2016 Raúl Mur-Artal <raulmur at unizar dot es> (University of Zaragoza)
* For more information see <https://github.com/raulmur/ORB_SLAM2>
* Copyright (C) 2018 Joe Bedard <mr dot joe dot bedard at gmail dot com>
* For more information see <https://github.com/joebedard/ORB_SLAM2_NET>
*
* ORB-SLAM2-NET is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* ORB-SLAM2-NET is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with ORB-SLAM2-NET. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "Map.h"
#include "MapPoint.h"
#include "KeyFrame.h"
#include "LoopClosing.h"
#include "Frame.h"

#include "Thirdparty/g2o/g2o/types/types_seven_dof_expmap.h"

namespace ORB_SLAM2
{

   class LoopClosing;

   class Optimizer
   {
   public:

      void static BundleAdjustment(
         const std::vector<KeyFrame*> & vpKF, 
         const std::vector<MapPoint*> & vpMP,
         int nIterations = 5, 
         bool * pbStopFlag = NULL, 
         const unsigned long nLoopKF = 0,
         const bool bRobust = true);

      void static GlobalBundleAdjustment(
         Map & map,
         int nIterations = 5,
         bool * pbStopFlag = NULL,
         const unsigned long nLoopKF = 0, 
         const bool bRobust = true);

      void static LocalBundleAdjustment(
         KeyFrame * pKF, 
         bool * pbStopFlag, 
         Map & map,
         std::mutex & mutexMapUpdate, 
         MapChangeEvent & mapChanges);

      int static PoseOptimization(Frame* pFrame);

      // if bFixScale is true, 6DoF optimization (stereo,rgbd), 7DoF otherwise (mono)
      void static OptimizeEssentialGraph(
         Map & map, 
         std::mutex & mutexMapUpdate, 
         KeyFrame * pLoopKF, 
         KeyFrame * pCurKF,
         const LoopClosing::KeyFrameAndPose & NonCorrectedSim3,
         const LoopClosing::KeyFrameAndPose & CorrectedSim3,
         const std::map<KeyFrame *, set<KeyFrame *> > & LoopConnections,
         const bool & bFixScale);

      // if bFixScale is true, optimize SE3 (stereo,rgbd), Sim3 otherwise (mono)
      static int OptimizeSim3(
         KeyFrame * pKF1,
         KeyFrame * pKF2,
         std::vector<MapPoint *> & vpMatches1,
         g2o::Sim3 & g2oS12, 
         const float th2, 
         const bool bFixScale);

   private:

      static void Print(const char * message);

   };

} //namespace ORB_SLAM

#endif // OPTIMIZER_H
