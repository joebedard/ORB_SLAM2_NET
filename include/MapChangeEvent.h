/**
* This file is part of ORB-SLAM2-TEAM.
*
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

#ifndef MAPCHANGEEVENT_H
#define MAPCHANGEEVENT_H

#include <set>
#include "MapPoint.h"
#include "KeyFrame.h"
#include "SyncPrint.h"

namespace ORB_SLAM2
{

   // collects all map changes into a composite event
   class MapChangeEvent : protected SyncPrint
   {
   public:

      MapChangeEvent();

      std::set<KeyFrame *> updatedKeyFrames;

      std::set<id_type> deletedKeyFrames;

      std::set<MapPoint *> updatedMapPoints;

      // The id of the deleted MapPoint id is the key. The replacer MapPoint, is the value.
      //std::unordered_map<id_type, MapPoint *> replacedMapPoints;

      //std::set<id_type> deletedMapPoints;

      inline bool empty()
      {
         return updatedKeyFrames.empty() && deletedKeyFrames.empty() && updatedMapPoints.empty() /*&& deletedMapPoints.empty()*/;
      }

      size_t GetBufferSize();

      void * ReadBytes(void * const buffer, Map & map);

      void * WriteBytes(void * const buffer);

   private:

      struct MapPointReplacement
      {
         id_type deletedId;
         id_type replacerId;
      };

      static void * ReadReplacements(
         void * const buffer,
         Map & map,
         std::unordered_map<id_type, MapPoint *> & newMapPoints,
         std::unordered_map<id_type, MapPoint *> & replacements);

      static void * WriteReplacements(void * const buffer, std::unordered_map<id_type, MapPoint *> & replacements);

   };

}

#endif // MAPCHANGEEVENT_H