# Confidants
This README discusses what this piece of software does, as well as my design choices in its implementation.

## What is this?
Confidants is a backup tool I am writing for personal use. It's purpose is to transfer data from my android phone to my Ubuntu machine over my private wifi through a raspberry pi that I use as a router. 

I am aware that tools that can backup data on a phone exists, however one point of this project is to not have my data running through machines I don't own.
I am also not interested in backing up my data manually. One of the end-goals of this software is complete automatization - upon my phone connecting to my private wifi, any files that have not yet been transferred to my desktop computer will begiv transferring without any initialization from my side.

### Why C?
I want to exercise and expand my knowledge of C and Linux. I am fully aware that implementation-wise, there are easier languages to write this in.

## API
The following modes are available from the commandline:

### Predefined modes
* "--phone"
  * Run as sender only. In this mode, all files from host that have not already been transferred will be transferred to the pi as soon as a connection can be established between them.
* "--pi"
  * Run as both receiver and sender. Receiver portion will receive files from phone and store them locally. Sender portion will send any files gotten from the receiver portion, and send them to the desktop pc as soon as a connection can be established between them.
* "--desktop"
  * Run as receiver only. Will receive files from pi and store them locally.

### Manual mode
Following arguments are supported:
* `-s [IP] [Port] [filepath]` - will send the file located at `[filepath]` to the given address.
* `-r [Port]` - will listen for incoming connections on `[Port]`.
One or both of these arguments must be supplied. Giving no arguments will print the above, then terminate the program.


## Steps in development
This is a loose list of the steps that I am updating as I go along in the project.

**Functionality:**
* (Done) Implement transfer of a single file between two instances of confidant.c than can run on my android, pi, and ubuntu machine. 
* Design and implement a tracking mechanism to identify and transfer newly added files on my android and raspberry pi.
  * Implement sqlite database
* Implement the predefined modes.

**Correctness:**
* Write a test suite that reasonably ensures correct file-transfer
* Debug flag, allowing diagnostics of all procedures

**Robustness:**
* Server able to request specific blocks (eg. ensuing corrupted block transfer)
* Graceful handling of connection drops.
  * Server:
    * Store whatever data has been correctly received
    * Go back into listening state
  * Client: 
    * Go into pending state, waiting to reconnect
    * Restart transfer of file

**Performance:**
* Apparently my phone has 3 different CPU's on it. Benchmarking with different number of threads is necessary since I don't know how many threads are employable.
* Implement a job queue
* Benchmark performance of transfer record lookup, compare to sql or other db implementations.

### Post-completion
Some ideas to improve ease-of-use:
* A control panel on my ubuntu machine would be neat in order to throttle/pause the backup. 
* A transfer log display

Maybe I'll come back and add other features here.


## File transfer tracking
Tracking what files have been transferred has the following components:

Android-side:
The android needs to keep a record of files that have already been transferred. In design this record, I have gone by the following principles:
* Allow duplicate file names (Unneccessary for a single backup, but between backups we may have file `x` named `a` be altered/deleted, then introduce file `y` also named `a`. I want to have a copy of both files on my ubuntu machine, hence duplicate names must be allowed in the record.
* Never transfer the same file twice (outside of error correction).


To accomodate for these points, I have decided on a database where entries have the following data associated:
* File name (including path) | Transfer date

The following procedure checks whether a file has previously been transferred, and transfers it if not:
* Locate all entries of file in database.
* If not in database, transfer file, insert into database
* Else:
  * If file has not been modified since last transfer, move on to next file.
  * Else transfer file and add a new entry.



Pi-side:



