#include "RDC_base.h"

/*******************************************************************************************

    AUTO-GENERATED CODE. DO NOT MODIFY

    The following class functions are for the base class for the device class. To
    customize any of these functions, do not modify them here. Instead, overload them
    on the child class

******************************************************************************************/

RDC_base::RDC_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl) :
    frontend::FrontendTunerDevice<frontend_tuner_status_struct_struct>(devMgr_ior, id, lbl, sftwrPrfl),
    ThreadedComponent()
{
    construct();
}

RDC_base::RDC_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, char *compDev) :
    frontend::FrontendTunerDevice<frontend_tuner_status_struct_struct>(devMgr_ior, id, lbl, sftwrPrfl, compDev),
    ThreadedComponent()
{
    construct();
}

RDC_base::RDC_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities) :
    frontend::FrontendTunerDevice<frontend_tuner_status_struct_struct>(devMgr_ior, id, lbl, sftwrPrfl, capacities),
    ThreadedComponent()
{
    construct();
}

RDC_base::RDC_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities, char *compDev) :
    frontend::FrontendTunerDevice<frontend_tuner_status_struct_struct>(devMgr_ior, id, lbl, sftwrPrfl, capacities, compDev),
    ThreadedComponent()
{
    construct();
}

RDC_base::~RDC_base()
{
    RFInfo_in->_remove_ref();
    RFInfo_in = 0;
    DigitalTuner_in->_remove_ref();
    DigitalTuner_in = 0;
    dataShort_out->_remove_ref();
    dataShort_out = 0;
    dataSDDS_out->_remove_ref();
    dataSDDS_out = 0;
}

void RDC_base::construct()
{
    loadProperties();

    RFInfo_in = new frontend::InRFInfoPort("RFInfo_in", this);
    RFInfo_in->setLogger(this->_baseLog->getChildLogger("RFInfo_in", "ports"));
    addPort("RFInfo_in", RFInfo_in);
    DigitalTuner_in = new frontend::InDigitalTunerPort("DigitalTuner_in", this);
    DigitalTuner_in->setLogger(this->_baseLog->getChildLogger("DigitalTuner_in", "ports"));
    addPort("DigitalTuner_in", DigitalTuner_in);
    dataShort_out = new bulkio::OutShortPort("dataShort_out");
    dataShort_out->setLogger(this->_baseLog->getChildLogger("dataShort_out", "ports"));
    addPort("dataShort_out", dataShort_out);
    dataSDDS_out = new bulkio::OutSDDSPort("dataSDDS_out");
    dataSDDS_out->setLogger(this->_baseLog->getChildLogger("dataSDDS_out", "ports"));
    addPort("dataSDDS_out", dataSDDS_out);
    this->setHost(this);

}

/*******************************************************************************************
    Framework-level functions
    These functions are generally called by the framework to perform housekeeping.
*******************************************************************************************/
void RDC_base::start() throw (CORBA::SystemException, CF::Resource::StartError)
{
    frontend::FrontendTunerDevice<frontend_tuner_status_struct_struct>::start();
    ThreadedComponent::startThread();
}

void RDC_base::stop() throw (CORBA::SystemException, CF::Resource::StopError)
{
    frontend::FrontendTunerDevice<frontend_tuner_status_struct_struct>::stop();
    if (!ThreadedComponent::stopThread()) {
        throw CF::Resource::StopError(CF::CF_NOTSET, "Processing thread did not die");
    }
}

void RDC_base::releaseObject() throw (CORBA::SystemException, CF::LifeCycle::ReleaseError)
{
    // This function clears the device running condition so main shuts down everything
    try {
        stop();
    } catch (CF::Resource::StopError& ex) {
        // TODO - this should probably be logged instead of ignored
    }

    frontend::FrontendTunerDevice<frontend_tuner_status_struct_struct>::releaseObject();
}

void RDC_base::loadProperties()
{
    addProperty(device_kind,
                "FRONTEND::TUNER",
                "DCE:cdc5ee18-7ceb-4ae6-bf4c-31f983179b4d",
                "device_kind",
                "readonly",
                "",
                "eq",
                "allocation");

    addProperty(device_model,
                "DCE:0f99b2e4-9903-4631-9846-ff349d18ecfb",
                "device_model",
                "readonly",
                "",
                "eq",
                "allocation");

    addProperty(frontend_listener_allocation,
                frontend::frontend_listener_allocation_struct(),
                "FRONTEND::listener_allocation",
                "frontend_listener_allocation",
                "writeonly",
                "",
                "external",
                "allocation");

    addProperty(frontend_tuner_allocation,
                frontend::frontend_tuner_allocation_struct(),
                "FRONTEND::tuner_allocation",
                "frontend_tuner_allocation",
                "writeonly",
                "",
                "external",
                "allocation");

    addProperty(frontend_tuner_status,
                "FRONTEND::tuner_status",
                "frontend_tuner_status",
                "readonly",
                "",
                "external",
                "property");

}

CF::Properties* RDC_base::getTunerStatus(const std::string& allocation_id)
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

void RDC_base::frontendTunerStatusChanged(const std::vector<frontend_tuner_status_struct_struct>* oldValue, const std::vector<frontend_tuner_status_struct_struct>* newValue)
{
    this->tuner_allocation_ids.resize(this->frontend_tuner_status.size());
}

void RDC_base::assignListener(const std::string& listen_alloc_id, const std::string& allocation_id)
{
    // find control allocation_id
    std::string existing_alloc_id = allocation_id;
    std::map<std::string,std::string>::iterator existing_listener;
    while ((existing_listener=listeners.find(existing_alloc_id)) != listeners.end())
        existing_alloc_id = existing_listener->second;
    listeners[listen_alloc_id] = existing_alloc_id;

}

void RDC_base::removeListener(const std::string& listen_alloc_id)
{
    if (listeners.find(listen_alloc_id) != listeners.end()) {
        listeners.erase(listen_alloc_id);
    }
}
void RDC_base::removeAllocationIdRouting(const size_t tuner_id) {
}
