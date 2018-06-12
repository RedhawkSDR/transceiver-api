---
title: "REDHAWK API Review"
weight: 30
---
## FRONTEND Tuner Control APIs
REDHAWK is designed to support command and control in two different ways.  The
different methods are intended to provide for two unique CONOPs that are worth
reviewing.  The **first CONOP** is for an application that requires no
particular or specialized use of radio hardware; the application simply wants to
receive pre-d or transmit modulated data from any available radio hardware.
This application is only loosely coupled with radio hardware and need only use
the REDHAWK `Device` interface, which defines an abstract device with
_capacities_, and BULKIO.   **Caveat:** _the application must obviously know how
to format the tuner allocation  and status properties._  The **second CONOP** is
an application that is _more integrated_ with the radio hardware and wishes to
make more specialized use of a radio's features. The `AnalogTuner`,
`DigitalTuner`, and `ScanningTuner` interfaces of REDHAWK's FRONTEND module are
for this purpose.  These applications must still use the `allocateCapacity`
interface like the first CONOP, but now require an additional port to interface
to the radio hardware.

The major benefit of this approach is that an application with no knowledge of
radio hardware can be integrated into any REDHAWK system through simple text
modifications to it's SAD file.  No other integration effort is required other than
to add a `usesdevice` relationship with a `frontend_tuner_allocation_struct`
property (in the simplest case).

For the sake of developing a grammar to aid discussion of this topic we will
adopt the following:

* Loosely Integrated Radio Pattern - this is the "first CONOP" that
  requires an application to use only the `Device` and `BULKIO` interfaces.

* Integrated Radio Pattern - this is the "second CONOP" that requires an application
  to implement a FrontendTuner port to accomplish specific control of the radio
  device beyond just allocation and deallocation.

## Current FRONTEND Tuner Data Structures
### Allocation Structures
#### Control Allocation
The `frontend_tuner_allocation_struct` is shown below.
```C++
    struct frontend_tuner_allocation_struct {
        std::string tuner_type;
        std::string allocation_id;
        double      center_frequency;
        double      bandwidth;
        double      bandwidth_tolerance;
        double      sample_rate;
        double      sample_rate_tolerance;
        bool        device_control;
        std::string group_id;
        std::string rf_flow_id;

        static std::string getId() { return std::string("FRONTEND::tuner_allocation"); }
    };
```

##### tuner_type
The `tuner_type` is a string instead of an enumerated type because although there are a common set of types, shown below, it is possible for sensor developer to create new types.  New types should continue to use FEI-compliant interfaces and define the expected behavior of those interfaces when applied to the custom tuner type.

| **Tuner Type**  | **Definition** |
| :---------------------------| :----------------------------|
| TX | "Although the exact functionality of the TX tuner is not yet defined, it is reserved for transmitter devices" |
| RX | "A simple receiver, or RX tuner, is an RF to IF conversion only." |
| RX_DIGITIZER | "An RX_DIGITIZER tuner is an RX device that also samples the analog data and provides it as a digitized stream." |
| CHANNELIZER | "A CHANNELIZER tuner takes a digital wideband input and provides tuned, filtered, and decimated narrowband output. Allocating a CHANNELIZER establishes control over the input to a CHANNELIZER and allows users to understand that they have the ability to attach a new stream to the wideband input. Typical operation is to allocate a CHANNELIZER to gain control over the input prior to connecting a data stream to the input, though the order is not mandatory." |
| DDC | "DDC tuners provide a narrowband output from an existing channelizer capability. These narrowband channels are typically selectable in terms of the center frequency and bandwidth/sample rate within the constraints of the wideband input to the channelizer." |
| RX_DIGITIZER_CHANNELIZER | "The RX_DIGITIZER_CHANNELIZER tuner is a combination of an RX_DIGITIZER and CHANNELIZER capability into a single tuner type. Input is through an analog-RF input port, and output is through DDC tuners." |
| RX_SCANNER_DIGITIZER | An RX_DIGITIZER tuner with enhanced scanning control. |

##### allocation_id
The `allocation_id` acts as a *handle* for the allocated tuner.  All `AnalogTuner`, `DigitalTuner`, and `ScanningTuner` methods will require this handle to know which tuner the action is to be applied to and whether the caller has *control* privileges to execute the action.

##### Optional Properties
The `frontend_tuner_allocation_struct` does not allow for optional properties.

#### Listener Allocation
A *listener* allocation is used when no control over the tuner is required.  This is often the case when there is **system** level resource management.  A privileged system service performs all the control allocations and then individual processing waveforms get listener allocations.  Because a *listener* is not able to control the tuner, the allocation structure is greatly simplified.
```C++
  struct frontend_listener_allocation_struct {

        std::string existing_allocation_id;
        std::string listener_allocation_id;

        static std::string getId() { return std::string("FRONTEND::listener_allocation"); }
    };
```
The `existing_allocation_id` should be the controlling allocation id.  The `listener_allocation_id` can then be handed to processing waveforms without fear of their misusing the *system* level tuner resource.

## Existing BULKIO

The existing BULKIO (and BURSTIO) APIs are identified and discussed in the context
of how they may or may not support transmit CONOPs.

### API : `pushPacket`
```C++
    interface dataShort : ProvidesPortStatisticsProvider, updateSRI {
        void pushPacket(in PortTypes::ShortSequence data,
                        in PrecisionUTCTime T,
                        in boolean EOS,
                        in string streamID);
    };
```

The `pushPacket` functionality has been mapped onto a _stream_ since the REDHAWK
2.0 LTS.  Now, `streamID` is inherint to the _stream_ object that a user creates.
`EOS` is sent when `close()` is called on the stream.  The `write()` method of a
stream takes the data buffer to be sent and the timestamp to send the data at.

##### Observations : `pushPacket`
* The `PrecisionUTCTime` attibute allows the data to be _scheduled_ at a precise instant in time.
* The 'EOS' flag is not really useful for the _start-of-burst_ and _end-of-burst_ functionality that would be required for a device to comprehend _buffer overrun_ and _buffer underrun_ because there is no way to push an EOS through the stream without just closing the stream.
* The `streamID` correlates directly to a `StreamSRI` supplied by a `pushSRI` call, which comes from the `updateSRI` inherited interface of the port.

#### API : `updateSRI`
```C++
    interface updateSRI {
        // List of all active streamSRIs (that have not been ended)
        readonly attribute StreamSRISequence activeSRIs;

        void pushSRI(in StreamSRI H);
    };
```

#### Observations : `updateSRI`
* Th
* The `StreamSRI` contains a structure of data about the data to be pushed the `pushPacket` interface.


### API : `StreamSRI`
```C++
    struct StreamSRI {
        long hversion;    /* version of the StreamSRI header */
        double xstart;    /* start time of the stream */
        double xdelta;    /* delta between two samples */
        short xunits;     /* unit types from Platinum specification; common codes defined above */
        long subsize;     /* 0 if the data is one dimensional; > 0 if two dimensional */
        double ystart;    /* start of second dimension */
        double ydelta;    /* delta between two samples of second dimension */
        short yunits;     /* unit types from Platinum specification; common codes defined above */
        short mode;       /* 0-Scalar, 1-Complex */
        string streamID;  /* stream identifier */
        boolean blocking; /* flag to determine whether the receiving port should exhibit back pressure*/
        sequence<CF::DataType> keywords; /* user defined keywords */
    };

    typedef sequence<StreamSRI> StreamSRISequence;
```

#### Observations : `StreamSRI`


### BULKIO Error Codes and Exceptions
There are no BULKIO error codes and excpetions.  They all get swallowed by the core
framework.  How then does a user know when the `stream.write()` or `pushPacket() call
is working or not?

```C++
  void OutPort<PortType>::_sendPacket(
          const BufferType&               data,
          const BULKIO::PrecisionUTCTime& T,
          bool                            EOS,
          const std::string&              streamID)
  {
    // ... block of code folded


    if (active) {

            // ... block of code folded

            try {
                transport->pushSRI(streamID, stream.sri(), stream.modcount());
                transport->pushPacket(data, T, EOS, streamID, stream.sri());
            } catch (const redhawk::FatalTransportError& err) {
                LOG_ERROR(_portLog, "PUSH-PACKET FAILED " << err.what()
                          << " PORT/CONNECTION: " << name << "/" << connection_id);
                transport->setAlive(false);
            } catch (const redhawk::TransportError& err) {
                LOG_ERROR(_portLog, "pushPacket error on connection '" << connection_id << "': " << err.what());
            }
        }
    }
  }
```

# THIS IS OLD GARBAGE THAT NEEDS TO BE CLEANED UP
## TunerControl Extensions
```C++
   struct TxOperation {
     std::string existing_allocation_id;         // a tuner or group of tuners already allocated to this id.
     BULKIO::PrecisionUTCTime action_timestamp;  // when this action is required to occur
   }
```

#### action_timestamp
The `action_timestamp` is a high precision timestamp indicating when this TxOperation must be accomplished.  It is the devices responsibility to create an error code or exception if the action cannot be accomplished at the exact time specified in this

### TxOperations
A sequence of TxOperations that can be sent to a transmitter device.
* Can these actions be for different allocated tuners or tuner groups?
* Should the allocation id be pulled out into something more like
```C++
  schedule_operations( allocation_id, TX|RX, Operations)

  // Operations : TxOperations or RxOperations
```

# General Observations
## UHD Model
The UHD separates control of the RF characteristics of a tuner from the data *stream*.

```C++
    uhd::tx_metadata_t md;
    md.start_of_burst = false;
    md.end_of_burst = false;
    md.has_time_spec = true;
    md.time_spec = uhd::time_spec_t(seconds_in_future);

```
This brings up an interesting concept.  What if we created a tx_metadata property that could be pushed to a device using the *keywords* field in `BULKIO::StreamSRI`.  Any device that had a transmit conop could use either a property structure or a set of namespaced properties to communicate
