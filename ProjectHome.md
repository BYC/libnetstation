**The library allows developers to send messages to EGI's EEG recording software "NetStation". The library is not developed or supported by EGI.**

Last Update: Monday, September 21, 2009

Update Information:

Monday September 21, 2009

**Second Snow Leopard Update**
sizeof(systemSpec) was incorrect when compiled for X86\_64, and has now been fixed.

**Working With Snow Leopard**
Compiling for X86\_64 caused problems. sizeof(long) has changed, and the code now uses int and sizeof(int) in place of long and sizeof(long).


Wednesday June 17, 2009

Fixed a data underflow issue when sending triggers. **This seems to have resolved the compatibility issue when running NetStation on OS 10.5.**

Interface has changed to allow more accurate timings.

Net Station will time stamp events as (event.time - synch.time) + netStation.timeWhenSynchWasReceived.
As such, Net Station can not be more accurate than the amount of time it takes for the synch packet to be transmitted and received.   All subsequent events are relative to the synch time.


### Example: Synchronizing Stimulus To Triggers ###
```
  // Connecting:
  // Rather than deal with endianness issues on the experiment side, netstation lets you tell it what
  // type of machine you are using, and determines from the flag if it needs to deal with endianness differences.

  // We are running on a little endian processor (eg. Intel) 
  connection->connect("142.66.55.28", 55513);
  connection->sendBeginSession(NetStation::kLittleEndian);
  connection->sendBeginRecording();

  // Synch the clocks after connectiong
  connection->sendAttention(); 
  mBaseTime = GetTimeStampInMilliseconds();
  pConnection->sendSynch(0);

  // ... Application Logic ...

  // Sending Events To NetStation:
  // 1) Synch the clocks
  pConnection->sendAttention();
  timeStamp = GetTimeStampInMilliseconds();
  timeElapsed = timeStamp - mBaseTime;
  pConnection->sendSynch(timeElapsed);

  // 2) Present the stimulus
  stimulus->presentAtTime(timeElapsed + timeOffset);

  // 3) Send the stimulus event to netstation
  pConnection->sendTrigger("STIM", timeElapsed + timeOffset, 1);  	

  // ... Application Logic ...

  // Disconnecting:
  connection->sendEndSession();
  connection->sendEndRecording();
  connection->disconnect();
```

The EGIConnection::sendBeginSession method takes a flag that indicates the type of system you are currently running on. This information is used by NetStation to determine the endianness of incoming data. When using NetStation, send all data in its native endianness; NetStation will manage the endianness itself based on the sendBeginSession flag.

The flags are:
```
NetStation::kLittleEndian
NetStation::kBigEndian
```