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

Currently there are makefiles strewn about 

watchdog
--------

A daemon listening on two sockets for open and execv syscalls. 
A bashtrace_wrapper is used to  


watchdog_ctrl
-------------

Controls watchdog process 

Quits the process 

  $ ./watchdog_ctrl --stop/-q 

Gets status of watchdog daemon if no arguments are provided or if status is requested 

  $ ./watchdog_ctrl --status/-s

Can add directory or files to watchlist

  $ ./watchdog_ctrl --add/-a <directory/file>





