#include "myclass.h"
#include <QThread>
#include <qpushbutton.h>
#include <pugixml.hpp>

MyClass::MyClass(QWidget *parent)
	: QMainWindow(parent),
	snapIndex(0)
{
	ui.setupUi(this);
	m_emHandler = new DR::EMHandler(this);
	m_emHandler->connect();
	m_leapHandler = new LeapHander();
	m_leapHandler->moveToThread(&leapThread);
	connect(this, &MyClass::deleteLeap, m_leapHandler, &QObject::deleteLater, Qt::QueuedConnection);
	leapThread.start();
	m_leapHandler->connect();
}

MyClass::~MyClass()
{
	emit deleteLeap();
	leapThread.wait();
}


void MyClass::takeSnapShoot(){
	std::cout << "snapShoot" << std::endl;
	pugi::xml_document doc;
	pugi::xml_node frameNode = doc.append_child("Leap");
	pugi::xml_node emNode = doc.append_child("EM");
	m_leapHandler->frame(frameNode);
	m_emHandler->getDynamicRecording(emNode);
	std::stringstream sstm;
	sstm << "snap_" << snapIndex << ".xml";
	doc.save_file(sstm.str().c_str());
	snapIndex++;
}
