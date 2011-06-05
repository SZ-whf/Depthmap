// Feasibilty study for GML loader

#include <iostream>
#include <fstream>

using namespace std;

#include <generic/paftl.h>
#include <generic/comm.h>  // For communicator

#include <Sala/mgraph.h>
#include "gmlmap.h"

bool GMLMap::parse(const pvecstring& fileset, Communicator *comm)
{
   __time64_t time = 0;
   qtimer( time, 0 );
     
   bool firstever = true;
   int tracking = 0;
   int linecount = 0;
   int feature = -1;
   pstring key;

   for (int i = 0; i < fileset.size(); i++) {

      ifstream stream(fileset[i].c_str());

      while (!stream.eof())
      {
         pstring line(4096);
         stream >> line;
         linecount++;

         if (line.length()) {
            switch (tracking) {
               case 0:
                  if (compare(line,"<osgb:RoadLink",14)) {
                     feature = 0;
                     tracking = 1;
                  }
                  else if (compare(line,"<osgb:PathLink",14)) {
                     feature = 1;
                     tracking = 1;
                  }
                  else if (compare(line,"<osgb:ConnectingLink",20)) {
                     feature = 2;
                     tracking = 1;
                  }
                  break;
               case 1:
                  if (compare(line,"</osgb:RoadLink",15) || compare(line,"</osgb:PathLink",15) || compare(line,"</osgb:ConnectingLink",21)) {
                     tracking = 0;
                     feature = -1;
                  }
                  else if (compare(line,"<osgb:descriptiveTerm>",22)) {
                     // pick out description (might not be only one)
                     int begin = line.findindex('>');
                     int end = line.findindexreverse('<');
                     if (begin != -1 && end != -1) {
                        pstring thisdesc = line.substr(begin+1,end-begin-1);
                        if (key.empty()) {
                           key = thisdesc;
                        }
                        else {
                           key = key + pstring(" / ") + thisdesc;
                        }
                     }
                  }
                  else if (compare(line,"<gml:LineString",15)) {
                     tracking = 2;
                  }
                  break;
               case 2:
                  if (compare(line,"</gml:LineString",16)) {
                     key.clear();
                     tracking = 1;
                  }
                  else if (compare(line,"<gml:co",7)) {
                     pvecstring tokens = line.tokenize(' ',true);
                     pvecpoint poly;
                     for (int j = 0 ; j < tokens.size(); j++) {
                        if (j == tokens.size() - 1) {
                           // strip </gml:coordinate>
                           int x = tokens[j].findindexreverse('<');
                           tokens[j] = tokens[j].substr(0,x);
                        }
                        if (j == 0) {
                           // strip <gml:coordinate>
                           int x = tokens[j].findindex('>');
                           tokens[j] = tokens[j].substr(x+1);
                        }
                        pvecstring subtokens = tokens[j].tokenize(',');
                        if (subtokens.size() == 2) {
                           poly.push_back( Point2f(subtokens[0].c_double(),subtokens[1].c_double()) );
                           if (!firstever) {
                              m_region.encompass(poly.tail());
                           }
                           else {
                              m_region = Region(poly.head(),poly.head());
                              firstever = false;
                           }
                        }
                     }
                     if (poly.size()) {
                        if (key.empty()) {
                           key = "Miscellaneous";
                        }
                        int n = m_keys.searchindex(key);
                        if (n == -1) {
                           n = m_keys.add(key,polyset(),paftl::ADD_HERE);
                        }
                        m_keys.value(n).push_back(poly);
                     }
                  }
                  break;
            }
         }

         if (comm)
         {
            if (qtimer( time, 500 )) {
               if (comm->IsCancelled()) {
                  throw Communicator::CancelledException();
               }
            }
         }
      }
   }

   return !firstever;
}
