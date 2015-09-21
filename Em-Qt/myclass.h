#ifndef MYCLASS_H
#define MYCLASS_H

#include <QtWidgets/QMainWindow>
#include "ui_myclass.h"
#include "EM\emhandler.h"
#include "Leap\leaphander.h"
#include <QThread>

class MyClass : public QMainWindow
{
	Q_OBJECT
public:
	MyClass(QWidget *parent = 0);
	~MyClass();

public slots:
	void takeSnapShoot();

signals:
	void deleteLeap();

private:
	DR::EMHandler* m_emHandler;
	LeapHander* m_leapHandler;
	Ui::MyClassClass ui;
	QThread leapThread;
	int snapIndex;
};

#endif // MYCLASS_H
