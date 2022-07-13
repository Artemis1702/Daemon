# Daemon

A daemon is a service process that runs in the background and supervises the system or provides functionality to other processes. Traditionally, daemons are implemented following a scheme originating in SysV Unix.<br />
Modern daemons should follow a simpler yet more powerful scheme, as implemented by systemd.

## Objective

To create a traditional SysV daemon, following the guidelines in systemd's recommendations for writing a traditional SysV daemon (this information can also be found by opening the following manual page: man 7 daemon).<br />
This daermon will create a file with the processid of the daemon so that the user will be able to find out that a daemon process is created. It will also send a notification to the user's output screen using a pipe command.;
