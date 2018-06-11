---
title: "Transceiver API"
weight: 30
---

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

## Multi-Channel Allocation


## BULKIO Extensions

### Existing BULKIO
```C++

```


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
