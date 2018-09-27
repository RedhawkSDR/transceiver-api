---
title: "REDHAWK API Review"
weight: 30
---
# Background on REDHAWK API Design
REDHAWK is designed to support command and control in two different ways.  The
different methods are intended to provide for two unique CONOPs that are worth
reviewing.  The **first CONOP** is for an application that requires no
particular or _specialized_ use of radio hardware; the application simply wants
to receive pre-d or transmit modulated data from any available radio hardware.
This application is only loosely coupled with radio hardware and need only use
the REDHAWK `Device` interface, which defines an abstract device with
_capacities_ and BULKIO.  The **second CONOP** is an application that is _more
integrated_ with the radio hardware and wishes to make more specialized use of a
radio's features. The `AnalogTuner`, `DigitalTuner`, and `ScanningTuner`
interfaces of REDHAWK's FRONTEND module are for this purpose.  These
applications must still use the `allocateCapacity` interface like the first
CONOP, but now require an additional port to interface to the radio hardware.

The major benefit of this two CONOP approach is that an application with no
knowledge of radio hardware can be integrated into any REDHAWK system through
simple text modifications to it's SAD file.  No other integration effort is
required other than to add a `usesdevice` relationship with a
`frontend_tuner_allocation_struct` property (in the simplest case).

For the sake of developing a grammar to aid discussion of this topic we will
adopt the following:

* Loosely Integrated Radio Pattern - this is the "first CONOP" that
  requires an application to use only the `Device` and `BULKIO` interfaces.

* Integrated Radio Pattern - this is the "second CONOP" that requires an application
  to implement a FrontendTuner port to accomplish specific control of the radio
  device beyond just allocation and deallocation.

# Current FRONTEND Tuner Data Structures
## Allocation Structures
### Control Allocation
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

#### tuner_type
The `tuner_type` is a string instead of an enumerated type because although
there are a common set of types, shown below, it is possible for sensor
developer to create new types.  New types should continue to use FEI-compliant
interfaces and define the expected behavior of those interfaces when applied to
the custom tuner type.

| **Tuner Type**  | **Definition** |
| :---------------------------| :----------------------------|
| TX | "Although the exact functionality of the TX tuner is not yet defined, it is reserved for transmitter devices" |
| RX | "A simple receiver, or RX tuner, is an RF to IF conversion only." |
| RX_DIGITIZER | "An RX_DIGITIZER tuner is an RX device that also samples the analog data and provides it as a digitized stream." |
| CHANNELIZER | "A CHANNELIZER tuner takes a digital wideband input and provides tuned, filtered, and decimated narrowband output. Allocating a CHANNELIZER establishes control over the input to a CHANNELIZER and allows users to understand that they have the ability to attach a new stream to the wideband input. Typical operation is to allocate a CHANNELIZER to gain control over the input prior to connecting a data stream to the input, though the order is not mandatory." |
| DDC | "DDC tuners provide a narrowband output from an existing channelizer capability. These narrowband channels are typically selectable in terms of the center frequency and bandwidth/sample rate within the constraints of the wideband input to the channelizer." |
| RX_DIGITIZER_CHANNELIZER | "The RX_DIGITIZER_CHANNELIZER tuner is a combination of an RX_DIGITIZER and CHANNELIZER capability into a single tuner type. Input is through an analog-RF input port, and output is through DDC tuners." |
| RX_SCANNER_DIGITIZER | An RX_DIGITIZER tuner with enhanced scanning control. |

#### allocation_id
The `allocation_id` acts as a *handle* for the allocated tuner.  All
`AnalogTuner`, `DigitalTuner`, and `ScanningTuner` methods will require this
handle to know which tuner the action is to be applied to and whether the caller
has *control* privileges to execute the action.

#### Optional Properties
The `frontend_tuner_allocation_struct` does not allow for optional properties.

### Listener Allocation
A *listener* allocation is used when no control over the tuner is required.
This is often the case when there is **system** level resource management.  A
privileged system service performs all the control allocations and then
individual processing waveforms get listener allocations.  Because a *listener*
is not able to control the tuner, the allocation structure is greatly
simplified.
```C++
  struct frontend_listener_allocation_struct {

        std::string existing_allocation_id;
        std::string listener_allocation_id;

        static std::string getId() { return std::string("FRONTEND::listener_allocation"); }
    };
```
The `existing_allocation_id` should be the controlling allocation id.  The
`listener_allocation_id` can then be handed to processing waveforms without fear
of their misusing the *system* level tuner resource.

_**Note :** listener allocations are one potential way to have multiple streams
be associated with a single tuner as in the case of multiple components trying
to push data to a single transmitter._

# Existing BULKIO

The existing BULKIO (and BURSTIO) APIs are identified and discussed in the
context of how they may or may not support transmit CONOPs.  We begin with a
discussion of the low level BULKIO interfaces that are implemented in IDL;
`pushPacket()` and `pushSRI()`.

The REDHAWK Core Framework team recommends that developer's not use the
low-level IDL interface but employ the _streamAPI_ instead.  This API consists
largely of the `InputStream` and `OutputStream`.  Since we are primarily
interested in transmit, we will only review the `OutputStream`.

## API : `pushPacket`
```ruby
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

### Observations : `pushPacket`

* The `PrecisionUTCTime` attibute allows the data to be _scheduled_ at a precise
  instant in time.

* A `PrecisionUTCTime` attribute equal to "0" should indicate that the data is
  to be sent immediately with no delay.  The timestamp is not relevant.

* The `EOS` flag is not really useful for the _start-of-burst_ and
  _end-of-burst_ functionality that would be required for a device to comprehend
  _buffer overrun_ and _buffer underrun_ because there is no way to push an EOS
  through the stream without just closing the stream.

* The `streamID` correlates directly to a `StreamSRI` supplied by a `pushSRI`
  call, which comes from the `updateSRI` inherited interface of the port.

## API : `updateSRI`
```ruby
    interface updateSRI {
        // List of all active streamSRIs (that have not been ended)
        readonly attribute StreamSRISequence activeSRIs;

        void pushSRI(in StreamSRI H);
    };
```

### Observations : `updateSRI`
* The `StreamSRI` contains a structure of information about the data to be
  pushed through the `pushPacket` interface.


## API : `StreamSRI`
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

### Observations : `StreamSRI`
  * `StreamSRI` can be extended with keywords.  Some keywords can be made part of
    a specification in order to facilitate radio device control or other logic
    relevant to the implementation of an FEI transmit API.


## BULKIO Error Codes and Exceptions
There are no BULKIO error codes and excpetions.  They all get swallowed by the
core framework.  While the code block below is demonstrating how the
`pushPacket()` call works, `stream.write()` uses the same method and suffers the
same lack of results response.

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

## API : `OutputStream`

_Streams_ are ultimately an encapsulation of the `pushPacket` and `pushSRI` APIs
that enforces behavior for simplified development.  There are some important
concepts associated with streams that are overviewed below.

* Each stream is associated with a `streamID`.  The `streamID` uniquely
  identifies the SRI and the data.

* In most REDHAWK systems, the `streamID` must be equal to the `allocation_id`
  or `listener_allocation_id` used during allocation so that the device knows to
  associate a stream with a tuner.

* A stream is expected to be a contiguous block of data.  This will have
  implications on the proposed approach to providing a transmit API since the
  REDHAWK program recommends managing each _burst_ or _block_ of data as a
  unique stream.

* A stream will only send SRI updates at the time a `stream.write()` is called.
  This means that a device will always receive any SRI updates just prior to the
  data that it describes.

* A stream will only send the `EOS` flag when it is closed.  Once an `EOS` is
  sent, the stream cannot be used an longer.

# Connection Logic
Historically, the `allocation_id` or `listener_allocation_id` is provided to the
component wishing to connect to a device during the connection process as the
`connectionId`.  This allows the 'uses' port to use the `connectionId` as the
'streamID' for its SRI.  In this way, when the component begins to send SRI and
data, the device is able to correlate the `streamID` to whichever tuner was
allocated for this data.  This is particular necessary for multi-input ports on
FEI devices.  Every component sending data to the device uses a single port.
Some mapping is required to direct data from a particular connection to the
correct tuner or tuners.

**This use-case will no longer work for the proposed transmit CONOPs and changes
will be proposed.**
