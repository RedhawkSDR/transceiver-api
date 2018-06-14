
Title: "FEI Modifications"

# Table of Contents
| [Objective](#objective)
| [Design Goals](#design-goals)
| [Proposed Modifications](#proposed-modifications)
| [Gaps](#gaps)

| Proposed Modifications TOC|
|: |
| [Provide allocation_id or connection_id in StreamSRI](#provide-allocation_id-or-connection_id-in-streamsri) |
| [Transmit Status / Error Reporting](#transmit-status-error-reporting) |
| [Allocation for Multi-channel Transmit (or Receive)](#allocation-for-multi-channel-transmit-or-receive) |
| [Add referenceSettlingTime to frontend_tuner_status Definition](#add-referencesettlingtime-to-frontend_tuner_status-definition) |
| [Radio Control Using StreamSRI Keywords](#radio-control-using-streamsri-keywords) |
| [Extend `DigitalTuner` Interface to Support Transmit Queue](#extend-digitaltuner-interface-to-support-transmit-queue)   |

# Objective
This file is intended to capture a draft proposal for changes to the REDHAWK
FEI2.0 specification specifically for supporting transmit concept-of-operations
(CONOPs).

Initially, the content will be fairly free-form as the the scope of
changes is captured.  After attempting to identify the lion-share of required
changes, the document will be restructured to organize the changes by
`frontendInterfaces`, `bulkioInterfaces`, and _other_ interfaces.

# Design Goals
REDHAWK's `frontendInterfaces` and `bulkioInterfaces` (including
`burstioInterfaces`) are intended to allow a very loose, but effective coupling
of application code and the radio hardware that supports it.  Application code
should be able to make very high-level allocation requests of a REDHAWK system
and then pull data from or push data to the allocated radio hardware with no
additional integration programming.  This ensures a maximum portability of
application code across different REDHAWK sensor platforms.  At the same time,
application code that wishes to employ a tighter coupling with radio hardware in
order to effect some specific behavior or capability should be able to do that
as well.   The design suggestions below attempt to:

1.  Preserve the loosely-coupled pattern between application code and radio hardware.
2.  Maintain distinct separation between `frontendInterfaces` and
`bulkioInterfaces`; do not couple radio device interfaces into specialized data
conveyance APIs.

# Overall Questions for Core Framework

3.  Don't have a way to send RF characteristics @ the same time we send data.
    This becomes important when a block of data needs to be transmitted at specific
    time **and** using specific RF parameters.

    a.  One of the ways to handle this is with SRI Keywords that set all the same
    attributes of the radio that can normally be set with the Analog/DigitalTuner
    port.

    b.  Can a struct property be set as an SRI keyword?  It wasn't clear from
    looking at `bulkio_out_stream.h`.

4.  Does BURSTIO support the shared memory and Vita49 transports as well?

5.  What's the best way to create an equivalent to the UHD library's
    `async_metadata_t` interface?

    a. It looks like all `Components` (and therefore `Devices`) generate
       `Property Change Events`.

    b. Is it worth extending the base `Device` implementation to generate
       messages with a structure like `async_metadata_t` that can be used to
       advertise errors and exceptions of the device.


# Proposed Modifications

## Provide `allocation_id` or `connection_id` in StreamSRI
As previously discussed there has always been an association problem for devices
with multiple tuners that employ multi-in/out ports (which they all do).  In
order for the device to associate a connection with a tuner, the `allocation_id`
associated with the tuner must be provided via the `streamID`.  This has been a
challenge for many applications.  To alleviate this problem, it is recommended
that the `StreamSRI` definition be augmented to include the `allocation_id` (for
purposes of this discussion, `alloation_id` and `listener_allocation_id` are
synonymous).

---
**Concern**

No matter how this proposal is addressed, it will preclude the ability to have a
component not specifically developed for device interaction from using a device.
For the 'Simple Single Channel Transmit' case, it was hoped that a `usesdevice`
update to a waveforms SAD file would be sufficient to connect the component's
continuous output stream to a transmitter device.  The need to **inject** the
`resourceID` into the stream's SRI will preclude this.

If the default behavior of a legacy component is to push the `connectionId` into
the `streamID`, then devices could be required to provide a fall-back mode:
* if the `streamID` is equal to an `allocation_id`, then use it.
* if the `streamID` is **not** equal to an `allocation_id` search for the
  `resourceID`
* if neither of the previous is true disallow the connection and produce an error

---

### Proposal 1 : Modify the IDL Definition of `StreamSRI`
Modify the `struct StreamSRI` definition in
`bulkioInterfaces/idl/ossie/BULKIO/bulkioDataTypes.idl` to inlcude a field to
indicate the allocation_id associated with this stream.

Example:
```C++
struct StreamSRI {
        long hversion;
        double xstart;
        double xdelta;
        short xunits;
        long subsize;
        double ystart;
        double ydelta;
        short yunits;
        short mode;
        string streamID;
        string resourceID; /* NEW FIELD - allocation or listener id */
        boolean blocking;
        sequence<CF::DataType> keywords;
```

#### Pros : Modify the IDL Definition of `StreamSRI`
* No matter how this problem is addressed it will be a significant positive
  response to a challenge that has plagued the REDHAWK program since inception.

* Creates a clear enumeration of the capability to convey an `allocation_id` to
  the device.  This is a stronger API than is presented in Proposal 2, below.

#### Cons : Modify the IDL Definition of `StreamSRI`

* Introduces a _required_ keyword that is not relevant for all BULKIO conections that are **not** to devices.

### Proposal 2 : Insert a Well-Known Keyword in the `StreamSRI.keywords`
Modify the `struct StreamSRI` definition in
`bulkioInterfaces/idl/ossie/BULKIO/bulkioDataTypes.idl` to inlcude a field to
indicate the allocation_id associated with this stream.

Example:
```C++
// Create a new stream with ID "my_stream_id" and default SRI
bulkio::OutFloatStream stream = dataFloat_out->createStream("my_stream_id");

// Stream is complex data at a sample rate of 250Ksps, centered at 91.1MHz
stream.complex(true);
stream.xdelta(1.0 / 250000.0);
stream.setKeyword( "resourceID"), 'my_control_allocation_id');
```

#### Pros : Insert a Well-Known Keyword in the `StreamSRI.keywords`
* Does not require a modification to the bulkioInterfaces IDL.

* Does not put a field in the `StreamSRI` that is only relevant to a device.

* Can be left off for all stream operations that are not connected to devices.


#### Cons : Insert a Well-Known Keyword in the `StreamSRI.keywords`
* Not explicit.  The user must _know_ that this is a required behavior when
  performing connections to a device.

### Corollary Proposal : Add UsesPort IOR to StreamSRI as Required Field.
While the 'uses' port is always presented a reference to the 'provides' port
during the `connectPort` call.  This make it easy to discover a processing
flow-chain from data source to data sink.  The more natural discovery challenge
is from data sink to data source.  There is no easy way to traverse a data flow
from sink to source.  To alleviate this problem, the 'uses' port reference could
be a mandatory field in the `StreamSRI`.  As before, this could also be done
through a keyword, but keywords are implicitely optional.  A _required_ field
like this should be part of the IDL definition of the structure.

_**Note:** the `NegotiablePort` patten used for shared memory and Vita49
transport may be a better means of implementing this functionality.  An
application (component) developer isn't necessarily interested in the upstream
port reference and therefore shouldn't have to bothered with it.  Additionally,
it should not be presented in a way that allows modification by the user.  The
`NegotiablePort` construct seems to provide a means of conveying the information
and making it available via an API without any of the possible risks of using a
field in the `StreamSRI`, whether required structure member or keyword._

## Transmit Status / Error Reporting
A quick review of the BULKIO source will reveal that there are absolutely no
error responses and that most exceptions are swallowed after logging a,
hopefully informative, message.  Given the rest of the proposed design approach,
we are therfore denied a means of getting status on transmit operations, or any
other device interaction that results from a BULKIO transaction, without some
further mechanism.

No matter what approach is effected this modification will result in valuable
error responses that will help application developers and system developers
diagnose and resond to error conditions in the system.

### Proposal 1 : MessageEvent Port
REDHAWK already maintains a definition of a `MessageEvent` port which works both
as a `CF::Port` and a `CosEventChannelAdmin::EventChannel`.  The two
derived-from interfaces allow the port to send a message structure both
point-to-point (ie: between two ports) and to an EventService EventChannel.
A `MessageEvent` port would allow the definition of a status message as follows:

```xml
 <properties>
    <struct id="FRONTEND::DEVICE_STATUS_MESSAGE" name="async_message">
      <simple type="string" id="streamID"/>
      <simple type="utctime" id="timestamp"/>
      <simple type="short" id="event_code">
        <enumerations>
          <enumeration label="EVENT_CODE_BURST_ACK" value="0x1"/>
          <enumeration label="EVENT_CODE_UNDERFLOW" value="0x2"/>
          <enumeration label="EVENT_CODE_OVERFLOW" value="0x4"/>
          <enumeration label="EVENT_CODE_TIME_ERROR" value="0x8"/>
          <enumeration label="EVENT_CODE_SRI" value="0x10"/>
          <enumeration label="EVENT_CODE_CONNECTION_ERROR" value="0x20"/>
          <enumeration label="EVENT_CODE_CUSTOM" value="0x40"/>
        </enumerations>
      </simple>
      <simplesequence type="string" id="custom_code"/>
      <configurationkind kindtype="message"/>
    </struct>
</properties>
```

_**Note:** Some thought was given to adding the radio settings to this status
message, but that information is already being emmitted through the property
change events.  Does it need to be here too?_

There can be multiple status messages as well; for instance, one for each of the
enumerations above.  Just remember that with message definitions, they must be
copied into the PRF of every component that wants to send or receive them.  The
benefit of using multiple messages would be the ability to then provide
`event_code` specific paramaters to provide more detailed information.

#### Pros : Message Event Port
* The `MessageEvent` port implementation is already completed.

* The message definition is simple and requires only updates to XML files and
  therefore would not require a modification to the framework.

* Device status messages could be connected directly to an EventService event
  channel for consumption by any participant of the REDHAWK domain.

* Device status messages can go point-to-point; meaning that the particular
  component transmitting with the device could connect to get status updates on
  all transactions.

#### Cons : Message Event Port
* There is no way to _extend_ a message given the current implementation in the
  way that you could with an IDL defined structure like
  `PropertySetChangeEventType` (defined in `ExtendedEvent.idl`).  This means
  that any future updates to the structure would require a redefinition of the
  message.  While devices emitting status or components consuming status could
  be made to ignore any _additional_ fields that they didn't know about, the
  design is imprecise and dependent on a 'best practice' instead of an API.

* Since the device status messages are intended to respond to every stream or
  radio control event received by the device, the volume of these messages may
  be high.  The EventService in domains with multiple domains will be constantly
  enduring a load of events that may promote a higher failure rate.

### Proposal 2 : Custom IDL Device Status Port
This concept attempts to build on all the benefits of "Proposal 1 : Message
Event Port" while addressing the concern about having the message not be
extensible.  A port defined like the below would still have the benefits of
being able to be published to an EventChannel as well as point-to-point
communications.

```ruby
interface DeviceStatus : CosEventChannelAdmin::EventChannel, CF::Port {};
```

The implementation of the `DeviceStatus` interface would pass an IDL structure
similar to the above message definition.

```C++
enum EventCodeType {
   EVENT_CODE_BURST_ACK = Ox1,
   EVENT_CODE_UNDERFLOW = 0x2,
   EVENT_CODE_OVERFLOW  = 0x4,
   EVENT_CODE_TIME_ERROR = 0x8,
   EVENT_CODE_SRI = 0x10,
   EVENT_CODE_CONNECTION_ERROR = 0x20,
   EVENT_CODE_CUSTOM = 0x40
};

struct FrontendDeviceStatusEventType {
   string streamID;
   CF::UTCTime timestamp;
   EventCodeType event_code;
   CF::Properties custom_codes;
};

```
This structure can be extended with any additional properties in the same way
that `keywords` can be added to SRI in BULKIO.  As with Proposal 1, this IDL
structure can be broken up into discreet messages as well.  This is an
attractive idea to simplify custom codes.  A consumer can use the `event_code`
field to only parse event codes that were relevant to their processing.  Any
`EVENT_CODE_CUSTOM` could be ignored or processed only if they were of some
specific type.  For instance, assume a structure like that below

```C++
/* no need for EventCodeType enum */

struct FrontendDeviceStatusBurstACKEventType {
   string streamID;
   CF::UTCTime timestamp;
   /* other burst ack specific attributes possiible */
};

struct FrontendDeviceStatusUnderflowEventType {
  string streamID;
  CF::UTCTime timestamp;
  /* other underflow specific attributes possiible */
}

/* etc.... for each EventCodeType */

struct FrontendDeviceStatusCustomEventType {
  string streamID;
  CF::UTCTime timestamp;
  short CustomCodeId;
  CF:Properties CustomCodeProperties;
}
```

#### Pros : Custom IDL Device Status Port
* More structured definition of a Device Event message with API level
  enforcement of required fields.

* Extensible for device specific, custom status messages.

#### Cons : Custom IDL Device Status Port
* Custom port type that an application or system service would have to implement
* in order to make use of.

## Allocation for Multi-channel Transmit (or Receive)
There are several scenarios that suggest that a _multi-channnel_ allocation
should be supported

| **Scenario**             | **Description**                                                                                                                       |
|:------------------------ |:----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Tx Beamforming  | A transmit CONOP that requires the allocation of _N_ phase coherent channels. Data should be pushed to the group of channels with the exact same timing metadata and then phase and gain adjusted for each element by the hardware in order to affect a beam. Summary: 1 `bulkio` connection feeds _N_ transmit tuners.  A `DigitalTuner` cont |
| Rx Beamforming       | A receive CONOP that requires the allocation of _N_ coherent tuners that are able to receive data on the same frequency.  Summary: 1 allocation, _N_ `bulkio` channels each with time stamp and metadata information for the respective channels    |
| UHD-like  | The UHD library supports n-channel transmit.  We have been asked to create a Tx ability on par with the UHD. |

### Proposal 1 : Multi-Channel Allocation
It is trivial to extend the `frontend_tuner_allocation_struct` with an _overall_
allocation id and a sequence of standard `frontend_tuner_allocation_struct`
properties.  This would create a third type of allocation structure as proposed
below.

```C++
    typedef sequence <frontend_tuner_allocation_struct> ftas;

    struct frontend_array_allocation_struct {
        std::string allocation_id;
        ftas array_tuners;

        static std::string getId() { return std::string("FRONTEND::array_allocation"); }
    };
```

The `allocation_id` can be used by the normal `DigitalTuner` control port
methods to control the gain, centerfrequency, samplerate, bandwidth and
tolerances of the entire array.

Each `fta` (frontent_tuner_allocation_struct) should set the `device_control`
member to **true** and provide a   valid `allocation_id`.  Each member of the
array can use its `allocation_id` to get status and provide any custom
configuration of _array weights_ (or other element specific configuration).

There are really only two allocation use-cases for an array.
1.  Use the `frontend_array_allocation_struct` with `rf_flow_id` empty in each
of the `array_tuners`.  This should cause the device to allocate any two
**coherent** channels.  Note: there is no point in using this entry point i fyo
u don't want coherent channels; use the normal
`frontend_tuner_allocation_struct`.  The device should either return all
antennas in the available array or return a _contiguous_ subset of the array.
E.g. If an array has elements (1, 2, 3, 4), but the user allocations only 3
tuners, the device should return (1, 2, 3) or (2, 3, 4), not (1, 3, 4).

2.  Use the `frontend_array_allocation_struct` with the `rf_flow_id` of each
`array_tuners` member filled in to select the desired antenna/tuner.  In this
canse the device should return exactly the tuners requested if they are able to
be allocated.


## Add referenceSettlingTime to frontend_tuner_status Definition

In order to provide more insight into whether the device will be able to push
data at a particular time, it is recommended that the `frontend_tuner_status`
structure be extended with

```C++
double referenceSettlingTime;  // 0 if less than 1 microsecond else nearest
                               // microsecond
```
The current `default_frontend_status_struct` contains the following required
paramaters:
```C++
struct default_frontend_tuner_status_struct_struct {

        std::string tuner_type;
        std::string allocation_id_csv;
        double      center_frequency;
        double      bandwidth;
        double      sample_rate;
        std::string group_id;
        std::string rf_flow_id;
        bool        enabled;
    };
    static std::string getId() { return std::string("frontend_tuner_status_struct"); }
```

## Proposal 1 : Add referenceSettlingTime to `frontend_tuner_status_struct`
Define `referenceSettlingTime` as an attribute of a device that summarizes the
cumulative settling time required when a _retune_ event occurs (adjustments to
center frequency, sample rate, bandwidth)

Add this property to the `AnalogTuner` interface and then advertise it in the
`FRONTEND::tuner_status` structure.

```C++
struct default_frontend_tuner_status_struct_struct {

        std::string tuner_type;
        std::string allocation_id_csv;
        double      center_frequency;
        double      bandwidth;
        double      sample_rate;
        double      referenceSettlingTime; // NEW FIELD, units = microseconds.
        std::string group_id;
        std::string rf_flow_id;
        bool        enabled;
    };
    static std::string getId() { return std::string("frontend_tuner_status_struct"); }
```

The referenceSettlingTime should be set to '0' if the device doesn't support it.

_**Question**:  Does this not beg the question whether we should have a flag to
indicate whether all settling times have been accomplished and the devices PLLs
are _locked_?_

## Radio Control Using StreamSRI Keywords
There are two ways to control the _radio_ portion of an FEI device.  In this
case the _radio_ portion is defined as the following collection of settings for
each tuner:

  1. RF center frequency
  2. Bandwidth (both analog and digital)
  3. Sample rate
  4. Gain

_**Note**: there are other control facilities provided by the
`TunerControl.idl`, but these are generally set globally and never changed
during an applications use of a device.  Some of these controls include
'ReferenceSource' selection, 'Tuner Enable', and 'TunerAgcEnable', to name a
few_

The _radio_ may be configured using the `frontend_tuner_allocation_struct` with
an `allocateCapacity` call or the _radio_ may be configured using the individual
setter/getter methods defined by the `AnalogTuner` and `DigitalTuner`
interfaces.

There are several challenges in providing only these two methods of control.

  1. While 'allocation' is an acceptable way to configure the radio, it can be a
  _slow_ process and requires that the device first be deallocated.  It is
  possible that the device will not be available for further allocation in a
  resource competitive environment.  Then a search for an available device must
  occur.  This does not allow for a time-constrained use-case.
  2. While all _radio_ settings can be changed using the `AnalogTuner` and
  `DigitalTuner` interfaces, each change may incur additional settling time.
  Since the radio settings must be changed individually using these interfaces,
  the overrall time to update the radio may be at least 4 times the settling
  time.
  3. Stakeholders have reported scenarios with certain devices where the order
  in which the _radio_ settings are changed becomes important.  While the device
  should be expected to handle this type of ordering, it is not possible if the
  settings are not presented atomically.
  4. The current _radio_ control does not allow for the 'Time Multiplexed Single
  Channel' CONOP presented by AFRL.  There is no way to queue a _radio_ control
  profile.

### Proposal 1 : Radio Control Using frontend_tuner_allocation_struct as StreamSRI Keyword
Specify a defined behavior of all FEI compliant devices that they accept an
`frontend_tuner_allocation_struct` as a `StreamSRI` keyword to effect atomic
configuration of the radio's settings.

_**Concern**: the `StreamSRI` already has a hard requirement on the `xdelta` and REDHAWK already has a convention for `CHAN_RF` and `COL_RF`.  Perhaps we should just add keywords for the bandwidth and the gain?  This would affect the proposed IDL for QueuedTransmitter interface below._

#### Pros :

* Many stakeholders have asked for atomic configuration of the radio.

* REDHAWK has provided a similar capability through the `ScanningTuner` for the
  receive use-case.

* Allows for the 'Time Multiplexed Single Channel' CONOP presented by AFRL.  An
  application employing this CONOP would implement the following pseudo code.
  Suggested changes to the REDHAWK `Device` pattern and ways to status and
  manipulate a device's transmit transaction queue will be discussed in other
  sections of this document.

  ```C++
    queue <bulkio::OutFloatStream> transactionQ;

    for each burst to be enqueued

      // must have a unique streamID - this will serve as the transaction id
      string uniqueId = uniqueId_generator();

      // get and keep the stream
      transactionQ.push_back( port->createStream( uniqueId) );

      // set SRI for unique RF information
      transactionQ.back().xdelta( 1.0 / burst_specific_sample_rate );
      transactionQ.back().setKeyword( "FRONTEND::tuner_allocation", burst_specific_tuner_alloc_struct );

      // add data and the desired time of transmission
      transactionQ.back().write( some_buffer, time_to_transmit_burst );
  ```

#### Cons :
* As a non-IDL API pattern, there is little opportunity to validate and qualify the
formatting of the keyword and mandate a device's processing of it.

## Extend `DigitalTuner` Interface to Support Transmit Queue

```C++
    interface QueuedTransmitter : DigitalTuner
    {
        struct TunerActions {
            BULKIO::PrecisionUTCTime transmitTime;
            string streamID;                         // this should be the unique transaction id
            CF::Property tuner_alloc_struct;         // possibly change this to a discrete list
            CF::Properties additional_properties;
        };

        typedef sequence <TunerAction> TunerActions;

        // alternative to using 'tuner_alloc_struct'
        struct TunerConfig {
            double center_frequency;
            double sample_rate;
            double bandwidth;
            float  gain;
        };

        TunerActions getTunerActions( in string id );  // id = allocation_id

        void cancelTunerAction( in string id, in string transactionId );
    };
```

Internally, the device would keep a list of TunerActions for each allocated
tuner.  the `streamID` is the unique transaction id by which both the radio
settings and the data to be sent can be uniquely correlated and indexed.

All TunerActions returned from the queue have not yet been started.

A TunerAction is immediately removed from the queue when effected.

All radio configuration settings (like rf center frequency) would be subject to
the tolerance values specified at allocation time.

`additional_properties` can be used for either mode selection or transmit repitition commands.  These commands would be very device and implementation specific.

Status for each TunerAction will be conveyed through the transmit status port proposed [above](#transmit-status-error-reporting).

#### Pros :
* Provides a hardware queue for transmit operations that can support both
  transmit data as well as _mode_ selection using additional `StreamSRI`
  keywords.

* Prevents the need for `FrontendTuner` ports with strong data types.

* Very flexible - ultimately allows for any metadata to be pushed to a device through `StreamSRI` keywords and then leaves the interpretation of what to do with those keywords up to the device (this same statement should appear in the 'Cons' section as well).

#### Cons :
* It seems cumbersome to manage the TunerActions.  How realistic is it to believe that there is a real use-case for canceling a queued action?

* Makes heavy use of SRI keywords in order to continue using BULKIO rather than adding new `FrontendTuner` ports.  The rationale is based on the complexity and loss of portability caused by adding _data_ to the tuner ports.  Each port would then have to be overloaded for all supported data types and would become incompatible with other tuner ports of different data types.


# Gaps
## Transmit Buffer Management
When an application is transmitting data, there is an implied synchrony between
the software providing the data and the hardware actually transmitting the data.
There is an implied requirement for the software to provide all the data that
_needs to be sent_ in a timely fashion so that there are no gaps in the
transmitted waveform.  There is no facility in the provided REDHAWK interfaces
today for telling the hardware what the beginning of transmission and the end of
a transmission is.  As such, there is no way for the hardware to report "buffer
underrun" or "buffer overrun" type errors.

The Core Framework team recommends that the lifecycle of a _stream_ be used to
indicate when a data block starts and when it ends.  This requires a new stream
instance, and unique `streamID` for each burst.
