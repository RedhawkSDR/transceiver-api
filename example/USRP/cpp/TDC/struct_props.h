#ifndef STRUCTPROPS_H
#define STRUCTPROPS_H

/*******************************************************************************************

    AUTO-GENERATED CODE. DO NOT MODIFY

*******************************************************************************************/

#include <ossie/CorbaUtils.h>
#include <CF/cf.h>
#include <ossie/PropertyMap.h>

#include <frontend/fe_tuner_struct_props.h>

struct frontend_transmitter_allocation_struct {
    frontend_transmitter_allocation_struct ()
    {
    }

    static std::string getId() {
        return std::string("FRONTEND::transmitter_allocation");
    }

    static const char* getFormat() {
        return "dddd";
    }

    double min_freq;
    double max_freq;
    double control_limit;
    double max_power;
};

inline bool operator>>= (const CORBA::Any& a, frontend_transmitter_allocation_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("FRONTEND::transmitter_allocation::min_freq")) {
        if (!(props["FRONTEND::transmitter_allocation::min_freq"] >>= s.min_freq)) return false;
    }
    if (props.contains("FRONTEND::transmitter_allocation::max_freq")) {
        if (!(props["FRONTEND::transmitter_allocation::max_freq"] >>= s.max_freq)) return false;
    }
    if (props.contains("FRONTEND::transmitter_allocation::control_limit")) {
        if (!(props["FRONTEND::transmitter_allocation::control_limit"] >>= s.control_limit)) return false;
    }
    if (props.contains("FRONTEND::transmitter_allocation::max_power")) {
        if (!(props["FRONTEND::transmitter_allocation::max_power"] >>= s.max_power)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const frontend_transmitter_allocation_struct& s) {
    redhawk::PropertyMap props;
 
    props["FRONTEND::transmitter_allocation::min_freq"] = s.min_freq;
 
    props["FRONTEND::transmitter_allocation::max_freq"] = s.max_freq;
 
    props["FRONTEND::transmitter_allocation::control_limit"] = s.control_limit;
 
    props["FRONTEND::transmitter_allocation::max_power"] = s.max_power;
    a <<= props;
}

inline bool operator== (const frontend_transmitter_allocation_struct& s1, const frontend_transmitter_allocation_struct& s2) {
    if (s1.min_freq!=s2.min_freq)
        return false;
    if (s1.max_freq!=s2.max_freq)
        return false;
    if (s1.control_limit!=s2.control_limit)
        return false;
    if (s1.max_power!=s2.max_power)
        return false;
    return true;
}

inline bool operator!= (const frontend_transmitter_allocation_struct& s1, const frontend_transmitter_allocation_struct& s2) {
    return !(s1==s2);
}

struct frontend_tuner_status_struct_struct : public frontend::default_frontend_tuner_status_struct_struct {
    frontend_tuner_status_struct_struct () : frontend::default_frontend_tuner_status_struct_struct()
    {
    }

    static std::string getId() {
        return std::string("FRONTEND::tuner_status_struct");
    }

    static const char* getFormat() {
        return "sddbssds";
    }
};

inline bool operator>>= (const CORBA::Any& a, frontend_tuner_status_struct_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("FRONTEND::tuner_status::allocation_id_csv")) {
        if (!(props["FRONTEND::tuner_status::allocation_id_csv"] >>= s.allocation_id_csv)) return false;
    }
    if (props.contains("FRONTEND::tuner_status::bandwidth")) {
        if (!(props["FRONTEND::tuner_status::bandwidth"] >>= s.bandwidth)) return false;
    }
    if (props.contains("FRONTEND::tuner_status::center_frequency")) {
        if (!(props["FRONTEND::tuner_status::center_frequency"] >>= s.center_frequency)) return false;
    }
    if (props.contains("FRONTEND::tuner_status::enabled")) {
        if (!(props["FRONTEND::tuner_status::enabled"] >>= s.enabled)) return false;
    }
    if (props.contains("FRONTEND::tuner_status::group_id")) {
        if (!(props["FRONTEND::tuner_status::group_id"] >>= s.group_id)) return false;
    }
    if (props.contains("FRONTEND::tuner_status::rf_flow_id")) {
        if (!(props["FRONTEND::tuner_status::rf_flow_id"] >>= s.rf_flow_id)) return false;
    }
    if (props.contains("FRONTEND::tuner_status::sample_rate")) {
        if (!(props["FRONTEND::tuner_status::sample_rate"] >>= s.sample_rate)) return false;
    }
    if (props.contains("FRONTEND::tuner_status::tuner_type")) {
        if (!(props["FRONTEND::tuner_status::tuner_type"] >>= s.tuner_type)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const frontend_tuner_status_struct_struct& s) {
    redhawk::PropertyMap props;
 
    props["FRONTEND::tuner_status::allocation_id_csv"] = s.allocation_id_csv;
 
    props["FRONTEND::tuner_status::bandwidth"] = s.bandwidth;
 
    props["FRONTEND::tuner_status::center_frequency"] = s.center_frequency;
 
    props["FRONTEND::tuner_status::enabled"] = s.enabled;
 
    props["FRONTEND::tuner_status::group_id"] = s.group_id;
 
    props["FRONTEND::tuner_status::rf_flow_id"] = s.rf_flow_id;
 
    props["FRONTEND::tuner_status::sample_rate"] = s.sample_rate;
 
    props["FRONTEND::tuner_status::tuner_type"] = s.tuner_type;
    a <<= props;
}

inline bool operator== (const frontend_tuner_status_struct_struct& s1, const frontend_tuner_status_struct_struct& s2) {
    if (s1.allocation_id_csv!=s2.allocation_id_csv)
        return false;
    if (s1.bandwidth!=s2.bandwidth)
        return false;
    if (s1.center_frequency!=s2.center_frequency)
        return false;
    if (s1.enabled!=s2.enabled)
        return false;
    if (s1.group_id!=s2.group_id)
        return false;
    if (s1.rf_flow_id!=s2.rf_flow_id)
        return false;
    if (s1.sample_rate!=s2.sample_rate)
        return false;
    if (s1.tuner_type!=s2.tuner_type)
        return false;
    return true;
}

inline bool operator!= (const frontend_tuner_status_struct_struct& s1, const frontend_tuner_status_struct_struct& s2) {
    return !(s1==s2);
}

#endif // STRUCTPROPS_H
