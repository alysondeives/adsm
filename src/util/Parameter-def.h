// GMAC global settings
PARAM(ParamProtocol, const char *, "Rolling", "GMAC_PROTOCOL")
PARAM(ParamAllocator, const char *, "Slab", "GMAC_ALLOCATOR")
//PARAM(ParamAcquireOnWrite, bool, false, "GMAC_ACQUIRE_ON_WRITE")
PARAM(ParamIOMemory, long_t, 16 * 1024 * 1024, "GMAC_IOMEMORY")
PARAM(ParamAutoSync, bool, false, "GMAC_AUTOSYNC")

// GMAC debug settings
PARAM(ParamDebug, const char *, "none", "GMAC_DEBUG")
//PARAM(ParamDebugFile, const char *, NULL, "GMAC_DEBUG_FILE")

// GMAC tracing
PARAM(ParamTrace, const char *, "trace", "GMAC_TRACE")

// GMAC Page table settings
PARAM(ParamBlockSize, long_t, 2 * 1024 * 1024, "GMAC_BLOCK_SIZE", PARAM_NONZERO)

// Rolling Manager specific settings
PARAM(ParamRollSize, long_t, 2, "GMAC_ROLL_SIZE", PARAM_NONZERO)

// Miscelaneous Parameters
PARAM(configPrintParams, bool, false, "GMAC_PRINT_PARAMS")

// OpenCL Parameters
PARAM(ParamOpenCLSources, const char *, "", "GMAC_OPENCL_SOURCES")
PARAM(ParamOpenCLFlags,   const char *, "", "GMAC_OPENCL_FLAGS")

// Bitmap Parameters
PARAM(ParamGatherRatio, float, 0.2f, "GMAC_GATHER_RATIO")
PARAM(ParamSubBlocks, unsigned, 512, "GMAC_SUBBLOCKS", PARAM_NONZERO)
PARAM(ParamBitmapLevels, unsigned, 3, "GMAC_BITMAP_LEVELS", PARAM_NONZERO)
PARAM(ParamBitmapL1Entries, unsigned, 512, "GMAC_BITMAP_L1ENTRIES", PARAM_NONZERO)
PARAM(ParamBitmapL2Entries, unsigned, 128, "GMAC_BITMAP_L2ENTRIES", PARAM_NONZERO)
PARAM(ParamBitmapL3Entries, unsigned, 4096, "GMAC_BITMAP_L3ENTRIES", PARAM_NONZERO)

// GMAC Parameters for auto-tunning
PARAM(ParamModelToHostConfig, float, 40.0, "GMAC_MODEL_TOHOSTCONFIG")       // DMA configuration costs
PARAM(ParamModelToHostTransferL1, float, 0.0007f, "GMAC_MODEL_TOHOSTTRANSFER_L1") // Transfer costs for data that fits in the L1 cache
PARAM(ParamModelToHostTransferL2, float, 0.0008f, "GMAC_MODEL_TOHOSTTRANSFER_L2") // Transfer costs for data that fits in the L2 cache
PARAM(ParamModelToHostTransferMem, float, 0.0010f, "GMAC_MODEL_TOHOSTTRANSFER_L2") // Transfer costs for data that does not fit in the L2 cache
PARAM(ParamModelToDeviceConfig, float, 40.0, "GMAC_MODEL_TODEVICECONFIG")       // DMA configuration costs
PARAM(ParamModelToDeviceTransferL1, float, 0.0007f, "GMAC_MODEL_TODEVICETRANSFER_L1") // Transfer costs for data that fits in the L1 cache
PARAM(ParamModelToDeviceTransferL2, float, 0.0008f, "GMAC_MODEL_TODEVICETRANSFER_L2") // Transfer costs for data that fits in the L2 cache
PARAM(ParamModelToDeviceTransferMem, float, 0.0010f, "GMAC_MODEL_TODEVICETRANSFER_L2") // Transfer costs for data that does not fit in the L2 cache
PARAM(ParamModelL1, long_t, 32 * 1024, "GMAC_MODEL_L1") // Size of the L1 cache
PARAM(ParamModelL2, long_t, 256 * 1024, "GMAC_MODEL_L2") // Size of the L2 cache
