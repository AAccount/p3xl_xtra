/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
/* izat_core::IzatApiV02::injectXtraData(char const*, unsigned int) */

void __thiscall izat_core::IzatApiV02::injectXtraData(IzatApiV02 *this,char *param_1,uint param_2)

{
  uint uVar1;
  uint uVar2;
  long lVar3;
  ulong uVar4;
  bool bVar5;
  byte bVar6;
  int iVar7;
  undefined8 uVar8;
  undefined4 uVar9;
  int iVar10;
  bool bVar11;
  uint uVar12;
  undefined8 local_4c8;
  undefined8 uStack_4c0;
  undefined8 local_4b8;
  undefined8 local_4b0;
  undefined8 uStack_4a8;
  undefined8 uStack_4a0;
  undefined8 uStack_498;
  uint local_488;
  undefined2 local_484;
  short local_482;
  int local_480;
  undefined1 auStack_47c [1024];
  undefined1 local_7c;
  undefined4 local_78;
  long local_70;
  
  lVar3 = tpidr_el0;
  local_70 = *(long *)(lVar3 + 0x28);
  if ((param_1 == (char *)0x0) || (param_2 == 0)) 
  {
    if ((_loc_logger & 0xfffffffffffffffe) == 4) 
    {
      __android_log_print(3,"LocSvc_IzatApiV02","[%s:%d] invalid argument data = %p length = %u\n",
                          "injectXtraData",0xad3,param_1,param_2);
    }
    bVar6 = 2;
  }
  else 
  {
    memset(auStack_47c,0,0x404);
    local_78 = 0;
    uVar2 = param_2 - 1 >> 10;
    bVar11 = false;
    iVar10 = 0;
    uVar12 = 0xffffffff;
    local_7c = 1;
    local_484 = (undefined2)(uVar2 + 1);
    local_488 = param_2;
    do 
    {
      local_480 = param_2 - iVar10;
      uVar1 = uVar12 + 2;
      if (0x3ff < local_480) 
      {
        local_480 = 0x400;
      }
      local_482 = (short)uVar1;
      __memcpy_chk(auStack_47c,param_1 + iVar10,local_480,0x408);
      if ((_loc_logger & 0xfffffffffffffffe) == 4) 
      {
        __android_log_print(3,"LocSvc_IzatApiV02",
                            "[%s:%d] part %d/%d, len = %d, total injected = %d\n","injectXtraData",
                            0xae9,local_482,uVar2 + 1,local_480,iVar10);
      }
      local_4c8 = 0;
      uStack_4c0 = 0;
      local_4b8 = 0;
      iVar7 = LocApiV02::locSyncSendReq
                        (*(undefined8 *)(*(long *)(this + 0x58) + 8),0xa7,&local_488,1000,0xa7,
                         &local_4c8);
      if (iVar7 == 0 && (int)local_4c8 == 0) 
      {
        uVar9 = 1;
        uVar4 = local_4c8;
joined_r0x0011ef3c:
        bVar5 = (int)local_4c8 != 0;
        local_4c8 = uVar4;
        if ((bVar5) || (local_4c8._6_2_ = (short)(uVar4 >> 0x30), bVar5 = local_482 != local_4c8._6_2_, bVar5))
        {
LAB_0011efe4:
          if (_loc_logger - 1 < 5) 
          {
            __android_log_print(6,"LocSvc_IzatApiV02",
                                "%s:%d]: failed status = %d, ind.status = %d, part num = %d, ind.partNum = %d\n"
                                ,"injectXtraData",0xaf4,iVar7,local_4c8 & 0xffffffff,local_482,
                                local_4c8._6_2_);
          }
          if (_getCarrierCapabilities == 0) 
          {
            if (_loc_logger == 5) 
            {
              __android_log_print(2,"LocSvc_IzatApiV02","%s %s line %d %d",&EXIT_TAG,
                                  "injectXtraData",0xaf7,uVar9);
            }
          }
          else 
          {
            uStack_4a8 = 0;
            local_4b0 = 0;
            uStack_498 = 0;
            uStack_4a0 = 0;
            if (_loc_logger == 5) {
              uVar8 = get_timestamp(&local_4b0,0x20);
              __android_log_print(2,"LocSvc_IzatApiV02","[%s] %s %s line %d %d",uVar8,&EXIT_TAG,
                                  "injectXtraData",0xaf7,uVar9);
            }
          }
          break;
        }
      }
      else 
      {
        if (_loc_logger - 1 < 5)
        {
          __android_log_print(6,"LocSvc_IzatApiV02","%s:%d]: Error : st = %d, ind.status = %d",
                              "injectXtraData",0xaec,iVar7);
        }
        if (iVar7 != 6) 
        {
          if (iVar7 == 0) 
          {
            uVar9 = 0;
            uVar4 = local_4c8;
            goto joined_r0x0011ef3c;
          }
          uVar9 = 0;
          goto LAB_0011efe4;
        }
      }
      iVar10 = local_480 + iVar10;
      if ((_loc_logger & 0xfffffffffffffffe) == 4) 
      {
        __android_log_print(3,"LocSvc_IzatApiV02","%s:%d]: XTRA accumulated injected length: %d\n", "injectXtraData",0xafc,iVar10);
      }
      uVar12 = uVar12 + 1;
      bVar11 = uVar2 < uVar1;
    } while (uVar2 != uVar12);
    bVar6 = ~bVar11 & 1;
  }
  if (*(long *)(lVar3 + 0x28) == local_70) 
  {
    return;
  }
                    /* WARNING: Subroutine does not return */
  __stack_chk_fail(bVar6);
}
