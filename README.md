# Confidants
This README discusses what this piece of software does, as well as my design choices in its implementation.

## What is this?
Confidants is an automatised backup tool, designed to transfer data from my android phone, through my raspberry pi router, to my Ubuntu machine. 

### Why not use existing tools?
By writing the whole thing from scratch, we:
* Avoid sharing data with cloud service providers.
* Have complete control over the process.
* Gain knowledge of the systems worked on.

### Why C?
I want to exercise and expand my knowledge of C and Linux. I am fully aware that there are languages more convenient for this process.

### Disclaimer
I am writing this project for my own personal use. This repo is made public to showcase my work, and for anyone interested in network-programming in C. If the software runs on my machines, it is good enough - although I do write the code to not be platform specific when it is not too inconvenient. Therefore, use at your own discretion.

## API
The following modes are available from the commandline:

### Predefined modes
* "--phone"
  * In this mode, all files from host that have not already been transferred will be transferred to the pi as soon as a connection can be established between them.
* "--pi"
  * Receive files from phone and store them locally. Send received files to Linux machine.
* "--desktop"
  * Receive files from pi and store them locally.

### Manual mode
Following arguments are supported:
* `-s [IP] [Port] [filepath]` - will send the file located at `[filepath]` to the given address.
* `-r [Port]` - will listen for incoming connections on `[Port]`.
One or both of these arguments must be supplied. Giving no arguments will print the above, then terminate the program.

## Steps in development
This is a loose list of the steps that I am updating as I go along in the project.

**Functionality:**
* (Done) Design and implement a file transfer protocol.
* (Done) Implement transfer of a single file between two instances of confidant.c than can run on my android, pi, and ubuntu machine. 
* Design and implement a tracking mechanism to identify and transfer newly added files on my android and raspberry pi.
  * Implement sqlite database
* Implement the predefined modes.

**Correctness:**
* Write a test suite that reasonably ensures correct file-transfer
* Debug flag, allowing diagnostics of all procedures

**Robustness:**
* Add config file; remove hardcoded parts.
* Server able to request specific blocks (eg. ensuing corrupted block transfer)
* Graceful handling of connection drops.
  * Server:
    * Store whatever data has been correctly received
    * Go back into listening state
  * Client: 
    * Go into pending state, waiting to reconnect
    * Restart transfer of file

**Performance:**
* (Done) Implement a job queue utilized by threads.
* Apparently my phone has 3 different CPU's in it. Benchmarking with different number of threads is necessary since I don't know how many threads are employable.
* Benchmark performance of transfer record lookup, compare to sql or other db implementations.

### Post-completion
Some ideas to improve ease-of-use:
* A control panel on my ubuntu machine would be neat in order to throttle/pause the backup. 
* A transfer log display

Maybe I'll come back and add other features here.

## File transfer tracking
**Android-side**:
The android needs to keep a record of files that have already been transferred. In designing this record, I have gone by the following principles:
* Allow duplicate file names (Unneccessary for a single backup, but between backups we may have file `x` named `a` be altered/deleted, then introduce file `y` also named `a`. I want to have a copy of both files on my Linux machine, hence duplicate names must be allowed in the record.
* Never transfer the same file twice (outside of error correction).

To accomodate for these points, we can use the following database entry:
* File name (including path) | Transfer date

**Pi-side:**
(work in progress)



