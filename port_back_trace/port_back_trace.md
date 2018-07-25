
Title: "Port Back Reference"

# Objective
In REDHAWK, port connectivity moves from the source (user) to the destination (provides). This structure makes tracing the flow backwards very difficult. This is a proposal to provide a reference in the provides-side to the uses-side of a connection. Furthermode, connectivity is defined between ports, not components/devices. The user and provides side of a connection also needs information regarding the owner of the port.

# Proposed Modifications

## A new interface that extends CF::QueryablePort that contains the additional port owner information
This interface provides new connection functions (connect/disconnect) that can be relayed back to the base interface's connectPort/disconnectPort functions. This extension supports backward compatibility

void disconnect (in string connectionId) raises (CF::Port::InvalidPort);
void connect (in Object providesPort, in Object providesParent, in Object usesParent, in string connectionId);

To track these connections, the UsesConnection data structure has been extended to contain the providesParent and usesParent references.

While servicing the connect function, the uses port calls the provides port and establishes the reference back to the uses port. In other words, the reference from the provides port to the uses port is established by the uses port connecting to the provides port, not by the software requesting the connection.

## A new interface that will be a base to any port interface that is meant to support a back reference

This interface is called by the uses port to establish the reference back. Note that this interface includes a function to refresh the port's references. The point of this function is to clean up any stale references; since the core framework is not establishing or removing the reference back to the uses port, if the component hosting the uses port fails to remove the reference (because it crashed or timed out during tear down), then there is a risk that the provides port will contain stale references.

Also not that the provides port interface does not inherit from QueryablePort, since it is not expected to initiate a connection.

readonly attribute PortReferenceSequence references;
void refreshReferences ();
void removeReference (in string connectionId) raises (CF::Port::InvalidPort);
void establishReference (in Object usesPort, in Object usesParent, in Object providesParent, in string connectionId);

This directory contains the updated Port.idl and QueryablePort.idl files with the proposed updates.

# Concerns

The back-tracing capability will be available only in ports that implement the ProvidesPort interface, thus:
- Custom IDL ports must include this interface to support the functionality
- Base supported interfaces (e.g.: CF::Resource) do not support back-tracing
- BULKIO, BURSTIO, and FRONTEND will need to be updated to support this functionality

If a component contains BULKIO, BURSTIO, or FRONTEND ports that were generated rather than linked (pre-1.8 in BULKIO, pre-1.10 in FRONTEND), they will need to be regenerated or manually updated to support the new required functions.
