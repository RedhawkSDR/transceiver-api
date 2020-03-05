/**************************************************************************

    This is the device code. This file contains the child class where
    custom functionality can be added to the device. Custom
    functionality to the base class can be extended here. Access to
    the ports can also be done from this class

**************************************************************************/

#include "USRP.h"
#include <ios>

PREPARE_LOGGING(USRP_i)

namespace {
    static inline void wait_pps(uhd::usrp::multi_usrp::sptr device)
    {
        boost::system_time end_time = boost::get_system_time() + boost::posix_time::milliseconds(1100);
        uhd::time_spec_t time_start_last_pps = device->get_time_last_pps();
        while (time_start_last_pps == device->get_time_last_pps())
        {
            if (boost::get_system_time() > end_time)
            {
                throw uhd::runtime_error(
                    "Board 0 may not be getting a PPS signal!\n"
                    "No PPS detected within the time interval.\n"
                    "See the application notes for your device.\n"
                );
            }
            boost::this_thread::sleep(boost::posix_time::milliseconds(1));
        }
    }

    static const long PREDELAY_USEC = 250;
}

USRP_i::USRP_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl) :
    USRP_base(devMgr_ior, id, lbl, sftwrPrfl)
{
}

USRP_i::USRP_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, char *compDev) :
    USRP_base(devMgr_ior, id, lbl, sftwrPrfl, compDev)
{
}

USRP_i::USRP_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities) :
    USRP_base(devMgr_ior, id, lbl, sftwrPrfl, capacities)
{
}

USRP_i::USRP_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities, char *compDev) :
    USRP_base(devMgr_ior, id, lbl, sftwrPrfl, capacities, compDev)
{
}

USRP_i::~USRP_i()
{
}

void USRP_i::constructor()
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

    addPropertyListener(device_reference_source_global, this, &USRP_i::deviceReferenceSourceChanged);

    uhd::device_addr_t hint;
    uhd::device_addrs_t dev_addrs = uhd::device::find(hint);
    if (dev_addrs.size() > 1) {
        std::stringstream errstr;
        errstr << "Unambiguous USRP. Found "<<dev_addrs.size()<<" instead of just 1";
        RH_ERROR(this->_baseLog, errstr.str());
        CF::StringSequence messages;
        ossie::corba::push_back(messages, errstr.str().c_str());
        throw CF::LifeCycle::InitializeError(messages);
    } else if (dev_addrs.empty()) {
        std::string errstr("No USRP found");
        RH_ERROR(this->_baseLog, errstr);
        CF::StringSequence messages;
        ossie::corba::push_back(messages, errstr.c_str());
        throw CF::LifeCycle::InitializeError(messages);
    }
    usrp_device_ptr = uhd::usrp::multi_usrp::make(dev_addrs[0]);
    if (usrp_device_ptr.get() != NULL) {
        const size_t num_rx_channels = usrp_device_ptr->get_rx_num_channels();
        const size_t num_tx_channels = usrp_device_ptr->get_tx_num_channels();
        std::cout<<"number of rx channels: "<<num_rx_channels<<std::endl;
        std::cout<<"number of tx channels: "<<num_tx_channels<<std::endl;
        for (unsigned int i=0; i<num_rx_channels; i++) {
            std::ostringstream rdc_name;
            rdc_name << "RDC_" << i+1;
            RDCs.push_back(this->addChild<RDC_i>(rdc_name.str()));
            RDCs.back()->setTunerNumber(i);
            RDCs.back()->setUHDptr(usrp_device_ptr);
        }
        for (unsigned int i=0; i<num_tx_channels; i++) {
            std::ostringstream tdc_name;
            tdc_name << "TDC_" << i+1;
            TDCs.push_back(this->addChild<TDC_i>(tdc_name.str()));
        }
        std::cout<<"len RDC: "<<RDCs.size()<<std::endl;
        std::cout<<"len TDC: "<<TDCs.size()<<std::endl;
    }
    setPropertyQueryImpl(frontend_tuner_status, this, &USRP_i::get_fts);
}

std::vector<frontend_tuner_status_struct_struct> USRP_i::get_fts()
{
    frontend_tuner_status.resize(0);
    for (std::vector<RDC_i*>::iterator it=RDCs.begin(); it!=RDCs.end(); it++) {
        Device_impl* dev = dynamic_cast<Device_impl*>(*it);
        CF::Properties prop;
        prop.length(1);
        prop[0].value = CORBA::Any();
        prop[0].id = CORBA::string_dup("FRONTEND::tuner_status");
        if (dev) {
            dev->query(prop);
            CORBA::AnySeq *anySeqPtr;
            frontend_tuner_status_struct_struct tmp;
            if (prop[0].value >>= anySeqPtr) {
                CORBA::AnySeq& anySeq = *anySeqPtr;
                if (anySeq[0] >>= tmp) {
                    frontend_tuner_status.push_back(tmp);
                }
            }
        }
    }
    for (std::vector<TDC_i*>::iterator it=TDCs.begin(); it!=TDCs.end(); it++) {
        Device_impl* dev = dynamic_cast<Device_impl*>(*it);
        CF::Properties prop;
        prop.length(1);
        prop[0].value = CORBA::Any();
        prop[0].id = CORBA::string_dup("FRONTEND::tuner_status");
        if (dev) {
            dev->query(prop);
            CORBA::AnySeq *anySeqPtr;
            frontend_tuner_status_struct_struct tmp;
            if (prop[0].value >>= anySeqPtr) {
                CORBA::AnySeq& anySeq = *anySeqPtr;
                if (anySeq[0] >>= tmp) {
                    frontend_tuner_status.push_back(tmp);
                }
            }
        }
    }
    return frontend_tuner_status;
}

void USRP_i::frontendTunerStatusChanged(const std::vector<frontend_tuner_status_struct_struct>* oldValue, const std::vector<frontend_tuner_status_struct_struct>* newValue)
{
}

bool USRP_i::_synchronizeClock(const std::string source)
{
    RH_DEBUG(this->_baseLog, "Synchronizing clock to " << source);

    // Wait for lock on the clock reference, although this does not seem to be
    // sufficient when setting the clock from GPSDO to internal
    // TODO: Timeout?
    while (!(usrp_device_ptr->get_mboard_sensor("ref_locked", 0).to_bool())) {
        boost::this_thread::sleep(boost::posix_time::milliseconds(10));
    }

    if (source == "GPSDO") {
        RH_DEBUG(this->_baseLog, "Waiting for GPS lock");
        // Require GPS lock before clock sync
        // TODO: Timeout?
        while (!(usrp_device_ptr->get_mboard_sensor("gps_locked", 0).to_bool())) {
            boost::this_thread::sleep(boost::posix_time::seconds(1));
        }

        // Synchronize the USRP clock to GPS time by waiting for a PPS edge 
        // and setting the next GPS time (in seconds) on the following PPS
        // edge.
        wait_pps(usrp_device_ptr);
        uhd::time_spec_t next_gps(time_t(usrp_device_ptr->get_mboard_sensor("gps_time").to_int()+1));
        usrp_device_ptr->set_time_next_pps(next_gps);

        // Check that the last PPS time and most recent GPS time match. Per the
        // UHD docs, there is supposed to be a 200ms lag between the PPS edge
        // and GPS time updating, but in practice this seemed problematic.
        wait_pps(usrp_device_ptr);
        uhd::time_spec_t last_pps = usrp_device_ptr->get_time_last_pps();
        uhd::time_spec_t last_gps(usrp_device_ptr->get_mboard_sensor("gps_time").to_int(), 0.0);
        if (last_pps != last_gps) {
            RH_WARN(this->_baseLog, "Waiting for GPS lock");
            return false;
        }
        RH_INFO(this->_baseLog, "Device clock synchronized with GPS");
    } else {
        // Synchronize the USRP clock as closely as possible with the host by
        // waiting for a PPS edge, then determining what the time would be at
        // the next PPS edge and set that.
        wait_pps(usrp_device_ptr);
        struct timeval tmp_time;
        gettimeofday(&tmp_time, NULL);
        time_t wsec = tmp_time.tv_sec;
        double fsec = tmp_time.tv_usec / 1e6;
        usrp_device_ptr->set_time_next_pps(uhd::time_spec_t(wsec+1,fsec));
        // NB: If you don't sleep or otherwise wait for the next PPS pulse,
        //     the time will be set to the Unix epoch.
        wait_pps(usrp_device_ptr);
    }

    return true;
}

void USRP_i::deviceReferenceSourceChanged(std::string old_value, std::string new_value){
    RH_DEBUG(this->_baseLog,__PRETTY_FUNCTION__ << "old_value=" << old_value << "  new_value=" << new_value);
    RH_DEBUG(this->_baseLog,"deviceReferenceSourceChanged|device_reference_source_global=" << device_reference_source_global);

    updateDeviceReferenceSource(new_value);

    clock_sync = _synchronizeClock(new_value);
}

void USRP_i::updateDeviceReferenceSource(std::string source){
    RH_TRACE(this->_baseLog,__PRETTY_FUNCTION__ << " source=" << source);

    if (usrp_device_ptr.get() == NULL)
        return;

    if (source == "MIMO") {
        usrp_device_ptr->set_clock_source("MIMO",0);
        usrp_device_ptr->set_time_source("MIMO",0);
    } else if (source == "EXTERNAL") {
        usrp_device_ptr->set_clock_source("external",0);
        usrp_device_ptr->set_time_source("external",0);
    } else if (source == "INTERNAL") {
        usrp_device_ptr->set_clock_source("internal",0);
        usrp_device_ptr->set_time_source("external",0);
    } else if (source == "GPSDO") {
        usrp_device_ptr->set_clock_source("gpsdo",0);
        usrp_device_ptr->set_time_source("gpsdo",0);
    }
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
        
        void USRP_i::my_message_callback(const std::string& id, const my_msg_struct &msg){
        }
        
        Register the message callback onto the input port with the following form:
        this->msg_input->registerMessage("my_msg", this, &USRP_i::my_message_callback);
        
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
        (USRP_base).
    
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
        addPropertyListener(<property>, this, &USRP_i::<callback method>)
        in the constructor.

        The callback method receives two arguments, the old and new values, and
        should return nothing (void). The arguments can be passed by value,
        receiving a copy (preferred for primitive types), or by const reference
        (preferred for strings, structs and vectors).

        Example:
            // This example makes use of the following Properties:
            //  - A float value called scaleValue
            //  - A struct property called status
            
        //Add to USRP.cpp
        USRP_i::USRP_i(const char *uuid, const char *label) :
            USRP_base(uuid, label)
        {
            addPropertyListener(scaleValue, this, &USRP_i::scaleChanged);
            addPropertyListener(status, this, &USRP_i::statusChanged);
        }

        void USRP_i::scaleChanged(float oldValue, float newValue)
        {
            RH_DEBUG(this->_baseLog, "scaleValue changed from" << oldValue << " to " << newValue);
        }
            
        void USRP_i::statusChanged(const status_struct& oldValue, const status_struct& newValue)
        {
            RH_DEBUG(this->_baseLog, "status changed");
        }
            
        //Add to USRP.h
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
        
        bool USRP_i::my_alloc_fn(const std::string &value)
        {
            // perform logic
            return true; // successful allocation
        }
        void USRP_i::my_dealloc_fn(const std::string &value)
        {
            // perform logic
        }
        
        The allocation and deallocation functions are then registered with the Device
        base class with the setAllocationImpl call. Note that the variable for the property is used rather
        than its id:
        
        this->setAllocationImpl(my_alloc, this, &USRP_i::my_alloc_fn, &USRP_i::my_dealloc_fn);
        
        

************************************************************************************************/
int USRP_i::serviceFunction()
{
    RH_DEBUG(this->_baseLog, "serviceFunction() example log message");
    
    return NOOP;
}

CF::Device::Allocations* USRP_i::allocate(const CF::Properties& capacities)
throw (CF::Device::InvalidState, CF::Device::InvalidCapacity, CF::Device::InsufficientCapacity, CORBA::SystemException)
{
    CF::Device::Allocations_var result = new CF::Device::Allocations();

    if (capacities.length() == 0) {
        RH_TRACE(this->_baseLog, "no capacities to configure.");
        return result._retn();
    }

    std::string allocation_id = ossie::generateUUID();
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(capacities);

    // copy the const properties to something that is modifiable
    CF::Properties local_capacities;
    redhawk::PropertyMap& local_props = redhawk::PropertyMap::cast(local_capacities);
    local_props = props;

    if (local_props.find("FRONTEND::coherent_feeds") != local_props.end()) {
        /*redhawk::PropertyMap& tuner_alloc = redhawk::PropertyMap::cast(local_props["FRONTEND::tuner_allocation"].asProperties());
        if (tuner_alloc.find("FRONTEND::tuner_allocation::allocation_id") != tuner_alloc.end()) {
            std::string requested_alloc = tuner_alloc["FRONTEND::tuner_allocation::allocation_id"].toString();
            if (not requested_alloc.empty()) {
                if (_delegatedAllocations.find(requested_alloc) == _delegatedAllocations.end()) {
                    allocation_id = requested_alloc;
                } else {
                    allocation_id = "_"+allocation_id;
                    allocation_id = requested_alloc+allocation_id;
                }
                tuner_alloc["FRONTEND::tuner_allocation::allocation_id"] = allocation_id;
            }
        }*/
    }
    if (local_props.find("FRONTEND::tuner_allocation") != local_props.end()) {
        redhawk::PropertyMap& tuner_alloc = redhawk::PropertyMap::cast(local_props["FRONTEND::tuner_allocation"].asProperties());
        if (tuner_alloc.find("FRONTEND::tuner_allocation::allocation_id") != tuner_alloc.end()) {
            std::string requested_alloc = tuner_alloc["FRONTEND::tuner_allocation::allocation_id"].toString();
            if (not requested_alloc.empty()) {
                if (_delegatedAllocations.find(requested_alloc) == _delegatedAllocations.end()) {
                    allocation_id = requested_alloc;
                } else {
                    allocation_id = "_"+allocation_id;
                    allocation_id = requested_alloc+allocation_id;
                }
                tuner_alloc["FRONTEND::tuner_allocation::allocation_id"] = allocation_id;
            }
        }
    }
    /*if (local_props.find("FRONTEND::tuner_allocation") != local_props.end()) {
        redhawk::PropertyMap& tuner_alloc = redhawk::PropertyMap::cast(local_props["FRONTEND::tuner_allocation"].asProperties());
        if (tuner_alloc.find("FRONTEND::tuner_allocation::tuner_type") != tuner_alloc.end()) {
            std::string requested_device = tuner_alloc["FRONTEND::tuner_allocation::tuner_type"].toString();
            if (not requested_alloc.empty()) {
                if (_delegatedAllocations.find(requested_alloc) == _delegatedAllocations.end()) {
                    allocation_id = requested_alloc;
                } else {
                    allocation_id = "_"+allocation_id;
                    allocation_id = requested_alloc+allocation_id;
                }
                tuner_alloc["FRONTEND::tuner_allocation::allocation_id"] = allocation_id;
            }
        }
    }*/

    // Verify that the device is in a valid state
    if (!isUnlocked() || isDisabled() || isError()) {
        const char* invalidState;
        if (isLocked()) {
            invalidState = "LOCKED";
        } else if (isDisabled()) {
            invalidState = "DISABLED";
        } else if (isError()) {
            invalidState = "ERROR";
        } else {
            invalidState = "SHUTTING_DOWN";
        }
        throw CF::Device::InvalidState(invalidState);
    }

    for (std::vector<RDC_i*>::iterator it=RDCs.begin(); it!=RDCs.end(); it++) {
        result = (*it)->allocate(local_capacities);
        if (result->length() > 0) {
            _delegatedAllocations[allocation_id] = *it;
            return result._retn();
        }
    }

    for (std::vector<TDC_i*>::iterator it=TDCs.begin(); it!=TDCs.end(); it++) {
        result = (*it)->allocate(local_capacities);
        if (result->length() > 0) {
            _delegatedAllocations[allocation_id] = *it;
            return result._retn();
        }
    }

    return result._retn();
}

void USRP_i::deallocate (const char* alloc_id)
throw (CF::Device::InvalidState, CF::Device::InvalidCapacity, CORBA::SystemException)
{
    std::string _alloc_id = ossie::corba::returnString(alloc_id);
    if (_delegatedAllocations.find(_alloc_id) != _delegatedAllocations.end()) {
        Device_impl* dev = dynamic_cast<Device_impl*>(_delegatedAllocations[_alloc_id]);
        if (dev) {
            dev->deallocate(alloc_id);
            return;
        }
    }
    CF::Properties invalidProps;
    throw CF::Device::InvalidCapacity("Capacities do not match allocated ones in the child devices", invalidProps);
}

/*************************************************************
Functions supporting tuning allocation
*************************************************************/
void USRP_i::deviceEnable(frontend_tuner_status_struct_struct &fts, size_t tuner_id){
    /************************************************************
     *  not used. All allocations delegated to children
    ************************************************************/
    return;
}
void USRP_i::deviceDisable(frontend_tuner_status_struct_struct &fts, size_t tuner_id){
    /************************************************************
     *  not used. All allocations delegated to children
    ************************************************************/
    return;
}
bool USRP_i::deviceSetTuningScan(const frontend::frontend_tuner_allocation_struct &request, const frontend::frontend_scanner_allocation_struct &scan_request, frontend_tuner_status_struct_struct &fts, size_t tuner_id){
    /************************************************************
     *  not used. All allocations delegated to children
    ************************************************************/
    return false;
}
bool USRP_i::deviceSetTuning(const frontend::frontend_tuner_allocation_struct &request, frontend_tuner_status_struct_struct &fts, size_t tuner_id){
    /************************************************************
     *  not used. All allocations delegated to children
    ************************************************************/
    return false;
}
bool USRP_i::deviceDeleteTuning(frontend_tuner_status_struct_struct &fts, size_t tuner_id) {
    /************************************************************
     *  not used. All allocations delegated to children
    ************************************************************/
    return true;
}
/*************************************************************
Functions servicing the tuner control port
*************************************************************/
std::string USRP_i::getTunerType(const std::string& allocation_id) {
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    return frontend_tuner_status[idx].tuner_type;
}

bool USRP_i::getTunerDeviceControl(const std::string& allocation_id) {
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    if (getControlAllocationId(idx) == allocation_id)
        return true;
    return false;
}

std::string USRP_i::getTunerGroupId(const std::string& allocation_id) {
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    return frontend_tuner_status[idx].group_id;
}

std::string USRP_i::getTunerRfFlowId(const std::string& allocation_id) {
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    return frontend_tuner_status[idx].rf_flow_id;
}

void USRP_i::setTunerCenterFrequency(const std::string& allocation_id, double freq) {
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    if(allocation_id != getControlAllocationId(idx))
        throw FRONTEND::FrontendException(("ID "+allocation_id+" does not have authorization to modify the tuner").c_str());
    if (freq<0) throw FRONTEND::BadParameterException("Center frequency cannot be less than 0");
    // set hardware to new value. Raise an exception if it's not possible
    this->frontend_tuner_status[idx].center_frequency = freq;
}

double USRP_i::getTunerCenterFrequency(const std::string& allocation_id) {
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    return frontend_tuner_status[idx].center_frequency;
}

void USRP_i::setTunerBandwidth(const std::string& allocation_id, double bw) {
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    if(allocation_id != getControlAllocationId(idx))
        throw FRONTEND::FrontendException(("ID "+allocation_id+" does not have authorization to modify the tuner").c_str());
    if (bw<0) throw FRONTEND::BadParameterException("Bandwidth cannot be less than 0");
    // set hardware to new value. Raise an exception if it's not possible
    this->frontend_tuner_status[idx].bandwidth = bw;
}

double USRP_i::getTunerBandwidth(const std::string& allocation_id) {
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    return frontend_tuner_status[idx].bandwidth;
}

void USRP_i::setTunerAgcEnable(const std::string& allocation_id, bool enable)
{
    throw FRONTEND::NotSupportedException("setTunerAgcEnable not supported");
}

bool USRP_i::getTunerAgcEnable(const std::string& allocation_id)
{
    throw FRONTEND::NotSupportedException("getTunerAgcEnable not supported");
}

void USRP_i::setTunerGain(const std::string& allocation_id, float gain)
{
    throw FRONTEND::NotSupportedException("setTunerGain not supported");
}

float USRP_i::getTunerGain(const std::string& allocation_id)
{
    throw FRONTEND::NotSupportedException("getTunerGain not supported");
}

void USRP_i::setTunerReferenceSource(const std::string& allocation_id, long source)
{
    throw FRONTEND::NotSupportedException("setTunerReferenceSource not supported");
}

long USRP_i::getTunerReferenceSource(const std::string& allocation_id)
{
    throw FRONTEND::NotSupportedException("getTunerReferenceSource not supported");
}

void USRP_i::setTunerEnable(const std::string& allocation_id, bool enable) {
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    if(allocation_id != getControlAllocationId(idx))
        throw FRONTEND::FrontendException(("ID "+allocation_id+" does not have authorization to modify the tuner").c_str());
    // set hardware to new value. Raise an exception if it's not possible
    this->frontend_tuner_status[idx].enabled = enable;
}

bool USRP_i::getTunerEnable(const std::string& allocation_id) {
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    return frontend_tuner_status[idx].enabled;
}

void USRP_i::setTunerOutputSampleRate(const std::string& allocation_id, double sr) {
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    if(allocation_id != getControlAllocationId(idx))
        throw FRONTEND::FrontendException(("ID "+allocation_id+" does not have authorization to modify the tuner").c_str());
    if (sr<0) throw FRONTEND::BadParameterException("Sample rate cannot be less than 0");
    // set hardware to new value. Raise an exception if it's not possible
    this->frontend_tuner_status[idx].sample_rate = sr;
}

double USRP_i::getTunerOutputSampleRate(const std::string& allocation_id){
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    return frontend_tuner_status[idx].sample_rate;
}
frontend::ScanStatus USRP_i::getScanStatus(const std::string& allocation_id) {
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    frontend::ManualStrategy* tmp = new frontend::ManualStrategy(0);
    frontend::ScanStatus retval(tmp);
    return retval;
}

void USRP_i::setScanStartTime(const std::string& allocation_id, const BULKIO::PrecisionUTCTime& start_time) {
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    if(allocation_id != getControlAllocationId(idx))
        throw FRONTEND::FrontendException(("ID "+allocation_id+" does not have authorization to modify the tuner").c_str());
}

void USRP_i::setScanStrategy(const std::string& allocation_id, const frontend::ScanStrategy* scan_strategy) {
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    if(allocation_id != getControlAllocationId(idx))
        throw FRONTEND::FrontendException(("ID "+allocation_id+" does not have authorization to modify the tuner").c_str());
}

/*************************************************************
Functions servicing the RFInfo port(s)
- port_name is the port over which the call was received
*************************************************************/
std::string USRP_i::get_rf_flow_id(const std::string& port_name)
{
    return std::string("none");
}

void USRP_i::set_rf_flow_id(const std::string& port_name, const std::string& id)
{
}

frontend::RFInfoPkt USRP_i::get_rfinfo_pkt(const std::string& port_name)
{
    frontend::RFInfoPkt pkt;
    return pkt;
}

void USRP_i::set_rfinfo_pkt(const std::string& port_name, const frontend::RFInfoPkt &pkt)
{
}

