#ifndef UHD_ACCESS_H
#define UHD_ACCESS_H

#include <uhd/usrp/multi_usrp.hpp>

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

#endif // UHD_ACCESS_H
