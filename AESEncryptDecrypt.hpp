#ifndef AESENCRYPTDECRYPT_H_
#define AESENCRYPTDECRYPT_H_

#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <SDKCommon.hpp>
#include <SDKApplication.hpp>
#include <SDKCommandArgs.hpp>
#include <SDKFile.hpp>
#include <SDKBitMap.hpp>

#include "StreamGenerator.hpp"
#include "Log.h"

using namespace streamsdk;

#define NUM_FLOWS 1024
#define FLOW_LEN 256
#define ITERATION 10
#define EORD false  // false->encryption, true->decryption
#define DEVICE_ID 0 //0->integrated GPU, 1->discrete GPU

#define STREAM_NUM 2048
#define INTERVAL 50 //ms

#define AVG_DEADLINE 120 //ms
#define VAR_DEADLINE 5
#define AVG_RATE	50000
#define VAR_RATE	10
#define AVG_PERIOD	120
#define VAR_PERIOD	5


/**
 * AESEncryptDecrypt 
 * Class implements OpenCL AESEncryptDecrypt sample
 * Derived from SDKSample base class
 */

#define AES_IV_SIZE 16
#define AES_BLOCK_SIZE 16

namespace AES
{
    class AESEncryptDecrypt : public SDKSample
    {
        cl_uint                     seed;/**< Seed value for random number generation */
        cl_double              setupTime;/**< Time for opencl setup */
        cl_double        totalKernelTime;/**< Time for kernel execution */
        cl_double       totalProgramTime;/**< Time for program execution */
        cl_double    referenceKernelTime;/**< Time for reference implementation */
        cl_uchar      *input;            /**< Input array */
        cl_uchar      *output;           /**< Output array */
        cl_uchar      *keys;              /**< Encryption Key */
        cl_uchar      *ivs;              /**< Initial data */
        cl_uint	      *pkt_offset;       /**< Packet Offset */
        cl_uchar      *verificationOutput;/**< Output array for reference implementation */
		
        cl_mem       inputBuffer;        /**< CL memory input buffer */
        cl_mem       outputBuffer;       /**< CL memory output buffer */
        cl_mem       pktOffsetBuffer; 
		cl_mem       pktIndexBuffer; /* For networking decryption*/
        cl_mem       keyBuffer;
        cl_mem       ivsBuffer;
		
        cl_context   context;            /**< CL context */
        cl_device_id *devices;           /**< CL device list */		
        cl_command_queue commandQueue;   /**< CL command queue */
        cl_program   program;            /**< CL program  */
        cl_kernel    kernel;             /**< CL kernel */
        cl_bool      decrypt;
         
        cl_uint      keySizeBits;
        cl_uint      keySize;
        cl_uint      rounds;

        cl_ulong availableLocalMemory; 
        cl_ulong    neededLocalMemory;
        int         iterations;    /**< Number of iterations for kernel execution */
        streamsdk::SDKDeviceInfo deviceInfo;						/**< Structure to store device information*/
	    streamsdk::KernelWorkGroupInfo kernelInfo;			/**< Structure to store kernel related info */   

		/* For network processing */
        cl_uint 	num_flows;	/* Number of flows for processing */
        cl_uint		flow_len;	/* The length of each flow, 16KB maximum */

		cl_uint	    block_count; /* for network processing decryption, how many 16byte blocks */
		cl_uint     *pkt_index; /* for network processing decryption, marking which packet this block belongs to*/

		StreamGenerator streamGenerator;
		unsigned int buffer_size;
		unsigned int stream_num;

		bool noOverlap;      // Disallow memset/kernel overlap
		int  numWavefronts;

		TestLog *timeLog;

        public:
        /** 
         * Constructor 
         * Initialize member variables
         * @param name name of sample (string)
         */
        AESEncryptDecrypt(std::string name)
            : SDKSample(name)   {
                seed = 123;
                input  = NULL;
                output = NULL;
                keys    = NULL;
                ivs    = NULL;
                pkt_offset = NULL;
				pkt_index = NULL;
                verificationOutput = NULL;
                decrypt = EORD;
                keySizeBits = 128;
                rounds = 10;
                setupTime = 0;
                totalKernelTime = 0;
                iterations = ITERATION;
				num_flows = NUM_FLOWS; //max is 16384 for global threads?
				flow_len = FLOW_LEN;
            }
    
        /** 
         * Constructor 
         * Initialize member variables
         * @param name name of sample (const char*)
         */
        AESEncryptDecrypt(const char* name)
            : SDKSample(name)   {
                seed = 123;
                input  = NULL;
                output = NULL;
                keys    = NULL;
                ivs    = NULL;
                pkt_offset = NULL;
				pkt_index = NULL;
                verificationOutput = NULL;
                decrypt = EORD;
                keySizeBits = 128;
                rounds = 10;
                setupTime = 0;
                totalKernelTime = 0;
                iterations = ITERATION;
				num_flows = NUM_FLOWS;
				flow_len = FLOW_LEN;
            }
    
		/* Generate Input Data */
        void write_pkt_offset(cl_uint *pkt_offset);
        void set_random(cl_uchar *input, int len);

		int setupDecryption();
		int setupEncryption();
	
        /**
         * Allocate and initialize host memory array with random values
         * @return SDK_SUCCESS on success and SDK_FAILURE on failure
         */
        int setupAESEncryptDecrypt();
    
        /**
         * OpenCL related initialisations. 
         * Set up Context, Device list, Command Queue, Memory buffers
         * Build CL kernel program executable
         * @return SDK_SUCCESS on success and SDK_FAILURE on failure
         */
        int setupCL();
    
        /**
         * Set values for kernels' arguments, enqueue calls to the kernels
         * on to the command queue, wait till end of kernel execution.
         * Get kernel start and end time if timing is enabled
         * @return SDK_SUCCESS on success and SDK_FAILURE on failure
         */
        int runCLKernels();
    
        /**
         * Override from SDKSample. Print sample stats.
         */
        void printStats();
    
        /**
         * Override from SDKSample. Initialize 
         * command line parser, add custom options
         * @return SDK_SUCCESS on success and SDK_FAILURE on failure
         */
        int initialize();
    
        /**
         * Override from SDKSample, Generate binary image of given kernel 
         * and exit application
         * @return SDK_SUCCESS on success and SDK_FAILURE on failure
         */
        int genBinaryImage();

        /**
         * Override from SDKSample, adjust width and height 
         * of execution domain, perform all sample setup
         * @return SDK_SUCCESS on success and SDK_FAILURE on failure
         */
        int setup();
    
        /**
         * Override from SDKSample
         * Run OpenCL Bitonic Sort
         * @return SDK_SUCCESS on success and SDK_FAILURE on failure
         */
        int run();
    
        /**
         * Override from SDKSample
         * Cleanup memory allocations
         * @return SDK_SUCCESS on success and SDK_FAILURE on failure
         */
        int cleanup();
    
        /**
         * Override from SDKSample
         * Verify against reference implementation
         * @return SDK_SUCCESS on success and SDK_FAILURE on failure
         */
        int verifyResults();
    };
}//namespace AES

#endif

