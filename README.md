# uds
Separation of upstream and downstream traffic.

## How to build

1. Install all libs. We highly recommend to use [vcpkg](https://github.com/microsoft/vcpkg).

   ```powershell
   //1.install boost
   //WARNING:if you use vcpkg install boost,32-bit version will be installed.
   vcpkg install boost:x86-windows
   vcpkg install boost:x64-windows
   //2.install openssl
   //openssl 1.1.1 is recommended,but 3.x is acceptable.
   vcpkg install openssl
   //3.install jemalloc
   vcpkg install jemalloc
   ```

2. Add Preprocessor Definitions

   ```vs
   _CRT_SECURE_NO_DEPRECATE
   
   _CRT_NONSTDC_NO_DEPRECATE
   ```

3. build it!

   
## How to use

create a file with the following name:

```
"uds.ini"
"udsd.ini"
"udsc.ini"
"udss.ini"
```

and input your config.

An example:

udsc.ini:

```
[app]
alignment=4096
mode=client
ip=127.0.0.1
port=7000
backlog=511
fast-open=true
turbo.lan=true
turbo.wan=true
connect.timeout=10
inactive.timeout=72
protocol=tcp

[http]
type=tcp
local-ip=185.207.153.31
local-port=80
remote-port=80
reconnect=1
concurrent=1

[ssh]
type=tcp
local-ip=192.168.0.24
local-port=22
remote-port=10022
reconnect=1
concurrent=1

[dns]
type=udp
local-ip=192.168.0.24
local-port=53
remote-port=10053
reconnect=1
concurrent=1
```

udss.ini:

```
[app]
alignment=4096
mode=server
ip=::
port=7000
backlog=511
fast-open=true
turbo.lan=true
turbo.wan=true
connect.timeout=10
inactive.timeout=72
handshake.timeout=5
protocol=tcp
```

totally the same as the previous [frp project](https://github.com/liulilittle/frp/tree/main/frp/samples)

## 作者补充

内置SSL/TLS、encryptor加密机、websocket、TCP等。

默认配置是TCP透明协议，如果只是爬梯目前来说加不加密不重要，梯子流量自己加密就行了。

TLS缺省为TLSv13，如果需要TLSv12，需要自己改下代码或者扩展一下。
