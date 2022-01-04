# Can do Stm32 - app framework

A framework based on CANopenNode, libopencm3 and FreeRTOS that enables writing modular networking applications for STM32 devices.

## Basics 

* **CAN bus** - reliable wired transport for everything that shouldn't break
* **CANopen** - higher level protocol for managing, configuring and connecting CAN devices
* **libopencm3** - hardware-independency with no overhead

## Approach

Code is split into two groups of objects: modules and devices. Modules are everything about perhiphery of the MCU itself (usart, can, modbus, etc), while devices is everything that plugs in to MCU (buttons, screens, sensors, etc). The framework provides approach to connect modules and devices together, provides common state machine,  as well as ability to observe and change values reactively.

Each object is configured via CANopen dictionary record, and all the meaningful state is observed on the network. Configuration and commands between different network nodes happens by writing observed values by one devices to another. Internal changes of state can be broardcasted to network as individual values or packed into PDO objects (8 bytes). CANopen provides ability to snapshot and send data from multiple devices simultaneously by simulating synchronous messaging. Devices can acquire addresses automatically, then can be monitored via heartbeats and orchestrated to boot up at once. Master nodes can log errors on the network and report issues.

CANopen dictionary itself is created with CANopenEditor that presents a device profile with a lot of features and support of different devices that work together out of the box. The development and system design happens mostly by subtraction instead of addition. Creating a new device involves taking a profile with all the features and turning some of them away. The scripts allow overlaying a generic profile with custom configuration customizations, so that the same device can have slightly different settings, or be powered by mcus with different pin configuration, even cpu families.

The framework is written in a way that emulates asynchrony (for things like individual object timed state transitions), but still uses FreeRTOS for larger picture task coordination. All the low level interactions are non-blocking and hidden from the author. DMA is used extensively for off-loading the cpu of blocking tasks like periphery transmissions.