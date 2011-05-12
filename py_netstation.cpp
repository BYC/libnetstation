#include <Python.h>
#include "netstation.h"

static NetStation::EGIConnection netstation;

extern "C"
{
	static PyObject* netstation_littleEndian(PyObject* self, PyObject* args)
	{
		return Py_BuildValue("s", NetStation::kLittleEndian);
	}
	
	static PyObject* netstation_bigEndian(PyObject* self, PyObject* args)
	{
		return Py_BuildValue("s", NetStation::kBigEndian);
	}

	static PyObject* netstation_connect(PyObject* self, PyObject* args)
	{
		char* address = 0;
		unsigned short port = 0;
		
		PyArg_ParseTuple(args, "sH", &address, &port);
		netstation.connect(address, port);
		
		return Py_BuildValue("");
	}
	
	static PyObject* netstation_disconnect(PyObject* self, PyObject* args)
	{
		netstation.disconnect();
		return Py_BuildValue("");		
	}
	
	static PyObject* netstation_sendBeginSession(PyObject* self, PyObject* args)
	{
		char endian[4] = {0};
		PyArg_ParseTuple(args, "cccc", &endian[0], &endian[1], &endian[2], &endian[3]);
		netstation.sendBeginSession(endian);
		
		return Py_BuildValue("");
	}
	
	static PyObject* netstation_sendEndSession(PyObject* self, PyObject* args)
	{
		netstation.sendEndSession();
		return Py_BuildValue("");
	}
	
	static PyObject* netstation_sendBeginRecording(PyObject* self, PyObject* args)
	{
		netstation.sendBeginRecording();
		return Py_BuildValue("");
	}
	
	static PyObject* netstation_sendEndRecording(PyObject* self, PyObject* args)
	{	
		netstation.sendEndRecording();
		return Py_BuildValue("");
	}
	
	static PyObject* netstation_sendAttention(PyObject* self, PyObject* args)
	{
		netstation.sendAttention();
		return Py_BuildValue("");
	}
	
	static PyObject* netstation_sendSynch(PyObject* self, PyObject* args)
	{
		int timeStamp = 0;
		
		PyArg_ParseTuple(args, "i", &timeStamp);
		netstation.sendSynch(timeStamp);
		
		return Py_BuildValue("");
	}
	
	static PyObject* netstation_sendTrigger(PyObject* self, PyObject* args)
	{
		char *code = 0;
		int timeStamp = 0;
		int duration = 0;
		
		PyArg_ParseTuple(args, "sii", &code, &timeStamp, &duration);
		netstation.sendTrigger(code, timeStamp, duration);
		
		return Py_BuildValue("");
	}
	
	static PyMethodDef netstation_methods[] = 
	{
		{"netstation_littleEndian",			netstation_littleEndian,		METH_VARARGS},
		{"netstation_bigEndian",			netstation_bigEndian,			METH_VARARGS},
		{"netstation_connect", 				netstation_connect, 			METH_VARARGS},
		{"netstation_disconnect",			netstation_disconnect, 			METH_VARARGS},
		{"netstation_sendBeginSession",		netstation_sendBeginSession, 	METH_VARARGS},
		{"netstation_sendEndSession", 		netstation_sendEndSession, 		METH_VARARGS},
		{"netstation_sendBeginRecording", 	netstation_sendBeginRecording,	METH_VARARGS},
		{"netstation_sendEndRecording", 	netstation_sendEndRecording,	METH_VARARGS},
		{"netstation_sendSynch",			netstation_sendSynch,			METH_VARARGS},
		{"netstation_sendTrigger",			netstation_sendTrigger,			METH_VARARGS},
		{NULL, NULL, NULL}
	};
	
	void initpy_netstation() 
	{
		Py_InitModule("py_netstation", netstation_methods);
	}
}