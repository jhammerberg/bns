<!-- Source: https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html (ESP-IDF v6.1.1, fetched 2026-11-20 via Firecrawl) -->
- [Home](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/index.html)
- [API Reference](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/index.html)
- [Networking APIs](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/index.html)
- ESP-NOW
- [Edit on GitHub](https://github.com/espressif/esp-idf/blob/v6.1.1/docs/en/api-reference/network/esp_now.rst)

* * *

# ESP-NOW [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html\#esp-now "Permalink to this heading")

[\[中文\]](https://docs.espressif.com/projects/esp-idf/zh_CN/v6.1.1/esp32c6/api-reference/network/esp_now.html)

## Overview [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html\#overview "Permalink to this heading")

ESP-NOW is a kind of connectionless Wi-Fi communication protocol that is defined by Espressif. In ESP-NOW, application data is encapsulated in a vendor-specific action frame and then transmitted from one Wi-Fi device to another without connection.

CTR with CBC-MAC Protocol (CCMP) is used to protect the action frame for security. ESP-NOW is widely used in smart light, remote controlling, sensor, etc.

## Frame Format [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html\#frame-format "Permalink to this heading")

ESP-NOW uses a vendor-specific action frame to transmit ESP-NOW data. The default ESP-NOW bit rate is 1 Mbps.

Currently, ESP-NOW supports two versions: v1.0 and v2.0. The maximum packet length supported by v2.0 devices is 1470 (`ESP_NOW_MAX_DATA_LEN_V2`) bytes, while the maximum packet length supported by v1.0 devices is 250 (`ESP_NOW_MAX_DATA_LEN`) bytes.

The v2.0 devices are capable of receiving packets from both v2.0 and v1.0 devices. In contrast, v1.0 devices can only receive packets from other v1.0 devices.

However, v1.0 devices can receive v2.0 packets if the packet length is less than or equal to 250 (`ESP_NOW_MAX_IE_DATA_LEN`).For packets exceeding this length, the v1.0 devices will either truncate the data to the first 250 (`ESP_NOW_MAX_IE_DATA_LEN`) bytes or discard the packet entirely.

For detailed behavior, please refer to the documentation corresponding to the specific IDF version.

The format of the vendor-specific action frame is as follows:

```
------------------------------------------------------------------------------------------------------------
| MAC Header | Category Code | Organization Identifier | Random Values | Vendor Specific Content |   FCS   |
------------------------------------------------------------------------------------------------------------
  24 bytes         1 byte              3 bytes               4 bytes             7-x bytes        4 bytes
```

Copy to clipboard

- Category Code: The Category Code field is set to the value (127) indicating the vendor-specific category.

- Organization Identifier: The Organization Identifier contains a unique identifier (0x18fe34), which is the first three bytes of MAC address applied by Espressif.

- Random Value: The Random Value filed is used to prevents relay attacks.

- Vendor Specific Content: The Vendor Specific Content contains several (at least one) vendor-specific element fields. For version v2.0, x = 1512(1470 + 6\*7), for version v1.0, x = 257(250 + 7).


The format of the vendor-specific element frame is as follows:

```
ESP-NOW v1.0:
------------------------------------------------------------------------------------------
| Element ID | Length | Organization Identifier | Type | Reserved | Version |    Body    |
------------------------------------------------------------------------------------------
                                                         7~4 bits | 3~0 bits
    1 byte     1 byte            3 bytes         1 byte        1 byte         0-250 bytes

ESP-NOW v2.0:
-----------------------------------------------------------------------------------------------------
| Element ID | Length | Organization Identifier | Type | Reserved | More data |Version |    Body    |
-----------------------------------------------------------------------------------------------------
                                                         7~5 bits |   1 bit   |3~0 bits
    1 byte     1 byte            3 bytes         1 byte             1 byte                0-250 bytes
```

Copy to clipboard

- Element ID: The Element ID field is set to the value (221), indicating the vendor-specific element.

- Length: The length is the total length of Organization Identifier, Type, Version and Body, the maximum value is 255.

- Organization Identifier: The Organization Identifier contains a unique identifier (0x18fe34), which is the first three bytes of MAC address applied by Espressif.

- Type: The Type field is set to the value (4) indicating ESP-NOW.

- Version: The Version field is set to the version of ESP-NOW.

- Body: The Body contains the actual ESP-NOW data to be transmitted.


As ESP-NOW is connectionless, the MAC header is a little different from that of standard frames. The FromDS and ToDS bits of FrameControl field are both 0. The first address field is set to the destination address. The second address field is set to the source address. The third address field is set to broadcast address (0xff:0xff:0xff:0xff:0xff:0xff).

## Security [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html\#security "Permalink to this heading")

ESP-NOW uses the CCMP method, which is described in IEEE Std. 802.11-2012, to protect the vendor-specific action frame. The Wi-Fi device maintains a Primary Master Key (PMK) and several Local Master Keys (LMKs, each paired device has one LMK). The lengths of both PMK and LMK are 16 bytes.

> - PMK is used to encrypt LMK with the AES-128 algorithm. Call [`esp_now_set_pmk()`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv415esp_now_set_pmkPK7uint8_t) to set PMK. If PMK is not set, a default PMK will be used.
>
> - LMK of the paired device is used to encrypt the vendor-specific action frame with the CCMP method. If the LMK of the paired device is not set, the vendor-specific action frame will not be encrypted.

Encrypting multicast vendor-specific action frame is not supported.

## Initialization and Deinitialization [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html\#initialization-and-deinitialization "Permalink to this heading")

Call [`esp_now_init()`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv412esp_now_initv) to initialize ESP-NOW and [`esp_now_deinit()`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv414esp_now_deinitv) to de-initialize ESP-NOW. ESP-NOW data must be transmitted after Wi-Fi is started, so it is recommended to start Wi-Fi before initializing ESP-NOW and stop Wi-Fi after de-initializing ESP-NOW.

When [`esp_now_deinit()`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv414esp_now_deinitv) is called, all of the information of paired devices are deleted.

## Add Paired Device [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html\#add-paired-device "Permalink to this heading")

Call [`esp_now_add_peer()`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv416esp_now_add_peerPK19esp_now_peer_info_t) to add the device to the paired device list before you send data to this device. If security is enabled, the LMK must be set. A device with a broadcast MAC address must be added before sending broadcast data.

You can send ESP-NOW data via both the Station and the SoftAP interface. Make sure that the interface is enabled before sending ESP-NOW data.

The range of the channel of paired devices is from 0 to 14. If the channel is set to 0, data will be sent on the current channel. Otherwise, the channel must be set as the channel that the local device is on.

For the receiving device, calling [`esp_now_add_peer()`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv416esp_now_add_peerPK19esp_now_peer_info_t) is not required. If no paired device is added, it can only receive broadcast packets and unencrypted unicast packets. To receive encrypted unicast packets, a paired device must be added, and the same LMK must be set.

The maximum number of paired devices is 20, and the paired encryption devices are no more than 17, the default is 7. If you want to change the number of paired encryption devices, set [CONFIG\_ESP\_WIFI\_ESPNOW\_MAX\_ENCRYPT\_NUM](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/kconfig-reference.html#config-esp-wifi-espnow-max-encrypt-num) in the Wi-Fi component configuration menu.

## Send ESP-NOW Data [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html\#send-esp-now-data "Permalink to this heading")

Call [`esp_now_send()`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv412esp_now_sendPK7uint8_tPK7uint8_t6size_t) to send ESP-NOW data and [`esp_now_register_send_cb()`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv424esp_now_register_send_cb17esp_now_send_cb_t) to register sending callback function. It will return ESP\_NOW\_SEND\_SUCCESS in sending callback function if the data is received successfully on the MAC layer. Otherwise, it will return ESP\_NOW\_SEND\_FAIL. Several reasons can lead to ESP-NOW fails to send data. For example, the destination device does not exist; the channels of the devices are not the same; the action frame is lost when transmitting on the air, etc. It is not guaranteed that application layer can receive the data. If necessary, send back ack data when receiving ESP-NOW data. If receiving ack data timeouts, retransmit the ESP-NOW data. A sequence number can also be assigned to ESP-NOW data to drop the duplicate data.

If there is a lot of ESP-NOW data to send, call [`esp_now_send()`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv412esp_now_sendPK7uint8_tPK7uint8_t6size_t) to send less than or equal to the maximum packet length (v1.0 is 250 bytes, v2.0 is 1470 bytes) of data once a time. Note that too short interval between sending two ESP-NOW data may lead to disorder of sending callback function. So, it is recommended that sending the next ESP-NOW data after the sending callback function of the previous sending has returned. The sending callback function runs from a high-priority Wi-Fi task. So, do not do lengthy operations in the callback function. Instead, post the necessary data to a queue and handle it from a lower priority task.

## Receiving ESP-NOW Data [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html\#receiving-esp-now-data "Permalink to this heading")

Call [`esp_now_register_recv_cb()`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv424esp_now_register_recv_cb17esp_now_recv_cb_t) to register receiving callback function. Call the receiving callback function when receiving ESP-NOW. The receiving callback function also runs from the Wi-Fi task. So, do not do lengthy operations in the callback function.
Instead, post the necessary data to a queue and handle it from a lower priority task.

## Config ESP-NOW Rate [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html\#config-esp-now-rate "Permalink to this heading")

Call [`esp_now_set_peer_rate_config()`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv428esp_now_set_peer_rate_configPK7uint8_tP21esp_now_rate_config_t) to configure ESP-NOW rate of each peer. Make sure that the peer is added before configuring the rate. This API should be called after [`esp_wifi_start()`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_wifi.html#_CPPv414esp_wifi_startv) and [`esp_now_add_peer()`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv416esp_now_add_peerPK19esp_now_peer_info_t).

## Config ESP-NOW Power-saving Parameter [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html\#config-esp-now-power-saving-parameter "Permalink to this heading")

Sleep is supported only when ESP32-C6 is configured as station.

Call [`esp_now_set_wake_window()`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv423esp_now_set_wake_window8uint16_t) to configure Window for ESP-NOW RX at sleep. The default value is the maximum, which allowing RX all the time.

If Power-saving is needed for ESP-NOW, call [`esp_wifi_connectionless_module_set_wake_interval()`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_wifi.html#_CPPv448esp_wifi_connectionless_module_set_wake_interval8uint16_t) to configure Interval as well.

Please refer to [connectionless module power save](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-guides/wifi-driver/wifi-performance-and-power-save.html#connectionless-module-power-save) to get more detail.

## Application Examples [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html\#application-examples "Permalink to this heading")

- [wifi/espnow](https://github.com/espressif/esp-idf/tree/v6.1.1/examples/wifi/espnow) demonstrates how to use the ESPNOW feature of ESP32-C6's Wi-Fi, including starting Wi-Fi, initializing ESP-NOW, registering ESP-NOW sending or receiving callback function, adding ESP-NOW peer information, and sending and receiving ESP-NOW data between two devices.


## API Reference [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html\#api-reference "Permalink to this heading")

### Header File [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html\#header-file "Permalink to this heading")

- [components/esp\_wifi/include/esp\_now.h](https://github.com/espressif/esp-idf/blob/v6.1.1/components/esp_wifi/include/esp_now.h)

- This header file can be included with:


> ```
> #include "esp_now.h"
> ```
>
> Copy to clipboard

- This header file is a part of the API provided by the `esp_wifi` component. To declare that your component depends on `esp_wifi`, add the following to your CMakeLists.txt:


> ```
> REQUIRES esp_wifi
> ```
>
> Copy to clipboard
>
> or
>
> ```
> PRIV_REQUIRES esp_wifi
> ```
>
> Copy to clipboard


### Functions [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html\#functions "Permalink to this heading")

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/system/esp_err.html#_CPPv49esp_err_t) esp\_now\_init(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv412esp_now_initv "Permalink to this definition")

Initialize ESPNOW function.

Returns:

- ESP\_OK : succeed

- ESP\_ERR\_ESPNOW\_INTERNAL : Internal error


[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/system/esp_err.html#_CPPv49esp_err_t) esp\_now\_deinit(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv414esp_now_deinitv "Permalink to this definition")

De-initialize ESPNOW function.

Returns:

- ESP\_OK : succeed


[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/system/esp_err.html#_CPPv49esp_err_t) esp\_now\_get\_version(uint32\_t\*version) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv419esp_now_get_versionP8uint32_t "Permalink to this definition")

Get the version of ESPNOW. Currently, ESPNOW supports two versions: v1.0 and v2.0.

```
       The v2.0 devices are capable of receiving packets from both v2.0 and v1.0 devices. In contrast, v1.0 devices can only receive packets from other v1.0 devices.
       However, v1.0 devices can receive v2.0 packets if the packet length is less than or equal to ESP_NOW_MAX_IE_DATA_LEN.
       For packets exceeding this length, the v1.0 devices will either truncate the data to the first ESP_NOW_MAX_IE_DATA_LEN bytes or discard the packet entirely.
       For detailed behavior, please refer to the documentation corresponding to the specific IDF version.
```

Copy to clipboard

Parameters:

**version** \-\- ESPNOW version

Returns:

- ESP\_OK : succeed

- ESP\_ERR\_ESPNOW\_ARG : invalid argument


[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/system/esp_err.html#_CPPv49esp_err_t) esp\_now\_register\_recv\_cb( [esp\_now\_recv\_cb\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv417esp_now_recv_cb_t) cb) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv424esp_now_register_recv_cb17esp_now_recv_cb_t "Permalink to this definition")

Register callback function of receiving ESPNOW data.

Parameters:

**cb** \-\- callback function of receiving ESPNOW data

Returns:

- ESP\_OK : succeed

- ESP\_ERR\_ESPNOW\_NOT\_INIT : ESPNOW is not initialized

- ESP\_ERR\_ESPNOW\_INTERNAL : internal error


[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/system/esp_err.html#_CPPv49esp_err_t) esp\_now\_unregister\_recv\_cb(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv426esp_now_unregister_recv_cbv "Permalink to this definition")

Unregister callback function of receiving ESPNOW data.

Returns:

- ESP\_OK : succeed

- ESP\_ERR\_ESPNOW\_NOT\_INIT : ESPNOW is not initialized


[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/system/esp_err.html#_CPPv49esp_err_t) esp\_now\_register\_send\_cb( [esp\_now\_send\_cb\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv417esp_now_send_cb_t) cb) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv424esp_now_register_send_cb17esp_now_send_cb_t "Permalink to this definition")

Register callback function of sending ESPNOW data.

Parameters:

**cb** \-\- callback function of sending ESPNOW data

Returns:

- ESP\_OK : succeed

- ESP\_ERR\_ESPNOW\_NOT\_INIT : ESPNOW is not initialized

- ESP\_ERR\_ESPNOW\_INTERNAL : internal error


[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/system/esp_err.html#_CPPv49esp_err_t) esp\_now\_unregister\_send\_cb(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv426esp_now_unregister_send_cbv "Permalink to this definition")

Unregister callback function of sending ESPNOW data.

Returns:

- ESP\_OK : succeed

- ESP\_ERR\_ESPNOW\_NOT\_INIT : ESPNOW is not initialized


[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/system/esp_err.html#_CPPv49esp_err_t) esp\_now\_send(constuint8\_t\*peer\_addr, constuint8\_t\*data, size\_tlen) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv412esp_now_sendPK7uint8_tPK7uint8_t6size_t "Permalink to this definition")

Send ESPNOW data.

**Attention**

1\. If peer\_addr is not NULL, send data to the peer whose MAC address matches peer\_addr

**Attention**

2\. If peer\_addr is NULL, send data to all of the peers that are added to the peer list

**Attention**

3\. The maximum length of data must be less than ESP\_NOW\_MAX\_DATA\_LEN

**Attention**

4\. The buffer pointed to by data argument does not need to be valid after esp\_now\_send returns

Parameters:

- **peer\_addr** \-\- peer MAC address

- **data** \-\- data to send

- **len** \-\- length of data


Returns:

- ESP\_OK : succeed

- ESP\_ERR\_ESPNOW\_NOT\_INIT : ESPNOW is not initialized

- ESP\_ERR\_ESPNOW\_ARG : invalid argument

- ESP\_ERR\_ESPNOW\_INTERNAL : internal error

- ESP\_ERR\_ESPNOW\_NO\_MEM : out of memory, when this happens, you can delay a while before sending the next data

- ESP\_ERR\_ESPNOW\_NOT\_FOUND : peer is not found

- ESP\_ERR\_ESPNOW\_IF : current Wi-Fi interface doesn't match that of peer

- ESP\_ERR\_ESPNOW\_CHAN: current Wi-Fi channel doesn't match that of peer


[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/system/esp_err.html#_CPPv49esp_err_t) esp\_now\_add\_peer(const [esp\_now\_peer\_info\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv419esp_now_peer_info_t)\*peer) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv416esp_now_add_peerPK19esp_now_peer_info_t "Permalink to this definition")

Add a peer to peer list.

Parameters:

**peer** \-\- peer information

Returns:

- ESP\_OK : succeed

- ESP\_ERR\_ESPNOW\_NOT\_INIT : ESPNOW is not initialized

- ESP\_ERR\_ESPNOW\_ARG : invalid argument

- ESP\_ERR\_ESPNOW\_FULL : peer list is full

- ESP\_ERR\_ESPNOW\_NO\_MEM : out of memory

- ESP\_ERR\_ESPNOW\_EXIST : peer has existed


[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/system/esp_err.html#_CPPv49esp_err_t) esp\_now\_del\_peer(constuint8\_t\*peer\_addr) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv416esp_now_del_peerPK7uint8_t "Permalink to this definition")

Delete a peer from peer list.

Parameters:

**peer\_addr** \-\- peer MAC address

Returns:

- ESP\_OK : succeed

- ESP\_ERR\_ESPNOW\_NOT\_INIT : ESPNOW is not initialized

- ESP\_ERR\_ESPNOW\_ARG : invalid argument

- ESP\_ERR\_ESPNOW\_NOT\_FOUND : peer is not found


[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/system/esp_err.html#_CPPv49esp_err_t) esp\_now\_mod\_peer(const [esp\_now\_peer\_info\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv419esp_now_peer_info_t)\*peer) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv416esp_now_mod_peerPK19esp_now_peer_info_t "Permalink to this definition")

Modify a peer.

Parameters:

**peer** \-\- peer information

Returns:

- ESP\_OK : succeed

- ESP\_ERR\_ESPNOW\_NOT\_INIT : ESPNOW is not initialized

- ESP\_ERR\_ESPNOW\_ARG : invalid argument

- ESP\_ERR\_ESPNOW\_FULL : peer list is full


[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/system/esp_err.html#_CPPv49esp_err_t) esp\_now\_set\_peer\_rate\_config(constuint8\_t\*peer\_addr, [esp\_now\_rate\_config\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv421esp_now_rate_config_t)\*config) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv428esp_now_set_peer_rate_configPK7uint8_tP21esp_now_rate_config_t "Permalink to this definition")

Set ESPNOW rate config for each peer.

**Attention**

1\. This API should be called after esp\_wifi\_start() and esp\_now\_init().

Parameters:

- **peer\_addr** \-\- peer MAC address

- **config** \-\- rate config to be configured.


Returns:

- ESP\_OK : succeed

- ESP\_ERR\_ESPNOW\_NOT\_INIT : ESPNOW is not initialized

- ESP\_ERR\_ESPNOW\_ARG : invalid argument

- ESP\_ERR\_ESPNOW\_INTERNAL : internal error


[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/system/esp_err.html#_CPPv49esp_err_t) esp\_now\_get\_peer(constuint8\_t\*peer\_addr, [esp\_now\_peer\_info\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv419esp_now_peer_info_t)\*peer) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv416esp_now_get_peerPK7uint8_tP19esp_now_peer_info_t "Permalink to this definition")

Get a peer whose MAC address matches peer\_addr from peer list.

Parameters:

- **peer\_addr** \-\- peer MAC address

- **peer** \-\- peer information


Returns:

- ESP\_OK : succeed

- ESP\_ERR\_ESPNOW\_NOT\_INIT : ESPNOW is not initialized

- ESP\_ERR\_ESPNOW\_ARG : invalid argument

- ESP\_ERR\_ESPNOW\_NOT\_FOUND : peer is not found


[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/system/esp_err.html#_CPPv49esp_err_t) esp\_now\_fetch\_peer(boolfrom\_head, [esp\_now\_peer\_info\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv419esp_now_peer_info_t)\*peer) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv418esp_now_fetch_peerbP19esp_now_peer_info_t "Permalink to this definition")

Fetch a peer from peer list. Only return the peer which address is unicast, for the multicast/broadcast address, the function will ignore and try to find the next in the peer list.

Parameters:

- **from\_head** \-\- fetch from head of list or not

- **peer** \-\- peer information


Returns:

- ESP\_OK : succeed

- ESP\_ERR\_ESPNOW\_NOT\_INIT : ESPNOW is not initialized

- ESP\_ERR\_ESPNOW\_ARG : invalid argument

- ESP\_ERR\_ESPNOW\_NOT\_FOUND : peer is not found


boolesp\_now\_is\_peer\_exist(constuint8\_t\*peer\_addr) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv421esp_now_is_peer_existPK7uint8_t "Permalink to this definition")

Peer exists or not.

Parameters:

**peer\_addr** \-\- peer MAC address

Returns:

- true : peer exists

- false : peer not exists


[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/system/esp_err.html#_CPPv49esp_err_t) esp\_now\_get\_peer\_num( [esp\_now\_peer\_num\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv418esp_now_peer_num_t)\*num) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv420esp_now_get_peer_numP18esp_now_peer_num_t "Permalink to this definition")

Get the number of peers.

Parameters:

**num** \-\- number of peers

Returns:

- ESP\_OK : succeed

- ESP\_ERR\_ESPNOW\_NOT\_INIT : ESPNOW is not initialized

- ESP\_ERR\_ESPNOW\_ARG : invalid argument


[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/system/esp_err.html#_CPPv49esp_err_t) esp\_now\_set\_pmk(constuint8\_t\*pmk) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv415esp_now_set_pmkPK7uint8_t "Permalink to this definition")

Set the primary master key.

**Attention**

1\. primary master key is used to encrypt local master key

Parameters:

**pmk** \-\- primary master key

Returns:

- ESP\_OK : succeed

- ESP\_ERR\_ESPNOW\_NOT\_INIT : ESPNOW is not initialized

- ESP\_ERR\_ESPNOW\_ARG : invalid argument


[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/system/esp_err.html#_CPPv49esp_err_t) esp\_now\_set\_wake\_window(uint16\_twindow) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv423esp_now_set_wake_window8uint16_t "Permalink to this definition")

Set wake window for esp\_now to wake up in interval unit.

**Attention**

1\. This configuration could work at connected status. When ESP\_WIFI\_STA\_DISCONNECTED\_PM\_ENABLE is enabled, this configuration could work at disconnected status.

**Attention**

2\. Default value is the maximum.

Parameters:

**window** \-\- Milliseconds would the chip keep waked each interval, from 0 to 65535.

Returns:

- ESP\_OK : succeed

- ESP\_ERR\_ESPNOW\_NOT\_INIT : ESPNOW is not initialized


[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/system/esp_err.html#_CPPv49esp_err_t) esp\_now\_set\_user\_oui(uint8\_t\*oui) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv420esp_now_set_user_ouiP7uint8_t "Permalink to this definition")

Set the OUI (Organization Identifier) in the vendor-specific element for ESPNOW.

Parameters:

**oui** \-\- The oui should occupy 3 bytes. If the oui is NULL, then use the default value (0x18fe34).

Returns:

- ESP\_OK : succeed


[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/system/esp_err.html#_CPPv49esp_err_t) esp\_now\_get\_user\_oui(uint8\_t\*oui) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv420esp_now_get_user_ouiP7uint8_t "Permalink to this definition")

Get the OUI (Organization Identifier) in the vendor-specific element for ESPNOW.

Parameters:

**oui** \-\- user configured OUI.

Returns:

- ESP\_OK : succeed

- ESP\_ERR\_ESPNOW\_ARG : invalid argument


[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/system/esp_err.html#_CPPv49esp_err_t) esp\_now\_switch\_channel\_tx( [esp\_now\_switch\_channel\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv424esp_now_switch_channel_t)\*config) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv425esp_now_switch_channel_txP24esp_now_switch_channel_t "Permalink to this definition")

ESPNOW switch to a specific channel for a required duration, and send one ESPNOW data.

**Attention**

1\. The data\_len should not exceed ESP\_NOW\_MAX\_DATA\_LEN\_V2.

**Attention**

2\. The sec\_channel must be set to a valid value.

**Attention**

3\. The status field in WIFI\_EVENT\_ACTION\_TX\_STATUS event has only two possible cases: WIFI\_ACTION\_TX\_OP\_CANCELLED and WIFI\_ACTION\_TX\_DURATION\_COMPLETED. For the TX status, ESP-NOW already has a TX callback to report it.

Parameters:

**config** \-\- ESPNOW switch channel relevant information

Returns:

- ESP\_OK : succeed

- ESP\_ERR\_NO\_MEM: failed to allocate memory

- ESP\_ERR\_INVALID\_ARG: the <channel, sec\_channel> pair is invalid; the data len is invalid; the dest mac is invalid.

- ESP\_FAIL: failed to send frame


[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/system/esp_err.html#_CPPv49esp_err_t) esp\_now\_remain\_on\_channel( [esp\_now\_remain\_on\_channel\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv427esp_now_remain_on_channel_t)\*config) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv425esp_now_remain_on_channelP27esp_now_remain_on_channel_t "Permalink to this definition")

ESPNOW remain on the target channel for required duration.

Parameters:

**config** \-\- ESPNOW remain on channel relevant information

Returns:

- ESP\_OK : succeed

- ESP\_ERR\_NO\_MEM: failed to allocate memory

- ESP\_ERR\_INVALID\_ARG: the <channel, sec\_channel> pair is invalid

- ESP\_FAIL: failed to perform roc operation


### Structures [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html\#structures "Permalink to this heading")

structesp\_now\_peer\_info [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv417esp_now_peer_info "Permalink to this definition")

ESPNOW peer information parameters.

Public Members

uint8\_tpeer\_addr\[ESP\_NOW\_ETH\_ALEN\] [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv4N17esp_now_peer_info9peer_addrE "Permalink to this definition")

ESPNOW peer MAC address that is also the MAC address of station or softap

uint8\_tlmk\[ESP\_NOW\_KEY\_LEN\] [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv4N17esp_now_peer_info3lmkE "Permalink to this definition")

ESPNOW peer local master key that is used to encrypt data

uint8\_tchannel [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv4N17esp_now_peer_info7channelE "Permalink to this definition")

Wi-Fi channel that peer uses to send/receive ESPNOW data. If the value is 0, use the current channel which station or softap is on. Otherwise, it must be set as the channel that station or softap is on.

[wifi\_interface\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_wifi.html#_CPPv416wifi_interface_t) ifidx [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv4N17esp_now_peer_info5ifidxE "Permalink to this definition")

Wi-Fi interface that peer uses to send/receive ESPNOW data

boolencrypt [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv4N17esp_now_peer_info7encryptE "Permalink to this definition")

ESPNOW data that this peer sends/receives is encrypted or not

void\*priv [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv4N17esp_now_peer_info4privE "Permalink to this definition")

ESPNOW peer private data

structesp\_now\_peer\_num [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv416esp_now_peer_num "Permalink to this definition")

Number of ESPNOW peers which exist currently.

Public Members

inttotal\_num [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv4N16esp_now_peer_num9total_numE "Permalink to this definition")

Total number of ESPNOW peers, maximum value is ESP\_NOW\_MAX\_TOTAL\_PEER\_NUM

intencrypt\_num [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv4N16esp_now_peer_num11encrypt_numE "Permalink to this definition")

Number of encrypted ESPNOW peers, maximum value is ESP\_NOW\_MAX\_ENCRYPT\_PEER\_NUM

structesp\_now\_recv\_info [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv417esp_now_recv_info "Permalink to this definition")

ESPNOW receive packet information.

Public Members

uint8\_t\*src\_addr [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv4N17esp_now_recv_info8src_addrE "Permalink to this definition")

Source address of ESPNOW packet

uint8\_t\*des\_addr [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv4N17esp_now_recv_info8des_addrE "Permalink to this definition")

Destination address of ESPNOW packet

[wifi\_pkt\_rx\_ctrl\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_wifi.html#_CPPv418wifi_pkt_rx_ctrl_t)\*rx\_ctrl [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv4N17esp_now_recv_info7rx_ctrlE "Permalink to this definition")

Rx control info of ESPNOW packet

structesp\_now\_switch\_channel\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv424esp_now_switch_channel_t "Permalink to this definition")

ESPNOW switch channel information.

Public Members

[wifi\_action\_tx\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_wifi.html#_CPPv416wifi_action_tx_t) type [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv4N24esp_now_switch_channel_t4typeE "Permalink to this definition")

ACTION TX operation type

uint8\_tchannel [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv4N24esp_now_switch_channel_t7channelE "Permalink to this definition")

Channel on which to perform ESPNOW TX Operation

[wifi\_second\_chan\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_wifi.html#_CPPv418wifi_second_chan_t) sec\_channel [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv4N24esp_now_switch_channel_t11sec_channelE "Permalink to this definition")

Secondary channel

uint32\_twait\_time\_ms [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv4N24esp_now_switch_channel_t12wait_time_msE "Permalink to this definition")

Duration to wait for on target channel

uint8\_top\_id [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv4N24esp_now_switch_channel_t5op_idE "Permalink to this definition")

Unique Identifier for operation provided by wifi driver

uint8\_tdest\_mac\[6\] [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv4N24esp_now_switch_channel_t8dest_macE "Permalink to this definition")

Destination MAC address

uint16\_tdata\_len [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv4N24esp_now_switch_channel_t8data_lenE "Permalink to this definition")

Length of the appended Data

uint8\_tdata\[0\] [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv4N24esp_now_switch_channel_t4dataE "Permalink to this definition")

Appended Data payload

structesp\_now\_remain\_on\_channel\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv427esp_now_remain_on_channel_t "Permalink to this definition")

ESPNOW remain on channel information.

Public Members

[wifi\_roc\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_wifi.html#_CPPv410wifi_roc_t) type [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv4N27esp_now_remain_on_channel_t4typeE "Permalink to this definition")

ROC operation type

uint8\_tchannel [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv4N27esp_now_remain_on_channel_t7channelE "Permalink to this definition")

Channel on which to perform ESPNOW ROC Operation

[wifi\_second\_chan\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_wifi.html#_CPPv418wifi_second_chan_t) sec\_channel [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv4N27esp_now_remain_on_channel_t11sec_channelE "Permalink to this definition")

Secondary channel

uint32\_twait\_time\_ms [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv4N27esp_now_remain_on_channel_t12wait_time_msE "Permalink to this definition")

Duration to wait for on target channel

uint8\_top\_id [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv4N27esp_now_remain_on_channel_t5op_idE "Permalink to this definition")

ID of this specific ROC operation provided by wifi driver

### Macros [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html\#macros "Permalink to this heading")

ESP\_ERR\_ESPNOW\_BASE [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#c.ESP_ERR_ESPNOW_BASE "Permalink to this definition")

ESPNOW error number base.

ESP\_ERR\_ESPNOW\_NOT\_INIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#c.ESP_ERR_ESPNOW_NOT_INIT "Permalink to this definition")

ESPNOW is not initialized.

ESP\_ERR\_ESPNOW\_ARG [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#c.ESP_ERR_ESPNOW_ARG "Permalink to this definition")

Invalid argument

ESP\_ERR\_ESPNOW\_NO\_MEM [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#c.ESP_ERR_ESPNOW_NO_MEM "Permalink to this definition")

Out of memory

ESP\_ERR\_ESPNOW\_FULL [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#c.ESP_ERR_ESPNOW_FULL "Permalink to this definition")

ESPNOW peer list is full

ESP\_ERR\_ESPNOW\_NOT\_FOUND [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#c.ESP_ERR_ESPNOW_NOT_FOUND "Permalink to this definition")

ESPNOW peer is not found

ESP\_ERR\_ESPNOW\_INTERNAL [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#c.ESP_ERR_ESPNOW_INTERNAL "Permalink to this definition")

Internal error

ESP\_ERR\_ESPNOW\_EXIST [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#c.ESP_ERR_ESPNOW_EXIST "Permalink to this definition")

ESPNOW peer has existed

ESP\_ERR\_ESPNOW\_IF [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#c.ESP_ERR_ESPNOW_IF "Permalink to this definition")

Interface error

ESP\_ERR\_ESPNOW\_CHAN [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#c.ESP_ERR_ESPNOW_CHAN "Permalink to this definition")

Channel error

ESP\_NOW\_ETH\_ALEN [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#c.ESP_NOW_ETH_ALEN "Permalink to this definition")

Length of ESPNOW peer MAC address

ESP\_NOW\_KEY\_LEN [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#c.ESP_NOW_KEY_LEN "Permalink to this definition")

Length of ESPNOW peer local master key

ESP\_NOW\_MAX\_TOTAL\_PEER\_NUM [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#c.ESP_NOW_MAX_TOTAL_PEER_NUM "Permalink to this definition")

Maximum number of ESPNOW total peers

ESP\_NOW\_MAX\_ENCRYPT\_PEER\_NUM [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#c.ESP_NOW_MAX_ENCRYPT_PEER_NUM "Permalink to this definition")

Maximum number of ESPNOW encrypted peers

ESP\_NOW\_MAX\_IE\_DATA\_LEN [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#c.ESP_NOW_MAX_IE_DATA_LEN "Permalink to this definition")

Maximum data length in a vendor-specific element

ESP\_NOW\_MAX\_DATA\_LEN [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#c.ESP_NOW_MAX_DATA_LEN "Permalink to this definition")

Maximum length of data sent in each ESPNOW transmission for v1.0

ESP\_NOW\_MAX\_DATA\_LEN\_V2 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#c.ESP_NOW_MAX_DATA_LEN_V2 "Permalink to this definition")

Maximum length of data sent in each ESPNOW transmission for v2.0

### Type Definitions [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html\#type-definitions "Permalink to this heading")

typedefstruct [esp\_now\_peer\_info](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv417esp_now_peer_info) esp\_now\_peer\_info\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv419esp_now_peer_info_t "Permalink to this definition")

ESPNOW peer information parameters.

typedefstruct [esp\_now\_peer\_num](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv416esp_now_peer_num) esp\_now\_peer\_num\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv418esp_now_peer_num_t "Permalink to this definition")

Number of ESPNOW peers which exist currently.

typedefstruct [esp\_now\_recv\_info](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv417esp_now_recv_info) esp\_now\_recv\_info\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv419esp_now_recv_info_t "Permalink to this definition")

ESPNOW receive packet information.

typedef [wifi\_tx\_info\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_wifi.html#_CPPv414wifi_tx_info_t) esp\_now\_send\_info\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv419esp_now_send_info_t "Permalink to this definition")

ESPNOW sending packet information.

typedef [wifi\_tx\_rate\_config\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_wifi.html#_CPPv421wifi_tx_rate_config_t) esp\_now\_rate\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv421esp_now_rate_config_t "Permalink to this definition")

ESPNOW rate config.

typedefvoid(\*esp\_now\_recv\_cb\_t)(const [esp\_now\_recv\_info\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv419esp_now_recv_info_t)\*esp\_now\_info,constuint8\_t\*data,intdata\_len) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv417esp_now_recv_cb_t "Permalink to this definition")

Callback function of receiving ESPNOW data.

**Attention**

esp\_now\_info is a local variable，it can only be used in the callback.

Param esp\_now\_info:

received ESPNOW packet information

Param data:

received data

Param data\_len:

length of received data

typedefvoid(\*esp\_now\_send\_cb\_t)(const [esp\_now\_send\_info\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv419esp_now_send_info_t)\*tx\_info, [esp\_now\_send\_status\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv421esp_now_send_status_t) status) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv417esp_now_send_cb_t "Permalink to this definition")

Callback function of sending ESPNOW data.

Param tx\_info:

Sending information for ESPNOW data

Param status:

status of sending ESPNOW data (succeed or fail). This is will be removed later, since the tx\_info->tx\_status also works.

### Enumerations [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html\#enumerations "Permalink to this heading")

enumesp\_now\_send\_status\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv421esp_now_send_status_t "Permalink to this definition")

Status of sending ESPNOW data .

_Values:_

enumeratorESP\_NOW\_SEND\_SUCCESS [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv4N21esp_now_send_status_t20ESP_NOW_SEND_SUCCESSE "Permalink to this definition")

Send ESPNOW data successfully

enumeratorESP\_NOW\_SEND\_FAIL [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html#_CPPv4N21esp_now_send_status_t17ESP_NOW_SEND_FAILE "Permalink to this definition")

Send ESPNOW data fail

* * *

**Was this page helpful?**

- Thank you! We received your feedback.
- If you have any comments, fill in [Espressif Documentation Feedback Form](https://www.espressif.com/en/company/documents/documentation_feedback?docId=4287&sections=ESP-NOW%20(api-reference/network/esp_now)&version=esp32c6%20v6.1.1%20(v6.1.1)).

- We value your feedback.
- Let us know how we can improve this page by filling in [Espressif Documentation Feedback Form](https://www.espressif.com/en/company/documents/documentation_feedback?docId=4287&sections=ESP-NOW%20(api-reference/network/esp_now)&version=esp32c6%20v6.1.1%20(v6.1.1)).
