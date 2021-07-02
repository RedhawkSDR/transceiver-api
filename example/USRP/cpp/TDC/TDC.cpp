/**************************************************************************

    This is the device code. This file contains the child class where
    custom functionality can be added to the device. Custom
    functionality to the base class can be extended here. Access to
    the ports can also be done from this class

**************************************************************************/

#include "TDC.h"

using namespace TDC_ns;

PREPARE_LOGGING(TDC_i)

TDC_i::TDC_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl) :
    TDC_base(devMgr_ior, id, lbl, sftwrPrfl)
{
}

TDC_i::TDC_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, char *compDev) :
    TDC_base(devMgr_ior, id, lbl, sftwrPrfl, compDev)
{
}

TDC_i::TDC_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities) :
    TDC_base(devMgr_ior, id, lbl, sftwrPrfl, capacities)
{
}

TDC_i::TDC_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities, char *compDev) :
    TDC_base(devMgr_ior, id, lbl, sftwrPrfl, capacities, compDev)
{
}

TDC_i::~TDC_i()
{
}

void TDC_i::constructor()
{
    /***********************************************************************************
     This is the RH constructor. All properties are properly initialized before this function is called 

     For a tuner device, the structure frontend_tuner_status needs to match the number
     of tuners that this device controls and what kind of device it is.
     The options for devices are: TX, RX, RX_DIGITIZER, CHANNELIZER, DDC, RX_DIGITIZER_CHANNELIZER
     
     For example, if this device has 5 physical
     tuners, 3 RX_DIGITIZER and 2 CHANNELIZER, then the code in the construct function 
     should look like this:

     this->addChannels(3, "RX_DIGITIZER");
     this->addChannels(2, "CHANNELIZER");
     
     The incoming request for tuning contains a string describing the requested tuner
     type. The string for the request must match the string in the tuner status.
    ***********************************************************************************/
    this->addChannels(1, "TDC");
    this->setDataPort(dataShortTX_in->_this());
    this->setControlPort(TransmitControl_in->_this());

    bulkio::StreamQueue<bulkio::InShortPort>& queue = dataShortTX_in->getQueue();
    queue.update_ignore_error(true);
    queue.update_ignore_timestamp(true);

    _tuner_number = -1;
    _error_state = false;
    if (usrp_tuner.lock.cond == NULL)
        usrp_tuner.lock.cond = new boost::condition_variable;
    if (usrp_tuner.lock.mutex == NULL)
        usrp_tuner.lock.mutex = new boost::mutex;
}

void TDC_i::setTunerNumber(size_t tuner_number) {
    this->_tuner_number = tuner_number;
    this->updateDeviceCharacteristics();
}

void TDC_i::setUHDptr(const uhd::usrp::multi_usrp::sptr parent_device_ptr) {
    usrp_device_ptr = parent_device_ptr;
    this->updateDeviceCharacteristics();
}

void TDC_i::updateDeviceCharacteristics() {
    if ((usrp_device_ptr.get() != NULL) and (_tuner_number != -1))  {
        device_characteristics.tuner_type = "TDC";
        device_characteristics.ch_name = usrp_device_ptr->get_tx_subdev_name(_tuner_number);
        device_characteristics.antenna = usrp_device_ptr->get_tx_antenna(_tuner_number);
        device_characteristics.available_antennas = usrp_device_ptr->get_tx_antennas(_tuner_number);
        device_characteristics.freq_current = usrp_device_ptr->get_tx_freq(_tuner_number);
        device_characteristics.bandwidth_current = usrp_device_ptr->get_tx_bandwidth(_tuner_number);
        device_characteristics.rate_current = usrp_device_ptr->get_tx_rate(_tuner_number);

        usrp_range.bandwidth = usrp_device_ptr->get_tx_bandwidth_range(_tuner_number);
        usrp_range.sample_rate = usrp_device_ptr->get_tx_rates(_tuner_number);
        device_characteristics.gain_current = usrp_device_ptr->get_tx_gain(_tuner_number);
        usrp_range.gain = usrp_device_ptr->get_tx_gain_range(_tuner_number);
        usrp_range.frequency = usrp_device_ptr->get_tx_freq_range(_tuner_number);

        device_characteristics.bandwidth_min = usrp_range.bandwidth.start();
        device_characteristics.bandwidth_max = usrp_range.bandwidth.stop();
        device_characteristics.rate_min = usrp_range.sample_rate.start();
        device_characteristics.rate_max = usrp_range.sample_rate.stop();
        device_characteristics.gain_min = usrp_range.gain.start();
        device_characteristics.gain_max = usrp_range.gain.stop();
        device_characteristics.freq_min = usrp_range.frequency.start();
        device_characteristics.freq_max = usrp_range.frequency.stop();

        try {
            std::vector<double> rates = usrp_device_ptr->get_tx_dboard_iface(_tuner_number)->get_clock_rates(uhd::usrp::dboard_iface::UNIT_RX);
            device_characteristics.clock_min = rates.back();
            device_characteristics.clock_max = rates.front();
        } catch (...) {
            device_characteristics.clock_min = 0;
            device_characteristics.clock_max = 2*device_characteristics.rate_max;
        }
    }
}

bool TDC_i::usrpEnable()
{
    RH_TRACE(this->_baseLog,__PRETTY_FUNCTION__ << " tuner_number=" << _tuner_number );

    bool prev_enabled = frontend_tuner_status[0].enabled;
    frontend_tuner_status[0].enabled = true;

    //str2rfinfo_map_t::iterator it=rf_port_info_map.begin();
    /*for (; it!=rf_port_info_map.end(); it++) {
        if (it->second.tuner_idx == tuner_id && it->second.antenna == frontend_tuner_status[tuner_id].antenna) {
            break;
        }
    }
    if (it==rf_port_info_map.end()) {
        RH_ERROR(this->_baseLog,"usrpEnable|tuner_id=" << tuner_id << "No matching RFInfo port found!! Failed enable.");
        return false;
    }

    it->second.rfinfo_pkt.rf_center_freq = frontend_tuner_status[tuner_id].center_frequency;
    it->second.rfinfo_pkt.if_center_freq = frontend_tuner_status[tuner_id].center_frequency;
    it->second.rfinfo_pkt.rf_bandwidth = frontend_tuner_status[tuner_id].bandwidth;*/

    if(!prev_enabled){
        /*RH_DEBUG(this->_baseLog,"usrpEnable|tuner_id=" << tuner_id << "Sending updated rfinfo_pkt: port="<<it->first
                <<" rf_center_freq="<<it->second.rfinfo_pkt.rf_center_freq
                <<" if_center_freq="<<it->second.rfinfo_pkt.if_center_freq
                <<" bandwidth="<<it->second.rfinfo_pkt.rf_bandwidth);*/
        /*if (it->first == "RFInfoTX_out")
            RFInfoTX_out->rfinfo_pkt(it->second.rfinfo_pkt);
        else if (it->first == "RFInfoTX_out2")
            RFInfoTX_out2->rfinfo_pkt(it->second.rfinfo_pkt);*/
        usrp_tuner.update_sri = false;
    }

    if (usrp_tx_streamer.get() == NULL){
        usrpCreateTxStream(); // assume short for now since we don't know until data is received over a port
        RH_TRACE(this->_baseLog,"usrpEnable|tuner_number=" << _tuner_number << " got tx_streamer[" << _tuner_number << "]");
    }
    return true;
}

bool TDC_i::usrpCreateTxStream(){
    RH_TRACE(this->_baseLog,__PRETTY_FUNCTION__);
    //cleanup possible old one
    usrp_tx_streamer.reset();

    /*!
     * The CPU format is a string that describes the format of host memory.
     * Conversions for the following CPU formats have been implemented:
     *  - fc64 - complex<double>
     *  - fc32 - complex<float>
     *  - sc16 - complex<int16_t>
     *  - sc8 - complex<int8_t>
     */

    std::string cpu_format = "sc16";
    usrp_tx_streamer_typesize = sizeof(short);

    /*!
     * The OTW format is a string that describes the format over-the-wire.
     * The following over-the-wire formats have been implemented:
     *  - sc16 - Q16 I16
     *  - sc8 - Q8_1 I8_1 Q8_0 I8_0
     */
    std::string wire_format = "sc16";
    if(device_mode == "8bit")
        wire_format = "sc8"; // enable 8-bit mode with "sc8"
    RH_DEBUG(this->_baseLog,"usrpCreateTxStream|using wire_format" << wire_format);

    uhd::stream_args_t stream_args(cpu_format, wire_format);
    stream_args.channels.push_back(_tuner_number);
    stream_args.args["noclear"] = "1";
    usrp_tx_streamer = usrp_device_ptr->get_tx_stream(stream_args);
    return true;
}

/***********************************************************************************************

    Basic functionality:

        The service function is called by the serviceThread object (of type ProcessThread).
        This call happens immediately after the previous call if the return value for
        the previous call was NORMAL.
        If the return value for the previous call was NOOP, then the serviceThread waits
        an amount of time defined in the serviceThread's constructor.
        
    SRI:
        To create a StreamSRI object, use the following code:
                std::string stream_id = "testStream";
                BULKIO::StreamSRI sri = bulkio::sri::create(stream_id);

        To create a StreamSRI object based on tuner status structure index 'idx' and collector center frequency of 100:
                std::string stream_id = "my_stream_id";
                BULKIO::StreamSRI sri = this->create(stream_id, this->frontend_tuner_status[idx], 100);

    Time:
        To create a PrecisionUTCTime object, use the following code:
                BULKIO::PrecisionUTCTime tstamp = bulkio::time::utils::now();

        
    Ports:

        Data is passed to the serviceFunction through by reading from input streams
        (BulkIO only). The input stream class is a port-specific class, so each port
        implementing the BulkIO interface will have its own type-specific input stream.
        UDP multicast (dataSDDS and dataVITA49) ports do not support streams.

        The input stream from which to read can be requested with the getCurrentStream()
        method. The optional argument to getCurrentStream() is a floating point number that
        specifies the time to wait in seconds. A zero value is non-blocking. A negative value
        is blocking.  Constants have been defined for these values, bulkio::Const::BLOCKING and
        bulkio::Const::NON_BLOCKING.

        More advanced uses of input streams are possible; refer to the REDHAWK documentation
        for more details.

        Input streams return data blocks that automatically manage the memory for the data
        and include the SRI that was in effect at the time the data was received. It is not
        necessary to delete the block; it will be cleaned up when it goes out of scope.

        To send data using a BulkIO interface, create an output stream and write the
        data to it. When done with the output stream, the close() method sends and end-of-
        stream flag and cleans up.

        NOTE: If you have a BULKIO dataSDDS or dataVITA49  port, you must manually call 
              "port->updateStats()" to update the port statistics when appropriate.

        Example:
            // This example assumes that the device has two ports:
            //  An input (provides) port of type bulkio::InShortPort called dataShort_in
            //  An output (uses) port of type bulkio::OutFloatPort called dataFloat_out
            // The mapping between the port and the class is found
            // in the device base class header file

            bulkio::InShortStream inputStream = dataShort_in->getCurrentStream();
            if (!inputStream) { // No streams are available
                return NOOP;
            }

            // Get the output stream, creating it if it doesn't exist yet
            bulkio::OutFloatStream outputStream = dataFloat_out->getStream(inputStream.streamID());
            if (!outputStream) {
                outputStream = dataFloat_out->createStream(inputStream.sri());
            }

            bulkio::ShortDataBlock block = inputStream.read();
            if (!block) { // No data available
                // Propagate end-of-stream
                if (inputStream.eos()) {
                   outputStream.close();
                }
                return NOOP;
            }

            if (block.sriChanged()) {
                // Update output SRI
                outputStream.sri(block.sri());
            }

            // Get read-only access to the input data
            redhawk::shared_buffer<short> inputData = block.buffer();

            // Acquire a new buffer to hold the output data
            redhawk::buffer<float> outputData(inputData.size());

            // Transform input data into output data
            for (size_t index = 0; index < inputData.size(); ++index) {
                outputData[index] = (float) inputData[index];
            }

            // Write to the output stream; outputData must not be modified after
            // this method call
            outputStream.write(outputData, block.getStartTime());

            return NORMAL;

        If working with complex data (i.e., the "mode" on the SRI is set to
        true), the data block's complex() method will return true. Data blocks
        provide a cxbuffer() method that returns a complex interpretation of the
        buffer without making a copy:

            if (block.complex()) {
                redhawk::shared_buffer<std::complex<short> > inData = block.cxbuffer();
                redhawk::buffer<std::complex<float> > outData(inData.size());
                for (size_t index = 0; index < inData.size(); ++index) {
                    outData[index] = inData[index];
                }
                outputStream.write(outData, block.getStartTime());
            }

        Interactions with non-BULKIO ports are left up to the device developer's discretion
        
    Messages:
    
        To receive a message, you need (1) an input port of type MessageEvent, (2) a message prototype described
        as a structure property of kind message, (3) a callback to service the message, and (4) to register the callback
        with the input port.
        
        Assuming a property of type message is declared called "my_msg", an input port called "msg_input" is declared of
        type MessageEvent, create the following code:
        
        void TDC_i::my_message_callback(const std::string& id, const my_msg_struct &msg){
        }
        
        Register the message callback onto the input port with the following form:
        this->msg_input->registerMessage("my_msg", this, &TDC_i::my_message_callback);
        
        To send a message, you need to (1) create a message structure, (2) a message prototype described
        as a structure property of kind message, and (3) send the message over the port.
        
        Assuming a property of type message is declared called "my_msg", an output port called "msg_output" is declared of
        type MessageEvent, create the following code:
        
        ::my_msg_struct msg_out;
        this->msg_output->sendMessage(msg_out);

    Accessing the Device Manager and Domain Manager:
    
        Both the Device Manager hosting this Device and the Domain Manager hosting
        the Device Manager are available to the Device.
        
        To access the Domain Manager:
            CF::DomainManager_ptr dommgr = this->getDomainManager()->getRef();
        To access the Device Manager:
            CF::DeviceManager_ptr devmgr = this->getDeviceManager()->getRef();
    
    Properties:
        
        Properties are accessed directly as member variables. For example, if the
        property name is "baudRate", it may be accessed within member functions as
        "baudRate". Unnamed properties are given the property id as its name.
        Property types are mapped to the nearest C++ type, (e.g. "string" becomes
        "std::string"). All generated properties are declared in the base class
        (TDC_base).
    
        Simple sequence properties are mapped to "std::vector" of the simple type.
        Struct properties, if used, are mapped to C++ structs defined in the
        generated file "struct_props.h". Field names are taken from the name in
        the properties file; if no name is given, a generated name of the form
        "field_n" is used, where "n" is the ordinal number of the field.
        
        Example:
            // This example makes use of the following Properties:
            //  - A float value called scaleValue
            //  - A boolean called scaleInput
              
            if (scaleInput) {
                dataOut[i] = dataIn[i] * scaleValue;
            } else {
                dataOut[i] = dataIn[i];
            }
            
        Callback methods can be associated with a property so that the methods are
        called each time the property value changes.  This is done by calling 
        addPropertyListener(<property>, this, &TDC_i::<callback method>)
        in the constructor.

        The callback method receives two arguments, the old and new values, and
        should return nothing (void). The arguments can be passed by value,
        receiving a copy (preferred for primitive types), or by const reference
        (preferred for strings, structs and vectors).

        Example:
            // This example makes use of the following Properties:
            //  - A float value called scaleValue
            //  - A struct property called status
            
        //Add to TDC.cpp
        TDC_i::TDC_i(const char *uuid, const char *label) :
            TDC_base(uuid, label)
        {
            addPropertyListener(scaleValue, this, &TDC_i::scaleChanged);
            addPropertyListener(status, this, &TDC_i::statusChanged);
        }

        void TDC_i::scaleChanged(float oldValue, float newValue)
        {
            RH_DEBUG(this->_baseLog, "scaleValue changed from" << oldValue << " to " << newValue);
        }
            
        void TDC_i::statusChanged(const status_struct& oldValue, const status_struct& newValue)
        {
            RH_DEBUG(this->_baseLog, "status changed");
        }
            
        //Add to TDC.h
        void scaleChanged(float oldValue, float newValue);
        void statusChanged(const status_struct& oldValue, const status_struct& newValue);

    Logging:

        The member _baseLog is a logger whose base name is the component (or device) instance name.
        New logs should be created based on this logger name.

        To create a new logger,
            rh_logger::LoggerPtr my_logger = this->_baseLog->getChildLogger("foo");

        Assuming component instance name abc_1, my_logger will then be created with the 
        name "abc_1.user.foo".

    Allocation:
    
        Allocation callbacks are available to customize the Device's response to 
        allocation requests. For example, if the Device contains the allocation 
        property "my_alloc" of type string, the allocation and deallocation
        callbacks follow the pattern (with arbitrary function names
        my_alloc_fn and my_dealloc_fn):
        
        bool TDC_i::my_alloc_fn(const std::string &value)
        {
            // perform logic
            return true; // successful allocation
        }
        void TDC_i::my_dealloc_fn(const std::string &value)
        {
            // perform logic
        }
        
        The allocation and deallocation functions are then registered with the Device
        base class with the setAllocationImpl call. Note that the variable for the property is used rather
        than its id:
        
        this->setAllocationImpl(my_alloc, this, &TDC_i::my_alloc_fn, &TDC_i::my_dealloc_fn);
        
        

************************************************************************************************/
int TDC_i::serviceFunction()
{
    RH_DEBUG(this->_baseLog, "serviceFunction() example log message");

    usrpTransmit();

    return NORMAL;
}

void TDC_i::verifyHWStatus(const std::string &stream_id, const BULKIO::PrecisionUTCTime &rightnow) {
    if (usrp_tx_streamer.get() != NULL) {
        uhd::async_metadata_t metadata;
        bool got_msg = usrp_tx_streamer->recv_async_msg(metadata, 0.0);
        if (got_msg and (not _error_state)) {
            FRONTEND::TransmitStatusType status;
            status.stream_id = CORBA::string_dup(stream_id.c_str());
            status.allocation_id = CORBA::string_dup(_allocationTracker.begin()->first.c_str());
            status.timestamp = rightnow;
            status.total_samples = 0;
            status.total_packets = 0;
            status.transmitting = true;
            status.settling_time = 0;
            status.queued_packets = 0;
            if ((metadata.event_code == uhd::async_metadata_t::EVENT_CODE_UNDERFLOW) || 
                (metadata.event_code == uhd::async_metadata_t::EVENT_CODE_UNDERFLOW_IN_PACKET)) {
                status.status = CF::DEV_UNDERFLOW;
            } else if (metadata.event_code == uhd::async_metadata_t::EVENT_CODE_TIME_ERROR) {
                status.status = CF::DEV_MISSED_TRANSMIT_WINDOW;
            } else if ((metadata.event_code == uhd::async_metadata_t::EVENT_CODE_USER_PAYLOAD) ||
                (metadata.event_code == uhd::async_metadata_t::EVENT_CODE_SEQ_ERROR) ||
                (metadata.event_code == uhd::async_metadata_t::EVENT_CODE_SEQ_ERROR_IN_BURST) ||
                (metadata.event_code == uhd::async_metadata_t::EVENT_CODE_BURST_ACK)) {
                status.status = CF::DEV_HARDWARE_FAILURE;
            } else {
                status.status = CF::DEV_HARDWARE_FAILURE;
            }
            _error_state = true;
            this->TransmitDeviceStatus_out->transmitStatusChanged(status);
        }
    }
}

void TDC_i::verifyQueueStatus(const std::string &stream_id, const BULKIO::PrecisionUTCTime &rightnow, const std::vector<bulkio::StreamStatus> &error_status) {
    if ((error_status.size() != 0) and (not _error_state)) {
        FRONTEND::TransmitStatusType status;
        status.stream_id = CORBA::string_dup(stream_id.c_str());
        status.allocation_id = CORBA::string_dup(_allocationTracker.begin()->first.c_str());
        status.timestamp = rightnow;
        status.total_samples = 0;
        status.total_packets = 0;
        status.transmitting = true;
        status.settling_time = 0;
        status.queued_packets = 0;
        status.status = error_status[0].code;
        _error_state = true;
        this->TransmitDeviceStatus_out->transmitStatusChanged(status);
    }
}

bool TDC_i::usrpTransmit(){
    RH_TRACE(this->_baseLog,__PRETTY_FUNCTION__);

    bulkio::StreamQueue<bulkio::InShortPort>& queue = dataShortTX_in->getQueue();

    if(usrp_tuner.update_sri){

        /*str2rfinfo_map_t::iterator it=rf_port_info_map.begin();
        for (; it!=rf_port_info_map.end(); it++) {
            if (it->second.tuner_idx == tuner_id && it->second.antenna == frontend_tuner_status[tuner_id].antenna) {
                break;
            }
        }
        if (it==rf_port_info_map.end()) {
            LOG_ERROR(USRP_UHD_i,"usrpTransmit|tuner_id=" << tuner_id << "No matching RFInfo port found!! Failed transmit.");
            return false;
        }
        it->second.rfinfo_pkt.rf_center_freq = frontend_tuner_status[tuner_id].center_frequency;
        it->second.rfinfo_pkt.if_center_freq = frontend_tuner_status[tuner_id].center_frequency;
        it->second.rfinfo_pkt.rf_bandwidth = frontend_tuner_status[tuner_id].bandwidth;
        LOG_DEBUG(USRP_UHD_i,"usrpTransmit|tuner_id=" << tuner_id << "Sending updated rfinfo_pkt: RFInfo port="<<it->first
                <<" rf_center_freq="<<it->second.rfinfo_pkt.rf_center_freq
                <<" if_center_freq="<<it->second.rfinfo_pkt.if_center_freq
                <<" bandwidth="<<it->second.rfinfo_pkt.rf_bandwidth);

        if (it->first == "RFInfoTX_out")
            RFInfoTX_out->rfinfo_pkt(it->second.rfinfo_pkt);
        else if (it->first == "RFInfoTX_out2")
            RFInfoTX_out2->rfinfo_pkt(it->second.rfinfo_pkt);*/
        usrp_tuner.update_sri = false;
    }

    std::vector<bulkio::StreamStatus> error_status;
    BULKIO::PrecisionUTCTime ts_now = bulkio::time::utils::now();
    bulkio::ShortDataBlock block = queue.getNextBlock(ts_now, error_status);
    std::string empty_string;

    verifyHWStatus(empty_string, ts_now);

    if (block) {
        std::string stream_id(block.sri().streamID);
        verifyQueueStatus(stream_id, ts_now, error_status);
        if (block.size() != 0) {
            uhd::tx_metadata_t _metadata;
            _metadata.start_of_burst = false;
            _metadata.end_of_burst = false;

            if (usrp_tx_streamer.get() == NULL || sizeof(short) != usrp_tx_streamer_typesize){
                usrpCreateTxStream();
                RH_DEBUG(this->_baseLog,"usrpTransmit|tuner_number=" << _tuner_number << " got tx_streamer[" << _tuner_number << "]");
            }
            // Send in size/2 because it is complex
            if( usrp_tx_streamer->send(&block.buffer()[0], block.buffer().size() / 2, _metadata, 0.1) != block.buffer().size() / 2) {
                RH_WARN(this->_baseLog, "WARNING: THE USRP WAS UNABLE TO TRANSMIT " << block.buffer().size() / 2 << " NUMBER OF SAMPLES!");
                return false;
            }
        }
        verifyHWStatus(stream_id, ts_now);
    }
    return true;
}

/*************************************************************
Functions supporting tuning allocation
*************************************************************/
void TDC_i::deviceEnable(frontend_tuner_status_struct_struct &fts, size_t tuner_id){
    /************************************************************
    modify fts, which corresponds to this->frontend_tuner_status[tuner_id]
    Make sure to set the 'enabled' member of fts to indicate that tuner as enabled
    ************************************************************/
    //#warning deviceEnable(): Enable the given tuner  *********
    fts.enabled = true;
    return;
}
void TDC_i::deviceDisable(frontend_tuner_status_struct_struct &fts, size_t tuner_id){
    /************************************************************
    modify fts, which corresponds to this->frontend_tuner_status[tuner_id]
    Make sure to reset the 'enabled' member of fts to indicate that tuner as disabled
    ************************************************************/
    //#warning deviceDisable(): Disable the given tuner  *********
    fts.enabled = false;
    return;
}
bool TDC_i::deviceSetTuning(const frontend::frontend_tuner_allocation_struct &request, frontend_tuner_status_struct_struct &fts, size_t tuner_id){
    /************************************************************
    modify fts, which corresponds to this->frontend_tuner_status[tuner_id]
      At a minimum, bandwidth, center frequency, and sample_rate have to be set
      If the device is tuned to exactly what the request was, the code should be:
        fts.bandwidth = request.bandwidth;
        fts.center_frequency = request.center_frequency;
        fts.sample_rate = request.sample_rate;

    return true if the tuning succeeded, and false if it failed
    ************************************************************/
    double if_offset = 0.0;
    double opt_sr = 0.0;
    double opt_bw = 0.0;

    const bool complex = true; // USRP operates using complex data
    try {
        // check device constraints
        // see if IF center frequency is set in rfinfo packet
        double request_if_center_freq = request.center_frequency;
        /*if(request.tuner_type != "TX" && floatingPointCompare(rfinfo.if_center_freq,0) > 0 && floatingPointCompare(rfinfo.rf_center_freq,rfinfo.if_center_freq) > 0) {
            if (rfinfo.spectrum_inverted) {
                request_if_center_freq = rfinfo.if_center_freq - (request.center_frequency - rfinfo.rf_center_freq);
            } else {
                request_if_center_freq = rfinfo.if_center_freq + (request.center_frequency - rfinfo.rf_center_freq);
            }
        }*/

        // check vs. device center freq capability (ensure 0 <= request <= max device capability)
        if ( !frontend::validateRequest(device_characteristics.freq_min,device_characteristics.freq_max,request_if_center_freq) ) {
            throw FRONTEND::BadParameterException("INVALID REQUEST -- device capabilities cannot support freq request");
        }

        // check vs. device bandwidth capability (ensure 0 <= request <= max device capability)
        if ( !frontend::validateRequest(0,device_characteristics.bandwidth_max,request.bandwidth) ){
            throw FRONTEND::BadParameterException("INVALID REQUEST -- device capabilities cannot support bw request");
        }

        // check vs. device sample ratehold capability (ensure 0 <= request <= max device capability)
        if ( !frontend::validateRequest(0,device_characteristics.rate_max,request.sample_rate) ){
            throw FRONTEND::BadParameterException("INVALID REQUEST -- device capabilities cannot support sr request");
        }

        // calculate overall frequency range of the device (not just CF range)
        const size_t scaling_factor = (complex) ? 2 : 4; // adjust for complex data
        const double min_device_freq = device_characteristics.freq_min-(device_characteristics.rate_max/scaling_factor);
        const double max_device_freq = device_characteristics.freq_max+(device_characteristics.rate_max/scaling_factor);

        // check based on bandwidth
        double min_requested_freq = request_if_center_freq-(request.bandwidth/2);
        double max_requested_freq = request_if_center_freq+(request.bandwidth/2);

        if ( !frontend::validateRequest(min_device_freq,max_device_freq,min_requested_freq,max_requested_freq) ) {
            throw FRONTEND::BadParameterException("INVALID REQUEST -- device capabilities cannot support freq/bw request");
        }

        // check based on sample rate
        min_requested_freq = request_if_center_freq-(request.sample_rate/scaling_factor);
        max_requested_freq = request_if_center_freq+(request.sample_rate/scaling_factor);

        if ( !frontend::validateRequest(min_device_freq,max_device_freq,min_requested_freq,max_requested_freq) ){
            throw FRONTEND::BadParameterException("INVALID REQUEST -- device capabilities cannot support freq/sr request");
        }
        /*if( !frontend::validateRequestVsDevice(request, it->second.rfinfo_pkt, complex, device_characteristics.freq_min, device_characteristics.freq_max,
                device_characteristics.bandwidth_max, device_characteristics.rate_max) ){
            throw FRONTEND::BadParameterException("INVALID REQUEST -- falls outside of analog input or device capabilities");
        }*/
    } catch(FRONTEND::BadParameterException& e){
        RH_INFO(this->_baseLog,"deviceSetTuning|BadParameterException - " << e.msg);
        return false;
    }

    scoped_tuner_lock tuner_lock(usrp_tuner.lock);

    // account for RFInfo_pkt that specifies RF and IF frequencies
    // since request is always in RF, and USRP may be operating in IF
    // adjust requested center frequency according to tx rfinfo packet

    // configure hw
    usrp_device_ptr->set_tx_freq(request.center_frequency+if_offset, _tuner_number);
    usrp_device_ptr->set_tx_bandwidth(opt_bw, _tuner_number);
    usrp_device_ptr->set_tx_rate(opt_sr, _tuner_number);

    // update frontend_tuner_status with actual hw values
    fts.center_frequency = usrp_device_ptr->get_tx_freq(_tuner_number)+if_offset;
    fts.bandwidth = usrp_device_ptr->get_tx_bandwidth(_tuner_number);
    fts.sample_rate = usrp_device_ptr->get_tx_rate(_tuner_number);

    // update tolerance
    fts.bandwidth_tolerance = request.bandwidth_tolerance;
    fts.sample_rate_tolerance = request.sample_rate_tolerance;

    updateDeviceCharacteristics();
    this->start();
    return true;
}
bool TDC_i::deviceDeleteTuning(frontend_tuner_status_struct_struct &fts, size_t tuner_id) {
    /************************************************************
    modify fts, which corresponds to this->frontend_tuner_status[tuner_id]
    return true if the tune deletion succeeded, and false if it failed
    ************************************************************/
    //#warning deviceDeleteTuning(): Deallocate an allocated tuner  *********
    return true;
}

double TDC_i::optimizeRate(const double& req_rate){
    RH_TRACE(this->_baseLog,__PRETTY_FUNCTION__ << " req_rate=" << req_rate);

    if(frontend::floatingPointCompare(req_rate,0) <= 0){
        return usrp_range.sample_rate.clip(device_characteristics.rate_min);
    }
    size_t dec = round(device_characteristics.clock_max/req_rate);
    double opt_rate = device_characteristics.clock_max / double(dec);
    double usrp_rate = usrp_range.sample_rate.clip(opt_rate);
    if(frontend::floatingPointCompare(usrp_rate,req_rate) >=0 ){
        return usrp_rate;
    }
    size_t min_dec = round(device_characteristics.clock_max/device_characteristics.rate_max);
    for(dec--; dec >= min_dec; dec--){
        opt_rate = device_characteristics.clock_max / double(dec);
        usrp_rate = usrp_range.sample_rate.clip(opt_rate);
        if(frontend::floatingPointCompare(usrp_rate,req_rate) >=0 ){
            return usrp_rate;
        }
    }

    RH_DEBUG(this->_baseLog,"optimizeRate|could not optimize rate, returning req_rate (" << req_rate << ")");
    return req_rate;
}

/* acquire prop_lock prior to calling this function */
double TDC_i::optimizeBandwidth(const double& req_bw){
    RH_TRACE(this->_baseLog,__PRETTY_FUNCTION__ << " req_bw=" << req_bw);

    if(frontend::floatingPointCompare(req_bw,0) <= 0){
        return usrp_range.bandwidth.clip(device_characteristics.bandwidth_min);
    }
    double usrp_bw = usrp_range.bandwidth.clip(req_bw);
    if(frontend::floatingPointCompare(usrp_bw,req_bw) >=0 ){
        return usrp_bw;
    }

    RH_DEBUG(this->_baseLog,"optimizeBandwidth|could not optimize bw, returning req_bw (" << req_bw << ")");
    return req_bw;
}

/*************************************************************
Functions servicing the tuner control port
*************************************************************/
std::string TDC_i::getTunerType(const std::string& allocation_id) {
    return frontend_tuner_status[0].tuner_type;
}

bool TDC_i::getTunerDeviceControl(const std::string& allocation_id) {
    return true;
}

std::string TDC_i::getTunerGroupId(const std::string& allocation_id) {
    return frontend_tuner_status[0].group_id;
}

std::string TDC_i::getTunerRfFlowId(const std::string& allocation_id) {
    return frontend_tuner_status[0].rf_flow_id;
}

void TDC_i::setTunerCenterFrequency(const std::string& allocation_id, double freq) {
    if (freq<0) throw FRONTEND::BadParameterException("Center frequency cannot be less than 0");
    // set hardware to new value. Raise an exception if it's not possible
    this->frontend_tuner_status[0].center_frequency = freq;
}

double TDC_i::getTunerCenterFrequency(const std::string& allocation_id) {
    return frontend_tuner_status[0].center_frequency;
}

void TDC_i::setTunerBandwidth(const std::string& allocation_id, double bw) {
    if (bw<0) throw FRONTEND::BadParameterException("Bandwidth cannot be less than 0");
    // set hardware to new value. Raise an exception if it's not possible
    this->frontend_tuner_status[0].bandwidth = bw;
}

double TDC_i::getTunerBandwidth(const std::string& allocation_id) {
    return frontend_tuner_status[0].bandwidth;
}

void TDC_i::setTunerAgcEnable(const std::string& allocation_id, bool enable)
{
    throw FRONTEND::NotSupportedException("setTunerAgcEnable not supported");
}

bool TDC_i::getTunerAgcEnable(const std::string& allocation_id)
{
    throw FRONTEND::NotSupportedException("getTunerAgcEnable not supported");
}

void TDC_i::setTunerGain(const std::string& allocation_id, float gain)
{
    throw FRONTEND::NotSupportedException("setTunerGain not supported");
}

float TDC_i::getTunerGain(const std::string& allocation_id)
{
    throw FRONTEND::NotSupportedException("getTunerGain not supported");
}

void TDC_i::setTunerReferenceSource(const std::string& allocation_id, long source)
{
    throw FRONTEND::NotSupportedException("setTunerReferenceSource not supported");
}

long TDC_i::getTunerReferenceSource(const std::string& allocation_id)
{
    throw FRONTEND::NotSupportedException("getTunerReferenceSource not supported");
}

void TDC_i::setTunerEnable(const std::string& allocation_id, bool enable) {
    // set hardware to new value. Raise an exception if it's not possible
    this->frontend_tuner_status[0].enabled = enable;
}

bool TDC_i::getTunerEnable(const std::string& allocation_id) {
    return frontend_tuner_status[0].enabled;
}

void TDC_i::setTunerOutputSampleRate(const std::string& allocation_id, double sr) {
    if (sr<0) throw FRONTEND::BadParameterException("Sample rate cannot be less than 0");
    // set hardware to new value. Raise an exception if it's not possible
    this->frontend_tuner_status[0].sample_rate = sr;
}

double TDC_i::getTunerOutputSampleRate(const std::string& allocation_id){
    return frontend_tuner_status[0].sample_rate;
}

void TDC_i::configureTuner(const std::string& id, const CF::Properties& tunerSettings){
    // set the appropriate tuner settings
}

CF::Properties* TDC_i::getTunerSettings(const std::string& id){
    // return the tuner settings
    redhawk::PropertyMap* tuner_settings = new redhawk::PropertyMap();
    return tuner_settings;
}

void TDC_i::reset(const std::string& allocation_id, const std::string& stream_id) {
    bulkio::StreamQueue<bulkio::InShortPort>& queue = dataShortTX_in->getQueue();
    _error_state = false;
    queue.reset(stream_id);
    FRONTEND::TransmitStatusType status;
    status.stream_id = CORBA::string_dup(stream_id.c_str());
    status.allocation_id = CORBA::string_dup(_allocationTracker.begin()->first.c_str());
    status.timestamp = bulkio::time::utils::now();
    status.total_samples = 0;
    status.total_packets = 0;
    status.transmitting = true;
    status.settling_time = 0;
    status.queued_packets = 0;
    status.status = CF::DEV_OK;
    this->TransmitDeviceStatus_out->transmitStatusChanged(status);
}

bool TDC_i::hold(const std::string& allocation_id, const std::string& stream_id) {
    bulkio::StreamQueue<bulkio::InShortPort>& queue = dataShortTX_in->getQueue();
    return queue.hold(stream_id);
}

std::vector<std::string> TDC_i::held(const std::string& allocation_id, const std::string& stream_id) {
    bulkio::StreamQueue<bulkio::InShortPort>& queue = dataShortTX_in->getQueue();
    return queue.held();
}

bool TDC_i::allow(const std::string& allocation_id, const std::string& stream_id) {
    bulkio::StreamQueue<bulkio::InShortPort>& queue = dataShortTX_in->getQueue();
    return queue.allow(stream_id);
}

void TDC_i::setTransmitParemeters(const std::string& allocation_id, const frontend::TransmitParameters& transmit_parameters) {
    bulkio::StreamQueue<bulkio::InShortPort>& queue = dataShortTX_in->getQueue();
    queue.update_ignore_error(transmit_parameters.ignore_error);
    queue.update_ignore_timestamp(transmit_parameters.ignore_timestamp);
}

frontend::TransmitParameters TDC_i::getTransmitParemeters(const std::string& allocation_id) {
    frontend::TransmitParameters transmit_parameters;
    bulkio::StreamQueue<bulkio::InShortPort>& queue = dataShortTX_in->getQueue();
    transmit_parameters.ignore_error = queue.get_ignore_error();
    transmit_parameters.ignore_timestamp = queue.get_ignore_timestamp();
    return transmit_parameters;
}

