#include "AESEncryptDecrypt.hpp"

#if defined(TRANSFER_OVERLAP)

using namespace AES;

int AESEncryptDecrypt::setupAESEncryptDecrypt()
{
	// Initialie TimeLog
	timeLog = new TestLog(ITERATION * 50);
	CHECK_ALLOCATION(timeLog, "Failed to allocate host memory. (timeLog)");

	// Set parameters for the stream set
	streamGenerator.SetStreamNumber(STREAM_NUM);
	streamGenerator.SetInterval(INTERVAL);
	streamGenerator.InitStreams(AVG_RATE, VAR_RATE, AVG_DEADLINE, 
		VAR_DEADLINE, AVG_PERIOD, VAR_PERIOD);
	// FIXME: !!!!!! Set deadline = period

	/* 1 Byte = 8 bits, 128bits => 16bytes */
	keySize = keySizeBits/8;
  
	/* due to unknown represenation of cl_uchar */ 
	keySizeBits = keySize*sizeof(cl_uchar); 

	buffer_size = streamGenerator.GetMaxBufferSize();
	stream_num = streamGenerator.GetStreamNumber();

	return SDK_SUCCESS;
}

int AESEncryptDecrypt::genBinaryImage()
{
	streamsdk::bifData binaryData;
	binaryData.kernelName = std::string("AESEncryptDecrypt_Kernel.cl");
	binaryData.flagsStr = std::string("");
	if(isComplierFlagsSpecified())
		binaryData.flagsFileName = std::string(flags.c_str());

	binaryData.binaryName = std::string(dumpBinary.c_str());
	int status = sampleCommon->generateBinaryImage(binaryData);
	return status;
}

int AESEncryptDecrypt::setupEncryption(void)
{
	cl_int status = 0;
	// Set Presistent memory only for AMD platform
	cl_mem_flags inMemFlags = CL_MEM_READ_ONLY;
	if(isAmdPlatform())
		inMemFlags |= CL_MEM_USE_PERSISTENT_MEM_AMD;

	// TRANSFER_OVERLAP ---- Buffer set 0 ----
	inputBuffer = clCreateBuffer(
					context,
					inMemFlags,
					sizeof(cl_uchar) * buffer_size,
					NULL,
					&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (inputBuffer)");

	outputBuffer = clCreateBuffer(
					context,
					CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR,
					sizeof(cl_uchar) * buffer_size,
					NULL,
					&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (outputBuffer)");

	pktOffsetBuffer = clCreateBuffer(
					context,
					inMemFlags,
					sizeof(cl_uint) * (stream_num + 1),
					NULL,
					&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (pktOffsetBuffer)");

	keyBuffer = clCreateBuffer(
					context,
					inMemFlags,
					keySize * stream_num,
					NULL,
					&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (keyBuffer)");

	ivsBuffer = clCreateBuffer(
					context,
					inMemFlags,
					AES_IV_SIZE * stream_num,
					NULL,
					&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (ivsBuffer)");


	// TRANSFER_OVERLAP ---- Buffer set 1 ----
	inputBuffer1 = clCreateBuffer(
					context,
					inMemFlags,
					sizeof(cl_uchar) * buffer_size,
					NULL,
					&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (inputBuffer1)");

	outputBuffer1 = clCreateBuffer(
					context,
					CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR,
					sizeof(cl_uchar) * buffer_size,
					NULL,
					&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (outputBuffer1)");

	pktOffsetBuffer1 = clCreateBuffer(
					context,
					inMemFlags,
					sizeof(cl_uint) * (stream_num + 1),
					NULL,
					&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (pktOffsetBuffer1)");

	keyBuffer1 = clCreateBuffer(
					context,
					inMemFlags,
					keySize * stream_num,
					NULL,
					&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (keyBuffer1)");

	ivsBuffer1 = clCreateBuffer(
					context,
					inMemFlags,
					AES_IV_SIZE * stream_num,
					NULL,
					&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (ivsBuffer1)");

	return SDK_SUCCESS;
}

int AESEncryptDecrypt::setupDecryption(void)
{
	return SDK_SUCCESS;
}

int AESEncryptDecrypt::verifyResults(void)
{
	return SDK_SUCCESS;
}

int AESEncryptDecrypt::setupCL(void)
{
	cl_int status = 0;
	cl_device_type dType;
    
	if(deviceType.compare("cpu") == 0)
	{
		std::cout << "Running on CPU......" << std::endl;
		dType = CL_DEVICE_TYPE_CPU;
	}
	else //deviceType = "gpu" 
	{
		dType = CL_DEVICE_TYPE_GPU;
		std::cout << "Running on GPU......" << std::endl;
		if(isThereGPU() == false)
		{
			std::cout << "GPU not found. Falling back to CPU device" << std::endl;
			dType = CL_DEVICE_TYPE_CPU;
		}
	}

	/*
		* Have a look at the available platforms and pick either
		* the AMD one if available or a reasonable default.
		*/
	cl_platform_id platform = NULL;
	int retValue = sampleCommon->getPlatform(platform, platformId, isPlatformEnabled());
	CHECK_ERROR(retValue, SDK_SUCCESS, "sampleCommon::getPlatform() failed");

	// Display available devices.
	retValue = sampleCommon->displayDevices(platform, dType);
	CHECK_ERROR(retValue, SDK_SUCCESS, "sampleCommon::displayDevices() failed");

	/*
		* If we could find our platform, use it. Otherwise use just available platform.
		*/
	cl_context_properties cps[3] = 
	{
		CL_CONTEXT_PLATFORM, 
		(cl_context_properties)platform, 
		0
	};

	context = clCreateContextFromType(
					cps,
					dType,
					NULL,
					NULL,
					&status);
	CHECK_OPENCL_ERROR(status, "clCreateContextFromType failed.");

	// getting device on which to run the sample
	status = sampleCommon->getDevices(context, &devices, deviceId, isDeviceIdEnabled());
	CHECK_ERROR(status, SDK_SUCCESS, "sampleCommon::getDevices() failed");

	// Select the device!!!!!!!!
	deviceId = DEVICE_ID;

	{
		// The block is to move the declaration of prop closer to its use
		cl_command_queue_properties prop = 0;
		commandQueue = clCreateCommandQueue(
				context, 
				devices[deviceId], 
				prop, 
				&status);
		CHECK_OPENCL_ERROR( status, "clCreateCommandQueue failed.");
	}

	std::cout << "Device ID : " << deviceId << std::endl;
	
	//Set device info of given cl_device_id
	retValue = deviceInfo.setDeviceInfo(devices[deviceId]);
	CHECK_ERROR(retValue, 0, "SDKDeviceInfo::setDeviceInfo() failed");
   
	// create a CL program using the kernel source 
	streamsdk::buildProgramData buildData;
	buildData.kernelName = std::string("AESEncryptDecrypt_Kernel.cl");
	buildData.devices = devices;
	buildData.deviceId = deviceId;
	buildData.flagsStr = std::string("");
	if(isLoadBinaryEnabled())
		buildData.binaryName = std::string(loadBinary.c_str());

	if(isComplierFlagsSpecified())
		buildData.flagsFileName = std::string(flags.c_str());

	retValue = sampleCommon->buildOpenCLProgram(program, context, buildData);
	CHECK_ERROR(retValue, 0, "sampleCommon::buildOpenCLProgram() failed");

	/* get a kernel object handle for a kernel with the given name */
	if(decrypt)
	{
		kernel = clCreateKernel(program, "AES_cbc_128_decrypt", &status);
		setupDecryption();
	}
	else
	{
		kernel = clCreateKernel(program, "AES_cbc_128_encrypt_new", &status);
		setupEncryption();
	}
	CHECK_OPENCL_ERROR(status, "clCreateKernel failed.");

	return SDK_SUCCESS;
}

int AESEncryptDecrypt::overlapMapBuffer0(cl_event *mapEvent)
{
	cl_int status;

	// Write buffers
	input = (cl_uchar *)clEnqueueMapBuffer(
					commandQueue,
					inputBuffer,
					CL_FALSE,
					CL_MAP_WRITE,
					0,
					sizeof(cl_uchar) * buffer_size,
					0,
					NULL,
					mapEvent,
					&status);
	if(input == NULL) {
		std::cout << "clEnqueueMapBuffer(inputbuffer) failed.";
		return SDK_FAILURE;
	}

	//////////
	pkt_offset = (cl_uint *)clEnqueueMapBuffer(
					commandQueue,
					pktOffsetBuffer,
					CL_FALSE,
					CL_MAP_WRITE,
					0,
					sizeof(cl_uint) * (stream_num + 1),
					0,
					NULL,
					mapEvent,
					&status);
	if(pkt_offset == NULL) {
		std::cout << "clEnqueueMapBuffer(pktOffsetBuffer) failed.";
		return SDK_FAILURE;
	}

	//////////
	keys = (cl_uchar *)clEnqueueMapBuffer(
					commandQueue,
					keyBuffer,
					CL_FALSE,
					CL_MAP_WRITE,
					0,
					keySize * stream_num,
					0,
					NULL,
					mapEvent,
					&status);
	if(keys == NULL) {
		std::cout << "clEnqueueMapBuffer(keyBuffer) failed.";
		return SDK_FAILURE;
	}

	//////////
	ivs = (cl_uchar *)clEnqueueMapBuffer(
					commandQueue,
					ivsBuffer,
					CL_FALSE,
					CL_MAP_WRITE,
					0,
					AES_IV_SIZE * stream_num,
					0,
					NULL,
					mapEvent,
					&status);
	if(ivs == NULL) {
		std::cout << "clEnqueueMapBuffer(ivsBuffer) failed.";
		return SDK_FAILURE;
	}

	// Flush
	status = clFlush(commandQueue);
	CHECK_OPENCL_ERROR(status, "clFlush failed.");

	return SDK_SUCCESS;
}

int AESEncryptDecrypt::overlapMapBuffer1(cl_event *mapEvent)
{
	cl_int status;

	// Write buffers
	input1 = (cl_uchar *)clEnqueueMapBuffer(
					commandQueue,
					inputBuffer1,
					CL_FALSE,
					CL_MAP_WRITE,
					0,
					sizeof(cl_uchar) * buffer_size,
					0,
					NULL,
					mapEvent,
					&status);
	if(input1 == NULL) {
		std::cout << "clEnqueueMapBuffer(inputbuffer1) failed.";
		return SDK_FAILURE;
	}

	//////////
	pkt_offset1 = (cl_uint *)clEnqueueMapBuffer(
					commandQueue,
					pktOffsetBuffer1,
					CL_FALSE,
					CL_MAP_WRITE,
					0,
					sizeof(cl_uint) * (stream_num + 1),
					0,
					NULL,
					mapEvent,
					&status);
	if(pkt_offset1 == NULL) {
		std::cout << "clEnqueueMapBuffer(pktOffsetBuffer1) failed.";
		return SDK_FAILURE;
	}

	//////////
	keys1 = (cl_uchar *)clEnqueueMapBuffer(
					commandQueue,
					keyBuffer1,
					CL_FALSE,
					CL_MAP_WRITE,
					0,
					keySize * stream_num,
					0,
					NULL,
					mapEvent,
					&status);
	if(keys1 == NULL) {
		std::cout << "clEnqueueMapBuffer(keyBuffer1) failed.";
		return SDK_FAILURE;
	}

	//////////
	ivs1 = (cl_uchar *)clEnqueueMapBuffer(
					commandQueue,
					ivsBuffer1,
					CL_FALSE,
					CL_MAP_WRITE,
					0,
					AES_IV_SIZE * stream_num,
					0,
					NULL,
					mapEvent,
					&status);
	if(ivs1 == NULL) {
		std::cout << "clEnqueueMapBuffer(ivsBuffer1) failed.";
		return SDK_FAILURE;
	}

	// Flush
	status = clFlush(commandQueue);
	CHECK_OPENCL_ERROR(status, "clFlush failed.");

	return SDK_SUCCESS;
}

int AESEncryptDecrypt::overlapFillBufferUnmap0(unsigned int *this_stream_num, unsigned int *this_buffer_size, cl_event *mapEvent)
{
	cl_int status;
    cl_event event;
	CPerfCounter t;

	// wait for previous map to complete
	status = clWaitForEvents(1, mapEvent);
    CHECK_OPENCL_ERROR(status, "clWaitForEvents() failed.");

	t.Reset();
	t.Start();
	// Fill the buffers
	*this_buffer_size = streamGenerator.GetStreams(input, buffer_size, 
			keys, ivs, pkt_offset, this_stream_num);

	t.Stop();
	timeLog->Timer(
           "%s %f ms\n", " 0 -- Get Streams Time",
           t.GetTotalTime(),
           10,
           1);
	// Log
	timeLog->Msg("%s %d streams\n", "     -- This time we have ", *this_stream_num);
	timeLog->Msg("%s %d bytes\n", "     -- Buffer size is ", *this_buffer_size);

	if (*this_stream_num > stream_num) {
		std::cout << "What's the problem!!!!" << std::endl;
		exit(0);
	}

	t.Reset();
	t.Start();
	// Unmap all of them
	status = clEnqueueUnmapMemObject(
                commandQueue,
                inputBuffer,
                (void *) input,
                0,
                NULL,
                &event);
    CHECK_OPENCL_ERROR(status, "clEnqueueUnmapMemObject(inputBuffer1) failed.");

	status = clEnqueueUnmapMemObject(
                commandQueue,
                pktOffsetBuffer,
                (void *) pkt_offset,
                0,
                NULL,
                &event);
    CHECK_OPENCL_ERROR(status, "clEnqueueUnmapMemObject(pktOffsetBuffer1) failed.");

	status = clEnqueueUnmapMemObject(
                commandQueue,
                keyBuffer,
                (void *) keys,
                0,
                NULL,
                &event);
    CHECK_OPENCL_ERROR(status, "clEnqueueUnmapMemObject(keyBuffer1) failed.");

	status = clEnqueueUnmapMemObject(
                commandQueue,
                ivsBuffer,
                (void *) ivs,
                0,
                NULL,
                &event);
    CHECK_OPENCL_ERROR(status, "clEnqueueUnmapMemObject(ivsBuffer1) failed.");
 
	// Wait for all Unmap and previous operations to complete!!!
    status = clWaitForEvents(1, &event);
    CHECK_OPENCL_ERROR(status, "clWaitForEvents() failed.");

	 // Release event
    status = clReleaseEvent(event);
    CHECK_OPENCL_ERROR(status, "clReleaseEvent(event) failed.");

	t.Stop();
	timeLog->Timer(
           "%s %f ms\n", " 0 -- Unmap Memory Time",
           t.GetTotalTime(),
           10,
           1);

	return SDK_SUCCESS;
}

int AESEncryptDecrypt::overlapFillBufferUnmap1(unsigned int *this_stream_num, unsigned int *this_buffer_size, cl_event *mapEvent)
{
	cl_int status;
    cl_event event;
	CPerfCounter t;

	// wait for previous map to complete
	status = clWaitForEvents(1, mapEvent);
    CHECK_OPENCL_ERROR(status, "clWaitForEvents() failed.");


	t.Reset();
	t.Start();
	// Fill the buffers
	*this_buffer_size = streamGenerator.GetStreams(input1, buffer_size, 
			keys1, ivs1, pkt_offset1, this_stream_num);

	t.Stop();
	timeLog->Timer(
           "%s %f ms\n", " 1 -- Get Streams Time",
           t.GetTotalTime(),
           10,
           1);
	timeLog->Msg("%s %d streams\n", "     -- This time we have ", *this_stream_num);
	timeLog->Msg("%s %d bytes\n", "     -- Buffer size is ", *this_buffer_size);

	if (*this_stream_num > stream_num) {
		std::cout << "What's the problem!!!!" << std::endl;
		exit(0);
	}

	t.Reset();
	t.Start();
	// Unmap all of them
	status = clEnqueueUnmapMemObject(
                commandQueue,
                inputBuffer1,
                (void *) input1,
                0,
                NULL,
                &event);
    CHECK_OPENCL_ERROR(status, "clEnqueueUnmapMemObject(inputBuffer1) failed.");

	status = clEnqueueUnmapMemObject(
                commandQueue,
                pktOffsetBuffer1,
                (void *) pkt_offset1,
                0,
                NULL,
                &event);
    CHECK_OPENCL_ERROR(status, "clEnqueueUnmapMemObject(pktOffsetBuffer1) failed.");

	status = clEnqueueUnmapMemObject(
                commandQueue,
                keyBuffer1,
                (void *) keys1,
                0,
                NULL,
                &event);
    CHECK_OPENCL_ERROR(status, "clEnqueueUnmapMemObject(keyBuffer1) failed.");

	status = clEnqueueUnmapMemObject(
                commandQueue,
                ivsBuffer1,
                (void *) ivs1,
                0,
                NULL,
                &event);
    CHECK_OPENCL_ERROR(status, "clEnqueueUnmapMemObject(ivsBuffer1) failed.");
 
    status = clWaitForEvents(1, &event);
    CHECK_OPENCL_ERROR(status, "clWaitForEvents() failed.");

	 // Release event
    status = clReleaseEvent(event);
    CHECK_OPENCL_ERROR(status, "clReleaseEvent(event) failed.");

	t.Stop();
	timeLog->Timer(
           "%s %f ms\n", " 1 -- Unmap Memory Time",
           t.GetTotalTime(),
           10,
           1);

	return SDK_SUCCESS;
}

int AESEncryptDecrypt::overlapLaunchKernel0(unsigned int this_stream_num)
{
	cl_int status;
	CPerfCounter t;

	size_t globalThreads;
	size_t localThreads = 256; // 64 * 4, maximum

	t.Reset();
	t.Start();
	//!!!!!!!!!!!!!!!!!!! FIXME: globalThreads cannot be greater than this_stream_num!!!!!!!!!!!!!!!!!!!
	// Get the reasonable global thread number
	if (this_stream_num / localThreads > 1) {
		if (this_stream_num % localThreads == 0) { // just equal
			globalThreads = this_stream_num;
		} else { // pad globalThreads to be a multiple of localThreads
			globalThreads = (this_stream_num / localThreads) * localThreads;
		} // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!why cannot +1 ? more than this_stream_num
	} else {
		globalThreads = localThreads;
	}

	if (this_stream_num < 256) 
			globalThreads = localThreads = this_stream_num;

	timeLog->Msg("%s %d\n", " 0 -- global Threads:", globalThreads);

	status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&outputBuffer);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (outputBuffer)");

	status = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&inputBuffer);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (inputBuffer)");

	status = clSetKernelArg(kernel, 2, sizeof(cl_uint), (void *)&this_stream_num);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (stream_num)");

	status = clSetKernelArg(kernel, 3, sizeof(cl_uint), (void *)&rounds);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (rounds)");

	status = clSetKernelArg(kernel, 4, sizeof(cl_mem), (void *)&pktOffsetBuffer);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (pktOffsetBuffer)");

	status = clSetKernelArg(kernel, 5, sizeof(cl_mem), (void *)&keyBuffer);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (keyBuffer)");

	status = clSetKernelArg(kernel, 6, sizeof(cl_mem), (void *)&ivsBuffer);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (ivsBuffer)");


	//Enqueue a kernel run call.
	cl_event ndrEvt;
	status = clEnqueueNDRangeKernel(
			commandQueue,
			kernel,
			1,
			NULL,
			&globalThreads,
			&localThreads,
			0,
			NULL,
			&ndrEvt);
	CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");

	status = clFlush(commandQueue);
	CHECK_OPENCL_ERROR(status, "clFlush() failed.");

	t.Stop();
	timeLog->Timer(
           "%s %f ms\n", " 0 -- Kernel launch Time",
           t.GetTotalTime(),
           10,
           1);

	return SDK_SUCCESS;
}

int AESEncryptDecrypt::overlapLaunchKernel1(unsigned int this_stream_num)
{
	cl_int status;
	CPerfCounter t;

	size_t globalThreads;
	size_t localThreads = 256; // 64 * 4, maximum

	t.Reset();
	t.Start();
	//!!!!!!!!!!!!!!!!!!! FIXME: globalThreads cannot be greater than this_stream_num!!!!!!!!!!!!!!!!!!!
	// Get the reasonable global thread number
	if (this_stream_num / localThreads > 1) {
		if (this_stream_num % localThreads == 0) { // just equal
			globalThreads = this_stream_num;
		} else { // pad globalThreads to be a multiple of localThreads
			globalThreads = (this_stream_num / localThreads) * localThreads;
		} // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!why cannot +1 ? more than this_stream_num
	} else {
		globalThreads = localThreads;
	}

	if (this_stream_num < 256) 
			globalThreads = localThreads = this_stream_num;

	timeLog->Msg("%s %d\n", " 1 -- global Threads:", globalThreads);

	status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&outputBuffer1);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (outputBuffer)");

	status = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&inputBuffer1);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (inputBuffer)");

	status = clSetKernelArg(kernel, 2, sizeof(cl_uint), (void *)&this_stream_num);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (stream_num)");

	status = clSetKernelArg(kernel, 3, sizeof(cl_uint), (void *)&rounds);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (rounds)");

	status = clSetKernelArg(kernel, 4, sizeof(cl_mem), (void *)&pktOffsetBuffer1);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (pktOffsetBuffer)");

	status = clSetKernelArg(kernel, 5, sizeof(cl_mem), (void *)&keyBuffer1);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (keyBuffer)");

	status = clSetKernelArg(kernel, 6, sizeof(cl_mem), (void *)&ivsBuffer1);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (ivsBuffer)");


	//Enqueue a kernel run call.
	cl_event ndrEvt;
	status = clEnqueueNDRangeKernel(
			commandQueue,
			kernel,
			1,
			NULL,
			&globalThreads,
			&localThreads,
			0,
			NULL,
			&ndrEvt);
	CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");

	status = clFlush(commandQueue);
	CHECK_OPENCL_ERROR(status, "clFlush() failed.");

	t.Stop();
	timeLog->Timer(
           "%s %f ms\n", " 1 -- Kernel launch Time",
           t.GetTotalTime(),
           10,
           1);

	return SDK_SUCCESS;
}

int AESEncryptDecrypt::overlapOutput(unsigned int this_buffer_size, cl_mem resultBuffer, int id)
{
	cl_int status;
	CPerfCounter t;
	void *ptrResult;
	cl_event event;

    t.Reset(); 
    t.Start();

    ptrResult = (void*)clEnqueueMapBuffer(
                    commandQueue,
                    resultBuffer,
                    CL_TRUE,
                    CL_MAP_READ,
                    0,
                    this_buffer_size,
                    0,
                    NULL,
                    NULL,
                    &status);
    CHECK_OPENCL_ERROR(status, "clEnqueueMapBuffer() failed.(resultBuffer)");

	/*verify or transfer to upper layer applications*/


	status = clEnqueueUnmapMemObject(commandQueue, resultBuffer, (void *) ptrResult, 0, NULL, &event);
    CHECK_OPENCL_ERROR(status, "clEnqueueUnmapMemObject() failed.(resultBuffer)");
 
    status = clWaitForEvents(1, &event);
    CHECK_OPENCL_ERROR(status, "clWaitForEvents()");

    t.Stop();
	if (id == 0) {
		timeLog->Timer(
			   "%s %f ms\n", " 0 -- Output buffer read time",
			   t.GetTotalTime(),
			   10,
			   1);
	} else {
		timeLog->Timer(
			   "%s %f ms\n", " 1 -- Output buffer read time",
			   t.GetTotalTime(),
			   10,
			   1);
	}

	return SDK_SUCCESS;
}


int 
AESEncryptDecrypt::runCLKernels(void)
{
	cl_int   status;
	cl_int eventStatus = CL_QUEUED;

	// 64 is wavefront size
	// localThreads is to specify the work-group size, 
	//   which is better to be a multiple of wavefront
	// This is also the maximum localThreads number
	size_t localThreads = 256; // 64 * 4
	

//	std::cout << "Dimension : " << globalThreads << " " << localThreads << std::endl;

	status = kernelInfo.setKernelWorkGroupInfo(kernel, devices[deviceId]);
	CHECK_ERROR(status, SDK_SUCCESS, "KernelInfo.setKernelWorkGroupInfo() failed");

	availableLocalMemory = deviceInfo.localMemSize - kernelInfo.localMemoryUsed; 

	
	if((cl_uint)(localThreads) > kernelInfo.kernelWorkGroupSize )
	{
		std::cout << "!!! Work group size : " << kernelInfo.kernelWorkGroupSize
			<< " localThreads : " << localThreads << std::endl;
		localThreads = kernelInfo.kernelWorkGroupSize / 4;
	}

	if(localThreads > deviceInfo.maxWorkItemSizes[0] ||
		localThreads > deviceInfo.maxWorkGroupSize)
	{
		std::cout << "Unsupported: Device does not support requested number of work items."<<std::endl;
		return SDK_SUCCESS;
	}
	
 
    std::cout << "kernelWorkGroupSize : " << kernelInfo.kernelWorkGroupSize
        << ", maxWorkItemSizes[0] : " << deviceInfo.maxWorkItemSizes[0]
        << ", maxWorkGroupSize : " << deviceInfo.maxWorkGroupSize
		<< ", maxComputeUnits £º" << deviceInfo.maxComputeUnits
		<< ", maxMemAllocSize : " << deviceInfo.maxMemAllocSize << std::endl;


	cl_event map_event_0, map_event_1;
	unsigned int stream_num_0, stream_num_1;
	unsigned int buffer_size_0, buffer_size_1;
	unsigned int bytes = 0, first;
	bool buffer1_launched = false;

	CPerfCounter t, counter, loopcounter;
	double elapsed_time;
	int interval_count = 1;
	double time_point = INTERVAL;

	std::cout << "Stream num : " << stream_num
		<< ", Start time : " << streamGenerator.GetStartTimestamp()
		<< ", Interval : " << streamGenerator.GetInterval() << std::endl;

	streamGenerator.StartStreams();

	loopcounter.Reset();
	loopcounter.Start();

	/* 0. Map buffer 0 at first time before loop */
	overlapMapBuffer0(&map_event_0);

	for (int i = 0; i < iterations; i ++) {

		timeLog->loopMarker();
		timeLog->Msg( "\n%s %d loop\n", "---------------------------", i);
		

		/* 1. Fills buffer *0*, unless the first loop, this overlap with kernel 1*/
		overlapFillBufferUnmap0(&stream_num_0, &buffer_size_0, &map_event_0);

		if(i != 0)	
			bytes += buffer_size_0;

		/* -------------------------------------------------------------
		  This is a CPU/GPU synchronization point, as all commands in the
        in-order queue before the preceding cl*Unmap() are now finished.
        We can accurately sample the per-loop timer here.
		loopcounter.Stop();
		loopcounter.GetElapsedTime();
		timeLog->Timer(
			   "%s %f ms\n", " >>>>>> Loop time elapsed",
			   loopcounter.GetTotalTime(),
			   10,
			   1);
		loopcounter.Reset();
		loopcounter.Start();
		------------------------------------------------------------- */
		first = 1;
		do {
			elapsed_time = loopcounter.GetElapsedTime();
			if (first) {
				timeLog->Msg( "\n%s %d\n", "<<<<<<<<Elapsed Time : ", elapsed_time);
				first = 0;
			}

			if (elapsed_time - time_point > 1) { // surpassed the time point more than 1 ms
				//std::cout << "Timepoint Lost! " << elapsed_time << "/" << time_point << std::endl;
				timeLog->Msg( "\n%s %d\n", ">>>>>>>>Time point lost!!!! : ", elapsed_time);
				break;
			}
		} while(abs(elapsed_time - time_point) > 1);
		//std::cout << elapsed_time << "  " << time_point << std::endl;
		timeLog->Msg( "%s %d\n", ">>>>>>>>Time point arrived : ", elapsed_time);
		loopcounter.Reset();
		loopcounter.Start();

		// Start the timer of the algorithm
		if (i == 1)	{
			counter.Reset();
			counter.Start();
		}

		/* 2. Map buffer *1*, The map needs to precede
        the next kernel launch in the in-order queue, otherwise waiting
        for the map to finish would also wait for the kernel to finish.*/
		overlapMapBuffer1(&map_event_1);

		// The output buffer 1
		//if (buffer1_launched == true) // if not the first time
		//	overlapOutput(buffer_size_1, outputBuffer1,1);

		/* 3. Asynchronous launch of kernel for buffer *0* */
		overlapLaunchKernel0(stream_num_0);

		/* 4. Fill buffer *1*, Overlap with kernel 0 */
		overlapFillBufferUnmap1(&stream_num_1, &buffer_size_1, &map_event_1);
		if(i != 0)
			bytes += buffer_size_1;

		/* -------------------------------------------------------------
		  This is a CPU/GPU synchronization point, as all commands in the
        in-order queue before the preceding cl*Unmap() are now finished.
        We can accurately sample the per-loop timer here.
		------------------------------------------------------------- */
		first = 1;
		do {
			elapsed_time = loopcounter.GetElapsedTime();
			if (first) {
				timeLog->Msg( "\n%s %d\n", "<<<<<<<<Elapsed Time : ", elapsed_time);
				first = 0;
			}

			if (elapsed_time - time_point > 1) { // surpassed the time point more than 1 ms
				//std::cout << "Timepoint Lost! " << elapsed_time << "/" << time_point << std::endl;
				timeLog->Msg( "\n%s %d\n", ">>>>>>>>Time point lost!!!! : ", elapsed_time);
				break;
			}
		} while(abs(elapsed_time - time_point) > 1);
		//std::cout << elapsed_time << "  " << time_point << std::endl;
		timeLog->Msg( "%s %d\n", ">>>>>>>>Time point arrived : ", elapsed_time);
		loopcounter.Reset();
		loopcounter.Start();


		/* 5. Map buffer *0*, The map needs to precede
        the next kernel launch in the in-order queue, otherwise waiting
        for the map to finish would also wait for the kernel to finish.*/
		overlapMapBuffer0(&map_event_0);

		// The output buffer 0
		//overlapOutput(buffer_size_0, outputBuffer,0);

		/* 6. Asynchronous launch of kernel for buffer *1* */
		overlapLaunchKernel1(stream_num_1);
		buffer1_launched = true;
	}

	status = clFinish(commandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush() failed.");
    
    status = clReleaseEvent(map_event_0);
    CHECK_OPENCL_ERROR(status, "clReleaseEvent() failed.");

	counter.Stop();

	std::cout << "End of execution, now the program costs : " << counter.GetTotalTime() << " ms" 
		<< ", bits : " << bytes * 8 << std::endl;
	std::cout << "Processing speed is " << ((bytes * 8) / 1e3) / counter.GetTotalTime() << " Mbps" << std::endl;

	return SDK_SUCCESS;
}

int 
AESEncryptDecrypt::initialize()
{
	// Call base class Initialize to get default configuration
	if(this->SDKSample::initialize())
		return SDK_FAILURE;

	streamsdk::Option* decrypt_opt = new streamsdk::Option;
	CHECK_ALLOCATION(decrypt_opt, "Memory allocation error.\n"); 

	decrypt_opt->_sVersion = "z";
	decrypt_opt->_lVersion = "decrypt";
	decrypt_opt->_description = "Decrypt the Input Image"; 
	decrypt_opt->_type     = streamsdk::CA_NO_ARGUMENT;
	decrypt_opt->_value    = &decrypt;
	sampleArgs->AddOption(decrypt_opt);

	delete decrypt_opt;

	streamsdk::Option* num_iterations = new streamsdk::Option;
	CHECK_ALLOCATION(num_iterations, "Memory allocation error.\n");

	num_iterations->_sVersion = "i";
	num_iterations->_lVersion = "iterations";
	num_iterations->_description = "Number of iterations for kernel execution";
	num_iterations->_type = streamsdk::CA_ARG_INT;
	num_iterations->_value = &iterations;

	sampleArgs->AddOption(num_iterations);

	delete num_iterations;

	return SDK_SUCCESS;
}

int 
AESEncryptDecrypt::setup()
{
	if(setupAESEncryptDecrypt() != SDK_SUCCESS)
		return SDK_FAILURE;
    
	int timer = sampleCommon->createTimer();
	sampleCommon->resetTimer(timer);
	sampleCommon->startTimer(timer);

	if(setupCL()!= SDK_SUCCESS)
		return SDK_FAILURE;

	sampleCommon->stopTimer(timer);

	setupTime = (double)(sampleCommon->readTimer(timer));

	return SDK_SUCCESS;
}


int 
AESEncryptDecrypt::run()
{
	std::cout << "Executing kernel for " << iterations << 
		" iterations" << std::endl;
	std::cout << "-------------------------------------------" << std::endl;

	int timer = sampleCommon->createTimer();
	sampleCommon->resetTimer(timer);
	sampleCommon->startTimer(timer);

	if(runCLKernels() != SDK_SUCCESS)
		return SDK_FAILURE;

	sampleCommon->stopTimer(timer);
	totalKernelTime = (double)(sampleCommon->readTimer(timer)) / iterations;
    
	return SDK_SUCCESS;
}


void AESEncryptDecrypt::printStats()
{
	totalTime = setupTime + totalKernelTime;

	timeLog->printLog();

	std::cout << "\n-------------------------------------------------\n";

}

int AESEncryptDecrypt::cleanup()
{
	// Releases OpenCL resources (Context, Memory etc.)
	cl_int status;

	status = clReleaseKernel(kernel);
	CHECK_OPENCL_ERROR(status, "clReleaseKernel failed.");

	status = clReleaseProgram(program);
	CHECK_OPENCL_ERROR(status, "clReleaseProgram failed.");


	// TRANSFER_OVERLAP ---- Buffer set 0 ----
	status = clReleaseMemObject(inputBuffer);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.");

	status = clReleaseMemObject(outputBuffer);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.");

	status = clReleaseMemObject(keyBuffer);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.");

	status = clReleaseMemObject(ivsBuffer);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.");

	status = clReleaseMemObject(pktOffsetBuffer);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.");


	// TRANSFER_OVERLAP ---- Buffer set 1 ----
	status = clReleaseMemObject(inputBuffer1);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.");

	status = clReleaseMemObject(outputBuffer1);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.");

	status = clReleaseMemObject(keyBuffer1);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.");

	status = clReleaseMemObject(ivsBuffer1);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.");

	status = clReleaseMemObject(pktOffsetBuffer1);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.");


	status = clReleaseCommandQueue(commandQueue);
	CHECK_OPENCL_ERROR(status, "clReleaseCommandQueue failed.");

	status = clReleaseContext(context);
	CHECK_OPENCL_ERROR(status, "clReleaseContext failed.");


	FREE(devices);

	delete timeLog;

	return SDK_SUCCESS;
}

int main(int argc, char * argv[])
{
	AESEncryptDecrypt clAESEncryptDecrypt("OpenCL AES Encrypt Decrypt");

	if(clAESEncryptDecrypt.initialize() != SDK_SUCCESS)
		return SDK_FAILURE;
	if(clAESEncryptDecrypt.parseCommandLine(argc, argv))
		return SDK_FAILURE;

	if(clAESEncryptDecrypt.isDumpBinaryEnabled())
	{
		return clAESEncryptDecrypt.genBinaryImage();
	}
    
	if(clAESEncryptDecrypt.setup() != SDK_SUCCESS)
		return SDK_FAILURE;
	if(clAESEncryptDecrypt.run() != SDK_SUCCESS)
		return SDK_FAILURE;
	clAESEncryptDecrypt.printStats();
	if(clAESEncryptDecrypt.cleanup() != SDK_SUCCESS)
		return SDK_FAILURE;

	return SDK_SUCCESS;
}


#endif // TRANSFER_OVERLAP