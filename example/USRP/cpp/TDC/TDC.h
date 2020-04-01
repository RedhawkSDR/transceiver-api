#ifndef TDC_I_IMPL_H
#define TDC_I_IMPL_H

#include "TDC_base.h"
#include <uhd/usrp/multi_usrp.hpp>
#include "../uhd_access.h"

class TDC_i : public TDC_base
{
    ENABLE_LOGGING
    public:
        TDC_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl);
        TDC_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, char *compDev);
        TDC_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities);
        TDC_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities, char *compDev);
        ~TDC_i();

        void constructor();

        int serviceFunction();

        void setTunerNumber(size_t tuner_number);
        void setUHDptr(const uhd::usrp::multi_usrp::sptr parent_device_ptr);
        void updateDeviceCharacteristics();

    protected:
        std::string getTunerType(const std::string& allocation_id);
        bool getTunerDeviceControl(const std::string& allocation_id);
        std::string getTunerGroupId(const std::string& allocation_id);
        std::string getTunerRfFlowId(const std::string& allocation_id);
        double getTunerCenterFrequency(const std::string& allocation_id);
        void setTunerCenterFrequency(const std::string& allocation_id, double freq);
        double getTunerBandwidth(const std::string& allocation_id);
        void setTunerBandwidth(const std::string& allocation_id, double bw);
        bool getTunerAgcEnable(const std::string& allocation_id);
        void setTunerAgcEnable(const std::string& allocation_id, bool enable);
        float getTunerGain(const std::string& allocation_id);
        void setTunerGain(const std::string& allocation_id, float gain);
        long getTunerReferenceSource(const std::string& allocation_id);
        void setTunerReferenceSource(const std::string& allocation_id, long source);
        bool getTunerEnable(const std::string& allocation_id);
        void setTunerEnable(const std::string& allocation_id, bool enable);
        double getTunerOutputSampleRate(const std::string& allocation_id);
        void setTunerOutputSampleRate(const std::string& allocation_id, double sr);

        uhd::tx_streamer::sptr usrp_tx_streamer;
        usrpTunerStruct usrp_tuner; // data buffer/timestamps, lock
        bool usrpCreateTxStream();
        bool usrpEnable();
        bool usrpTransmit();
        int _tuner_number;
        std::string _stream_id;
        uhd::usrp::multi_usrp::sptr usrp_device_ptr;
        usrpRangesStruct usrp_range;    // freq/bw/sr/gain ranges supported by each tuner channel
                                        // indices map to tuner_id
                                        // protected by prop_lock
        size_t usrp_tx_streamer_typesize;  // leftover from when usrp input had multiple types
        double optimizeRate(const double& req_rate);
        double optimizeBandwidth(const double& req_bw);

        void verifyHWStatus(const std::string &stream_id, const BULKIO::PrecisionUTCTime &rightnow);
        void verifyQueueStatus(const std::string &stream_id, const BULKIO::PrecisionUTCTime &rightnow, const std::vector<bulkio::StreamStatus> &error_status);

    private:
        ////////////////////////////////////////
        // Required device specific functions // -- to be implemented by device developer
        ////////////////////////////////////////

        // these are pure virtual, must be implemented here
        void deviceEnable(frontend_tuner_status_struct_struct &fts, size_t tuner_id);
        void deviceDisable(frontend_tuner_status_struct_struct &fts, size_t tuner_id);
        bool deviceSetTuning(const frontend::frontend_tuner_allocation_struct &request, frontend_tuner_status_struct_struct &fts, size_t tuner_id);
        bool deviceDeleteTuning(frontend_tuner_status_struct_struct &fts, size_t tuner_id);

};

#endif // TDC_I_IMPL_H
