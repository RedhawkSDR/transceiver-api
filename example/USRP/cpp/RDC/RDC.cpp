/**************************************************************************

    This is the device code. This file contains the child class where
    custom functionality can be added to the device. Custom
    functionality to the base class can be extended here. Access to
    the ports can also be done from this class

**************************************************************************/

#include "RDC.h"

PREPARE_LOGGING(RDC_i)

RDC_i::RDC_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl) :
    RDC_base(devMgr_ior, id, lbl, sftwrPrfl)
{
}

RDC_i::RDC_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, char *compDev) :
    RDC_base(devMgr_ior, id, lbl, sftwrPrfl, compDev)
{
}

RDC_i::RDC_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities) :
    RDC_base(devMgr_ior, id, lbl, sftwrPrfl, capacities)
{
}

RDC_i::RDC_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities, char *compDev) :
    RDC_base(devMgr_ior, id, lbl, sftwrPrfl, capacities, compDev)
{
}

RDC_i::~RDC_i()
{
}

void RDC_i::constructor()
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
    this->addChannels(1, "RDC");
    _tuner_number = -1;
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
        
        void RDC_i::my_message_callback(const std::string& id, const my_msg_struct &msg){
        }
        
        Register the message callback onto the input port with the following form:
        this->msg_input->registerMessage("my_msg", this, &RDC_i::my_message_callback);
        
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
        (RDC_base).
    
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
        addPropertyListener(<property>, this, &RDC_i::<callback method>)
        in the constructor.

        The callback method receives two arguments, the old and new values, and
        should return nothing (void). The arguments can be passed by value,
        receiving a copy (preferred for primitive types), or by const reference
        (preferred for strings, structs and vectors).

        Example:
            // This example makes use of the following Properties:
            //  - A float value called scaleValue
            //  - A struct property called status
            
        //Add to RDC.cpp
        RDC_i::RDC_i(const char *uuid, const char *label) :
            RDC_base(uuid, label)
        {
            addPropertyListener(scaleValue, this, &RDC_i::scaleChanged);
            addPropertyListener(status, this, &RDC_i::statusChanged);
        }

        void RDC_i::scaleChanged(float oldValue, float newValue)
        {
            RH_DEBUG(this->_baseLog, "scaleValue changed from" << oldValue << " to " << newValue);
        }
            
        void RDC_i::statusChanged(const status_struct& oldValue, const status_struct& newValue)
        {
            RH_DEBUG(this->_baseLog, "status changed");
        }
            
        //Add to RDC.h
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
        
        bool RDC_i::my_alloc_fn(const std::string &value)
        {
            // perform logic
            return true; // successful allocation
        }
        void RDC_i::my_dealloc_fn(const std::string &value)
        {
            // perform logic
        }
        
        The allocation and deallocation functions are then registered with the Device
        base class with the setAllocationImpl call. Note that the variable for the property is used rather
        than its id:
        
        this->setAllocationImpl(my_alloc, this, &RDC_i::my_alloc_fn, &RDC_i::my_dealloc_fn);
        
        

************************************************************************************************/
int RDC_i::serviceFunction()
{
    if ((usrp_device_ptr.get() == NULL) or (frontend_tuner_status[0].allocation_id_csv.empty()) or (not frontend_tuner_status[0].enabled)) {
        return NOOP;
    }

    bool rx_data = false;

    scoped_tuner_lock tuner_lock(usrp_tuner.lock);

    long num_samps = usrpReceive(1.0); // 1 second timeout

    /* if auto-gain enabled, push data to gain method */
    if (trigger_rx_autogain) {
        float newGain = auto_gain(); // auto_gain will set trigger to false if appropriate
        if(newGain != device_gain)
            updateDeviceRxGain(newGain, false);
    }

    // if the buffer is full OR (overflow occurred and buffer isn't empty), push buffer out as is and move to next buffer
    if(usrp_tuner.buffer_size >= usrp_tuner.buffer_capacity || (num_samps < 0 && usrp_tuner.buffer_size > 0) ){
        rx_data = true;

        RH_DEBUG(this->_baseLog, "serviceFunctionReceive|pushing buffer of " << usrp_tuner.buffer_size/2 << " samples");

        // set stream id (creates one if not already created for this tuner)
        getStreamId();

        // Send updated SRI
        if (usrp_tuner.update_sri){
            RH_DEBUG(this->_baseLog, "USRP_UHD_i::serviceFunctionReceive|creating SRI for tuner: "<<_tuner_number<<" with stream id: "<< _stream_id);
            //BULKIO::StreamSRI sri = bulkio::sri::create(_stream_id, frontend_tuner_status[0], -1.0);
            BULKIO::StreamSRI sri = this->create(_stream_id, frontend_tuner_status[0], -1.0);
            sri.mode = 1; // complex
            //printSRI(&sri,"USRP_UHD_i::serviceFunctionReceive SRI"); // DEBUG
            dataShort_out->pushSRI(sri);
            //dataSDDS_out->pushSRI(sri);
            usrp_tuner.update_sri = false;
        }

        // Pushing Data
        // handle partial packet (b/c overflow occured)
        if(usrp_tuner.buffer_size < usrp_tuner.buffer_capacity){
            usrp_tuner.output_buffer.resize(usrp_tuner.buffer_size);
        }
        // Only push on active ports
        if(dataShort_out->isActive()){
            dataShort_out->pushPacket(usrp_tuner.output_buffer, usrp_tuner.output_buffer_time, false, _stream_id);
        }
        // Don't check isActive because could be relying on attach override rather than a connection
        // It doesn't actually do anything if the tuner/stream isn't configured for sdds already anyway
        //dataSDDS_out->pushPacket(usrp_tuner.output_buffer, usrp_tuner.output_buffer_time, false, _stream_id);
        // restore buffer size if necessary
        if(usrp_tuner.buffer_size < usrp_tuner.buffer_capacity){
            usrp_tuner.output_buffer.resize(usrp_tuner.buffer_capacity);
        }
        usrp_tuner.buffer_size = 0;
        
    } else if(num_samps != 0){ // either received data or overflow occurred, either way data is available
        rx_data = true;
    }

    if(rx_data)
        return NORMAL;
    return NOOP;
}

void RDC_i::getStreamId() {
    if (_stream_id.empty()){
        std::ostringstream id;
        id<<"tuner_freq_"<<long(frontend_tuner_status[0].center_frequency)<<"_Hz_"<<frontend::uuidGenerator();
        _stream_id = id.str();
        usrp_tuner.update_sri = true;
        RH_DEBUG(this->_baseLog,"USRP_UHD_i::getStreamId - created NEW stream id: "<< _stream_id);
    } else {
        RH_DEBUG(this->_baseLog,"USRP_UHD_i::getStreamId - returning EXISTING stream id: "<< _stream_id);
    }
}

void RDC_i::updateDeviceRxGain(double gain, bool lock) {
    RH_TRACE(this->_baseLog,__PRETTY_FUNCTION__ << " gain=" << gain);

    if (usrp_device_ptr.get() == NULL)
        return;

    if (lock) {
        scoped_tuner_lock tuner_lock(usrp_tuner.lock);
    }
    usrp_device_ptr->set_rx_gain(gain,_tuner_number);
    device_gain = usrp_device_ptr->get_rx_gain(_tuner_number);
    RH_DEBUG(this->_baseLog,__PRETTY_FUNCTION__ << " Updated Gain. New gain is " << device_gain);
}

/* acquire tuner_lock prior to calling this function *
 * this function will block up to "timeout" seconds
 */
long RDC_i::usrpReceive(double timeout){
    RH_TRACE(this->_baseLog,__PRETTY_FUNCTION__ << " timeout:" << timeout);

    // calc num samps to rx based on timeout, sr, and buffer size
    size_t samps_to_rx = size_t((usrp_tuner.buffer_capacity-usrp_tuner.buffer_size) / 2);
    if( timeout > 0 ){
        samps_to_rx = std::min(samps_to_rx, size_t(timeout*frontend_tuner_status[0].sample_rate));
    }

    uhd::rx_metadata_t _metadata;

    if (usrp_rx_streamer.get() == NULL){
        usrpCreateRxStream();
        RH_TRACE(this->_baseLog, "got rx_streamer[" << this->_tuner_number << "]");
    }

    size_t num_samps = 0;
    try{
        num_samps = usrp_rx_streamer->recv(
            &usrp_tuner.output_buffer.at(usrp_tuner.buffer_size), // address of buffer to start filling data
            samps_to_rx,
            _metadata);
    } catch(...){
        RH_ERROR(this->_baseLog, "uhd::rx_streamer->recv() threw unknown exception");
        return 0;
    }
    RH_TRACE(this->_baseLog, "usrpReceive|tuner_number=" << _tuner_number << " num_samps=" << num_samps);
    usrp_tuner.buffer_size += (num_samps*2);

    //handle possible errors conditions
    switch (_metadata.error_code) {
        case uhd::rx_metadata_t::ERROR_CODE_NONE:
            break;
        case uhd::rx_metadata_t::ERROR_CODE_TIMEOUT:
            RH_WARN(this->_baseLog, "WARNING: TIMEOUT OCCURED ON USRP RECEIVE! (received num_samps=" << num_samps << ")");
            return 0;
        case uhd::rx_metadata_t::ERROR_CODE_OVERFLOW:
            RH_WARN(this->_baseLog, "WARNING: USRP OVERFLOW DETECTED!");
            // may have received data, but 0 is returned by usrp recv function so we don't know how many samples, must throw away
            return -1; // this will just cause us to return NORMAL so there's no wait before next iteration
        default:
            RH_WARN(this->_baseLog, "WARNING: UHD source block got error code 0x" << _metadata.error_code);
            return 0;
    }
    RH_TRACE(this->_baseLog, "usrpReceive|tuner_number=" << _tuner_number << " after error switch");

    if(num_samps == 0)
        return 0;

    RH_DEBUG(this->_baseLog, "usrpReceive|received data.  num_samps=" << num_samps
                                                << "  buffer_size=" << usrp_tuner.buffer_size
                                                << "  buffer_capacity=" << usrp_tuner.buffer_capacity );

    // if first samples in buffer, update timestamps
    if(num_samps*2 == usrp_tuner.buffer_size){
        usrp_tuner.output_buffer_time = bulkio::time::utils::now();
        usrp_tuner.output_buffer_time.twsec = (double)_metadata.time_spec.get_full_secs();
        usrp_tuner.output_buffer_time.tfsec = _metadata.time_spec.get_frac_secs();
        if (usrp_tuner.time_up.twsec <= 0)
            usrp_tuner.time_up = usrp_tuner.output_buffer_time;
        usrp_tuner.time_down = usrp_tuner.output_buffer_time;
    }

    return num_samps;
}

/*----------------------------------------------------------------------------
        This is a very simple auto-gain function.
        Notes:
             1. Calculates the bit 'loading' of a sample over a sample set.
             2. Kept generic to work for all daughtercard types in all modes.
             3. Calculates instantaneous values; not continuously calculating.
             4. Requires minimum of 500 samples (250 complex samples)
 ----------------------------------------------------------------------------*/
float RDC_i::auto_gain() {
    size_t  samplesRequired = 500; // not configurable; hard-coded to 500, which is really 250 complex samples
    size_t  samplesFound    = 0;
    long    maxBits         = (device_mode == "8bit") ? 8 : 16;
    maxBits -= this->rx_autogain_guard_bits;
    short   maxValue        = (device_mode == "8bit") ? 0x7f  : 0x7fff;
    short   maxValueFound   = 0; // max value in current buffer
    long    bitsInUse       = 0;
    float   maxGain         = 0;
    float   minGain         = 0;
    float   gainAdjust      = 0;
    float   newGain         = device_gain;

    maxGain = device_characteristics.gain_max;
    minGain = device_characteristics.gain_min;
    samplesFound = usrp_tuner.buffer_size;
    for(size_t sampleNum=0 ; sampleNum<usrp_tuner.buffer_size; sampleNum++) {
        // max value tracker. Look at Real and Complex values.
        if((short)(usrp_tuner.output_buffer[sampleNum]) > maxValueFound)
            maxValueFound = (short)(usrp_tuner.output_buffer[sampleNum]);
    }

    // require buffer to have sufficient number of samples before turning off trigger
    RH_DEBUG(this->_baseLog, __PRETTY_FUNCTION__ << " Got " << samplesFound << " of " << samplesRequired << " samples required for auto-gain calculation.");
    if (samplesFound >= samplesRequired) {
        RH_DEBUG(this->_baseLog, __PRETTY_FUNCTION__ << " Max value in buffer is " << maxValueFound << " compared to a fully loaded max of " << maxValue);
        trigger_rx_autogain = false;
    } else {
        RH_DEBUG(this->_baseLog, __PRETTY_FUNCTION__ << " Not enough samples to calculate auto-gain; not resetting trigger yet.");
        trigger_rx_autogain = true;
        return device_gain; // no change yet
    }

    // compute bits currently in use
    for (int32_t m=maxValueFound; m; m>>=1) bitsInUse++; // floor(log10(maxValueFound) / log10(2)) + 1
    bitsInUse++; // add 1 to account for negative range as well
    RH_DEBUG(this->_baseLog, __PRETTY_FUNCTION__ << " Max value in buffer uses " << bitsInUse <<" bits compared to " << maxBits << " available non-guard bits");

    // compute gain adjustment
    gainAdjust = (maxBits-bitsInUse) * 6; // x6 to convert bits to dB
    RH_DEBUG(this->_baseLog, __PRETTY_FUNCTION__ << " Gain adjustment calculated is " << gainAdjust);

    // adjust gain according to actual device min and max values
    if (gainAdjust > 0) { // increase gain if possible
        RH_DEBUG(this->_baseLog, __PRETTY_FUNCTION__ << " device_gain=" << device_gain << "  maxGain=" << maxGain);
        if (device_gain < maxGain) {
            newGain = fmin(gainAdjust+device_gain, maxGain);
            RH_DEBUG(this->_baseLog,__PRETTY_FUNCTION__<<" Increasing Gain to " << newGain);
        } else {
            // input signal too low.  can't increase gain further
            RH_DEBUG(this->_baseLog,__PRETTY_FUNCTION__<<" Input too low; no more gain possible!");
            newGain = maxGain;
        }
    } else if (gainAdjust < 0) { // reduce gain if possible
        RH_DEBUG(this->_baseLog, __PRETTY_FUNCTION__ << " device_gain=" << device_gain << "  minGain=" << minGain);
        if (device_gain > minGain) {
            newGain = fmax(gainAdjust+device_gain, minGain);
            RH_DEBUG(this->_baseLog, __PRETTY_FUNCTION__ << " Decreasing Gain to " << newGain);
        } else {
            // input signal too hot.  can't reduce gain further
            RH_DEBUG(this->_baseLog, __PRETTY_FUNCTION__ << " Input too high; no more attenuation possible!");
            newGain = minGain;
        }
    } else { // no change necessary
        RH_DEBUG(this->_baseLog, __PRETTY_FUNCTION__ << " No gain adjustments necessary.");
        newGain = device_gain;
    }
    return newGain;
}

void RDC_i::setTunerNumber(size_t tuner_number) {
    this->_tuner_number = tuner_number;
    this->updateDeviceCharacteristics();
}

void RDC_i::updateDeviceCharacteristics() {
    if ((usrp_device_ptr.get() != NULL) and (_tuner_number != -1))  {
        device_characteristics.tuner_type = "RDC";
        device_characteristics.ch_name = usrp_device_ptr->get_rx_subdev_name(_tuner_number);
        device_characteristics.antenna = usrp_device_ptr->get_rx_antenna(_tuner_number);
        device_characteristics.available_antennas = usrp_device_ptr->get_rx_antennas(_tuner_number);
        device_characteristics.freq_current = usrp_device_ptr->get_rx_freq(_tuner_number);
        device_characteristics.bandwidth_current = usrp_device_ptr->get_rx_bandwidth(_tuner_number);
        device_characteristics.rate_current = usrp_device_ptr->get_rx_rate(_tuner_number);

        usrp_range.bandwidth = usrp_device_ptr->get_rx_bandwidth_range(_tuner_number);
        usrp_range.sample_rate = usrp_device_ptr->get_rx_rates(_tuner_number);
        device_characteristics.gain_current = usrp_device_ptr->get_rx_gain(_tuner_number);
        usrp_range.gain = usrp_device_ptr->get_rx_gain_range(_tuner_number);
        usrp_range.frequency = usrp_device_ptr->get_rx_freq_range(_tuner_number);

        device_characteristics.bandwidth_min = usrp_range.bandwidth.start();
        device_characteristics.bandwidth_max = usrp_range.bandwidth.stop();
        device_characteristics.rate_min = usrp_range.sample_rate.start();
        device_characteristics.rate_max = usrp_range.sample_rate.stop();
        device_characteristics.gain_min = usrp_range.gain.start();
        device_characteristics.gain_max = usrp_range.gain.stop();
        device_characteristics.freq_min = usrp_range.frequency.start();
        device_characteristics.freq_max = usrp_range.frequency.stop();

        try {
            std::vector<double> rates = usrp_device_ptr->get_rx_dboard_iface(_tuner_number)->get_clock_rates(uhd::usrp::dboard_iface::UNIT_RX);
            device_characteristics.clock_min = rates.back();
            device_characteristics.clock_max = rates.front();
        } catch (...) {
            device_characteristics.clock_min = 0;
            device_characteristics.clock_max = 2*device_characteristics.rate_max;
        }
    }
}

void RDC_i::setUHDptr(const uhd::usrp::multi_usrp::sptr parent_device_ptr) {
    usrp_device_ptr = parent_device_ptr;
    this->updateDeviceCharacteristics();
}

/* acquire tuner_lock prior to calling this function *
 */
bool RDC_i::usrpCreateRxStream(){
    //cleanup possible old one
    usrp_rx_streamer.reset();

    /*!
     * The CPU format is a string that describes the format of host memory.
     * Conversions for the following CPU formats have been implemented:
     *  - fc64 - complex<double>
     *  - fc32 - complex<float>
     *  - sc16 - complex<int16_t>
     *  - sc8 - complex<int8_t>
     */
    std::string cpu_format = "sc16"; // complex dataShort
    RH_DEBUG(this->_baseLog, "usrpCreateRxStream|using cpu_format " << cpu_format);

    /*!
     * The OTW format is a string that describes the format over-the-wire.
     * The following over-the-wire formats have been implemented:
     *  - sc16 - Q16 I16
     *  - sc8 - Q8_1 I8_1 Q8_0 I8_0
     */
    std::string wire_format = "sc16";
    if(device_mode == "8bit") {
        wire_format = "sc8"; // enable 8-bit mode with "sc8"
    }
    RH_DEBUG(this->_baseLog, "usrpCreateRxStream|using wire_format " << wire_format);

    uhd::stream_args_t stream_args(cpu_format,wire_format);
    stream_args.channels.push_back(this->_tuner_number);
    stream_args.args["noclear"] = "1";
    usrp_rx_streamer = usrp_device_ptr->get_rx_stream(stream_args);
    return true;
}

/*************************************************************
Functions supporting tuning allocation
*************************************************************/
void RDC_i::deviceEnable(frontend_tuner_status_struct_struct &fts, size_t tuner_id){
    /************************************************************
    modify fts, which corresponds to this->frontend_tuner_status[tuner_id]
    Make sure to set the 'enabled' member of fts to indicate that tuner as enabled
    ************************************************************/
    //#warning deviceEnable(): Enable the given tuner  *********
    fts.enabled = true;
    return;
}
void RDC_i::deviceDisable(frontend_tuner_status_struct_struct &fts, size_t tuner_id){
    /************************************************************
    modify fts, which corresponds to this->frontend_tuner_status[tuner_id]
    Make sure to reset the 'enabled' member of fts to indicate that tuner as disabled
    ************************************************************/
    //#warning deviceDisable(): Disable the given tuner  *********
    fts.enabled = false;
    return;
}
bool RDC_i::deviceSetTuning(const frontend::frontend_tuner_allocation_struct &request, frontend_tuner_status_struct_struct &fts, size_t tuner_id){
    /************************************************************
    modify fts, which corresponds to this->frontend_tuner_status[tuner_id]
      At a minimum, bandwidth, center frequency, and sample_rate have to be set
      If the device is tuned to exactly what the request was, the code should be:
        fts.bandwidth = request.bandwidth;
        fts.center_frequency = request.center_frequency;
        fts.sample_rate = request.sample_rate;

    return true if the tuning succeeded, and false if it failed
    ************************************************************/
    //#warning deviceSetTuning(): Evaluate whether or not a tuner is added  *********
    return true;
}
bool RDC_i::deviceDeleteTuning(frontend_tuner_status_struct_struct &fts, size_t tuner_id) {
    /************************************************************
    modify fts, which corresponds to this->frontend_tuner_status[tuner_id]
    return true if the tune deletion succeeded, and false if it failed
    ************************************************************/
    //#warning deviceDeleteTuning(): Deallocate an allocated tuner  *********
    return true;
}
/*************************************************************
Functions servicing the tuner control port
*************************************************************/
std::string RDC_i::getTunerType(const std::string& allocation_id) {
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    return frontend_tuner_status[idx].tuner_type;
}

bool RDC_i::getTunerDeviceControl(const std::string& allocation_id) {
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    if (getControlAllocationId(idx) == allocation_id)
        return true;
    return false;
}

std::string RDC_i::getTunerGroupId(const std::string& allocation_id) {
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    return frontend_tuner_status[idx].group_id;
}

std::string RDC_i::getTunerRfFlowId(const std::string& allocation_id) {
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    return frontend_tuner_status[idx].rf_flow_id;
}

void RDC_i::setTunerCenterFrequency(const std::string& allocation_id, double freq) {
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    if(allocation_id != getControlAllocationId(idx))
        throw FRONTEND::FrontendException(("ID "+allocation_id+" does not have authorization to modify the tuner").c_str());
    if (freq<0) throw FRONTEND::BadParameterException("Center frequency cannot be less than 0");
    // set hardware to new value. Raise an exception if it's not possible
    this->frontend_tuner_status[idx].center_frequency = freq;
}

double RDC_i::getTunerCenterFrequency(const std::string& allocation_id) {
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    return frontend_tuner_status[idx].center_frequency;
}

void RDC_i::setTunerBandwidth(const std::string& allocation_id, double bw) {
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    if(allocation_id != getControlAllocationId(idx))
        throw FRONTEND::FrontendException(("ID "+allocation_id+" does not have authorization to modify the tuner").c_str());
    if (bw<0) throw FRONTEND::BadParameterException("Bandwidth cannot be less than 0");
    // set hardware to new value. Raise an exception if it's not possible
    this->frontend_tuner_status[idx].bandwidth = bw;
}

double RDC_i::getTunerBandwidth(const std::string& allocation_id) {
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    return frontend_tuner_status[idx].bandwidth;
}

void RDC_i::setTunerAgcEnable(const std::string& allocation_id, bool enable)
{
    throw FRONTEND::NotSupportedException("setTunerAgcEnable not supported");
}

bool RDC_i::getTunerAgcEnable(const std::string& allocation_id)
{
    throw FRONTEND::NotSupportedException("getTunerAgcEnable not supported");
}

void RDC_i::setTunerGain(const std::string& allocation_id, float gain)
{
    throw FRONTEND::NotSupportedException("setTunerGain not supported");
}

float RDC_i::getTunerGain(const std::string& allocation_id)
{
    throw FRONTEND::NotSupportedException("getTunerGain not supported");
}

void RDC_i::setTunerReferenceSource(const std::string& allocation_id, long source)
{
    throw FRONTEND::NotSupportedException("setTunerReferenceSource not supported");
}

long RDC_i::getTunerReferenceSource(const std::string& allocation_id)
{
    throw FRONTEND::NotSupportedException("getTunerReferenceSource not supported");
}

void RDC_i::setTunerEnable(const std::string& allocation_id, bool enable) {
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    if(allocation_id != getControlAllocationId(idx))
        throw FRONTEND::FrontendException(("ID "+allocation_id+" does not have authorization to modify the tuner").c_str());
    // set hardware to new value. Raise an exception if it's not possible
    this->frontend_tuner_status[idx].enabled = enable;
}

bool RDC_i::getTunerEnable(const std::string& allocation_id) {
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    return frontend_tuner_status[idx].enabled;
}

void RDC_i::setTunerOutputSampleRate(const std::string& allocation_id, double sr) {
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    if(allocation_id != getControlAllocationId(idx))
        throw FRONTEND::FrontendException(("ID "+allocation_id+" does not have authorization to modify the tuner").c_str());
    if (sr<0) throw FRONTEND::BadParameterException("Sample rate cannot be less than 0");
    // set hardware to new value. Raise an exception if it's not possible
    this->frontend_tuner_status[idx].sample_rate = sr;
}

double RDC_i::getTunerOutputSampleRate(const std::string& allocation_id){
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    return frontend_tuner_status[idx].sample_rate;
}

/*************************************************************
Functions servicing the RFInfo port(s)
- port_name is the port over which the call was received
*************************************************************/
std::string RDC_i::get_rf_flow_id(const std::string& port_name)
{
    return std::string("none");
}

void RDC_i::set_rf_flow_id(const std::string& port_name, const std::string& id)
{
}

frontend::RFInfoPkt RDC_i::get_rfinfo_pkt(const std::string& port_name)
{
    frontend::RFInfoPkt pkt;
    return pkt;
}

void RDC_i::set_rfinfo_pkt(const std::string& port_name, const frontend::RFInfoPkt &pkt)
{
}

