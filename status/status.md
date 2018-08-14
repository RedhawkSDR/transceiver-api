# Device Status

Device status changes are communicated via a uses port on the FEI device.

## DeviceStatus IDL

See [DeviceStatus](./DeviceStatus.idl).

The `DeviceStatusEvent` structure represents the entire body of information that distinguishes a device status change.
The specific device (`device_id`) and tuner (`allocation_id`), plus the timestamp are common to all types of status messages.
The status event payload is encapsulated with a discriminated union; currently, only queue, error and custom status are supported.
This gives the potential of extending the set of message types without breaking backwards compatibility.
Implentations that do not support added payload types can ignore the status.

### Errors

The error status payload `DeviceErrorStatus` also uses a discriminated union to allow for future extension.

Device-specific status can be reflected using the "custom" error type, which provides a string and a `CF::Properties` to allow arbitrary error information.

## DeviceStatus Ports

The expectation is that a FEI device will have a `DeviceStatus` out (uses) port that it uses to notify listeners of changes to the device state.

A CONOP-specific controller component will have a `DeviceStatus` input port.
The REDHAWK `frontend` library will provide an implementation that dispatches the notification to the component.

### CORBA EventChannel Support

There is some reasonable desire to monitor device status on a systemic basis.
For example, system like SKQ could listen for errors from all devices registered with the system.

To support this CONOP, the uses-side DeviceStatus port can support connections to endpoints that implement `FRONTEND::DeviceStatus` or `CosEventChannelAdmin::EventChannel`.
In the former case, direct calls are made.
In the latter case, `DeviceStatusEvent` messages are broadcast to the event channel.

For simplicity of implementation and to avoid the confusion that bi-directional ports cause in messaging, unless there is a clear need to connect provides ports to event channels, the `DeviceStatus` interface does not need to follow the MessageEvent pattern.

### Filtering

Broadcasting all device status changes via the EventService may overtask it.
Likewise, even with direct `DeviceStatus` connections, the receiver may only be interested in some messages.
Therefore, there may be a need to specify the class(es) of status change events that a given connection is interested in.
The mechanism is TBD.
