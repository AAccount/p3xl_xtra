// The original ChatGPT attempts. xtra-hand-start is the redo of this after understanding what is going on.
#include <dlfcn.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <algorithm>

using LocClientHandle = void*;

using EventCallback =
    void (*)(LocClientHandle, std::uint32_t, const void*, void*);

using ResponseCallback =
    void (*)(LocClientHandle, std::uint32_t, const void*, std::uint32_t, void*);

using ErrorCallback =
    void (*)(LocClientHandle, std::uint32_t, void*);

struct LocClientCallbacks {
    std::uint32_t size;
    EventCallback eventIndCb;
    ResponseCallback respIndCb;
    ErrorCallback errorCb;
};

using LocClientOpenFn =
    std::uint32_t (*)(std::uint64_t, const LocClientCallbacks*,
                      LocClientHandle*, const void*);

using LocClientSendReqFn =
    std::uint32_t (*)(LocClientHandle, std::uint32_t, void*);

using LocClientCloseFn =
    std::uint32_t (*)(LocClientHandle*);

struct XtraRequest {
    std::uint32_t totalSize;
    std::uint16_t totalParts;
    std::uint16_t partNum;
    std::uint32_t partData_len;
    std::uint8_t  partData[1024];

    std::uint8_t  formatType_valid;
    std::uint8_t  padding[3];
    std::uint32_t formatType;
};

static_assert(sizeof(XtraRequest) == 0x414);

static void responseCallback(
    LocClientHandle,
    std::uint32_t responseId,
    const void*,
    std::uint32_t payloadSize,
    void*) {
    std::printf("response callback: id=%u payloadSize=%u\n",
                responseId, payloadSize);
}

int main() 
{
    const char* path = "/data/local/tmp/xtra3grcej.bin";

    FILE* xtraFile = std::fopen(path, "rb");
    if (!xtraFile) 
		{
        std::perror("fopen");
        return 1;
    }

    std::fseek(xtraFile, 0, SEEK_END);
    const long fileSize = std::ftell(xtraFile);
    std::fseek(xtraFile, 0, SEEK_SET);

    if (fileSize <= 0) 
		{
        std::fprintf(stderr, "invalid XTRA file size: %ld\n", fileSize);
        std::fclose(xtraFile);
        return 1;
    }

    if (static_cast<unsigned long>(fileSize) > UINT32_MAX) 
		{
        std::fprintf(stderr, "XTRA file is too large\n");
        std::fclose(xtraFile);
        return 1;
    }

    const std::uint32_t totalSize = static_cast<std::uint32_t>(fileSize);

    const std::uint32_t totalParts = (totalSize + 1023) / 1024;

    if (totalParts > UINT16_MAX) 
		{
        std::fprintf(stderr, "too many XTRA parts\n");
        std::fclose(xtraFile);
        return 1;
    }

    void* lib = dlopen("/vendor/lib64/libloc_api_v02.so", RTLD_NOW);

    if (!lib) 
		{
        std::fprintf(stderr, "dlopen failed: %s\n", dlerror());
        std::fclose(xtraFile);
        return 1;
    }

    auto open = reinterpret_cast<LocClientOpenFn>(dlsym(lib, "locClientOpen"));

    auto send = reinterpret_cast<LocClientSendReqFn>(dlsym(lib, "locClientSendReq"));

    auto close = reinterpret_cast<LocClientCloseFn>(dlsym(lib, "locClientClose"));

    if (!open || !send || !close) 
		{
        std::fprintf(stderr, "required symbol lookup failed\n");
        dlclose(lib);
        std::fclose(xtraFile);
        return 1;
    }

    LocClientCallbacks callbacks{};
    callbacks.size = sizeof(callbacks);
    callbacks.respIndCb = responseCallback;

    LocClientHandle handle = nullptr;

    const auto openStatus = open(0, &callbacks, &handle, nullptr);

    std::printf("locClientOpen status=%u handle=%p\n",openStatus, handle);

    if (openStatus != 0 || handle == nullptr) 
		{
        dlclose(lib);
        std::fclose(xtraFile);
        return 1;
    }

    std::printf("XTRA file: %u bytes, %u parts\n",totalSize, totalParts);

    XtraRequest req{};

    req.totalSize = totalSize;
    req.totalParts = static_cast<std::uint16_t>(totalParts);
    req.formatType_valid = 1;
    req.formatType = 0;

    std::uint32_t offset = 0;

    for (std::uint32_t part = 1; part <= totalParts; ++part) 
		{
        req.partNum = static_cast<std::uint16_t>(part);

        req.partData_len = std::min<std::uint32_t>(1024, totalSize - offset);

        std::memset(req.partData, 0, sizeof(req.partData));

        const std::size_t n = std::fread(req.partData, 1, req.partData_len, xtraFile);

        if (n != req.partData_len) {
            std::fprintf(
                stderr,
                "short read on part %u: got %zu, expected %u\n",
                part, n, req.partData_len);

            close(&handle);
            dlclose(lib);
            std::fclose(xtraFile);
            return 1;
        }

        std::printf("part %u/%u: offset=%u length=%u ... ", part, totalParts, offset, req.partData_len);

        const auto status = send(handle, 0xa7, &req);

        std::printf("status=%u\n", status);

        if (status != 0) {
            std::fprintf(
                stderr,
                "XTRA injection stopped at part %u\n",
                part);

            close(&handle);
            dlclose(lib);
            std::fclose(xtraFile);
            return 1;
        }

        offset += req.partData_len;
    }

    std::fclose(xtraFile);

    const auto closeStatus = close(&handle);

    std::printf(
        "complete: injected %u bytes in %u parts\n",
        offset, totalParts);

    std::printf("locClientClose status=%u\n", closeStatus);

    dlclose(lib);
    return 0;
}

// #include <dlfcn.h>
// #include <cstdint>
// #include <cstdio>
// #include <cstring>

// using LocClientHandle = void*;

// using EventCallback = void (*)(LocClientHandle, std::uint32_t, const void*, void*);

// using ResponseCallback = void (*)(LocClientHandle, std::uint32_t, const void*, std::uint32_t, void*);

// using ErrorCallback =  void (*)(LocClientHandle, std::uint32_t, void*);

// struct LocClientCallbacks {
//     std::uint32_t size;
//     EventCallback eventIndCb;
//     ResponseCallback respIndCb;
//     ErrorCallback errorCb;
// };

// using LocClientOpenFn = std::uint32_t (*)(std::uint64_t, const LocClientCallbacks*, LocClientHandle*, const void*);

// using LocClientSendReqFn = std::uint32_t (*)(LocClientHandle, std::uint32_t, void*);

// using LocClientCloseFn = std::uint32_t (*)(LocClientHandle*);

// struct XtraRequest {
//     std::uint32_t totalSize;
//     std::uint16_t totalParts;
//     std::uint16_t partNum;
//     std::uint32_t partData_len;
//     std::uint8_t  partData[1024];

//     std::uint8_t  formatType_valid;
//     std::uint8_t  padding[3];
//     std::uint32_t formatType;
// };

// static void responseCallback(
//     LocClientHandle,
//     std::uint32_t responseId,
//     const void*,
//     std::uint32_t payloadSize,
//     void*) 
// {
//   std::printf("response callback: id=%u payloadSize=%u\n", responseId, payloadSize);
// }

// static_assert(sizeof(XtraRequest) == 0x414);

// int main() 
// {
//     const char* path = "/data/local/tmp/xtra3grcej.bin";

//     FILE* f = std::fopen(path, "rb");
//     if (!f) 
// 		{
//         std::perror("fopen");
//         return 1;
//     }

//     std::fseek(f, 0, SEEK_END);
//     const long size = std::ftell(f);
//     std::fseek(f, 0, SEEK_SET);

//     if (size <= 0) 
// 		{
//         std::fprintf(stderr, "invalid XTRA file size: %ld\n", size);
//         std::fclose(f);
//         return 1;
//     }

//     XtraRequest req{};
//     req.totalSize = static_cast<std::uint32_t>(size);
//     req.totalParts = static_cast<std::uint16_t>((size + 1023) / 1024);
//     req.partNum = 1;
//     req.partData_len = static_cast<std::uint32_t>(size > 1024 ? 1024 : size);
//     req.formatType_valid = 1;
//     req.formatType = 0;

//     const std::size_t n = std::fread(req.partData, 1, req.partData_len, f);
//     std::fclose(f);

//     if (n != req.partData_len) 
// 		{
//         std::fprintf(stderr, "short read: got %zu, expected %u\n", n, req.partData_len);
//         return 1;
//     }

//     void* lib = dlopen("/vendor/lib64/libloc_api_v02.so", RTLD_NOW);
//     if (!lib) 
// 		{
//         std::fprintf(stderr, "dlopen failed: %s\n", dlerror());
//         return 1;
//     }

//     auto open = reinterpret_cast<LocClientOpenFn>(dlsym(lib, "locClientOpen"));
//     auto send = reinterpret_cast<LocClientSendReqFn>(dlsym(lib, "locClientSendReq"));
//     auto close = reinterpret_cast<LocClientCloseFn>(dlsym(lib, "locClientClose"));

//     if (!open || !send || !close) 
// 		{
//         std::fprintf(stderr, "required symbol lookup failed\n");
//         dlclose(lib);
//         return 1;
//     }

//     LocClientCallbacks callbacks{};
//     callbacks.size = sizeof(callbacks);
// 		callbacks.respIndCb = responseCallback; // apparently not having this is a deal breaker

//     LocClientHandle handle = nullptr;

//     const auto openStatus = open(0, &callbacks, &handle, nullptr);

//     std::printf("locClientOpen status=%u handle=%p\n", openStatus, handle);

//     if (openStatus != 0 || !handle) 
// 		{
//         dlclose(lib);
//         return 1;
//     }

//     std::printf(
//         "sending XTRA: totalSize=%u totalParts=%u "
//         "partNum=%u partData_len=%u formatType=%u\n",
//         req.totalSize,
//         req.totalParts,
//         req.partNum,
//         req.partData_len,
//         req.formatType);

//     const auto status = send(handle, 0xa7, &req);

//     std::printf("locClientSendReq(0xa7) status=%u\n", status);

//     const auto closeStatus = close(&handle);
//     std::printf("locClientClose status=%u\n", closeStatus);

//     dlclose(lib);
//     return status == 0 ? 0 : 1;
// }

// #include <dlfcn.h>
// #include <cstdint>
// #include <cstdio>

// using LocClientHandle = void*;
// using EventMask = std::uint64_t;

// // These callback parameter types are deliberately opaque.
// // The Qualcomm API passes the response union by value; on AArch64
// // that union is pointer-sized.
// using EventCallback = void (*)(LocClientHandle, std::uint32_t, const void*, void*);

// using ResponseCallback = void (*)(LocClientHandle, std::uint32_t, const void*, std::uint32_t, void*);

// using ErrorCallback = void (*)(LocClientHandle, std::uint32_t, void*);

// struct LocClientCallbacks {
//     std::uint32_t size;
//     EventCallback eventIndCb;
//     ResponseCallback respIndCb;
//     ErrorCallback errorCb;
// };

// using LocClientOpenFn = std::uint32_t (*)(EventMask, const LocClientCallbacks*, LocClientHandle*, const void*);

// using LocClientCloseFn = std::uint32_t (*)(LocClientHandle*);

// static void responseCallback(
//     LocClientHandle,
//     std::uint32_t responseId,
//     const void*,
//     std::uint32_t payloadSize,
//     void*) 
// {
//   std::printf("response callback: id=%u payloadSize=%u\n", responseId, payloadSize);
// }

// int main() 
// {
//     void* lib = dlopen("/vendor/lib64/libloc_api_v02.so", RTLD_NOW);
//     if (!lib) 
// 		{
//         std::fprintf(stderr, "dlopen failed: %s\n", dlerror());
//         return 1;
//     }

//     auto open = reinterpret_cast<LocClientOpenFn>(dlsym(lib, "locClientOpen"));
//     auto close = reinterpret_cast<LocClientCloseFn>(dlsym(lib, "locClientClose"));

//     if (!open || !close) 
// 		{
//         std::fprintf(stderr, "required symbol lookup failed\n");
//         dlclose(lib);
//         return 1;
//     }

//     LocClientCallbacks callbacks{};
//     callbacks.size = sizeof(callbacks);
//     callbacks.respIndCb = responseCallback;

//     LocClientHandle handle = nullptr;

//     std::printf("callback struct size = %zu\n", sizeof(callbacks));

//     const auto status =
//         open(/* eventRegMask = */ 0,
//              &callbacks,
//              &handle,
//              /* cookie = */ nullptr);

//     std::printf("locClientOpen status = %u, handle = %p\n", status, handle);

//     if (status == 0 && handle != nullptr) 
// 		{
//         const auto closeStatus = close(&handle);
//         std::printf("locClientClose status = %u, handle = %p\n", closeStatus, handle);
//     }

//     dlclose(lib);
//     return 0;
// }
