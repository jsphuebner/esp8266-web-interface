Arduino IDE usage
=================

# Table of Contents
<details>
 <summary>Click to open TOC</summary>
<!-- MarkdownTOC autolink="true" levels="1,2,3,4,5,6" bracket="round" style="unordered" indent="    " autoanchor="false" markdown_preview="github" -->

- [Compilation](#compilation)
- [Compile and Flash board](#compile-and-flash-board)
- [Flashing the filesystem](#flashing-the-filesystem)
- [Debugging](#debugging)

<!-- /MarkdownTOC -->
</details>

# Compilation
To compile, you need to choose, in the `Sketch` menu, `Verify / Compile`

# Compile and Flash board
Choose, in the `Sketch` menu, `Upload`. It will compile and send to the board with the chosen method (serial / OTA)

# Flashing the filesystem
Choose, in the `Tools` menu, the `ESP8266 Sketch Data Upload` option, which will package, and flash the files in the `data` directory (HTML files for the web interface)

# Debugging
In case you're debugging and have enabled (`Tools` > `Debug port` / `Tools` > `Debug level`) some kind of debugging output, you may use the `Tools` / `Serial monitor` to check the debug messages.

Please note that the serial port is intended to be attached to the inverter (or other application), so do not leave debug messages in normal operation.