#ifndef EMHANDLER_H
#define EMHANDLER_H

#include <ATC3DG.h>
#include <QMutex>
#include <QObject>
#include <pthread.h>
#include <pugixml.hpp>

namespace DR {


	class EMHandler :public QObject
	{
		Q_OBJECT
	public:
		explicit EMHandler(QObject *parent = 0);
		virtual ~EMHandler();

		Q_INVOKABLE void connect();
		Q_INVOKABLE void disconnect();

		QString getRecordingName();

		bool isRecordingStatic();
		bool isRecordingDynamic();
		void getDynamicRecording(pugi::xml_node &frame);


		void* thread_connect();

	private:
		class CSystem
		{
		public:
			SYSTEM_CONFIGURATION	m_config;
		};

		class CSensor
		{
		public:
			SENSOR_CONFIGURATION	m_config;
		};

		class CXmtr
		{
		public:
			TRANSMITTER_CONFIGURATION	m_config;
		};
		pthread_t		m_thread;

		CSystem			ATC3DG;
		CSensor			*pSensor;
		CXmtr			*pXmtr;

		bool			m_running;
		bool			m_connected;

		QMutex			m_lock;

		void errorHandler(int error);
	};

}

#endif // EMHANDLER_H
