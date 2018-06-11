# General Notes Regarding UHD Transmit functionality

## Overall Structure
The transmit ability of the UHD library is split into a C2 layer that is implemented for each type of USRP device and _streamer_ interface which implements an I/O mechanism to the card.

The UHD library supports C2 of the USRP device for the following functions:

1.  set/get_tx_rate() - outgoing sample rate
2.  set/get_tx_freq() - RF transmit frequency
3.  set/get_tx_gain() - lump gain setting for transmit chain
4.  set/get_tx_bandwidth() - analog transmit filter
5.  set/get_tx_antenna() - select the transmit antenna (RF bulkhead connector)

The _streamer_ interface provides:

## Streamer API
Defined in `include/uhd/stream.cpp`.  The API appears to be partitioned into three sections:

1.  the streamer configuration expressed by the `stream_args_t` structure when creating the _streamer_,
2.  the _streamer_ itself, and
3.  what now?



### streamer_args_t
Configures a data path between the device and the user.  The `cpu` and `otw` attributes would normally be handled by the BULKIO port type natively and do not bear further comment.  The `device_addr_t` attribute allows key/value pairs that should affect the packaging or transport of data.  One of the interesting use-cases that's mentioned in the UHD source documentation is

>>>
   how the TX DSP should recover from underflow.  Possible options are "next_burst" or "next_packet".  In the "next_burst" mode, the DSP drops incoming packets until a new next_burst is started.  In the "next_packet" mode, the DSP starts transmitting again at the next packet."
>>>

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
