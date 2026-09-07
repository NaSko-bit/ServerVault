# ServerVault
<<<<<<< HEAD
What was supposed to be a small file transfer program for android, turned into much larger idea. A full GUI control panel for linux based server. The control panel will be accessed by several clients for different devices and different purposes.
=======
ANDROID GUI build on top of C back-end. A way for server users, to save files, even when the server is down or offline or whatever. THe idea is the user will have their uploaded files in a local temporary storage, which will be emptied once the server is online and the files transfered.

## Client identity

When a TCP client connects, the host records its numeric IP address in
`LOG.txt`. The client should send its device type as the first newline-terminated
message:

```text
DEVICE_TYPE Android
```

The host records both values, for example:

```text
[2026-09-07 12:00:00] [CLIENT] Client identity: ip=192.168.1.20 device_type=Android
```

If the client does not send this message, its device type is recorded as
`unknown`.
>>>>>>> 14e84b0 (The host now gets metadata about the conencted client)
