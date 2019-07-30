1. Connect a CMBS.
2. Move to the installation folder.
3. Run CMBS TCX. An example of how to make it look for /dev/ttyACM0 is shown below:
> ```
> ./bin/cmbs_tcx -usb -com 0 -han
> ```
4. Edit setenv.sh to point to /lib folder.
5. Configure bash with HAN-FUN to IoTivity Adaptor environment:
> ```
> source setenv.sh
> ```
6. Set a clean state. Repeat this step everytime a clean initialization is needed.
> ```
> sh clear-state.sh
> ```
7. Run the HF-to-IoTivity part of the adaptor:
> ```
> ./bin/PluginManager ./bin/HanFunBridge --hf
> ```