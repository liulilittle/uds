# uds
Separation of upstream and downstream traffic.

## How to build on Windows

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
   Just click the build button.
   
## How to build on Linux
1. install the libboost libopenssl and jemalloc
   Due to difference among various Linux distributions, there would be little changes and problems when trying to install the packages.
   a. The Package names are not correct 
   b. The system's jemalloc contains no 'Je_' prefix
2. Modify the CMakeList to add the system libs when building 
3. build it 
   ```bash
   cd uds && cmake . -DCMAKE_BUILD_TYPE=Release --fresh && make -j$nproc
   ```
   

   
## How to use

create a file and type in your config.

Config examples have been listed in the uds/example dir, just a little modify is required

Then use '-c [config_file]' parameter to let the program know which config to use.
## 作者补充

内置SSL/TLS、encryptor加密机、websocket、TCP等。

默认配置是TCP透明协议，如果只是爬梯目前来说加不加密不重要，梯子流量自己加密就行了。

TLS缺省为TLSv13，如果需要TLSv12，需要自己改下代码或者扩展一下。
