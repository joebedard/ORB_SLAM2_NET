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

#include "MapDrawer.h"
#include <mutex>

namespace ORB_SLAM2
{


   MapDrawer::MapDrawer(cv::FileStorage & fSettings, Mapper & mapper) :
      SyncPrint("MapDrawer: "),
      mMapper(mapper),
      mMap(mapper.GetMap())
   {
      mKeyFrameSize = fSettings["Viewer.KeyFrameSize"];
      mKeyFrameLineWidth = fSettings["Viewer.KeyFrameLineWidth"];
      mGraphLineWidth = fSettings["Viewer.GraphLineWidth"];
      mPointSize = fSettings["Viewer.PointSize"];
      mCameraSize = fSettings["Viewer.CameraSize"];
      mCameraLineWidth = fSettings["Viewer.CameraLineWidth"];

      mViewpointX = fSettings["Viewer.ViewpointX"];
      mViewpointY = fSettings["Viewer.ViewpointY"];
      mViewpointZ = fSettings["Viewer.ViewpointZ"];
      mViewpointF = fSettings["Viewer.ViewpointF"];
   }

   void MapDrawer::Reset()
   {
      unique_lock<mutex> mutex(mMutexReferenceMapPoints);
      mvpReferenceMapPoints.clear();
   }

   void MapDrawer::Follow(pangolin::OpenGlRenderState & pRenderState)
   {
      unique_lock<mutex> lock(mMutexCamera);
      pangolin::OpenGlMatrix Twc;
      ConvertMatrixFromOpenCvToOpenGL(Twc, mCameraPose);
      pRenderState.Follow(Twc);
   }

   void MapDrawer::DrawMapPoints()
   {
      //Print("begin DrawMapPoints");
      float currentColor[4];
      glGetFloatv(GL_CURRENT_COLOR, currentColor);

      //Print("unique_lock<mutex> lock1(mMap->mMutexMapUpdate);");
      unique_lock<mutex> lock1(mMapper.GetMutexMapUpdate());
      //Print("const vector<MapPoint*> &vpMPs = mMap->GetAllMapPoints();");
      const vector<MapPoint*> &vpMPs = mMap.GetAllMapPoints();

      //Print("unique_lock<mutex> lock2(mMutexReferenceMapPoints);");
      unique_lock<mutex> lock2(mMutexReferenceMapPoints);
      //Print("set<MapPoint*> spRefMPs(mvpReferenceMapPoints.begin(), mvpReferenceMapPoints.end());");
      set<MapPoint*> spRefMPs(mvpReferenceMapPoints.begin(), mvpReferenceMapPoints.end());
      //stringstream ss;
      //ss << "spRefMPs = ";
      //for (MapPoint * mp : spRefMPs)
      //{
      //    if (mp)
      //        ss << mp->GetWorldPos() << " ";
      //    else
      //        ss << "NULL" << " ";
      //}
      //Print(ss.str().c_str());

      if (vpMPs.empty())
      {
         //Print("end DrawMapPoints 1");
         return;
      }

      glPointSize(mPointSize);
      glBegin(GL_POINTS);
      glColor3f(0.0, 0.0, 0.0);

      //Print("for(size_t i=0, iend=vpMPs.size(); i < iend;i++)");
      for (size_t i = 0, iend = vpMPs.size(); i < iend;i++)
      {
         //Print("if (vpMPs[i]->isBad() || spRefMPs.count(vpMPs[i]))");
         //Print(vpMPs[i] == NULL ? "vpMPs[i] == NULL" : "vpMPs[i] != NULL");
         if (vpMPs[i]->isBad() || spRefMPs.count(vpMPs[i]))
            continue;
         //Print("cv::Mat pos = vpMPs[i]->GetWorldPos();");
         cv::Mat pos = vpMPs[i]->GetWorldPos();
         //Print("glVertex3f(pos.at<float>(0), pos.at<float>(1), pos.at<float>(2));");
         glVertex3f(pos.at<float>(0), pos.at<float>(1), pos.at<float>(2));
      }
      //Print("glEnd();");
      glEnd();

      glPointSize(mPointSize);
      glBegin(GL_POINTS);
      glColor3f(1.0, 0.0, 0.0);

      /*Print("for(set<MapPoint*>::iterator sit=spRefMPs.begin(), send=spRefMPs.end(); sit!=send; sit++)");
      for(set<MapPoint*>::iterator sit=spRefMPs.begin(), send=spRefMPs.end(); sit!=send; sit++)
      {
          //Print("if((*sit)->isBad())");
          if((*sit)->isBad())
              continue;
          //Print("cv::Mat pos = (*sit)->GetWorldPos();");
          cv::Mat pos = (*sit)->GetWorldPos();
          if (pos.empty())
              throw exception("GetWorldPos() is empty!!!!!!!!!!!!!!!!!!!!!!");
          //stringstream ss;
          //ss << "pos = "<< pos;
          //Print(ss.str().c_str());
          glVertex3f(pos.at<float>(0),pos.at<float>(1),pos.at<float>(2));
      }*/

      //Print("for (MapPoint * pMP : mvpReferenceMapPoints)");
      for (MapPoint * pMP : mvpReferenceMapPoints)
      {
         if (pMP->isBad())
            continue;
         cv::Mat pos = pMP->GetWorldPos();
         if (pos.empty())
         {
            //Print(to_string(pMP->GetId()) + "=MapPointId GetWorldPos() is empty! replaced=" + (pMP->GetReplaced() ? "true" : "false"));
            throw exception("GetWorldPos() is empty!!!!!!!!!!!!!!!!!!!!!!");
         }
         else
         {
            //stringstream ss;
            //ss << "pos = "<< pos;
            //Print(ss.str().c_str());
            glVertex3f(pos.at<float>(0), pos.at<float>(1), pos.at<float>(2));
         }
      }

      //Print("glEnd();");
      glEnd();
      glColor4fv(currentColor);
      //Print("end DrawMapPoints 2");
   }

   void MapDrawer::DrawKeyFrames(const bool bDrawKF, const bool bDrawGraph)
   {
      unique_lock<mutex> lock(mMapper.GetMutexMapUpdate());
      //Print("begin DrawKeyFrames");

      float currentColor[4];
      glGetFloatv(GL_CURRENT_COLOR, currentColor);

      const float &w = mKeyFrameSize;
      const float h = w * 0.75;
      const float z = w * 0.6;

      const vector<KeyFrame*> vpKFs = mMap.GetAllKeyFrames();

      if (bDrawKF)
      {
         for (size_t i = 0; i < vpKFs.size(); i++)
         {
            KeyFrame* pKF = vpKFs[i];
            cv::Mat Twc = pKF->GetPoseInverse().t();

            glPushMatrix();

            glMultMatrixf(Twc.ptr<GLfloat>(0));

            glLineWidth(mKeyFrameLineWidth);
            glColor3f(0.0f, 0.0f, 1.0f);
            glBegin(GL_LINES);
            glVertex3f(0, 0, 0);
            glVertex3f(w, h, z);
            glVertex3f(0, 0, 0);
            glVertex3f(w, -h, z);
            glVertex3f(0, 0, 0);
            glVertex3f(-w, -h, z);
            glVertex3f(0, 0, 0);
            glVertex3f(-w, h, z);

            glVertex3f(w, h, z);
            glVertex3f(w, -h, z);

            glVertex3f(-w, h, z);
            glVertex3f(-w, -h, z);

            glVertex3f(-w, h, z);
            glVertex3f(w, h, z);

            glVertex3f(-w, -h, z);
            glVertex3f(w, -h, z);
            glEnd();

            glPopMatrix();
         }
      }

      if (bDrawGraph)
      {
         glLineWidth(mGraphLineWidth);
         glColor4f(0.0f, 1.0f, 0.0f, 0.6f);
         glBegin(GL_LINES);

         for (size_t i = 0; i < vpKFs.size(); i++)
         {
            // Covisibility Graph
            const vector<KeyFrame*> vCovKFs = vpKFs[i]->GetCovisiblesByWeight(100);
            cv::Mat Ow = vpKFs[i]->GetCameraCenter();
            if (!vCovKFs.empty())
            {
               for (vector<KeyFrame*>::const_iterator vit = vCovKFs.begin(), vend = vCovKFs.end(); vit != vend; vit++)
               {
                  if ((*vit)->GetId() < vpKFs[i]->GetId())
                     continue;
                  cv::Mat Ow2 = (*vit)->GetCameraCenter();
                  glVertex3f(Ow.at<float>(0), Ow.at<float>(1), Ow.at<float>(2));
                  glVertex3f(Ow2.at<float>(0), Ow2.at<float>(1), Ow2.at<float>(2));
               }
            }

            // Spanning tree
            KeyFrame* pParent = vpKFs[i]->GetParent();
            if (pParent)
            {
               cv::Mat Owp = pParent->GetCameraCenter();
               glVertex3f(Ow.at<float>(0), Ow.at<float>(1), Ow.at<float>(2));
               glVertex3f(Owp.at<float>(0), Owp.at<float>(1), Owp.at<float>(2));
            }

            // Loops
            set<KeyFrame*> sLoopKFs = vpKFs[i]->GetLoopEdges();
            for (set<KeyFrame*>::iterator sit = sLoopKFs.begin(), send = sLoopKFs.end(); sit != send; sit++)
            {
               if ((*sit)->GetId() < vpKFs[i]->GetId())
                  continue;
               cv::Mat Owl = (*sit)->GetCameraCenter();
               glVertex3f(Ow.at<float>(0), Ow.at<float>(1), Ow.at<float>(2));
               glVertex3f(Owl.at<float>(0), Owl.at<float>(1), Owl.at<float>(2));
            }
         }

         glEnd();
      }
      glColor4fv(currentColor);
      //Print("end DrawKeyFrames");
   }

   void MapDrawer::DrawCurrentCameras()
   {
      //Print("begin DrawCurrentCameras");
      float currentColor[4];
      glGetFloatv(GL_CURRENT_COLOR, currentColor);

      const float &w = mCameraSize;
      const float h = w * 0.75;
      const float z = w * 0.6;

      pangolin::OpenGlMatrix Twc, pivotGL;
      vector<cv::Mat> poses = mMapper.GetTrackerPoses();
      vector<cv::Mat> pivots = mMapper.GetTrackerPivots();
      assert(poses.size() == pivots.size());
      for (int i = 0; i < poses.size(); i++)
      {
         glPushMatrix();
         glLineWidth(mCameraLineWidth);

         // display the camera frustrum
         Twc.SetIdentity();
         ConvertMatrixFromOpenCvToOpenGL(Twc, poses[i]);
#ifdef HAVE_GLES
         glMultMatrixf(Twc.m);
#else
         glMultMatrixd(Twc.m);
#endif

         glColor3f(0.0f, 1.0f, 0.0f);
         glBegin(GL_LINES);
         glVertex3f(0, 0, 0);
         glVertex3f(w, h, z);
         glVertex3f(0, 0, 0);
         glVertex3f(w, -h, z);
         glVertex3f(0, 0, 0);
         glVertex3f(-w, -h, z);
         glVertex3f(0, 0, 0);
         glVertex3f(-w, h, z);

         glVertex3f(w, h, z);
         glVertex3f(w, -h, z);

         glVertex3f(-w, h, z);
         glVertex3f(-w, -h, z);

         glVertex3f(-w, h, z);
         glVertex3f(w, h, z);

         glVertex3f(-w, -h, z);
         glVertex3f(w, -h, z);

         glColor3f(1.0f, 0.0f, 0.0f);
         glVertex3f(0.0f, 0.0f, 0.0f);
         glVertex3f(0.1f, 0.0f, 0.0f);
         glColor3f(0.0f, 1.0f, 0.0f);
         glVertex3f(0.0f, 0.0f, 0.0f);
         glVertex3f(0.0f, 0.1f, 0.0f);
         glColor3f(0.0f, 0.0f, 1.0f);
         glVertex3f(0.0f, 0.0f, 0.0f);
         glVertex3f(0.0f, 0.0f, 0.1f);
         glEnd();

         // display a line for the pivot calibration
         pivotGL.SetIdentity();
         ConvertMatrixFromOpenCvToOpenGL(pivotGL, pivots[i]);
#ifdef HAVE_GLES
         glMultMatrixf(pivotGL.m);
#else
         glMultMatrixd(pivotGL.m);
#endif

         float x = pivots[i].at<float>(0, 3);
         float y = pivots[i].at<float>(1, 3);
         float z = pivots[i].at<float>(2, 3);

         glBegin(GL_LINES);
         glColor3f(1.0f, 0.0f, 1.0f);
         glVertex3f(0, 0, 0);
         glVertex3f(x, y, z);
         glEnd();

         glPopMatrix();
      }

      glColor4fv(currentColor);
      //Print("end DrawCurrentCameras");
   }

   void MapDrawer::SetCurrentCameraPose(const cv::Mat &Tcw)
   {
      unique_lock<mutex> lock(mMutexCamera);
      mCameraPose = Tcw.clone();
   }

   void MapDrawer::ConvertMatrixFromOpenCvToOpenGL(pangolin::OpenGlMatrix &M, cv::Mat pose)
   {
      //Print("begin ConvertMatrixFromOpenCvToOpenGL");
      if (!pose.empty())
      {
         cv::Mat Rwc(3, 3, CV_32F);
         cv::Mat twc(3, 1, CV_32F);
         {
            Rwc = pose.rowRange(0, 3).colRange(0, 3).t();
            twc = -Rwc * pose.rowRange(0, 3).col(3);
         }

         M.m[0] = Rwc.at<float>(0, 0);
         M.m[1] = Rwc.at<float>(1, 0);
         M.m[2] = Rwc.at<float>(2, 0);
         M.m[3] = 0.0;

         M.m[4] = Rwc.at<float>(0, 1);
         M.m[5] = Rwc.at<float>(1, 1);
         M.m[6] = Rwc.at<float>(2, 1);
         M.m[7] = 0.0;

         M.m[8] = Rwc.at<float>(0, 2);
         M.m[9] = Rwc.at<float>(1, 2);
         M.m[10] = Rwc.at<float>(2, 2);
         M.m[11] = 0.0;

         M.m[12] = twc.at<float>(0);
         M.m[13] = twc.at<float>(1);
         M.m[14] = twc.at<float>(2);
         M.m[15] = 1.0;
      }
      else
         M.SetIdentity();
      //Print("end ConvertMatrixFromOpenCvToOpenGL");
   }

   void MapDrawer::SetReferenceMapPoints(const std::vector<MapPoint*>& vpMPs)
   {
      //Print("begin SetReferenceMapPoints");
      unique_lock<mutex> lock(mMutexReferenceMapPoints);
      stringstream ss;
      //ss << "mvpReferenceMapPoints = ";
      //for (MapPoint * mp : vpMPs)
      //{
      //    ss << mp->GetWorldPos() << " ";
      //}
      //Print(ss.str().c_str());
      mvpReferenceMapPoints = vpMPs;
      //Print("end SetReferenceMapPoints");
   }

   float MapDrawer::GetViewpointX()
   {
      return mViewpointX;
   }

   float MapDrawer::GetViewpointY()
   {
      return mViewpointY;
   }

   float MapDrawer::GetViewpointZ()
   {
      return mViewpointZ;
   }

   float MapDrawer::GetViewpointF()
   {
      return mViewpointF;
   }

} //namespace ORB_SLAM
