#ifndef LEAPHANDER_H
#define LEAPHANDER_H
#include <iostream>
#include <cstring>
#include <QObject>
#include <Leap.h>
#include <pugixml.hpp>
using namespace Leap;
class SampleListener : public Listener {
public:
	SampleListener();
	virtual void onInit(const Controller&);
	virtual void onConnect(const Controller&);
	virtual void onDisconnect(const Controller&);
	virtual void onExit(const Controller&);
	virtual void onFrame(const Controller&);
	virtual void onServiceConnect(const Controller&);
	virtual void onServiceDisconnect(const Controller&);
	Leap::Frame frame() { return m_lastFrame; }
	bool isConnected() { return is_connected; }
private:
	Leap::Frame m_lastFrame;
	QMutex m_lock;
	bool is_connected;
};

class LeapHander : public QObject
{
	Q_OBJECT

public:
	LeapHander(QObject *parent=0);
	~LeapHander();
	void frame(pugi::xml_node &frameNode);
	void connect();
	void positionToXml(pugi::xml_node &node, Leap::Vector position);
	void rotationToXml(pugi::xml_node &node, Leap::Matrix orientation);

private:
	SampleListener m_sampleListener;
	Controller m_controller;
	QMutex m_lock;


};

#endif // LEAPHANDER_H
