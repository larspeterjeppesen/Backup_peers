# Backup_peers

## What is this?
This piece software is a backup tool I am writing for personal use. It's purpose is to transfer data from my phone to my desktop computer over my private wifi through a raspberry pi that I use as router. 

I am aware that tools that can backup data on a phone exists, however I am not interested in my data running through machines I don't own.
I am also not interested in backing up my data manually. One of the end-goals of this software is complete automatization - upon my phone connecting to my private wifi, any files that have not yet been transferred to my desktop computer will begiv transferring without any initialization from my side.

### Why C?
I want to exercise and expand my knowledge of C. If you are looking to write a backup-tool as fast as possible, choose something else, like python fx.


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


