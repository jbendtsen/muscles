# Muscles
*Work-in-progress struct-based memory editor and analyser*

Written in C++ with SDL, it is designed to provide an informed view of the current state of another process in real time,
allowing the user to understand what the program is doing in terms of its variables (with types) within objects – as opposed to
having to manually decode hex dumps. Currently targets Windows and Linux.

<img style="float: right;" src="https://raw.githubusercontent.com/jbendtsen/muscles/master/images/snap1.png" alt="screenshot" width="100%">

## TODO
* Replace field formatting with type options
	- Bind formatting options like base and prefix to a type, the same way a type has signedness and size
* Fix various C parsing bugs (eg. circular typedef dependencies)
* "Taskbar" feature to minimise/maximise windows
* Mechanism for writing data back to the file/process
* Develop an AST to allow for things like calculating addresses of objects, the first step toward tracking objects in containers and allocators