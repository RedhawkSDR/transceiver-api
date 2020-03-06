#ifndef USRP_BASE_IMPL_BASE_H
#define USRP_BASE_IMPL_BASE_H

#include <boost/thread.hpp>
#include <frontend/frontend.h>
#include <CF/AggregateDevices.h>
#include <ossie/AggregateDevice_impl.h>
#include <ossie/ThreadedComponent.h>
#include <ossie/DynamicComponent.h>

#include <frontend/frontend.h>
#include "struct_props.h"
#include "TDC/TDC.h"
#include "RDC/RDC.h"

#define BOOL_VALUE_HERE 0

namespace enums {
    // Enumerated values for device_reference_source_global
    namespace device_reference_source_global {
        static const std::string INTERNAL = "INTERNAL";
        static const std::string EXTERNAL = "EXTERNAL";
        static const std::string MIMO = "MIMO";
        static const std::string GPSDO = "GPSDO";
    }
}

class USRP_base : public frontend::FrontendScanningTunerDevice<frontend_tuner_status_struct_struct>, public virtual POA_CF::AggregatePlainDevice, public AggregateDevice_impl, public virtual frontend::digital_scanning_tuner_delegation, public virtual frontend::rfinfo_delegation, protected ThreadedComponent, public virtual DynamicComponent
{
    public:
        USRP_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl);
        USRP_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, char *compDev);
        USRP_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities);
        USRP_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities, char *compDev);
        ~USRP_base();

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
        /// Property: device_reference_source_global
        std::string device_reference_source_global;
        /// Property: clock_sync
        bool clock_sync;
        /// Property: ip_address
        std::string ip_address;
        /// Property: frontend_coherent_feeds
        std::vector<std::string> frontend_coherent_feeds;
        /// Property: device_characteristics
        device_characteristics_struct device_characteristics;

        // Ports
        /// Port: RFInfo_in
        frontend::InRFInfoPort *RFInfo_in;
        /// Port: DigitalTuner_in
        frontend::InDigitalScanningTunerPort *DigitalTuner_in;
        /// Port: RFInfo_out
        frontend::OutRFInfoPort *RFInfo_out;

        std::map<std::string, std::string> listeners;

    private:
        void construct();
};
#endif // USRP_BASE_IMPL_BASE_H
