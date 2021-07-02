/*
 * necessary to allow FrontendTunerDevice template class to be split
 * into separate header and implementation files (.h and .cpp)
 */

#include <frontend/fe_tuner_device.cpp>
#include "struct_props.h"
#include "TDC/TDC_struct_props.h"
#include "RDC/RDC_struct_props.h"

template class frontend::FrontendScanningTunerDevice<frontend_tuner_status_struct_struct>;
template class frontend::FrontendTunerDevice<TDC_ns::frontend_tuner_status_struct_struct>;
template class frontend::FrontendTunerDevice<RDC_ns::frontend_tuner_status_struct_struct>;

