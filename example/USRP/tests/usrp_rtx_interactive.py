from ossie.utils import sb
import frontend
from ossie.cf import CF, CF__POA
from omniORB import any
from redhawk.frontendInterfaces import FRONTEND, FRONTEND__POA
from frontend import tuner_device, fe_types

class StatusConsumer(FRONTEND__POA.TransmitDeviceStatus):
    def __init__(self):
        pass
    def transmitStatusChanged(self, status):
        print status
    def statusChanged(self, status):
        print status

usrp=sb.launch('USRP')
_allocation_id = 'hello'
frontend_allocation_gf = tuner_device.createTunerAllocation(tuner_type="TDC",center_frequency=600e6, allocation_id=_allocation_id, returnDict=False)
alloc_response_1 = usrp.allocate([frontend_allocation_gf])
print alloc_response_1[0].data_port
dev=usrp.devices[0]
#center_freq = alloc_response_1[0].control_port.getTunerCenterFrequency(_allocation_id)
#alloc_response_1[0].control_port.setTunerCenterFrequency('hello', center_freq+100)
#usrp.ports[2].ref.getTunerCenterFrequency(_allocation_id)
#usrp.frontend_tuner_status[1]

src=sb.StreamSource()
src.blocking = True
src.complex = True
src.getPort('shortOut').connectPort(alloc_response_1[0].data_port, 'connection_id')

frontend_allocation_rx = tuner_device.createTunerAllocation(tuner_type="RDC",center_frequency=600e6, allocation_id=_allocation_id, returnDict=False)
alloc_response_2 = usrp.allocate([frontend_allocation_rx])
dev=usrp.devices[0]
#center_freq = alloc_response_2[0].control_port.getTunerCenterFrequency(_allocation_id)
#alloc_response_2[0].control_port.setTunerCenterFrequency('hello', center_freq+100)
#usrp.ports[2].ref.getTunerCenterFrequency(_allocation_id)
#alloc_response_1[0].control_port.setTunerCenterFrequency('hello', center_freq+100)
#usrp.ports[2].ref.getTunerCenterFrequency(_allocation_id)
#usrp.frontend_tuner_status[0]

snk=sb.StreamSink()
alloc_response_2[0].data_port.connectPort(snk.getPort('shortIn'), 'connection_id')

status_snk = StatusConsumer()
status_port = alloc_response_1[0].device_ref.getPort('TransmitDeviceStatus_out')
status_port.connectPort(status_snk._this(), 'connection id')

send_length = 32000
magnitude = 300
_data = []
for i in range(send_length):
    _data.append(complex(magnitude,0))

sb.start()
while True:
    for i in range(10):
        src.write(_data)
    data=snk.read()
    print len(data.data)
    c=[]
    for i in data.data:
        c.append(abs(i))
    max_idx = c.index(max(c))
    print max_idx, data.data[max_idx-20:max_idx+20], data.data[max_idx-20+5000:max_idx+20+5000]

