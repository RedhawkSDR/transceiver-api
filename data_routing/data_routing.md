# Data Routing

FEI transmit devices need a method to map BulkIO data to a physical tuner, represented by an allocation ID.

## Connection ID

The connection ID shall be the same as the allocation ID for the desired transmit tuner; this is symmetric with receivers.
This requires the ability to determine with which connection a stream is associated; see [BulkIO Modifications](#bulkio-modifications).

## Allocation ID Override

In the manual execution mode (see [Transaction](../queueing/transaction.md#manual-mode)), the allocation ID from the `FRONTEND::tuner_allocation` keyword in the BulkIO StreamSRI supersedes the connection ID.
This allows a controller to manage multiple tuners, and thus allocation IDs, via a single connection.
While it is technically possible to use a `connectionTable` in the controller's BulkIO output port, this adds additional complexity with no demonstrable benefit.

## BulkIO Modifications

To reconcile a BulkIO stream with a particular connection, it is necessary to add a `connectionId` argument to both `pushSRI()` and `pushPacket()`.

[BULKIO::updateSRI](bio_updateSRI.idl)

For each data type _X_ that has a `pushPacket()` method:
[BULKIO::dataX](bio_dataX.idl)

## Non-Controlling Allocations

Sharing a transmit tuner, such as for a time-multiplexed CONOP, can be managed with non-controlling allocations (called "listener" allocations for receive).
