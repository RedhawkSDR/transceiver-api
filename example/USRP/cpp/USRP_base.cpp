#include "USRP_base.h"

/*******************************************************************************************

    AUTO-GENERATED CODE. DO NOT MODIFY

    The following class functions are for the base class for the device class. To
    customize any of these functions, do not modify them here. Instead, overload them
    on the child class

******************************************************************************************/

USRP_base::USRP_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl) :
    frontend::FrontendScanningTunerDevice<frontend_tuner_status_struct_struct>(devMgr_ior, id, lbl, sftwrPrfl),
    AggregateDevice_impl(),
    ThreadedComponent()
{
    construct();
}

USRP_base::USRP_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, char *compDev) :
    frontend::FrontendScanningTunerDevice<frontend_tuner_status_struct_struct>(devMgr_ior, id, lbl, sftwrPrfl, compDev),
    AggregateDevice_impl(),
    ThreadedComponent()
{
    construct();
}

USRP_base::USRP_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities) :
    frontend::FrontendScanningTunerDevice<frontend_tuner_status_struct_struct>(devMgr_ior, id, lbl, sftwrPrfl, capacities),
    AggregateDevice_impl(),
    ThreadedComponent()
{
    construct();
}

USRP_base::USRP_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities, char *compDev) :
    frontend::FrontendScanningTunerDevice<frontend_tuner_status_struct_struct>(devMgr_ior, id, lbl, sftwrPrfl, capacities, compDev),
    AggregateDevice_impl(),
    ThreadedComponent()
{
    construct();
}

USRP_base::~USRP_base()
{
    RFInfo_in->_remove_ref();
    RFInfo_in = 0;
    DigitalTuner_in->_remove_ref();
    DigitalTuner_in = 0;
    RFInfo_out->_remove_ref();
    RFInfo_out = 0;
}

void USRP_base::construct()
{
    loadProperties();

    RFInfo_in = new frontend::InRFInfoPort("RFInfo_in", this);
    RFInfo_in->setLogger(this->_baseLog->getChildLogger("RFInfo_in", "ports"));
    addPort("RFInfo_in", RFInfo_in);
    DigitalTuner_in = new frontend::InDigitalScanningTunerPort("DigitalTuner_in", this);
    DigitalTuner_in->setLogger(this->_baseLog->getChildLogger("DigitalTuner_in", "ports"));
    addPort("DigitalTuner_in", DigitalTuner_in);
    RFInfo_out = new frontend::OutRFInfoPort("RFInfo_out");
    RFInfo_out->setLogger(this->_baseLog->getChildLogger("RFInfo_out", "ports"));
    addPort("RFInfo_out", RFInfo_out);
    this->setHost(this);

}

/*******************************************************************************************
    Framework-level functions
    These functions are generally called by the framework to perform housekeeping.
*******************************************************************************************/
void USRP_base::start() throw (CORBA::SystemException, CF::Resource::StartError)
{
    frontend::FrontendScanningTunerDevice<frontend_tuner_status_struct_struct>::start();
    ThreadedComponent::startThread();
}

void USRP_base::stop() throw (CORBA::SystemException, CF::Resource::StopError)
{
    frontend::FrontendScanningTunerDevice<frontend_tuner_status_struct_struct>::stop();
    if (!ThreadedComponent::stopThread()) {
        throw CF::Resource::StopError(CF::CF_NOTSET, "Processing thread did not die");
    }
}

void USRP_base::releaseObject() throw (CORBA::SystemException, CF::LifeCycle::ReleaseError)
{
    // This function clears the device running condition so main shuts down everything
    try {
        stop();
    } catch (CF::Resource::StopError& ex) {
        // TODO - this should probably be logged instead of ignored
    }

    frontend::FrontendScanningTunerDevice<frontend_tuner_status_struct_struct>::releaseObject();
}

void USRP_base::loadProperties()
{
    device_kind = "FRONTEND::TUNER";
    addProperty(device_reference_source_global,
                "INTERNAL",
                "device_reference_source_global",
                "device_reference_source_global",
                "readwrite",
                "",
                "external",
                "property");

    addProperty(clock_sync,
                false,
                "clock_sync",
                "",
                "readonly",
                "",
                "external",
                "property");

    addProperty(ip_address,
                "",
                "ip_address",
                "",
                "readwrite",
                "",
                "external",
                "property");

    addProperty(frontend_coherent_feeds,
                "FRONTEND::coherent_feeds",
                "frontend_coherent_feeds",
                "readwrite",
                "",
                "external",
                "allocation");

    frontend_listener_allocation = frontend::frontend_listener_allocation_struct();
    frontend_tuner_allocation = frontend::frontend_tuner_allocation_struct();
    frontend_scanner_allocation = frontend::frontend_scanner_allocation_struct();
    addProperty(device_characteristics,
                device_characteristics_struct(),
                "device_characteristics",
                "device_characteristics",
                "readonly",
                "",
                "external",
                "property");

}

CF::Properties* USRP_base::getTunerStatus(const std::string& allocation_id)
{
    CF::Properties* tmpVal = new CF::Properties();
    long tuner_id = getTunerMapping(allocation_id);
    if (tuner_id < 0)
        throw FRONTEND::FrontendException(("ERROR: ID: " + std::string(allocation_id) + " IS NOT ASSOCIATED WITH ANY TUNER!").c_str());
    CORBA::Any prop;
    prop <<= *(static_cast<frontend_tuner_status_struct_struct*>(&this->frontend_tuner_status[tuner_id]));
    prop >>= tmpVal;

    CF::Properties_var tmp = new CF::Properties(*tmpVal);
    return tmp._retn();
}

void USRP_base::frontendTunerStatusChanged(const std::vector<frontend_tuner_status_struct_struct>* oldValue, const std::vector<frontend_tuner_status_struct_struct>* newValue)
{
    this->tuner_allocation_ids.resize(this->frontend_tuner_status.size());
}

void USRP_base::assignListener(const std::string& listen_alloc_id, const std::string& allocation_id)
{
    // find control allocation_id
    std::string existing_alloc_id = allocation_id;
    std::map<std::string,std::string>::iterator existing_listener;
    while ((existing_listener=listeners.find(existing_alloc_id)) != listeners.end())
        existing_alloc_id = existing_listener->second;
    listeners[listen_alloc_id] = existing_alloc_id;

}

void USRP_base::removeListener(const std::string& listen_alloc_id)
{
    if (listeners.find(listen_alloc_id) != listeners.end()) {
        listeners.erase(listen_alloc_id);
    }
}
void USRP_base::removeAllocationIdRouting(const size_t tuner_id) {
}

