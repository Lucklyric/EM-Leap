#include "leaphander.h"
#include <QThread>
const std::string fingerNames[] = { "Thumb", "Index", "Middle", "Ring", "Pinky" };
const std::string boneNames[] = { "Metacarpal", "Proximal", "Middle", "Distal" };
SampleListener::SampleListener(){
	is_connected = false;
}

void SampleListener::onInit(const Controller& controller) {
	std::cout << "[Leap]:Initialized" << std::endl;
}

void SampleListener::onConnect(const Controller& controller) {
	std::cout << "[Leap]:Connected" << std::endl;
	controller.enableGesture(Gesture::TYPE_CIRCLE);
	controller.enableGesture(Gesture::TYPE_KEY_TAP);
	controller.enableGesture(Gesture::TYPE_SCREEN_TAP);
	controller.enableGesture(Gesture::TYPE_SWIPE);
}

void SampleListener::onDisconnect(const Controller& controller) {
	// Note: not dispatched when running in a debugger.
	std::cout << "[Leap]:Disconnected" << std::endl;
	m_lock.lock();
	is_connected = true;
	m_lock.unlock();
}

void SampleListener::onExit(const Controller& controller) {
	std::cout << "[Leap]:Exited" << std::endl;
}

void SampleListener::onFrame(const Controller& controller) {
	m_lock.lock();
	//std::cout << "updateFrame" << std::endl;
	m_lastFrame = controller.frame();
	m_lock.unlock();
}
void SampleListener::onServiceConnect(const Controller& controller) {
	std::cout << "[Leap]:Service Connected" << std::endl;
}

void SampleListener::onServiceDisconnect(const Controller& controller) {
	std::cout << "[Leap]:Service Disconnected" << std::endl;
}

LeapHander::LeapHander(QObject *parent)
	: QObject(parent)
{
	
}


LeapHander::~LeapHander()
{
	m_controller.removeListener(m_sampleListener);
	this->thread()->quit();
}

void LeapHander::connect() {
	m_controller.addListener(m_sampleListener);
	m_controller.setPolicy(Leap::Controller::POLICY_BACKGROUND_FRAMES);
}

void LeapHander::positionToXml(pugi::xml_node &node,Leap::Vector position) {
	node.append_attribute("X").set_value(position.x);
	node.append_attribute("Y").set_value(position.y);
	node.append_attribute("Z").set_value(position.z);
}

void LeapHander::rotationToXml(pugi::xml_node &node, Leap::Matrix rotation) {
	float m00, m01, m02, m10, m11, m12, m20, m21, m22;
	m00 = rotation.xBasis.x; m01 = rotation.yBasis.x; m02 = rotation.zBasis.x;
	m10 = rotation.xBasis.y; m11 = rotation.yBasis.y; m12 = rotation.zBasis.y;
	m20 = rotation.xBasis.z; m21 = rotation.yBasis.z; m22 = rotation.zBasis.z;
	float w = sqrtf(1.0 + m00 + m11 + m22);
	double w4 = (4 * w);
	float x = (m21 - m12) / w4;
	float y = (m02 - m20) / w4;
	float z = (m10 - m01) / w4;
	node.append_attribute("W").set_value(w);
	node.append_attribute("X").set_value(x);
	node.append_attribute("Y").set_value(y);
	node.append_attribute("Z").set_value(z);
}

void LeapHander::frame(pugi::xml_node &frameNode){
	Leap::Frame currentFrame = m_sampleListener.frame();
	m_lock.lock();
	frameNode.append_attribute("id").set_value(currentFrame.id());
	pugi::xml_node handList = frameNode.append_child("hands");
	HandList hands = currentFrame.hands();
	
	for (HandList::const_iterator hl = hands.begin(); hl != hands.end(); ++hl) {
		// Get the first hand
		const Hand hand = *hl;
		pugi::xml_node handNode = handList.append_child("hand");
		handNode.append_attribute("id").set_value(hand.id());
		std::string handType;
		if (hand.isLeft) {
			handType = "Left";
		}
		else {
			handType = "Right";
		}
		handNode.append_attribute("type").set_value(handType.c_str());
		
		pugi::xml_node positionNode = handNode.append_child("position");
		positionToXml(positionNode, hand.palmPosition());
		
		pugi::xml_node normalNode = handNode.append_child("normal");
		positionToXml(normalNode, hand.palmNormal());

		pugi::xml_node directionNode = handNode.append_child("direction");
		positionToXml(directionNode, hand.direction());

		pugi::xml_node rotationNode = handNode.append_child("biasis");
		rotationToXml(rotationNode, hand.basis());
		//// Get fingers
		pugi::xml_node fingerList = handNode.append_child("fingers");
		const FingerList fingers = hand.fingers();
		for (FingerList::const_iterator fl = fingers.begin(); fl != fingers.end(); ++fl) {
			const Finger finger = *fl;
			pugi::xml_node fingerNode = fingerList.append_child("finger");
			fingerNode.append_attribute("id").set_value(finger.id());
			fingerNode.append_attribute("name").set_value(fingerNames[finger.type()].c_str());
			pugi::xml_node boneList = fingerNode.append_child("bones");
			

			// Get finger bones
			for (int b = 0; b < 4; ++b) {
				Bone::Type boneType = static_cast<Bone::Type>(b);
				Bone bone = finger.bone(boneType);
				pugi::xml_node boneNode = boneList.append_child("bone");
				boneNode.append_attribute("length").set_value(bone.length());
				boneNode.append_attribute("name").set_value(boneNames[boneType].c_str());

				pugi::xml_node prevJoint = boneNode.append_child("prevJoint");
				positionToXml(prevJoint, bone.prevJoint());

				pugi::xml_node nextJoint = boneNode.append_child("nextJoint");
				positionToXml(nextJoint, bone.nextJoint());

				pugi::xml_node rotation = boneNode.append_child("biasis");
				rotationToXml(rotation, bone.basis());
			}
		}
	}
	m_lock.unlock();
}
