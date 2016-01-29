nrf52-capsense-example
======================

This project includes a driver and example for reading capacitive
button sensors from the nRF52 device, without requiring a dedicated
capacitive sensor. It uses the single-pin capacitive sensor support in
the comparator peripheral (COMP), and requires no external
components. Up to 8 buttons are supported when all analog inputs are
used.
 
The example in its current state will not work with a SoftDevice
enabled, as it accesses the PPI peripheral directly. There are several
aspects that need improvement if used in an end product. Particularly
the calibration algorithm requires more work.

A tutorial that has been written to accompany this example can be
found at https://devzone.nordicsemi.com/tutorials/30/.

Requirements
------------

- nRF5 SDK version 11
- nRF52 DK
- Some form of capacitive sensor

The project may need modifications to work with later versions or
other boards.

To compile it, clone the repository in the /examples/peripheral/
folder in the nRF5 SDK version 11 or later (or any other folder under
/examples/).

About this project
------------------

This application is one of several applications that has been built by
the support team at Nordic Semiconductor, as a demo of some particular
feature or use case. It has not necessarily been thoroughly tested, so
there might be unknown issues. It is hence provided as-is, without any
warranty.

However, in the hope that it still may be useful also for others than
the ones we initially wrote it for, we've chosen to distribute it here
on GitHub.

The application is built to be used with the official nRF5 SDK. The
SDK can be downloaded at https://developer.nordicsemi.com/

Please post any questions about this project on
https://devzone.nordicsemi.com.
