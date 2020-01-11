# masterserver

A simple SeriousProton master server implementation in PHP that allows game servers to register themselves to the master server. Game clients can then query the master server for a list of available servers, their IP address, their port, and an identifying name.

Servers need to send frequent updates to let the master server know they are still alive. When the master server has not received anything from a game server for 5 minutes, it's assumed to be offline.

## Prerequisites

-   MySQL server (see `config.inc.php` for default configuration)
-   PHP 5.?+
    -   Socket PHP extension
    -   pdo_mysql extension

## /register.php (POST)

Register a game server by writing its details to the database.

### Parameters

All three parameters are required.

- **port**: The game server's port, such as `35666`.
- **version**: The game server's version, such as `20191101`.
- **name**: A name to identify the server to clients.

The game server's IP address is also recorded.

### Example

Assuming a game server is running on port 35666 at 127.0.0.1:

```
$ curl -d "port=35666&name=Test&version=20190111" -X POST http://127.0.0.1:8000/register.php
OK
```

If the registry server fails to connect to the game server, it responds:

```
CONNECT FAILED: 127.0.0.1:35666
```

## /list.php (GET)

Retrieve a list of registered servers, each in a colon-delimted line of (in order) its IP address, port, version, and name.

### Example

```
$ curl http://127.0.0.1:8000/list.php
127.0.0.1:35666:20200111:Server
```

An empty list provides no response.
