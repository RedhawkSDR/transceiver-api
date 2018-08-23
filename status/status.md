# Device Status

Device status changes are communicated via a uses port on the FEI device.

## DeviceStatus IDL

See [DeviceStatus](./DeviceStatus.idl).

The `DeviceStatus` interface provides methods for notifying listeners when some aspect of the device's status changes.
The specific device (`device_id`) and tuner (`allocation_id`), plus the timestamp are common to all types of status messages.
There are methods and data structures defined for three classes of device status:
  * Errors
  * Queue Status
  * Other Status

### Errors

Device errors are reported with a `DeviceErrorEvent` struct via `DeviceStatus::deviceErrorOccurred()`.
The `error_code` field uses an enumerated value to allow for future definition of error types.
Implentations that do not support extended error types can ignore the event, or only perform basic handling (e.g., logging).

Device-specific status can be reflected using the "custom" error type, which may insert arbitrary error information into the `error_props` field.

### Queue Status

Queued transaction state transitions are reported with a `DeviceQueueEvent` struct via `DeviceStatus::deviceQueueChanged()`.
The `queue_action` field indicates what type of state transition occurred.
Additional information about the state transition, such as the reason for cancellation or rejection, is conveyed via the optional `queue_props` field.

### Other Status

Generic or device-specific status messages can be reported with a `DeviceStatusEvent` struct via `DeviceStatus::deviceStatusChanged()`.
The `status_type` field is a string identifier that should indicate what aspect of the device changed, while `status_props` allows arbitrary status information via properties.

## DeviceStatus Ports

The expectation is that a FEI device will have a `DeviceStatus` out (uses) port that it uses to notify listeners of changes to the device state.

A CONOP-specific controller component will have a `DeviceStatus` input port.
The REDHAWK `frontend` library will provide an implementation that dispatches the notification to the component.

### CORBA EventChannel Support

There is some reasonable desire to monitor device status on a systemic basis.
For example, system like SKQ could listen for errors from all devices registered with the system.

To support this CONOP, the uses-side DeviceStatus port can support connections to endpoints that implement `FRONTEND::DeviceStatus` or `CosEventChannelAdmin::EventChannel`.
In the former case, direct calls are made.
In the latter case, `DeviceErrorEvent`, `DeviceQueueEvent` and `DeviceStatusEvent` messages are broadcast to the event channel.

For simplicity of implementation and to avoid the confusion that bi-directional ports cause in messaging, unless there is a clear need to connect provides ports to event channels, the `DeviceStatus` interface does not need to follow the MessageEvent pattern.

### Filtering

Broadcasting all device status changes via the EventService may overtask it.
Likewise, even with direct `DeviceStatus` connections, the receiver may only be interested in some messages.
Therefore, there may be a need to specify the class(es) of status change events that a given connection is interested in.
The mechanism is TBD.
