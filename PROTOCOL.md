# Openinverter Web Interface Protocol

This document describes the protocol used on the serial interface between this ESP8266 module, and
an inverter or VCU.

## General

Commands are sent by the ESP8266. Each command consists of a single line consisting of a command word
followed optionally by parameters and terminates with a newline character. Commands must be echoed back
to the ESP8266.

Following the echo, the esp8266 will receive an unlimited quantity of response data, terminated by a
100ms timeout.

Except where otherwise noted, the responses are free text and should generally contain a single human
readable line indicating success or faulure of the operation.

## Parameters and other data

Openinverter makes available two types of data - parameters and non-parameters.

Parameters are user configurable values, generally used for configuration. They can be stored in
nonvolatile memory and should not change except in response to a user request.

Other values are made available that are not configurable, but are instead indicative of the immediate
state of the inverter. These are useful for monitoring and debugging.

## Commands

| Command | Description|
|---------|------------|
|`save`|save current parameters to nonvolatile memory|
|`load`|load parameters from nonvolatile memory|
|`fastuart`|increases the baud rate to 921600Response must begin "OK" in success case|
|`set [parameter] [value]`|set the decimal value of a named parameter|
|`can [direction] [name] [canid] [offset] [length] [gain]`|map values to CAN messages|
|`can clear`|clear all can mappings|
|`start [opmode]`|start the inverter in a specified modemode 2 is manual run|
|`stop`|stop the inverter|
|`get [parameter]`|get the value of a parameter|
|`stream [repetitions] [val1,val2,val3]`| repeatedly read and return one or more values|
|`json [hidden]`| return an JSON encoded mapping of all parameters and values - see JSON format below|
|`errors`|print information about all currently active error states, or indicate that everything is okay|
|`reset`|reboot the device|
|`defaults`|restore all parameters to default values|

Note: This is not an exhaustive list of commands supported by openinverter devices, but does include
all commands currently used by the openinverter web intrface.

## JSON Mapping

The json command requests a dump of the full schema and values of both the configurable parameters
and other available data. The optional "hidden" flag requests data that would not normally be
displayed to the user.

The following example shows a non-parameter value, a parameter value, and a value that has been
mapped to CAN.

```json
{
  "udc":   {"unit":"V",   "value": 400.0,  "isparam": false},
  "fweak": {"unit":"Hz",  "value": 67.0,   "isparam": true, "minimum": 0.0, "maximum": 400.0, "default": 67.0, "category": "Motor (sine)", "i": 8},
  "speed": {"unit":"rpm", "value": 1000.0, "isparam": false, "canid": 123, "canoffset":0, "canlength":32, "cangain":5, "isrx": false}
}
```