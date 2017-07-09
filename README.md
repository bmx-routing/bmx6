s# BMX7

Bmx7 is a mesh routing protocol for Linux based operating systems.
It is based on bmx6 but provides new features such as:
 * Securely entrusted multi-topology routing (SEMTOR).
 * Link-capacity aware wireless metric for may80211 based radios
 * IPv4 in IPv6 and IPv6 in IPv6 end-to-end tunnels
 * Extended description using compression and re-referenced description content
The following intro provides kind of tutorial to get started.

## Content

*   [Installation](#installation)
    *   [Installing in OpenWRT](#installing-in-openwrt)
*   [Usage (hello mesh)](#usage-hello-mesh)
*   [Concepts](#concepts)
*   [Autoconfiguration](#address-auto-and-manual-configuration)
*   [Unicast Host Network Announcements (UHNA)](#unicast-host-network-announcements-uhna)
*   [Tunnel Announcements](#tunnel-announcements)
*   [Bmx7 Plugins](#bmx7-plugins)
    *   [Config Plugin](#config-plugin)
    *   [Json Plugin](#json-plugin)
    *   [SMS Plugin](#sms-plugin)
    *   [Table plugin](#table-plugin)
    *   [Quagga Plugin](#quagga-plugin)


Note: This document is written using Markdown syntax. Modifications should be
synced via README.md file in bmx7 repositories [bmx6.net][bmx7] and [github.com][github].
Nice syntax examples are [here][syntax].

  [github]: https://github.com/axn/bmx6/tree/bmx7
  [bmx6]: http://bmx6.net
  [syntax]: http://daringfireball.net/projects/markdown/syntax.text


## Installation ##

### Requirements ###

The following tools are needed to obtain, compile, and install bmx7:
* git (debian package: git-core)
* gcc
* make
* libz-dev
* libmbedtls-dev
* libiw-dev
* libjson0-dev (for later config plugin)



The following Linux-kernel modules are needed (depending on used bmx7 features)
* ipv6
* tunnel6
* ip6_tunnel

The polorssl or mbedtls crypto library is needed for cryptographic operations:
Tested with debian and polarssl-1.3.3:
<pre>
wget https://polarssl.org/code/releases/polarssl-1.3.3-gpl.tgz
tar xzvf polarssl-1.3.3-gpl.tgz 
cd polarssl-1.3.3
make
sudo make install
# compile bmx7 with: make EXTRA_CFLAGS="-DCRYPTLIB=POLARSSL_1_3_3"
</pre>

Mbedtls has been tested with debian and mbedtls-2.4.0:
<pre>
wget https://tls.mbed.org/download/mbedtls-2.4.0-gpl.tgz
tar xzvf mbedtls-2.4.0-gpl.tgz
cd mbedtls-2.4.0
make
sudo make install
# compile bmx7 with: make EXTRA_CFLAGS="-DCRYPTLIB=MBEDTLS_2_4_0"
</pre>


### Downloading

Latest development sources are available from bmx6 git repository:

<pre>
git clone https://github.com/axn/bmx6.git
cd bmx6
checkout bmx7
</pre>

### Compile and Install

To only compile the main bmx7 daemon (no plugins):
<pre>
make
sudo make install
</pre>




## Installing in OpenWRT

Bmx7 is currently in the official routing feed of OpenWRT and Lede, so to install it from a existing system you can use opkg:
<pre>
opkg install bmx7 bmx7-uci-config
</pre>

If you are compiling your own OpenWRT, you can add the routing feed (already enabled by default) which can be found here: https://github.com/openwrt-routing/packages

Then run "make menuconfig" and select the bmx7 package in Networking -> Routing and redirection

It is recommended to select also, at least, the uci plugin (bmx7-uci-config)

You can select "luci-app-bmx7" to have a nice web interface for manage and monitorize the routing daemon.

Finally type "make" to build the image.

## Usage (hello mesh)

### Starting

In the most simple configuration, the only required parameter are the interfaces names that should be used for meshing.
The following example starts bmx7 on interface wlan0:
<pre>
root@mlc1001:~# bmx7 dev=eth1
</pre>

However, to let this simple command work as expected also check the following basic requirements:

* bmx7 must be executed in root context (with super user permissions). If you are not already root, prepend all commands with sudo (eg: sudo bmx7 dev=eth1 ).

* NO IP address needs to be configured. By default bmx7 assumes IPv6 and autoconfigures a cryptographic generated ULA IPv6 address (CGA). Just, the interfaces must be UP. The linux ip command can do this for you (eg: ip link set wlan0 up ). Also, if you are using a wireless interface, the wireless interface settings must be set correctly so that link-layer connectivity is given with bmx7 daemons running on other nodes (computers). The good old iwconfig command may help to achieve that. For example: <pre> iwconfig wlan0 mode ad-hoc ap 02:ca:ff:ee:ba:be channel 11 essid my-mesh-network </pre> is a typical configuration for a wireless mesh setup.

* Bmx7 (by default) works in daemon mode, thus sends itself to background and gives back the prompt. To let it run in foreground specify a debug level with the startup command like: <pre> bmx7 debug=0 dev=eth1 </pre>. Of course you may need to kill a previously started bmx7 daemon beforehand  ( killall bmx7 )

If everything went fine bmx7 is running now, searches for neighboring bmx7 daemons via the configured interface (link), and coordinates with them to learn about existence-of and routes-to all other bmx7 nodes in the network.


### Accessing Protocol Events, Status, and Network Information

To access debug and status information of the bmx7 daemon which has just been started, a second bmx7 process can be launched in client mode (with the `--connect` or `-c` parameter) to connect to the main bmx7 daemon and retrieve the desired information.

In the following, a few example will be discussed. Continuous debug levels with different verbosity and scope are accessible with the `--debug` or `-d` parameter.

* Debug level 0 only reports critical events
* Debug level 3 reports relevant changes and
* Debug level 4 reports everything.
* Debug level 12 dump in and outgoing protocol traffic

Eg.: `bmx7 -cd3` connects a bmx7 client process to debug-level 3 of the main daemon and logs the output stdout until terminated with `ctrl-c`
Status, network, and statistic information are accessible with dedicated parameters:

* `status`
* `interfaces`
* `links`
* `originators`
* `descriptions`, plus optional sub-parameters for filtering
* `tunnels`
* `traffic=DEV` where DEV:= all or eth1, ....

<pre>
root@mlc1001:~# bmx7 -c status
STATUS:
shortId  name    nodeKey linkKeys          revision primaryIp                               tun6Address            tun4Address uptime     cpu txQ  rxBpP   txBpP   nbs rts nodes 
A7F0818D mlc1000 RSA2048 RSA896,DH2048M112 dff585f  fd70:a7f0:818d:18c5:f017:395a:8506:accd 2012:3001:0:1100::1/56 1.1.17.1/24 0:04:33:32 0.0 0/50 188/1.2 134/1.2 1   2   3/3  
</pre>

So apart from (git-) revision, the status reveals the daemon's [Global ID (short version)](wiki#global-id), its primary (self-configured) IPv6 address, the time since when it is running (40 minutes), its current cpu consumption (0.1%) and the total number of 4 learned nodes in the network (including itself).

These desired types can be combined. Also the above given example shows kind of shortcut.
The long argument would be:
`bmx7 connect show=status`. A more informative case using the long form would be:

<pre>
root@mlc1001:~# bmx7 connect show=status show=interfaces show=links show=originators show=tunnels
</pre>

Only if relevant information for a requested type is available it will be shown.
In this example no tunnels are configured nor offered by other nodes and therefore no tunnel information is shown.

The loop argument can be prepended to the connect argument to continuously show the requested information.
Many of the long arguments are usable via a short notation, like `l` for `loop`, `c` for `connect`, `s` for `show`, `d` for `debug`.
And there is another shortcut summarizing my current favorite information types via debug level 8
The following commands do the same as above: bmx7 -lc status interfaces links originators tunnels or just `bmx7 -lcd8` .
Description of the provided info:

    interfaces: Followed by one line per configured interface
        dev: Interface name
        state and type: Whether the interface is UP or DOWN and its assumed link-layer type.
        rateMin and rateMax: Min- and maximum transmit rates assumed for this interface.
        llocalIp: IPv6 link-local address (used as source address for all outgoing protocol data).
        globalIp: Autoconfigured address used for sending network traffic via this interface and which is propagated to other nodes.
        multicastIp: Multicast IP (used as destination address for all bmx7 protocol traffic send via this interface).
        primary: Indicates whether the global ip of this interface is used as primary ip for this daemon.
    links: Followed by one line per detected neighboring bmx7 node.
        globalId: GlobalId of that neighbor (see: Wiki ).
        llocalIp: Link-local IP of the neighbor's interface building the other side of the link.
        viaDev: Interface of this node for the link.
        rxRate: Measured receive rate in percent for the link.
        txRate: Measured transmit rate in percent for the link.
        bestTxLink: Indicates whether this link is the best link to a neighboring nodes.
        routes: Indicates for how much routes to other nodes this link is used.
        wantsOgms: Indicates whether the neighboring node has requested (this node) to propagate originator messsages (OGMs) via this link.
        nbLocalId: Neighbors local ID.
    originators: Followed by one line per aware originator in the network (including itself).
        globalId: Global Id of that node (see: Wiki ).
        blocked: Indicates whether this node is currently blocked (see: Wiki ).
        primaryIp: The primary IP of that node.
        routes: Number of potential routes towards this node.
        viaIp: Next hops link-local IP of the best route towards this node.
        viaDev: Outgoing interface of the best route towards this node.
        metric: The end to end path metric to this node
        lastDesc: Seconds since the last description update was received (see: Widi )
        lastRef: Seconds since this node was referenced by any neighboring node (like last sign of life)

Quick summary of provided info:

* Node mlc1001 uses one wired interface (eth1) which is up and actively used for meshing.
* Node mlc1001 got aware of 2 neighbors and 4 nodes (originators) including itself.
* The link qualities (rx and tx rate) to its neighbors are perfect (100%) and actively used (bestTxLink)
* Routes to nodes mlc1000 and mlc1002 are via interface eth1 and directly to the neighbor's link-local address with a metric of 999M (nearly maximum tx/rx rate of the configured interface)
* Route to node mlc1003 is setup via interface eth1 and via the link-local address of neighbor mlc1002 (at least two hops to the destination node).

The following links of the total network topology can be guessed from this information (further links may exist):
<pre>
mlc1000 --- mlc1001 --- mlc1002 - - - mlc1003
</pre>

### Simple Ping Test ###

This could be verified using traceroute6 towards the primary IP of the other nodes.

To mlc1003's primary IP fd66:66:66:0:a2cd:efff:fe10:301 shows two hops:

<pre>
root@mlc1000:~# traceroute6 -n -q1 fd70:92b0:2483:1373:f64e:56b2:d244:be92
traceroute to fd70:92b0:2483:1373:f64e:56b2:d244:be92 (fd70:92b0:2483:1373:f64e:56b2:d244:be92), 30 hops max, 80 byte packets
 1  fd70:c92:d142:b97d:ac33:8990:a84e:e8c5  0.068 ms
 2  fd70:45d6:2d24:2b17:28db:392f:1397:2980  0.052 ms
 3  fd70:92b0:2483:1373:f64e:56b2:d244:be92  0.149 ms
</pre>

### Dynamic Reconfiguration ###

Most bmx7 parameters can be applied not only at startup, but also dynamically to an already running main daemon, using the `--connect` command.
For example interfaces can be added, removed, or specified with more details:
The following example removes interface eth1 and adds eth2 with a max rate of 100 Mbits (overwriting the default assumption of 1000Mbits for ethernet interfaces).

<pre>
bmx7 -c dev=-eth1 dev=eth2 /rateMax=100000
bmx7 -cd8
</pre>

Checking new status of interfaces, links, and originator:

TODO: This must be adapted to bmx7
<pre>
root@mlc1001:~# bmx6 -cd8
status:
version        compatibility codeVersion globalId                     primaryIp                       myLocalId uptime     cpu nodes
BMX6-0.1-alpha 16            9           mlc1001.7A7422752001EC4AC4C8 fd66:66:66:0:a2cd:efff:fe10:102 06100101  0:02:26:00 0.1 4
interfaces:
devName state type     rateMin rateMax llocalIp                    globalIp                           multicastIp primary
eth2    UP    ethernet 100M    100M    fe80::a2cd:efff:fe10:102/64 fd66:66:66:0:a2cd:efff:fe10:102/64 ff02::2     1
links:
globalId                     llocalIp               viaDev rxRate txRate bestTxLink routes wantsOgms nbLocalId
mlc1000.0AE58311046412F248CD fe80::a2cd:efff:fe10:2 eth2   89     88     1          3      1         9B100001
originators:
globalId                     blocked primaryIp                       routes viaIp                  viaDev metric lastDesc lastRef
mlc1000.0AE58311046412F248CD 0       fd66:66:66:0:a2cd:efff:fe10:1   1      fe80::a2cd:efff:fe10:2 eth2   81757K 18       0
mlc1001.7A7422752001EC4AC4C8 0       fd66:66:66:0:a2cd:efff:fe10:102 0      ::                     ---    128G   80       0
mlc1002.91DCF042934B5913BB00 0       fd66:66:66:0:a2cd:efff:fe10:201 1      fe80::a2cd:efff:fe10:2 eth2   83620K 14       4
mlc1003.09E796BC491D386248C3 0       fd66:66:66:0:a2cd:efff:fe10:301 1      fe80::a2cd:efff:fe10:2 eth2   81488K 9        0
</pre>

It can be seen that:

* Interface eth1 has been replaced by eth2 with a lower rate.
* The primary IP of the node has changed (using the autoconfigured IP from eth2.
* The old links (via eth1) are removed and a single new link via eth2 to mlc1000 has been detected
* All routes are now going via eth2 and mlc1000's link-local IP fe80::a2cd:efff:fe10:2




## Concepts ##

### Global ID ###

Each bmx7 node creates during its initialization (first boot) a global ID for itself.
This ID is based on the public/private key that is also created during first boot (if not already there) and is given by the SHA224 hash of the public key.

### Descriptions ###

Instead of propagating individual routing updates for each announced network and interface address, each bmx7 daemon summarizes this and other node specific attributes into a single node-specific description. A specific description is propagated only once to all other nodes. Subsequent routing updates are referencing to the corresponding description with it's hash.
If a node is reconfigured, for example because its interfaces change or a new network shall be announced, than also the node's description changes.
Other nodes are becoming aware of the changed attributes of a reconfigured node by receiving a corresponding description update.
Subsequent references to this node will use the hash of the new description.

Because the description is designed very generic it can be easily used to piggyback other non-routing specific data. For example the bmx7-sms plugin is taking advantage of this option by adding arbitrary short messages data to the node's description.

### Blocked Nodes ###

Nodes may be blocked by other nodes.
When a node is blocked no routing updates (OGMs) of the blocked node are propagated by the blocking node.
The decision for blocking another node is done locally based on the detection of more than one node announcing the same unique resource.



## Address auto and manual configuration ##
TODO: This must be adapted to bmx7



## Unicast Host Network Announcements (UHNA) ###

A Host Network Announcements (HNA) describes the advertisement of IP addresses and networks by a node to other nodes in the mesh.
Typically (but not with BMX7), several nodes can announce the same or overlapping HNAs at the same time.
Announced networks do overlap if they are equal or one being a subset of another (eg. 10.1.1.0/24 is a subset and overlapped by 10.1.0.0/16).
Packets with a destination address matching an announced networks will be routed toward any node that originated a corresponding HNA.
Therefore these HNA types may also be called anycast HNA.

In bmx7, HNAs have an unicast nature (UHNAs) because each network can only be announced once and announced networks MUST NOT overlap (See also Wiki).
This way it can be ensured that the destination of an UHNA routed packet is exactly known.

In a sense the origination and propagation (by intermediate nodes) of UHNA announcements can be thought of a promise that guarantees:
  1. All packets with a destination address matching an announced UHNA network will be routed exactly to the node (with the global ID) that originated the UHNA and
  2. each node on the forwarding path towards the originator of the UHNA is supporting this promise.

By default, Bmx7 only announces primary addresses via UHNAs.
The auto address configuration ensures that interface addresses are unique.

Using UHNAs for the announcements of networks requires a strict coordination to ensure that no network is announced twice.

Technically, multiple UHNAs, each wrapped into a single message, are aggregated into a UHNA frame and attached to the description of a node.

The announcement of UHNAs can be configured with the `--unicastHna` or `-u` parameter followed by a network specification in ip/prefixlen notation.
By default all interface addresses are announced via UHNAs. However, this can be disabled by setting the `--dev` subparameter `/announce` or `/a` to 0.

The following example reconfigures an already running bmx7 daemon (in IPv6 mode) to UHNA announce the network fd00:ffff:ffff:ffff::/64 and fd01:ffff:ffff::/48.
By omitting the `--connect / -c` parameter, the same could be configured as startup parameter for bmx7.

<pre>
bmx7 -c u=fd00:ffff:ffff:ffff::/64 u=fd01:ffff:ffff::/48
</pre>

An already active announcement can be removed by preceeding the network with the `-` char:
<pre>
bmx7 -c u=-fd00:ffff:ffff:ffff::/64
</pre>

Before bmx7 accepts a dynamically configured UHNA announcement it checks if this UHNA is not overlapping with an already existing UHNA announcement form another node.
If this is the case the configuration will fail.
To check if a chain of dynamic commands would be accepted by a bmx6 daemon without actually applying it, the `--test` command may follow the `--connect` command.






## Tunnel Announcements ##
TODO: This must be adapted to bmx7

Tunnel announcements offer an alternative mechanism to propagate routes.
Tunnel announcements are currently only implemented for Bmx6-IPv6 mode.
However, in IPv6 mode IPv6 and IPv4 networks can be announced.
In contrast to UHNAs, using tunnel announcements, the same or overlapping networks can be announced from different nodes. Tunnel announcements are an offer from the originating node to other nodes. Other nodes can take the offer or not. For example several nodes in a network may offer to share their DSL connection by doing a default-route (0.0.0.0/0 or ::/0) tunnel announcement.
Other nodes looking for a route to the internet (a default route) can choose between the multiple offers by establishing a tunnel to one specific of the offering nodes.
Therefore an unidirectional (onw-way) tunnel is established from the searching to the offering node.
At the searching node, the remote (outer) tunnel address is configured with an UHNA address (usually the primary address) of the offering node.
The networks advertised with the tunnel announcements are configured at the client side as routes via (into) the unidirectional tunnel.

This way, each node can make an individual choice between networks offered via tunnel announcements.
The automatic selection can be specified via a policy description that considers parameters such as advertised bandwidth, path metric, trust in specific GW nodes, hysteresis, ... .
Since an UHNA address is used as the outer (remote) tunnel address, the client end of the tunnel can be sure that all packets routed into the tunnel will indeed end up at the intended GW node (see Wiki).

Technically, multiple tunnel announcements, each wrapped into a single tun4/6in6-net message, are aggregated into a tun4/6in6-net frame and attached to the description of a node.

Tunnel announcements are also used for redistributing routes from other routing protocols (see Wiki) into a bmx6 zone.
Therefore, each announcements message is decorated with a route-type field indicating the routing protocol that exported the route for being redistributed.


### Tunnel requirements  ###

The following Linux-kernel modules are needed for tunnel-based overlay networking:
* ipv6
* tunnel6
* ip6_tunnel

### Tunnel Configuration and Debugging ###
In general, a specific tunnel configuration is described from two perspectives:

* Gateway (GW) nodes or just GWs are offering GW services to networks via the advertizement of tunnel announcements and the provisioning of tunnel-end-points.

* GW-client nodes (or just GW-clients) that are searching for GWs with tunnel endpoints and routing services to networks.

A node can (and usually is) operating in both modes (as GW and as GW-client).
But regarding a specific network each node is operating either in GW mode (thus, offering GW-services to that network) or in GW-client mode (thus, searching and using GW-services to that network)!

A quick and simple tunnel configuration example is given here.
Further details and options are described in the next Sections.
The full set of available options for configuring tunnels is given via the build-in --help and --verboseHelp commands

* First make your own tunnel addresses known and reachable for other nodes, eg:
`bmx6 -c tunDev=Default /tun4Address=10.254.10.123/32 /tun6Address=2012:1234:5678:123::1/64`

* Second, configure the automatic establishment of outgoing tunnels to other nodes by searching and selecting any kind of announcement: `bmx6 -c tunOut=v4Default /network=0.0.0.0/0 tunOut=v6Default /network=::/0`

* Optionally, check the currently offered tunnel announcements of other GW nodes and the selected tunnel routes by this node with: `bmx6 -c show=tunnels`

Remark: Since master commit f2fd75072f7dc4738069be6c69625419b9cc7767 the syntax for configuring tunnels has changed.
In the following the new syntax is explained.
For the old syntax please use the build-in `--help` and `--verboseHelp` of the binary you are using

#### Tunnel Device Configuration ####

Operation in GW and/or GW-client mode implies the configuration of a bmx6 tunnel device and the IPv4 and/or IPv6 addresses that shall be used for tunnel traffic.
The selection of these addresses should be coordinated with:
* the mesh community because conflicting tunnel address usage will cause problems for the conflicting nodes
* GW administrators because (depending on the GW connection to other networks) only specific addresses may be routable and considered to be originated from the bmx6 cloud.

The command
<pre>
bmx6 -c tunDev=Default /tun4Address=10.254.10.123/32 /tun6Address=2012:1234:5678:123::1/64
</pre>
dynamically
* configures a linux ip4/6in6 tunnel device called bmx6Default (check it with command: ip link show).
* assignes the address `10.254.10.123` and `2012:1234:5678:123::1` to the tunnel interface and uses them for outgoing tunnel traffic.
* enables GW-mode for the specified networks: Makes a tunnel announcement so that other nodes can select it for tunneling packets to this node.

Now other nodes can send tunneled packets to this node via the unidirectional tunnel end point offered by this node.

But for bidirectional tunnel communication with any another node also a backwards tunnel is needed (an unidirectional tunnel from this node to the other node).

The automatic selection and establishemt of tunnels to other nodes is achieved with the GW-client mode as described in more derail in the next Section.

#### Gateway-Client Nodes ####

The configuration of GW clients can be simple but also, depending on the preferences for a GW-selection policy, very complex.
Through the configuration of the mandatory tunDev and it's addresses (see above), each GW client node is also a GW node to its own (usually small) tunnel address space.

In the following simple example a GW-client node is searching for any other kind of offered IPv4 and v6 tunnels:
<pre>
bmx6 -c tunOut=v4Default /network=0.0.0.0/0 tunOut=v6Default /network=::/0
</pre>

With the above configured tunnel selection policy, tunnels are selected in the following order:
  1. prefix-length of announced tunnels (networks that are more specific than others).
  2. the resulting tunnelMetric (combination of the advertised bandwidth, path metric in the bmx6 cloud, and locally specified prefereces like hysteresis or bonus)

The disadvantage of this simple config is that other nodes can easily redirect your tunnel selections
to specific networks by announcing more precise tunnel networks (larger prefix length).
To prevent this, the selection policy can be split into several and more precise search directives.

Imagine the following address assignment policy for IPv4 tunnel addresses in a mesh cloud (the general
idea can be straight translated to IPv6).

* Nodes in the mesh cloud announce their private and local address ranges with a prefix length of 24 and somewhere in the range of 10.254.0.0/16.

* Announcements of this type should always be preferred, even if any of the following announced types has a better end-to-end metric or more precise announcement.
<pre>
    bmx6 -c tunOut=v4Nodes /network=10.254.0.0/16 /minPrefixLen=24 /maxPrefixLen=24 /ipmetric=2001
</pre>

* Some BGP GW nodes are connected to other mesh clouds/areas of the same overall community network. These clouds are operating in a different IPv4 range (than 10.254.0.0/16) but always somewhere in the range of 10.0.0.0/8. Route announcements of this type should be preferred over the announcement of a default route.
<pre>
    bmx6 -c tunOut=v4Clouds /network=10.0.0.0/8 /maxPrefixLen=16 /bgp=1
</pre>

* Some DSL GW nodes are offering to share their DSL line and are announcing a default route (0.0.0.0/0).

* Further, to mitigate the effects of GW switching between GWs having a similar end-to-end metric a GW switch should only happen if the other GW is at least 30% better.
<pre>
    bmx6 -c tunOut=v4Default /network=0.0.0.0/0 /maxPrefixLen=0 /hysteresis=30 # refine the above configured v4 tunnel search
</pre>

* In case my node is directly connected to a DSL gateway and gets a automatically (dhcp) configured default route in the main routing table (use: ip route show table main ). then this route should be preferred and should NOT clash with default tunnel routes configured by bmx6.
* Therefore move all bmx6 tunnel routes to 0.0.0.0/0 into a separate routing table with lower lookup prioriy (check with: ip rule show; ip route show table 150)
<pre>
    bmx6 -c tunOut=v4Default /network=0.0.0.0/0 /maxPrefixLen=0 /hysteresis=30 /tableRule=50000/150 # again refine the above default search
</pre>

* The default route announcements from two well known GWs (with hostname pepe and paula) should be strictly preferred over unknown GWs.
* So, if available, move them to new table (with lower priority than main and higher priority than used for the backup tunnel rule configured above)
<pre>
    bmx6 -c tunOut=v4DefaultPepe  /network=0.0.0.0/0 /maxPrefixLen=0 /gwName=pepe  /hysteresis=30 /tableRule=40000/140
    bmx6 -c tunOut=v4DefaultPaula /network=0.0.0.0/0 /maxPrefixLen=0 /gwName=paula /hysteresis=30 /tableRule=40000/140
</pre>

* Finally, GW Paula turned out to be more stable. Therefore I want to prefer GW Paula over Pepe:
<pre>
    bmx6 -c tunOut=v4DefaultPaula /network=0.0.0.0/0 /maxPrefixLen=0 /gwName=paula /hysteresis=30 /bonus=100
</pre>

#### Gateway Nodes ####

The advertisement of a tunnel endpoint to a network can be configured with the --tunIn=<arbitrary name> and /network=<network> argument and an optional bandwidth specification (given as bits per second) using the /bandwidth or /b sub parameter.
Announcement can be removed by preceeding the name argument with a '-' char.
The following command dynamically configures the advertisement of the following routes:

* An IPv4 default route 0.0.0.0/0 with a bandwidth of 32 Mbps.
* A more specific route to 10.10.0.0/16 with a bandwidth of 10 Mbps (eg: a local v4 Network).
* An IPv6 route to the [RFC 4291] designated `2000::/3` global unicast address space with a bandwidth of 16 Mbps.
* A more specific route to the `2012:1234::/32` IPv6 space at 10 Mbps (eg: a local v6 Network).
<pre>
bmx6 -c tunIn=def4Offer /n=0.0.0.0/0 /b=32000000  tunIn=local4 /n=10.10.0.0/16 /b=10000000  tunIn=def6Offer /n=2000::/3 /b=16000000  tunIn=local6 /n=2012:1234::/32 /b=10000000
</pre>

#### Tunnel Status Information ####

Tunnel status information can be accessed with the `--tunnels or --show=tunnels` parameters.

## Bmx7 Plugins ##

### Compile and Install ###

To compile and install bmx7 daemon and all its plugins simply do:
<pre>
make build_all
sudo make install_all
</pre>

However. specific requirements may need to be fulfilled for some plugins in order to compile correctly.
These requirements are described in the corresponding plugin section.

### Config Plugin ###

#### Requirements ####

uci libs are needed for the bmx7-config plugin.
To install try (old version):
<pre>
wget http://downloads.openwrt.org/sources/uci-0.7.5.tar.gz
tar xzvf uci-0.7.5.tar.gz
cd uci-0.7.5
make
sudo make install
</pre>
or check: http://www.wakoond.hu/2013/06/using-uci-on-ubuntu.html


Depending on your system there happens to be an error during compilation.
Then edit cli.c and change line 465 to: char *argv[MAX_ARGS+2];

#### Compile and Install ####
<pre>
make -C lib/bmx7_uci_config/
sudo make -C lib/bmx7_uci_config/ install
</pre>

#### Usage ####

### Json Plugin ###

#### Requirements ####

json-c for bmx7_json plugin (debian package: libjson0 libjson0-dev)

json-c developer libs are needed!
For further reading check: http://json.org/ or https://github.com/jehiah/json-c

Note for debian sid:
The debian package libjson0-dev 0.10-1 seems to miss the file /usr/include/json/json_object_iterator.h
Manually copying it from the below mentioned json-c_0.10.orig.tar.gz archive helps.

To install manually (only if NOT installed via debian or other package management system):
<pre>
wget http://ftp.de.debian.org/debian/pool/main/j/json-c/json-c_0.10.orig.tar.gz
tar xzvf json-c_0.10.orig.tar.gz
cd json-c..
./configure ; make ; make install; ldconfig
</pre>

#### Compile and Install ####

To compile and install only the bmx7 json plugins:
<pre>
make -C lib/bmx7_json/
sudo make -C lib/bmx7_json/ install
</pre>

#### Usage ####

### SMS Plugin ###

This plug-in uses routing packets to transmit any information from one node to the
whole network. The good point is that propagation works even if there is no continuous data-
path. Even though the WiFi network is under bad conditions (because the Wireless noise,
distance between nodes, etc...), the data will be propagated. However in the current implemen-
tation, there exist a maximum size limit of 240 Bytes for each file.

The API of the sms plug-in is very simple. It simply clones the content of one or more files
given by one node to all other nodes. All other nodes can do the same. Once started, each
node will have two directories:/var/run/bmx7/sms/rcvdSms and /var/run/bmx7/sms/sendSms. Files
put into the sendSms folder will be cloned to all other nodes inside rcvdSms folder.
QMP is using this feature for several things. The positioning Map information is transmitted
using it. There is a chat in web interface which uses it too. And in the future we are planning
to use it for more purposes like statistics, captive portal, MAC filter rules, etc...

### Table plugin ###

This plug-in can be used to automatically announce routes from specific routing tables.
For example to dynamically announce (redistribute) routes from another routing protocol.

#### Usage ####

To use the bmx7 table plugin it must be loaded during bmx7 daemon startup with the plugin=bmx7_table.so argument.
Alternatively a plugin section can be defined in the bmx7 config file like this:
<pre>
config 'plugin'
        option 'plugin' 'bmx7_table.so'
</pre>

Once the plugin is successfully loaded, the new parameters for redistributing routes from specific tables are enabled.

A full documentation of the table-related parameters is available via the --help and --verboseHelp /r=1 option.

#### Configuring route redistribution ####

Redistribution of routes is configurable with the `--redistTable` parameter.
Similar to the `--tunIn parameter, --redistTable` must be given with an arbitrary name for referencing to a specific redistribution directive and further sub-criterias.

Mandatary sub-criterias are /table and at least one route of the available types ( /kernel, /boot, /static )
Typical further but optional sub parameters are: /network, /minPrefixLen, /maxPrefixLen, /aggregatePrefixLen, /bandwidth
The following example automatically and dynamically announces (and removes announcements of) routes as they appear/disappear in routing table 100 and that match the criterias of:
* being a sub network of 192.168.0.0/16
* have a prefix length >= 24
* are configured as type kernel or boot
  (manually configured routes via the ip command will appear by default as type boot,
  eg: `ip r add 192.168.254.2/31 via 10.0.0.1 table 100`

If routes matching these criterias exist, then:
* They are announced with a bandwidth of 1Mbit
* Subsequent routes are aggregated down to a minimum prefix length of 24
<pre>
 bmx7 -c \
 redistTable            otherProtocol        \
    /network            192.168.0.0/16       \
    /table              100                  \
    /aggregatePrefixLen 24                   \
    /minPrefixLen       24                   \
    /kernel             1                    \
    /boot               1                    \
    /bandwidth          1000000
</pre>

### Quagga Plugin ###

The bmx7 quagga plugin is currently broken.

