#!/usr/bin/env python

import ossie.utils.testing
from ossie.utils import sb
import frontend
from ossie.cf import CF
from omniORB import any
from redhawk.frontendInterfaces import FRONTEND
from frontend import tuner_device, fe_types

class DeviceTests(ossie.utils.testing.RHTestCase):
    # Path to the SPD file, relative to this file. This must be set in order to
    # launch the device.
    SPD_FILE = '../USRP.spd.xml'

    # setUp is run before every function preceded by "test" is executed
    # tearDown is run after every function preceded by "test" is executed
    
    # self.comp is a device using the sandbox API
    # to create a data source, the package sb contains sources like StreamSource or FileSource
    # to create a data sink, there are sinks like StreamSink and FileSink
    # to connect the component to get data from a file, process it, and write the output to a file, use the following syntax:
    #  src = sb.FileSource('myfile.dat')
    #  snk = sb.StreamSink()
    #  src.connect(self.comp)
    #  self.comp.connect(snk)
    #  sb.start()
    #
    # components/sources/sinks need to be started. Individual components or elements can be started
    #  src.start()
    #  self.comp.start()
    #
    # every component/elements in the sandbox can be started
    #  sb.start()

    def setUp(self):
        # Launch the device, using the selected implementation
        self.comp = sb.launch(self.spd_file, impl=self.impl)
    
    def tearDown(self):
        # Clean up all sandbox artifacts created during test
        sb.release()

    def _check_fts_member(self, devptr, name, value):
        fts = devptr.query([CF.DataType(id='FRONTEND::tuner_status',value=any.to_any(None))])
        fts_v = fts[0].value._v[0]
        for prop in fts_v._v:
            if prop.id == name:
                found_value = prop.value._v
        self.assertEquals(found_value, value)

    def testBasicBehavior(self):
        #######################################################################
        # Make sure start and stop can be called without throwing exceptions
        #print self.comp.devices
        for dev in self.comp.devices:
            print dev.label
            if 'RDC' in dev.label:
                break
        #frontend_allocation = tuner_device.createTunerAllocation(tuner_type="RDC",bandwidth=24.576, center_frequency=30000000, bandwidth_tolerance=100, allocation_id='hello', returnDict=False)

        _allocation_id = 'hello'
        frontend_allocation = tuner_device.createTunerAllocation(tuner_type="RDC",center_frequency=30000000, allocation_id=_allocation_id, returnDict=False)
        frontend_allocation_gf = tuner_device.createTunerAllocation(tuner_type="RDC",center_frequency=600e6, allocation_id=_allocation_id, returnDict=False)
        allocation_id_csv = 'abc'

        self._check_fts_member(dev, 'FRONTEND::tuner_status::allocation_id_csv', '')

        alloc_response_0 = dev.allocate([frontend_allocation])
        self.assertEquals(len(alloc_response_0), 0)
        alloc_response_1 = dev.allocate([frontend_allocation_gf])
        #print '+++++++++++++', dev.query([CF.DataType(id='FRONTEND::tuner_status', value=any.to_any(None))])
        #print alloc_response_1
        self.assertEquals(len(alloc_response_1), 1)
        alloc_response_2 = dev.allocate([frontend_allocation_gf])
        self.assertEquals(len(alloc_response_2), 0)
        #print alloc_response_2
        #try:
            #alloc_response_1 = dev.allocate([frontend_allocation])
            #print '+++++++++++++', dev.query([CF.DataType(id='FRONTEND::tuner_status', value=any.to_any(None))])
            #print alloc_response_1
            #self.assertEquals(len(alloc_response_1), 1)
            #alloc_response_2 = dev.allocate([frontend_allocation])
            #self.assertEquals(len(alloc_response_2), 0)
            #print alloc_response_2
        #except Exception, e:
            #print e
            #caps = e.capacities
            #for cap in caps:
                #print ' ', cap.id
                #for _cap in cap.value._v:
                    #print '   ',_cap.id, _cap.value._v
            #print dir(e)
        self._check_fts_member(dev, 'FRONTEND::tuner_status::allocation_id_csv', _allocation_id)

        try:
            retval = self.comp.allocate([frontend_allocation])
            #print retval
        except Exception, e:
            print e
            caps = e.capacities
            for cap in caps:
                print ' ', cap.id
                for _cap in cap.value._v:
                    print '   ',_cap.id, _cap.value._v
            print dir(e)
        dev.deallocate(alloc_response_1[0].alloc_id)

        self._check_fts_member(dev, 'FRONTEND::tuner_status::allocation_id_csv', '')

        alloc_response_3 = self.comp.allocate([frontend_allocation_gf])
        #print '=============', self.comp.frontend_tuner_status
        dev_status = dev.query([CF.DataType(id='FRONTEND::tuner_status', value=any.to_any(None))])[0]
        dev_status_members = dev_status.value._v[0]._v
        #print '+++++++++++++', dev_status_members
        parent_stat = self.comp.frontend_tuner_status
        found_status = False
        for par in parent_stat:
            found_status = True
            for entry in dev_status_members:
                if entry.id in par.keys():
                    if entry.value._v != par[entry.id]:
                        found_status = False
                    print entry.id, ':  ', entry.value._v
                if not found_status:
                    break
            if found_status:
                #print '......found a match'
                break

        self._check_fts_member(dev, 'FRONTEND::tuner_status::allocation_id_csv', _allocation_id)

        self.comp.deallocate(alloc_response_3[0].alloc_id)

        self._check_fts_member(dev, 'FRONTEND::tuner_status::allocation_id_csv', '')


if __name__ == "__main__":
    ossie.utils.testing.main() # By default tests all implementations
