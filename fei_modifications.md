# Objective
This file is intended to capture a draft proposal for changes to the REDHAWK FEI2.0 specification specifically for supporting transmit concept-of-operations (CONOPs).
Initially, the content will be fairly free-form as the the scope of changes is captured.  After attempting to identify the lion-share of required changes, the document will be restructured to organize the changes by `frontendInterfaces`, `bulkioInterfaces`, and _other_ interfaces.

# Design Goals
REDHAWK's `frontendInterfaces` and `bulkioInterfaces` (including `burstioInterfaces`) are intended to allow a very loose, but effective coupling of application code and the radio hardware that supports it.  Application code should be able to make very high-level allocation requests of a REDHAWK system and then pull data from or push data to the allocated radio hardware with no additional integration programming.  This ensures a maximum portability of application code across different REDHAWK sensor platforms.  At the same time, application code that wishes to employ a tighter coupling with radio hardware in order to effect some specific behavior or capability should be able to do that as well.   The design suggestions below attempt to:

1.  Preserve the loosely-coupled pattern between application code and radio hardware.
2.  Maintain distinct separation between `frontendInterfaces` and `bulkioInterfaces`; do not couple radio device interfaces into specialized data conveyance APIs.


# Overall Questions for Core Framework

1. `referenceSettlingTime` : an attribute of a device that summarizes the
cumulative settling time required when a _retune_ even occurs (adjustments to
center frequency, sample rate, bandwidth)

    a.  Can we add this property to the `AnalogTuner` interface and then
    advertise it in the `FRONTEND::tuner_status` structure?
    b.  Should it just be '0' if the device doesn't support it? c.  Does this not beg the question whether we should have a flag to indicate whether all settling times have been accomplished and the devices PLLs are _locked_?

2.  `frontend_array_allocation_struct` - a new allocation structure for coherent Rx or Tx tuners.  See
    [here](#the-multi-channel-allocation).

3.  Don't have a way to send RF characteristics @ the same time we send data.
    This becomes important when a block of data needs to be transmitted at specific
    time **and** using specific RF parameters.

    a.  One of the ways to handle this is with SRI Keywords that set all the same
    attributes of the radio that can normally be set with the Analog/DigitalTuner
    port.

    b.  Can a struct property be set as an SRI keyword?  It wasn't clear from
    looking at `bulkio_out_stream.h`.

4.  Does BURSTIO support the shared memory and Vita49 transports as well?

5.  What's the best way to create an equivalent to the UHD library's `async_metadata_t` interface?
    a. It looks like all `Components` (and therefore `Devices`) generate `Property Change Events`.
    b. Is it worth extending the base `Device` implementation to generate messages with a structure like `async_metadata_t` that can be used to advertise errors and exceptions of the device.


# Concepts

##

# Allocation

## Multi-channel Transmit (or Receive)
There are several scenarios that suggest that a _multi-channnel_ allocation should be supported

| **Scenario**             | **Description**                                                                                                                       |
|:------------------------ |:----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Tx Beamforming       | A transmit CONOP that requires the allocation of _N_ phase coherent channels.  Data should be pushed to the group of channels with the exact same timing metadata and then phase and gain adjusted for each element by the hardware in order to affect a beam. Summary: 1 `bulkio` connection feeds _N_ transmit tuners.  A `DigitalTuner` cont |
| Rx Beamforming       | A receive CONOP that requires the allocation of _N_ coherent tuners that are able to receive data on the same frequency.  Summary: 1 allocation, _N_ `bulkio` channels each with time stamp and metadata information for the respective channels    |
| UHD-like  | The UHD library supports n-channel transmit.  We have been asked to create a Tx ability on par with the UHD. |

### The Multi-Channel Allocation
It is trivial to extend the `frontend_tuner_allocation_struct` with an _overall_ allocation id and a sequence of standard `frontend_tuner_allocation_struct` properties.  This would create a third type of allocation structure as proposed below.

```C++
    typedef sequence <frontend_tuner_allocation_struct> ftas;

    struct frontend_array_allocation_struct {
        std::string allocation_id;
        ftas array_tuners;

        static std::string getId() { return std::string("FRONTEND::array_allocation"); }
    };
```

The `allocation_id` can be used by the normal `DigitalTuner` control port methods to control the gain, centerfrequency, samplerate, bandwidth and tolerances of the entire array.

Each `fta` (frontent_tuner_allocation_struct) should set the `device_control` member to **true** and provide a   valid `allocation_id`.  Each member of the array can use its `allocation_id` to get status and provide any custom configuration of _array weights_ (or other element specific configuration).

#### Multi-Channel Allocation CONOPs.
There are really only two allocation use-cases for an array.
1.  Use the `frontend_array_allocation_struct` with `rf_flow_id` empty in each of the `array_tuners`.  This should cause the device to allocate any two **coherent** channels.  Note: there is no point in using this entry point i fyo u don't want coherent channels; use the normal `frontend_tuner_allocation_struct`.  The device should either return all antennas in the available array or return a _contiguous_ subset of the array. E.g. If an array has elements (1, 2, 3, 4), but the user allocations only 3 tuners, the device should return (1, 2, 3) or (2, 3, 4), not (1, 3, 4).

2.  Use the `frontend_array_allocation_struct` with the `rf_flow_id` of each `array_tuners` member filled in to select the desired antenna/tuner.  In this canse the device should return exactly the tuners requested if they are able to be allocated.


## Extensions to the frontend_status_struct
In order to provide more insight into whether the device will be able to push
data at a particular time, it is recommended that the `frontend_tuner_status`
structure be extended with

```C++
double referenceSettlingTime;  // 0 if less than 1 microsecond else nearest
                               // microsecond
```
The current `default_frontend_status_struct` contains the following required paramaters:
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
