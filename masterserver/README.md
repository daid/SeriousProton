# masterserver

A simple SeriousProton master server implementation in PHP that allows game servers to register themselves to the master server. Game clients can then query the master server for a list of available servers, their IP address, their port, and an identifying name.

Servers need to send frequent updates to let the master server know they are still alive. When the master server has not received anything from a game server for 5 minutes, it's assumed to be offline.

## Prerequisites

- MySQL server (see `config.inc.php` for default configuration)
- PHP 5.?+
  - Socket PHP extension
  - pdo_mysql extension

## /register.php (POST)

Register a game server by writing its details to the database.

### Parameters

`port`, `version`, and `name` are required.  `ip` is optional.

- **port**: The game server's port, such as `35666`.
- **version**: The game server's version, such as `20241208`.
- **name**: A name to identify the server to clients.
- **ip** _(optional)_: The IP address or hostname to advertise to clients. When omitted, the TCP connection's source address is used.

  If advertising with a reverse proxy (running with the `server=listen` option on the proxy server, and with `serverproxy=<IP ADDRESS>` on the game server), supply the reverse proxy's address here so that clients connect to the proxy. The IP must contain only valid IPv4/IPv6/hostname characters and be at most 253 characters long.

On first registration, the master server opens a TCP connection to the advertised address and port to verify the server is publicly reachable.  Subsequent registrations from the same address and port act as a heartbeat and skip this check.

### Responses

| Body | Meaning |
| ---- | ------- |
| `OK` | Registered successfully |
| `CONNECT FAILED: ip:port` | Master server could not TCP-connect to the advertised address — port forwarding or the reverse proxy is not reachable |
| `'port' not set` (HTTP 400) | Missing required parameter |
| `Invalid 'ip'` (HTTP 400) | `ip` override contains disallowed characters or exceeds 253 characters |

### Example

Assuming a game server is running on port 35666 at 127.0.0.1:

```
$ curl -d "port=35666&name=Test&version=20190111" -X POST http://127.0.0.1:8000/register.php
OK
```

Registering the server at 127.0.0.1 via a reverse proxy at 203.0.113.1:

```
$ curl -d "port=35666&name=Test&version=20190111&ip=203.0.113.1" -X POST http://127.0.0.1:8000/register.php
OK
```

If the master server fails to connect to the advertised address:

```
CONNECT FAILED: 127.0.0.1:35666
```

## /list.php (GET)

Retrieve a list of registered servers, each as a colon-delimited line of (in order) its IP address, port, version, and name.

### Example

```
$ curl http://127.0.0.1:8000/list.php
127.0.0.1:35666:20200111:Server
```

An empty list provides no response.
