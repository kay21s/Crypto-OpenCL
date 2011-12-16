AESEncryptDecrypt:AESEncryptDecrypt.o
	g++ -o AESEncryptDecrypt build/AESEncryptDecrypt.o -lpthread -ldl -L/usr/X11R6/lib   -lSDKUtil  -lOpenCL   -L/opt/AMDAPP/lib/x86_64  -L/opt/AMDAPP/TempSDKUtil/lib/x86_64 -L"/lib/x86_64"

AESEncryptDecrypt.o:AESEncryptDecrypt.cpp
	g++  -Wpointer-arith   -Wfloat-equal -g3 -ffor-scope   -I  /opt/AMDAPP/samples/opencl/SDKUtil/include -I  "/include"  -I  /opt/AMDAPP/include  -o build/AESEncryptDecrypt.o -c  AESEncryptDecrypt.cpp
clean:
	rm AESEncryptDecrypt build/AESEncryptDecrypt.o
