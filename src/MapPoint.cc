/**
* This file is part of ORB-SLAM2-TEAM.
*
* Copyright (C) 2014-2016 Raúl Mur-Artal <raulmur at unizar dot es> (University of Zaragoza)
* For more information see <https://github.com/raulmur/ORB_SLAM2>
* Copyright (C) 2018 Joe Bedard <mr dot joe dot bedard at gmail dot com>
* For more information see <https://github.com/joebedard/ORB_SLAM2_TEAM>
*
* ORB-SLAM2-TEAM is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* ORB-SLAM2-TEAM is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with ORB-SLAM2-TEAM. If not, see <http://www.gnu.org/licenses/>.
*/

#include "MapPoint.h"
#include "ORBmatcher.h"
#include "Serializer.h"

#include<mutex>

namespace ORB_SLAM2_TEAM
{

   mutex MapPoint::mGlobalMutex;

   MapPoint::MapPoint(id_type id)
      : SyncPrint("MapPoint: ")
      , mnId(id)
      , mnFirstKFid(-1)
      , mnObs(0)
      , mnTrackReferenceForFrame(0)
      , mnLastFrameSeen(0)
      , mnBALocalForKF(0)
      , mnFuseCandidateForKF(0)
      , mnLoopPointForKF(0)
      , mnCorrectedByKF(0)
      , mnCorrectedReference(0)
      , mnBAGlobalForKF(0)
      , mpRefKF(static_cast<KeyFrame*>(NULL))
      , mnVisible(1)
      , mnFound(1)
      , mbBad(false)
      , mpReplaced(static_cast<MapPoint*>(NULL))
      , mfMinDistance(0)
      , mfMaxDistance(0)

      // public read-only access to private variables
      , id(mnId)
      , firstKFid(mnFirstKFid)
   {
   }

   MapPoint::MapPoint(id_type id, const cv::Mat & worldPos, KeyFrame *pRefKF) 
      : SyncPrint("MapPoint: ")
      , mnId(id)
      , mnFirstKFid(pRefKF->id)
      , mnObs(0)
      , mnTrackReferenceForFrame(0)
      , mnLastFrameSeen(0)
      , mnBALocalForKF(0)
      , mnFuseCandidateForKF(0)
      , mnLoopPointForKF(0)
      , mnCorrectedByKF(0)
      , mnCorrectedReference(0)
      , mnBAGlobalForKF(0)
      , mpRefKF(pRefKF)
      , mnVisible(1)
      , mnFound(1)
      , mbBad(false)
      , mpReplaced(static_cast<MapPoint*>(NULL))
      , mfMinDistance(0)
      , mfMaxDistance(0)
      , mNormalVector(cv::Mat::zeros(3, 1, CV_32F))
      , mWorldPos(worldPos)
   
      // public read-only access to private variables
      , id(mnId)
      , firstKFid(mnFirstKFid)
   {
      if (worldPos.empty())
         throw exception("MapPoint::SetWorldPos([])!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
   }

   void MapPoint::SetWorldPos(const cv::Mat &Pos)
   {
      unique_lock<mutex> lock2(mGlobalMutex);
      unique_lock<mutex> lock(mMutexPos);
      if (Pos.empty())
         throw exception("MapPoint::SetWorldPos([])!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
      Pos.copyTo(mWorldPos);
      mModified = true;
   }

   cv::Mat MapPoint::GetWorldPos()
   {
      unique_lock<mutex> lock(mMutexPos);
      return mWorldPos.clone();
   }

   cv::Mat MapPoint::GetNormal()
   {
      unique_lock<mutex> lock(mMutexPos);
      return mNormalVector.clone();
   }

   KeyFrame* MapPoint::GetReferenceKeyFrame()
   {
      unique_lock<mutex> lock(mMutexFeatures);
      return mpRefKF;
   }

   map<KeyFrame*, size_t> MapPoint::GetObservations()
   {
      unique_lock<mutex> lock(mMutexFeatures);
      return mObservations;
   }

   size_t MapPoint::Observations()
   {
      unique_lock<mutex> lock(mMutexFeatures);
      return mnObs;
   }

   MapPoint * MapPoint::GetReplaced()
   {
      unique_lock<mutex> lock(mMutexFeatures);
      return mpReplaced;
   }

   MapPoint * MapPoint::FindFinalReplacement(MapPoint * pMP)
   {
      //Print("MapPoint: ", "begin FindFinalReplacement");
      if (!pMP)
         throw exception("MapPoint::FindFinalReplacement pMP == NULL");

      set<MapPoint *> alreadySeen;

      // search through replacement points until it finds the last one
      MapPoint * pNext = pMP->GetReplaced();
      while (pNext)
      {
         if (alreadySeen.count(pNext) > 0) {
            string m = string("detected a loop in replacement MapPoints pNext->id==") + to_string(pNext->id);
            throw exception(m.c_str());
         }
         alreadySeen.insert(pNext);

         pMP = pNext;
         pNext = pMP->GetReplaced();
      }
      //Print("MapPoint: ", "end FindFinalReplacement");
      return pMP;
   }

   bool MapPoint::IsBad()
   {
      unique_lock<mutex> lock(mMutexFeatures);
      return mbBad;
   }

   void MapPoint::IncreaseVisible(int n)
   {
      unique_lock<mutex> lock(mMutexFeatures);
      mnVisible += n;
      mModified = true;
   }

   void MapPoint::IncreaseFound(int n)
   {
      unique_lock<mutex> lock(mMutexFeatures);
      mnFound += n;
      mModified = true;
   }

   float MapPoint::GetFoundRatio()
   {
      unique_lock<mutex> lock(mMutexFeatures);
      return static_cast<float>(mnFound) / mnVisible;
   }

   void MapPoint::ComputeDistinctiveDescriptors()
   {
      // Retrieve all observed descriptors
      vector<cv::Mat> vDescriptors;

      map<KeyFrame*, size_t> obs;

      {
         unique_lock<mutex> lock(mMutexFeatures);
         if (mbBad) return;
         if (mObservations.empty()) return;
         obs = mObservations;
      }

      vDescriptors.reserve(obs.size());

      for (map<KeyFrame*, size_t>::iterator mit = obs.begin(), mend = obs.end(); mit != mend; mit++)
      {
         KeyFrame* pKF = mit->first;

         if (!pKF->IsBad())
            vDescriptors.push_back(pKF->descriptors.row(mit->second));
      }

      if (vDescriptors.empty())
         return;

      // Compute distances between them
      const size_t N = vDescriptors.size();

      //float Distances[N][N];
      float * Distances = new float[N*N];
      for (size_t i = 0;i < N;i++)
      {
         //Distances[i][i]=0;
         Distances[i*N + i] = 0;
         for (size_t j = i + 1;j < N;j++)
         {
            int distij = ORBmatcher::DescriptorDistance(vDescriptors[i], vDescriptors[j]);
            //Distances[i][j]=distij;
            Distances[i*N + j] = distij;
            //Distances[j][i]=distij;
            Distances[j*N + i] = distij;
         }
      }

      // Take the descriptor with least median distance to the rest
      int BestMedian = INT_MAX;
      int BestIdx = 0;
      for (size_t i = 0;i < N;i++)
      {
         //vector<int> vDists(Distances[i],Distances[i]+N);
         vector<int> vDists(&Distances[i*N], &Distances[i*N] + N);
         sort(vDists.begin(), vDists.end());
         int median = vDists[0.5*(N - 1)];

         if (median < BestMedian)
         {
            BestMedian = median;
            BestIdx = i;
         }
      }

      delete[] Distances;

      {
         unique_lock<mutex> lock(mMutexFeatures);
         mDescriptor = vDescriptors[BestIdx].clone();
         mModified = true;
      }
   }

   cv::Mat MapPoint::GetDescriptor()
   {
      unique_lock<mutex> lock(mMutexFeatures);
      return mDescriptor.clone();
   }

   int MapPoint::GetIndexInKeyFrame(KeyFrame *pKF)
   {
      unique_lock<mutex> lock(mMutexFeatures);
      if (mObservations.count(pKF))
         return mObservations[pKF];
      else
         return -1;
   }

   bool MapPoint::IsObserving(KeyFrame *pKF)
   {
      unique_lock<mutex> lock(mMutexFeatures);
      return (mObservations.count(pKF));
   }

   void MapPoint::UpdateNormalAndDepth()
   {
      map<KeyFrame*, size_t> obs;
      KeyFrame* pRefKF;
      cv::Mat Pos;
      {
         unique_lock<mutex> lock1(mMutexFeatures);
         if (mbBad) return;
         if (mpRefKF == NULL)
            throw exception("MapPoint::UpdateNormalAndDepth mpRefKF is NULL");
         pRefKF = mpRefKF;
         obs = mObservations;
         unique_lock<mutex> lock2(mMutexPos);
         Pos = mWorldPos.clone();
      }

      if (obs.empty())
         return;

      cv::Mat normal = cv::Mat::zeros(3, 1, CV_32F);
      int n = 0;
      for (map<KeyFrame*, size_t>::iterator mit = obs.begin(), mend = obs.end(); mit != mend; mit++)
      {
         KeyFrame* pKF = mit->first;
         cv::Mat Owi = pKF->GetCameraCenter();
         cv::Mat normali = Pos - Owi;
         normal = normal + normali / cv::norm(normali);
         n++;
      }

      cv::Mat PC = Pos - pRefKF->GetCameraCenter();
      const float dist = cv::norm(PC);
      const int level = pRefKF->keysUn[obs[pRefKF]].octave;
      const float levelScaleFactor = pRefKF->scaleFactors[level];
      const int nLevels = pRefKF->scaleLevels;

      {
         mModified = true;
         unique_lock<mutex> lock3(mMutexPos);
         mfMaxDistance = dist * levelScaleFactor;
         mfMinDistance = mfMaxDistance / pRefKF->scaleFactors[nLevels - 1];
         mNormalVector = normal / n;
      }
   }

   float MapPoint::GetMinDistanceInvariance()
   {
      unique_lock<mutex> lock(mMutexPos);
      return 0.8f*mfMinDistance;
   }

   float MapPoint::GetMaxDistanceInvariance()
   {
      unique_lock<mutex> lock(mMutexPos);
      return 1.2f*mfMaxDistance;
   }

   int MapPoint::PredictScale(const float &currentDist, KeyFrame* pKF)
   {
      float ratio;
      {
         unique_lock<mutex> lock(mMutexPos);
         ratio = mfMaxDistance / currentDist;
      }

      int nScale = ceil(log(ratio) / pKF->logScaleFactor);
      if (nScale < 0)
         nScale = 0;
      else if (nScale >= pKF->scaleLevels)
         nScale = pKF->scaleLevels - 1;

      return nScale;
   }

   int MapPoint::PredictScale(const float &currentDist, Frame* pF)
   {
      float ratio;
      {
         unique_lock<mutex> lock(mMutexPos);
         ratio = mfMaxDistance / currentDist;
      }

      int nScale = ceil(log(ratio) / pF->mfLogScaleFactor);
      if (nScale < 0)
         nScale = 0;
      else if (nScale >= pF->mnScaleLevels)
         nScale = pF->mnScaleLevels - 1;

      return nScale;
   }

   bool MapPoint::GetModified()
   {
      return mModified;
   }

   void MapPoint::SetModified(bool b)
   {
      mModified = b;
   }

   void MapPoint::CompleteLink(size_t idx, KeyFrame & rKF) 
   {
      if (mpRefKF == NULL)
         mpRefKF = &rKF;

      if (rKF.right[idx] >= 0)
         mnObs += 2;
      else
         mnObs++;

      mModified = true;
   }

   void MapPoint::CompleteUnlink(size_t idx, KeyFrame & rKF) 
   {
      if (rKF.right[idx] >= 0)
         mnObs -= 2;
      else
         mnObs--;
      
      if (mpRefKF == &rKF) {
         if (mObservations.empty())
            mpRefKF = NULL;
         else
            mpRefKF = mObservations.begin()->first;
      }

      if (mnObs <= 2)
         mbBad = true;

      mModified = true;
   }

   MapPoint * MapPoint::Find(const id_type id, const Map & rMap, std::unordered_map<id_type, MapPoint *> & newMapPoints)
   {
      MapPoint * pMP = rMap.GetMapPoint(id);
      if (pMP == NULL)
      {
         pMP = (newMapPoints.count(id) == 1) ? newMapPoints.at(id) : NULL;
      }
      return pMP;
   }

   id_type MapPoint::PeekId(void * const buffer)
   {
      MapPoint::Header * pHeader = (MapPoint::Header *)buffer;
      return pHeader->mnId;
   }

   size_t MapPoint::GetVectorBufferSize(const std::vector<MapPoint *> & mpv)
   {
      size_t size = sizeof(size_t);
      for (MapPoint * pMP : mpv)
      {
         if (pMP == NULL)
            size += sizeof(id_type);
         else
            size += pMP->GetBufferSize();
      }
      return size;
   }

   void * MapPoint::ReadVector(
      void * buffer, 
      const Map & rMap, 
      std::unordered_map<id_type, KeyFrame *> & newKeyFrames, 
      std::unordered_map<id_type, MapPoint *> & newMapPoints, 
      std::vector<MapPoint *> & mpv)
   {
      SyncPrint::Print("MapPoint: ", "begin ReadVector");
      size_t quantityMPs;
      void * pData = Serializer::ReadValue<size_t>(buffer, quantityMPs);
      mpv.resize(quantityMPs);
      for (int i = 0; i < quantityMPs; ++i)
      {
         id_type id = MapPoint::PeekId(pData);
         if (id == (id_type)-1)
         {
            pData = (id_type *)pData + 1;
            mpv[i] = NULL;
         }
         else
         {
            MapPoint * pMP = rMap.GetMapPoint(id);
            if (pMP == NULL)
            {
               pMP = (newMapPoints.count(id) == 1) ? newMapPoints.at(id) : NULL;
               if (pMP == NULL)
               {
                  pMP = new MapPoint(id);
                  newMapPoints[id] = pMP;
               }
            }
            pData = pMP->ReadBytes(pData, rMap, newKeyFrames, newMapPoints);
            mpv[i] = pMP;
         }
      }
      SyncPrint::Print("MapPoint: ", "end ReadVector");
      return pData;
   }

   void * MapPoint::WriteVector(
      void * buffer,
      std::vector<MapPoint *> & mpv)
   {
      void * pData = Serializer::WriteValue<size_t>(buffer, mpv.size());
      for (MapPoint * pMP : mpv)
      {
         if (pMP == NULL)
         {
            *(id_type *)pData = (id_type)-1;
            pData = (id_type *)pData + 1;
         }
         else
            pData = pMP->WriteBytes(pData);
      }
      return pData;
   }

   size_t MapPoint::GetSetBufferSize(const std::set<MapPoint *> & mps)
   {
      size_t size = sizeof(size_t);
      for (MapPoint * pMP : mps)
         size += pMP->GetBufferSize();
      return size;
   }

   void * MapPoint::ReadSet(
      void * buffer, 
      const Map & rMap, 
      std::unordered_map<id_type, KeyFrame *> & newKeyFrames, 
      std::unordered_map<id_type, MapPoint *> & newMapPoints, 
      std::set<MapPoint *> & mps)
   {
      size_t quantityMPs;
      void * pData = Serializer::ReadValue<size_t>(buffer, quantityMPs);
      mps.clear();
      for (int i = 0; i < quantityMPs; ++i)
      {
         id_type id = MapPoint::PeekId(pData);
         MapPoint * pMP = rMap.GetMapPoint(id);
         if (pMP == NULL)
         {
            pMP = (newMapPoints.count(id) == 1) ? newMapPoints.at(id) : NULL;
            if (pMP == NULL)
            {
               pMP = new MapPoint(id);
               newMapPoints[id] = pMP;
            }
         }
         pData = pMP->ReadBytes(pData, rMap, newKeyFrames, newMapPoints);
         mps.insert(pMP);
      }
      return pData;
   }

   void * MapPoint::WriteSet(
      void * buffer,
      std::set<MapPoint *> & mps)
   {
      void * pData = Serializer::WriteValue<size_t>(buffer, mps.size());
      for (MapPoint * pMP : mps)
      {
         pData = pMP->WriteBytes(pData);
      }
      return pData;
   }

   size_t MapPoint::GetBufferSize()
   {
      unique_lock<mutex> lock1(mMutexPos);
      unique_lock<mutex> lock2(mMutexFeatures);

      size_t size = sizeof(MapPoint::Header);
      size += Serializer::GetMatBufferSize(mWorldPos);
      size += Serializer::GetMatBufferSize(mNormalVector);
      size += Serializer::GetMatBufferSize(mDescriptor);
      size += Serializer::GetVectorBufferSize<Observation>(mObservations.size());
      return size;
   }

   void * MapPoint::ReadBytes(
      void * const buffer, 
      const Map & rMap, 
      std::unordered_map<id_type, KeyFrame *> & newKeyFrames, 
      std::unordered_map<id_type, MapPoint *> & newMapPoints)
   {
      unique_lock<mutex> lock1(mMutexPos);
      unique_lock<mutex> lock2(mMutexFeatures);

      MapPoint::Header * pHeader = (MapPoint::Header *)buffer;
      if (mnId != pHeader->mnId)
         throw exception("MapPoint::ReadBytes mnId != pHeader->mnId");

      mnFirstKFid = pHeader->mnFirstKFId;
      mnObs = pHeader->mnObs;

      mpRefKF = KeyFrame::Find(pHeader->mpRefKFId, rMap, newKeyFrames);
      if (mpRefKF == NULL)
      {
         std::stringstream ss;
         ss << "MapPoint::ReadBytes detected an unknown KeyFrame with id=" << pHeader->mpRefKFId;
         throw exception(ss.str().c_str());
         //mpRefKF = new KeyFrame(pHeader->mpRefKFId);
         //newKeyFrames[pHeader->mpRefKFId] = mpRefKF;
      }

      mnVisible = pHeader->mnVisible;
      mnFound = pHeader->mnFound;
      mbBad = pHeader->mbBad;
      if (pHeader->mpReplacedId == (id_type)-1)
         mpReplaced = NULL;
      else
      {
         mpReplaced = MapPoint::Find(pHeader->mpReplacedId, rMap, newMapPoints);
         if (mpReplaced == NULL)
         {
            mpReplaced = new MapPoint(pHeader->mpReplacedId);
            newMapPoints[pHeader->mpReplacedId] = mpReplaced;
         }
      }
      mfMinDistance = pHeader->mfMinDistance;
      mfMaxDistance = pHeader->mfMaxDistance;

      // read variable-length data
      void * pData = pHeader + 1;
      pData = Serializer::ReadMatrix(pData, mWorldPos);
      pData = Serializer::ReadMatrix(pData, mNormalVector);
      pData = Serializer::ReadMatrix(pData, mDescriptor);
      pData = ReadObservations(pData, rMap, newKeyFrames, mObservations);
      return pData;
   }

   void * MapPoint::WriteBytes(void * const buffer)
   {
      unique_lock<mutex> lock1(mMutexPos);
      unique_lock<mutex> lock2(mMutexFeatures);

      MapPoint::Header * pHeader = (MapPoint::Header *)buffer;
      pHeader->mnId = mnId;
      pHeader->mnFirstKFId = firstKFid;
      pHeader->mnObs = mnObs;
      pHeader->mpRefKFId = mpRefKF->id;
      pHeader->mnVisible = mnVisible;
      pHeader->mnFound = mnFound;
      pHeader->mbBad = mbBad;
      if (mpReplaced)
         pHeader->mpReplacedId = mpReplaced->id;
      else
         pHeader->mpReplacedId = (id_type)-1;
      pHeader->mfMinDistance = mfMinDistance;
      pHeader->mfMaxDistance = mfMaxDistance;

      // write variable-length data
      void * pData = pHeader + 1;
      pData = Serializer::WriteMatrix(pData, mWorldPos);
      pData = Serializer::WriteMatrix(pData, mNormalVector);
      pData = Serializer::WriteMatrix(pData, mDescriptor);
      pData = WriteObservations(pData, mObservations);
      return pData;
   }

   void * MapPoint::ReadObservations(
      void * const buffer,
      const Map & rMap,
      std::unordered_map<id_type, KeyFrame *> & newKeyFrames,
      std::map<KeyFrame *, size_t> & observations)
   {
      observations.clear();
      size_t * pQuantity = (size_t *)buffer;
      Observation * pData = (Observation *)(pQuantity + 1);
      Observation * pEnd = pData + *pQuantity;
      while (pData < pEnd)
      {
         KeyFrame * pKF = KeyFrame::Find(pData->keyFrameId, rMap, newKeyFrames);
         if (pKF == NULL)
         {
            std::stringstream ss;
            ss << "MapPoint::ReadObservations detected an unknown KeyFrame with id=" << pData->keyFrameId;
            throw exception(ss.str().c_str());
            //pKF = new KeyFrame(pData->keyFrameId);
            //newKeyFrames[pData->keyFrameId] = pKF;
         }
         observations[pKF] = pData->index;
         ++pData;
      }
      return pData;
   }

   void * MapPoint::WriteObservations(void * const buffer, std::map<KeyFrame *, size_t> & observations)
   {
      size_t * pQuantity = (size_t *)buffer;
      *pQuantity = observations.size();
      Observation * pData = (Observation *)(pQuantity + 1);
      for (std::pair<KeyFrame *, size_t> p : observations)
      {
         pData->keyFrameId = p.first->id;
         pData->index = p.second;
         ++pData;
      }
      return pData;
   }

} //namespace ORB_SLAM
