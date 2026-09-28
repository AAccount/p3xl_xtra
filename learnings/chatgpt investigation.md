# Original Observations

The `Location` > `Use assisted GPS` is always disabled on the Pixel 3XL with LineageOS 22. 
Turning it on, turns it off immediately. Nothing is able to make it stick. 
Manually setting the property. Manually enabling `persist.sys.xtra-daemon.enabled` doesn't seem to do anything.
Below are the only log messages that show up when trying to set agps. 
The log messages are always the same whether you set the property, turn on the toggle, or try anything else. 
```text
09-03 18:57:12.952  1910  2069 I GnssLocationProvider: Toggling xtra-daemon via property 
09-03 18:57:12.972  1910  2069 I GnssLocationProvider: Toggling xtra-daemon via property 
09-03 18:57:12.974     0     0 I         : c2      1 init: processing action (persist.sys.xtra-daemon.enabled=*) from (/vendor/etc/init/hw/init.sdm845.rc:667) 
09-03 18:57:12.975     0     0 I         : c2      1 init: Sending signal 9 to service 'loc_launcher' (pid 5820) process group... 
09-03 18:57:12.981     0     0 I         : c2      1 libprocessgroup: Removed cgroup /sys/fs/cgroup/uid_0/pid_5820 
09-03 18:57:12.982     0     0 I         : c1      1 init: Service 'loc_launcher' (pid 5820) received signal 9 
09-03 18:57:12.982     0     0 I         : c1      1 init: Sending signal 9 to service 'loc_launcher' (pid 5820) process group... 
09-03 18:57:12.983     0     0 E         : c1      1 libprocessgroup: Failed to open /sys/fs/cgroup/uid_0/pid_5820/cgroup.procs: No such file or directory 
09-03 18:57:12.983     0     0 I         : c3      1 init: Untracked pid 5823 received signal 9 
09-03 18:57:12.983     0     0 I         : c3      1 init: Untracked pid 5823 did not have an associated service entry and will not be reaped 
09-03 18:57:12.984     0     0 I         : c3      1 init: processing action (persist.sys.xtra-daemon.enabled=*) from (/vendor/etc/init/hw/init.sdm845.rc:667) 
09-03 18:57:12.984     0     0 I         : c3      1 init: starting service 'loc_launcher'... 
09-03 18:57:12.986  1299  5936 E QCALOG  : [MessageQ_Client] run failed 103 
09-03 18:57:12.991     0     0 I         : c0      1 init: ... started service 'loc_launcher' has pid 8790 
09-03 18:57:13.030  8790  8791 D QCALOG  : [location-mq] SRV Socket-Opend 
09-03 18:57:13.033  8790  8790 E LocSvc_utils_cfg: isXtraDaemonEnabled:90] xtra-daemon enabled: 0 
09-03 18:57:13.033  8790  8790 E LocSvc_utils_cfg: loc_read_process_conf:808]: Process xtra-daemon is disabled via property 
09-03 18:57:13.061  8793  8793 I LOWI-8.6.0.26: [MessageQ_Client] connecting to server [/dev/socket/location/mq/location-mq-s] 
09-03 18:57:13.061  8793  8793 I LOWI-8.6.0.26: [MessageQ_Client] connected 
```
You can see it kill the existing `loc_launcher` then start it again. 
Something happens and it disables `persist.sys.xtra-daemon.enabled` by itself.
The main idea was to see if it is possible to "hand start" the injection process. 
Could agps data be downloaded by normal means such as `curl` or `wget` and the data sent to the gps hardware?

I have never done any kind of binary blob reverse engineering. 
The most I've done is reverse engineer obufscated javascript from a sketchy manga website to rip it for offline viewing.
That was successfully achieved over a long weekend of browser breakpoints, renaming a local copy of the js, and writing a minimal same implementation.

# ChatGPT Investigation

(#) here means step number

## (1) Initial check
Look inside `xtra-deamon` for any interesting strings to chase
```
strings -a xtra-daemon | grep -iE "inject|xtra|location-mq|server|socket"
...
onRequestXtraData
handleXtraDataRequest
doDownloadXtraData
onXtraDownloadResult
XTRA download success. inject data into modem.
call mIzatApi->injectXtraData(len=%lu)
injectXtraData success
injectXtraData fail
...
```
There are success and fail log strings (last 2). Other function names suggest a download and inject lifecycle.

The following info is not the 1:1 chat conversation. 
The majority of the 1st half was done purely by command line and would be impractical to do for a person.
This is a reread of the investigation and following along using Ghidra trying to understand what was happening.

## `onRequestXtraData` 

### (2) Check symbol table of any hints this binary implements any interesting functions
For basic surveying, the command line tools are still useful.
Instead of wasting time loading Ghidra for every possible suspect, do a quick 1st pass by command line to see which ones are worth loading.
```
readelf --symbols --wide xtra-daemon | grep -E "onRequestXtraData|injectXtraData|mIzatApi"
   237: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND _ZN9izat_core15IzatAdapterBase17onRequestXtraDataEv
_ZN9izat_core15IzatAdapterBase17onRequestXtraDataEv = izat_core::IzatAdapterBase::onRequestXtraData()
```
This funciton is not implemented in xtra-daemon as it is marked `UND` (undefined)

### (3) Look for anyone else who implements `_ZN9izat_core15IzatAdapterBase17onRequestXtraDataEv`
It was ripped from `/vendor/bin/xtra-daemon`. It is a 64bit executable. Check `/vendor/lib64`. (Ran on the cell phone.)
Android version can't do full --symbols and doesn't have an equivalent --wide option. Have to use single letter abbreviations.

```
for f in /vendor/lib64/*.so; do readelf -s "$f" 2>/dev/null | grep -q "_ZN9izat_core15IzatAdapterBase17onRequestXtraDataEv" && echo "$f"; done
/vendor/lib64/libflp.so
/vendor/lib64/libgeofence.so
/vendor/lib64/libizat_core.so

```

3 possible candidates. Check the raw grep output instead of just printing the file name of interest
```
for f in /vendor/lib64/*.so; do readelf -s "$f" 2>/dev/null | grep -q "_ZN9izat_core15IzatAdapterBase17onRequestXtraDataEv"; done
=== /vendor/lib64/libflp.so ===
    13: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND _ZN9izat_core15IzatAdapterBase17onRequestXtraDataEv
=== /vendor/lib64/libgeofence.so ===
     8: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND _ZN9izat_core15IzatAdapterBase17onRequestXtraDataEv
=== /vendor/lib64/libizat_core.so ===
   228: 0000000000015694    76 FUNC    GLOBAL DEFAULT   14 _ZN9izat_core15IzatAdapterBase17onRequestXtraDataEv
```
It's `libizat_core.so` that appears to implement the function

### (4) Inspect `libizat_core`'s implementation
Now use Ghidra to specifically look for IzatAdapterBase::onRequestXtraData
The decompiled C slop is actually immediatley useful. This function is probalby intended to be overriden. It does nothing here.
```C++
undefined8 izat_core::IzatAdapterBase::onRequestXtraData(void)
{
  if ((_loc_logger & 0xfffffffffffffffe) == 4) 
  {
    __android_log_print(3,"LocSvc_IzatAdapterBase","%s: default implementation invoked", "onRequestXtraData");
  }
  return 0;
}
```

### (5) Look for other implementation of `onRequestXtraData` from the overriding class
`llvm-nm --demangle libizat_core.so | grep -E "IzatAdapterBase|onRequestXtraData"`

Got nothing.

Side note: Ghidra's symbol explorer appears to be more than just the elf symbol tables.
It appears to add extra entries based on analysis).
The command above only shows the one implmentation above.

`readelf --dyn-syms -C libizat_core.so | grep -E "IzatAdapterBase|onRequestXtraData" `

The dynamic symbols have to be exported or imported. Perhaps there is a clue to an imported "IzatAdapterReal"::onRequestXtraData.
Dynamic symbols cuts down on unwanted matches.
Still got nothing.

### (6) See who calls `onRequestXtraData`
Ask Ghidra to tell you.

In the left column area: Symbol tree > find `onRequestXtraData` > right click > References > Show references to address
```
Entry Point	??	EXTERNAL
001103a0		fde_table_entry 	INDIRECTION
001113c8		ddw izat_core::IzatAdapterBase::onRequestXtraData	DATA
0012d0b8	PTR_onRequestXtraData_0012d0b8	addr izat_core::IzatAdapterBase::onRequestXtraData	DATA
```
Nothing. No actual jumps.
ChatGPT gives up looking for `onRequestXtraData` and backtracks to a different search.

## The string "XTRA download success. inject data into modem"
### (7) Who actually says "XTRA download success"
See step 1 where this string was flagged by ChatGPT as interesting.

Ghidra: window > defined strings > type in `XTRA download success`... > right click > references > show references to address

Takes you to: `izat_xtra::XtraNativeClient::onXtraDownloadResultInternal`. The Ghidra C slop is actually helpful again
```C++
if ((_loc_logger & 0xfffffffffffffffe) == 4) 
{
  __android_log_print(3,0,
                      "%s:%d] XTRA download success. inject data into modem. bootupDone[%d] ",
                      "onXtraDownloadResultInternal",0xdf,cVar3);
}
XtraIzatAdapter::onInjectXtraData((XtraIzatAdapter *)(*(long *)(this + 8) + 0xa8),param_3);
```
Following the `XtraIzatAdapter::onInjectXtraData` leads to an undefined thunk by this wild goose chase: 
`XtraIzatAdapter::onInjectXtraData` -> `izat_xtra::XtraClient::sendMsg` -> `MsgTask::sendMsg` -> ?? (thunk undefined `sendMsg(LocMsg * param_1)`).

Ghidra looses the trail. The MsgTask class is not defined here. In the original ChatGPT command line only investigation, it takes ChatGPT quite a while longer to get to the `-> ??` after `MsgTask::sendMsg`

### (8) Look for other injection related strings
```
strings --radix=x xtra-daemon | grep -iE 'Xtra|inject|modem|LocMsg|sendMsg'
...
8c80 %s:%d] call mIzatApi->injectXtraData(len=%lu)
8a46 %s:%d] injectXtraData success
8fd3 %s:%d] injectXtraData fail. reason: invalid arguments.
...
```

### (9) Hunt `mIzatApi->injectXtraData`
Ghidra: Windows > Defined Strings > "mIzatApi->injectXtraData" > right click on the only location > references > Show References to Address > click on the only location.

End up in: `std::__function::__func<>::operator(__func<> *this)` offset 00118344
```C++
lVar10 = *(long *)(this + 8);
if ((_loc_logger & 0xfffffffffffffffe) == 4) 
{
  uVar2 = (ulong)((byte)this[0x10] >> 1);
  if (((byte)this[0x10] & 1) != 0) 
  {
    uVar2 = *(ulong *)(this + 0x18);
  }
  __android_log_print(3,"LocSvc_xtra2","%s:%d] call mIzatApi->injectXtraData(len=%lu)", "operator()",0x5b,uVar2);
}
plVar6 = *(long **)(lVar10 + 0x10);
bVar4 = ((byte)this[0x10] & 1) != 0;
p_Var3 = this + 0x11;
if (bVar4) 
{
  p_Var3 = *(__func<> **)(this + 0x20);
}
uVar1 = (uint)((byte)this[0x10] >> 1);
if (bVar4) 
{
  uVar1 = *(uint *)(this + 0x18);
}
iVar5 = (**(code **)(*plVar6 + 0x158))(plVar6,p_Var3,uVar1);
```
The `(code**)` shows a function from plVar6 which itself came from some offset to the input parameter "this".
ChatGPT concludes:
- `p_Var3` = xtra data pointer
- `uVar1` = xtra data file size

## Backtrack on what `xtra-daemon` even does with `libizat_core` that is "Izat" or "Xtra" related
### (10) Back track results
```
readelf --symbols --wide --demangle xtra-daemon | grep -E 'vtable|VTT|Izat|Xtra'
    16: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND izat_core::IzatAdapterBase::~IzatAdapterBase()
    99: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND izat_core::IzatAdapterBase::setUserPreference(bool)
   100: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND izat_core::IzatAdapterBase::geofenceBreachEvent(unsigned long, unsigned int*, Location&, GeofenceBreachType, unsigned long)
   101: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND izat_core::IzatAdapterBase::geofenceStatusEvent(GeofenceStatusAvailable)
   102: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND izat_core::IzatAdapterBase::onGdtUploadEndEvent(int, int, int)
   103: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND izat_core::IzatAdapterBase::onSendCsmConfigResp(int, bool, bool, unsigned char, unsigned char, unsigned short, unsigned char, unsigned int, unsigned char const*)
   104: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND izat_core::IzatAdapterBase::reportLocationsEvent(FlpExtLocation_s const*, unsigned long, BatchingMode)
   105: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND izat_core::IzatAdapterBase::handleEngineDownEvent()
   106: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND izat_core::IzatAdapterBase::handleGtpApStatusResp(int, unsigned short, unsigned char, unsigned char, unsigned char)
   107: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND izat_core::IzatAdapterBase::onGdtDownloadEndEvent(int, unsigned int, int)
   108: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND izat_core::IzatAdapterBase::onGdtReceiveDoneEvent(int, unsigned int, int)
   109: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND izat_core::IzatAdapterBase::onGdtUploadBeginEvent(int, int, char const*, unsigned int)
   110: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND izat_core::IzatAdapterBase::updateGfBreachLocation(FlpExtLocation_s&)
   111: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND izat_core::IzatAdapterBase::onGdtDownloadBeginEvent(int, unsigned int, unsigned int, unsigned char const*, unsigned int, unsigned char const*, unsigned int, char const*, unsigned int, unsigned int, signed char, unsigned int, signed char, unsigned int, signed char)
   112: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND izat_core::IzatAdapterBase::reportCompletedTripsEvent(unsigned int)
   113: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND izat_core::IzatAdapterBase::reportBatchStatusChangeEvent(BatchingStatus)
   114: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND izat_core::IzatAdapterBase::IzatAdapterBase(unsigned long, loc_core::ContextBase*)
   137: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND getIzatPcid
   175: 0000000000000000     0 OBJECT  GLOBAL DEFAULT  UND vtable for LocThread
   236: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND izat_core::IzatAdapterBase::onRequestNtpTime()
   237: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND izat_core::IzatAdapterBase::onRequestXtraData()
   238: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND izat_core::IzatAdapterBase::onReceiveXtraConfig(unsigned int, unsigned int, unsigned int, unsigned short, unsigned long, unsigned char, unsigned short, char const*, char const*, char const*, char const*, char const*, char const*)
   239: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND izat_core::IzatAdapterBase::reportPositionEvent(UlpLocation&, GpsLocationExtended&, loc_sess_status, unsigned int)
   240: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND izat_core::IzatAdapterBase::onReceiveXtraServers(char const*, char const*, char const*)
```

What does `libizat_core` do that is xtra related
```
readelf --symbols --wide --demangle libizat_core.so | grep -E 'Xtra' 
   111: 0000000000017874    76 FUNC    GLOBAL DEFAULT   14 izat_core::IzatApiBase::requestXtraConfigInfo(unsigned int, unsigned int)
   210: 000000000001ed8c   904 FUNC    GLOBAL DEFAULT   14 izat_core::IzatApiV02::injectXtraData(char const*, unsigned int)
   223: 000000000001f35c   480 FUNC    GLOBAL DEFAULT   14 izat_core::IzatApiV02::requestXtraServers()
   228: 0000000000015694    76 FUNC    GLOBAL DEFAULT   14 izat_core::IzatAdapterBase::onRequestXtraData()
   265: 00000000000176ac    76 FUNC    GLOBAL DEFAULT   14 izat_core::IzatApiBase::setXtraVersionCheck(izat_core::XtraVersionCheck)
   281: 000000000001e5b8   588 FUNC    GLOBAL DEFAULT   14 izat_core::IzatApiV02::setXtraVersionCheck(izat_core::XtraVersionCheck)
   295: 0000000000017744    76 FUNC    GLOBAL DEFAULT   14 izat_core::IzatApiBase::injectXtraData_legacy(char const*, unsigned int)
   353: 0000000000017790    76 FUNC    GLOBAL DEFAULT   14 izat_core::IzatApiBase::injectXtraData(char const*, unsigned int)
   357: 0000000000017c74   172 FUNC    GLOBAL DEFAULT   14 izat_core::IzatApiBase::handleReceiveXtraConfigInfo(unsigned int, unsigned int, unsigned int, unsigned short, unsigned long, unsigned char, unsigned short, char const*, char const*, char const*, char const*, char const*, char const*)
   396: 00000000000178c0   404 FUNC    GLOBAL DEFAULT   14 izat_core::IzatApiBase::handleReceiveXtraServers(char const*, char const*, char const*)
   413: 000000000002f480     1 OBJECT  GLOBAL DEFAULT   23 izat_core::IzatApiBase::mIsXtraInitialized
   432: 0000000000017a54   272 FUNC    GLOBAL DEFAULT   14 izat_core::IzatApiBase::handleRequestXtraData()
   460: 0000000000017828    76 FUNC    GLOBAL DEFAULT   14 izat_core::IzatApiBase::requestXtraServers()
   500: 000000000001ea08   900 FUNC    GLOBAL DEFAULT   14 izat_core::IzatApiV02::injectXtraData_legacy(char const*, unsigned int)
   522: 000000000001572c    76 FUNC    GLOBAL DEFAULT   14 izat_core::IzatAdapterBase::onReceiveXtraConfig(unsigned int, unsigned int, unsigned int, unsigned short, unsigned long, unsigned char, unsigned short, char const*, char const*, char const*, char const*, char const*, char const*)
   533: 0000000000015648    76 FUNC    GLOBAL DEFAULT   14 izat_core::IzatAdapterBase::onReceiveXtraServers(char const*, char const*, char const*)
   545: 000000000001f53c   940 FUNC    GLOBAL DEFAULT   14 izat_core::IzatApiV02::requestXtraConfigInfo(unsigned int, unsigned int)
```

In fact, you might want to reverse grep "Base" as these are likely useless "abstract class" like objects or "java interface" like.

```
readelf --symbols --wide --demangle libizat_core.so | grep -E 'Xtra' | grep -v -i "base"
   210: 000000000001ed8c   904 FUNC    GLOBAL DEFAULT   14 izat_core::IzatApiV02::injectXtraData(char const*, unsigned int)
   223: 000000000001f35c   480 FUNC    GLOBAL DEFAULT   14 izat_core::IzatApiV02::requestXtraServers()
   281: 000000000001e5b8   588 FUNC    GLOBAL DEFAULT   14 izat_core::IzatApiV02::setXtraVersionCheck(izat_core::XtraVersionCheck)
   500: 000000000001ea08   900 FUNC    GLOBAL DEFAULT   14 izat_core::IzatApiV02::injectXtraData_legacy(char const*, unsigned int)
   545: 000000000001f53c   940 FUNC    GLOBAL DEFAULT   14 izat_core::IzatApiV02::requestXtraConfigInfo(unsigned int, unsigned int
```

### (11) Investigate `IzatApiV02::injectXtraData`
Ghidra C slop has a very suspicious looking call: `LocApiV02::locSyncSendReq`. 
Of special interest is the mysterious `0xa7` parameter. ChatGPT thinks this is some request code.

### (12) `LocApiV02::locSyncSendReq`
Look for any function named `locSyncSendReq`. Doesn't have to be from the `LocApiV02` class.
```
readelf --symbols --wide --demangle libizat_core.so | grep "locSyncSendReq"
12: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND LocApiV02::locSyncSendReq(unsigned int, locClientReqUnionType, unsigned int, unsigned int, void*)
186: 00000000000195b0    12 FUNC    GLOBAL DEFAULT   14 izat_core::IzatApiV02::locSyncSendReq(unsigned int, locClientReqUnionType, unsigned int, unsigned int, void*)
```

The 2 byte length of the IzatApiV02 version is just a wrapper for the actual, undefined one. 
Ghidra C slop confirms.
```C++
void izat_core::IzatApiV02::locSyncSendReq(long param_1)
{
    LocApiV02::locSyncSendReq(*(undefined8 *)(*(long *)(param_1 + 0x58) + 8));
    return;
}
```

Is it calling it with `param_1->inner_struct->inner_something`?

## `locSyncSendReq`
### (13) Search the Pixel 3XL for where the real locSyncSendReq is defined and what it does
```
for f in /vendor/lib64/*.so /vendor/lib64/hw/*.so; do readelf -Ws "$f" 2>/dev/null | grep -q 'LocApiV02.*locSyncSendReq' && echo "$f"; done

/vendor/lib64/libizat_core.so 
/vendor/lib64/liblbs_core.so 
/vendor/lib64/libloc_api_v02.so
```
`libloc_api_v02` is the most suspicious name
```
readelf --symbols --wide --demangle libloc_api_v02.so | grep 'LocApiV02::locSyncSendReq'
100: 00000000000111fc   484 FUNC    GLOBAL DEFAULT   14 LocApiV02::locSyncSendReq(unsigned int, locClientReqUnionType, unsigned int, unsigned int, void*)

```
According to the Ghidra C slop, the "real" locSyncSendReq doesn't do anything with `0xa7`.
The mysterious 0xa7 is just passed along untouched to `loc_sync_send_req`.

### (14) What is `loc_sync_send_req` doing
All the ghidra C slop shows this being some kind of submit and wait. 
The slop has a lot of mutex finagling and log messages talking about waiting.
Even more interesting, the 0xa7 is not messed with at all. 
This goes straight to the modem's gps. This is an important number to remember.

### (15) Attempt to assign meaning to 0xa7
Look for any string related to xtra or inject
```
strings --radix=x libloc_api_v02.so | grep -i -E 'xtra|inject'
1dfe _ZN8loc_core10LocApiBase15requestXtraDataEv
1e67 _ZN8loc_core10LocApiBase16reportXtraServerEPKcS2_S2_i
2275 _ZN8loc_core14LocDualContext19injectFeatureConfigEPNS_11ContextBaseE
23ab _ZN9LocApiV0211setXtraDataEPci
24dd _ZN9LocApiV0214injectPositionERK8Location
2507 _ZN9LocApiV0214injectPositionEddf
290a _ZN9LocApiV0217requestXtraServerEv
2aad _ZN9LocApiV0219reportXtraServerUrlEPK46qmiLocEventInjectPredictedOrbitsReqIndMsgT_v02
2b56 _ZN9LocApiV0219setXtraVersionCheckEj
495a %s:%d]: error! status = %s, inject_pos_ind.status = %s
4ac9 QMI_LOC_INJECT_UTC_TIME_RESP_V02
4b26 QMI_LOC_SET_XTRA_T_SESSION_CONTROL_RESP_V02
4c04 QMI_LOC_INJECT_GTP_CLIENT_DOWNLOADED_DATA_REQ_V02
4c36 QMI_LOC_INJECT_XTRA_PCID_REQ_V02
4c7e QMI_LOC_INJECT_SRN_AP_DATA_RESP_V02
4e91 QMI_LOC_QUERY_XTRA_INFO_IND_V02
... among many more
```
Demangled `LocApiV02::setXtraData` looks interesting.

### (16) `LocApiV02::setXtraData`
Use the Ghidra symbol tree and its C slop.
The line of interest: `locSyncSendReq(this,0x35,&local_488,1000,0x35,&local_490);` 
However it is a different request type. 0x35, not 0xa7.

Who calls `locSyncSendReq`?
Use Ghidra's symbol table right clik > References > Show References To address

Nothing interesting.

## IzatApiV02
### (17) Look for the constructor
In Ghidra just search IzaApiV02, You will see the class has a function with the same name as the class.

C slop confirms Ghidra's conclusion that it doesn't do anything. It just zeros out memory then calls something else.
```C++
IzatApiBase::IzatApiBase((IzatApiBase *)this,(LocApiProxyBase *)param_1);
//^^^ right away calls the base
```

Using Ghidra's click and follow, the base constructor does:
```C++
void __thiscall izat_core::IzatApiBase::IzatApiBase(IzatApiBase *this,LocApiProxyBase *param_1)

{
  *(LocApiProxyBase **)(this + 0x58) = param_1;
  *(undefined8 *)(this + 0x10) = 0;
  *(undefined8 *)(this + 8) = 0;
  *(undefined8 *)(this + 0x20) = 0;
  *(undefined8 *)(this + 0x18) = 0;
  *(undefined8 *)(this + 0x30) = 0;
  *(undefined8 *)(this + 0x28) = 0;
  *(undefined8 *)(this + 0x40) = 0;
  *(undefined8 *)(this + 0x38) = 0;
  *(undefined8 *)(this + 0x50) = 0;
  *(undefined8 *)(this + 0x48) = 0;
```

ChatGPT takes an educated guess that `LocApiProxyBase` must have a `LocApiProxyV02` like "Izat v2" and proposes.
```
IzatApiV02
   ↓
IzatApiBase + 0x58
   ↓
LocApiProxyV02 *
   ↓
LibLocApiV02::locSyncSendReq()
(something to do with the mysterious 0xa7
```

### (18) Look for the assumed `LocApiProxyV02` actually does
ChatGPT goes to the only library so far not checked yet: `liblbs_core` from step 13.
```
readelf --wide --symbols --demangle liblbs_core.so | grep "LocApiProxyV02"
    56: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND izat_core::IzatApiV02::IzatApiV02(lbs_core::LocApiProxyV02*)
   118: 000000000000d2e0    16 FUNC    GLOBAL DEFAULT   14 lbs_core::LocApiProxyV02::eventCb(void*, unsigned int, locClientEventIndUnionType)
   165: 000000000000d270    44 FUNC    GLOBAL DEFAULT   14 lbs_core::LocApiProxyV02::~LocApiProxyV02()
   173: 000000000000d1e0   144 FUNC    GLOBAL DEFAULT   14 lbs_core::LocApiProxyV02::LocApiProxyV02(lbs_core::LBSApiV02*)
   192: 000000000000d29c    68 FUNC    GLOBAL DEFAULT   14 lbs_core::LocApiProxyV02::~LocApiProxyV02()
   211: 000000000000d1e0   144 FUNC    GLOBAL DEFAULT   14 lbs_core::LocApiProxyV02::LocApiProxyV02(lbs_core::LBSApiV02*)
   219: 00000000000124a8    40 OBJECT  GLOBAL DEFAULT   16 vtable for lbs_core::LocApiProxyV02
   253: 000000000000d270    44 FUNC    GLOBAL DEFAULT   14 lbs_core::LocApiProxyV02::~LocApiProxyV02()
```
This library does not have the syncSendReq. There must be more steps in between the 2nd last and last step.

### (19) Spot check the constructor
```C++
void __thiscall lbs_core::LocApiProxyV02::LocApiProxyV02(LocApiProxyV02 *this,LBSApiV02 *param_1)
{
  IzatApiV02 *this_00;
  
  *(undefined ***)this = &PTR_~LocApiProxyV02_001124b8;
  *(LBSApiV02 **)(this + 8) = param_1;
  this_00 = operator.new(0x80);
  izat_core::IzatApiV02::IzatApiV02(this_00,this);
  *(IzatApiV02 **)(this + 0x10) = this_00;
  if ((_loc_logger & 0xfffffffffffffffe) == 4) 
  {
    __android_log_print(3,0,"%s:%d]: LocApiProxyV02 created:%p, mLBSApi:%p, mIzatApi:%p, ",
                        "LocApiProxyV02",0x17,this,*(undefined8 *)(this + 8),this_00);
    return;
  }
  return;
}
```
It stores an LBSApiV02 right before itself
LocApiProxyV02
- +0x08 → LBSApiV02
- +0x10 → IzatApiV02

### (20) Spot check `LocApiProxyV02::eventCb()`
```C++
void lbs_core::LocApiProxyV02::eventCb(long param_1)

{
  /* WARNING: Could not recover jumptable at 0x0010d2ec. Too many branches */
  /* WARNING: Treating indirect jump as call */
  (**(code **)(**(long **)(param_1 + 0x10) + 0x180))();
  return;
}
```
This just calls another function at IzatApiV02.vtable + 0x180. 
In step 19 you can see the structure of the LocApiProxyV02 has an IzatApiV02 at self+0x10. 
It's likely param_1 is an LocApiProxyV02*

(In hindsight cb is callback.)

## Final Pieces
### (21) Go back to `LibLocApiV02::locSyncSendReq()`
Couldn't get meaning from `0xa7` but it appears to be the actual xtra data request. 
This is the place to pay attention to. 

```C++
int __thiscall
LocApiV02::locSyncSendReq
          (LocApiV02 *this,undefined4 param_1,undefined8 param_3,undefined4 param_4,
          undefined4 param_5,int *param_6)

{
  long lVar1;
  void *__src;
  int iVar2;
  uint uVar3;
  void *__dest;
  ulong uVar4;
  code *pcVar5;
  undefined8 uVar6;
  void *local_b0;
  uint local_a4;
  function afStack_a0 [32];
  function *local_80;
  long local_68;
  
  lVar1 = tpidr_el0;
  local_68 = *(long *)(lVar1 + 0x28);
  iVar2 = loc_sync_send_req(*(undefined8 *)(this + 0xe8));
  if ((iVar2 == 4) || (((param_6 != (int *)0x0 && (iVar2 == 0)) && (*param_6 == 4)))) 
  {
    if ((*(long *)(this + 0x128) == *(long *)(this + 0x130)) &&
       (uVar4 = *(ulong *)(this + 0x110), ((uint)uVar4 >> 7 & 1) == 0)) 
    {
      uVar6 = *(undefined8 *)(this + 0xe8);
      uVar3 = loc_core::LocApiBase::isMaster();
      locClientRegisterEventMask(uVar6,uVar4 | 0x80,uVar3 & 1);
    }
    if ((_loc_logger & 0xfffffffffffffffe) == 4) 
    {
      __android_log_print(3,"LocSvc_ApiV02","%s:%d] Engine busy, cache req: %d","locSyncSendReq", 0x1315,param_1);
    }
    local_a4 = 0;
    local_b0 = (void *)0x0;
    validateRequest(param_1,param_3,&local_b0,&local_a4);
    __src = local_b0;
    if (local_b0 == (void *)0x0) 
    {
      __dest = (void *)0x0;
    }
...
```
It looks like `loc_sync_send_req` is fed some kind of struct from `param_1->inner_struct`.
You can somewhat guess what is in the struct based on the fact that the parameters of `loc_sync_send_req` has expanded it out.

```C
int loc_sync_send_req(undefined8 param_1,undefined4 param_2,undefined8 param_3,uint param_4, undefined4 param_5,undefined8 param_6)
{
  pthread_mutex_t *__mutex;
  long lVar1;
  long lVar2;
  bool bVar3;
  uint uVar4;
  int iVar5;
  undefined8 uVar6;
  ulong uVar7;
  undefined1 *puVar8;
  ulong uVar9;
  undefined4 uVar10;
  undefined4 uVar11;
  char *pcVar12;
  bool bVar13;
  timespec local_78;
  long local_68;
  
// skipped all the mutex finagling

  uVar7 = locClientSendReq(param_1,param_2,param_3);
  iVar5 = (int)uVar7;
```
Being the only 4byte/32bit parameter passed into `locClientSendReq`, this is probably the request code.
Interestingly, param_2 hasn't changed at all from the function signature to the call to `locClientSendReq`.
It is passed along without modification. This really makes 0xa7 interesting.

```C
undefined8 locClientSendReq(long param_1,int param_2,undefined8 *param_3)

{
  long lVar1;
  byte bVar2;
  int iVar3;
  ulong uVar4;
  undefined8 uVar5;
  undefined8 uVar6;
  undefined8 local_98;
  undefined4 local_8c;
  undefined8 local_88;
  undefined8 local_80;
  undefined8 uStack_78;
  undefined8 uStack_70;
  undefined8 uStack_68;
  long local_58;
  
  lVar1 = tpidr_el0;
  local_58 = *(long *)(lVar1 + 0x28);
  local_88 = 0;
  local_8c = 0;
  local_98 = 0;
  if (((param_1 == 0) || (*(long *)(param_1 + 8) == 0)) || (*(long *)(param_1 + 0x30) != param_1)) {
    if (_loc_logger - 1U < 5) {
      __android_log_print(6,"LocSvc_api_v02","%s:%d]: invalid handle \n","locClientSendReq",0x87b);
    }
    uVar6 = 10;
    goto LAB_001196ec;
  }
  uVar4 = validateRequest(param_2,param_3,&local_98,&local_8c);
  // skipped
  iVar3 = qmi_client_send_msg_sync(*(undefined8 *)(param_1 + 8),param_2,local_98,local_8c,&local_88,8,1000);
```
Tracing `param_2` to the validation function, conveniently `0xa7` is a case. ChatGPT guesse 0x414 is a size of something. 
This shows the assumed request code 0xa7 is just passed along like a hot potato.
Based on ChatGPT's Google searching, the `qmi_client...` looks like the end of the road and shows 0xa7 is actually something the gps hardware understands.

(Validation function)
```C
validateRequest(undefined4 param_1,undefined8 param_2,undefined8 *param_3,undefined4 *param_4)
{
  undefined4 uVar1;
  
  if (_loc_logger == 5) {
    __android_log_print(2,"LocSvc_api_v02","%s:%d]: reqId = %d\n","validateRequest",0x481,param_1);
  }
  uVar1 = 4;
  switch(param_1) 
  {
   ...
   case 0xa7:
    uVar1 = 0x414;
    break;
  }
  ...
}

The mystery request 0xa7 returns 0x414 = 1044. Probably a size of something.

```

### (22) Reexamine IzatApiV02::injectXtraData
This is where we first saw 0xa7. If 1044 is the size of some struct, perhaps the IzaApiV02 version of injectXtraData can tell us what this mystery payload is.

Based on how `IzatApiV02::injectXtraData` calls `LibLocApiV02::locSyncSendReq` the following conclusions can be made

`iVar7 = LocApiV02::locSyncSendReq(*(undefined8 *)(*(long *)(this + 0x58) + 8), 0xa7, &local_488, 1000, 0xa7, &local_4c8);`
- param 1 is a handle
- param 2 is the request code
- param 3 is the mystery struct
- param 4 is probably a timeout
- param 5 is the request code again
- param 6 is some return results other mystery struct

(See the `IzatApiV02::injectXtraData.cpp` file for the full Ghidra slop.)

### (23) param3 mystery struct
The Ghidra C slop shows local_488 passed in as the struct. local + ### is just an offset into the function's scratch space stack.

You can guess this struct is 1044 bytes long. 0x488 - 0x414 = 0x74. The _### from 488 to 74 are of interest. Subtraction because the stack grows from ceiling to floor.

Ghidra C slop shows:
```C
uint local_488;
undefined2 local_484;
short local_482;
int local_480;
undefined1 auStack_47c [1024];
undefined1 local_7c;
undefined4 local_78;
```

```
488 is the ceiling which makes it the very start of the struct
{
   488-484 -> 4 bytes -> int4 some int
   484-482 -> 2 bytes -> int2 some short
   482-480 -> 2 bytes -> int2 some short
   480-47c -> 4 bytes -> int4 some int
   (47c to 7c) 1024 bytes (it tells you) -> char* [1024]
   7c directly: 1 byte
   7d-78 : 3 bytes (probably compiler padding)
   78-74 : 4 bytes (the remaining 4 based on the 1044 struct size)
}
```
Next slop variable is local_70 which is close enough

### (24) obfuscated javascript like undoing

The conclusions below are based on reading parts of the slop and guessing what they do

- 488 = param2 = total size
- 484 = total size - 1 / 1024 -> # of 1024 byte chunks off by 1 then +1
- 482 = some 1 based incrementer -> probably chunk #
- 480 = min of chunk size - something OR 1024, probably chunk size
- 1 byte after the [1024] = preset to 1
- 4 bytes after the 1byte after [1024] + its padding = preset to 0

```
{
   totalSize: uint4
   numberOf1024Chunks: uint2
   chunkNumber: uint2
   chunkSize: uint4
   chunk: byte[1024]
   always1: byte = 1
   filler: byte[3]
   tail: uint4 = 0
}
```

### (25) Pixel 1XL help
Information on this location api v2 can be borrowed from the pixel 1 XL, which I also have.

https://android.googlesource.com/device/google/marlin/+/android-7.1.0_r2/location

^^^ tells you how to open the client, how to close the client, exactly what an xtra injection payload looks like.

## Done
You know `IzatApiV02::injectXtraData` eventually calls locSyncSendReq. 
This tells you how to take an xtra file and cut it up in to pieces in a way the gps hardware can understand. 