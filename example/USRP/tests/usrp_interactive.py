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
snk=sb.StreamSink()
alloc_response_1[0].data_port.connectPort(snk.getPort('shortIn'), 'connection_id')
sb.start()
data=snk.read()
print len(data.data)
