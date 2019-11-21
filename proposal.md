# FEI 2.0 deficiencies to be resolved
The following deficiencies were identified in FEI 2.0 and are to be resolved in FEI 3.0:

* No defined way to schedule a transmission burst
* No feedback from a transmitter regarding the status of a transmission
* No standard way for FEI devices to advertise that they are in an error state
* No easy to associate an allocation with a data set coming out of a port
* The allocation of a single tuner on a device (since DDC in RX_DIGITIZER or single DUC in TX) requires a tedious search through the tuner status structure to find the actual tuner

Several updates to RH are proposed that will address these shortcomings

## Advertise Error State

The proposed change is to add a state to the CF::Device OperationalType enum. Now, those states are ENABLED and DISABLED. We propose adding ERROR to that enum. The actual error description can be retrieved from the device's log

## Allow dynamic device creation

In FEI/RH 2.X, devices are statically deployed; xml describes which devices are deployed by the device manager. This approach means that tuners become attributes of the device that supports an allocation. The problem with this approach is that the device now requires a sophisticated API to select specific tuners to control or get data from. The term API is pretty generous in this case; for example, to get data from a specific tuner on the device, a connection to the device's output bulkio port is needed where the connection id matches the allocation id. If someone wants to inspect that data set, a "listener" allocation is required. This is further complicated with multi-out interfaces, where stream ids are matched to connection ids on an output port. The solution to this problem is to allow the FEI device to dynamically deploy child tuner devices.

With this programmatic device deployment, an allocation of a tuner can be associated with a single tuner instance, where this tuner instance is its own device (that lives as a thread in the device process) that implements the CF::Device interface and has its own bulkio and control port(s). Since now there is a one-to-one mapping between allocations and tuner instances, the mapping of allocation/connection/stream id strings is no longer necessary

## Tuner allocations are framework-level calls

allocateCapacity's major drawback is that it does not have an feedback other than true/false. Tuner allocations on an FEI device require very specific feedback, namely: pointer to the tuner device that satisfied the allocation, pointer to the port that outputs the data, pointer to the tuner's control port, and the actual allocated parameters (things like tuner bandwidth may be close but not exact). To resolve this problem, we propose the method allocateTuners

The method allocateTuners takes an array of allocation properties, supporting both scan and coherent tuner allocations. The return value for the call is a data structure that contains a list of the devices that satisfied the request. If the allocation failed, the return value is an empty list.

The specific content of the returned data structure will be specified in the IDL, but it contains, at a minimum, the tuner device, its bulkio port (input or output, depending on RX or TX), its control port, and the tuner status structure.

## Feedback from the transmitter

Transmit is the only FEI device that, at the API level that RH operates, requires sophisticated status. The reason for this is that in RX applications, the application has limited ability to remedy the situation. In TX applications, the application crafted the transmission, so errors in the transmit plan can be addressed programmatically. We propose creating a status port that pushes status information regarding the TX device.

## Transmit API

Transmission control is really a policy issue; how should the device behave when an error occurs and when should status messages be sent out? The transmit control API sets these parameters

## Transmission planning

Transmissions are planned based on bulk io. SRI keyword CHAN_RF is used to set the center frequency for the transmission. Queued bulkio packets, with their respective timestamps, are used to schedule transmissions. In the case where sending happens when data is available (the current CONOP), the policy can be set to ignore timestamps

## Transmission allocation

Beyond setting the device to TX, an additional transmission allocation property is required to specify the minimum and maximum frequencies that transmit will be required as well as the maximum retuning rate and maximum transmit power. The transmission's bandwidth is set as part of the standard allocation structure
