# Pixel 3XL Reverse Engineered GPS XTRA data injector.
Early this year I noticed the Pixel 3XL GPS takes an ungodly amount of time to get its first GPS fix and can be unstable. 
I clean wiped and reinstalled mind the gapps. That didn't help. I also noticed the agps toggle always turns itself back off.
Terrible *unrelated problems* made it impossible to have the energy to look any further

## Original Observations
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

(This section was copied from [the investigation info](https://github.com/AAccount/p3xl_xtra/blob/master/learnings/chatgpt%20investigation.md).)

## Original Intention
I saw these logs back in `2026-02` already. It is pretty clear something doesn't want to start the gps xtra data injection. 
Before this, I already knew GPS XTRA data has something to do with precalculated satellite info and the GPS uses this somehow to get get a quick GPS fix.
This satellite data is available publicly and is downloaded by the cell phone and "sent" to the gps receiver hardware.

I wanted to know if it was possible to "hand start" this injection process. It's well known where to get the file. What if I get it myself and then feed it?
Unfortunately, I have never done any kind of binary reverse engineering. I thought "let's see if Gemini can show me how to do this".
Gemini sucked. It went in circles with the logcat, and when I suggested reverse engineering it had the memory of a goldfish. It went it circles spinning its virtual wheels.

Chat GPT was very eager to take on reverse engineering but had the problem of ouputting a mountain of output and 5 suggestions to try.
I asked for 1 suggestion at a time and much less text. The first evening was a blind attempt. 
I am backend by trade (Java/Typescript), but have dabbled in C++ (see other repos), Linux sysadmin, and still have some understanding of pointers and memory models.
This was barely enough to follow along the first evening. You start to see some patterns emerge.
The 2nd night I asked it to walk me through Ghidra as I suspected it would make things much easier to follow. It did. It also produced the working xtra-probes.

That night, I saw the Pixel 3XL's GPS get a fix in under 30 seconds for the first time in years. Reconfirmed the next day.

## Timeline
- 1 week of learning basic assembly by ChatGPT and googling parts that it seemed to assume I had background in.
- 1 week to retrace the investigation with Ghidra from the start.
- 1 week to relearn how to write android apps. I last dabbled in the traditional Java + XML. Now it's Kotlin + Jetpack compose. Finally, write this small app.
The xtra injection process was tested once per week during these weeks to reprove the concept.

**"Field test"**: 2026-09-27: let OSMAnd+ on FDroid with Android Auto xposed unlocker module run on my car for an 82km run.
Starting point was where the screenshot shows. Not doxing myself for where the end point was, but it held a stable GPS lock the whole time.

## Gotcha
**You NEED Magisk installed. This uses libsu.**

## Usage
- turn on location. (Doesn't matter what the agps toggle is)
- run the app
- wait 10 seconds
- turn location off and on
- pull up sat stat

Sat stat post injection works a bit differently. 
It will show zero satellites, the suddenly after 5 to 30 seconds (depending on where you are), **instant** 12/20 satellites locked on.

## In Action
Fast first fix with LineageOS 22 (Android 15)
![Fast first fix with LineageOS 22 (Android 15)](https://github.com/AAccount/p3xl_xtra/blob/master/screenshots/los22%20a15%20fast%20first%20fix.png)

Current LineageOS build
![Current LineageOS build](https://github.com/AAccount/p3xl_xtra/blob/master/screenshots/los22%20a15.png)

Sample UI of this app. (The simulation mode warning is wrong. I forgot to remove that.)
![Sample UI of this app](https://github.com/AAccount/p3xl_xtra/blob/master/screenshots/ui.png)

## Hacks
The original plan was to take the C code of the investigation and run it through a JNI (KNI) interface.
That involves setting up a root service and doing binder calls through AIDL. It was all setup to work until I got a nasty surprise.
Android apps don't have full view of the os image anymore. My "xtra inject service" couldn't open `libloc_api_v02.so` anymore.

After some googling the least worst option was to keep the C as its own executable, copy it to somehwere you *can* actually see the whole OS image, run it from there, then recapture the output.
This has its own problems of the standard cmake producing libraries, not executables. Not really having an interest in cmake administrative work, I let Gemini wizard that one.

Every single command that is run as root is printed to the debug text box so you can see what it is doing. You can also save the debug output and clear it.

## "Field Test" exported debug output
Saved the actual test run logs before the sat stat screenshot.
```text
Download xtra from https://xtrapath2.izatcloud.net/xtra3grcej.bin to /data/user/0/dt.gpsxtra/files/xtra.blob
cp -F /data/user/0/dt.gpsxtra/files/xtra.blob /data/local/tmp/xtra.blob
-----------------------
copy xtra data blob
return code 0
-----------------------
cp -F /data/app/~~3UpPyazQZHA1SU0Y9xUzjQ==/dt.gpsxtra-jUcS1zH-ZAV3GKr5lZLSpQ==/lib/arm64/libxtra-hand-start.so /data/local/tmp/libxtra-hand-start.so
-----------------------
copy hand start utility
return code 0
-----------------------
running /data/local/tmp/libxtra-hand-start.so
-----------------------
hand start result
return code 0
!! SIMULATION MODE !!
xtra file size 50981
total parts 50
libloc handle 0x503a1c29b137d361
open handle 0x71c5db04d0 send request handle 0x71c5daf65c close handle 0x71c5db0280
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 1, partData_len: 1024}
successfully sent
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 2, partData_len: 1024}
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 3, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 02 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 4, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 03 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 04 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 5, partData_len: 1024}
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 6, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 05 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 06 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 7, partData_len: 1024}
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 8, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 07 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 9, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 08 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 10, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 09 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 11, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 0A 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 12, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 0B 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 13, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 0C 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 14, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 0D 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 15, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 0E 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 16, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 0F 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 10 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 17, partData_len: 1024}
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 18, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 11 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 19, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 12 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 20, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 13 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 14 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 21, partData_len: 1024}
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 22, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 15 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 23, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 16 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 24, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 17 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 18 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 25, partData_len: 1024}
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 26, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 19 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 27, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 1A 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 28, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 1B 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 29, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 1C 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 1D 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 30, partData_len: 1024}
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 31, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 1E 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 32, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 1F 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 20 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 33, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 21 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 34, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 22 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 35, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 23 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 36, partData_len: 1024}
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 37, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 24 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 25 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 38, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 26 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 39, partData_len: 1024}
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 40, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 27 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 41, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 28 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 42, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 29 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 2A 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 43, partData_len: 1024}
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 44, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 2B 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 45, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 2C 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 46, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 2D 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 47, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 2E 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 48, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 2F 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 49, partData_len: 1024}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 30 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
XtraRequest{totalSize: 50981, totalParts: 50, partNum: 50, partData_len: 805}
response callback got 167 of size 24
raw payload: [00 00 00 00 01 00 31 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
successfully sent
closed handle exit with 0
-----------------------
```
