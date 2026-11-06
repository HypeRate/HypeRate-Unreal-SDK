# Blueprint and C++ API

## `UHypeRateClientComponent`

### Functions

| Function | Purpose |
| --- | --- |
| `Configure(Token, DeviceId, Endpoint)` | Supplies in-memory runtime configuration. |
| `ClearConfiguration()` | Removes in-memory credentials. |
| `Connect()` | Starts a live connection. Safe to call when already connected. |
| `Disconnect()` | Leaves the channel, closes the socket, and stops reconnects. |
| `BeginSimulation(BPM)` | Replaces any live connection with simulated data. |
| `SetSimulatedHeartRate(BPM)` | Publishes a value through the normal event path. |
| `EndSimulation()` | Returns to `Disconnected`. |
| `IsConnected()` | True for live and simulated data. |
| `IsSimulating()` | True only in simulation mode. |
| `HasConfiguration()` | Checks runtime or local Editor configuration. |

### Events

| Event | Payload |
| --- | --- |
| `OnHeartRateChanged` | `int32 HeartRate` |
| `OnConnected` | None |
| `OnDisconnected` | None |
| `OnError` | `FString Message` |
| `OnStateChanged` | `EHypeRateConnectionState` |

### States

`Disconnected`, `Connecting`, `Connected`, `Simulating`, `Reconnecting`,
`Disconnecting`, and `Error`.

## `UHypeRateSimulatorComponent`

Attach it beside a client. It resolves the client automatically unless one is
assigned explicitly. `StartSimulation`, `StopSimulation`, `SetPreset`, and
`SetCustomHeartRate` are available to Blueprints.

## `UHypeRateHeartRateWidget`

Create the native user widget, assign a client using `SetClient`, and add it to
the viewport. `PushHeartRate` also allows previewing or manually feeding the
display independently from a client.

