# General Notes Regarding UHD Transmit functionality
The UHD separates control of the RF characteristics of a tuner from the data
*stream*.

## Overall Structure
The transmit ability of the UHD library is split into a C2 layer that is
implemented for each type of USRP device and _streamer_ interface which
implements an I/O mechanism to the card.

The UHD library supports C2 of the USRP device for the following functions:

1.  set/get_tx_rate() - outgoing sample rate
2.  set/get_tx_freq() - RF transmit frequency
3.  set/get_tx_gain() - lump gain setting for transmit chain
4.  set/get_tx_bandwidth() - analog transmit filter
5.  set/get_tx_antenna() - select the transmit antenna (RF bulkhead connector)

Defined in `include/uhd/stream.cpp`,  the _streamer_ API appears to be
partitioned into four facilities:

1.  the streamer configuration expressed by the `stream_args_t` structure when
creating the _streamer_,
2.  the _streamer_ itself, and
3.  a `tx_metadata_t` structure that conditions the data in _streamer_.
4.  an `async_metadata_t` struct that provides status of transmit operations.

Under the hood, the _stream_ is a command and control connection used by the UHD
library to communicate with a piece of hardware over USB, PCI, ethernet, or other
PHY / transport combination.  When the user requests a stream for purposes of
receiving or transmitting data, most of the control interfaces are opaque and
they only work with interfaces listed above.

## Streamer API
This section only discusses the 4 facilities of a UHD _stream_ listed above and
does not seek to address the hardware command and control interfaces.

### streamer_args_t
Configures a data path between the device and the user.  This facility is
primarily intended to provide automatic data conversion between the native
device format and the user desired/required format.

``` C++
struct UHD_API stream_args_t{

 //! Convenience constructor for streamer args
 stream_args_t(
     const std::string &cpu = "",
     const std::string &otw = ""
 ){
     cpu_format = cpu;
     otw_format = otw;
 }
 std::string cpu_format;
 std::string otw_format;
 device_addr_t args;
 std::vector<size_t> channels;
```

The `cpu` and `otw` attributes would normally be handled by the BULKIO port type
natively and do not bear further comment.  The `device_addr_t` attribute allows
key/value pairs that should affect the packaging or transport of data.  One of
the interesting use-cases that's mentioned in the UHD source documentation is

>underflow_policy:
>how the TX DSP should recover from underflow.
>Possible options are "next_burst" or "next_packet".
>In the "next_burst" mode, the DSP drops incoming packets until a new next_burst
>is started.  In the "next_packet" mode, the DSP starts transmitting again at
>the next packet.

There is no facility for this at the moment outside of a device's `CF::Property`
that the user could set.  Given other design goals, this functionality should be
on a per _stream_ basis and should become part of either the `stream` API in
REDHAWK or a well-known keyword that can be injected into the stream `SRI`.

The `channels` can be handled by associating a stream with an allocation and
then allowing allocation using a list instead of just a single allocation
structure.  This is discussed elsewhere under "_"The Multi-Channel Allocation_".

### streamer
The _streamer_ is most like the `bulkio` port.  It allows the a
block of data to be pushed to the device with a `tx_metadata_t` structure
and a `timeout`.

``` C++
class UHD_API tx_streamer : boost::noncopyable {
   virtual size_t get_num_channels(void) const = 0;
   virtual size_t get_max_num_samps(void) const = 0;
   typedef ref_vector<const void *> buffs_type;
   virtual size_t send( const buffs_type &buffs,
                        const size_t nsamps_per_buff,
                        const tx_metadata_t &metadata,
                        const double timeout = 0.1);

  virtual bool recv_async_msg(
        async_metadata_t &async_metadata, double timeout = 0.1
    ) = 0;
};
```

The `timeout` value indicates how long to wait before the data is sent.  This
seems redundant to me since the `tx_metadata_t` structure indicates when to send
the data.  I recommend discarding this from the proposed API.

### tx_metadata_t
The `tx_metadata_t` structure indicates whether the data should be sent
immediately (`has_time_spec = false`) or sent at the time provided by
`time_spec`.  It also indicates whether the buffer is a `start_of_burst` or
`end_of_burst`.

``` C++
    /*!
     * TX metadata structure for describing received IF data.
     * Includes time specification, and start and stop burst flags.
     * The send routines will convert the metadata to IF data headers.
     */
    struct UHD_API tx_metadata_t{
        /*!
         * Has time specification?
         * - Set false to send immediately.
         * - Set true to send at the time specified by time spec.
         */
        bool has_time_spec;

        //! When to send the first sample.
        time_spec_t time_spec;

        //! Set start of burst to true for the first packet in the chain.
        bool start_of_burst;

        //! Set end of burst to true for the last packet in the chain.
        bool end_of_burst;

        /*!
         * The default constructor:
         * Sets the fields to default values (flags set to false).
         */
        tx_metadata_t(void);
    };
```

If the `time_spec` is valid, then the UHD code will update it for every buffer
in a `send()` call.  The `start_of_burst` flag will be cleared after the first
send.  The `end_of_burst` flag allows the hardware to report on buffer
**underrun** conditions.  If a data stream has been started and more than a
sample period has passed without enough data to send **and** the `end_of_burst`
flag has not been set, there is a buffer underrun condition.  This buffer
underrun and buffer overrun logic is difficult to enable in REDHAWK and is
discussed in [fei_modifications.md](./fei_modifications) under _Gaps->Transmit
Buffer Management_.


### async_metadata_t
At the lowest level, the UHD driver assigns a sequence number to each packet
that it sends to a device and looks for an ACK for each sequence number.  If it
does not get one, the stream adds an `async_metadata_t` message to a queue, that
appears to be 1000 messages deep, to indicate what failure occurred.  The
message structure is show below.  We anticipate adding a MessageEvent port with
a message definition that is structured similar to the below and would be
unambiguously associated with either a BULKIO stream or some sort of unique
transation id.

``` C++
    /*!
     * Async metadata structure for describing transmit related events.
     */
    struct UHD_API async_metadata_t{
        //! The channel number in a mimo configuration
        size_t channel;

        //! Has time specification?
        bool has_time_spec;

        //! When the async event occurred.
        time_spec_t time_spec;

        /*!
         * The type of event for a receive async message call.
         */
        enum event_code_t {
            //! A burst was successfully transmitted.
            EVENT_CODE_BURST_ACK  = 0x1,
            //! An internal send buffer has emptied.
            EVENT_CODE_UNDERFLOW  = 0x2,
            //! Packet loss between host and device.
            EVENT_CODE_SEQ_ERROR  = 0x4,
            //! Packet had time that was late.
            EVENT_CODE_TIME_ERROR = 0x8,
            //! Underflow occurred inside a packet.
            EVENT_CODE_UNDERFLOW_IN_PACKET = 0x10,
            //! Packet loss within a burst.
            EVENT_CODE_SEQ_ERROR_IN_BURST  = 0x20,
            //! Some kind of custom user payload
            EVENT_CODE_USER_PAYLOAD = 0x40
        } event_code;

        /*!
         * A special payload populated by custom FPGA fabric.
         */
        uint32_t user_payload[4];

    };
```
