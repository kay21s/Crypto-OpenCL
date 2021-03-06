#include "AESEncryptDecrypt.hpp"

//#if defined(STREAM_DYNAMIC)

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
	// !!!!!! Set deadline = period

	/* 1 Byte = 8 bits, 128bits => 16bytes */
	keySize = keySizeBits/8;
  
	/* due to unknown represenation of cl_uchar */ 
	keySizeBits = keySize*sizeof(cl_uchar); 

	buffer_size = streamGenerator.GetMaxBufferSize();
	// round up to 256, or it won't work in Linux
	buffer_size = buffer_size & (~0xFF) | 0x100;
	std::cout << ">>>>>>>>>>.Max buffer size is : " << buffer_size << std::endl;
	stream_num = streamGenerator.GetStreamNumber();

	input = (cl_uchar *)malloc(buffer_size * sizeof(cl_uchar));
	CHECK_ALLOCATION(input, "Failed to allocate host memory. (input)");

	memset(input, 'a', buffer_size * sizeof(cl_uchar));

	output = (cl_uchar *)malloc(buffer_size * sizeof(cl_uchar));
	CHECK_ALLOCATION(output, "Failed to allocate host memory. (output)");

	pkt_offset = (cl_uint *)malloc((stream_num + 1) * sizeof(cl_uint));
	CHECK_ALLOCATION(pkt_offset, "Failed to allocate host memory. (pkt_offset)");

	keys = (cl_uchar *)malloc(stream_num * keySize);
	CHECK_ALLOCATION(keys, "Failed to allocate host memory. (keys)");

	ivs = (cl_uchar *)malloc(stream_num * AES_IV_SIZE);
	CHECK_ALLOCATION(ivs, "Failed to allocate host memory. (ivs)");

	return SDK_SUCCESS;
}

int 
AESEncryptDecrypt::genBinaryImage()
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

int
AESEncryptDecrypt::setupEncryption(void)
{
	cl_int status = 0;
	// Set Presistent memory only for AMD platform
	cl_mem_flags inMemFlags = CL_MEM_READ_ONLY;
	if(isAmdPlatform())
		inMemFlags |= CL_MEM_USE_PERSISTENT_MEM_AMD;

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
					CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
					sizeof(cl_uint) * (stream_num + 1),
					pkt_offset,
					&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (pktOffsetBuffer)");

	keyBuffer = clCreateBuffer(
					context,
					CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
					keySize * stream_num,
					keys,
					&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (keyBuffer)");

	ivsBuffer = clCreateBuffer(
					context,
					CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
					AES_IV_SIZE * stream_num,
					ivs,
					&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (ivsBuffer)");

	return SDK_SUCCESS;
}

int
AESEncryptDecrypt::setupCL(void)
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
		dType = CL_DEVICE_TYPE_CPU;

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
	kernel = clCreateKernel(program, "AES_cbc_128_encrypt", &status);
	CHECK_OPENCL_ERROR(status, "clCreateKernel failed.");
	setupEncryption();

	return SDK_SUCCESS;
}


int 
AESEncryptDecrypt::runCLKernels(void)
{
	cl_int   status;
	cl_int eventStatus = CL_QUEUED;
	size_t globalThreads;
    
	// 64 is wavefront size
	// localThreads is to specify the work-group size, 
	//   which is better to be a multiple of wavefront
	size_t localThreads = 256; // 64 * 4
	

	std::cout << "Dimension : " << globalThreads << " " << localThreads << std::endl;

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
		<< ", maxComputeUnits : " << deviceInfo.maxComputeUnits
		<< ", maxMemAllocSize : " << deviceInfo.maxMemAllocSize << std::endl;


	unsigned int this_stream_num;
	unsigned int this_buffer_size = 0;
	unsigned int bytes = 0, first;
	double time_point = INTERVAL, elapsed_time;

	cl_event writeEvt;
	CPerfCounter t, counter, loopcounter;

	std::cout << "Stream num : " << stream_num
		<< ", Start time : " << streamGenerator.GetStartTimestamp()
		<< ", Interval : " << streamGenerator.GetInterval() << std::endl;

	// start streams
	streamGenerator.StartStreams();

	// Counter for each kernel launch
	loopcounter.Reset();
	loopcounter.Start();

	for (int i = 0; i < iterations; i ++) {

		timeLog->loopMarker();

		t.Reset();
		t.Start();

		// Counter for the whole loop
		// From the second loop
		if (i == 2) {
			counter.Reset();
			counter.Start();
		}


		// Generate Stream Data
		this_buffer_size = streamGenerator.GetStreams(input, buffer_size, 
			keys, ivs, pkt_offset, &this_stream_num);
		
		// Record the bytes processed for calculating the speed
		// From the second loop
		if (i > 1)
			bytes += this_buffer_size;

		if (this_stream_num > stream_num || this_stream_num < 256) {
			std::cout << "What's the problem!!!!" << std::endl;
			continue;
			//exit(0);
		}

		t.Stop();

		timeLog->Msg( "\n%s\n", "---------------------------", 0);

		timeLog->Timer(
			"%s %f ms\n", "Get Streams Time",
			t.GetTotalTime(),
			10,
			1);

		//timeLog->Msg("%s %d ms\n", " Time after GetStreams is " , counter.GetElapsedTime());
		//timeLog->Msg("%s %d ms\n", " Time ought to be ", INTERVAL * (i + 1));
		timeLog->Msg("%s %d streams\n", " This time we have ", this_stream_num);
		timeLog->Msg("%s %d byte\n", " This buffer size is ", this_buffer_size);


		//!!!!!!!!!!!!!!!!!!! BUG HERE: globalThreads cannot be greater than this_stream_num!!!!!!!!!!!!!!!!!!!
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

		//FIXME: 
		if (this_stream_num < 256) 
			globalThreads = localThreads = this_stream_num;

		/* -------------------------------------------------------------
		  This is a CPU/GPU synchronization point, as all commands in the
		in-order queue before the preceding cl*Unmap() are now finished.
		We can accurately sample the per-loop timer here.
		*/
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

		
		/* ------------------------------------------------------------- */




		timeLog->Msg("%s %d\n", "global Threads:", globalThreads);
		timeLog->Msg("%s %d\n", "this_stream_number:", this_stream_num);
//		std::cout<<"Global threads: " << globalThreads 
//			<< "\n Local Thread: "<< localThreads 
//			<< "\n this_stream_num�� " << this_stream_num << std::endl;

		t.Reset();
		t.Start();

		// Write buffers
		status = clEnqueueWriteBuffer(
				commandQueue,
				inputBuffer,
				CL_FALSE,
				0,
				sizeof(cl_uchar) * this_buffer_size,
				input,
				0,
				NULL,
				&writeEvt);
		CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (inputBuffer)");

		status = clEnqueueWriteBuffer(
				commandQueue,
				pktOffsetBuffer,
				CL_FALSE,
				0,
				sizeof(cl_uint) * (this_stream_num + 1),
				pkt_offset,
				0,
				NULL,
				&writeEvt);
		CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (pktOffsetBuffer)");

		status = clEnqueueWriteBuffer(
				commandQueue,
				keyBuffer,
				CL_FALSE,
				0,
				keySize * this_stream_num,
				keys,
				0,
				NULL,
				&writeEvt);
		CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (keyBuffer)");

		status = clEnqueueWriteBuffer(
				commandQueue,
				ivsBuffer,
				CL_TRUE, // !!! Measure the Data transfer time, the last one BLOCKING. 
				0,
				AES_IV_SIZE * this_stream_num,
				ivs,
				0,
				NULL,
				&writeEvt);
		CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (ivsBuffer)");

		status = clFinish(commandQueue);
		CHECK_OPENCL_ERROR(status, "clFinish failed.");

		t.Stop();

		timeLog->Timer(
			"%s %f ms\n", "Input Data Transfer Time", 
			t.GetTotalTime(), 
			10,
			1);

		t.Reset();
		t.Start();

		// Set appropriate arguments to the kernel
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

		status = clFinish(commandQueue);
		CHECK_OPENCL_ERROR(status, "clFinish failed.");

		status = sampleCommon->waitForEventAndRelease(&ndrEvt);
		CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(ndrEvt) Failed");

		t.Stop();

		timeLog->Timer(
			"%s %f ms\n", "Execution Time", 
			t.GetTotalTime(), 
			10,
			1);

		t.Reset();
		t.Start();

		/* Enqueue the results to application pointer*/
		cl_event readEvt;
		status = clEnqueueReadBuffer(
					commandQueue,
					outputBuffer,
					CL_TRUE,
					0,
					sizeof(cl_uchar) * this_buffer_size,
					output,
					0,
					NULL,
					&readEvt);
		CHECK_OPENCL_ERROR(status, "clEnqueueReadBuffer failed.");

		status = clFinish(commandQueue);
		CHECK_OPENCL_ERROR(status, "clFinish failed.");

		status = sampleCommon->waitForEventAndRelease(&readEvt);
		CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(readEvt) Failed");

		t.Stop();

		timeLog->Timer(
			"%s %f ms\n", "Output Data Time",
			t.GetTotalTime(),
			10,
			1);

		timeLog->Msg( "%s %dth iteration\n", "This is", i);
		//if (i > 1)	timeLog->Msg( "%s %f ms\n", "Time after is", counter.GetElapsedTime());
	}

	counter.Stop();

	std::cout << "End of execution, now the program costs : " << counter.GetTotalTime() << " ms" << std::endl;
	std::cout << "Processing speed is " << (bytes * 8) / (1e3 * counter.GetTotalTime()) << " Mbps" << std::endl;

	uint64_t speed = (AVG_RATE/1000) * (STREAM_NUM/1000);
	std::cout << "Theoretical speed is " << speed << " Mbps" << std::endl;

	return SDK_SUCCESS;
}

int 
AESEncryptDecrypt::initialize()
{
	// Call base class Initialize to get default configuration
	if(this->SDKSample::initialize())
		return SDK_FAILURE;

	streamsdk::Option* num_iterations = new streamsdk::Option;
	CHECK_ALLOCATION(num_iterations, "Memory allocation error.\n");

	num_iterations->_sVersion = "i";
	num_iterations->_lVersion = "iterations";
	num_iterations->_description = "Number of iterations for kernel execution";
	num_iterations->_type = streamsdk::CA_ARG_INT;
	num_iterations->_value = &iterations;

	sampleArgs->AddOption(num_iterations);

	delete num_iterations;

	streamsdk::Option* show = new streamsdk::Option;
	CHECK_ALLOCATION(show, "Memory allocation error.\n");

	show->_sVersion = "l";
	show->_lVersion = "show the log";
	show->_description = "To show the log of each loop";
	show->_type = streamsdk::CA_ARG_INT;
	show->_value = &show_log;

	sampleArgs->AddOption(show);

	delete show;

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
	std::cout << "Executing kernel for " << iterations << " iterations" << std::endl;
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

int 
AESEncryptDecrypt::verifyResults()
{
	return SDK_SUCCESS;
}

void AESEncryptDecrypt::printStats()
{
	if (show_log) {
		timeLog->printLog();
	}

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
 
	status = clReleaseMemObject(inputBuffer);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.");

	status = clReleaseMemObject(outputBuffer);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.");

	status = clReleaseMemObject(keyBuffer);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.");

	status = clReleaseMemObject(ivsBuffer);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.");

	if (decrypt) {
		status = clReleaseMemObject(pktIndexBuffer);
		CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.");
	} else {
		status = clReleaseMemObject(pktOffsetBuffer);
		CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.");
	}

	status = clReleaseCommandQueue(commandQueue);
	CHECK_OPENCL_ERROR(status, "clReleaseCommandQueue failed.");

	status = clReleaseContext(context);
	CHECK_OPENCL_ERROR(status, "clReleaseContext failed.");

	// release program resources (input memory etc.)
	FREE(input);
    
	FREE(keys);
    
	if (decrypt) {
		FREE(pkt_index);
	} else {
		FREE(pkt_offset);
	}
    
	FREE(ivs);

	FREE(output);

	FREE(verificationOutput);

	FREE(devices);

	delete timeLog;

	return SDK_SUCCESS;
}

int 
main(int argc, char * argv[])
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
	//if(clAESEncryptDecrypt.verifyResults() != SDK_SUCCESS)
	//    return SDK_FAILURE;
	clAESEncryptDecrypt.printStats();
	//if(clAESEncryptDecrypt.cleanup() != SDK_SUCCESS)
	//	return SDK_FAILURE;

	return SDK_SUCCESS;
}

//#endif // STREAM_DYNAMIC
