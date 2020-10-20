#ifndef TDC_BASE_IMPL_BASE_H
#define TDC_BASE_IMPL_BASE_H

#include <boost/thread.hpp>
#include <frontend/frontend.h>
#include <ossie/ThreadedComponent.h>
#include <ossie/DynamicComponent.h>

#include <frontend/frontend.h>
#include <bulkio/bulkio.h>
//#include "TDC_struct_props.h"
#include "../struct_props.h"

#define BOOL_VALUE_HERE 0

class TDC_base : public frontend::FrontendTunerDevice<frontend_tuner_status_struct_struct>, public virtual frontend::transmit_control_delegation, protected ThreadedComponent, public virtual DynamicComponent
{
    public:
        TDC_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl);
        TDC_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, char *compDev);
        TDC_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities);
        TDC_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities, char *compDev);
        ~TDC_base();

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
        /// Property: device_gain
        float device_gain;
        /// Property: device_mode
        std::string device_mode;
        /// Property: device_characteristics
        device_characteristics_struct device_characteristics;

        // Ports
        /// Port: TransmitControl_in
        frontend::InTransmitControlPort *TransmitControl_in;
        /// Port: dataShortTX_in
        bulkio::InShortPort *dataShortTX_in;
        /// Port: TransmitDeviceStatus_out
        frontend::OutTransmitDeviceStatusPort *TransmitDeviceStatus_out;
        /// Port: RFInfoTX_out
        frontend::OutRFInfoPort *RFInfoTX_out;

        std::map<std::string, std::string> listeners;

    private:
        void construct();
};
#endif // TDC_BASE_IMPL_BASE_H
