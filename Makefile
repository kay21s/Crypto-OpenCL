CFLAGS = -DDEVICE_ID=0 -DITERATION=10
# DEVICE_ID : 0->GPU in APU, 1->discrete GPU
CFALGS += -DSTREAM_DYNAMIC -DTRANSFER_OVERLAP_0 

AESEncryptDecrypt:AES_dynamic.o Log.o Timer.o StreamGenerator.o
	g++ -g -o AESEncryptDecrypt build/Log.o build/Timer.o build/StreamGenerator.o build/AES_dynamic.o -lpthread -ldl -L/usr/X11R6/lib   -lSDKUtil  -lOpenCL   -L/opt/AMDAPP/lib/x86_64  -L/opt/AMDAPP/TempSDKUtil/lib/x86_64 -L"/lib/x86_64" -lrt

AES_dynamic.o:AES_dynamic.cpp
	g++ $(CFLAGS) -g -Wpointer-arith   -Wfloat-equal -g3 -ffor-scope   -I  /opt/AMDAPP/samples/opencl/SDKUtil/include -I  "/include"  -I  /opt/AMDAPP/include  -o build/AES_dynamic.o -c  AES_dynamic.cpp -lrt
Log.o:Log.cpp
	g++ $(CFLAGS) -g -Wpointer-arith   -Wfloat-equal -g3 -ffor-scope   -I  /opt/AMDAPP/samples/opencl/SDKUtil/include -I  "/include"  -I  /opt/AMDAPP/include  -o build/Log.o -c  Log.cpp -lrt
Timer.o:Timer.cpp
	g++ $(CFLAGS) -g -Wpointer-arith   -Wfloat-equal -g3 -ffor-scope   -I  /opt/AMDAPP/samples/opencl/SDKUtil/include -I  "/include"  -I  /opt/AMDAPP/include  -o build/Timer.o -lrt -c  Timer.cpp -lrt
StreamGenerator.o:StreamGenerator.cpp
	g++ $(CFLAGS) -g -Wpointer-arith   -Wfloat-equal -g3 -ffor-scope   -I  /opt/AMDAPP/samples/opencl/SDKUtil/include -I  "/include"  -I  /opt/AMDAPP/include  -o build/StreamGenerator.o -c  StreamGenerator.cpp -lrt
clean:
	rm -v -f AESEncryptDecrypt build/AESEncryptDecrypt.o build/AES_overlap.o build/Log.o build/Timer.o build/StreamGenerator.o
