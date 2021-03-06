//\todo napravi da parser pamti pokazivac na zadnji poslani node koko se ne bi svaki put trebalo parsati korz cijeli xml
//\todo napraviti missionParser (tj klasu koja direktno poziva primitive) klasu tako da extenda razlicite klase za parsanje (u buducnosti pojednostavljeno
//prebacivanje na razlicite mission planere)
//\todo Pogledja treba li poslati sve evente kao jednu poruku na poectku, kako bi se kasnije smanjila kolicina podataka koja se salje skupa s primitivom
//\TODO Dodati mogucnost odabira vise evenata na koje primitiv može reagirati. (Nema potrebe za svaki slučaj nanovo definirati event)

/*********************************************************************
 * mission_parser.cpp
 *
 *  Created on: Apr 18, 2014
 *      Author: Filip Mandic
 *
 ********************************************************************/

/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (c) 2014, LABUST, UNIZG-FER
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the LABUST nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

#include <labust_mission/labustMission.hpp>

#include <misc_msgs/StartParser.h>
#include <misc_msgs/EvaluateExpression.h>
#include <tinyxml2.h>

using namespace tinyxml2;

namespace labust {
	namespace mission {

		/*********************************************************************
		 ***  MissionParser class definition
		 ********************************************************************/

		class MissionParser{

		public:

			/*****************************************************************
			 ***  Class functions
			 ****************************************************************/

			MissionParser(ros::NodeHandle& nh);

			void sendPrimitve();

			void go2pointFA(double north, double east, double heading, double speed, double victoryRadius);

			void go2pointUA(double north, double east, double speed, double victoryRadius);

			void dynamicPositioning(double north, double east, double heading);

			void courseKeepingFA(double course, double heading, double speed);

			void courseKeepingUA(double course, double speed);

			void ISOprimitive(int dof, double command, double hysteresis, double reference, double sampling_rate);

			int parseMission(int id, string xmlFile);

			int parseEvents(string xmlFile);

			int parseMissionParam(string xmlFile);

			void onRequestPrimitive(const std_msgs::UInt16::ConstPtr& req);

			void onEventString(const std_msgs::String::ConstPtr& msg);

			void onReceiveXmlPath(const misc_msgs::StartParser::ConstPtr& msg);

			/*****************************************************************
			 ***  Helper functions
			 ****************************************************************/

			//template <typename primitiveType>
			void serializePrimitive(int id, vector<uint8_t> serializedData);

			void onEventNextParse(XMLElement *elem2);

			/*****************************************************************
			 ***  Class variables
			 ****************************************************************/

			int ID, lastID, eventID;
			int newDOF;
			double newXpos, newYpos, newVictoryRadius, newSpeed, newCourse, newHeading, newTimeout;
			double newCommand, newHysteresis, newReference, newSamplingTime;


			string xmlFile;
			string missionEvents, missionParams;

			vector<uint8_t> onEventNextActive, onEventNext;

			ros::Publisher pubSendPrimitive, pubRiseEvent, pubMissionSetup;
			ros::Subscriber subRequestPrimitive, subEventString, subReceiveXmlPath;
			ros::ServiceClient srvExprEval;

			auv_msgs::NED offset;
			int breakpoint;
		};


		/*****************************************************************
		 ***  Class functions
		 ****************************************************************/

		MissionParser::MissionParser(ros::NodeHandle& nh):ID(0), lastID(0), newXpos(0), newYpos(0), newVictoryRadius(0), newSpeed(0),
				newCourse(0), newHeading(0), newTimeout(0), eventID(0), breakpoint(1),
				missionEvents(""){

			/* Subscribers */
			subRequestPrimitive = nh.subscribe<std_msgs::UInt16>("requestPrimitive",1,&MissionParser::onRequestPrimitive, this);
			subEventString = nh.subscribe<std_msgs::String>("eventString",1,&MissionParser::onEventString, this);
			subReceiveXmlPath = nh.subscribe<misc_msgs::StartParser>("startParse",1,&MissionParser::onReceiveXmlPath, this);

			/* Publishers */
			pubSendPrimitive = nh.advertise<misc_msgs::SendPrimitive>("sendPrimitive",1);
			pubRiseEvent = nh.advertise<std_msgs::String>("eventString",1);
			pubMissionSetup = nh.advertise<misc_msgs::MissionSetup>("missionSetup",1);

			/* Service */
			srvExprEval = nh.serviceClient<misc_msgs::EvaluateExpression>("evaluate_expression");

			/* Parse file path */
			ros::NodeHandle ph("~");
			xmlFile = "mission.xml";
			ph.param("xml_save_path", xmlFile, xmlFile);
		}

		void MissionParser::sendPrimitve(){

			ROS_ERROR("%s",xmlFile.c_str());

			ROS_ERROR("%d",ID);
			int status = parseMission(ID, xmlFile);
			ROS_ERROR("%s", primitives[status]);

			switch(status){

				case go2point_FA:

					ROS_ERROR("T2 = %f,%f, Heading = %f, Speed = %f, Victory radius = %f", newXpos, newYpos, newHeading, newSpeed, newVictoryRadius);
					go2pointFA(newXpos, newYpos, newHeading, newSpeed, newVictoryRadius);
					break;

				case go2point_UA:

					ROS_ERROR("T2 = %f,%f, Speed = %f, Victory radius = %f", newXpos, newYpos, newSpeed, newVictoryRadius);
					go2pointUA(newXpos, newYpos, newSpeed, newVictoryRadius);
					break;

				case dynamic_positioning:

					ROS_ERROR("T2 = %f,%f, Heading = %f", newXpos, newYpos, newHeading);
					dynamicPositioning(newXpos, newYpos, newHeading);
					break;

				case course_keeping_FA:

					ROS_ERROR("Course = %f, Heading = %f, Speed = %f", newCourse, newHeading, newSpeed);
					courseKeepingFA(newCourse, newHeading, newSpeed);
					break;

				case course_keeping_UA:

					ROS_ERROR("Course = %f, Speed = %f", newCourse, newSpeed);
					courseKeepingUA(newCourse, newSpeed);
					break;

				case iso:

					ROS_ERROR("DOF = %d, command = %f, hysteresis = %f, reference = %f, sampling_rate = %f", newDOF, newCommand, newHysteresis, newReference, newSamplingTime);
					ISOprimitive(newDOF, newCommand, newHysteresis, newReference, newSamplingTime);
					break;

				case none:

					ROS_ERROR("Mission ended.");
					std_msgs::String tmp;
					tmp.data = "/STOP";
					pubRiseEvent.publish(tmp);

					/* Reset file path */
					ros::NodeHandle ph("~");
					xmlFile = "mission.xml";
					ph.param("xml_save_path", xmlFile, xmlFile);
			}
		}

		void MissionParser::go2pointFA(double north, double east, double heading, double speed, double victoryRadius){

			misc_msgs::Go2PointFA data;
			data.point.north = north-offset.north;
			data.point.east = east-offset.east;
			data.point.depth = 0;
			data.heading = heading;
			data.speed = speed;
			data.victoryRadius = victoryRadius;

			serializePrimitive(go2point_FA, labust::utilities::serializeMsg(data));

		}

		void MissionParser::go2pointUA(double north, double east, double speed, double victoryRadius){

			misc_msgs::Go2PointUA data;
			data.point.north = north-offset.north;
			data.point.east = east-offset.east;
			data.point.depth = 0;
			data.speed = speed;
			data.victoryRadius = victoryRadius;

			serializePrimitive(go2point_UA, labust::utilities::serializeMsg(data));
		}

		void MissionParser::dynamicPositioning(double north, double east, double heading){

			misc_msgs::DynamicPositioning data;
			data.point.north = north-offset.north;
			data.point.east = east-offset.east;
			data.point.depth = 0;
			data.heading = heading;

			serializePrimitive(dynamic_positioning, labust::utilities::serializeMsg(data));

		}

		void MissionParser::courseKeepingFA(double course, double heading, double speed){

			misc_msgs::CourseKeepingFA data;
			data.course = course;
			data.heading = heading;
			data.speed = speed;

			serializePrimitive(course_keeping_FA, labust::utilities::serializeMsg(data));
		}

		void MissionParser::courseKeepingUA(double course, double speed){

			misc_msgs::CourseKeepingUA data;
			data.course = course;
			data.speed = speed;

			serializePrimitive(course_keeping_UA, labust::utilities::serializeMsg(data));
		}

		void MissionParser::ISOprimitive(int dof, double command, double hysteresis, double reference, double sampling_rate){

			misc_msgs::ISO data;
			data.dof = dof;
			data.command = command;
			data.hysteresis = hysteresis;
			data.reference = reference;
			data.sampling_rate = sampling_rate;

			serializePrimitive(iso, labust::utilities::serializeMsg(data));
		}


		/* Function for parsing primitives in XML mission file */
		int MissionParser::parseMission(int id, string xmlFile){

		   XMLDocument xmlDoc;

		   XMLNode *mission;
		   XMLNode *primitive;
		   XMLNode *primitiveParam;

		   /* Open XML file */
		   if(xmlDoc.LoadFile(xmlFile.c_str()) == XML_SUCCESS) {

			   /* Find mission node */
			   mission = xmlDoc.FirstChildElement("main")->FirstChildElement("mission");
			   if(mission){

				   /* Loop through primitive nodes */
				   primitive = mission->FirstChildElement("primitive");
				   do{

					   XMLElement *elem = primitive->ToElement();
					   string primitiveName = elem->Attribute("name");
					   ROS_INFO("%s", primitiveName.c_str());

					   primitiveParam = primitive->FirstChildElement("id");
					   XMLElement *elemID = primitiveParam->ToElement();

					   /* If ID is correct process primitive data */
					   string id_string = static_cast<ostringstream*>( &(ostringstream() << id) )->str();
					   string tmp = elemID->GetText();

					   if (tmp.compare(id_string) == 0){

						   /* Reset data */
							newTimeout = 0;
							onEventNextActive.clear();
							onEventNext.clear();

							/* Initialize service call data */
							misc_msgs::EvaluateExpression evalExpr;


						   /* Case: go2point_FA *****************************/
						   if(primitiveName.compare("go2point_FA") == 0){

							   primitiveParam = primitive->FirstChildElement("param");
							   do{

								   XMLElement *elem2 = primitiveParam->ToElement();
								   string primitiveParamName = elem2->Attribute("name");

								   if(primitiveParamName.compare("north") == 0){

									   evalExpr.request.expression = elem2->GetText();
									   newXpos = (labust::utilities::callService(srvExprEval, evalExpr)).response.result;

								   } else if(primitiveParamName.compare("east") == 0){

									   evalExpr.request.expression = elem2->GetText();
									   newYpos = (labust::utilities::callService(srvExprEval, evalExpr)).response.result;


								   } else if(primitiveParamName.compare("speed") == 0){

									   evalExpr.request.expression = elem2->GetText();
									   newSpeed = (labust::utilities::callService(srvExprEval, evalExpr)).response.result;


								   } else if(primitiveParamName.compare("victory_radius") == 0){

									   evalExpr.request.expression = elem2->GetText();
									   newVictoryRadius = (labust::utilities::callService(srvExprEval, evalExpr)).response.result;

								   } else if(primitiveParamName.compare("heading") == 0){

									   evalExpr.request.expression = elem2->GetText();
									   newHeading = (labust::utilities::callService(srvExprEval, evalExpr)).response.result;

								   } else if(primitiveParamName.compare("onEventNext") == 0){

									   onEventNextParse(elem2);
								   }
							   } while(primitiveParam = primitiveParam->NextSiblingElement("param"));

							   return go2point_FA;

							/* Case: go2point_UA ****************************/
							} else if (primitiveName.compare("go2point_UA") == 0){

							   primitiveParam = primitive->FirstChildElement("param");
							   do{

								   XMLElement *elem2 = primitiveParam->ToElement();
								   string primitiveParamName = elem2->Attribute("name");

								   if(primitiveParamName.compare("north") == 0){

									   evalExpr.request.expression = elem2->GetText();
									   newXpos = (labust::utilities::callService(srvExprEval, evalExpr)).response.result;

								   } else if(primitiveParamName.compare("east") == 0){

									   evalExpr.request.expression = elem2->GetText();
									   newYpos = (labust::utilities::callService(srvExprEval, evalExpr)).response.result;

								   } else if(primitiveParamName.compare("speed") == 0){
						
									   evalExpr.request.expression = elem2->GetText();
									   newSpeed = (labust::utilities::callService(srvExprEval, evalExpr)).response.result;

								   } else if(primitiveParamName.compare("victory_radius") == 0){

									   evalExpr.request.expression = elem2->GetText();
									   newVictoryRadius = (labust::utilities::callService(srvExprEval, evalExpr)).response.result;

								   } else if(primitiveParamName.compare("onEventNext") == 0){

									   onEventNextParse(elem2);
								   }
							   } while(primitiveParam = primitiveParam->NextSiblingElement("param"));

							   return go2point_UA;

							/* Case: dynamic_positioning ********************/
							} else if (primitiveName.compare("dynamic_positioning") == 0){

							   primitiveParam = primitive->FirstChildElement("param");
							   do{

								   XMLElement *elem2 = primitiveParam->ToElement();
								   string primitiveParamName = elem2->Attribute("name");

								   if(primitiveParamName.compare("north") == 0){

									   evalExpr.request.expression = elem2->GetText();
									   newXpos = (labust::utilities::callService(srvExprEval, evalExpr)).response.result;

								   } else if(primitiveParamName.compare("east") == 0){

									   evalExpr.request.expression = elem2->GetText();
									   newYpos = (labust::utilities::callService(srvExprEval, evalExpr)).response.result;

								   } else if(primitiveParamName.compare("heading") == 0){

									   evalExpr.request.expression = elem2->GetText();
									   newHeading = (labust::utilities::callService(srvExprEval, evalExpr)).response.result;

								   } else if(primitiveParamName.compare("timeout") == 0){

									   evalExpr.request.expression = elem2->GetText();
									   newTimeout = (labust::utilities::callService(srvExprEval, evalExpr)).response.result;

								   } else if(primitiveParamName.compare("onEventNext") == 0){

									   onEventNextParse(elem2);
								   }
							   } while(primitiveParam = primitiveParam->NextSiblingElement("param"));

								return dynamic_positioning;

							/* Case: course_keeping_FA **********************/
							}else if (primitiveName.compare("course_keeping_FA") == 0){

							   primitiveParam = primitive->FirstChildElement("param");
							   do{

								   XMLElement *elem2 = primitiveParam->ToElement();
								   string primitiveParamName = elem2->Attribute("name");

								   if(primitiveParamName.compare("course") == 0){

									   evalExpr.request.expression = elem2->GetText();
									   newCourse = (labust::utilities::callService(srvExprEval, evalExpr)).response.result;

								   } else if(primitiveParamName.compare("speed") == 0){

									   evalExpr.request.expression = elem2->GetText();
									   newSpeed = (labust::utilities::callService(srvExprEval, evalExpr)).response.result;

								   } else if(primitiveParamName.compare("heading") == 0){

									   evalExpr.request.expression = elem2->GetText();
									   newHeading = (labust::utilities::callService(srvExprEval, evalExpr)).response.result;

								   } else if(primitiveParamName.compare("timeout") == 0){

									   evalExpr.request.expression = elem2->GetText();
									   newTimeout = (labust::utilities::callService(srvExprEval, evalExpr)).response.result;

								   } else if(primitiveParamName.compare("onEventNext") == 0){

									   onEventNextParse(elem2);
								   }
							   } while(primitiveParam = primitiveParam->NextSiblingElement("param"));

							   return course_keeping_FA;

							/* Case: course_keeping_UA **********************/
							}else if (primitiveName.compare("course_keeping_UA") == 0){

							   primitiveParam = primitive->FirstChildElement("param");
							   do{

								   XMLElement *elem2 = primitiveParam->ToElement();
								   string primitiveParamName = elem2->Attribute("name");

								   if(primitiveParamName.compare("course") == 0){

									   evalExpr.request.expression = elem2->GetText();
									   newCourse = (labust::utilities::callService(srvExprEval, evalExpr)).response.result;

								   } else if(primitiveParamName.compare("speed") == 0){

									   evalExpr.request.expression = elem2->GetText();
									   newSpeed = (labust::utilities::callService(srvExprEval, evalExpr)).response.result;

								   } else if(primitiveParamName.compare("timeout") == 0){

									   evalExpr.request.expression = elem2->GetText();
									   newTimeout = (labust::utilities::callService(srvExprEval, evalExpr)).response.result;

								   } else if(primitiveParamName.compare("onEventNext") == 0){

									   onEventNextParse(elem2);
								   }

							   } while(primitiveParam = primitiveParam->NextSiblingElement("param"));

							   return course_keeping_UA;

								/* Case: ISO ************************/
							}else if (primitiveName.compare("iso") == 0){

							   primitiveParam = primitive->FirstChildElement("param");
							   do{

								   XMLElement *elem2 = primitiveParam->ToElement();
								   string primitiveParamName = elem2->Attribute("name");

								   if(primitiveParamName.compare("dof") == 0){

									   evalExpr.request.expression = elem2->GetText();
									   newDOF = int((labust::utilities::callService(srvExprEval, evalExpr)).response.result);

								   } else if(primitiveParamName.compare("command") == 0){

									   evalExpr.request.expression = elem2->GetText();
									   newCommand = (labust::utilities::callService(srvExprEval, evalExpr)).response.result;

								   } else if(primitiveParamName.compare("hysteresis") == 0){

									   evalExpr.request.expression = elem2->GetText();
									   newHysteresis = (labust::utilities::callService(srvExprEval, evalExpr)).response.result;

								   } else if(primitiveParamName.compare("reference") == 0){

									   evalExpr.request.expression = elem2->GetText();
									   newReference = (labust::utilities::callService(srvExprEval, evalExpr)).response.result;

								   } else if(primitiveParamName.compare("sampling_rate") == 0){

									   evalExpr.request.expression = elem2->GetText();
									   newSamplingTime = (labust::utilities::callService(srvExprEval, evalExpr)).response.result;

								   } else if(primitiveParamName.compare("onEventNext") == 0){

									   onEventNextParse(elem2);
								   }

							   } while(primitiveParam = primitiveParam->NextSiblingElement("param"));

							   return iso;
						  }
					   }
				   } while(primitive = primitive->NextSiblingElement("primitive"));

				   return none;

			   } else {
				   ROS_ERROR("No mission defined");
				   return -1;
			   }
		   } else {
			   ROS_ERROR("Cannot open XML file!");
			   return -1;
		   }
		}

		void MissionParser::onEventNextParse(XMLElement *elem2){

			string primitiveNext = elem2->GetText();

			if(primitiveNext.empty()==0){
				if(strcmp(primitiveNext.c_str(),"bkp") == 0){
					onEventNext.push_back(breakpoint);
				}else{
					onEventNext.push_back(atoi(primitiveNext.c_str()));
				}
			} else {
					onEventNext.push_back(ID+1); // provjeri teba li ovo
			}

			onEventNextActive.push_back(atoi(elem2->Attribute("event")));
		}


		/*************************************************************
		 *** Initial parse of XML mission file
		 ************************************************************/

		/* Parse events on mission load */
		int MissionParser::parseEvents(string xmlFile){

		   XMLDocument xmlDoc;

		   XMLNode *events;
		   XMLNode *event;
		   XMLNode *primitiveParam;

		   /* Open XML file */
		   if(xmlDoc.LoadFile(xmlFile.c_str()) == XML_SUCCESS) {

			   /* Find events node */
			   events = xmlDoc.FirstChildElement("main")->FirstChildElement("events");;
			   if(events){
				   for (event = events->FirstChildElement("event"); event != NULL; event = event->NextSiblingElement()){

					   //eventsContainer.push_back(event->ToElement()->GetText());
					   missionEvents.append(event->ToElement()->GetText());
					   missionEvents.append(":");
				   }
				  // eventsFlag = true;
			   } else {
				   ROS_ERROR("No events defined");
				   return -1;
			   }
		   } else {
			   ROS_ERROR("Cannot open XML file!");
			   return -1;
		   }
		   return 0;
		}

		/* Parse mission parameters on mission load */
		int MissionParser::parseMissionParam(string xmlFile){

		   XMLDocument xmlDoc;

		   XMLNode *events;
		   XMLNode *event;
		   XMLNode *primitiveParam;

		   /* Open XML file */
		   if(xmlDoc.LoadFile(xmlFile.c_str()) == XML_SUCCESS) {

			   /* Find events node */
			   events = xmlDoc.FirstChildElement("main")->FirstChildElement("params");
			   if(events){
				   for (event = events->FirstChildElement("param"); event != NULL; event = event->NextSiblingElement()){

					   missionParams.append(event->ToElement()->Attribute("name"));
					   missionParams.append(":");
					   missionParams.append(event->ToElement()->GetText());
					   missionParams.append(":");
				   }
				 //  eventsFlag = true;
			   } else {
				   ROS_ERROR("No mission parameters defined");
				   return -1;
			   }
		   } else {
			   ROS_ERROR("Cannot open XML file!");
			   return -1;
		   }
		   return 0;
		}

		/*************************************************************
		 *** ROS subscriptions
		 ************************************************************/

		void MissionParser::onRequestPrimitive(const std_msgs::UInt16::ConstPtr& req){

			if(req->data){
				lastID = ID;
				ID = req->data;
				if(abs(ID-lastID)>1){
					breakpoint = lastID;
					ROS_ERROR("DEBUG: New breakpoint %d", breakpoint);
				}

				sendPrimitve();
			}
		}

		void MissionParser::onEventString(const std_msgs::String::ConstPtr& msg){

			if(strcmp(msg->data.c_str(),"/STOP") == 0){
				ID = 0;
				missionParams.clear();
				missionEvents.clear();
			}
		}

		void MissionParser::onReceiveXmlPath(const misc_msgs::StartParser::ConstPtr& msg){

			if(msg->fileName.empty() == 0){

				xmlFile.assign(msg->fileName.c_str());

				/* Set mission start offset */
				if(msg->relative){
					offset.north = -msg->startPosition.north;
					offset.east = -msg->startPosition.east;
				} else {
					offset.north = offset.east = 0;
				}

				/* On XML load parse mission parameters and mission events */
				parseEvents(xmlFile.c_str());
				parseMissionParam(xmlFile.c_str());

				/* Publish mission setup */
				misc_msgs::MissionSetup missionSetup;
				missionSetup.missionEvents = missionEvents;
				missionSetup.missionParams = missionParams;
				missionSetup.missionOffset = offset;
				pubMissionSetup.publish(missionSetup);
			}
		}


		/*****************************************************************
		 ***  Helper functions
		 ****************************************************************/

		//template <typename primitiveType>
		void MissionParser::serializePrimitive(int id, vector<uint8_t> serializedData){

			misc_msgs::SendPrimitive sendContainer;
			sendContainer.primitiveID = id;
			sendContainer.primitiveData = serializedData;
			sendContainer.event.timeout = newTimeout;
			sendContainer.event.onEventNextActive = onEventNextActive;
			sendContainer.event.onEventNext = onEventNext;

			pubSendPrimitive.publish(sendContainer);
		}
	}
}


/*********************************************************************
 ***  Main function
 ********************************************************************/

int main(int argc, char** argv){

	ros::init(argc, argv, "mission_parser");
	ros::NodeHandle nh;
	labust::mission::MissionParser MP(nh);
	ros::spin();
	return 0;
}






