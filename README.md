# STM32.app - Operating System

An operating system based on CANopenNode, libopencm3 and FreeRTOS that enables writing modular networking applications for STM32 microcontrollers.. 

## Features
  * **CAN bus-first** - bulletproof wired networking for actors that shouldnt break
  * **Actor-oriented** - individual objects sending messages to each other
  * **Event-driven** - loosely coupling, highly composable
  * **Cooperative** - pre-emptive multi-tasking with no effort  
  * **Observable** - fully configurable via CANopen via actor profiles, via UI or programmatically over network
  * **Hardware independent** - aimed to support all STM32 actors transparently
  * **Low overhead** - get what you paid for
  * **Batteries included** - use anything STM32 offers out of the box

## Standing on shoulders of giants

* **CANopenNode** - higher level protocol for managing, configuring and connecting CAN actors (Apache 2)
* **libopencm3** - stm32 hardware-independency with no overhead (MIT)
* **FreeRTOS** - a realtime OS for cooperative multitasking (LGPL)

## Actors, Dictionaries, Profiles

**Actors** are individual objects within the STM32.app with different responsibilities. They include user applications, peripherials, drivers and MCU subsystems. Actors need to communicate and cooperate with each other to produce complex behavior. They resemble traditional OOP objects, but the difference is that they have the same public interface. Instead of methods calls, devices send each other signals, events and callbacks.

CANopen splits objects into actor-specific (general framework) and manufacturer-specific (user application). STM32.app constitutes a framework of actor-specific objects that all work together in a cooperative, non-blocking way.
  * MCU features
    - Timers
    - ADC
    - DMA
  * Transport
    - USART
    - SPI
    - I2C
    - CAN
  * Storage drivers
    - Flash memory
    - Eeprom
    - Static memory
  * Simple input
    - Analog input
  * Output
    - Screens
    - Indicators (led, beepers, etc) 

Each object is configured via CANopen dictionary record, and all the meaningful state is observed on the network. Configuration and commands between different network nodes happens by writing observed values by one actors to another. 

CANopen dictionary itself is created with CANopenEditor that presents a actor profile with a lot of features and support of different actors that work together out of the box. The development and system design happens mostly by subtraction instead of addition. Creating a new actor involves taking a profile with all the features and turning some of them away. The scripts allow overlaying a generic profile with custom configuration customizations, so that the same actor can have slightly different settings, or be powered by mcus with different pin configuration, even cpu families.

# Cooperation
STM32.app implements cheap asynchrony, prioritization of work, event-driven communication with queues and cooperation through easy to use primitives.

There is a small set of built-in threads with different priorities that objects in the framework share. This is unlike typical usage of FreeRTOS in which there are many tasks with overlapping priorities. Let's overview different approaches to cooperation, to see why STM32.app does what it does:

### Multiple FreeRTOS tasks per object
The most flexible way is to define more than one task per each object, each having different priorities. One task to accept input, one task to produce output, maybe even some more tasks for for background jobs. 

#### Pros
* ✅ **Flexible**: An object can use however many priorities it needs to
* ✅ **Non-blocking**: If a task takes too much time, FreeRTOS will pause it to allow other tasks with the same priority do run
* ✅ **Simple execution flow**: Tasks code *can* be easy to follow and write, since the execution never leaves task function. Tasks may use stack variables to store intermediate results, and places of asynchrony can be easy to see.
#### Cons
* ❌ **Memory over-allocation**: Memory for each task stack and its queues has to be allocated upfront to handle bursts of activity without losing any messages or input. There is a lot of memory left unused, because it had to be allocated just in case or because it is needed once.
* ❌ **Comunication complexity**: Tasks of the same object need to pass each other notifications or queue messages to pass messages and work. If queues arent used or are full, the more important tasks can be blocked by less important tasks. Usage of queues for each task leads to over-allocation of memory. 
* ❌ **No system bus or broadcasting**: Messages are only passed point to point, leading to increased amount of coupling. Tasks can only be blocked on a single queue, making it hard to handle messages of a different kind and origin. Events can't be broadcasted or published for the first taker.
* ❌ **Can be harder to manage**: Spreading execution across multiple priorities can easily make program logic and its state machine complex and opaque


### Single FreeRTOS task per object
Very often developers choose to sacrifice ability of a single object to do work with different priorities, and instead define a single task (and thus a single priority) for the whole object. It's a tempting approach as it keeps things simple, but it often leads to inefficiency.

#### Pros
* ✅ **Non-blocking:** If a task takes too much time, FreeRTOS will pause it to allow other tasks with the same priority do run
* ✅ **Low complexity**: A single task is easy to reason about
* ✅ **Simple execution flow**: Tasks code can be easy to follow and write, since the execution never leaves task function. Tasks may use stack variables to store intermediate results, and points of asynchrony are easy to see.
#### Cons
* ❌ **Single thread**: If an object does some work, it is not responsive for (potentially more important) input, like a command to abort the task
* ❌ **No priorities**: There is no way for an object to identify that some of its functions are more important than others. If it starts taking on unimportant work, it has to finish it before starting on something else.
* ❌ **Memory over-allocation**: Memory for each task stack and its queues has to be allocated upfront to handle bursts of activity without losing any messages or input. There is a lot of memory left unused, because it had to be allocated just in case or because it is needed once.
* ❌ **No system bus or broadcasting**: Actors can only talk one-to-one, needing to know about each other's queues. Tasks can only be blocked on a single queue, making it hard to handle messages of different kind and origin. Events can't be broadcasted or published for the first taker.


### System-wide shared threads
STM32.app provides a primitive called thread - a combination of task, a queue and a timer manager. STM32.app defines a small set of threads with different priorities. Actors in the system can declare their work to run in any of the builtin threads without overhead. This however prevents time-slicing to be effective, so the code has to be written in a way to avoid blocking as much as possible. 

#### Pros
* ✅ **Flexible**: Actors can do work with however many different priorities they need to
* ✅ **Simple**: Actors dont need to deal with task lifecycle or their memory requirements
* ✅ **Per-thread event bus**: Actors share a message queue provided by the thread. This dramatically reduces memory over-allocation and reduces potential for bottle necks. The bus holds events of various kinds allowing objects to receieve input from multiple places. 
* ✅ **Broadcasting**: All objects that use thread have a chance to receive incoming messages, handle it exclusively or let others handle it too 
* ✅ **Responsive**: Input is processed with highest priority, allowing objects to abort or change their workload. Actors can signal to input thread that they are currently unable to take on the new input, causing event to be off-load edthe event to a backlog bus. This ensures input bus is never blocked and events are never lost.
* ✅ **Customizable**: Some objects may choose to define their own threads in addition to ones provided by the system. In that case they will reuse all the features that threads provide (event bus, timers, etc), retaining all of the control over time slicing, execution flow and blocking that typical FreeRTOS tasks provide.   
#### Cons
* ❌ **Limited time-slicing**: Since objects share tasks, workloads of the same priority block each other. FreeRTOS time slicing which usually grants ability for tasks of the same priority to run for a bit, even if some take too much time does not help in this case. Thus the code has to be written in a way that avoid blocking the CPU as much as possible.
  * ✅ **Thoughtful built-ins**: All standard objects and drivers leverage non-blocking features like DMA to avoid blocking
  * ✅ **Customizable**: Some objects may choose to define their own threads in addition to ones provided by the system. In that case they will reuse all the features that threads provide (event bus, timers, etc), retaining all of the control over time slicing, execution flow and blocking that typical FreeRTOS tasks provide. However just like with regular FreeRTOS, developer would have to manually decide how much memory should be allocated for thread's stack and optional queue.
* ❌ **Harder to follow**: Spreading work across tasks with multiple priorities can make the execution flow opaque and complicated. 
  * ✅ **Task primitive**: STM32.app offers a very lightweight approach to writing asynchronous code that contains all of its steps close to each other in a state machine. All the minutiae is done behind the scenes, and handling of input and switching priorities mid-way is separated from the logical flow of a task. This often makes the code even easier to read than even with more advanced languages async/await or couroutines.


## Primitives

  ### CANopen node
  CANopen is a higher-level layer built on top of CAN bus which itself is decentralized. Each node in the network is defined by its Object Dictionary, which is an observable registry that configures behavior of a node. 
  * **Can work without network**: Even though CANopen is made for networking, since it is fully decentralized, devices are operational without the network 
  * **Composable**: ODs are meant to provide scaffolding for typical devices (elevators, hospital beds, vehicles, etc.) that can be customized and layered together for complex and reusable behaviors. Often new nodes can be configured by disabling features, instead of adding them, allowing a single codebase powering devices with different features.
  * **Uniquely-addressable** There can be up to 255 networked devices and each configuration property of each device has its own address in the network. There is a builtin mechanism to acquire unique node address and negotiate bitrate.
  * **Networked**: Properties in the registry may be designated as chanagable remotely, so devices can reconfigure each other. The same mechanism is used to make devices communicate with each other - instead of sending abstract events, they send requests to modify each other's state.
  * **Robust**: There're multiple layers of prioritization, safety, error correction, failure recovery, monitoring and network operation built into the system. CAN is used in safety-critical applications and comes from automotive industry.
  * **Synchronizable**: Provides mechanisms to sync timestamps and emulate synchronous execution of commands (i.e. reporting metrics at the same exact moment) in an inherently blocking network.
  

  ### Actor
  Actors are individual objects representing a single standalone feature (devices, subsystems, peripherals, etc). Each actor corresponds to entry in CANopen Object Dictionary. They define callbacks and event handlers, and receieve messages from message bus. Some actors may compose together and link to others (e.g. EEPROM to SPI transport) to be able to send signals to each other directly, but generally they communicate by publishing events to the message bus. 

  * **Observable properties**: All configuration options of an actor, together with its changable runtime properties are listed in an Object Dictionary. Devices on the CAN bus are able manipulate each other's state, and the actors will react to those state changes. Observability and reporting of state changes to other nodes is fully configurable and is built in to the CANopen system.
  * **Loose coupling**: Since STM32.app is written in C, there is no subclassing of the objects. A generic container struct with unified API interface is used when objects need to communicate to each other. They send each other signals and events, instead of calling methods reducing logical coupling. Actors may subscribe to events broadcasted over the bus, or recieve events directed point-to-point. An actor that consumed and handled event, can later report to producer actor when the work is complete.
  * **Prioritized execution**: Actors can have parts of their logic run and schedule with different priorities with no extra overhead of spawning tasks and queues.


  ### Thread
  Threads is an abstraction of prioritized task, and event bus with dispatcher. Multiple actors share the same task provided by the thread, reducing the total memory consumption.

  * **Cooperation**: Threads have different priorities, to make more important things run before less important
  * **Event bus**: Threads provide a pub/sub queue for incoming events, that can be dispatched to subscribed objects. Using unified queue is characterized by predictable and managable memory footprint, and ability to mix events of different types coming from different sources. Events can be broadcasted, served to first taker, or sent directly. Actors may ask the thread to keep the event in a backlog for later processing.
  * **Alarm management**: Threads allow devices to schedule periodical or delayed execution with precision of 1ms at almost no overhead. Higher precision can be achieved by using the timer modules.

# Networking
Internal changes of state can be broardcasted to network as individual values or packed into PDO objects (8 bytes). CANopen provides ability to snapshot and send data from multiple actors simultaneously by simulating synchronous messaging. Actors can acquire addresses automatically, then can be monitored via heartbeats and orchestrated to boot up at once. Master nodes can log errors on the network and report issues.