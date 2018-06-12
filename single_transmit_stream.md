# USRP Tranmist CONOP
The user wishes to transmit a single stream of data with the following
characteristics.

* The transmission must start at a specific precision time.
* The transmission should be centered at an RF.
* The transmission should be limited to a bandwidth.
* The data to be transmitted is sampled at a specific rate.

In this scenario, the user sets up a radio device, which can be conceptualized
most simply as a Digital Up Convertor (DUC) and then sends data to it that should
be transmitted at a specific time.

## Design Goals
This is the simplest transmit case and can be accomodated using the

## Overview of Order of Operations
1. Allocate a DUC on an FEI device.
2. Configure the RF center frequency, sample rate, bandwidth, gain (and possibly
phase delay) of the DUC.
3. Send data to be transmitted at an instant in time.  The instant may be _now_.
4. Recieve messages from the device if any error conditions occur.

## Current Capability
This use case can be partially accomplished using the existing API.  As such,
this document enumerates how a single stream of data can be transmitted using
the existing APIs.  In working through this simple case we identify deficiencies
of the existing API that must be addressed for more complex transmit CONOPs.

1. Allocate a TX tuner (_the conceptual DUC_) with the values you intend mapped to a
`frontend_tuner_allocation_struct`.  For the simplest transmit use case, this
should be sufficient configuration to be able to just send data with timestamps
to the tuner.

2. Create a *stream* to push data to the allocated tuner (as shown below).

   ```C++
   // Create a new stream with ID "my_stream_id" and default SRI
   bulkio::OutFloatStream stream = dataFloat_out->createStream("my_stream_id");

   // Stream is complex data at a sample rate of 250Ksps, centered at 91.1MHz
   stream.complex(true);
   stream.xdelta(1.0 / 250000.0);         // this sets the data sample rate
   stream.setKeyword("CHAN_RF", 91.1e6);  // this sets the transmit RF freq
   ```

3.  Use the stream to push data with precision timestamps to the tuner.  For
    transmit tuners, the timestamp in the `stream.write()` method will be
    interpreted as the time of transmission.  This leads to two scenarios following
    the initial `pushPacket` call.

    a. if the timestamp is zero, the device shall transmit the data and create an
       exception (don't know how we're propagating that yet) if the data was not
       delivered in time to make it contiguous with the previous `stream.write()`.
       This suggests a buffer **underrun** error.

    b. if the timestamp is non-zero, disable any buffer underrun logic, wait till
       the requested time to transmit the data.

4.  If data is pushed to the device **faster** than it can buffer and transmit the data
    a buffer **overrun** error should occur.  If the data is pushed to the device
    **slower** than is required to prevent a `xdelta` gap in the transmitted
    waveform a buffer **underrun** should occur.


## Notes

### Exceptions
A principal deficiency of this approach is the inability to provide status
responses and exceptions related to transmit operations.  There is no clean way
to provide exceptions that relate to FRONTEND operations without entangling the
BULKIO and FRONTEND APIs.  This is not desirable.  BULKIO has **no** defined
exceptions.

Some exception cases that pertain to transmit functions are:

* If the `pushPacket()` timestamp is either in the past or not able to be
accomplished by the hardware.

* If the sample rate of the data is not able to be accomplished by the hardware.

* If the user is not streaming data fast enough and then causes a buffer underrun.

* If the user is pushing data too quickly and causes a buffer overrun.


### Buffer Underrun / Overrun
The challenge here is that we have to keep the stream *full* of data (we don't
have a way to express buffer underrun in the hardware through a BULKIO API; nor
do we want to).  This means that we either have to update the  `pushPacket()`
timestamp for each successive call (painful) or we have to accept a zero-ized
timestamp as an indication that we just want the data pushed out and are
expecting it to be contiguous with the last `pushPacket` call.

 ### Data Sample Rate
The `stream.xdelta` should not *control* the output sample rate, but should only
indicate what the sample rate of the data stream is.  The actual DAC sample rate
**shall** be set by the initial tuner allocation call using the
`frontend_tuner_allocation_struct`.

Removing the above requirement allows the `pushSRI` and `pushPacket` calls to
*control* the transmit frequency, bandwidth, and sample rate of the data.  Each
change of the SRI, shown below for convenience, would still require a **new**
stream since we have no way of pushing and EOS.



The device must be
responsible for either resampling the data to
There are two conditions that are worth considering regarding the sample rate
specified by `xdelta`.

1.  The DAC (or some resampler) on the device may not be able to achieve the exact
 sample rate requested.  For this simple case, the user must request the desired
 sample rate during allocation and then check the device's achieved sample rate
 using the `frontend_tuner_status` structure.


### Stream API
There is no ability to *schedule* timed data streams on the device.  This defeats
the intentions of a developer that wishes to mux transmit operations onto a
single DUC by effectively time multiplexing them.


### Hardware Transmit Effect / Mode
There is no way to select a *hardware transmit mode* that is simply being
controlled through a FRONTEND interface.

### Multi-in Ports (Spectrum Synthesis)
There is no support for for multi-in ports.  This makes the spectrum synthesis
idea impossible for the time being.

There are no examples of devices that handle **multi-in** transmit.  The
USRP_UHD device, for instance, supports transmit, but it accepts any `pushSRI()`
call and as long as the transmit tuner is enabled, it will push the data to the
device.
