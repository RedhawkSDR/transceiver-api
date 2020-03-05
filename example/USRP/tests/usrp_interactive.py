from ossie.utils import sb
import frontend
from ossie.cf import CF
from omniORB import any
from redhawk.frontendInterfaces import FRONTEND
from frontend import tuner_device, fe_types
src=sb.launch('USRP')
_allocation_id = 'hello'
frontend_allocation_gf = tuner_device.createTunerAllocation(tuner_type="RDC",center_frequency=600e6, allocation_id=_allocation_id, returnDict=False)
alloc_response_1 = src.allocate([frontend_allocation_gf])
print alloc_response_1[0].data_port
dev=src.devices[0]
center_freq = alloc_response_1[0].control_port.getTunerCenterFrequency(_allocation_id)
alloc_response_1[0].control_port.setTunerCenterFrequency('hello', center_freq+100)
src.ports[2].ref.getTunerCenterFrequency(_allocation_id)
src.frontend_tuner_status[0]

snk=sb.StreamSink()
alloc_response_1[0].data_port.connectPort(snk.getPort('shortIn'), 'connection_id')
sb.start()
data=snk.read()
print len(data.data)
