### 1. What is SDCP?

SDCP or Scalable Device Communication Protocol is a scalable way of connecting many devices together in a master/slave configuration. This is a serial / parallel (more on that later) and synchronous communication protocol. This protocol is scalable because it depends on the requirements for your project: you can choose the clock speed and the number of lanes to communicate on, you can also create your own packets (more details below). There is currently no limit for how many lanes can be used at once. Furthermore, if slave devices cannot use all available lanes the master can allow the slave to communicate with the device’s preferred number of lanes. For example, if the device can use a maximum of 4 lanes but 8 lanes are available then all devices that support 8 lanes will run on 8 lanes but the device that doesn’t support that will run on 4 lanes. The protocol needs at least 2 connections and it scales with the number of lanes: 1 for the clock and x for the lanes (minimum of 1), one lane corresponds to one bit per clock cycle.

The maximum number of devices on a single network is 255 (+1 for the master) but many more can be used: each device has a unique ID in the network but devices of the same type and working in the same way can have the same ID. For example, if you have 5 internal lights and 2 external lights, you can use one ID for the 2 external lights and another ID for the 5 internal lights because they are controlled the same but it works only one way around (only master -> multiple slaves but not slaves -> master). 

Warning: Only one device can communicate at once, if two devices are sending data at the same time the data might be corrupted or not routed properly.



### 2. Basics of the SDCP Protocol:

All ongoing packets must have a similar structure:


<table>
  <tr>
   <td>Header, 48 bits (6 bytes)
   </td>
   <td>Packet, 40 bits at least (5 bytes)
   </td>
  </tr>
</table>



<table>
  <tr>
   <td>Target Device ID
<p>
8 bits (1 byte)
   </td>
   <td>Packet Length*
<p>
32 bits (4 bytes)
   </td>
   <td>Header Checksum 
<p>
8 bits (1 byte)
   </td>
   <td>Packet ID
<p>
32 bits (4 bytes)
   </td>
   <td>Payload
<p>
8+ bits (at least 1 byte)
   </td>
  </tr>
</table>


All implementations of the protocol :


<table>
  <tr>
   <td><em>Packet ID</em>
   </td>
   <td><em>Payload structure</em>
   </td>
   <td><em>Description</em>
   </td>
  </tr>
  <tr>
   <td>0x0
   </td>
   <td>Byte (Master device id) + String (master device name, this field is optional)
   </td>
   <td><strong>Discover:</strong> (Master->Slaves) Asks for all device’s protocol details
   </td>
  </tr>
  <tr>
   <td>0x1
   </td>
   <td>Byte (Sender device ID) + String (device name, at least 2 char) + Integer (number of assignables lanes) + Byte (encryption field**)
   </td>
   <td><strong>Device Info:</strong> (Slave->Master in response of #0) The slave must respond with their hardware details like the name, the number of assignable lanes (max lanes supported on the device)
   </td>
  </tr>
  <tr>
   <td>0x2
   </td>
   <td>Byte (Sender device ID) + Integer (assigned number of lanes) + Short (encryption field**)
   </td>
   <td><strong>Set mode:</strong> (Master->Slave in response of #1) Master sets the mode of the device (how many lanes can be used) and validate or not the encryption field**
   </td>
  </tr>
  <tr>
   <td>0x3
   </td>
   <td>Byte (Sender device ID) + Byte (Mode***) + Integer (assigned number of lanes) + Short (encryption field**)
   </td>
   <td><strong>Switch mode:</strong> (Slave->Master) The slave can ask to the master to switch the lane configuration (add or remove lanes) or update the encryption field (activate or deactivate encryption)
   </td>
  </tr>
  <tr>
   <td>0x4
   </td>
   <td>Byte (Sender device ID) + Byte (Pause Flags****) + Long (Pause period / timeout)
   </td>
   <td><strong>Line pause:</strong> (Master->Slaves) The master device can pause the data bus for a short amount of time (it should not exceed 1s)  or (if the hold line is available)
   </td>
  </tr>
  <tr>
   <td>0x5
   </td>
   <td>Byte (Sender device ID) + Long (Timestamp)
   </td>
   <td><strong>Line test:</strong> (Both ways)
   </td>
  </tr>
  <tr>
   <td>0x6
   </td>
   <td>
   </td>
   <td><strong>Line OK:</strong> (Both ways as a response)
   </td>
  </tr>
  <tr>
   <td>0x7
   </td>
   <td>[Any data model can be used according to your configuration or depending on the device]
   </td>
   <td><strong>Transaction Start</strong> (Slave->Master or Master->Slave)
   </td>
  </tr>
  <tr>
   <td>0x8
   </td>
   <td>[Any data model can be used according to your configuration or depending on the device]
   </td>
   <td><strong>Transaction End:</strong> (Master->Slave or Slave->Master)
   </td>
  </tr>
  <tr>
   <td>0x9
   </td>
   <td>Byte (Sender device ID)
   </td>
   <td><strong>Disconnect:</strong> (Master->Slave) The device is not allowed anymore to use the communication protocol on this network. Warning: the client cannot read any data on the bus (lanes) and to reactivate it you must restart it from the device itself
   </td>
  </tr>
</table>


**Encryption field: <TODO>

***__Mode__: The mode byte is a bit mask, available masks:


<table>
  <tr>
   <td>Not Assigned
   </td>
   <td>Not Assigned
   </td>
   <td>Not Assigned
   </td>
   <td>Not Assigned
   </td>
   <td>Not Assigned
   </td>
   <td>Not Assigned
   </td>
   <td>Encryption active
   </td>
   <td>Updated number of lanes
   </td>
  </tr>
  <tr>
   <td>0x80
   </td>
   <td>0x40
   </td>
   <td>0x20
   </td>
   <td>0x10
   </td>
   <td>0x8
   </td>
   <td>0x4
   </td>
   <td>0x2
   </td>
   <td>0x1
   </td>
  </tr>
</table>


****<span style="text-decoration:underline;">Pause flags:</span> The pause flag byte is a bit mask, available masks are:


<table>
  <tr>
   <td>Not Assigned
   </td>
   <td>Not Assigned
   </td>
   <td>Not Assigned
   </td>
   <td>Not Assigned
   </td>
   <td>Not Assigned
   </td>
   <td>Reset mode on Recover
   </td>
   <td>Recover with Hold event
   </td>
   <td>Recover with Time
   </td>
  </tr>
  <tr>
   <td>0x80
   </td>
   <td>0x40
   </td>
   <td>0x20
   </td>
   <td>0x10
   </td>
   <td>0x8
   </td>
   <td>0x4
   </td>
   <td>0x2
   </td>
   <td>0x1
   </td>
  </tr>
</table>


_Details:_

**Recover with Time:** This flag triggers the Recover after a timeout in microseconds, the timeout actually starts after the next clock pulse or the next hold event.

**Recover with Hold Event:** This flag triggers the Recover within the next clock pulse or hold event, be aware that once clock pulse or hold event is required to activate this mode. _<span style="text-decoration:underline;">Note:</span>_ we can use both recover with time and with hold event, the recover with hold event will trigger first and the recover with time will act like a fallback recovery.

**Reset mode on Recover:** This flag resets the current mode of the device (assigned lanes and encryption), the master must use 1 lane until the device

### TODO
error correction + hot plug specification