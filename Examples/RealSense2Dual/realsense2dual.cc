/**
* This file is part of ORB-SLAM2-CS.
*
* Copyright (C) 2018 Joe Bedard <mr dot joe dot bedard at gmail dot com>
* For more information see <https://github.com/joebedard/ORB_SLAM2_CS>
*
* ORB-SLAM2-CS is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* ORB-SLAM2-CS is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with ORB-SLAM2-CS. If not, see <http://www.gnu.org/licenses/>.
*/


#include<iostream>
#include<algorithm>
#include<fstream>
#include<iomanip>
#include<chrono>

#include <librealsense2/rs.hpp>
#include <opencv2/core/core.hpp>

#include <Sleep.h>
#include <Enums.h>
#include <Tracking.h>
#include <ORBVocabulary.h>
#include <FrameDrawer.h>
#include <Map.h>

using namespace std;
using namespace ORB_SLAM2;
using namespace cv;

struct ThreadParam
{
   int returnCode;
   char * settingsFilePath;
   ORBVocabulary * vocabulary;
   Map * map;
   MapDrawer * mapDrawer;
   Mapper * mapper;
};
ThreadParam mThreadParams[2];
bool mShouldRun = true;

// command line parameters
char * mVocFile = NULL;
char * mSettingsFile1 = NULL;
char * mSettingsFile2 = NULL;

void ParseParams(int paramc, char * paramv[])
{
   if (paramc != 4)
   {
      const char * usage = "Usage: ./realsense2 vocabulary_file_and_path first_settings_file_and_path second_settings_file_and_path";
      std::exception e(usage);
      throw e;
   }
   mVocFile = paramv[1];
   mSettingsFile1 = paramv[2];
   mSettingsFile2 = paramv[3];
}

void ParseSettings(FileStorage & settings, const char * settingsFilePath, string & serial, int & width, int & height)
{
   if (!settings.isOpened())
   {
      std::string m("Failed to open settings file at: ");
      m.append(settingsFilePath);
      throw std::exception(m.c_str());
   }

   serial.append(settings["Camera.serial"]);
   if (0 == serial.length())
      throw new exception("Camera.serial property is not set or value is not in quotes.");

   width = settings["Camera.width"];
   if (0 == width)
      throw new exception("Camera.width is not set.");

   height = settings["Camera.height"];
   if (0 == height)
      throw new exception("Camera.height is not set.");
}

void RunTracker(int threadId) try
{
   FileStorage settings(mThreadParams[threadId].settingsFilePath, FileStorage::READ);
   
   string serial;
   int width, height;
   ParseSettings(settings, mThreadParams[threadId].settingsFilePath, serial, width, height);

   // Declare RealSense pipeline, encapsulating the actual device and sensors
   rs2::pipeline pipe;

   // create and resolve custom configuration for RealSense
   rs2::config customConfig;
   customConfig.enable_device(serial);
   customConfig.enable_stream(RS2_STREAM_INFRARED, 1, width, height, RS2_FORMAT_Y8, 30);
   customConfig.enable_stream(RS2_STREAM_INFRARED, 2, width, height, RS2_FORMAT_Y8, 30);
   if (!customConfig.can_resolve(pipe))
      throw new exception("Can not resolve RealSense config.");

   rs2::pipeline_profile profile = pipe.start(customConfig);
   rs2::depth_sensor sensor = profile.get_device().first<rs2::depth_sensor>();

   // disable the projected IR pattern when using stereo IR images
   sensor.set_option(RS2_OPTION_EMITTER_ENABLED, 0);

   FrameDrawer frameDrawer(mThreadParams[threadId].map);

   Tracking tracker(mThreadParams[threadId].vocabulary, &frameDrawer, mThreadParams[threadId].mapDrawer,
      mThreadParams[threadId].map, mThreadParams[threadId].mapper, settings, eSensor::STEREO);

   // vector for tracking time statistics
   vector<float> vTimesTrack;
   std::chrono::steady_clock::time_point tStart = std::chrono::steady_clock::now();

   while (mShouldRun)
   {
      rs2::frameset data = pipe.wait_for_frames(); // Wait for next set of frames from the camera
      rs2::video_frame irFrame1 = data.get_infrared_frame(1);
      rs2::video_frame irFrame2 = data.get_infrared_frame(2);

      // Create OpenCV matrix of size (width, height)
      Mat irMat1(Size(width, height), CV_8UC1, (void*)irFrame1.get_data(), Mat::AUTO_STEP);
      Mat irMat2(Size(width, height), CV_8UC1, (void*)irFrame2.get_data(), Mat::AUTO_STEP);

      std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();

      double tframe = std::chrono::duration_cast<std::chrono::duration<double> >(t1 - tStart).count();

      tracker.GrabImageStereo(irMat1, irMat2, tframe);

      std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();

      double ttrack = std::chrono::duration_cast<std::chrono::duration<double> >(t2 - t1).count();

      vTimesTrack.push_back(ttrack);
   }

   // calculate time statistics
   sort(vTimesTrack.begin(), vTimesTrack.end());
   float totaltime = 0;
   for (int i = 0; i<vTimesTrack.size(); i++)
   {
      totaltime += vTimesTrack[i];
   }
   cout << "tracking time statistics for thread " << threadId << ":" << endl;
   cout << "   median tracking time: " << vTimesTrack[vTimesTrack.size() / 2] << endl;
   cout << "   mean tracking time: " << totaltime / vTimesTrack.size() << endl;

   mThreadParams[threadId].returnCode = EXIT_SUCCESS;
}
catch (const rs2::error & e)
{
   std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
   mThreadParams[threadId].returnCode = EXIT_FAILURE;
}
catch (const std::exception& e)
{
   std::cerr << e.what() << std::endl;
   mThreadParams[threadId].returnCode = EXIT_FAILURE;
}

int main(int paramc, char * paramv[]) try
{
   ParseParams(paramc, paramv);

   /*
   TODO:
   + create map, mapper, viewers, etc
   + start tracker threads
   + map drawer loop
   + join threads
   */

   return EXIT_SUCCESS;
}
catch (const rs2::error & e)
{
   std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
   return EXIT_FAILURE;
}
catch (const std::exception& e)
{
   std::cerr << e.what() << std::endl;
   return EXIT_FAILURE;
}