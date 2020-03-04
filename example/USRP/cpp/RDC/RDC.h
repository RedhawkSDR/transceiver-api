#ifndef RDC_I_IMPL_H
#define RDC_I_IMPL_H

#include "RDC_base.h"
#include <uhd/usrp/multi_usrp.hpp>

/** Device Individual Tuner. This structure contains stream specific data for channel/tuner to include:
 *      - Data buffer
 *      - Additional stream metadata (timestamps)
 */
typedef struct ticket_lock {
    ticket_lock(){
        cond=NULL;
        mutex=NULL;
        queue_head=queue_tail=0;
    }
    boost::condition_variable* cond;
    boost::mutex* mutex;
    size_t queue_head, queue_tail;
} ticket_lock_t;

class scoped_tuner_lock{
    public:
        scoped_tuner_lock(ticket_lock_t& _ticket){
            ticket = &_ticket;

            boost::mutex::scoped_lock lock(*ticket->mutex);
            queue_me = ticket->queue_tail++;
            while (queue_me != ticket->queue_head)
            {
                ticket->cond->wait(lock);
            }
        }
        ~scoped_tuner_lock(){
            boost::mutex::scoped_lock lock(*ticket->mutex);
            ticket->queue_head++;
            ticket->cond->notify_all();
        }
    private:
        ticket_lock_t* ticket;
        size_t queue_me;
};

struct usrpTunerStruct {
    usrpTunerStruct(){

        setDefaultBufferSize();

        reset();
    }

    void setDefaultBufferSize() {
        // size buffer within CORBA transfer limits
        // Multiply by some number < 1 to leave some margin for the CORBA header
        // fyi: the bulkio pushPacket call does this same calculation as of 1.10,
        //      so we'll only require a single pushPacket call per buffer
        // Also, since data is complex, ensure number of samples is even
        // Since SDDS output was added, we're making the buffer a multiple of 1024,
        // which also satsifies the "even" requirement for complex samples.
        const size_t max_payload_size    = (size_t) (bulkio::Const::MaxTransferBytes() * .9);
        const size_t max_samples_per_push = (size_t((max_payload_size/sizeof(output_buffer[0]))/1024)*1024);

        buffer_capacity = max_samples_per_push;
        output_buffer.resize( buffer_capacity );
    }

    bool updateBufferSize(size_t newMaxSamplesPerPush) {
        bool retVal = false;
        const size_t max_payload_size    = (size_t) (bulkio::Const::MaxTransferBytes() * .9);
        const size_t max_samples_per_push = (size_t((max_payload_size/sizeof(output_buffer[0]))/1024)*1024);
        // first sanity check
        if (newMaxSamplesPerPush < max_samples_per_push) {
            size_t remainder = newMaxSamplesPerPush % 1024;
            size_t newVal = newMaxSamplesPerPush;
            if (remainder > 0) {
                // Round value to nearest 1024
                newVal = newVal + 1024 - remainder;
            }
            if (newVal <= max_samples_per_push) {
                buffer_capacity = newVal;
                output_buffer.resize( buffer_capacity );
                retVal = true;
            } else {
                std::cerr << "Invalid newMaxSamplesPerPush: " << newMaxSamplesPerPush << " would be too big for transport max sample size of " << max_samples_per_push << std::endl;
            }
        }  else {
            std::cerr << "Invalid newMaxSamplesPerPush: " << newMaxSamplesPerPush << " would be too big for transport max sample size of " << max_samples_per_push << std::endl;
        }
        return retVal;
    }

    redhawk::buffer<short> output_buffer;
    size_t buffer_capacity; // num samps buffer can hold
    size_t buffer_size; // num samps in buffer
    BULKIO::PrecisionUTCTime output_buffer_time;
    BULKIO::PrecisionUTCTime time_up;
    BULKIO::PrecisionUTCTime time_down;
    bool update_sri;
    ticket_lock_t lock;

    void reset(){
        buffer_size = 0;
        bulkio::sri::zeroTime(output_buffer_time);
        bulkio::sri::zeroTime(time_up);
        bulkio::sri::zeroTime(time_down);
        update_sri = false;
    }
};

class RDC_i : public RDC_base
{
    ENABLE_LOGGING

    public:
        struct usrpRangesStruct {
            usrpRangesStruct(){
                reset();
            }

            uhd::meta_range_t frequency;
            uhd::meta_range_t bandwidth;
            uhd::meta_range_t sample_rate;
            uhd::meta_range_t gain;

            void reset(){
                frequency.clear();
                bandwidth.clear();
                sample_rate.clear();
                gain.clear();
            };
        };

    public:
        RDC_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl);
        RDC_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, char *compDev);
        RDC_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities);
        RDC_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities, char *compDev);
        ~RDC_i();

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
        std::string get_rf_flow_id(const std::string& port_name);
        void set_rf_flow_id(const std::string& port_name, const std::string& id);
        frontend::RFInfoPkt get_rfinfo_pkt(const std::string& port_name);
        void set_rfinfo_pkt(const std::string& port_name, const frontend::RFInfoPkt& pkt);

        uhd::rx_streamer::sptr usrp_rx_streamer;
        usrpTunerStruct usrp_tuner; // data buffer/timestamps, lock
        bool usrpCreateRxStream();
        int _tuner_number;
        std::string _stream_id;
        uhd::usrp::multi_usrp::sptr usrp_device_ptr;
        long usrpReceive(double timeout);
        float auto_gain();
        void updateDeviceRxGain(double gain, bool lock);
        void getStreamId();
        bool usrpEnable();
        usrpRangesStruct usrp_range;    // freq/bw/sr/gain ranges supported by each tuner channel
                                        // indices map to tuner_id
                                        // protected by prop_lock
        double optimizeRate(const double& req_rate);
        double optimizeBandwidth(const double& req_bw);

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

#endif // RDC_I_IMPL_H
