

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
/* izat_core::IzatApiV02::injectXtraData(char const*, unsigned int) */
// ghidra renamed and minimally prettified for use
void __thiscall
izat_core::IzatApiV02::injectXtraData(IzatApiV02 *this,char *param_xtra_file,uint param_xtra_size)

{
  long lVar1;
  byte bVar2;
  int syncReqResult;
  undefined8 uVar3;
  long lVar4;
  undefined4 uVar5;
  int xtraFileReadOffsetStart;
  bool haveAllPartsBeenSent;
  uint32_t previousPart;
  undefined8 syncReqOutAdditional;
  undefined8 uStack_4c0;
  undefined8 local_4b8;
  undefined8 local_4b0;
  undefined8 uStack_4a8;
  undefined8 uStack_4a0;
  undefined8 uStack_498;
  qmiLocInjectXtraDataReqMsgT_v02 syncRequest;
  uint32_t currentPart;
  uint64_t originalSyncOutAdditionalPostWrite;
  bool syncReqOutFlagsUNClean;
  uint totalPartsMinus1;
  
  lVar1 = tpidr_el0;
  lVar4 = *(long *)(lVar1 + 0x28);
  if ((param_xtra_file == (char *)0x0) || (param_xtra_size == 0)) {
    if ((_loc_logger & 0xfffffffffffffffe) == 4) {
      __android_log_print(3,"LocSvc_IzatApiV02","[%s:%d] invalid argument data = %p length = %u\n",
                          "injectXtraData",0xad3,param_xtra_file,param_xtra_size);
    }
    bVar2 = 2;
  }
  else {
    memset(syncRequest.partData,0,0x404);
    syncRequest.formatType = 0;
    totalPartsMinus1 = param_xtra_size - 1 >> 10;
    haveAllPartsBeenSent = false;
    xtraFileReadOffsetStart = 0;
    previousPart = 0xffffffff;
    syncRequest.formatType_valid = 1;
    syncRequest.totalParts = (ushort)(totalPartsMinus1 + 1);
    syncRequest.totalSize = param_xtra_size;
    do {
      syncRequest.partData_len = param_xtra_size - xtraFileReadOffsetStart;
      currentPart = previousPart + 2;
      if (0x3ff < (int)syncRequest.partData_len) {
        syncRequest.partData_len = 0x400;
      }
      syncRequest.partNum = (ushort)currentPart;
      __memcpy_chk(syncRequest.partData,param_xtra_file + xtraFileReadOffsetStart,
                   syncRequest.partData_len,0x408);
      if ((_loc_logger & 0xfffffffffffffffe) == 4) {
        __android_log_print(3,"LocSvc_IzatApiV02",
                            "[%s:%d] part %d/%d, len = %d, total injected = %d\n","injectXtraData",
                            0xae9,syncRequest.partNum,totalPartsMinus1 + 1,syncRequest.partData_len,
                            xtraFileReadOffsetStart);
      }
      syncReqOutAdditional = 0;
      uStack_4c0 = 0;
      local_4b8 = 0;
      syncReqResult =
           LocApiV02::locSyncSendReq
                     (*(undefined8 *)(*(long *)(this + 0x58) + 8),0xa7,&syncRequest,1000,0xa7,
                      &syncReqOutAdditional);
      if (syncReqResult == 0 && (int)syncReqOutAdditional == 0) {
        uVar5 = 1;
        originalSyncOutAdditionalPostWrite = syncReqOutAdditional;
joined_r0x0011ef3c:
        syncReqOutFlagsUNClean = (int)syncReqOutAdditional != 0;
        syncReqOutAdditional = originalSyncOutAdditionalPostWrite;
        if ((syncReqOutFlagsUNClean) ||
           (syncReqOutAdditional._6_2_ = (ushort)(originalSyncOutAdditionalPostWrite >> 0x30),
           syncReqOutFlagsUNClean = syncRequest.partNum != syncReqOutAdditional._6_2_,
           syncReqOutFlagsUNClean)) {
LAB_0011efe4:
          if (_loc_logger - 1 < 5) {
            __android_log_print(6,"LocSvc_IzatApiV02",
                                "%s:%d]: failed status = %d, ind.status = %d, part num = %d, ind.partNum = %d\n"
                                ,"injectXtraData",0xaf4,syncReqResult,
                                syncReqOutAdditional & 0xffffffff,syncRequest.partNum,
                                syncReqOutAdditional._6_2_);
          }
          if (_getCarrierCapabilities == 0) {
            if (_loc_logger == 5) {
              __android_log_print(2,"LocSvc_IzatApiV02","%s %s line %d %d",&EXIT_TAG,
                                  "injectXtraData",0xaf7,uVar5);
            }
          }
          else {
            uStack_4a8 = 0;
            local_4b0 = 0;
            uStack_498 = 0;
            uStack_4a0 = 0;
            if (_loc_logger == 5) {
              uVar3 = get_timestamp(&local_4b0,0x20);
              __android_log_print(2,"LocSvc_IzatApiV02","[%s] %s %s line %d %d",uVar3,&EXIT_TAG,
                                  "injectXtraData",0xaf7,uVar5);
            }
          }
          break;
        }
      }
      else {
        if (_loc_logger - 1 < 5) {
          __android_log_print(6,"LocSvc_IzatApiV02","%s:%d]: Error : st = %d, ind.status = %d",
                              "injectXtraData",0xaec,syncReqResult);
        }
        if (syncReqResult != 6) {
          if (syncReqResult == 0) {
            uVar5 = 0;
            originalSyncOutAdditionalPostWrite = syncReqOutAdditional;
            goto joined_r0x0011ef3c;
          }
          uVar5 = 0;
          goto LAB_0011efe4;
        }
      }
      xtraFileReadOffsetStart = syncRequest.partData_len + xtraFileReadOffsetStart;
      if ((_loc_logger & 0xfffffffffffffffe) == 4) {
        __android_log_print(3,"LocSvc_IzatApiV02","%s:%d]: XTRA accumulated injected length: %d\n",
                            "injectXtraData",0xafc,xtraFileReadOffsetStart);
      }
      previousPart = previousPart + 1;
      haveAllPartsBeenSent = totalPartsMinus1 < currentPart;
    } while (totalPartsMinus1 != previousPart);
    bVar2 = ~haveAllPartsBeenSent & 1;
  }
  if (*(long *)(lVar1 + 0x28) == lVar4) {
    return;
  }
                    /* WARNING: Subroutine does not return */
  __stack_chk_fail(bVar2);
}
