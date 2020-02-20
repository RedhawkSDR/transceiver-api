#!/usr/bin/env python

import ossie.utils.testing
from ossie.utils import sb
import frontend
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

    def testBasicBehavior(self):
        #######################################################################
        # Make sure start and stop can be called without throwing exceptions
        print self.comp.devices
        for dev in self.comp.devices:
            print dev.label
            if 'RDC' in dev.label:
                break
        #frontend_allocation = tuner_device.createTunerAllocation(tuner_type="RDC",bandwidth=24.576, center_frequency=30000000, bandwidth_tolerance=100, allocation_id='hello', returnDict=False)
        frontend_allocation = tuner_device.createTunerAllocation(tuner_type="RDC",center_frequency=30000000, allocation_id='hello', returnDict=False)
        try:
            #retval = dev.allocateCapacity([frontend_allocation])
            retval = dev.allocate([frontend_allocation])
            print retval
        except Exception, e:
            print e
            caps = e.capacities
            for cap in caps:
                print ' ', cap.id
                for _cap in cap.value._v:
                    print '   ',_cap.id, _cap.value._v
            print dir(e)
        
        try:
            retval = self.comp.allocateCapacity([frontend_allocation])
            print retval
        except Exception, e:
            print e
            caps = e.capacities
            for cap in caps:
                print ' ', cap.id
                for _cap in cap.value._v:
                    print '   ',_cap.id, _cap.value._v
            print dir(e)
        

if __name__ == "__main__":
    ossie.utils.testing.main() # By default tests all implementations
