OS/Z - How To Series #6 - Service Management
============================================

Preface
-------

Last time we've seen how to [install](https://github.com/bztsrc/osz/blob/master/docs/howto5-install.md) OS/Z and it's packages.

Service Management
------------------

In most POSIX like operating systems, services are managed by scripts (SysV init) or by another service (systemd). In both cases
you have commands to start and stop services (or if you like daemons) from the shell. OS/Z is not an exception, it has
`init` system service which responsible for that. Please note that unlike user services, [system services](https://github.com/bztsrc/osz/blob/master/docs/services.md)
cannot be controlled from command line.

### Starting a service

To start a daemon temporarily, issue

```sh
sys start (service)
```

### Stoping a service

To stop a daemon, issue

```sh
sys stop (service)
```

### Start a service at boot time

To start a service and also start it permanently on subsequent boots, type

```sh
sys enable (service)
```

### Don't start a service at boot time

To do the opposite, type

```sh
sys disable (service)
```

### Restarting a service

If a service's configuration changed, it will automatically reload. But in cases when something
goes wrong (and the change is not detected or the daemon misbehaves), there should be a need to restart a service
entirely. For that, issue

```sh
sys restart (service)
```

### Query information

To get list of available services, issue the next command without second argument. To get detailed information on a service,
specify it's name.

```sh
sys status
sys status (service)
```

The next tutorial will be end user related, about how to use the [user interface](https://github.com/bztsrc/osz/blob/master/docs/howto7-interface.md).
