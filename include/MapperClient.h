/**
* This file is part of ORB-SLAM2.
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

#ifndef MAPPERCLIENT_H
#define MAPPERCLIENT_H

#include "Map.h"
#include "KeyFrame.h"
#include "KeyFrameDatabase.h"
#include "LocalMapping.h"
#include "LoopClosing.h"
#include "Enums.h"
#include "MapObserver.h"
#include "Mapper.h"
#include "MapperServer.h"

namespace ORB_SLAM2
{

   class LocalMapping;
   class LoopClosing;

   // interface for all Mapping functionality
   class MapperClient : public Mapper, protected SyncPrint
   {
   public:

      MapperClient(Map * pMap, ORBVocabulary* pVocab, const bool bMonocular);

      virtual long unsigned  KeyFramesInMap();

      virtual void Reset();

      virtual std::vector<KeyFrame*> DetectRelocalizationCandidates(Frame* F);

      virtual bool GetPauseRequested();

      virtual bool AcceptKeyFrames();

      virtual bool InsertKeyFrame(unsigned int trackerId, vector<MapPoint*> & mapPoints, KeyFrame *pKF);

      virtual void Initialize(unsigned int trackerId, vector<MapPoint*> & mapPoints, vector<KeyFrame*> & keyframes);

      virtual bool GetInitialized();

      virtual Map * GetMap();

      virtual unsigned int LoginTracker(
         unsigned long  & firstKeyFrameId,
         unsigned int & keyFrameIdSpan,
         unsigned long & firstMapPointId,
         unsigned int & mapPointIdSpan,
         const cv::Mat & pivotCalib
      );

      virtual void LogoutTracker(unsigned int id);

      virtual void UpdatePose(unsigned int trackerId, const cv::Mat & poseTcw);

      virtual vector<cv::Mat> GetTrackerPoses();

      virtual vector<cv::Mat> GetTrackerPivots();

   private:
      static const unsigned int MAX_TRACKERS = 2;

      static const unsigned int KEYFRAME_ID_SPAN = MAX_TRACKERS;

      /*
      The Local Mapper does not create KeyFrames, but it does create MapPoints. This is why the
      MAPPOINT_ID_SPAN is one more than the KEYFRAME_ID_SPAN. This set of MapPoint Ids is reserved
      for the Local Mapper.
      */
      static const unsigned int MAPPOINT_ID_SPAN = MAX_TRACKERS + 1;

      static const unsigned long FIRST_MAPPOINT_ID_LOCALMAPPER = MAX_TRACKERS;

      struct TrackerStatus {
         bool connected;
         unsigned long nextKeyFrameId;
         unsigned long nextMapPointId;
      };

      TrackerStatus mTrackers[MAX_TRACKERS];

      std::mutex mMutexLogin;

      ORBVocabulary * mpVocab;

      bool mbMonocular;

      Map * mpMap;

      bool mInitialized;

      MapperServer server;

      void MapperServerObserverReset();

      void MapperServerObserverMapChanged(MapChangeEvent & mce);

      class MapperServerObserver : public MapObserver
      {
         MapperClient * mpMapperClient;
      public:
         MapperServerObserver(MapperClient * pMapperClient) : mpMapperClient(pMapperClient) {};
         virtual void HandleReset() { mpMapperClient->MapperServerObserverReset(); };
         virtual void HandleMapChanged(MapChangeEvent & mce) { mpMapperClient->MapperServerObserverMapChanged(mce); }
      };

      MapperServerObserver mMapperServerObserver;
   };

}

#endif // MAPPERCLIENT_H