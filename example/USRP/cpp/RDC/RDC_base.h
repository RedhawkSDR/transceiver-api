#ifndef RDC_BASE_IMPL_BASE_H
#define RDC_BASE_IMPL_BASE_H

#include <boost/thread.hpp>
#include <frontend/frontend.h>
#include <ossie/ThreadedComponent.h>
#include <ossie/DynamicComponent.h>

#include <frontend/frontend.h>
#include <bulkio/bulkio.h>
#include "struct_props.h"

#define BOOL_VALUE_HERE 0

class RDC_base : public frontend::FrontendTunerDevice<frontend_tuner_status_struct_struct>, public virtual frontend::digital_tuner_delegation, public virtual frontend::rfinfo_delegation, protected ThreadedComponent, public virtual DynamicComponent
{
    public:
        RDC_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl);
        RDC_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, char *compDev);
        RDC_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities);
        RDC_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities, char *compDev);
        ~RDC_base();

        void start() throw (CF::Resource::StartError, CORBA::SystemException);

        void stop() throw (CF::Resource::StopError, CORBA::SystemException);

        void releaseObject() throw (CF::LifeCycle::ReleaseError, CORBA::SystemException);

        void loadProperties();
        void removeAllocationIdRouting(const size_t tuner_id);

        virtual CF::Properties* getTunerStatus(const std::string& allocation_id);
        virtual void assignListener(const std::string& listen_alloc_id, const std::string& allocation_id);
        virtual void removeListener(const std::string& listen_alloc_id);
        void frontendTunerStatusChanged(const std::vector<frontend_tuner_status_struct_struct>* oldValue, const std::vector<frontend_tuner_status_struct_struct>* newValue);

    protected:
        // Member variables exposed as properties
        /// Property: device_kind
        std::string device_kind;
        /// Property: device_model
        std::string device_model;
        /// Property: frontend_listener_allocation
        frontend::frontend_listener_allocation_struct frontend_listener_allocation;
        /// Property: frontend_tuner_allocation
        frontend::frontend_tuner_allocation_struct frontend_tuner_allocation;
        /// Property: frontend_tuner_status
        std::vector<frontend_tuner_status_struct_struct> frontend_tuner_status;

        // Ports
        /// Port: RFInfo_in
        frontend::InRFInfoPort *RFInfo_in;
        /// Port: DigitalTuner_in
        frontend::InDigitalTunerPort *DigitalTuner_in;
        /// Port: dataShort_out
        bulkio::OutShortPort *dataShort_out;

        std::map<std::string, std::string> listeners;

    private:
        void construct();
};
#endif // RDC_BASE_IMPL_BASE_H
