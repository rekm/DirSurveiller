# DirSurveiller
Surveills directories/files on -nix systems and logs filesystem events triggerd by processes 
Developed with resources of the Robert Koch-Instiut Berlin. 



-------------
Installation: 
-------------

No clear procedures exist at the moment 
 
------------
Development:
------------

Get all of the project including submodules

        git clone --recursive https://github.com/rekm/DirSurveiller.git

The build process is controlled via cmake

1. create a build directory somewhere
       
        $ mkdir <path/to/name_of_build_dir> 

3. change directory into the new folder

        $ cd <path/to/name_of_build_dir>

2. run cmake and point it to the DirSurveiller root folder 
   (first one of the project with CMakeLists.txt in it).  
        
        $ cmake <path/to/root/of/project> 

Alternatively Graphic cmake (for overview of configurable options):

        $ ccmake <path/to/root/of/project>
3. make 

        $ make 

watchdog
--------

A daemon listening on two sockets for open and execv syscalls. 
Right now the watchdog daemon is the client and whatever process is used to fill the sockets 
represents the server.  
bashtrace_wrapper can be used as a server. It redirects the standart input and output,
of a provided bash call, into the relevant sockets.
For the moment, the server needs to be started first. 


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

      $ ./bashtrace_wrapper <program>

program standard output -> openCallSocket 

program standard error  -> execCallSocket
 
Example:

      $ ./bashtrace_wrapper sudo bash bashtrace.sh
      




