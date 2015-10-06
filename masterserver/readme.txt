Simple SeriousProton master server implementation in php.

Allows servers to register themselves to the master server.
Servers need to send frequent updates to let the master server know they are still alive. When a server has not send anything for 5 minutes, it's assumed to be offline.
