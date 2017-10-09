# DirSurveiller
Surveills a directory on -nix systems and logs filesystem operations of processes 


-------------
Installation: 
-------------

No clear procedures exist at the moment 
 
------------
Development:
------------

Get all of the project including submodules

  git clone --recursive https://github.com/rekm/DirSurveiller.git

Currently there are makefiles strewn about.
If you run make in
  src/watchdog
and 
  src/opentrace 
all relevent programs should have been compiled

watchdog
--------

A daemon listening on two sockets for open and execv syscalls. 
Right now the watchdog is a client and whatever process is used to fill the server 
represents the server.  
bashtrace_wrapper can be used as a server. It redirects the standart input and output,
of a provided bash call, into the relevant sockets.





watchdog_ctrl
-------------

Controls watchdog process 

Quits the process 

      $ ./watchdog_ctrl --stop/-q 

Gets status of watchdog daemon if no arguments are provided or if status is requested 

      $ ./watchdog_ctrl --status/-s

Can add directory or files to watchlist

      $ ./watchdog_ctrl --add/-a <directory/file>

bashtrace_wrapper
-----------------

A wrapper program that is used to wire a random process into the socket architecture.

      $ ./bashtrace_wrapper sudo bash bashtrace.sh

StandardOutput -> openCallSocket

StandardError  -> execCallSocket





