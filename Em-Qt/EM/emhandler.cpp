#include "emhandler.h"

#include <QDateTime>
#include <QThread>
#include <iostream>

#define FPS 5

static void* connectThread(void* arg)
{
	return ((DR::EMHandler*)arg)->thread_connect();
}

DR::EMHandler::EMHandler(QObject *parent) :
QObject(parent),
m_connected(false),
m_running(false)
{

}

DR::EMHandler::~EMHandler()
{
}

void* DR::EMHandler::thread_connect()
{
	int	errorCode;

	m_lock.lock();

	

	// Initialize the ATC3DG driver and DLL
	//
	// It is always necessary to first initialize the ATC3DG "system". By
	// "system" we mean the set of ATC3DG cards installed in the PC. All cards
	// will be initialized by a single call to InitializeBIRDSystem(). This
	// call will first invoke a hardware reset of each board. If at any time
	// during operation of the system an unrecoverable error occurs then the
	// first course of action should be to attempt to Recall InitializeBIRDSystem()
	// if this doesn't restore normal operating conditions there is probably a
	// permanent failure - contact tech support.
	// A call to InitializeBIRDSystem() does not return any information.
	//
	
	errorCode = InitializeBIRDSystem();
	if (errorCode != BIRD_ERROR_SUCCESS)
	{
		errorHandler(errorCode);
		m_lock.unlock();
		return 0;
	}

	// GET SYSTEM CONFIGURATION
	//
	// In order to get information about the system we have to make a call to
	// GetBIRDSystemConfiguration(). This call will fill a fixed size structure
	// containing amongst other things the number of boards detected and the
	// number of sensors and transmitters the system can support (Note: This
	// does not mean that all sensors and transmitters that can be supported
	// are physically attached)
	//

	errorCode = GetBIRDSystemConfiguration(&ATC3DG.m_config);
	if (errorCode != BIRD_ERROR_SUCCESS)
	{
		errorHandler(errorCode);
		m_lock.unlock();
		return 0;
	}

	// GET SENSOR CONFIGURATION
	//
	// Having determined how many sensors can be supported we can dynamically
	// allocate storage for the information about each sensor.
	// This information is acquired through a call to GetSensorConfiguration()
	// This call will fill a fixed size structure containing amongst other things
	// a status which indicates whether a physical sensor is attached to this
	// sensor port or not.
	//

	pSensor = new CSensor[ATC3DG.m_config.numberSensors];
	for (int i = 0; i < ATC3DG.m_config.numberSensors; i++)
	{
		errorCode = GetSensorConfiguration(i, &(pSensor + i)->m_config);
		if (errorCode != BIRD_ERROR_SUCCESS)
		{
			errorHandler(errorCode);
			m_lock.unlock();
			return 0;
		}
	}

	// GET TRANSMITTER CONFIGURATION
	//
	// The call to GetTransmitterConfiguration() performs a similar task to the
	// GetSensorConfiguration() call. It also returns a status in the filled
	// structure which indicates whether a transmitter is attached to this
	// port or not. In a single transmitter system it is only necessary to
	// find where that transmitter is in order to turn it on and use it.
	//
	
	pXmtr = new CXmtr[ATC3DG.m_config.numberTransmitters];
	for (int i = 0; i < ATC3DG.m_config.numberTransmitters; i++)
	{
		errorCode = GetTransmitterConfiguration(i, &(pXmtr + i)->m_config);
		if (errorCode != BIRD_ERROR_SUCCESS)
		{
			errorHandler(errorCode);
			m_lock.unlock();
			return 0;
		}
	}

	// Search for the first attached transmitter and turn it on
	//

	for (short id = 0; id < ATC3DG.m_config.numberTransmitters; id++)
	{
		if ((pXmtr + id)->m_config.attached)
		{
			// Transmitter selection is a system function.
			// Using the SELECT_TRANSMITTER parameter we send the id of the
			// transmitter that we want to run with the SetSystemParameter() call
			errorCode = SetSystemParameter(SELECT_TRANSMITTER, &id, sizeof(id));
			if (errorCode != BIRD_ERROR_SUCCESS)
			{
				errorHandler(errorCode);
				m_lock.unlock();
				return 0;
			}
			break;
		}
	}

	printf("Seting Sensor Parameter\n");
	DATA_FORMAT_TYPE format = DOUBLE_POSITION_QUATERNION;

	// scan the sensors and request a record
	for (int sensorID = 0; sensorID < ATC3DG.m_config.numberSensors; sensorID++)
	{
		errorCode = SetSensorParameter(
			sensorID,			// index number of target sensor
			DATA_FORMAT,		// command parameter type
			&format,			// address of data source buffer
			sizeof(format)		// size of source buffer
			);

		if (errorCode != BIRD_ERROR_SUCCESS) errorHandler(errorCode);
	}

	m_connected = true;

	if (m_connected) printf("[EM]:Connected\n");

	m_lock.unlock();

	return 0;
}

void DR::EMHandler::connect()
{
	int rc;
	printf("[EM]:Start Connect..\n");
	rc = pthread_create(&m_thread, NULL, connectThread, (void *)this);
	if (rc){
		printf("ERROR; return code from pthread_create() is %d\n", rc);
	}
}

void DR::EMHandler::disconnect()
{
	int errorCode;

	m_lock.lock();

	

	if (!m_connected) {
		m_lock.unlock();
		return;
	}

	m_running = false;

	pthread_join(m_thread, NULL);

	m_connected = false;

	// Turn off the transmitter before exiting
	// We turn off the transmitter by "selecting" a transmitter with an id of "-1"
	//
	int id = -1;
	errorCode = SetSystemParameter(SELECT_TRANSMITTER, &id, sizeof(id));
	if (errorCode != BIRD_ERROR_SUCCESS) errorHandler(errorCode);

	//  Free memory allocations before exiting
	
	delete[] pSensor;
	delete[] pXmtr;

	m_lock.unlock();
}

void DR::EMHandler::errorHandler(int error)
{
	char			buffer[1024];
	char			*pBuffer = &buffer[0];
	size_t			numberBytes;

	while (error != BIRD_ERROR_SUCCESS)
	{
		error = GetErrorText(error, pBuffer, sizeof(buffer), SIMPLE_MESSAGE);
		numberBytes = strlen(buffer);
		buffer[numberBytes] = '\n';		// append a newline to buffer
	}
}

QString DR::EMHandler::getRecordingName()
{
	return "trackSTAR";
}

bool DR::EMHandler::isRecordingStatic()
{
	return false;
}

bool DR::EMHandler::isRecordingDynamic()
{
	return true;
}



void DR::EMHandler::getDynamicRecording(pugi::xml_node &frame)
{
	int	errorCode;

	if (!m_connected)
		return;

	
	pugi::xml_node trackSTAR = frame.append_child("TrackSTAR");
	m_lock.lock();

	// Collect data from all birds
	// Loop through all sensors and get a data record if the sensor is attached.
	// Print result to screen
	// Note: The default data format is DOUBLE_POSITION_ANGLES. We can use this
	// format without first setting it.
	//
	//
	DOUBLE_POSITION_QUATERNION_RECORD record, *pRecord = &record;

	// scan the sensors and request a record
	for (int sensorID = 0; sensorID < ATC3DG.m_config.numberSensors; sensorID++)
	{
		// sensor attached so get record
		errorCode = GetAsynchronousRecord(sensorID, pRecord, sizeof(record));
		if (errorCode != BIRD_ERROR_SUCCESS) {
			errorHandler(errorCode);
			return;
		}

		// get the status of the last data record
		// only report the data if everything is okay
		unsigned int status = GetSensorStatus(sensorID);

		if (status == VALID_STATUS)
		{
			pugi::xml_node bird_node = trackSTAR.append_child("BIRD");

			pugi::xml_attribute id = bird_node.append_attribute("id");
			id.set_value(sensorID);

			pugi::xml_node position_node = bird_node.append_child();
			position_node.set_name("Position");
			position_node.append_attribute("X").set_value(record.x);
			position_node.append_attribute("Y").set_value(record.y);
			position_node.append_attribute("Z").set_value(record.z);

			pugi::xml_node rotation_node = bird_node.append_child();
			rotation_node.set_name("Rotation");
			rotation_node.append_attribute("W").set_value(record.q[0]);
			rotation_node.append_attribute("X").set_value(record.q[1]);
			rotation_node.append_attribute("Y").set_value(record.q[2]);
			rotation_node.append_attribute("Z").set_value(record.q[3]);
		}
	}
	m_lock.unlock();
}