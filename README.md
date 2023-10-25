# Backup_peers



## API
The following modes are available from the commandline:

### Predefined modes
* "--phone"
  * Run as sender only. Goal is that in this mode, all files from host that have not already been transferred will be transferred to the pi as soon as a connection can be established between them.
* "--pi"
  * Run as both receiver and sender. Receiver portion will receive files from phone and store them locally. Sender portion will send any files gotten from the receiver portion, and send them to the desktop pc as soon as a connection can be established between them.
* "--desktop"
  * Run as receiver only. Will receive files from pi and store them locally.

### Manual mode
Following arguments are supported:
* `-s [IP] [Port] [filepath]` - will send `[filepath]` to the given address.
* `-r [Port]` - will listen for incoming connections on `[Port]`.
One or both of these arguments must be supplied. Giving no arguments will print the above, then terminate the program.


