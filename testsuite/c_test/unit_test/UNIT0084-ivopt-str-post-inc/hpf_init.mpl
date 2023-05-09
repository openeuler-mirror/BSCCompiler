flavor 1
srclang 1
id 65535
numfuncs 4706
var $g_mbufGlobalCtl extern <* <$HpeMbufGlobalCtl>> used
var $g_hpfHlogModId u32 used = 255
var $g_hpfHlogForcePrint extern u8 used
var $g_hpfHlogLevel extern i8 used
type $imaxdiv_t <struct {
  @quot i64 align(8),
  @rem i64 align(8)}>
type $_IO_FILE <structincomplete {}>
type $_G_fpos64_t <union {
  @__opaque <[16] i8>,
  @__lldata i64 align(8),
  @__align f64 align(8)}>
type $__va_list <struct {
  @__stack <* void> align(8),
  @__gr_top <* void> align(8),
  @__vr_top <* void> align(8),
  @__gr_offs i32 align(4),
  @__vr_offs i32 align(4)}>
type $_IO_cookie_io_functions_t <struct {
  @read <* <func (<* void>,<* i8>,u64) i64>> align(8),
  @write <* <func (<* void>,<* i8>,u64) i64>> align(8),
  @seek <* <func (<* void>,<* i64>,i32) i32>> align(8),
  @close <* <func (<* void>) i32>> align(8)}>
type $div_t <struct {
  @quot i32 align(4),
  @rem i32 align(4)}>
type $ldiv_t <struct {
  @quot i64 align(8),
  @rem i64 align(8)}>
type $lldiv_t <struct {
  @quot i64 align(8),
  @rem i64 align(8)}>
type $__locale_struct <structincomplete {}>
type $hpe_struct <struct {
  @logLevel u32 align(4),
  @hpeMem <* void> align(8),
  @hpeDrv <* void> align(8),
  @hpeCrypto <* void> align(8),
  @hpeLcore <* void> align(8),
  @hpeModm <* void> align(8),
  @hpeNotify <* void> align(8)}>
type $sched_param <struct {
  @sched_priority i32 align(4),
  @__reserved1 i32 align(4),
  @__reserved2 <[2] <$unnamed.5844>> align(8),
  @__reserved3 i32 align(4)}>
type $timespec <struct {
  @tv_sec i64 align(8),
  @unnamed.5842 :0 i32 align(4),
  @tv_nsec i64 align(8),
  @unnamed.5843 :0 i32 align(4)}>
type $cpu_set_t <struct {
  @__bits <[16] u64> align(8)}>
type $HpeAtom32 <struct {
  @cnt i32 volatile align(4)}>
type $HpeAtom16 <struct {
  @cnt i16 volatile align(2)}>
type $HpeAtom64 <struct {
  @cnt i64 volatile align(8)}>
type $HpeSpinlock <struct {
  @locked i32 volatile align(4),
  @pid i32 align(4),
  @lockId i32 align(4)}>
type $HpeSpinDelayStat <struct {
  @spinRounds u32 align(4),
  @sleepTime u32 align(4),
  @callSite <$HpeCallSite> align(8)}>
type $HpeSpinDelayConf <struct {
  @spinRounds u32 align(4),
  @spinFactor u32 align(4),
  @timeoutUs u32 align(4)}>
type $tagLflistEnds <struct {
  @head <* <$tagLflistNode>> volatile align(8),
  @tail <* <$tagLflistNode>> volatile align(8),
  @lock <$HpeSpinlock> align(4)}>
type $tagLflistNode <struct {
  @next <* <$tagLflistNode>> volatile align(8)}>
type $tagHpeListHead <struct {
  @prev <* <$tagHpeListHead>> align(8),
  @next <* <$tagHpeListHead>> align(8)}>
type $HPE_DLL <struct {
  @Head <$HPE_DLL_NODE> align(8),
  @count u32 align(4)}>
type $HPE_DLL_NODE <struct {
  @pNext <* <$HPE_DLL_NODE>> align(8),
  @pPrev <* <$HPE_DLL_NODE>> align(8),
  @ulHandle u64 align(8)}>
type $hpe_slist_head <struct {
  @next <* <$hpe_slist_head>> volatile align(8)}>
type $HpeHlistNode <struct {
  @next <* <$HpeHlistNode>> align(8),
  @pprev <* <* <$HpeHlistNode>>> align(8)}>
type $HpeHlistHead <struct {
  @first <* <$HpeHlistNode>> align(8)}>
type $HpeMemDllNode <struct {
  @node <$HPE_DLL_NODE> align(8),
  @size u32 align(4),
  @flag u32 align(4),
  @magic u32 align(4),
  @padSize u32 align(4)}>
type $tagHpeMemzone <struct {
  @type u8,
  @name <[32] i8>,
  @phys_addr u64 align(8),
  @unnamed.3370 <$unnamed.3371> implicit align(8),
  @len u64 align(8),
  @socket_id i32 align(4),
  @usedflags u32 align(4)}>
type $HpeRing <struct {
  @node <$tagHpeListHead> align(8),
  @name <[32] i8>,
  @flags i32 align(4),
  @ringSize u32 align(4),
  @ringCapacity u32 align(4),
  @ringMask u32 align(4),
  @prod <$HpeRingHeadTail> align(4),
  @cons <$HpeRingHeadTail> align(4)}>
type $HpeRingHeadTail <struct {
  @head u32 volatile align(4),
  @tail u32 volatile align(4),
  @single u32 align(4)}>
type $HpeRingControlBlock <struct {
  @ringNum u32 align(4),
  @qlock <$HpeSpinlock> align(4),
  @ringList <$tagHpeListHead> align(8)}>
type $tm <struct {
  @tm_sec i32 align(4),
  @tm_min i32 align(4),
  @tm_hour i32 align(4),
  @tm_mday i32 align(4),
  @tm_mon i32 align(4),
  @tm_year i32 align(4),
  @tm_wday i32 align(4),
  @tm_yday i32 align(4),
  @tm_isdst i32 align(4),
  @tm_gmtoff i64 align(8),
  @tm_zone <* i8> align(8)}>
type $sigevent <structincomplete {}>
type $itimerspec <struct {
  @it_interval <$timespec> align(8),
  @it_value <$timespec> align(8)}>
type $__pthread <structincomplete {}>
type $pthread_attr_t <struct {
  @__u <$unnamed.5845> align(8)}>
type $pthread_mutex_t <struct {
  @__u <$unnamed.5846> align(8)}>
type $pthread_mutexattr_t <struct {
  @__attr u32 align(4)}>
type $pthread_cond_t <struct {
  @__u <$unnamed.5847> align(8)}>
type $pthread_condattr_t <struct {
  @__attr u32 align(4)}>
type $pthread_rwlock_t <struct {
  @__u <$unnamed.5848> align(8)}>
type $pthread_rwlockattr_t <struct {
  @__attr <[2] u32> align(4)}>
type $pthread_barrier_t <struct {
  @__u <$unnamed.5849> align(8)}>
type $pthread_barrierattr_t <struct {
  @__attr u32 align(4)}>
type $__ptcb <struct {
  @__f <* <func (<* void>) void>> align(8),
  @__x <* void> align(8),
  @__next <* <$__ptcb>> align(8)}>
type $tagHpeAtomicCntBmp <union {
  @atomic <$HpeAtom64> align(8),
  @pair <$tagHpeCntBmp> align(4)}>
type $tagHpeCntBmp <struct {
  @cnt u32 align(4),
  @bmp u32 align(4)}>
type $fd_set <struct {
  @fds_bits <[16] u64> align(8)}>
type $timeval <struct {
  @tv_sec i64 align(8),
  @tv_usec i64 align(8)}>
type $__sigset_t <struct {
  @__bits <[16] u64> align(8)}>
type $itimerval <struct {
  @it_interval <$timeval> align(8),
  @it_value <$timeval> align(8)}>
type $timezone <struct {
  @tz_minuteswest i32 align(4),
  @tz_dsttime i32 align(4)}>
type $tagHpeEvent <struct {
  @name <[32] i8>,
  @nd i32 align(4),
  @events u16 align(2),
  @resEvents u16 align(2),
  @unnamed.5852 <$unnamed.5851> implicit align(8),
  @base <* <$tagHpeEventBase>> align(8),
  @callback <$tagHpeEventCallback> align(8)}>
type $tagHpeEventBase <struct {
  @ops <* <$tagHpeEventOps>> align(8),
  @opsBacker <* void> align(8),
  @activeQues <* <$HpeEventCallbackList>> align(8),
  @nActiveQues i32 align(4),
  @eventActiveCnt i32 align(4),
  @eventCnt i32 align(4),
  @runningFlag i32 align(4),
  @curProcEvent <* <$tagHpeEvent>> align(8),
  @levelTriggerList <$HpeEventCallbackList> align(8),
  @breakFlag i32 align(4),
  @lock <$HpeSpinlock> align(4),
  @limitCallbacksAfterPrio i32 align(4),
  @maxDispatchCallbacks i32 align(4),
  @maxDispatchInterval <$timeval> align(8)}>
type $tagHpeEventCbFunc <struct {
  @cb <* <func (i32,u16,<* void>) void>> align(8),
  @cbArg <* void> align(8)}>
type $tagHpeEventCfg <struct {
  @eventOpsName <[32] i8>,
  @nPriorities i32 align(4),
  @limitCallbacksAfterPrio i32 align(4),
  @maxDispatchCallbacks i32 align(4),
  @maxDispatchInterval <$timeval> align(8)}>
type $tagHpeLcore <struct {
  @state u32 volatile align(4),
  @busy u32 volatile align(4),
  @idleCnt u32 volatile align(4),
  @schedCnt u32 align(4),
  @recordCnt u32 align(4),
  @lcoreId u32 align(4),
  @pcoreId u32 align(4),
  @thread_id <* <$__pthread>> align(8),
  @tid u32 align(4),
  @base <* <$tagHpeEventBase>> align(8),
  @chanId1 i32 align(4),
  @chanId2 i32 align(4),
  @drvWork <[128] <* void>> align(8),
  @flushWork <* void> align(8),
  @mngWork <* void> align(8),
  @nRxPort u32 align(4),
  @rxPortList <[64] u32> align(4),
  @tmpAllocBitmap <[1] u64> align(8),
  @tmpBuf <[98304] u8>}>
type $HpeMempoolOps <struct {
  @type u32 align(4),
  @alloc <* <func (<* <$tagHpeMempool> align(64)>) i32>> align(8),
  @free <* <func (<* <$tagHpeMempool> align(64)>) void>> align(8),
  @getCount <* <func (<* <$tagHpeMempool> align(64)>) u32>> align(8),
  @enqueue <* <func (<* <$tagHpeMempool> align(64)>,<* <* void>>,u32) i32>> align(8),
  @dequeue <* <func (<* <$tagHpeMempool> align(64)>,<* <* void>>,u32) i32>> align(8),
  @objIter <* <func (<* <$tagHpeMempool> align(64)>,<* <func (<* <$tagHpeMempool> align(64)>,<* void>,<* void>,u32) void>>,<* void>) i32>> align(8)}>
type $tagHpeMempool <struct {
  @node <$tagHpeListHead> align(8),
  @name <[32] i8>,
  @unnamed.5856 <$unnamed.5855> implicit align(4),
  @flags u32 align(4),
  @opsType i32 align(4),
  @poolData <* void> align(8),
  @pool_config <* void> align(8),
  @mz <* <$tagHpeMemzone>> align(8),
  @socketId i32 align(4),
  @privateDataSize u32 align(4),
  @size u32 align(4),
  @populatedSize u32 align(4),
  @cacheSize u32 align(4),
  @eltSize u32 align(4),
  @headerSize u32 align(4),
  @trailerSize u32 align(4),
  @localCache <* <$HpeMempoolCache> align(64)> align(8),
  @eltList <$tagHpeListHead> align(8),
  @memChunk <$HpeMempoolMemChunk> align(8)}>
type $HpeMempoolOpsTable <struct {
  @lock <$HpeSpinlock> align(4),
  @mpOpsClass <[4] <* <$HpeMempoolOps> align(64)>> align(8)}>
type $HPE_MEMPOOL_INFO <struct {
  @gva u64 align(8),
  @index u32 align(4),
  @freeSize u64 align(8),
  @totalSize u64 align(8),
  @user <[20] <$HPE_MEMPOOL_USER>> align(8)}>
type $HpeMempoolObjsz <struct {
  @eltSize u32 align(4),
  @headerSize u32 align(4),
  @trailerSize u32 align(4),
  @totalSize u32 align(4)}>
type $HpeMempoolCache <struct {
  @size u32 align(4),
  @flushthresh u32 align(4),
  @len u32 align(4),
  @objs <[1536] <* void>> align(8)}>
type $hpe_mbuf <struct {
  @buf_addr <* void> align(8),
  @buf_physaddr u64 align(8),
  @data_off u16 align(2),
  @port u16 align(2),
  @data_len u16 align(2),
  @buf_len u16 align(2),
  @pool <* void> align(8),
  @next <* <$hpe_mbuf> align(64)> align(8),
  @pkt_len u32 align(4),
  @nb_segs u16 align(2),
  @seqn u16 align(2),
  @priv_size u16 align(2),
  @refcnt <$HpeAtom16> align(2),
  @qid u16 align(2),
  @capCode u16 align(2),
  @reorder_data u64 align(8),
  @userdata <* void> align(8),
  @sec_op <* void> align(8),
  @reorder_flag u8,
  @rsv2 u8,
  @tx_port u16 align(2),
  @last_worker u32 align(4),
  @dynfield1 <[2] u64> align(8),
  @unnamed.5871 <$unnamed.5859> implicit align(4),
  @unnamed.5872 <$unnamed.5867> implicit align(2),
  @ol_flags u64 align(8),
  @unnamed.5873 <$unnamed.5868> implicit align(8),
  @pkt_next <* <$hpe_mbuf> align(64)> align(8)}>
type $HpeRwlock <struct {
  @lock <$HpeAtom32> align(4)}>
type $tagHpeWfSpinlock <struct {
  @value <$HpeRwlock> align(4),
  @counter <$HpeAtom32> align(4)}>
type $HpeRwlockT <struct {
  @lock i32 volatile align(4)}>
type $HpeRWSpinStat <struct {
  @readerCount u32 align(4),
  @maxReadSpins u32 align(4),
  @maxWriteSpins u32 align(4)}>
type $HpeCallSite <struct {
  @file <* i8> align(8),
  @line i32 align(4),
  @func <* i8> align(8)}>
type $HpeDateTime <struct {
  @date <$HpeDate> align(4),
  @time <$HpeTime> align(2),
  @weekDay u8,
  @second u32 align(4),
  @milliSecond u64 align(8),
  @secondSince1970 u32 align(4),
  @timezoneOffset u32 align(4),
  @summertimeOffset u32 align(4),
  @isDST u32 align(4),
  @summertimeOffTime u32 align(4),
  @summertimeBeginDate <$HpeDate> align(4),
  @summertimeBeginTime <$HpeTime> align(2),
  @summertimeEndDate <$HpeDate> align(4),
  @summertimeEndTime <$HpeTime> align(2),
  @utcDate <$HpeDate> align(4),
  @utcTime <$HpeTime> align(2),
  @utcWeekDay u8,
  @uTCSecondSince1970 u32 align(4),
  @uTCMilliSecondSince1970 u64 align(8),
  @lock <$HpeRwlock> align(4),
  @uTCMilliSecondOffSince1970 i64 align(8),
  @timeTscFreq u64 align(8),
  @cyclePerMs u64 align(8)}>
type $HpeDate <struct {
  @date u16 align(2),
  @month u16 align(2),
  @year u32 align(4)}>
type $HpeTime <struct {
  @second u16 align(2),
  @minute u16 align(2),
  @hour u16 align(2)}>
type $tagTraceStatRoot <struct {
  @num i32 align(4),
  @trace <[512] <$tagTrace>> align(8)}>
type $tagTrace <struct {
  @num i32 align(4),
  @arr <[63] <* void>> align(8)}>
type $HpeMbufGlobalCtl <struct {
  @checkEnable :1 u32 align(4),
  @refreeCheck :1 u32 align(4),
  @addrCheckEnable :1 u32 align(4),
  @atomicCheckEnable :1 u32 align(4),
  @edgeCheckEnable :1 u32 align(4),
  @checkEnableCount :4 u32 align(4),
  @resv :23 u32 align(4),
  @checkTime u32 align(4),
  @refreeIsoRecTime u32 align(4),
  @leakIsoRecTime u32 align(4),
  @refreeTraceRoot <* <$tagTraceStatRoot>> align(8)}>
type $HpeMbufPrivInfo <struct {
  @ImcHead <$unnamed.5874> align(4),
  @MbufTrace <$unnamed.2048> align(8),
  @IbcNpCarInfo <$unnamed.5875> align(4),
  @zeroCopyReserve <[64] i8>}>
type $unnamed.2048 <struct {
  @safeMark u32 align(4),
  @enableCount :4 u8,
  @checkEnable :1 u8,
  @refreeCheck :1 u8,
  @resv0 :2 u8,
  @isLeakRecorded :1 u8,
  @resv1 :7 u8,
  @repeatTag u16 align(2),
  @allocLine u16 align(2),
  @curFuncStart u8,
  @funcIndex u8,
  @allocPid i32 align(4),
  @allocFile u64 align(8),
  @allocTime u64 align(8),
  @channelId u32 align(4),
  @funcId <[32] u16> align(2),
  @ownerWorkId u16 align(2),
  @unnamed.2049 <$unnamed.2050> implicit align(2),
  @ownerTid u32 align(4),
  @isoNode u64 align(8)}>
type $unnamed.2050 <union {
  @refreeTraceId u16 align(2),
  @preEdgeCheck u16 align(2)}>
type $HpeMbufPoolParam <struct {
  @name <[32] i8>,
  @num u32 align(4),
  @bufSize u32 align(4),
  @privDataSize u32 align(4),
  @cacheSize u32 align(4),
  @socketId i32 align(4),
  @opsName <* i8> align(8)}>
type $HpeBoardInfo <struct {
  @key u32 align(4),
  @chassisID u32 align(4),
  @slotID u32 align(4),
  @boardID u32 align(4),
  @boardRole u32 align(4),
  @cpuID u32 align(4),
  @ip u32 align(4),
  @mmpuip u32 align(4),
  @smpuip u32 align(4),
  @systemMacNumber u32 align(4),
  @registerStatus u32 align(4),
  @systemMac <[6] i8>,
  @ibcPortMac <[6] i8>,
  @selfBoardFlag u32 align(4),
  @peerBoardID u32 align(4),
  @mainBoardID u32 align(4),
  @sourceMod u32 align(4),
  @methMod u32 align(4),
  @methPort u32 align(4),
  @haveLpuBoard u8,
  @haveSpuBoard u8,
  @cpuType u8,
  @haveRemoteGX u8,
  @version u32 align(4),
  @fullmesh u8,
  @closeChnPri u8,
  @mmpuUpdatedByFwm u8,
  @stkEnable u32 align(4),
  @rsv <[2] u8>,
  @listNode <$tagHpeListHead> align(8),
  @boardLock <$HpeSpinlock> align(4)}>
type $tagHpeLfhashTable <struct {
  @hash_array_size u32 align(4),
  @node_used_num u32 volatile align(4),
  @node_unfreed_num u32 volatile align(4),
  @max_recycle_time u32 align(4),
  @tabId u32 align(4),
  @hash_array_base_addr <* <$tagLfhashArray>> align(8),
  @mem_alloc_func <* <func (u32) <* void>>> align(8),
  @mem_free_func <* <func (<* void>) u32>> align(8),
  @get_hash_index_func <* <func (<* void>) u32>> align(8)}>
type $tagHpeLfhashCreateInfo <struct {
  @hash_num u32 align(4),
  @max_recycle_time u32 align(4),
  @mem_alloc_func <* <func (u32) <* void>>> align(8),
  @mem_free_func <* <func (<* void>) u32>> align(8),
  @get_hashidx_func <* <func (<* void>) u32>> align(8)}>
type $tagHpeLfhashNode <struct {
  @next <* <$tagHpeLfhashNode>> volatile align(8),
  @state u32 align(4),
  @ref <$HpeAtom32> align(4)}>
type $tagLfhashArray <struct {
  @head <* <$tagHpeLfhashNode>> volatile align(8),
  @lock <$HpeSpinlock> align(4),
  @node_num u32 volatile align(4)}>
type $HpeStatModStrTag <struct {
  @spinlock <$HpeSpinlock> align(4),
  @normalStatStr <* <* i8>> align(8),
  @errStatStr <* <* i8>> align(8),
  @modAddrNormal <* void> align(8),
  @modAddrError <* void> align(8),
  @maxNormalNum u32 align(4),
  @maxErrNum u32 align(4),
  @modId i32 align(4),
  @name <[32] i8>}>
type $stat <struct {
  @st_dev u64 align(8),
  @st_ino u64 align(8),
  @st_mode u32 align(4),
  @st_nlink u32 align(4),
  @st_uid u32 align(4),
  @st_gid u32 align(4),
  @st_rdev u64 align(8),
  @__pad u64 align(8),
  @st_size i64 align(8),
  @st_blksize i32 align(4),
  @__pad2 i32 align(4),
  @st_blocks i64 align(8),
  @st_atim <$timespec> align(8),
  @st_mtim <$timespec> align(8),
  @st_ctim <$timespec> align(8),
  @__unused <[2] u32> align(4)}>
type $file_handle <struct {
  @handle_bytes u32 align(4),
  @handle_type i32 align(4),
  @f_handle <[1] u8 incomplete_array>}>
type $iovec <struct {
  @iov_base <* void> align(8),
  @iov_len u64 align(8)}>
type $HpfIpHdr <struct pack(1) {
  @ip_chHLen :4 u8,
  @ip_chVer :4 u8,
  @ip_chTOS u8,
  @ip_usLen u16 align(2),
  @ip_usId u16 align(2),
  @ip_usOff u16 align(2),
  @ip_chTTL u8,
  @ip_chPr u8,
  @ip_usSum u16 align(2),
  @ip_stSrc <$HpfInAddr>,
  @ip_stDst <$HpfInAddr>}>
type $HpfIp6Hdr <struct pack(1) {
  @ip6_ctlun <$unnamed.2295>,
  @ip6_stSrc <$HpfIn6Addr>,
  @ip6_stDst <$HpfIn6Addr>}>
type $unnamed.2295 <union pack(1) {
  @ip6_un1 <$unnamed.2296>,
  @ip6_un2 <$unnamed.5879>}>
type $unnamed.2296 <struct pack(1) {
  @ul_ip6_tclass_flow u32 align(4),
  @us_ip6_plen u16 align(2),
  @uc_ip6_nxt u8,
  @uc_ip6_hlim u8}>
type $HpeTmwbase <struct {
  @self <* <$HpeTmwbaseStruct>> align(8),
  @valid u8,
  @intvltypemax u8,
  @intvltypefree u8,
  @bktspertmw u8,
  @tmwBktBits u32 align(4),
  @tmw <* <$HpeTmw>> align(8),
  @jobs <* <$HpeTmwjob>> align(8),
  @maxjobs u32 align(4),
  @invalidjob u32 align(4),
  @bktmaxlLen u32 align(4),
  @tmwUse <$HpeTmwuse>,
  @seq u32 align(4)}>
type $HpeTmwmsg <struct {
  @timerid u32 align(4),
  @intvltype u8,
  @rsv u8,
  @job u32 align(4)}>
type $HpeTmwbkt <struct {
  @jobs u32 align(4),
  @header u32 align(4)}>
type $HpeTmwuse <struct {
  @usetmwnum u8,
  @tmwheader u8}>
type $HpeTmw <struct {
  @intvl u64 align(8),
  @dltintvl u64 align(8),
  @expires u64 align(8),
  @nextbase u64 align(8),
  @ticks u64 align(8),
  @next u8,
  @pre u8,
  @rsv u16 align(2),
  @curbkt u8,
  @hasJobs u32 align(4),
  @msg <$HpeTmwmsg> align(4),
  @astbkt <[128] <$HpeTmwbkt>> align(4)}>
type $HpfNatIntf <struct {
  @natAging <* <func (<* <$HpfSessionKey>>,<* <$HpfNatAgingParam>>,<* u8>) u32>> align(8),
  @natRefresh <* <func (<* <$hpe_mbuf> align(64)>,<* <$HpfNatParam>>,<* <$HpfNatParamRet>>,<* u8>) u32>> align(8),
  @natAddSuccess <* <func (<* <$hpe_mbuf> align(64)>,<* <$HpfNatParam>>,<* <$HpfNatParamRet>>,<* u8>) u32>> align(8),
  @dNat <* <func (<* <$hpe_mbuf> align(64)>,<* <$HpfNatParam>>,<* <$HpfNatParamRet>>,<* u8>) u32>> align(8),
  @sNat <* <func (<* <$hpe_mbuf> align(64)>,<* <$HpfNatParam>>,<* <$HpfNatParamRet>>,<* u8>) u32>> align(8)}>
type $HpfSessionKey <struct {
  @sIp u32 align(4),
  @dIp u32 align(4),
  @unnamed.5894 <$unnamed.5889> implicit align(2),
  @vpn u16 align(2),
  @proto u8,
  @resv1 u8}>
type $int8x8x2_t <struct {
  @val <[2] v8i8> align(8)}>
type $int8x16x2_t <struct {
  @val <[2] v16i8> align(16)}>
type $int16x4x2_t <struct {
  @val <[2] v4i16> align(8)}>
type $int16x8x2_t <struct {
  @val <[2] v8i16> align(16)}>
type $int32x2x2_t <struct {
  @val <[2] v2i32> align(8)}>
type $int32x4x2_t <struct {
  @val <[2] v4i32> align(16)}>
type $uint8x8x2_t <struct {
  @val <[2] v8u8> align(8)}>
type $uint8x16x2_t <struct {
  @val <[2] v16u8> align(16)}>
type $uint16x4x2_t <struct {
  @val <[2] v4u16> align(8)}>
type $uint16x8x2_t <struct {
  @val <[2] v8u16> align(16)}>
type $uint32x2x2_t <struct {
  @val <[2] v2u32> align(8)}>
type $uint32x4x2_t <struct {
  @val <[2] v4u32> align(16)}>
type $int64x1x2_t <struct {
  @val <[2] v1i64 oneelem_simd> align(8)}>
type $uint64x1x2_t <struct {
  @val <[2] v1u64 oneelem_simd> align(8)}>
type $int64x2x2_t <struct {
  @val <[2] v2i64> align(16)}>
type $uint64x2x2_t <struct {
  @val <[2] v2u64> align(16)}>
type $int8x8x3_t <struct {
  @val <[3] v8i8> align(8)}>
type $int8x16x3_t <struct {
  @val <[3] v16i8> align(16)}>
type $int16x4x3_t <struct {
  @val <[3] v4i16> align(8)}>
type $int16x8x3_t <struct {
  @val <[3] v8i16> align(16)}>
type $int32x2x3_t <struct {
  @val <[3] v2i32> align(8)}>
type $int32x4x3_t <struct {
  @val <[3] v4i32> align(16)}>
type $uint8x8x3_t <struct {
  @val <[3] v8u8> align(8)}>
type $uint8x16x3_t <struct {
  @val <[3] v16u8> align(16)}>
type $uint16x4x3_t <struct {
  @val <[3] v4u16> align(8)}>
type $uint16x8x3_t <struct {
  @val <[3] v8u16> align(16)}>
type $uint32x2x3_t <struct {
  @val <[3] v2u32> align(8)}>
type $uint32x4x3_t <struct {
  @val <[3] v4u32> align(16)}>
type $int64x1x3_t <struct {
  @val <[3] v1i64 oneelem_simd> align(8)}>
type $uint64x1x3_t <struct {
  @val <[3] v1u64 oneelem_simd> align(8)}>
type $int64x2x3_t <struct {
  @val <[3] v2i64> align(16)}>
type $uint64x2x3_t <struct {
  @val <[3] v2u64> align(16)}>
type $int8x8x4_t <struct {
  @val <[4] v8i8> align(8)}>
type $int8x16x4_t <struct {
  @val <[4] v16i8> align(16)}>
type $int16x4x4_t <struct {
  @val <[4] v4i16> align(8)}>
type $int16x8x4_t <struct {
  @val <[4] v8i16> align(16)}>
type $int32x2x4_t <struct {
  @val <[4] v2i32> align(8)}>
type $int32x4x4_t <struct {
  @val <[4] v4i32> align(16)}>
type $uint8x8x4_t <struct {
  @val <[4] v8u8> align(8)}>
type $uint8x16x4_t <struct {
  @val <[4] v16u8> align(16)}>
type $uint16x4x4_t <struct {
  @val <[4] v4u16> align(8)}>
type $uint16x8x4_t <struct {
  @val <[4] v8u16> align(16)}>
type $uint32x2x4_t <struct {
  @val <[4] v2u32> align(8)}>
type $uint32x4x4_t <struct {
  @val <[4] v4u32> align(16)}>
type $int64x1x4_t <struct {
  @val <[4] v1i64 oneelem_simd> align(8)}>
type $uint64x1x4_t <struct {
  @val <[4] v1u64 oneelem_simd> align(8)}>
type $int64x2x4_t <struct {
  @val <[4] v2i64> align(16)}>
type $uint64x2x4_t <struct {
  @val <[4] v2u64> align(16)}>
type $HpfMbufContext <struct {
  @mbufTxRxInfo <$unnamed.5975> align(4),
  @flag u32 align(4),
  @flag_ext u32 align(4),
  @egress_port u32 align(4),
  @sb u16 align(2),
  @sp u16 align(2),
  @egress_phy_port u32 align(4),
  @recv_phy_port u32 align(4),
  @recv_port u32 align(4),
  @vlan_key u16 align(2),
  @vlan_in u16 align(2),
  @ids_revert :1 u8,
  @l2_ipsec_flag :1 u8,
  @in_vlanmapp_flag :1 u8,
  @phy_send :1 u8,
  @mbuftr_debug_flag :1 u8,
  @bIsHitWls :1 u8,
  @iic_crc_flag :1 u8,
  @trunk_mem_send :1 u8,
  @trace4_debug_flag :2 u16 align(2),
  @trace6_debug_flag :2 u16 align(2),
  @trace_pkt_id :4 u16 align(2),
  @trace_pkt_dir :2 u16 align(2),
  @acl_recv_stat_flag :1 u16 align(2),
  @atkFlag :1 u16 align(2),
  @resv :4 u16 align(2),
  @l2_head_length u8,
  @tcp_flag u8,
  @in_zone u16 align(2),
  @out_zone u16 align(2),
  @frame_flag <$HpfMbufFrameFlag> align(2),
  @packet_key <$HpfFlowKey> align(8),
  @eth_type u16 align(2),
  @l3_hdr_offset u16 align(2),
  @src_nat_ip u32 align(4),
  @unnamed.5995 <$unnamed.5976> implicit align(4),
  @dst_nat_ip u32 align(4),
  @dst6NatIp <$HpfIn6Addr>,
  @src_nat_port u16 align(2),
  @dst_nat_port u16 align(2),
  @dslite_tunnel_id u16 align(2),
  @protocol u8,
  @fwd_type u8,
  @send_vrf u16 align(2),
  @vsys_index u16 align(2),
  @unnamed.5996 <$unnamed.5979> implicit align(4),
  @user_id u32 align(4),
  @app_data_len u16 align(2),
  @icmp_type u8,
  @icmp_code u8,
  @icmpCheckSum u16 align(2),
  @np_hashindex :31 u32 align(4),
  @np_hash_isvaild :1 u32 align(4),
  @icmpNsDstAdd <[4] u32> align(4),
  @src_inst_id u8,
  @dst_mac_type u8,
  @vlan_pri :4 u8,
  @hit_mail_cache :1 u8,
  @blacklist :1 u8,
  @whitelist :2 u8,
  @vsys_inbound_handled :1 u8,
  @vsys_outbound_handled :1 u8,
  @vsys_entire_handled :1 u8,
  @vsys_trans :1 u8,
  @vsys_from_flag :1 u8,
  @vsys_to_flag :1 u8,
  @vsys_drop_flag :1 u8,
  @vsys_handled_flag :1 u8,
  @unnamed.5997 <$unnamed.5981> implicit align(4),
  @not_used :3 u16 align(2),
  @mac_learn_disable_act_drop :1 u16 align(2),
  @flow_split :1 u16 align(2),
  @trace_pkt_in_tag :1 u16 align(2),
  @icmp_error_nated_flag :1 u16 align(2),
  @hrp_mgt_fwd_pkt :1 u16 align(2),
  @np_debug_flag :1 u16 align(2),
  @hash_index_valid :1 u16 align(2),
  @ah_esp_oper_flag :3 u16 align(2),
  @cross_vsys_limit :3 u16 align(2),
  @app_protocol u8,
  @quota_flag :1 u8,
  @ids_mode :1 u8,
  @acl_stat_dir :2 u8,
  @capt_flag :1 u8,
  @reassemble :1 u8,
  @from_vrp :1 u8,
  @ike_acquire :1 u8,
  @hash_index u16 align(2),
  @rev_fwd_type u8,
  @src_inst_type u8,
  @user_group_id u16 align(2),
  @ucNetworkType u8,
  @tcp_head_length u8,
  @usSrcSecGroupId u16 align(2),
  @usDstSecGroupId u16 align(2),
  @unnamed.5998 <$unnamed.5984> implicit align(4),
  @unnamed.5999 <$unnamed.5988> implicit align(4),
  @unnamed.6000 <$unnamed.5990> implicit align(4),
  @logic_send_port u32 align(4),
  @bwm_rule_index <[4] u16> align(2),
  @pbr6rdflag :1 u8,
  @pbrtpdnspro :1 u8,
  @tpdnspro :1 u8,
  @cgn_portrng_fwd :1 u8,
  @hit_dslite_fullcone :1 u8,
  @hit_nat_fullcone_portrange :1 u8,
  @hit_dst_fullcone :1 u8,
  @hit_src_fullcone :1 u8,
  @nat_is_stmp :1 u8,
  @resv1 :5 u8,
  @acl_fwd_stat_flag :1 u8,
  @tunnel_acl_stat :1 u8,
  @rev_vrf u16 align(2),
  @l2_pkt_fmt u8,
  @replayCount u8,
  @ipsec_pkt_length u16 align(2),
  @stat_node u64 align(8),
  @stat6_node u64 align(8),
  @unnamed.6001 <$unnamed.5991> implicit align(8),
  @l2_ipsec_outport u32 align(4),
  @tos u32 align(4),
  @l4_hdr_offset u16 align(2),
  @um_dev_id u16 align(2),
  @nat_pool_id u16 align(2),
  @nat_pool_section_id u16 align(2),
  @verify_tag u32 align(4),
  @init_tag u32 align(4),
  @unnamed.6002 <$unnamed.5992> implicit align(4),
  @um_acs_type u8,
  @portal_index :3 u8,
  @transmit :1 u8,
  @portal_type :4 u8,
  @slb_sslflag :1 u16 align(2),
  @slb_vservid :15 u16 align(2),
  @Is_twamp :1 u16 align(2),
  @Is_twamp_reflector :1 u16 align(2),
  @twamp_sess_id :14 u16 align(2),
  @unnamed.6003 <$unnamed.5994> implicit,
  @user_mac <[6] u8>,
  @np_type u8,
  @np_flow_version u8,
  @np_flow_deny u8,
  @Is_TLSSip :1 u16 align(2),
  @ipsec_tailused :1 u16 align(2),
  @l2tp_trace_debug_flag :1 u16 align(2),
  @ipsec_df_bit :3 u16 align(2),
  @clust_pat_transfer_end :1 u16 align(2),
  @isArpNdCached :1 u16 align(2),
  @clust_issu_version :8 u16 align(2),
  @clust_node_id :4 u8,
  @clust_bg_master_node :4 u8,
  @clust_flow_need_circus :1 u8,
  @clust_first_packet :1 u8,
  @clust_flow_need_transfer :1 u8,
  @clust_flow_need_nat :1 u8,
  @clust_owner_id :4 u8,
  @flow_id_ext u64 align(8),
  @np_port_flag :8 u64 align(8),
  @cp2np_opcode :8 u64 align(8),
  @np_car_id0 :18 u64 align(8),
  @np_car_id1 :18 u64 align(8),
  @delta_car_len :8 u64 align(8),
  @np_master :1 u64 align(8),
  @np_ing_public :1 u64 align(8),
  @np_resv :2 u64 align(8),
  @ingress_hgport :8 u32 align(4),
  @ucLQId :8 u32 align(4),
  @ulStreamTag :16 u32 align(4),
  @proc_id u32 align(4),
  @opp_inst_id u8,
  @ucAgileRefresh :1 u8,
  @isCopyToSpu :1 u8,
  @decapGreHead :1 u8,
  @encapGreHead :1 u8,
  @isBfdLinkBundle :1 u8,
  @isIpv6LinkLocal :1 u8,
  @resvChar :2 u8,
  @ethHdrBak <$HpfEthHdr>,
  @rsv1 u8,
  @bsIngSAclDone :1 u8,
  @bsEgrSAclDone :1 u8,
  @bsIpv4Redirect :1 u8,
  @bsL2L3DecodedFlag :1 u8,
  @bsL3IngSAclDone :1 u8,
  @bsL3Proc :1 u8,
  @bsSAclRsv :2 u8,
  @vsiIndex u16 align(2),
  @srvSetInfo <$HpfMbufSrvSetInfo> align(4),
  @u8TxFwdifTop :4 u8,
  @u8RxFwdifTop :4 u8,
  @fromCp :2 u8,
  @bsMngIf :1 u8,
  @bsRsvHost :1 u8,
  @bsForbiddenFwd :1 u8,
  @bsHostCached :1 u8,
  @bsVpnCrossed :2 u8,
  @appPktType u8,
  @TxFwdif <[8] <* void>> align(8),
  @RxFwdif <[8] <* void>> align(8),
  @bridgeType u8,
  @hostIngressType u8,
  @appType u8,
  @hostEgressType u8,
  @vDev u16 align(2),
  @reasonCode u16 align(2),
  @subreasonCode u16 align(2),
  @bridgeId u32 align(4),
  @vlanTag <[2] <$HpfVlanBaseInfo>>,
  @vlanTagNum u8,
  @protoType u8,
  @feTxPktType u8,
  @hostDiagSwitch u8,
  @lbFactor u32 align(4),
  @tb u16 align(2),
  @tp u16 align(2),
  @fwdVlanIndex u32 align(4),
  @fabricMid u32 align(4),
  @tmMid u32 align(4),
  @pruneVlan u16 align(2),
  @specialTpId u16 align(2),
  @pruneIf u32 align(4),
  @oeIfIndex u32 align(4),
  @sysTimeMs u32 align(4),
  @pktCos u8,
  @pktPri u8,
  @ldmFlag u8,
  @moduleInfo u8,
  @l2SocketPktType u8,
  @l2SocketCauseType u8,
  @mlagId u16 align(2),
  @recvOrigPortCtrlIfIdx u32 align(4),
  @recvPortCtrlIfIdx u32 align(4),
  @l2SocketFd u32 align(4),
  @l2SocketModuleID u8,
  @l2SocketNextID u8,
  @causeId u16 align(2),
  @forceSendToLpu u8,
  @debugFlag u32 align(4),
  @msgId u8,
  @arpHrdType u16 align(2),
  @arpProtocolType u16 align(2),
  @arpHrdLen u8,
  @arpProtocolLen u8,
  @opCode u16 align(2),
  @arpSrcIp u32 align(4),
  @arpDstIp u32 align(4),
  @arpSrcMac <[6] u8>,
  @arpDstMac <[6] u8>,
  @isArpMissCache :1 u8,
  @isNdMissCache :1 u8,
  @peerlinkFlag :1 u8,
  @mlagFlag :1 u8,
  @isL3socket :1 u8,
  @plcyPktTrace :1 u8,
  @crossBroadFlag :1 u8,
  @smallNpFlag :1 u8,
  @priorityFlag u8,
  @hpsFromHpfFlag u8,
  @aiFabricFlag u8,
  @stackRsv u8,
  @dstPid u32 align(4),
  @traceAddr <* void> align(8),
  @ngsfHdrOffset u16 align(2),
  @isSlowPath u16 align(2),
  @recvTimeMic u32 align(4),
  @saidType u8,
  @tracePktFlag u8,
  @tracePktInstance u8,
  @vxlanEncapCnt u8,
  @workIfIndex u32 align(4),
  @svp u16 align(2),
  @timeStamp u32 align(4),
  @tracePrevCycle u64 align(8),
  @tcpPcb <* void> align(8)}>
type $fwd_hook_param_s <struct {
  @flow_m <* void> align(8),
  @flow_s <* void> align(8),
  @recv_if <* void> align(8),
  @send_if <* void> align(8),
  @dst_svrmap <* void> align(8),
  @src_svrmap <* void> align(8),
  @flow_sync_flag u32 align(4),
  @dispatch_code u8,
  @car_id u8,
  @flags u8,
  @fib_selected_index u8,
  @drop_index u32 align(4),
  @flow_bak_flag u32 align(4),
  @usNatPoolID u16 align(2),
  @usNatSectionID u16 align(2),
  @ucGetPortTimes u8,
  @flow_new_sesslog_flag :1 u8,
  @carnat_noport :1 u8,
  @bidnat_flag :1 u8,
  @npflow_refresh :1 u8,
  @npflow_finrst :1 u8,
  @ucReserverd :3 u8,
  @ulReserverd u16 align(2),
  @fib_query_info <* void> align(8),
  @fib_ret_info <* void> align(8),
  @srcMacEntry <* void> align(8),
  @dstMacEntry <* void> align(8),
  @bwm_policy_ret <* void> align(8),
  @twamp_rcv_stamp <* void> align(8),
  @resev64 u64 align(8)}>
type $tagFwdIf <struct {
  @common <$tagCommonInfo> align(4),
  @phy <$tagPhyInfo> align(2),
  @link <$tagLinkInfo> align(4),
  @ipv4 <$tagIpv4Info> align(4),
  @ipv6 <$tagIpv6Info> align(2),
  @mpls <$tagMplsInfo>,
  @l1IngSvc <$tagIngL1Fsvc> align(2),
  @l3IngSvc <$tagIngL3Fsvc> align(4),
  @l3EgrSvc <$tagEgrL3Fsvc>,
  @arpfIfCfg <$ArpfIfCfg> align(4),
  @l1EgrSvc <$tagEgrL1Fsvc> align(2),
  @Qos <$tagSrvInfo> align(4),
  @Tunnel <$tagTunnelInfo> align(4),
  @ns <$tagNsInfo> align(4),
  @stat <$FwdIfStat> align(8),
  @multiCoreStat <$FwdIfMultiCoreStat> align(64),
  @flowfwdIfIdx u32 align(4),
  @flag u32 align(4),
  @portPairIdx u32 align(4),
  @subFwdIfNum <$HpeAtom32> align(4),
  @subFwdIf <* <* <$tagFwdIf>>> align(8),
  @decodeLinkType u8,
  @outLinkType u8,
  @outPhyType u8,
  @state u8,
  @vrId u32 align(4),
  @portSecEnable :1 u32 align(4),
  @sacPktRecvDir u8,
  @resv23 :23 u32 align(4),
  @rwlock <$HpeRwlock> align(4),
  @name <[64] u8>,
  @res1 <* void> align(8),
  @ddos_appdata <* void> align(8),
  @atkAppData <* void> align(8),
  @res2 <* void> align(8),
  @bwm_appdata <* void> align(8),
  @res3 <* void> align(8),
  @res4 <* void> align(8),
  @defend_appdata <* void> align(8),
  @resv5 u8,
  @loopTmTp u8,
  @loopTmQid u16 align(2),
  @protoQid u16 align(2),
  @protoLoopQid u16 align(2),
  @srvmanage_value u8,
  @redirect_next_hop u32 align(4),
  @vsiIndex u16 align(2),
  @res6 u16 align(2),
  @ulGateWay u32 align(4),
  @res7 u32 align(4),
  @res8 u32 align(4),
  @l2Idx u32 align(4),
  @slotId u8,
  @redirectPerPkt u8,
  @redirectNextHop6 <$HpfIn6Addr>,
  @res9 <[4] u32> align(4),
  @res10 <[8] u32> align(4),
  @phyFwdIfIdx u32 align(4),
  @brId u32 align(4),
  @version u32 align(4),
  @sacCountId u16 align(2),
  @rsv u16 align(2)}>
type $HpfPortIpv4Node <struct {
  @ipAddr u32 align(4),
  @ipMask u32 align(4),
  @ipType u32 align(4),
  @version u32 align(4),
  @resv1 u32 align(4)}>
type $HpfPortIpv6Node <struct {
  @ipv6Addr <$HpfIn6Addr>,
  @prefix u32 align(4),
  @ipv6Type u32 align(4),
  @version u32 align(4),
  @resv1 u32 align(4)}>
type $HpfIn6Addr <struct pack(1) {
  @u6_addr <$unnamed.5877>}>
type $unnamed.3371 <union {
  @addr <* void> align(8),
  @addr_64 u64 align(8)}>
type $HpfFlowKey <union {
  @detail <$unnamed.3448> align(4),
  @raw <$unnamed.5359> align(8)}>
type $unnamed.3448 <struct {
  @srcMac <[6] u8>,
  @dstMac <[6] u8>,
  @ethType u16 align(2),
  @bridgeId u16 align(2),
  @unnamed.4623 <$unnamed.4624> implicit align(4),
  @unnamed.4617 <$unnamed.4618> implicit align(4),
  @vrfIndex u16 align(2),
  @protocol u8,
  @isL2 :1 u8,
  @reserved :7 u8,
  @srcPort u16 align(2),
  @dstPort u16 align(2)}>
type $HpfMbufFrameFlag <struct {
  @qindex :4 u16 align(2),
  @color :2 u16 align(2),
  @bfd :1 u16 align(2),
  @ipv6 :1 u16 align(2),
  @l2Bridge :1 u16 align(2),
  @l2Leaning :1 u16 align(2),
  @ipv6From100c :1 u16 align(2),
  @selectedIfFwd :1 u16 align(2),
  @selectedIfFwdFirst :1 u16 align(2),
  @ipsecCar :1 u16 align(2),
  @ipsecEspEncShort :1 u16 align(2),
  @ipsecEspDecShort :1 u16 align(2),
  @selectedPhyIfFwd :1 u16 align(2),
  @specialMac :1 u16 align(2),
  @trustIf :1 u16 align(2),
  @timeStampValid :1 u16 align(2),
  @rsv :12 u16 align(2)}>
type $HpfMbufSrvSetInfo <struct {
  @bsUnicastFirst :1 u32 align(4),
  @bsFeRxIfIndex :1 u32 align(4),
  @bsDefaultVlanTaged :1 u32 align(4),
  @bsPingTimeStamp :1 u32 align(4),
  @bsNoLinkEncap :1 u32 align(4),
  @bsPktIsOurs :1 u32 align(4),
  @bsPktIsUcast :1 u32 align(4),
  @bsPktIsBcast :1 u32 align(4),
  @bsPktIsMcast :1 u32 align(4),
  @bsAddTagFlag :1 u32 align(4),
  @bsSkipCheckPv :1 u32 align(4),
  @bsQinqCpy :1 u32 align(4),
  @bsPktFwdType :2 u32 align(4),
  @bsIgnoreMtu :1 u32 align(4),
  @bsL2IfIgnoreVlan :1 u32 align(4),
  @bsSpecialDstMac :1 u32 align(4),
  @bsSpecialSrcMac :1 u32 align(4),
  @bsIgnoreIfState :1 u32 align(4),
  @bsSpecialVlan :1 u32 align(4),
  @bsOeOnlyOutVlan :1 u32 align(4),
  @bsOeCopyAllVlan :1 u32 align(4),
  @bsSpecialSrcL2If :1 u32 align(4),
  @bsTunnelInfo :1 u32 align(4),
  @bsExtVxlanInfo :1 u32 align(4),
  @bsFwdVxlanInfo :1 u32 align(4),
  @bsSndSockKey :1 u32 align(4),
  @bsPktIsCrossBoard :1 u32 align(4),
  @bsNdIgnoreDad :1 u32 align(4),
  @bsRsv :3 u32 align(4)}>
type $frame_param_s <struct {
  @egress_port u32 align(4),
  @nd_index u32 align(4),
  @unnamed.6107 <$unnamed.6106> implicit align(8),
  @out_port <* void> align(8),
  @vrfIndex :15 u16 align(2),
  @v6_gateway_flag :1 u16 align(2),
  @frame_flag <$HpfMbufFrameFlag> align(2),
  @vlan_if_id u16 align(2),
  @mtu :14 u16 align(2),
  @lpu_type :2 u16 align(2),
  @inner_label u32 align(4),
  @lsp_token u32 align(4),
  @ingress_port u32 align(4),
  @resv u32 align(4),
  @ucresv u8,
  @mac_version u8,
  @mac_address <[6] u8>}>
type $HpfTcpHdr <struct pack(1) {
  @usSrcPort u16 align(2),
  @usDstPort u16 align(2),
  @seqSeqNumber u32 align(4),
  @seqAckNumber u32 align(4),
  @ucX2 :4 u8,
  @ucOffset :4 u8,
  @ucFlags u8,
  @usWnd u16 align(2),
  @usCheckSum u16 align(2),
  @usUrgentPoint u16 align(2)}>
type $HpfArpEntry <struct {
  @stKey <$HpfArpKey> align(2),
  @stData <$HpfArpData> align(4),
  @version u32 align(4),
  @resv u32 align(4)}>
type $HpfArpTblEntry <struct {
  @stKey <$HpfArpKey> align(2),
  @stData <$HpfArpTblData> align(4),
  @version u32 align(4),
  @resv u32 align(4)}>
type $tagCAP_ND_ENTRY_S <struct {
  @stKey <$HpfNdKey> align(4),
  @stData <$HpfNdData> align(4),
  @version u32 align(4),
  @resv u32 align(4)}>
type $HpeLcoreMask <struct {
  @bits <[1] u64> align(8)}>
type $HpeRcuHeadType <struct {
  @next <* <$HpeRcuHeadType>> align(8),
  @func <* <func (<* <$HpeRcuHeadType>>) void>> align(8)}>
type $tagHpeRbtNode <struct {
  @parent <* <$tagHpeRbtNode>> align(8),
  @lchild <* <$tagHpeRbtNode>> align(8),
  @rchild <* <$tagHpeRbtNode>> align(8),
  @color i32 align(4),
  @dbg_key u64 align(8)}>
type $tagHpeRbTree <struct {
  @root <* <$tagHpeRbtNode>> align(8)}>
type $tagHpeDpHashTable <struct {
  @ulHashArraySize u32 align(4),
  @ulNodeSize u32 align(4),
  @ulNodeNum u32 align(4),
  @pstHashArrayBaseAddr <* <$tagDP_HASHARRAY_S>> align(8),
  @pstNodeBaseAddr <* <$tagHpeDpHashBucket>> align(8),
  @pstFreeListHead <* <$tagHpeDpHashBucket>> align(8),
  @pstFreeListTail <* <$tagHpeDpHashBucket>> align(8),
  @ulCurUsedNodeNum u32 align(4),
  @ulError u32 align(4),
  @ullAgeTimeId u64 align(8),
  @ulTimerState u32 align(4),
  @FreeListLock <$HpeRwlock> align(4)}>
type $tagHpeDpHashBucket <struct {
  @nextNode <* <$tagHpeDpHashBucket>> align(8),
  @preNode <* <$tagHpeDpHashBucket>> align(8)}>
type $unnamed.3670_57_2 <union {
  @__f f32 align(4),
  @__i u32 align(4)}>
type $unnamed.3671_63_2 <union {
  @__f f64 align(8),
  @__i u64 align(8)}>
type $HpfAclRuleIdSet <struct {
  @numRules u32 align(4),
  @ruleIndices <* u32> align(8),
  @threshold u32 align(4),
  @cmnSegCode u32 align(4),
  @ruleStat <* <$HpfAclRuleStat>> align(8)}>
type $HpfAclRuleStat <struct {
  @value <[6] u64> align(8),
  @mask <[6] u64> align(8),
  @validBit <[6] u64> align(8),
  @validMask <[6] u64> align(8),
  @validLen u32 align(4),
  @tid u32 align(4),
  @validSegCode u32 align(4)}>
type $HpfAclRuleList <struct {
  @len u32 align(4),
  @first u32 align(4),
  @maxLen u32 align(4),
  @holeList <* u32> align(8),
  @data <* u32> align(8)}>
type $HpfAclLinkList <struct {
  @len u64 align(8),
  @head <* <$tagHpfAclListNode>> align(8),
  @tail <* <$tagHpfAclListNode>> align(8)}>
type $HpfAclJumpTable <struct {
  @numRules u32 align(4),
  @numSubTrees u32 align(4),
  @notEmptySubTrees u32 align(4),
  @commonSegNode u32 align(4),
  @subBinTree <* <$HpfAclBinTree>> align(8),
  @ruleStat <* <$HpfAclRuleStat>> align(8),
  @cutbitMask <[6] u64> align(8),
  @pos <[6][64] u32> align(4),
  @bucketSize u16 align(2),
  @maxBinTreeDepth u16 align(2),
  @updDupThreshold u16 align(2),
  @cutbitsNum u16 align(2),
  @isEmpty u16 align(2),
  @jumpTableId u16 align(2),
  @cutBits <[15] u16> align(2),
  @searchJumptable <* <$tag_acl_search_jumptable>> align(8)}>
type $HpfAclSearchTable <struct {
  @rcu <$HpeRcuHeadType> align(8),
  @aclEntries u32 align(4),
  @keyWidth u16 align(2),
  @keyByLen u16 align(2),
  @numJumpTable u32 align(4),
  @numDtRules u16 align(2),
  @ruleUintSize u16 align(2),
  @dtHighPriority u32 align(4),
  @dtRules <* u8> align(8),
  @jumpTables <* <$tag_acl_search_jumptable>> align(8)}>
type $HpfAclTable <struct {
  @searchTable <$HpfAclSearchTable> align(8),
  @memPoolId u32 align(4),
  @tid :16 u32 align(4),
  @isEmpty :2 u32 align(4),
  @bucketSize :14 u32 align(4),
  @maxBinTreeDepth u16 align(2),
  @updDupThreshold u16 align(2),
  @keyLen u16 align(2),
  @numSeg u16 align(2),
  @nextTableId u32 align(4),
  @numNode u32 align(4),
  @maxNumJumpTable u16 align(2),
  @numJumpTable u16 align(2),
  @ruleUnitSize u32 align(4),
  @numRules u32 align(4),
  @ruleIdBv <[320] u64> align(8),
  @segcodeWeight <[20] u32> align(4),
  @numSegCode u32 align(4),
  @numGraphData u32 align(4),
  @numDtRules u32 align(4),
  @dtRules <* <$HpfAclRule>> align(8),
  @pickedRuleIds <$HpfAclLinkList> align(8),
  @graph <* void> align(8),
  @graphDataSet <* void> align(8),
  @segCodeClass <* void> align(8),
  @CBRule_To_VMRule <* <func (<* void>,<* <$HpfAclRule>>) void>> align(8),
  @Print_CBRule <* <func (<* <$HpfAclRule>>) void>> align(8),
  @jumpTables <* void> align(8)}>
type $HpfAclRule <struct {
  @data <$tagHpfAclSearchRule> align(8),
  @paddingData <[28] u64> align(8),
  @keyLen u32 align(4),
  @rangeMask <[4] u8>,
  @jumpTableId u16 align(2),
  @ruleId u32 align(4),
  @subTreeRuleId u32 align(4),
  @segCode u32 align(4),
  @subTreeId u32 align(4)}>
type $HpfAclNodeResource <struct {
  @expanded u32 align(4),
  @depth u32 align(4),
  @expDepth u32 align(4),
  @ruleNum u32 align(4),
  @bvRuleLen u32 align(4),
  @bvCutbitsLen u32 align(4),
  @cutbitsNum u32 align(4),
  @bvRules <* u64> align(8),
  @bvCutBits <[6] u64> align(8),
  @node <* void> align(8),
  @ruleSet <* void> align(8)}>
type $HpfAclBinTree <struct {
  @bucketSize u16 align(2),
  @tid u8,
  @isEmpty u8,
  @jumpTableId u8,
  @updIsNew u8,
  @treeId u32 align(4),
  @root <* <$tagAclBinNode>> align(8),
  @searchRoot <* <$tagAclSearchNode>> align(8),
  @statInfo <$HpfAclBinTreeStats> align(4),
  @treeRuleSet <* <$HpfAclRuleList>> align(8),
  @ruleStat <* <$HpfAclRuleStat>> align(8),
  @updInfo <* <$HpfAclLeafUpdInfo>> align(8)}>
type $tagAclBinNode <struct {
  @tid u16 align(2),
  @nodeDepth u16 align(2),
  @numSubIntrs u32 align(4),
  @treeId u32 align(4),
  @jtId u16 align(2),
  @posInArray u16 align(2),
  @nodeType u32 align(4),
  @bitIdx u16 align(2),
  @numRules u16 align(2),
  @child <[2] <* <$tagAclBinNode>>> align(8),
  @ruleIds <* u32> align(8),
  @rules <* <$tagHpfAclSearchRule>> align(8),
  @searchNode <* <$tagAclSearchNode>> align(8)}>
type $tagHpfAclSearchRule <struct {
  @priority u32 align(4),
  @index u32 align(4),
  @keyByLen u8,
  @numRanges u8,
  @tid u8,
  @keyType u8,
  @rsv u32 align(4),
  @key <[1] u64> align(8)}>
type $HpfAclLeafUpdInfo <struct {
  @leaf <* <$tagAclBinNode>> align(8),
  @parent <* <$tagAclBinNode>> align(8),
  @cutBit u32 align(4),
  @treeRuleId u32 align(4),
  @notEmpty u32 align(4)}>
type $HpfAclTableArrayState <struct {
  @updArrayId u32 align(4),
  @searchArrayId u32 align(4)}>
type $HpeCapFwdtblDeepOps <struct {
  @insert <* <func (<* <$HpeFwdTblInfo>>,<* void>,<* void>,<* <* void>>) u32>> align(8),
  @del <* <func (<* <$HpeFwdTblInfo>>,<* void>,<* void>) u32>> align(8),
  @update <* <func (<* <$HpeFwdTblInfo>>,<* void>,<* void>,<* void>) u32>> align(8),
  @search <* <func (<* <$HpeFwdTblInfo>>,<* void>,<* <* void>>) u32>> align(8),
  @init <* <func (<* <$HpeFwdTblInfo>>) u32>> align(8),
  @exit <* <func (<* <$HpeFwdTblInfo>>) u32>> align(8),
  @match <* <func (<* <$HpeFwdTblInfo>>,<* void>,<* <* void>>) u32>> align(8)}>
type $HpeCapTbmCb <struct {
  @phyTblIdSpec u32 align(4),
  @pstPhyTblAddr <* <$HpeFwdTblInfo>> align(8)}>
type $HpeTbmRoot <struct {
  @tbmCb <[8] <$HpeCapTbmCb>> align(8),
  @memLock <$HpeSpinlock> align(4),
  @memList <$tagHpeListHead> align(8),
  @tbmMemCnt <$HpeAtom32> align(4),
  @blockNum u32 align(4),
  @tbmOpCnt <[54] <$HpeTbmOperate>> align(4),
  @tbmOpFailCnt <[54] <$HpeTbmOperate>> align(4),
  @tbmFailRet <[54] <$HpeTbmOperate>> align(4)}>
type $HpeFwdTblInfo <struct {
  @tblCfg <$HpeFwdTblDef> align(2),
  @tblSpec <$HpeFwdTblSpec> align(8),
  @entNum u32 align(4),
  @pBaseAddr <* void> align(8),
  @unnamed.6130 <$unnamed.6129> implicit align(4)}>
type $HpeFwdtblTravContext <struct pack(4) {
  @tableName u32 align(4),
  @index1 u32 align(4),
  @index2 u32 align(4),
  @index3 u32 align(4)}>
type $HpeTableLinearCmpParam <struct {
  @equal <* <func (<* void>,<* void>) u1>> align(8),
  @param <* void> align(8)}>
type $HpeTableEmListCmpParam <struct {
  @equal <* <func (<* void>,<* void>) u1>> align(8),
  @param <* void> align(8)}>
type $HpeTableEmListHead <struct {
  @list <$tagHpeListHead> align(8),
  @lock <$HpeRwlock> align(4)}>
type $HpeFwdTblSpec <struct {
  @tableName <* i8> align(8),
  @tblSpec u32 align(4),
  @specExt u32 align(4),
  @hashSize u32 align(4),
  @verifyEnable u32 align(4)}>
type $HpeSrvmapField <struct {
  @ifIndex u16 align(2),
  @oldPort u16 align(2),
  @u8Protocol u8,
  @u8Rsv u8,
  @vpn u16 align(2),
  @otherIp u32 align(4),
  @oldIp u32 align(4)}>
type $HpeCapFibKey <struct {
  @prefix u16 align(2),
  @vpn u16 align(2),
  @dip <[4] u8>}>
type $tagCAP_RE_ENTRY_S <struct {
  @valid :1 u8,
  @natPool :1 u8,
  @easyIp :1 u8,
  @defRoute :1 u8,
  @bgpRoute :1 u8,
  @dirRoute :1 u8,
  @voiceMr :1 u8,
  @sslvpn :1 u8,
  @unnamed.6022 <$unnamed.6015> implicit,
  @magicNum u16 align(2),
  @voiceRoute :1 u32 align(4),
  @maskLen :6 u32 align(4),
  @vcLabel :20 u32 align(4),
  @ttlMode :1 u32 align(4),
  @expAppoint :1 u32 align(4),
  @exp :3 u32 align(4),
  @nhpIndex u32 align(4),
  @nhpIndex2 u32 align(4),
  @siteNhpIndex u32 align(4),
  @unnamed.6023 <$unnamed.6018> implicit align(4),
  @cibIdx u32 align(4),
  @frrIndex u32 align(4),
  @reIndex u32 align(4),
  @l3Vpn :1 u32 align(4),
  @vpnVni :24 u32 align(4),
  @ipsecUnrRoute :1 u32 align(4),
  @fwFlag :2 u32 align(4),
  @spr :1 u32 align(4),
  @vcLabelValid :1 u32 align(4),
  @resv1 :2 u32 align(4),
  @version u32 align(4),
  @unnamed.6024 <$unnamed.6021> implicit align(8),
  @fwdFunc <* <func (<* <$Ipv4FwdCache>>,<* <$tagCAP_RE_ENTRY_S>>,<* <$HpfNextHopEntry>>) void>> align(8),
  @fibKey <$HpeCapFibKey> align(2)}>
type $CAP_FIB6_KEY_S <struct {
  @prefix u16 align(2),
  @vpn u16 align(2),
  @dip <[16] u8>}>
type $tagCAP_RE6_ENTRY_S <struct {
  @valid :1 u8,
  @opcode :3 u8,
  @mulNhp :1 u8,
  @defRoute :1 u8,
  @bgpRoute :1 u8,
  @dirRoute :1 u8,
  @maskLen u8,
  @vpnId u16 align(2),
  @nhp6Index u32 align(4),
  @magicNum u16 align(2),
  @fwFlag :2 u16 align(2),
  @vcLabelValid :1 u16 align(2),
  @pppoe :1 u16 align(2),
  @frrEnable :1 u16 align(2),
  @srv6Func :5 u16 align(2),
  @reserved :6 u16 align(2),
  @unnamed.6041 <$unnamed.6037> implicit align(4),
  @re6Index u32 align(4),
  @version u32 align(4),
  @unnamed.6042 <$unnamed.6040> implicit align(8),
  @fwdFunc <* <func (<* <$tagIpv6Cache>>,<* <$tagCAP_RE6_ENTRY_S>>,<* <$tagHpfNhp6Entry>>) void>> align(8),
  @fib6Key <$CAP_FIB6_KEY_S> align(2)}>
type $HpeAclGroup <struct {
  @unnamed.6133 <$unnamed.6132> implicit align(8),
  @aclTableTemp <$HpfAclTable> align(8),
  @pSearchTable <* <$HpfAclSearchTable>> align(8),
  @ruleSet <$HpfAclRuleSet> align(8),
  @updHistoryArray <$HpfAclUpdHistory> align(8),
  @actionIndex <$HpeFwdTblInfo> align(8),
  @instanceId u32 align(4),
  @groupId u32 align(4),
  @groupType u16 align(2),
  @keyWidth u16 align(2),
  @version u32 align(4),
  @resv u32 align(4)}>
type $HpeAclRuleBinaryEntry <struct {
  @priority :16 u32 align(4),
  @groupType :16 u32 align(4),
  @data u32 align(4),
  @version u32 align(4),
  @field <[0] <$HpeAclRuleBinaryField>> align(2)}>
type $HpeMscTctlTravContext <struct pack(4) {
  @tbmTravContext <$HpeFwdtblTravContext> align(4),
  @respDataLen u32 align(4),
  @continueFlag u32 align(4),
  @maxDataLen u32 align(4),
  @oneDataLen u32 align(4),
  @matchEn u32 align(4),
  @param1 u32 align(4),
  @param2 u32 align(4),
  @param3 u32 align(4),
  @param4 u32 align(4),
  @param5 u32 align(4),
  @param6 u32 align(4),
  @param7 u32 align(4),
  @param8 u32 align(4),
  @param9 u32 align(4),
  @param10 u32 align(4),
  @param11 u32 align(4),
  @param12 u32 align(4)}>
type $HpeFwdTblDef <struct {
  @tblName u16 align(2),
  @tblAttr u16 align(2),
  @keyLen u16 align(2),
  @dataLen u16 align(2)}>
type $HpeCapFwdtblOps <struct {
  @init <* <func (<* <$HpeFwdTblInfo>>) u32>> align(8),
  @exit <* <func (<* <$HpeFwdTblInfo>>) u32>> align(8),
  @insert <* <func (<* <$HpeFwdTblInfo>>,<* void>,<* void>,<* <* void>>) u32>> align(8),
  @del <* <func (<* <$HpeFwdTblInfo>>,<* void>,<* void>) u32>> align(8),
  @search <* <func (<* <$HpeFwdTblInfo>>,<* void>,<* <* void>>) u32>> align(8),
  @clean <* <func (<* <$HpeFwdTblInfo>>) u32>> align(8),
  @entNum <* <func (<* <$HpeFwdTblInfo>>,<* u32>) u32>> align(8),
  @travNext <* <func (<* <$HpeFwdTblInfo>>,<* <$HpeFwdtblTravContext>>,<* u8>,<* u32>) u32>> align(8),
  @verifyAge <* <func (u32,<* <$HpeFwdTblInfo>>,<* u32>) u32>> align(8),
  @update <* <func (<* <$HpeFwdTblInfo>>,<* void>,<* void>,<* void>) u32>> align(8),
  @match <* <func (<* <$HpeFwdTblInfo>>,<* void>,<* <* void>>) u32>> align(8)}>
type $HpfGlobalCfg <struct {
  @sysMac <[6] u8>,
  @sysMacEnd <[6] u8>,
  @macNum u32 align(4),
  @lmFwdIfIndex u32 align(4),
  @lmactiveIfIndex u32 align(4),
  @lmbackupIfIndex u32 align(4),
  @dscp u8,
  @qosInner u8,
  @natArpReplyEnable :1 u8,
  @ndFastReplyEnable :1 u8,
  @icmpFastEnable :1 u8,
  @isolateEnable :1 u8,
  @dscpEnable :1 u8,
  @qosInnerEnable :1 u8,
  @isPipeLine :1 u8,
  @vlanPriEnable :1 u8,
  @vlanPri u8,
  @eNpNum u16 align(2),
  @pipeLineId u8,
  @qosWithoutIfg :1 u8,
  @dualGates :1 u8,
  @arpReplyEnable :1 u8,
  @bfdEnable :1 u8,
  @hostCapture :1 u8,
  @srv6Enable :1 u8,
  @dhcpClientEnable :1 u8,
  @dhcpRelayEnable :1 u8,
  @recvArpRequestNum u64 align(8),
  @sendArpReplyNum u64 align(8),
  @mirrorPktLen u32 align(4),
  @hostcarEnable u32 align(4),
  @evpnFlowAggPeriod u32 align(4),
  @srv6SrcAddr <[4] u32> align(4),
  @srv6PathMtu u16 align(2),
  @srv6ReservedMtu u16 align(2),
  @srv6ActiveMtu u16 align(2),
  @resv u16 align(2),
  @fwdifNull0 u32 align(4),
  @fwdifLo u32 align(4)}>
type $HpfAclIpv4Key <struct {
  @dstIpAddr u32 align(4),
  @srcIpAddr u32 align(4),
  @srcPort u16 align(2),
  @dstPort u16 align(2),
  @vrfIndex u16 align(2),
  @resv u16 align(2),
  @protocol u8,
  @tos u8,
  @tcpFlag u8,
  @icmpType u8,
  @icmpCode u8,
  @igmpType u8,
  @ttlExpired u8,
  @valid u8,
  @fragFlag u32 align(4),
  @resv1 u32 align(4)}>
type $HpfAclL2Key <struct {
  @dstMac <[6] u8>,
  @srcMac <[6] u8>,
  @ethType u16 align(2),
  @outerVlan u16 align(2),
  @innerVlan u16 align(2),
  @doubleTag u8,
  @valid u8,
  @encapType u32 align(4)}>
type $HpeAclStatData <struct {
  @rwlock <$HpeRwlock> align(4),
  @priorityList <* <$HpeAclPriorityNode>> align(8),
  @stat <[1] <$unnamed.6131>> align(8)}>
type $HpfMbuftrCarExtNode <struct {
  @enable u32 align(4),
  @sampleLen u32 align(4),
  @dftExtToken i32 align(4),
  @extToken i32 align(4),
  @carExtType u32 align(4),
  @lock <$HpeRwlock> align(4),
  @sampleArray <* <$HpfMbuftrCarExtSampleNode>> align(8)}>
type $HpfMbuftrCarExtSpec <struct {
  @sampleLen u32 align(4),
  @carToken i32 align(4),
  @carExtType u32 align(4)}>
type $HpfFrag4CopyInfo <struct {
  @protocol u8,
  @reassemble u8,
  @vrf_index u16 align(2),
  @src_ip u32 align(4),
  @dst_ip u32 align(4),
  @pkt_id u16 align(2),
  @vlan_id u16 align(2),
  @src_port u16 align(2),
  @dst_port u16 align(2),
  @cpe_ip <$HpfIn6Addr>,
  @frag4_pkt <* <$hpe_mbuf> align(64)> align(8),
  @offset u16 align(2),
  @frag4_fifo u8}>
type $HpfFrag4Cfg <struct {
  @frag4Mode u8,
  @frag4Fifo u8,
  @aclNum u16 align(2),
  @aclGroupId u32 align(4),
  @maxCacheEach u16 align(2),
  @frag4Ttl u32 align(4),
  @forceReass u8,
  @vsysIndex u16 align(2)}>
type $HpfFrag6CopyInfo <struct {
  @protocol u8,
  @reassemble u8,
  @vrf_index u16 align(2),
  @src_ip <$HpfIn6Addr>,
  @dst_ip <$HpfIn6Addr>,
  @pkt_id u32 align(4),
  @vlan_id u16 align(2),
  @src_port u16 align(2),
  @dst_port u16 align(2),
  @frag6_pkt <* <$hpe_mbuf> align(64)> align(8),
  @frag6_fifo u8,
  @offset u16 align(2)}>
type $HpfFrag6Cfg <struct {
  @frag6Mode u8,
  @frag6Fifo u8,
  @aclNum u16 align(2),
  @aclGroupId u32 align(4),
  @maxCacheEach u16 align(2),
  @frag6Ttl u32 align(4),
  @forceReass u8,
  @vsysIndex u16 align(2)}>
type $HpfInAddr <struct pack(1) {
  @s_ulAddr u32 align(4)}>
type $HpfArpKey <struct {
  @u16Rsv u16 align(2),
  @u16Vpn u16 align(2),
  @Nexthop <[4] u8>}>
type $HpfNdKey <struct {
  @u16Vpn u32 align(4),
  @l3IfIdx u32 align(4),
  @Nexthop <[16] u8>}>
type $HpfTracePktStat <struct {
  @pktNum u64 align(8),
  @delaySum u64 align(8),
  @delayMax u64 align(8)}>
type $HpfIbcHead <struct {
  @ibcDir u32 align(4),
  @traceFlag <$HpfIbcTraceFlag>,
  @mbuftrMod u32 align(4),
  @unnamed.5971 <$unnamed.5970> implicit align(4)}>
type $WatchPointInfo <struct {
  @start u64 align(8),
  @max u64 align(8),
  @min u64 align(8),
  @avg u64 align(8),
  @total u64 align(8),
  @pktCnt u64 align(8)}>
type $LcoreWatch <struct {
  @enable u32 align(4),
  @begin u64 align(8),
  @end u64 align(8),
  @threshold u64 align(8),
  @watPoint <[35] <$WatchPointInfo>> align(8)}>
type $HpfCmdMsgHead <struct {
  @msgId u16 align(2),
  @data u16 align(2),
  @flag u16 align(2),
  @tblType u16 align(2),
  @value u32 align(4)}>
type $HpeCli <struct {
  @buf <* i8> align(8),
  @len u32 align(4),
  @maxLen u32 align(4)}>
type $tagHpeDiagFunc <struct {
  @name <* i8> align(8),
  @fCmdhelp <* <func (<* <$tagHpeDiagCmdInfo>>,<* i8>,u32,i32,<* <* i8>>) void>> align(8),
  @cmdNum u32 align(4),
  @cmds <* <$tagHpeDiagCmdInfo>> align(8)}>
type $tagHpeDiagCmdInfo <struct {
  @name <* i8> align(8),
  @cb <* <func (<* <$tagHpeDiagCmdInfo>>,<* i8>,u32,i32,<* <* i8>>) void>> align(8),
  @param_num u32 align(4),
  @params <[10] i32> align(4)}>
type $HpfFlowPrivate <struct {
  @next u64 align(8),
  @offset u8,
  @fwdType u8,
  @refreshId u8,
  @oppInstId u8,
  @privateFlags u32 volatile align(4),
  @vlanSvnParam <$unnamed.5364> align(4),
  @packets u32 align(4),
  @bytes u64 align(8),
  @fwdKey <$HpfFwdKey> align(8),
  @ext u64 align(8),
  @fwdParam <$unnamed.5007> align(8),
  @trafficMerge :48 u64 align(8),
  @lastPps :16 u64 align(8)}>
type $HpfFlowPublic <struct {
  @publicFlags u32 volatile align(4),
  @rwlock <$HpeRwlock> align(4),
  @srcMac <[6] u8>,
  @dstMac <[6] u8>,
  @ethType u16 align(2),
  @bridgeId u16 align(2),
  @unnamed.4615 <$unnamed.4616> implicit align(4),
  @unnamed.4621 <$unnamed.4622> implicit align(4),
  @srcPort u16 align(2),
  @dstPort u16 align(2),
  @obvVrfIndex u16 align(2),
  @revVrfIndex u16 align(2),
  @protocol u8,
  @varCursor u8,
  @outZone u16 align(2),
  @securityPolicyId :18 u32 align(4),
  @slb :1 u32 align(4),
  @icmpEchoreplyFast :1 u32 align(4),
  @vlan :12 u32 align(4),
  @ttl u32 align(4),
  @hitTime u32 align(4),
  @createTime u32 align(4),
  @umNodeIndex :20 u32 align(4),
  @onlineIpChecked :1 u32 align(4),
  @onlineAppChecked :1 u32 align(4),
  @migrateFlag :1 u32 align(4),
  @migrateDoneFlag :1 u32 align(4),
  @pingProxyFlag :1 u32 align(4),
  @isRedirectPerPkt :1 u32 align(4),
  @isL2 :1 u32 align(4),
  @reservedBit :5 u32 align(4),
  @unnamed.5225 <$unnamed.5226> implicit,
  @appId u8,
  @appIdExt u16 align(2),
  @obvHashIndex :11 u32 align(4),
  @revHashIndex :11 u32 align(4),
  @tcpProxyStep :2 u32 align(4),
  @varSize :8 u32 align(4),
  @recvPort u32 align(4),
  @natNext u64 align(8),
  @inZone u16 align(2),
  @backTime u16 align(2),
  @varOffset <$var_offset_s>,
  @resverd <[3] u32> align(4)}>
type $HpfFlowNatInfo <struct {
  @ip u32 align(4),
  @port u16 align(2),
  @poolId u16 align(2),
  @rangId u16 align(2),
  @resv1 u8,
  @version u8,
  @isStaticmap :1 u16 align(2),
  @espNatUpdated :1 u16 align(2),
  @origNodeid :4 u16 align(2),
  @resv2 :10 u16 align(2),
  @type u16 align(2)}>
type $var_offset_s <struct {
  @nge u8,
  @srcNat u8,
  @dstNat u8,
  @split u8,
  @nat64 u8,
  @dslite u8,
  @unnamed.5027 <$unnamed.5028> implicit,
  @misc u8}>
type $unnamed.4616 <union {
  @srcIpv6 <$HpfIn6Addr>,
  @srcIp u32 align(4)}>
type $unnamed.4618 <union {
  @dstIp u32 align(4),
  @dstIpv6 <$HpfIn6Addr>}>
type $unnamed.4622 <union {
  @dstIpv6 <$HpfIn6Addr>,
  @dstIp u32 align(4)}>
type $unnamed.4624 <union {
  @srcIp u32 align(4),
  @srcIpv6 <$HpfIn6Addr>}>
type $HpfFlowSplitInfo <struct {
  @oppHitTime u32 align(4),
  @oppPackets u32 align(4),
  @oppBytes u64 align(8)}>
type $HpfFlowPrivateExt <struct {
  @vpnTunnelParam <$unnamed.5325> align(4),
  @unnamed.5639 <$unnamed.5640> implicit align(2),
  @parentProtocol u8,
  @resvered u8,
  @parentPortParam <$unnamed.5346> align(4),
  @unnamed.5339 <$unnamed.5340> implicit align(4),
  @agileSrcGroup u16 align(2),
  @agileDstGroup u16 align(2),
  @quotaProfileId u16 align(2),
  @gtpRefreshId :14 u16 align(2),
  @agileSrcUser :1 u16 align(2),
  @agileDstUser :1 u16 align(2),
  @stickySession :1 u32 align(4),
  @stickyHitted :3 u32 align(4),
  @smartRtType :4 u32 align(4),
  @pbrPolicyId :24 u32 align(4),
  @imLogFlag :18 u32 align(4),
  @tcpHasPkt :1 u32 align(4),
  @resvered1 :13 u32 align(4),
  @tcpExceptSeq u32 align(4),
  @maxDelayMicSec u32 align(4),
  @avgDelayMicSec u32 align(4)}>
type $HpfFlowMiscInfo <struct {
  @slbRserverId u16 align(2),
  @xffFlag :1 u16 align(2),
  @aspfDerived :1 u16 align(2),
  @resflag :14 u16 align(2),
  @userService u32 align(4),
  @umTag <$unnamed.5547> align(4),
  @slbPersisAlgo u16 align(2),
  @slbVserverId u16 align(2),
  @reserved <[2] u64> align(8)}>
type $HpfFlowTcpInfo <struct {
  @synSeqNum u32 align(4),
  @specialTtl u32 align(4),
  @activeBlk :1 u32 align(4),
  @javaBlk :1 u32 align(4),
  @rvs :30 u32 align(4),
  @reserved u32 align(4)}>
type $HpfFlowShowStatRetMsg <struct {
  @flowCurNum u32 align(4),
  @flowCreateSpeed u32 align(4),
  @maxFlowNum u32 align(4),
  @maxFlowCreateSpeed u32 align(4),
  @maxFlowNumTime u64 align(8),
  @maxFlowCreateSpeedTime u64 align(8),
  @maxFlowCpuMemInfo <$HpfFlowStatCpuAndMemRecord> align(8)}>
type $tagALARM_COMM_ATTR_S <struct {
  @state u32 align(4),
  @clearType u32 align(4),
  @severity u32 align(4),
  @time <* <$tagImSystm>> align(8),
  @reasonNum u32 align(4),
  @carrierObjIndex u32 align(4),
  @res u32 align(4)}>
type $tagPacketType <struct {
  @type u16 align(2),
  @func <* <func (<* <$hpe_mbuf> align(64)>,<* <$tagPacketType>>) i32>> align(8),
  @privinfo <* void> align(8),
  @next <* <$tagPacketType>> align(8)}>
type $unnamed.5007 <union {
  @arpParam u64 align(8),
  @phyOutPort <$unnamed.5010>,
  @sswSvcParam <$unnamed.5365> align(4),
  @npFlowAdpt <$unnamed.5008> align(2),
  @perPacketNhp <$unnamed.5709> align(2)}>
type $unnamed.5008 <struct {
  @arp_refresh_id u8,
  @npFlowDeny u8,
  @rsv2 u16 align(2),
  @rsv3 u8,
  @crossvrfCp2Np :1 u8,
  @rsv4 :7 u8,
  @cp2npTime u16 align(2)}>
type $HpfFwdKey <union {
  @vlanifParam <$unnamed.5009> align(4),
  @outputParam u64 align(8)}>
type $unnamed.5009 <struct {
  @outPort u32 align(4),
  @nextHop u32 align(4)}>
type $unnamed.5010 <struct {
  @arpRefreshId u8,
  @arpFindSkip u8,
  @dstMac <[6] u8>}>
type $HpfFlowProbeInfo <struct {
  @metadataCount u32 align(4),
  @flowprobeWhiteFlow :1 u32 align(4),
  @needLayer3Metadata :1 u32 align(4),
  @needLayer7Metadata :1 u32 align(4),
  @dnsIpFilter :1 u32 align(4),
  @flowprobeProcess :1 u32 align(4),
  @needEcaMetadata :1 u32 align(4),
  @flowprobeEcaProcess :1 u32 align(4),
  @rvs :25 u32 align(4),
  @flowId u64 align(8),
  @obvHistogramPointer u64 align(8),
  @revHistogramPointer u64 align(8),
  @secetaInfo u64 align(8),
  @reassInfo u64 align(8)}>
type $unnamed.5028 <union {
  @ddos u8,
  @probe u8}>
type $HpfFlowDsliteInfo <struct {
  @fromCpe <$HpfIn6Addr>,
  @toCpe <$HpfIn6Addr>,
  @fromCpeTunnel u16 align(2),
  @toCpeTunnel u16 align(2),
  @obvCpeNexthop6 <$HpfIn6Addr>,
  @revCpeNexthop6 <$HpfIn6Addr>,
  @sessLimit :1 u16 align(2),
  @_rvs7 :15 u16 align(2),
  @_rvs16 u16 align(2)}>
type $HpfFlowNat64Info <struct {
  @v6SrcIp <$HpfIn6Addr>,
  @v6DstIp <$HpfIn6Addr>,
  @v6SrcPort u16 align(2),
  @v6DstPort u16 align(2),
  @patType u8,
  @resv1 u8,
  @resv2 u16 align(2)}>
type $HpfFlowDdosInfo <struct {
  @buffer <[64] u8>}>
type $unnamed.5226 <union {
  @tcpInfo u8,
  @sctpInfo u8,
  @udpInfo u8,
  @icmpInfo u8}>
type $HpfFlowSctpInfo <struct {
  @verifyTag u32 align(4),
  @oppVerifyTag u32 align(4),
  @ackReceived u32 align(4),
  @reserved u32 align(4)}>
type $HpfFlowUdpInfo <struct {
  @dnsAgingTime u32 align(4),
  @reserved u32 align(4)}>
type $unnamed.5325 <union {
  @ipsecTunnel u32 align(4),
  @l2tpParam <$unnamed.5330> align(2),
  @ipsecParam <$unnamed.6145> align(4),
  @mplsParam <$unnamed.5670> align(4)}>
type $unnamed.5330 <struct {
  @l2tpCpuId u8,
  @l2tpReserved u8,
  @l2tpId u16 align(2)}>
type $unnamed.5340 <union {
  @unnamed.5341 <$unnamed.5342> implicit align(4),
  @unnamed.5454 <$unnamed.5455> implicit align(4)}>
type $unnamed.5342 <struct {
  @tcpNextSeq u32 align(4),
  @tcpNextAck u32 align(4),
  @tcpSeqAdjust i32 align(4),
  @tcpAckAdjust i32 align(4),
  @tcpSeqPrevAdjust i32 align(4),
  @tcpAckPrevAdjust i32 align(4)}>
type $unnamed.5346 <union {
  @imNumber u32 align(4),
  @parentPort <$unnamed.5347> align(2)}>
type $unnamed.5347 <struct {
  @parentSrcPort u16 align(2),
  @parentDstPort u16 align(2)}>
type $unnamed.5359 <struct {
  @val1 u64 align(8),
  @val2 u64 align(8)}>
type $unnamed.5364 <union {
  @s <$unnamed.5445> align(2),
  @vlanOutPort u32 align(4)}>
type $unnamed.5365 <struct {
  @sswRsv1 u32 align(4),
  @macIdx u32 align(4)}>
type $HpfFlowNgeInfo <struct {
  @cpuId u8,
  @chnId u8,
  @flags u16 align(2),
  @detectDir :2 u16 align(2),
  @qos_remark :6 u16 align(2),
  @repolicy :1 u16 align(2),
  @hasUrl :1 u16 align(2),
  @clustId :4 u16 align(2),
  @needCircus :1 u16 align(2),
  @reservedBit1 :1 u16 align(2),
  @bypassVer u16 align(2),
  @rev <[2] u16> align(2),
  @decodeId u16 align(2),
  @urlcateId u16 align(2)}>
type $unnamed.5445 <struct {
  @ctrlChnId u16 align(2),
  @u <$unnamed.5446> align(2)}>
type $unnamed.5446 <union {
  @svnPktType u16 align(2),
  @ipcCtrlInstId u16 align(2)}>
type $unnamed.5455 <struct {
  @udpStatTimestamp u32 align(4),
  @udpStatRate u32 align(4),
  @unnamed.5456 <$unnamed.5457> implicit align(4)}>
type $unnamed.5457 <struct {
  @gtpPolicyId :18 u32 align(4),
  @rvs :14 u32 align(4)}>
type $unnamed.5547 <union {
  @portalPara <$tag_portal>,
  @xffIp u32 align(4),
  @umUpdateFlowIndex u16 align(2)}>
type $tag_portal <struct {
  @portalIndex u8,
  @portalType u8,
  @sslFlag u8,
  @reserved1 u8}>
type $unnamed.5640 <union {
  @sendVrf u16 align(2),
  @vrpProtocol u8}>
type $HpfFwdHookFlow <struct {
  @fwd_hook_flow_create <[7] u8>,
  @fwd_hook_flow_aging <[19] u8>,
  @fwd_hook_flow_scan <[21] u8>}>
type $unnamed.5670 <struct {
  @mplsToken u32 align(4),
  @mplsInnerLable u32 align(4)}>
type $unnamed.5709 <struct {
  @rsv5 u16 align(2),
  @directRoute :1 u16 align(2),
  @nhpGroupId :15 u16 align(2),
  @nhpMagic u8,
  @rsv6 <[3] u8>}>
type $HpfIcmpErrCfgMsg <struct {
  @redirect u8,
  @unreachable u8,
  @ttlExceeded u8,
  @res u8}>
type $HpfEthHdr <struct pack(1) {
  @dmac <[6] u8>,
  @smac <[6] u8>,
  @ethType u16 align(2)}>
type $DrvLinkCb <struct {
  @f <* <func (<* <* <$hpe_mbuf> align(64)>>,i32) void>> align(8)}>
type $HpfFunDeployPos <struct {
  @funAddr <* <func () u32>> align(8),
  @posBitmap u64 align(8)}>
type $HpeModTag <struct {
  @runq <$unnamed.6146> align(8),
  @regNext <* <$HpeModTag>> align(8),
  @name <* i8> align(8),
  @params <* i8> align(8),
  @initEntry <* <func () u32>> align(8),
  @exitEntry <* <func () u32>> align(8),
  @localInitEntry <* <func (u32) u32>> align(8),
  @localExitEntry <* <func (u32) u32>> align(8),
  @deploy u32 align(4),
  @where <[1] u64> align(8),
  @modId u32 align(4),
  @isPlugin u32 align(4),
  @initialized u32 align(4),
  @pluginHandle <* void> align(8),
  @pluginInitStage2 <* <func () void>> align(8),
  @regPri u32 align(4),
  @dumpCb <* <func (<* i8>,u32,u32) i32>> align(8)}>
type $max_align_t <struct {
  @__ll i64 align(8),
  @__ld f64 align(16)}>
type $unnamed.5844 <struct {
  @__reserved1 i64 align(8),
  @__reserved2 i64 align(8)}>
type $HpeHeapMng <struct {
  @lock <$HpeSpinlock> align(4),
  @isCreate u32 align(4),
  @nodeList <$HPE_DLL> align(8),
  @freeSize u64 align(8)}>
type $tagHpeMemZonelCluster <struct {
  @lock <$HpeSpinlock> align(4),
  @isCreate u32 align(4),
  @zoneMaxNum u32 align(4),
  @virtStartAddr <* void> align(8),
  @freeSize u64 align(8),
  @nodeList <$HPE_DLL> align(8),
  @HpeMemzonePool <[1] <$tagHpeMemzone> incomplete_array> align(8)}>
type $unnamed.5845 <union {
  @__i <[14] i32> align(4),
  @__vi <[14] i32> volatile align(4),
  @__s <[7] u64> align(8)}>
type $unnamed.5846 <union {
  @__i <[10] i32> align(4),
  @__vi <[10] i32> volatile align(4),
  @__p <[5] <* void volatile>> volatile align(8)}>
type $unnamed.5847 <union {
  @__i <[12] i32> align(4),
  @__vi <[12] i32> volatile align(4),
  @__p <[6] <* void>> align(8)}>
type $unnamed.5848 <union {
  @__i <[14] i32> align(4),
  @__vi <[14] i32> volatile align(4),
  @__p <[7] <* void>> align(8)}>
type $unnamed.5849 <union {
  @__i <[8] i32> align(4),
  @__vi <[8] i32> volatile align(4),
  @__p <[4] <* void>> align(8)}>
type $HpePipeBlock <struct {
  @fds <[2] i32> align(4),
  @name <* i8> align(8),
  @flg u32 align(4),
  @werrn u32 align(4),
  @rerrn u32 align(4)}>
type $tagHpeEventCallback <struct {
  @activeNext <$unnamed.5850> align(8),
  @flags u16 align(2),
  @priority u8,
  @pad u8,
  @lastSeqno u32 align(4),
  @func <$tagHpeEventCbFunc> align(8)}>
type $unnamed.5850 <struct {
  @tqe_next <* <$tagHpeEventCallback>> align(8),
  @tqe_prev <* <* <$tagHpeEventCallback>>> align(8)}>
type $HpeEventCallbackList <struct {
  @tqh_first <* <$tagHpeEventCallback>> align(8),
  @tqh_last <* <* <$tagHpeEventCallback>>> align(8)}>
type $tagHpeEventOps <structincomplete {}>
type $unnamed.5851 <union {
  @curSeqno u64 align(8),
  @wakeCnt u64 align(8)}>
type $HpeMempoolMemChunk <struct {
  @addr <* void> align(8),
  @unnamed.5854 <$unnamed.5853> implicit align(8),
  @len u64 align(8),
  @reserve u32 align(4)}>
type $unnamed.5853 <union {
  @iova u64 align(8),
  @physAddr u64 align(8)}>
type $unnamed.5855 <union {
  @mpType i32 align(4),
  @poolId i32 align(4)}>
type $HpeMempoolObjhdr <struct {
  @node <$tagHpeListHead> align(8),
  @mp <* <$tagHpeMempool> align(64)> align(8),
  @unnamed.5858 <$unnamed.5857> implicit align(8)}>
type $unnamed.5857 <union {
  @iova u64 align(8),
  @physAddr u64 align(8)}>
type $HPE_MEMPOOL_MEM <struct {
  @gva <* void> align(8),
  @gpa u64 align(8),
  @hpa u64 align(8),
  @totalSize u32 align(4),
  @freePos u32 align(4),
  @mplock <$HpeSpinlock> align(4)}>
type $HPE_MEMPOOL_USER <struct {
  @addr u64 align(8),
  @name <[32] i8>,
  @size u64 align(8)}>
type $unnamed.5859 <union {
  @hash <$unnamed.5860> align(4)}>
type $unnamed.5860 <union {
  @rss u32 align(4),
  @fdir <$unnamed.5861> align(4),
  @txadapter <$unnamed.5866> align(4),
  @usr u32 align(4)}>
type $unnamed.5861 <struct {
  @unnamed.5865 <$unnamed.5862> implicit align(4),
  @hi u32 align(4)}>
type $unnamed.5862 <union {
  @unnamed.5864 <$unnamed.5863> implicit align(2),
  @lo u32 align(4)}>
type $unnamed.5863 <struct {
  @hash u16 align(2),
  @id u16 align(2)}>
type $unnamed.5866 <struct {
  @reserved1 u32 align(4),
  @reserved2 u16 align(2),
  @txq u16 align(2)}>
type $unnamed.5867 <union {
  @vlan_tci_outer u16 align(2),
  @dsa_info u8,
  @high_pri u8}>
type $unnamed.5868 <union {
  @tx_offload u64 align(8),
  @unnamed.5870 <$unnamed.5869> implicit align(8)}>
type $unnamed.5869 <struct {
  @l2_len :7 u64 align(8),
  @l3_len :9 u64 align(8),
  @l4_len :8 u64 align(8),
  @tso_segsz :16 u64 align(8),
  @outer_l3_len :9 u64 align(8),
  @outer_l2_len :7 u64 align(8),
  @tx_mode :1 u64 align(8),
  @groupid :7 u64 align(8)}>
type $unnamed.5874 <struct {
  @type u32 align(4),
  @slotId u32 align(4),
  @portId u32 align(4),
  @pad1 u32 align(4)}>
type $unnamed.5875 <struct {
  @pad2 u16 align(2),
  @causeId u16 align(2),
  @appType u32 align(4)}>
type $tagHpeHugePageInfo <struct {
  @hugepageName <[128] i8>,
  @hugepageSize u32 align(4)}>
type $tagHpeProcMeminfo <struct {
  @procName <[64] i8>,
  @virtualMem u64 align(8),
  @phyMem u64 align(8),
  @shmMem u64 align(8),
  @cache u64 align(8),
  @buffer u64 align(8),
  @memAvial u64 align(8),
  @pid u32 align(4)}>
type $HpeMbufControl <struct {
  @startAddr u64 align(8),
  @endAddr u64 align(8)}>
type $HpeLfhashTimerStat <struct {
  @init_ok u1,
  @create_times u32 align(4),
  @cb_exec_ok u32 align(4),
  @err_init u32 align(4),
  @err_cbi_null u32 align(4),
  @err_cb_null u32 align(4),
  @err_signo u32 align(4)}>
type $flock <struct {
  @l_type i16 align(2),
  @l_whence i16 align(2),
  @l_start i64 align(8),
  @l_len i64 align(8),
  @l_pid i32 align(4)}>
type $f_owner_ex <struct {
  @type i32 align(4),
  @pid i32 align(4)}>
type $HpfEthHdrTail <struct pack(1) {
  @typeLen u16 align(2),
  @dsap u8,
  @ssap u8,
  @ctrl u8,
  @ori <[3] u8>,
  @type u16 align(2)}>
type $HpfEthHdrSap <struct pack(1) {
  @typeLen u16 align(2),
  @sap u16 align(2)}>
type $HpfEthVlanHdr <struct pack(1) {
  @dmac <[6] u8>,
  @smac <[6] u8>,
  @ethType u16 align(2),
  @vlanId u16 align(2),
  @type u16 align(2)}>
type $HpfVlanBaseInfo <struct pack(1) {
  @vid :12 u16 align(2),
  @cfi :1 u16 align(2),
  @pri :3 u16 align(2)}>
type $HpfVlanHdr <struct pack(1) {
  @pri_cfi_vid u16 align(2),
  @type u16 align(2)}>
type $Hpf802Hdr <struct pack(1) {
  @dsap u8,
  @ssap u8,
  @ctrl u8}>
type $HpfEthSnapHdr <struct pack(1) {
  @dmac <[6] u8>,
  @smac <[6] u8>,
  @len u16 align(2),
  @dsap u8,
  @ssap u8,
  @ctrl u8,
  @ori <[3] u8>,
  @type u16 align(2)}>
type $HpfEthLlcHdr <struct pack(1) {
  @dsap u8,
  @ssap u8,
  @ctrl u8,
  @ori <[3] u8>,
  @type u16 align(2)}>
type $HpfArpHdr <struct pack(1) {
  @hrdAddrFormat u16 align(2),
  @proAddrFormat u16 align(2),
  @hrdAddrLen u8,
  @proAddrLen u8,
  @opType u16 align(2)}>
type $HpfEthArpHdr <struct pack(1) {
  @arpHdr <$HpfArpHdr>,
  @arpSrcMacAddr <[6] u8>,
  @arpSrcProAddr <[4] u8>,
  @arpDstMacAddr <[6] u8>,
  @arpDstProAddr <[4] u8>}>
type $HpfEthDot1qHdr <struct pack(1) {
  @destAddr <[6] u8>,
  @srcAddr <[6] u8>,
  @tpID u16 align(2),
  @TCI u16 align(2),
  @typeLen u16 align(2)}>
type $HpfDotqHdr <struct pack(1) {
  @ethType u16 align(2),
  @vlanId u16 align(2)}>
type $HpfEthQinqHdr <struct pack(1) {
  @dMacH32 u32 align(4),
  @dMacL16 u16 align(2),
  @sMacH32 u32 align(4),
  @sMacL16 u16 align(2),
  @qinqVlanType u16 align(2),
  @qinqVlan u16 align(2),
  @vlanType u16 align(2),
  @Vlan u16 align(2),
  @ethType u16 align(2)}>
type $HpfEthLswTag <struct pack(1) {
  @cpuTagType u16 align(2),
  @protocol u8,
  @reason u8,
  @portMaskTx :15 u32 align(4),
  @allow :1 u32 align(4),
  @vlanIdx :5 u32 align(4),
  @disaleLearning :1 u32 align(4),
  @vlanIdxEn :1 u32 align(4),
  @keep :1 u32 align(4),
  @priority :3 u32 align(4),
  @priorityEn :1 u32 align(4),
  @enhancedFId :3 u32 align(4),
  @eFId :1 u32 align(4)}>
type $HpfEthLswHdr <struct pack(1) {
  @dmac <[6] u8>,
  @smac <[6] u8>,
  @lswTag <$HpfEthLswTag>}>
type $HpfIcmp6Hdr <struct pack(1) {
  @type u8,
  @code u8,
  @chkSum u16 align(2),
  @data <$unnamed.5876>}>
type $unnamed.5876 <union pack(1) {
  @data32 <[1] u32> align(4),
  @data16 <[2] u16> align(2),
  @data8 <[4] u8>}>
type $HpfIcmp6NaHdr <struct {
  @type u8,
  @code u8,
  @chkSum u16 align(2),
  @icmpV6Flag u32 align(4),
  @tarAdd <[4] u32> align(4),
  @opType u8,
  @length u8,
  @dstMac <[6] u8>}>
type $HpfIcmp6NsHdr <struct {
  @type u8,
  @code u8,
  @chkSum u16 align(2),
  @res u32 align(4),
  @tarAdd <[4] u32> align(4),
  @opType u8,
  @length u8,
  @srcMac <[6] u8>}>
type $unnamed.5877 <union pack(1) {
  @u6_ucaddr <[16] u8>,
  @u6_usaddr <[8] u16> align(2),
  @u6_uladdr <[4] u32> align(4),
  @u6_ulladdr <[2] u64> align(8)}>
type $HpfIn46Addr <struct pack(1) {
  @ucVersion u8,
  @ucRes <[3] u8>,
  @uaddr <$unnamed.5878>}>
type $unnamed.5878 <union pack(1) {
  @st6Addr <$HpfIn6Addr>,
  @st4Addr <$HpfInAddr>}>
type $HpfIp6Ext <struct pack(1) {
  @ip6e_ucnxt u8,
  @ip6e_uclen u8}>
type $HpfIp6Hbh <struct pack(1) {
  @ip6h_ucnxt u8,
  @ip6h_uclen u8}>
type $HpfIp6Route <struct pack(1) {
  @ip6r_ucnxt u8,
  @ip6r_uclen u8,
  @ip6r_uctype u8,
  @ip6r_ucsegleft u8}>
type $HpfIp6SrhHdr <struct pack(1) {
  @ucNextHeader u8,
  @ucLength u8,
  @ucRoutingType u8,
  @ucSegmentsLeft u8,
  @ucLastEntry u8,
  @ucFlags u8,
  @usTag u16 align(2),
  @astSegmentList <[1] <$HpfIn6Addr>>}>
type $HpfIp6Dest <struct pack(1) {
  @ip6d_ucnxt u8,
  @ip6d_uclen u8}>
type $HpfIp6FragHdr <struct pack(1) {
  @ip6f_ucnxt u8,
  @ip6f_ucreserved u8,
  @ip6f_usofflg u16 align(2),
  @ip6f_ulident u32 align(4)}>
type $HpfIp6AhHdr <struct pack(1) {
  @ucNxtHdr u8,
  @ucLength u8}>
type $HpfNdNsPkt <struct pack(1) {
  @stNSHdr <$HpfIcmp6Hdr>,
  @stNSTarget <$HpfIn6Addr>}>
type $HpfNdOptLla <struct pack(1) {
  @ucType u8,
  @ucLength u8,
  @ucLLAddr <[6] u8>}>
type $HpfNdRsPkt <struct pack(1) {
  @stRSHdr <$HpfIcmp6Hdr>}>
type $HpfNdNaPkt <struct pack(1) {
  @stNAHdr <$HpfIcmp6Hdr>,
  @stNATarget <$HpfIn6Addr>}>
type $unnamed.5879 <struct pack(1) {
  @uc_ip6_tclass1 :4 u8,
  @uc_ip6_ver :4 u8,
  @uc_ip6_res :4 u8,
  @uc_ip6_tclassL :4 u8}>
type $HpfUdpHdr <struct pack(1) {
  @uh_usSPort u16 align(2),
  @uh_usDPort u16 align(2),
  @uh_sULen u16 align(2),
  @uh_usSum u16 align(2)}>
type $HpfDnsHdr <struct pack(1) {
  @transId u16 align(2),
  @rD :1 u8,
  @tC :1 u8,
  @aA :1 u8,
  @opcode :4 u8,
  @qR :1 u8,
  @rcode :4 u8,
  @zero :3 u8,
  @rA :1 u8,
  @questions u16 align(2),
  @answerRRs u16 align(2),
  @authorityRRs u16 align(2),
  @additionalRRs u16 align(2)}>
type $HpfDhcpCHdrS <struct {
  @op u8,
  @htype u8,
  @hlen u8,
  @hops u8,
  @xid u32 align(4),
  @secs u16 align(2),
  @flags u16 align(2),
  @ciaddr <$HpfInAddr>,
  @yiaddr <$HpfInAddr>,
  @siaddr <$HpfInAddr>,
  @giaddr <$HpfInAddr>,
  @chaddr <[16] u8>,
  @sname <[64] i8>,
  @file <[128] i8>,
  @options <[1200] i8>}>
type $HpfIcmpHdr <struct pack(1) {
  @icmpType u8,
  @icmpCode u8,
  @icmpCksum u16 align(2),
  @icmpHun <$unnamed.5880>,
  @icmpDun <$unnamed.5883>}>
type $unnamed.5880 <union pack(1) {
  @ihPPtr u8,
  @ihGwAddr <$HpfInAddr>,
  @ihIdSeq <$unnamed.5881>,
  @ihNVoid i32 align(4),
  @ihPMtu <$unnamed.5882>}>
type $unnamed.5881 <struct pack(1) {
  @icdNsId u16 align(2),
  @icdNsSeq u16 align(2)}>
type $unnamed.5882 <struct pack(1) {
  @ipmNsVoid u16 align(2),
  @ipmNsNextMtu u16 align(2)}>
type $unnamed.5883 <union pack(1) {
  @idTS <$unnamed.5884>,
  @idIp <$unnamed.5885>,
  @idMask u32 align(4),
  @idDataA <[1] i8>}>
type $unnamed.5884 <struct pack(1) {
  @itsNtOTime u32 align(4),
  @itsNtRTime u32 align(4),
  @itsNtTTime u32 align(4)}>
type $unnamed.5885 <struct pack(1) {
  @idiIp <$HpfIpHdr>}>
type $HpeTmwjob <struct {
  @next u32 align(4),
  @pre u32 align(4),
  @jobcb <* <func (u32,<* void>) u32>> align(8),
  @pParam <* void> align(8),
  @tmw u8,
  @bkt u8,
  @loop :1 u16 align(2),
  @rsv :15 u16 align(2),
  @timerid u32 align(4),
  @skipts u32 align(4),
  @seq u32 align(4)}>
type $HpeTmwbaseStruct <structincomplete {}>
type $HpeTmwInstance <struct {
  @timerId u8,
  @rev <[3] u8>,
  @jobId u32 align(4)}>
type $HpfNatParam <struct {
  @inIfIndex u16 align(2),
  @outIfIndex u16 align(2),
  @inZone u16 align(2),
  @outZone u16 align(2),
  @srcVpn u16 align(2),
  @dstVpn u16 align(2),
  @nextHop u32 align(4),
  @unnamed.5888 <$unnamed.5886> implicit align(4)}>
type $unnamed.5886 <union {
  @preNatParam u32 align(4),
  @afterNatParam <$unnamed.5887> align(4)}>
type $unnamed.5887 <struct {
  @newSip u32 align(4),
  @newDip u32 align(4),
  @newSport u16 align(2),
  @newDport u16 align(2),
  @newSrcVpn u16 align(2),
  @newDstVpn u16 align(2),
  @oldSip u32 align(4),
  @oldDip u32 align(4),
  @oldSport u16 align(2),
  @oldDport u16 align(2)}>
type $HpfNat6Param <struct {
  @inIfIndex u16 align(2),
  @outIfIndex u16 align(2),
  @inZone u16 align(2),
  @outZone u16 align(2),
  @srcVpn u16 align(2),
  @dstVpn u16 align(2),
  @rsv u32 align(4)}>
type $HpfNatParamRet <struct {
  @doNat u8,
  @skipPolicy u8,
  @rsv16 u16 align(2),
  @newSip u32 align(4),
  @newDip u32 align(4),
  @newSport u16 align(2),
  @newDport u16 align(2),
  @newVpn u16 align(2),
  @fatherProtocol u8,
  @fatherSport u16 align(2),
  @fatherDport u16 align(2),
  @appId u32 align(4)}>
type $HpfNat6ParamRet <struct {
  @doNat u8,
  @skipPolicy u8,
  @rsv16 u16 align(2),
  @newSip <[4] u32> align(4),
  @newDip <[4] u32> align(4),
  @newSport u16 align(2),
  @newDport u16 align(2)}>
type $HpfNatSuccessParam <struct {
  @rsv32 u32 align(4)}>
type $HpfNatAgingParam <struct {
  @zone u16 align(2),
  @rsv u32 align(4)}>
type $unnamed.5889 <union {
  @unnamed.5892 <$unnamed.5890> implicit align(2),
  @unnamed.5893 <$unnamed.5891> implicit align(2)}>
type $unnamed.5890 <struct {
  @sPort u16 align(2),
  @dPort u16 align(2)}>
type $unnamed.5891 <struct {
  @icmpId u16 align(2),
  @type u8,
  @code u8}>
type $HpfSAclActionCarInfo <struct {
  @colorMode :1 u8,
  @carType :1 u8,
  @rsv1 :6 u8,
  @layeredCar u8,
  @rsv2 u16 align(2),
  @lock <$HpeSpinlock> align(4),
  @cir i64 align(8),
  @pbs i64 align(8),
  @cbs i64 align(8),
  @pir i64 align(8),
  @ts i64 align(8),
  @tc i64 align(8),
  @tpte i64 align(8)}>
type $HpfCarColorAction <struct {
  @permit :1 u8,
  @rmkDot1p :1 u8,
  @rmkDscp :1 u8,
  @statEn :1 u8,
  @rmkExp :1 u8,
  @rsv1 :3 u8,
  @rmkExpVal :3 u8,
  @rmkDot1pVal :3 u8,
  @rsv2 :2 u8,
  @rmkDscpVal :6 u8,
  @rsv3 :2 u8,
  @rsv4 u8}>
type $HpfSAclActionCarAction <struct {
  @colorAction <[3] <$HpfCarColorAction>>}>
type $HpfSAclActionCarCoreInfo <struct {
  @ttlInitVal u8,
  @color u8,
  @rsv8 u8,
  @ttl u8,
  @pktLen u32 align(4),
  @pktCnt u32 align(4)}>
type $HpfSAclActionCarStat <struct {
  @coreInfo <$HpfSAclActionCarCoreInfo> align(4),
  @bytes u64 align(8),
  @pkts u64 align(8),
  @unnamed.5899 <$unnamed.5895> implicit align(8),
  @unnamed.5904 <$unnamed.5900> implicit align(8)}>
type $unnamed.5895 <union {
  @unnamed.5898 <$unnamed.5896> implicit align(8),
  @color <[3] <$unnamed.5897>> align(8)}>
type $unnamed.5896 <struct {
  @greenPkts u64 align(8),
  @yellowPkts u64 align(8),
  @redPkts u64 align(8),
  @greenBytes u64 align(8),
  @yellowBytes u64 align(8),
  @redBytes u64 align(8)}>
type $unnamed.5897 <struct {
  @pkts u64 align(8),
  @bytes u64 align(8)}>
type $unnamed.5900 <union {
  @unnamed.5903 <$unnamed.5901> implicit align(8),
  @access <[2] <$unnamed.5902>> align(8)}>
type $unnamed.5901 <struct {
  @dropPkts u64 align(8),
  @passPkts u64 align(8),
  @dropBytes u64 align(8),
  @passBytes u64 align(8)}>
type $unnamed.5902 <struct {
  @pkts u64 align(8),
  @bytes u64 align(8)}>
type $HpfSAclActionCar <struct {
  @carInfo <$HpfSAclActionCarInfo> align(8),
  @carAction <$HpfSAclActionCarAction>,
  @carStat <[1] <$HpfSAclActionCarStat> align(64)> align(64)}>
type $HpfSAclActionRemark <struct {
  @enable u8,
  @remark8021p u8,
  @remarkDscp u8,
  @remarkLp u8,
  @remarkExp u8,
  @remarkIn8021p u8,
  @remarkQosGroup u8,
  @new8021p u8,
  @newDscp u8,
  @newFrDe u8,
  @newExp u8,
  @newIn8021p u8,
  @newQosGroup u8,
  @newLp u8}>
type $HpfSAclActionMirror <struct {
  @enable u8,
  @resv u8,
  @observeId u16 align(2)}>
type $HpfSAclActionKey <struct {
  @group_id u32 align(4),
  @group_type u32 align(4)}>
type $HpfSAclActionRedirect <struct {
  @enable u8,
  @nhpEn u8,
  @nhpv6En u8,
  @toCp u8,
  @ipv4NhpIpAddr u32 align(4),
  @ipv4NhpIndex u32 align(4),
  @ipv6NhpIndex u32 align(4),
  @toPort u8,
  @reserved1 u8,
  @reserved2 u8}>
type $HpfSAclActionData <struct {
  @nextGroup u8,
  @res8 u8,
  @res16 u16 align(2),
  @redirect <$HpfSAclActionRedirect> align(4),
  @car <$HpfSAclActionCar> align(64),
  @remark <$HpfSAclActionRemark>,
  @mirror <$HpfSAclActionMirror> align(2)}>
type $HpfSAclActionTblEntry <struct {
  @stKey <$HpfSAclActionKey> align(4),
  @stData <$HpfSAclActionData> align(64),
  @version u32 align(4),
  @resv u32 align(4)}>
type $HpfSAclSvc <struct {
  @l2acl_filter :1 u8,
  @l2l4acl_filter :1 u8,
  @l4acl_filter :1 u8,
  @ipv6acl_filter :1 u8,
  @l2acl_stat :1 u8,
  @l2l4acl_stat :1 u8,
  @l4acl_stat :1 u8,
  @ipv6acl_stat :1 u8,
  @l2acl_redirect :1 u8,
  @l2l4acl_redirect :1 u8,
  @l4acl_redirect :1 u8,
  @ipv6acl_redirect :1 u8,
  @l2acl_remark :1 u8,
  @l2l4acl_remark :1 u8,
  @l4acl_remark :1 u8,
  @ipv6acl_remark :1 u8,
  @l2acl_car :1 u8,
  @l2l4acl_car :1 u8,
  @l4acl_car :1 u8,
  @ipv6acl_car :1 u8,
  @res20 :20 u32 align(4)}>
type $HpfSAclGroupid <struct {
  @ingL2AclFilterGid u32 align(4),
  @ingL4AclFilterGid u32 align(4),
  @ingL2AclStatGid u32 align(4),
  @ingL4AclStatGid u32 align(4),
  @ingL2AclRemarkGid u32 align(4),
  @ingL4AclRemarkGid u32 align(4),
  @ingL2AclRedirectGid u32 align(4),
  @ingL4AclRedirectGid u32 align(4),
  @ingL2AclCarGid u32 align(4),
  @ingL4AclCarGid u32 align(4),
  @egrL2AclGid u32 align(4),
  @egrL4AclGid u32 align(4),
  @egrL2AclRemarkGid u32 align(4),
  @egrL4AclRemarkGid u32 align(4),
  @egrL2AclCarGid u32 align(4),
  @egrL4AclCarGid u32 align(4),
  @egrL2AclFilterGid u32 align(4),
  @egrL4AclFilterGid u32 align(4),
  @egrL2AclStatGid u32 align(4),
  @egrL4AclStatGid u32 align(4)}>
type $HpfSAclActionApply <struct {
  @ingSvc <$HpfSAclSvc> align(4),
  @egrSvc <$HpfSAclSvc> align(4),
  @gid <$HpfSAclGroupid> align(4),
  @ingAction <$HpfSAclActionData> align(64),
  @egrAction <$HpfSAclActionData> align(64)}>
type $HpfAclIpv6Key <struct {
  @srcIpv6Addr <[4] u32> align(4),
  @dstIpv6Addr <[4] u32> align(4),
  @srcPort u16 align(2),
  @dstPort u16 align(2),
  @vrfIndex u16 align(2),
  @protocol u8,
  @tos u8,
  @tcpFlag u8,
  @icmpType u8,
  @icmpCode u8,
  @valid u8,
  @fragFlag u32 align(4)}>
type $HpfAclAction <struct {
  @ifDeny u32 align(4),
  @statId u32 align(4)}>
type $HpfVlanKey <struct {
  @vlanId u16 align(2),
  @bridgeId u16 align(2)}>
type $HpfVlanData <struct {
  @vlanId u16 align(2),
  @unnamed.5908 <$unnamed.5905> implicit align(2),
  @macLimitCycle u32 align(4),
  @bcMid u16 align(2),
  @statId u16 align(2),
  @macLimitNum u16 align(2),
  @curMacNum u16 align(2),
  @version u32 align(4)}>
type $unnamed.5905 <union {
  @flags u16 align(2),
  @unnamed.5907 <$unnamed.5906> implicit align(2)}>
type $unnamed.5906 <struct {
  @valid :1 u16 align(2),
  @macLearnEn :1 u16 align(2),
  @bcLimitEn :1 u16 align(2),
  @missUcAct :2 u16 align(2),
  @macLimitEn :1 u16 align(2),
  @macLimitFull :1 u16 align(2),
  @macLimitAct :2 u16 align(2),
  @macLimitAlarmEn :1 u16 align(2),
  @arpReplyEn :1 u16 align(2),
  @rsv1 :5 u16 align(2)}>
type $HpfVlanTbl <struct {
  @key <$HpfVlanKey> align(2),
  @data <$HpfVlanData> align(4)}>
type $VlanBitmap <struct {
  @ctrlIfIndex u32 align(4),
  @bitmap <[128] u32> align(4)}>
type $VlanUntagBitmap <struct {
  @ctrlIfIndex u32 align(4),
  @untagBitmap <[128] u32> align(4)}>
type $HpfBridgeCfg <struct {
  @brId u32 align(4),
  @flowIfIdx u32 align(4),
  @vlanFilter u8,
  @macLimitEn u8,
  @rsv <[2] u8>,
  @macAgeTime u32 align(4),
  @macLimitNum u32 align(4),
  @curMacNum u32 align(4)}>
type $HostEthHdr <struct pack(1) {
  @dmac <[6] u8>,
  @smac <[6] u8>,
  @ethType u16 align(2)}>
type $HostVlanBaseInfo <struct pack(1) {
  @vid :12 u16 align(2),
  @cfi :1 u16 align(2),
  @pri :3 u16 align(2)}>
type $tagTnlExtInfo <struct {
  @uiExtTnlFlag u32 align(4),
  @unExtInfo <$unnamed.5909> align(4)}>
type $unnamed.5909 <union {
  @uiVniId u32 align(4),
  @stVniIpInfo <$unnamed.5910> align(4)}>
type $unnamed.5910 <struct {
  @uiVniId u32 align(4),
  @uiSrcIp u32 align(4),
  @uiDstIp u32 align(4)}>
type $tagMbufTunnelIntf <struct {
  @usTlvType u16 align(2),
  @usTlvLength u16 align(2),
  @uiVRFIndex u32 align(4),
  @ucTunnelType u8,
  @aucReserved <[3] u8>,
  @uiTunnelID u32 align(4),
  @stTnlExtInfo <$tagTnlExtInfo> align(4)}>
type $tagMbufEvpnVlanIntf <struct {
  @usTlvType u16 align(2),
  @usTlvLength u16 align(2),
  @uiVrId u32 align(4),
  @uiEvpnIndex u32 align(4),
  @usVlanId u16 align(2),
  @aucPad <[2] u8>}>
type $tagMBufLinkBDIntf <struct {
  @usTlvType u16 align(2),
  @usTlvLength u16 align(2),
  @uiBdID u32 align(4),
  @usBDTag u16 align(2),
  @ucPruneType u8,
  @ucAttFlag u8,
  @ucTagPriority u8,
  @ucRsv1 u8,
  @usRsv2 u16 align(2),
  @unPrune <$unnamed.5911> align(4)}>
type $unnamed.5911 <union {
  @stAcIf <$unnamed.5912> align(4),
  @stPw <$unnamed.5913> align(4)}>
type $unnamed.5912 <struct {
  @uiPruneIf u32 align(4),
  @ausVlanId <[2] u16> align(2)}>
type $unnamed.5913 <struct {
  @uiPruneLabel u32 align(4)}>
type $HostMbufExtVxlanLbInfo <struct {
  @srcIP u32 align(4),
  @dstIP u32 align(4),
  @srcMac <[6] u8>,
  @srcPort u16 align(2),
  @dstMac <[6] u8>,
  @dstPort u16 align(2),
  @protocolId u8,
  @upFlag u8,
  @ttl u8,
  @tos u8}>
type $HostMbufExtVxlanInfo <struct {
  @extVxlanFlag u32 align(4),
  @lbInfo <$HostMbufExtVxlanLbInfo> align(4)}>
type $HostMbufTunnelInfo <struct {
  @vrfIndex u32 align(4),
  @tunnelType u8,
  @reserved <[3] u8>,
  @tunnelID u32 align(4),
  @extTnlFlag u32 align(4),
  @unExtInfo <$unnamed.5914> align(4)}>
type $unnamed.5914 <union {
  @vniIpInfo <$unnamed.5915> align(4)}>
type $unnamed.5915 <struct {
  @vniId u32 align(4),
  @srcIp u32 align(4),
  @dstIp u32 align(4)}>
type $HostMbufFwdVxlanInfo <struct {
  @serviceType u16 align(2),
  @sendType u8,
  @rsv u8}>
type $HostMbufSndSockKey <struct {
  @sockId u32 align(4),
  @hPid u32 align(4)}>
type $HostMbufVxlanInfo <struct {
  @tunnelInfo <$HostMbufTunnelInfo> align(4),
  @extVxlanInfo <$HostMbufExtVxlanInfo> align(4),
  @fwdVxlanInfo <$HostMbufFwdVxlanInfo> align(2)}>
type $HostIngressIntf <struct {
  @intfType u16 align(2),
  @intfLen u16 align(2),
  @unnamed.5920 <$unnamed.5916> implicit align(4),
  @vr u32 align(4),
  @vrfId u32 align(4),
  @recvIfIndex u32 align(4),
  @recvPhyIfIndex u32 align(4),
  @bridgeId u32 align(4),
  @appProto u8,
  @protoType u8,
  @cutHdrLen u8,
  @bsFrmPeerLink :1 u8,
  @rsv2 :3 u8,
  @bridgeType :4 u8,
  @reasonCode u16 align(2),
  @subReasonCode u16 align(2),
  @causeId u16 align(2),
  @svp u16 align(2),
  @srcMac <[6] u8>,
  @dstMac <[6] u8>,
  @vlanBaseInfo <[2] <$HpfVlanBaseInfo>>,
  @oeIfIndex u32 align(4),
  @sysTimeMs u32 align(4),
  @tunnelIntf <$tagMbufTunnelIntf> align(4),
  @evpnVlanIntf <$tagMbufEvpnVlanIntf> align(4),
  @bdIntf <$tagMBufLinkBDIntf> align(4),
  @unnamed.5921 <$unnamed.5918> implicit align(4),
  @sndSockKey <$HostMbufSndSockKey> align(4),
  @workIfIndex u32 align(4),
  @sendIfIndex u32 align(4),
  @nodeId u32 align(4),
  @rsv3 u32 align(4),
  @rsv4 u32 align(4)}>
type $unnamed.5916 <union {
  @ingressBitSet <$unnamed.5917> align(4),
  @ingressBitSetValue u32 align(4)}>
type $unnamed.5917 <struct {
  @bsAddTagFlag :1 u32 align(4),
  @bsVlanNum :2 u32 align(4),
  @bsMngIf :1 u32 align(4),
  @bsPktIsOurs :1 u32 align(4),
  @bsPktIsBcast :1 u32 align(4),
  @bsPingTimeStamp :1 u32 align(4),
  @bsTnlIntf :1 u32 align(4),
  @bsEvpnIntf :1 u32 align(4),
  @bsBdIntf :1 u32 align(4)}>
type $unnamed.5918 <union {
  @ingressExBitSet <$unnamed.5919> align(4),
  @ingressExBitSetValue u32 align(4)}>
type $unnamed.5919 <struct {
  @bsSndSockKey :1 u32 align(4),
  @bsNdIgnoreDad :1 u32 align(4),
  @bsDebugFilter :1 u32 align(4)}>
type $HostEgressIntf <struct {
  @intfType u16 align(2),
  @intfLen u16 align(2),
  @sendIfIndex u32 align(4),
  @sendPhyIfIndex u32 align(4),
  @vrId u32 align(4),
  @vrfIndex u32 align(4),
  @dstAddr <[4] u32> align(4),
  @nextHop <[4] u32> align(4),
  @unnamed.5924 <$unnamed.5922> implicit align(4),
  @unnamed.5927 <$unnamed.5925> implicit align(2),
  @oePriority u8,
  @protoType u8,
  @srcMac <[6] u8>,
  @dstMac <[6] u8>,
  @fwdPktType u8,
  @tunnelType u8,
  @oeIfIndex u32 align(4),
  @tpId u16 align(2),
  @pruneVlan u16 align(2),
  @ovIfIndex u32 align(4),
  @vlanBaseInfo <[2] <$HostVlanBaseInfo>>,
  @pruneIf u32 align(4),
  @unnamed.5929 <$unnamed.5928> implicit align(4),
  @bdId u32 align(4),
  @unnamed.5932 <$unnamed.5930> implicit align(4),
  @tunnelID u32 align(4),
  @srcAddr <[4] u32> align(4),
  @unnamed.5934 <$unnamed.5933> implicit align(8),
  @extVxlanInfo <$HostMbufExtVxlanInfo> align(4),
  @fwdVxlanInfo <$HostMbufFwdVxlanInfo> align(2),
  @sndSockKey <$HostMbufSndSockKey> align(4),
  @l2PortListId u16 align(2),
  @svp u16 align(2),
  @rsv1 u32 align(4),
  @rsv2 u32 align(4)}>
type $unnamed.5922 <union {
  @egressBitSet <$unnamed.5923> align(4),
  @egressBitSetValue u32 align(4)}>
type $unnamed.5923 <struct {
  @bsVlanNum :2 u32 align(4),
  @bsOePriority :1 u32 align(4),
  @bsOeDMAC :1 u32 align(4),
  @bsOeSMAC :1 u32 align(4),
  @bsOeMemPort :1 u32 align(4),
  @bsOeOnlyOutVlan :1 u32 align(4),
  @bsOeCopyAllVlan :1 u32 align(4),
  @bsPingTimeStamp :1 u32 align(4),
  @bsSpecialSrcL2If :1 u32 align(4),
  @bsNoLinkEncap :1 u32 align(4),
  @bsIgnoreMtu :1 u32 align(4),
  @bsIgnoreIfState :1 u32 align(4),
  @bsTrustIf :1 u32 align(4),
  @bsFromHps :1 u32 align(4)}>
type $unnamed.5925 <union {
  @linkStackBitSet <$unnamed.5926> align(2),
  @linkStackBitSetValue u16 align(2)}>
type $unnamed.5926 <struct {
  @bsOeFlag :1 u16 align(2),
  @bsOvFlag :1 u16 align(2),
  @bsOdFlag :1 u16 align(2),
  @bsOqFlag :1 u16 align(2)}>
type $unnamed.5928 <union {
  @recvIfIndex u32 align(4),
  @oobData u32 align(4)}>
type $unnamed.5930 <union {
  @egressExBitSet <$unnamed.5931> align(4),
  @egressExBitSetValue u32 align(4)}>
type $unnamed.5931 <struct {
  @bsTunnelInfo :1 u32 align(4),
  @bsExtVxlanInfo :1 u32 align(4),
  @bsFwdVxlanInfo :1 u32 align(4),
  @bsSndSockKey :1 u32 align(4),
  @bsSpecifyIpat :1 u32 align(4),
  @bsChassisCrossBoard :1 u32 align(4)}>
type $unnamed.5933 <union {
  @tunnelInfo <$HostMbufTunnelInfo> align(4),
  @tcpPcb <* void> align(8)}>
type $HpfIbcLpu2Mpu <struct {
  @debugFilter u8,
  @rsv <[3] u8>,
  @ingressIntf <$HostIngressIntf> align(4)}>
type $HpfFeTxInfo <struct {
  @pktType u16 align(2),
  @pktLen u16 align(2),
  @priority u16 align(2),
  @flag u16 align(2),
  @L2Info <$unnamed.5935> align(4),
  @extFlag u32 align(4),
  @TlvTxLen u32 align(4)}>
type $unnamed.5935 <union {
  @L2BDUC <$unnamed.5936> align(4),
  @L2UC <$unnamed.5941> align(4),
  @L2MC <$unnamed.5945> align(4),
  @stL3MC <$unnamed.5953> align(4),
  @SoftFwd <$unnamed.5961> align(2),
  @P2MPMC <$unnamed.5965> align(4),
  @Flow <$unnamed.5966> align(4),
  @FORWARDL3IPSec <$unnamed.5967> align(4),
  @AtmL2UC <$unnamed.5968> align(4)}>
type $unnamed.5936 <struct {
  @bdFid u32 align(4),
  @serviceTag u16 align(2),
  @pruneInfoType u8,
  @reservedL2BDUC u8,
  @L2BDUCPrune <$unnamed.5937> align(4)}>
type $unnamed.5937 <union {
  @L2BDUCSubIf <$unnamed.5938> align(4),
  @L2BDUCPhyIf <$unnamed.5939> align(4),
  @L2BDUCTrunkIf <$unnamed.5940> align(2)}>
type $unnamed.5938 <struct {
  @ifIndex u32 align(4),
  @vlanId <[2] u16> align(2)}>
type $unnamed.5939 <struct {
  @sbsp u32 align(4)}>
type $unnamed.5940 <struct {
  @trunkID u16 align(2),
  @reservedL2BDUCTrunkIf u16 align(2)}>
type $unnamed.5941 <struct {
  @tbtp u32 align(4),
  @dstVchannel u32 align(4),
  @fwdVlanIndex u32 align(4),
  @pruneInfoType u8,
  @reservedArr <[3] u8>,
  @UCPrune <$unnamed.5942> align(4)}>
type $unnamed.5942 <union {
  @UCPhyIf <$unnamed.5943> align(4),
  @UCTrunkIf <$unnamed.5944> align(2)}>
type $unnamed.5943 <struct {
  @sbsp u32 align(4)}>
type $unnamed.5944 <struct {
  @trunkID u16 align(2),
  @reservedUCTrunkIf u16 align(2)}>
type $unnamed.5945 <struct {
  @mid u32 align(4),
  @fabricMid u32 align(4),
  @tmMid u32 align(4),
  @pruneInfoType u8,
  @vsiAttFlag u8,
  @serviceTag u16 align(2),
  @L2MCPrune <$unnamed.5946> align(4)}>
type $unnamed.5946 <union {
  @PhyIf <$unnamed.5947> align(4),
  @Vxlan <$unnamed.5947> align(4),
  @TrunkIf <$unnamed.5948> align(2),
  @SubIf <$unnamed.5949> align(4),
  @TrunkSubIf <$unnamed.5950> align(2),
  @Label <$unnamed.5951> align(4),
  @FlowId <$unnamed.5952> align(4)}>
type $unnamed.5947 <struct {
  @sbsp u32 align(4)}>
type $unnamed.5948 <struct {
  @trunkID u16 align(2),
  @reservedTrunkIf u16 align(2)}>
type $unnamed.5949 <struct {
  @sbsp u32 align(4),
  @vlanId u16 align(2),
  @ceVlan u16 align(2)}>
type $unnamed.5950 <struct {
  @trunkID u16 align(2),
  @vlanId u16 align(2),
  @ceVlan u16 align(2),
  @resvTrunkSubIf u16 align(2)}>
type $unnamed.5951 <struct {
  @label u32 align(4)}>
type $unnamed.5952 <struct {
  @flowId u32 align(4)}>
type $unnamed.5953 <struct {
  @fvrf u32 align(4),
  @fwdVlanIndex u32 align(4),
  @fabricMid u32 align(4),
  @tmMid u32 align(4),
  @iifInfoType u8,
  @af u8,
  @reservedL3MC <[2] u8>,
  @L3MCPrune <$unnamed.5954> align(4)}>
type $unnamed.5954 <union {
  @L3MCPhyIf <$unnamed.5955> align(4),
  @L3MCTrunkIf <$unnamed.5956> align(2),
  @L3MCVlanIf <$unnamed.5957> align(4)}>
type $unnamed.5955 <struct {
  @sbsp u32 align(4)}>
type $unnamed.5956 <struct {
  @trunkID u16 align(2),
  @reservedL3MCTrunkIf u16 align(2)}>
type $unnamed.5957 <struct {
  @vlanID u16 align(2),
  @pruneInfoType u8,
  @reservedL3MCVlanIf u8,
  @L3MCVlanIfPrune <$unnamed.5958> align(4)}>
type $unnamed.5958 <union {
  @L3MCPrunePhyIf <$unnamed.5959> align(4),
  @L3MCPruneTrunkIf <$unnamed.5960> align(2)}>
type $unnamed.5959 <struct {
  @sbsp u32 align(4)}>
type $unnamed.5960 <struct {
  @trunkID u16 align(2),
  @reservedL3MCPruneTrunkIf u16 align(2)}>
type $unnamed.5961 <struct {
  @vlanNum u8,
  @mapVlanNum u8,
  @tpID u16 align(2),
  @type u8,
  @flowAction u8,
  @priorityBitMap u8,
  @resvSoftFwd u8,
  @VlanInfo <$unnamed.5962> align(2)}>
type $unnamed.5962 <union {
  @vlanInfo <[8] <$unnamed.5963>> align(2),
  @dot1qInfo <$unnamed.5964> align(2)}>
type $unnamed.5963 <struct {
  @priority :3 u16 align(2),
  @cfi :1 u16 align(2),
  @vlanID :12 u16 align(2)}>
type $unnamed.5964 <struct {
  @priority :3 u16 align(2),
  @cfi :1 u16 align(2),
  @vlanID :12 u16 align(2)}>
type $unnamed.5965 <struct {
  @fabricMid u32 align(4),
  @tmGid u32 align(4)}>
type $unnamed.5966 <struct {
  @flowId u32 align(4),
  @flag u16 align(2),
  @networkRole u8,
  @rsvFlow u8}>
type $unnamed.5967 <struct {
  @fvrf u32 align(4)}>
type $unnamed.5968 <struct {
  @tbtp u32 align(4),
  @vpi u16 align(2),
  @vci u16 align(2)}>
type $HpfIbcSpu2Mpu <struct {
  @ibcInfo <$unnamed.5969> align(4)}>
type $unnamed.5969 <union {
  @msgId u32 align(4),
  @ingressIntf <$HostIngressIntf> align(4)}>
type $HpfIbcMpu2Spu <struct {
  @msgId u32 align(4),
  @recvVrfIdx u16 align(2),
  @vrfIdx u16 align(2),
  @isLinkLocal :1 u8,
  @isOspf :1 u8,
  @ignoreMtu :1 u8,
  @resv1 :5 u8,
  @vlanPri u8,
  @innerPri u8,
  @resv2 u8,
  @ctrlIfIdx u32 align(4),
  @nhp <[4] u32> align(4),
  @oobData u32 align(4)}>
type $HpfHostPktEgressInfo <struct {
  @flag u8,
  @netId u8,
  @cos u8,
  @dataLen u32 align(4),
  @srcMod u16 align(2),
  @dstMod u16 align(2),
  @srcPort u8,
  @dstPort u8,
  @groupId u16 align(2),
  @trunkFlag u8,
  @srcTrunkId u16 align(2),
  @vrId u16 align(2),
  @srcVp u32 align(4),
  @vfi u16 align(2),
  @dmacType u8,
  @stackPort u8,
  @causeId u16 align(2),
  @txRxHeaderLen u8,
  @sysPort u32 align(4),
  @moduleId u8,
  @tagNum u8,
  @vsiId u16 align(2),
  @isVxlanEn u8,
  @isNs u8,
  @ndcId u16 align(2),
  @temId u8,
  @seqOffset u8,
  @isCrossBoard u8}>
type $HpfIbcMpu2Lpu <struct {
  @debugFilter :1 u8,
  @pingTimeStamp :1 u8,
  @bitRsv :6 u8,
  @linkLen u8,
  @protoType u8,
  @rsv <[1] u8>,
  @egressInfo <$HpfHostPktEgressInfo> align(4)}>
type $HpfIbcTraceFlag <struct {
  @traceStat :1 u8,
  @tracePrint :1 u8,
  @traceInstance :3 u8,
  @traceRsv :3 u8}>
type $unnamed.5970 <union {
  @lpu2mpu <$HpfIbcLpu2Mpu> align(4),
  @mpu2lpu <$HpfIbcMpu2Lpu> align(4),
  @spu2mpu <$HpfIbcSpu2Mpu> align(4),
  @mpu2spu <$HpfIbcMpu2Spu> align(4)}>
type $HpfMbufEspNatInfo <struct {
  @syncFlag u16 align(2),
  @srcIsStaticMap :1 u16 align(2),
  @dstIsStaticMap :1 u16 align(2),
  @res :14 u16 align(2),
  @srcPoolId u16 align(2),
  @srcRangId u16 align(2),
  @srcType u8,
  @srcVersion u8,
  @dstPoolId u16 align(2),
  @dstRangId u16 align(2),
  @dstType u8,
  @dstVersion u8,
  @mPrivateFlags u32 align(4),
  @sPrivateFlags u32 align(4),
  @publicFlags u32 align(4)}>
type $HpfMbufNgeInfo <struct {
  @pvPktInfo <* void> align(8),
  @ullNgeSessIndex u64 align(8),
  @uiDpSessVer u32 align(4),
  @uiTTL u32 align(4),
  @uiPolicyRuleID u32 align(4),
  @uiAuditRuleID u32 align(4),
  @iTcpProxyS3 i32 align(4),
  @sNatAlgDelta i16 align(2),
  @usAppTrace u16 align(2),
  @usAppID u16 align(2),
  @usDecoderID u16 align(2),
  @ulStreamDirection :1 u32 align(4),
  @bIsIPv6 :1 u32 align(4),
  @bisTcpProxySend :1 u32 align(4),
  @bisNatALG :1 u32 align(4),
  @bisNatDest :1 u32 align(4),
  @bisNatSrc :1 u32 align(4),
  @bisResetPkt :1 u32 align(4),
  @bRefreshFwdInfo :1 u32 align(4),
  @bCapture :1 u32 align(4),
  @bReassPayload :1 u32 align(4),
  @bPayloadMalloc :1 u32 align(4),
  @bIsCacheCpy :1 u32 align(4),
  @bIsInNge :1 u32 align(4),
  @bisChangedByIPS :1 u32 align(4),
  @bMailBypass :1 u32 align(4),
  @bIsIdentifiedByPM :1 u32 align(4),
  @bIsNeedSA :1 u32 align(4),
  @bIsNeedNGE :1 u32 align(4),
  @bIsNeedRepolicy :1 u32 align(4),
  @bIsFstPkt :1 u32 align(4),
  @bFWisBlacklistEnable :1 u32 align(4),
  @bIsDpDistrBypassed :1 u32 align(4),
  @bIsDpDistrBlocked :1 u32 align(4),
  @bIsHTTPSProxy :1 u32 align(4),
  @bBypassDetect :1 u32 align(4),
  @bIsSingleDetect :1 u32 align(4),
  @uiNoNeedContinue :1 u32 align(4),
  @uiResultChange :1 u32 align(4),
  @uiSAFlag :2 u32 align(4),
  @bIsSSL :1 u32 align(4),
  @bIsPassive :1 u32 align(4),
  @uTraceDebugTblId u16 align(2),
  @uTraceNGEStepRst u16 align(2),
  @bIsLwhttp :1 u32 align(4),
  @bIsTunnelMode :1 u32 align(4),
  @bIsPassThroughMode :1 u32 align(4),
  @bIsNgeCreate :1 u32 align(4),
  @bMailDetectBypass :1 u32 align(4),
  @bIsTcpProxy :1 u32 align(4),
  @bIsNgeSocket :1 u32 align(4),
  @bIsReferUrlCat :1 u32 align(4),
  @bIsIcmpUnreach :1 u32 align(4),
  @bIsIdcDetect :1 u32 align(4),
  @bIsNohanshake :1 u32 align(4),
  @bIsUdpRebound :1 u32 align(4),
  @bIsNgeMetadata :1 u32 align(4),
  @bIsMbufTracer :1 u32 align(4),
  @bIsMbufTracerId :4 u32 align(4),
  @bIsMbufTracerDir :1 u32 align(4),
  @bIsMbufTracerlage :1 u32 align(4),
  @bIsUmPortal :1 u32 align(4),
  @bIsMailProxy :1 u32 align(4),
  @uiReserve :10 u32 align(4),
  @uiParentSrcPort u16 align(2),
  @uiParentDstPort u16 align(2),
  @uiParentProtocol u8,
  @uiReserve2 <[3] u8>}>
type $HpfHostMbufRxVlanInfo <struct {
  @priority :3 u16 align(2),
  @cfi :1 u16 align(2),
  @vlanID :12 u16 align(2)}>
type $HpfHostMbufRxInfo <struct {
  @pktType u16 align(2),
  @pktLen u16 align(2),
  @priority u16 align(2),
  @reasonCode u16 align(2),
  @subReasonCode u16 align(2),
  @l3Stake u16 align(2),
  @sbsp u32 align(4),
  @src <$unnamed.5972> align(4),
  @tbtp u32 align(4),
  @dst <$unnamed.5974> align(4),
  @flag u16 align(2),
  @productInfo u16 align(2),
  @vrfIndex u32 align(4),
  @vrIndex u32 align(4),
  @extFlag u32 align(4),
  @tlvRxLen u32 align(4),
  @mtu u32 align(4)}>
type $unnamed.5972 <union {
  @srcVchannel u32 align(4),
  @vlanInfo <[2] <$HpfHostMbufRxVlanInfo>> align(2),
  @atmPvcInfo <$unnamed.5973> align(2)}>
type $unnamed.5973 <struct {
  @vpi u16 align(2),
  @vci u16 align(2)}>
type $unnamed.5974 <union {
  @dstVchannel u32 align(4),
  @vlanInfo <[2] <$HpfHostMbufRxVlanInfo>> align(2)}>
type $unnamed.5975 <union {
  @mbufTxInfo <$HpfFeTxInfo> align(4),
  @mbufRxInfo <$HpfHostMbufRxInfo> align(4)}>
type $unnamed.5976 <union {
  @src6NatIp <$HpfIn6Addr>,
  @sndSockKey <$HostMbufSndSockKey> align(4),
  @unnamed.5978 <$unnamed.5977> implicit align(4)}>
type $unnamed.5977 <struct {
  @oobData u32 align(4)}>
type $unnamed.5979 <union {
  @np_v4 <$unnamed.5980> align(4),
  @nexthop6 <$HpfIn6Addr>,
  @to_cpe_ip <$HpfIn6Addr>,
  @nat64_v6_dstip <$HpfIn6Addr>}>
type $unnamed.5980 <struct {
  @nexthop u32 align(4),
  @resv u32 align(4)}>
type $unnamed.5981 <union {
  @mpls_s <$unnamed.5982> align(4),
  @nsh_s <$unnamed.5983> align(4)}>
type $unnamed.5982 <struct {
  @mpls_inner_label u32 align(4),
  @mpls_token u32 align(4),
  @mpls_info u32 align(4)}>
type $unnamed.5983 <struct {
  @nsh_spi :24 u32 align(4),
  @nsh_si :8 u32 align(4),
  @nsh_operations :1 u32 align(4),
  @nsh_rev :31 u32 align(4)}>
type $unnamed.5984 <union {
  @v6_src_ip <$HpfIn6Addr>,
  @unnamed.5987 <$unnamed.5985> implicit align(2),
  @dhcp_s <$unnamed.5986> align(4),
  @trunk_mem_outport_index u32 align(4)}>
type $unnamed.5985 <struct {
  @svn_pkt_type u8,
  @ctrl_reserved u8,
  @ctrl_chn_id u16 align(2),
  @ips_ctrl_inst_id u16 align(2),
  @reserved <[5] u16> align(2)}>
type $unnamed.5986 <struct {
  @dhcp_dst_mac <[6] u8>,
  @usRes u16 align(2),
  @ulPortPhyIndex u32 align(4)}>
type $unnamed.5988 <union {
  @ipv6_pkt_info <$unnamed.5989> align(2),
  @esp_nat_info <$HpfMbufEspNatInfo> align(4)}>
type $unnamed.5989 <struct {
  @src_ip <$HpfIn6Addr>,
  @dst_ip <$HpfIn6Addr>,
  @hbh_offset u16 align(2),
  @dest_offset u16 align(2),
  @route_offset u16 align(2),
  @frag_offset u16 align(2)}>
type $unnamed.5990 <union {
  @ip6_token u32 align(4),
  @to_cpe_tunnel_id u16 align(2)}>
type $unnamed.5991 <union {
  @nge_info <$HpfMbufNgeInfo> align(8),
  @ipv4RedirectAddr u32 align(4)}>
type $unnamed.5992 <union {
  @portal_defaul_ip u32 align(4),
  @decrypt_rule_id u32 align(4),
  @sesslog_para <$unnamed.5993>}>
type $unnamed.5993 <struct {
  @ipv6_sess :1 u8,
  @log_type :2 u8,
  @syslog_match :1 u8,
  @sess_rsv :4 u8}>
type $unnamed.5994 <union {
  @mpls_exp u8,
  @flow_hittime_flag u8}>
type $float32x2x2_t <struct {
  @val <[2] v2i32> align(8)}>
type $SswMacKey <struct {
  @vlanId u16 align(2),
  @bridgeId u16 align(2),
  @mac <[6] u8>,
  @rsv u16 align(2)}>
type $SswMacData <struct {
  @unnamed.6007 <$unnamed.6004> implicit,
  @rsv1 <[3] u8>,
  @flowBitMap <[8] u32> align(4),
  @outIfIndex u32 align(4),
  @updateTime u32 align(4),
  @macIndex u32 align(4),
  @lastCycle u64 align(8),
  @lock <$HpeRwlock> align(4)}>
type $unnamed.6004 <union {
  @flags u8,
  @unnamed.6006 <$unnamed.6005> implicit}>
type $unnamed.6005 <struct {
  @valid :1 u8,
  @isStatic :1 u8,
  @drop :1 u8,
  @local :1 u8,
  @remote :1 u8,
  @security :1 u8,
  @aging :1 u8,
  @rsv :1 u8}>
type $SswMacIndex <struct {
  @key <$SswMacKey> align(2),
  @macIndex u32 align(4)}>
type $SswMacHash <struct {
  @key <$SswMacKey> align(2),
  @data <$SswMacData> align(8)}>
type $SswBcElbKey <struct {
  @bridgeId u16 align(2),
  @vlanId u16 align(2)}>
type $SswBcElbData <struct {
  @outFlowIfIdx u32 align(4)}>
type $SswBcElbHash <struct {
  @key <$SswBcElbKey> align(2),
  @unnamed.6009 <$unnamed.6008> implicit align(4)}>
type $unnamed.6008 <union {
  @data <[0] u8>,
  @elbData <$SswBcElbData> align(4)}>
type $SswBcDupPara <struct {
  @key <$SswBcElbKey> align(2),
  @dupNum u32 align(4),
  @inFlowIfIdx u32 align(4),
  @mbuf <* <$hpe_mbuf> align(64)> align(8)}>
type $SswVlanIfBcDupPara <struct {
  @key <$SswBcElbKey> align(2),
  @dupNum u32 align(4),
  @inFlowIfIdx u32 align(4),
  @dupTb <[4] u32> align(4),
  @dupBitMap <[4] u32> align(4),
  @outFlowIfIdx <[4] u32> align(4),
  @mbuf <* <$hpe_mbuf> align(64)> align(8)}>
type $BlackHoleMacKey <struct {
  @mac <[6] u8>,
  @rsv u16 align(2),
  @brId u32 align(4)}>
type $BlackHoleMacData <struct {
  @valid :1 u32 align(4),
  @rsv :31 u32 align(4)}>
type $BlackHoleMacHash <struct {
  @key <$BlackHoleMacKey> align(4),
  @data <$BlackHoleMacData> align(4)}>
type $SswPortIsolateKey <struct {
  @srcFwdIfIndex u32 align(4),
  @dstFwdIfIndex u32 align(4)}>
type $SswPortIsolateData <struct {
  @portIsolateEn :1 u32 align(4),
  @rsv :31 u32 align(4)}>
type $SswPortIsolateHash <struct {
  @key <$SswPortIsolateKey> align(4),
  @data <$SswPortIsolateData> align(4)}>
type $EgrFsvc <struct {
  @isL3Svc :1 u8,
  @egrSvc :7 u8,
  @fwdifStackIdx u8,
  @param1 u16 align(2),
  @param2 u32 align(4)}>
type $tagCommonInfo <struct {
  @fwdIfType u8,
  @subIfType u8,
  @enable :1 u8,
  @down :1 u8,
  @virtIf :1 u8,
  @vport :1 u8,
  @l2l3 :2 u8,
  @resv2 :2 u8,
  @cpuType u8,
  @fwdIfIndex u32 align(4),
  @fatherIndex u32 align(4),
  @ctrlIfIdx u32 align(4)}>
type $tagPhyInfo <struct {
  @phyType :4 u8,
  @wan :1 u8,
  @hitag :1 u8,
  @rsv2 :2 u8,
  @hiTagType :4 u8,
  @tagType :4 u8,
  @mod u16 align(2),
  @port u16 align(2),
  @cpuPort u8,
  @tmPhyTp u8}>
type $tagLinkInfo <struct {
  @encap <$unnamed.6064> align(2),
  @ac2pw <$unnamed.6065> align(2),
  @linkInfo <$unnamed.6066> align(4),
  @celnInfo <$unnamed.6070> align(4),
  @macInfo <$unnamed.6071> align(4)}>
type $tagIpv4Info <struct {
  @ipv4uc :1 u8,
  @ipv4mc :1 u8,
  @resmc :1 u8,
  @dhcpEn :1 u8,
  @dfForce :1 u8,
  @urpf :1 u8,
  @urpfStrict :1 u8,
  @urpfDefultRoute :1 u8,
  @rsv8 u8,
  @mtu u16 align(2),
  @vpn u16 align(2),
  @tcpmssVal u16 align(2),
  @ipAddr u32 align(4),
  @ipMask u32 align(4)}>
type $tagIpv6Info <struct {
  @ipv6uc :1 u8,
  @ipv6mc :1 u8,
  @resmc :1 u8,
  @dhcp6En :1 u8,
  @rsv1 :1 u8,
  @urpf :1 u8,
  @urpfStrict :1 u8,
  @urpfDefultRoute :1 u8,
  @rsv8 u8,
  @rsv16 u16 align(2),
  @mtu6 u16 align(2),
  @vpn6 u16 align(2),
  @gblAddr <$HpfIn6Addr>,
  @linkLocalAddr <$HpfIn6Addr>}>
type $tagMplsInfo <struct {
  @mplsuc :1 u8,
  @mplsmc :1 u8,
  @rsv6 :6 u8,
  @rsv24 <[3] u8>}>
type $tagIngL1Fsvc <struct {
  @mirror :1 u8,
  @stat :1 u8,
  @car :1 u8,
  @rsv5 :5 u8,
  @rsv8 u8,
  @rsv16 u16 align(2)}>
type $tagIngL3Fsvc <struct {
  @ipsg :1 u8,
  @gdoi :1 u8,
  @tcp_mss :1 u8,
  @sac :1 u8,
  @sacStatEn :1 u8,
  @woc :1 u8,
  @ns :1 u8,
  @ip_rate_limit :1 u8,
  @ipfpm :1 u8,
  @l2acl :1 u8,
  @l4acl :3 u8,
  @l2l4acl :1 u8,
  @ipv6acl :1 u8,
  @ip_count :1 u8,
  @ipv4_stat :1 u8,
  @ipv6_stat :1 u8,
  @aps :1 u8,
  @capt :1 u8,
  @nat :1 u8,
  @flowAggEn :1 u8,
  @l2acl_filter :1 u8,
  @l4acl_filter :1 u8,
  @l2l4acl_filter :1 u8,
  @ipv6acl_filter :1 u8,
  @l2acl_stat :1 u8,
  @l4acl_stat :1 u8,
  @l2l4acl_stat :1 u8,
  @ipv6acl_stat :1 u8,
  @l2acl_redirect :1 u8,
  @l4acl_redirect :1 u8,
  @l2l4acl_redirect :1 u8,
  @ipv6acl_redirect :1 u8,
  @l2acl_car :1 u8,
  @l4acl_car :1 u8,
  @l2l4acl_car :1 u8,
  @ipv6acl_car :1 u8,
  @l2acl_remark :1 u8,
  @l4acl_remark :1 u8,
  @l2l4acl_remark :1 u8,
  @ipv6acl_remark :1 u8,
  @l2AclMirror :1 u8,
  @l4AclMirror :1 u8,
  @l2l4AclMirror :1 u8,
  @ipv6AclMirror :1 u8,
  @rsv16 <[2] u8>,
  @rsv6 :6 u8,
  @RxL3svcTop u8,
  @RxL3svcSet <[16] <$IngFsvc>> align(4)}>
type $tagEgrL3Fsvc <struct {
  @dial :1 u8,
  @fw :1 u8,
  @ips :1 u8,
  @nat :1 u8,
  @alp :1 u8,
  @woc :1 u8,
  @sac :1 u8,
  @sacStatEn :1 u8,
  @tcp_mss :1 u8,
  @aps :1 u8,
  @ipfpm :1 u8,
  @ns :1 u8,
  @ipsec :1 u8,
  @gdoi :1 u8,
  @flowAggEn :1 u8,
  @rsv1 :1 u8,
  @rsv16 <[2] u8>}>
type $ArpfIfCfg <struct {
  @vrrpIPv4Addr u32 align(4),
  @vrrpIPv4Mask u32 align(4),
  @vrrpId u8,
  @vrrpEn :1 u8,
  @isVrrpMaster :1 u8,
  @isPassArp :1 u8,
  @arpFastReplyEn :1 u8,
  @arpInnerVlanProxyEn :1 u8,
  @arpRouteProxyEn :1 u8,
  @arpInterVlanProxyEn :1 u8,
  @vxLanDistriGateWayEn :1 u8,
  @bdArpBcastSuppressEn :1 u8,
  @resv1 :7 u8,
  @resv2 u8}>
type $tagEgrL1Fsvc <struct {
  @ip_rate_limit :1 u8,
  @l2acl :1 u8,
  @l4acl :3 u8,
  @l2l4acl :1 u8,
  @ipv6acl :1 u8,
  @l2acl_stat :1 u8,
  @l4acl_stat :1 u8,
  @l2l4acl_stat :1 u8,
  @ipv6acl_stat :1 u8,
  @l2acl_filter :1 u8,
  @l4acl_filter :1 u8,
  @l2l4acl_filter :1 u8,
  @ipv6acl_filter :1 u8,
  @l2acl_remark :1 u8,
  @l4acl_remark :1 u8,
  @l2l4acl_remark :1 u8,
  @ipv6acl_remark :1 u8,
  @l2acl_car :1 u8,
  @l4acl_car :1 u8,
  @l2l4acl_car :1 u8,
  @ipv6acl_car :1 u8,
  @l2AclMirror :1 u8,
  @l4AclMirror :1 u8,
  @l2l4AclMirror :1 u8,
  @ipv6AclMirror :1 u8,
  @ip_count :1 u8,
  @mirror :1 u8,
  @car :1 u8,
  @stat :1 u8,
  @ipv4_stat :1 u8,
  @ipv6_stat :1 u8,
  @capt :1 u8,
  @tmQueue :1 u8,
  @tmLoopQueue :1 u16 align(2),
  @rsv8 u8}>
type $tagSrvInfo <struct {
  @zoneid u16 align(2),
  @qosProfileIndex u8,
  @nat_en :1 u8,
  @nat_global :1 u8,
  @mac_auth :1 u8,
  @dot1x_auth :1 u8,
  @portal :1 u8,
  @rsv3 :3 u8,
  @trustType :3 u8,
  @phbRmkDscp :1 u8,
  @phbRmk8021p :1 u8,
  @phbRmkExp :1 u8,
  @preClassify :1 u8,
  @rsv1 :1 u8,
  @hostcarEnable u8,
  @ingCaptIndex u8,
  @egrCaptIndex u8,
  @srvcId u8,
  @interfacePri u8,
  @que_valid :1 u16 align(2),
  @enq_pri :3 u16 align(2),
  @que_flag :1 u16 align(2),
  @Qvalid :1 u16 align(2),
  @Qlevel :2 u16 align(2),
  @voice_enq_pri :3 u16 align(2),
  @rsv5 :5 u16 align(2),
  @queueid u16 align(2),
  @voiceQueueid u16 align(2),
  @ingIpsgGid u16 align(2),
  @ipfpmGid u16 align(2),
  @ingL2AclGid u16 align(2),
  @ingL4AclGid <[3] u16> align(2),
  @ingL2AclFilterGid u32 align(4),
  @ingL4AclFilterGid u32 align(4),
  @ingL2AclStatGid u32 align(4),
  @ingL4AclStatGid u32 align(4),
  @ingL2AclRedirectGid u32 align(4),
  @ingL4AclRedirectGid u32 align(4),
  @ingL2AclRemarkGid u32 align(4),
  @ingL4AclRemarkGid u32 align(4),
  @ingL2AclCarGid u32 align(4),
  @ingL4AclCarGid u32 align(4),
  @ingL2AclMirrorGid u32 align(4),
  @ingL4AclMirrorGid u32 align(4),
  @ingIprlGid u16 align(2),
  @ingIpv4UprfGid u16 align(2),
  @ingIpv6UrpfGid u16 align(2),
  @resrv16 u16 align(2),
  @egrL2AclGid u16 align(2),
  @egrL4AclGid <[3] u16> align(2),
  @egrL2AclFilterGid u32 align(4),
  @egrL4AclFilterGid u32 align(4),
  @egrL2AclStatGid u32 align(4),
  @egrL4AclStatGid u32 align(4),
  @egrL2AclRemarkGid u32 align(4),
  @egrL4AclRemarkGid u32 align(4),
  @egrL2AclCarGid u32 align(4),
  @egrL4AclCarGid u32 align(4),
  @egrL2AclMirrorGid u32 align(4),
  @egrL4AclMirrorGid u32 align(4),
  @egrIplrGid u16 align(2),
  @snatAclGid u16 align(2),
  @dnatAclGid u16 align(2),
  @gdoiGid u16 align(2),
  @ipsecFlowGid u16 align(2),
  @ipsecFlowexGid u16 align(2),
  @ipsecGid u16 align(2),
  @resrv2_16 u16 align(2),
  @ingCarid u32 align(4),
  @egrCarid u32 align(4),
  @observeId u32 align(4)}>
type $tagTunnelInfo <struct {
  @tunnelType u8,
  @qosGroup u8,
  @preEncapIncLen u8,
  @p2mpIpsecEn :1 u8,
  @rsv7 :7 u8,
  @tunnelId u32 align(4)}>
type $tagNsInfo <struct {
  @ingSampleMode :4 u8,
  @egrSampleMode :4 u8,
  @ingIpv4En :1 u8,
  @egrIpv4En :1 u8,
  @ingIpv6En :1 u8,
  @egrIpv6En :1 u8,
  @ingIpv4McEn :1 u8,
  @egrIpv4McEn :1 u8,
  @ipv4RpfFail :1 u8,
  @rsv1 :1 u8,
  @nsFwdifIndex u32 align(4),
  @ndcId u32 align(4)}>
type $FwdIfStat <struct {
  @inBytes u64 align(8),
  @inPkts u64 align(8),
  @inUPkts u64 align(8),
  @inMPkts u64 align(8),
  @inBPkts u64 align(8),
  @inDrops u64 align(8),
  @inErrors u64 align(8),
  @inUnknow u64 align(8),
  @outBytes u64 align(8),
  @outPkts u64 align(8),
  @outUPkts u64 align(8),
  @outMPkts u64 align(8),
  @outBPkts u64 align(8),
  @outDrops u64 align(8),
  @outErrors u64 align(8)}>
type $FwdIfMultiCoreStat <struct {
  @core <[1] <$FwdIfStatAligned> align(64)> align(64),
  @ifStatistic <$FwdIfStatAligned> align(64)}>
type $tagEgrInfo <struct {
  @bsValid :1 u8,
  @bsArpFake :1 u8,
  @bsArpMiss :1 u8,
  @srvcId :3 u8,
  @phbRmkDscp :1 u8,
  @phbRmk8021p :1 u8,
  @preClassify :1 u8,
  @protoTmLoop :1 u8,
  @rsv6 :6 u8,
  @encapType u8,
  @encapSize u8,
  @cpuPort u8,
  @TxFwdifTop u8,
  @TxL3svcTop u8,
  @TxL1svcTop u8,
  @TxLinksvcTop u8,
  @pktType u8,
  @mtu u16 align(2),
  @qosGroup u8,
  @rsv8 u8,
  @trunkId u16 align(2),
  @egr_func <* <func (<* <$hpe_mbuf> align(64)>,<* <$tagContext>>,<* u8>,<* <$tagEgrInfo>>) u32>> align(8),
  @TxPvcFwdif <* <$tagFwdIf>> align(8),
  @unnamed.6011 <$unnamed.6010> implicit align(8),
  @l2aclKey <$PfaAclL2Key> align(2),
  @encap <[72] u8>,
  @TxFwdif <[6] <* <$tagFwdIf>>> align(8),
  @TxL3svcSet <[8] <$EgrFsvc>> align(4),
  @TxL1svcSet <[8] <$EgrFsvc>> align(4),
  @TxLinksvcSet <[8] <$EgrFsvc>> align(4),
  @vpn u16 align(2),
  @fwdif u32 align(4)}>
type $tagContext <structincomplete {}>
type $unnamed.6010 <union {
  @tnl <* void> align(8),
  @trunk <* void> align(8)}>
type $PfaAclL2Key <struct {
  @dmac <[6] u8>,
  @smac <[6] u8>,
  @ethType u16 align(2),
  @outerVlan u16 align(2),
  @innerVlan u16 align(2),
  @flags <$unnamed.6012>,
  @qosGroup u8,
  @fwdIfIdx u16 align(2),
  @subFwdIfIdx u16 align(2)}>
type $unnamed.6012 <union {
  @data u8,
  @unnamed.6014 <$unnamed.6013> implicit}>
type $unnamed.6013 <struct {
  @ethLink :1 u8,
  @rsv :7 u8}>
type $unnamed.6015 <union {
  @u8Flag u8,
  @unnamed.6017 <$unnamed.6016> implicit}>
type $unnamed.6016 <struct {
  @opcode :3 u8,
  @mulNhp :1 u8,
  @subnetBC :1 u8,
  @oprFlag :1 u8,
  @pppoe :1 u8,
  @frrEnable :1 u8}>
type $unnamed.6018 <union {
  @crossVsysIfIdx u32 align(4),
  @unnamed.6020 <$unnamed.6019> implicit align(2)}>
type $unnamed.6019 <struct {
  @resv u16 align(2),
  @crossVrf u16 align(2)}>
type $unnamed.6021 <union {
  @nhp <* <$HpfNextHopEntry>> align(8),
  @nhpGrp <* <$HpfNextHopGrpEntry>> align(8)}>
type $HpfNextHopEntry <struct {
  @unnamed.6031 <$unnamed.6025> implicit align(2),
  @u16Vpn u16 align(2),
  @u16LspToken u16 align(2),
  @u16NlbID u16 align(2),
  @Nexthop <[16] u8>,
  @ctrlIfIdx u32 align(4),
  @u16Mtu u16 align(2),
  @unnamed.6032 <$unnamed.6028> implicit,
  @linkLocal :1 u8,
  @srv6Enable :1 u8,
  @srv6NhpType :3 u8,
  @bfdDown :1 u8,
  @bgpState :1 u8,
  @resv :1 u8,
  @version u32 align(4),
  @tunnelType u32 align(4),
  @tunnelId u32 align(4),
  @vni u32 align(4),
  @vxlanMac <[6] u8>,
  @outVpn u16 align(2),
  @fwdIfIndex u32 align(4),
  @srv6NextIndex u32 align(4),
  @srv6VpnSid <[4] u32> align(4),
  @bfdDisc u32 align(4),
  @bfdSession u32 align(4),
  @egrInfo <$tagEgrInfo> align(8)}>
type $HpfNextHopGrpEntry <struct {
  @bsFrr :1 u8,
  @resv1 :7 u8,
  @bsNhpNum u8,
  @resv2 u8,
  @resv3 u8,
  @hashIndex u32 align(4),
  @nhpIndex <[128] u32> align(4),
  @version u32 align(4),
  @nhp <[128] <* <$HpfNextHopEntry>>> align(8)}>
type $Ipv4FwdCache <structincomplete {}>
type $unnamed.6025 <union {
  @u16BitFlg u16 align(2),
  @unnamed.6027 <$unnamed.6026> implicit align(2)}>
type $unnamed.6026 <struct {
  @bsValid :1 u16 align(2),
  @bsMPLS :1 u16 align(2),
  @bsVpn :1 u16 align(2),
  @bsDialFlag :1 u16 align(2),
  @bsDrop :1 u16 align(2),
  @bsTunnelEnable :1 u16 align(2),
  @bsL3VpnOverGre :1 u16 align(2),
  @bsDialPktCount :7 u16 align(2),
  @bsNlbEnable :1 u16 align(2),
  @bsDirRoute :1 u16 align(2)}>
type $unnamed.6028 <union {
  @stat u8,
  @unnamed.6030 <$unnamed.6029> implicit}>
type $unnamed.6029 <struct {
  @status :1 u8,
  @arpMiss :1 u8,
  @resv2 :6 u8}>
type $HpfArpData <struct {
  @bsValid :1 u16 align(2),
  @bsFake :1 u16 align(2),
  @bsType :3 u16 align(2),
  @bsNLBMcEnable :1 u16 align(2),
  @bsVxlanBindType :2 u16 align(2),
  @tunnelType :8 u16 align(2),
  @ctrlIfIdx u16 align(2),
  @fwdIfIndex u16 align(2),
  @l3CtrlIfIdx u16 align(2),
  @l3FwdIfIndex u16 align(2),
  @u16Resv u16 align(2),
  @u16NLBId u16 align(2),
  @dmac <[6] u8>,
  @unLinkInfo <$unnamed.6033> align(2),
  @u32Vni u32 align(4),
  @u32Dip u32 align(4),
  @u32Sip u32 align(4),
  @replyTime u32 align(4)}>
type $unnamed.6033 <union {
  @u16PvcIf u16 align(2),
  @u16VlanId u16 align(2),
  @stQinq <$unnamed.6034> align(2)}>
type $unnamed.6034 <struct {
  @u16InnerVlanId u16 align(2),
  @u16OuterVlanId u16 align(2)}>
type $HpfArpTblData <struct {
  @bsValid :1 u16 align(2),
  @bsFake :1 u16 align(2),
  @bsType :3 u16 align(2),
  @bsNLBMcEnable :1 u16 align(2),
  @bsVxlanBindType :2 u16 align(2),
  @tunnelType :8 u16 align(2),
  @resv u16 align(2),
  @ctrlIfIdx u32 align(4),
  @fwdIfIndex u32 align(4),
  @l3CtrlIfIdx u32 align(4),
  @l3FwdIfIndex u32 align(4),
  @u16NLBId u16 align(2),
  @dmac <[6] u8>,
  @unLinkInfo <$unnamed.6035> align(2),
  @u32Vni u32 align(4),
  @u32Dip u32 align(4),
  @u32Sip u32 align(4),
  @replyTime u32 align(4)}>
type $unnamed.6035 <union {
  @stQinq <$unnamed.6036> align(2),
  @u16PvcIf u16 align(2),
  @u16VlanId u16 align(2)}>
type $unnamed.6036 <struct {
  @u16InnerVlanId u16 align(2),
  @u16OuterVlanId u16 align(2)}>
type $unnamed.6037 <union {
  @crossVsysIfIdx u32 align(4),
  @unnamed.6039 <$unnamed.6038> implicit align(2)}>
type $unnamed.6038 <struct {
  @resv u16 align(2),
  @crossVrf u16 align(2)}>
type $unnamed.6040 <union {
  @nhp6 <* <$tagHpfNhp6Entry>> align(8),
  @nhp6Grp <* <$tagHpfNhp6GrpEntry>> align(8)}>
type $tagHpfNhp6Entry <struct {
  @bsValid :1 u32 align(4),
  @bsVpn :1 u32 align(4),
  @bsDialFlag :1 u32 align(4),
  @bsVpnID :16 u32 align(4),
  @bsDialPktCount :7 u32 align(4),
  @bsDrop :1 u32 align(4),
  @bsTeTunnel :1 u32 align(4),
  @bsMpls :1 u32 align(4),
  @bsRev :3 u32 align(4),
  @ctrlIfIdx u32 align(4),
  @fwdIfIndex u32 align(4),
  @u16Mtu u16 align(2),
  @unnamed.6046 <$unnamed.6043> implicit,
  @srv6Enable :1 u8,
  @srv6NhpType :3 u8,
  @bfdDown :1 u8,
  @bgpState :1 u8,
  @resv :2 u8,
  @Nexthop <[16] u8>,
  @version u32 align(4),
  @tunnelType u32 align(4),
  @tunnelId u32 align(4),
  @vni u32 align(4),
  @vxlanMac <[6] u8>,
  @outVpn u16 align(2),
  @srv6NextIndex u32 align(4),
  @srv6VpnSid <[4] u32> align(4),
  @bfdDisc u32 align(4),
  @bfdSession u32 align(4),
  @egrInfo <$tagEgrInfo> align(8)}>
type $tagHpfNhp6GrpEntry <struct {
  @bsFrr :1 u8,
  @resv1 :7 u8,
  @bsNhpNum u8,
  @resv2 u8,
  @resv3 u8,
  @hashIndex u32 align(4),
  @nhp6Index <[128] u32> align(4),
  @version u32 align(4)}>
type $tagIpv6Cache <structincomplete {}>
type $unnamed.6043 <union {
  @stat u8,
  @unnamed.6045 <$unnamed.6044> implicit}>
type $unnamed.6044 <struct {
  @status :1 u8,
  @ndMiss :1 u8,
  @resv6 :6 u8}>
type $HpfNdData <struct {
  @bsValid :1 u16 align(2),
  @bsFake :1 u16 align(2),
  @bsType :3 u16 align(2),
  @bsResv :11 u16 align(2),
  @dmac <[6] u8>,
  @ctrlIfIdx u32 align(4),
  @fwdIfIndex u32 align(4),
  @l3CtrlIfIdx u32 align(4),
  @l3FwdIfIndex u32 align(4),
  @u16VrfId u16 align(2),
  @u16Resv u16 align(2),
  @unLinkInfo <$unnamed.6047> align(2),
  @u32Vni u32 align(4),
  @u32Sip u32 align(4),
  @u32Dip u32 align(4)}>
type $unnamed.6047 <union {
  @u16PvcIf u16 align(2),
  @u16VlanId u16 align(2),
  @stQinq <$unnamed.6048> align(2)}>
type $unnamed.6048 <struct {
  @u16InnerVlanId u16 align(2),
  @u16OuterVlanId u16 align(2)}>
type $tagMfib6Key <struct {
  @rsv u16 align(2),
  @vpn u16 align(2),
  @sip <[16] u8>,
  @mcip <[16] u8>}>
type $tagMfib6Data <struct {
  @rpf :1 u8,
  @modifyTtl :1 u8,
  @vlanMc :1 u8,
  @vaIfValid :1 u8,
  @rsv4 :4 u8,
  @minTtl u8,
  @mcMid u16 align(2),
  @mcStatId u32 align(4),
  @fwdIfIndex u32 align(4)}>
type $tagMfib6Entry <struct {
  @key <$tagMfib6Key> align(2),
  @data <$tagMfib6Data> align(4)}>
type $tagElb6Node <struct {
  @vlanMc :1 u8,
  @toCpu :1 u8,
  @toPhy :1 u8,
  @rsv5 :5 u8,
  @rsv24 <[3] u8>,
  @fwdIfIndex u32 align(4),
  @vlan u16 align(2),
  @rsv16 u16 align(2),
  @egrInfo <$tagEgrInfo> align(8)}>
type $tagElb6Key <struct {
  @index u32 align(4),
  @rsv u32 align(4),
  @unnamed.6050 <$unnamed.6049> implicit align(8)}>
type $unnamed.6049 <union {
  @data <[0] u8>,
  @elb6Node <$tagElb6Node> align(8)}>
type $HpfPathMtuKey <struct {
  @vrId u32 align(4),
  @vrfIndex u32 align(4),
  @ipAddr <$HpfIn6Addr>}>
type $HpfPathMtuEntry <struct {
  @key <$HpfPathMtuKey> align(4),
  @mtu u32 align(4),
  @version u32 align(4),
  @resv u32 align(4)}>
type $HpsStreamKey <struct {
  @unPort <$unnamed.6051> align(2),
  @u8Protocol u8,
  @u8Rsv u8,
  @u16Vpn u16 align(2),
  @u32Dip u32 align(4),
  @u32Sip u32 align(4)}>
type $unnamed.6051 <union {
  @stTcpUdp <$unnamed.6052> align(2),
  @stIcmp <$unnamed.6053> align(2)}>
type $unnamed.6052 <struct {
  @u16Dport u16 align(2),
  @u16Sport u16 align(2)}>
type $unnamed.6053 <struct {
  @u16IcmpID u16 align(2),
  @u8Type u8,
  @u8Code u8}>
type $HpfPortIpv4Index <struct {
  @ctrlIfIdx u32 align(4),
  @index u32 align(4)}>
type $HpfPortIpv6Index <struct {
  @ctrlIfIdx u32 align(4),
  @index u32 align(4)}>
type $HpfPortIpv4NodeKey <struct {
  @index u32 align(4),
  @unnamed.6055 <$unnamed.6054> implicit align(4)}>
type $unnamed.6054 <union {
  @data <[0] u8>,
  @portIpv4Node <$HpfPortIpv4Node> align(4)}>
type $HpfPortIpv6NodeKey <struct {
  @index u32 align(4),
  @unnamed.6057 <$unnamed.6056> implicit align(4)}>
type $unnamed.6056 <union {
  @data <[0] u8>,
  @portIpv6Node <$HpfPortIpv6Node> align(4)}>
type $HpfPortIpv4TravType <struct {
  @ipAddr u32 align(4),
  @ipMask u32 align(4),
  @ipType <[11] u32> align(4)}>
type $HpfPortIpv6TravType <struct {
  @ipv6Addr <$HpfIn6Addr>,
  @prefix u32 align(4),
  @ipv6Type <[7] u32> align(4)}>
type $HpfGreTunnelStatisticNode <struct {
  @outPackets u64 align(8),
  @outErrors u64 align(8),
  @outBytes u64 align(8),
  @inPackets u64 align(8),
  @inBytes u64 align(8),
  @inErrors u64 align(8),
  @inUnicasts u64 align(8),
  @inMulticasts u64 align(8),
  @outUnicasts u64 align(8),
  @outMulticasts u64 align(8)}>
type $HpfGreTunnelStat <struct {
  @tunnelId u32 align(4),
  @fwdIfIndex u32 align(4),
  @tunnelIfStat <$HpfGreTunnelStatisticNode> align(8),
  @tunnelIfLastStat <$HpfGreTunnelStatisticNode> align(8),
  @fwmTunnelId u32 align(4),
  @resv u32 align(4)}>
type $HpfGreUpTunnelCfg <struct {
  @tunnelId u32 align(4),
  @vrIndex u32 align(4),
  @vrfIndex u32 align(4),
  @srcVrf u32 align(4),
  @dstVrf u32 align(4),
  @zoneId u32 align(4),
  @ifIndex u32 align(4),
  @fwdIndex u32 align(4),
  @srcIfIndex u32 align(4),
  @fwmTunnelId u32 align(4),
  @tunnelIp <$HpfIn46Addr>,
  @srcAddr <$HpfIn46Addr>,
  @dstAddr <$HpfIn46Addr>,
  @tunnelCheckSum u8,
  @tunnelKey u8,
  @ifStatEnable u8,
  @nameId u16 align(2),
  @tunnelKeyValue u32 align(4),
  @encapProto u8,
  @transmitProto u8,
  @tunnelMode u8,
  @isIpsecOverGre u8,
  @outPortIndex u32 align(4),
  @outPortIndexVlanIf u32 align(4),
  @routeRefreshId u32 align(4),
  @nextHop u32 align(4),
  @vlanId u16 align(2),
  @nhrpRedirect :1 u16 align(2),
  @nhrpShortCut :1 u16 align(2),
  @nhrpServer :1 u16 align(2),
  @res :13 u16 align(2),
  @nhrpDomainId u16 align(2),
  @arpRefreshId u16 align(2),
  @ipv4PktId u32 align(4),
  @token u32 align(4),
  @innerLabel u32 align(4),
  @tunnelIfStat <* <$HpfGreTunnelStatisticNode>> align(8),
  @version u32 align(4),
  @resv u32 align(4),
  @unnamed.6059 <$unnamed.6058> implicit align(8),
  @unnamed.6061 <$unnamed.6060> implicit align(8)}>
type $unnamed.6058 <union {
  @re <* <$tagCAP_RE_ENTRY_S>> align(8),
  @re6 <* <$tagCAP_RE6_ENTRY_S>> align(8)}>
type $unnamed.6060 <union {
  @nhp <* <$HpfNextHopEntry>> align(8),
  @nhp6 <* <$tagHpfNhp6Entry>> align(8),
  @nhpGrp <* <$HpfNextHopGrpEntry>> align(8),
  @nhp6Grp <* <$tagHpfNhp6GrpEntry>> align(8)}>
type $HpfGreTunnelDecapKey <struct {
  @dstAddr u32 align(4),
  @srcAddr u32 align(4),
  @vrfIndex u16 align(2),
  @tunnelMode u16 align(2)}>
type $HpfGreTunnelDecapEntry <struct {
  @key <$HpfGreTunnelDecapKey> align(4),
  @tunnelId u32 align(4),
  @fwdIfIndex u32 align(4),
  @keyEn u8,
  @resv u8,
  @tnlIfVrf u16 align(2),
  @keyValue u32 align(4),
  @version u32 align(4),
  @fwdIf <* <$tagFwdIf>> align(8)}>
type $FwdIfStatAligned <struct {
  @inBytes u64 align(8),
  @outBytes u64 align(8),
  @inPkts u64 align(8),
  @outPkts u64 align(8),
  @inUPkts u64 align(8),
  @outUPkts u64 align(8),
  @inBPkts u64 align(8),
  @outBPkts u64 align(8),
  @inMPkts u64 align(8),
  @outMPkts u64 align(8),
  @inDrops u64 align(8),
  @outDrops u64 align(8),
  @inErrors u64 align(8),
  @outErrors u64 align(8),
  @inUnknow u64 align(8)}>
type $CtrlIfMapFwdIfTbl <struct {
  @ctrlIfIdx u32 align(4),
  @fwdIfIdx u32 align(4),
  @version u32 align(4),
  @resv u32 align(4)}>
type $FlowIfMapFwdIfTbl <struct {
  @flowIfIdx u32 align(4),
  @fwdIfIdx u32 align(4),
  @version u32 align(4),
  @resv u32 align(4)}>
type $HpfLinkEthInfo <struct {
  @portLinkType :3 u32 align(4),
  @trunkMem :1 u32 align(4),
  @trunk :1 u32 align(4),
  @mng :1 u32 align(4),
  @rsv :2 u32 align(4),
  @vlanIdBegin :12 u32 align(4),
  @vlanIdEnd :12 u32 align(4),
  @innerVlanIdBegin :12 u32 align(4),
  @innerVlanIdEnd :12 u32 align(4),
  @tpId u16 align(2)}>
type $HpfLinkInfo <struct {
  @portType u16 align(2),
  @linkType u16 align(2),
  @unnamed.6063 <$unnamed.6062> implicit align(4)}>
type $unnamed.6062 <union {
  @ethInfo <$HpfLinkEthInfo> align(4)}>
type $unnamed.6064 <struct {
  @l2EgrFuncId u16 align(2),
  @portType u16 align(2),
  @linkType u16 align(2),
  @resv16 u16 align(2)}>
type $unnamed.6065 <struct {
  @l2vpn :1 u8,
  @vpls :1 u8,
  @transBridge :1 u8,
  @linkBridge :1 u8,
  @vxlan :1 u8,
  @lsdp :1 u8,
  @pppoe :1 u8,
  @rsv1 :1 u8,
  @rsv8 u8,
  @vsi u16 align(2)}>
type $unnamed.6066 <union {
  @ethInfo <$unnamed.6067> align(4),
  @pppInfo <$unnamed.6068> align(4),
  @atmInfo <$unnamed.6069> align(4)}>
type $unnamed.6067 <struct {
  @portLinkType :3 u8,
  @trunk :1 u8,
  @trunkMem :1 u8,
  @mng :1 u8,
  @stp :1 u8,
  @lldp :1 u8,
  @dldp :1 u8,
  @gvrp :1 u8,
  @lacp :1 u8,
  @dot3ah :1 u8,
  @dot3ahActive :1 u8,
  @dot3ahPassive :1 u8,
  @dot3ahBlock :1 u8,
  @dot3ahForce :1 u8,
  @trunkId u16 align(2),
  @smac <[6] u8>,
  @pvid u16 align(2),
  @dot1qTpid u16 align(2),
  @qinqTpid u16 align(2),
  @vlanIdBegin :12 u32 align(4),
  @vlanIdEnd :12 u32 align(4),
  @innerVlanIdBegin :12 u32 align(4),
  @innerVlanIdEnd :12 u32 align(4),
  @tpId u16 align(2)}>
type $unnamed.6068 <struct {
  @resv32 u32 align(4)}>
type $unnamed.6069 <struct {
  @rsv32 u32 align(4)}>
type $unnamed.6070 <struct {
  @cellularLink u32 align(4),
  @peermac <[6] u8>,
  @rsv16 u16 align(2)}>
type $unnamed.6071 <struct {
  @macLimitEn :1 u8,
  @macLimitAlarmEn :1 u8,
  @macLearnEn :1 u8,
  @macRsv :5 u8,
  @macRsv2 u8,
  @macLimitAction u8,
  @macLearnEnAction u8,
  @macLimitNum u16 align(2),
  @macRsv3 u16 align(2),
  @curMacNum u32 align(4)}>
type $IngFsvc <struct {
  @isL3Svc :1 u8,
  @ingSvc :7 u8,
  @fwdifStackIdx u8,
  @param1 u16 align(2),
  @param2 u32 align(4)}>
type $HashFwdIf <struct {
  @mainIdx u32 align(4),
  @fwdIf <$tagFwdIf> align(64)}>
type $HpfCtlIfName <struct {
  @name <[64] u8>}>
type $HpfCtlIfIndex <struct {
  @ctlIfIndex u32 align(4)}>
type $HpfCtlIfAndName <struct {
  @key <$HpfCtlIfIndex> align(4),
  @data <$HpfCtlIfName>}>
type $HpfNameAndCtlIf <struct {
  @key <$HpfCtlIfName>,
  @data <$HpfCtlIfIndex> align(4)}>
type $HpfTrunkTbl <struct {
  @portNum u16 align(2),
  @hashType u8,
  @res u8,
  @inActivePortNum u16 align(2),
  @res1 u16 align(2),
  @trunkIfIdx u32 align(4),
  @ifName <[64] u8>,
  @fwdIfIdx <[256] u32> align(4),
  @memberStat <[256] u8>,
  @pktHashCnt u32 align(4),
  @version u32 align(4),
  @fwdIf <* <$tagFwdIf>> align(8)}>
type $HpfArpNatPoolKey <struct {
  @vsysId u16 align(2),
  @serviceId u16 align(2),
  @startIp u32 align(4)}>
type $HpfArpNatPool <struct {
  @ipNum u32 align(4),
  @vrrpId u32 align(4)}>
type $HpfArpNatPoolMsg <struct {
  @key <$HpfArpNatPoolKey> align(4),
  @data <$HpfArpNatPool> align(4),
  @version u32 align(4),
  @resv u32 align(4)}>
type $HpfArpNatPoolQueryMsg <struct {
  @key <$HpfArpNatPoolKey> align(4),
  @data <$HpfArpNatPool> align(4)}>
type $HpfArpNatPoolPara <struct {
  @sectMaxNum u32 align(4),
  @secVersion u32 align(4),
  @sectNum u32 align(4),
  @sectQueryData <* <$HpfArpNatPoolQueryMsg>> align(8)}>
type $HpfFwdCaptAclCfg <struct {
  @isValid u8,
  @aclGrpType u8,
  @resv u16 align(2),
  @aclGrpId u32 align(4)}>
type $HpfFwdCaptCfg <struct {
  @expireTime u32 align(4),
  @captHost u8,
  @pktLen u8,
  @pktNumRemain u16 align(2),
  @timeout u32 align(4),
  @direction u8,
  @clearPayload u8,
  @vlanId u16 align(2),
  @ifIndex <[8] u32> align(4),
  @acl <$HpfFwdCaptAclCfg> align(4)}>
type $NdFastReplyInfo <struct {
  @type u8,
  @code u8,
  @chkSum u16 align(2),
  @res u32 align(4),
  @tarAdd <[4] u32> align(4),
  @opType u8,
  @length u8,
  @srcMac <[6] u8>}>
type $HpfUrpfCfg <struct {
  @urpfEnable u8,
  @isStrict u8,
  @allowDefaultRoute u8,
  @debugSwitch u8,
  @aclGroupId u32 align(4)}>
type $HpfAtkPortCfg <struct {
  @ctrlIfIndex u32 align(4),
  @urpfCfg <$HpfUrpfCfg> align(4),
  @urpfCfgV6 <$HpfUrpfCfg> align(4),
  @dropNum u32 align(4),
  @suppressNum u32 align(4),
  @dropNumV6 u32 align(4),
  @suppressNumV6 u32 align(4)}>
type $HpfTunnel4over6Cfg <struct {
  @tunnelId u32 align(4),
  @tunnelType u32 align(4),
  @ctrlIfIndex u32 align(4),
  @srcAddr <$HpfIn6Addr>,
  @dstAddr <$HpfIn6Addr>,
  @outPort u32 align(4),
  @ndIndex u32 align(4),
  @nextHop <[4] u32> align(4),
  @routeRefreshId u8,
  @gwFlag u8,
  @vlanId u16 align(2),
  @classOriginal u8,
  @encapLimit u8,
  @hopLimit u8,
  @dscpCpy u8,
  @tClassFlow u32 align(4),
  @tunnelMode u8,
  @reserve u8,
  @encapProcess u16 align(2)}>
type $HpfTunnel6Rd <struct {
  @ipv6Prefix <$HpfIn6Addr>,
  @ipv6RdDelegatePrefix <$HpfIn6Addr>,
  @ipv6PrefixLen u8,
  @ipv4PrefixLen u8,
  @ipv6RdDelegatePrefixLen u8,
  @ipv4PrefixLenSet :1 u8,
  @ipv6RdDelegatePrefixValid :1 u8,
  @reserve :6 u8,
  @brAddr u32 align(4)}>
type $HpfTunnel6over4Cfg <struct {
  @tunnelId u32 align(4),
  @tunnelType u32 align(4),
  @ctrlIfIndex u32 align(4),
  @srcAddr u32 align(4),
  @dstAddr u32 align(4),
  @outPort u32 align(4),
  @nextHop u32 align(4),
  @vlanId u16 align(2),
  @routeRefreshId u8,
  @tunnelMode u8,
  @ipv4PktId u32 align(4),
  @tunnel6Rd <$HpfTunnel6Rd> align(4)}>
type $HpfTunnel6over4DecapKey <struct {
  @srcAddr u32 align(4),
  @dstAddr u32 align(4),
  @vrfIndex u32 align(4),
  @reserve u32 align(4)}>
type $HpfTunnel6over4DecapEntry <struct {
  @key <$HpfTunnel6over4DecapKey> align(4),
  @tunnelId u32 align(4),
  @tunnelId2 u32 align(4),
  @version u32 align(4),
  @resv u32 align(4)}>
type $HpfTunnel4over6DecapKey <struct {
  @srcAddr <$HpfIn6Addr>,
  @dstAddr <$HpfIn6Addr>,
  @tunnelMode u32 align(4),
  @reserve u32 align(4)}>
type $HpfTunnel4over6DecapEntry <struct {
  @key <$HpfTunnel4over6DecapKey> align(4),
  @tunnelId u32 align(4),
  @version u32 align(4),
  @resv u32 align(4)}>
type $MirrObserve <struct {
  @ifIndex u32 align(4)}>
type $HpfBfdFsm <struct {
  @sessionId u16 align(2),
  @valid :1 u16 align(2),
  @status :2 u16 align(2),
  @processPst :3 u16 align(2),
  @detectType :4 u16 align(2),
  @diagnostic :5 u16 align(2),
  @firstFlag :1 u16 align(2),
  @scenario u32 align(4),
  @processFwdIfIdx u32 align(4),
  @detectTimes u32 align(4),
  @detectInterval u32 align(4),
  @actTxInterval u32 align(4),
  @myDiscriminator u32 align(4),
  @yourDiscriminator u32 align(4),
  @globalIndex u16 align(2),
  @adminDown u8,
  @resv1 u8,
  @sendTimerId <* void> align(8),
  @detectTimerId <* void> align(8)}>
type $HpfBfdPkt <struct {
  @outIfIndex u32 align(4),
  @outCtrlIfIdx u32 align(4),
  @vlanIfIndex u32 align(4),
  @vrfIndex u16 align(2),
  @hop u8,
  @txType u8,
  @pktLen u32 align(4),
  @l3Offset u32 align(4),
  @ipType u32 align(4),
  @nextHop <[4] u32> align(4),
  @pkt <[128] u8>,
  @linkHdr <[64] u8>,
  @destMac <[6] u8>,
  @linkHdrLen u8,
  @npHdrLen u8,
  @rxPkts u32 align(4),
  @txPkts u32 align(4)}>
type $HpfBfdTimer <struct {
  @detectTtlTicks i32 align(4),
  @detectTicks <$HpeAtom32> align(4),
  @remainTimes <$HpeAtom32> align(4),
  @txPktTtlTicks u32 align(4),
  @txPktTicks i32 volatile align(4),
  @jobId u32 align(4)}>
type $HpfBfdEntry <struct {
  @bfdFsm <$HpfBfdFsm> align(8),
  @bfdPkt <$HpfBfdPkt> align(4),
  @timer <$HpfBfdTimer> align(4)}>
type $tagHpfBfdRxKey <struct {
  @myDiscr u32 align(4)}>
type $tagHpfBfdRxEntry <struct {
  @key <$tagHpfBfdRxKey> align(4),
  @sessionId u32 align(4)}>
type $HpfBfdDownMsgHead <struct {
  @resv <[3] u8>,
  @msgCount u8,
  @resv1 u32 align(4)}>
type $HpfVpnNameIdKey <struct {
  @nameLen u32 align(4),
  @vpnName <[32] u8>}>
type $HpfVpnNameIdData <struct {
  @key <$HpfVpnNameIdKey> align(4),
  @vrfId u32 align(4),
  @peerVrfId u32 align(4)}>
type $HpfFwdIfNotifyHead <struct {
  @lock <$HpeRwlock> align(4),
  @head <$tagHpeListHead> align(8)}>
type $HpfFwdIfNotifyNode <struct {
  @node <$tagHpeListHead> align(8),
  @cb <* <func (u32,<* <$tagFwdIf>>,<* <$tagFwdIf>>) void>> align(8)}>
type $HpfFwdIfNotifyFunc <struct {
  @node <$tagHpeListHead> align(8),
  @cb <* <func (u32,u32) void>> align(8)}>
type $HpfFwdIfMatchIp <struct {
  @dstIp u32 align(4),
  @ipAddr u32 align(4),
  @MatchMask u32 align(4),
  @count u32 align(4)}>
type $HpfFwdIfTravRet <struct {
  @ipAddr u32 align(4),
  @result u32 align(4)}>
type $HpfMbuftrMsgHd <struct {
  @msgType u32 align(4),
  @msgLen u32 align(4),
  @msgVal <[0] <* void>> align(8)}>
type $HpfMbuftrPktMsgBody <struct {
  @srcAddr <[4] u32> align(4),
  @dstAddr <[4] u32> align(4),
  @outIfIndex u32 align(4),
  @nextHop <[4] u32> align(4),
  @vrfIndex u32 align(4)}>
type $HpfTraceDbgMsg <struct {
  @enable u8,
  @invert u8,
  @fromCp u8,
  @fragType u8,
  @number u32 align(4),
  @srcMac <[6] u8>,
  @dstMac <[6] u8>,
  @srcAddr <$HpfIn46Addr>,
  @dstAddr <$HpfIn46Addr>,
  @protocol u8,
  @resv <[3] u8>,
  @ethType u16 align(2),
  @vlanId u16 align(2),
  @srcPort u16 align(2),
  @dstPort u16 align(2),
  @srcIntf u32 align(4),
  @maxLen u16 align(2),
  @minLen u16 align(2)}>
type $HpfLinkStateCheckMsg <struct {
  @checkStateFlagInfo u16 align(2),
  @res u16 align(2)}>
type $HpfLinkStateCheckIpv6Msg <struct {
  @checkStateFlagInfo u16 align(2),
  @res u16 align(2)}>
type $HpfLinkStateExcludeaclMsg <struct {
  @aclNum u32 align(4),
  @undoFlag u32 align(4)}>
type $HpfLinkStateExcludeaclIpv6Msg <struct {
  @aclNum u32 align(4),
  @undoFlag u32 align(4)}>
type $HpfFlowFastAgingCfg <struct {
  @enable u32 align(4),
  @lowerThreshold u32 align(4),
  @upperThreshold u32 align(4),
  @ttlPercent u32 align(4),
  @memLowerThreshold u32 align(4),
  @memUpperThreshold u32 align(4)}>
type $HpfDnsFastAgeCfg <struct {
  @enable u32 align(4),
  @agingTime u32 align(4)}>
type $HpfOnlineIpMsgInfo <struct {
  @sortType u8,
  @vsysSelectFlag u8,
  @vsysIndex u16 align(2)}>
type $HpfOnlineIpSortInfo <struct {
  @ipAddr u32 align(4),
  @appId u16 align(2),
  @vsysId u16 align(2),
  @unnamed.6073 <$unnamed.6072> implicit align(8),
  @upRate u64 align(8),
  @downRate u64 align(8),
  @userId u32 align(4),
  @remain u32 align(4)}>
type $unnamed.6072 <union {
  @sessNum u64 align(8),
  @trafficRate u64 align(8)}>
type $HpfOnlineIpNumInfo <struct {
  @onlineIpNum u32 align(4),
  @onlineIpNodeNum u32 align(4)}>
type $HpfFwdHookReq <struct {
  @version i32 align(4),
  @position i32 align(4)}>
type $HpfFlowHookMsg <struct {
  @tblType u8,
  @showType u8}>
type $HpfFlowHookMsgRet <struct {
  @retCode u32 align(4)}>
type $HpfFlowHookInfo <struct {
  @flowHookCreate <[32] u8>,
  @flowHookAging <[32] u8>,
  @flowHookScan <[32] u8>}>
type $HpfFlowHookIpv6Info <struct {
  @flowHookCreate <[32] u8>,
  @flowHookAging <[32] u8>,
  @flowHookScan <[32] u8>}>
type $HpfTopnLogHead <struct {
  @flag u32 align(4),
  @len u16 align(2),
  @rsv u16 align(2),
  @slot u64 align(8),
  @cpu u64 align(8)}>
type $HpfDbcuHead <struct {
  @id u8,
  @sign u8,
  @len u16 align(2),
  @slot u64 align(8),
  @cpu u64 align(8)}>
type $HpfDbcuDiscardInfo <struct {
  @pktDropType u32 align(4),
  @pktSport u32 align(4),
  @pktDport u32 align(4),
  @pktNatSip u32 align(4),
  @pktNatDip u32 align(4),
  @pktNatSport u32 align(4),
  @pktNatDport u32 align(4)}>
type $HpfDbcuPkt <struct {
  @tmUpms u32 align(4),
  @tm1970 u32 align(4),
  @fwPort u32 align(4),
  @pktDiscardFlag :1 u16 align(2),
  @ipType :2 u16 align(2),
  @fwVsys :13 u16 align(2),
  @fwZone u16 align(2),
  @unnamed.6075 <$unnamed.6074> implicit,
  @unnamed.6077 <$unnamed.6076> implicit,
  @unnamed.6079 <$unnamed.6078> implicit,
  @unnamed.6081 <$unnamed.6080> implicit align(2),
  @unnamed.6083 <$unnamed.6082> implicit align(4),
  @unnamed.6085 <$unnamed.6084> implicit align(2),
  @unnamed.6087 <$unnamed.6086> implicit align(4),
  @unnamed.6089 <$unnamed.6088> implicit align(4),
  @unnamed.6091 <$unnamed.6090> implicit align(4)}>
type $unnamed.6074 <union {
  @ipPro u8,
  @ipNexth u8}>
type $unnamed.6076 <union {
  @ipTtl u8,
  @ipHopl u8}>
type $unnamed.6078 <union {
  @ipOlen u8}>
type $unnamed.6080 <union {
  @ipTlen u16 align(2),
  @ipPayl u16 align(2)}>
type $unnamed.6082 <union {
  @ipId u16 align(2),
  @ipIdV6 u32 align(4)}>
type $unnamed.6084 <union {
  @ipFrag u16 align(2),
  @ipFragOffset u16 align(2)}>
type $unnamed.6086 <union {
  @ipSip u32 align(4),
  @ipSipV6 <$HpfIn6Addr>}>
type $unnamed.6088 <union {
  @ipDip u32 align(4),
  @ipDipV6 <$HpfIn6Addr>}>
type $unnamed.6090 <union {
  @ipPdu <[8] u32> align(4),
  @pktDiscardInfo <$HpfDbcuDiscardInfo> align(4)}>
type $HpfFlowTopnCfgMsg <struct {
  @execId u32 align(4),
  @vsysId u32 align(4),
  @zoneId u32 align(4),
  @topnNum u32 align(4),
  @allFlg u32 align(4),
  @entryIp u32 align(4),
  @entryIpStart u32 align(4),
  @entryIpEnd u32 align(4),
  @type u32 align(4),
  @ipTypeFlag u32 align(4)}>
type $HpfFlowTopnSessNumInfo <struct {
  @ipAddr u32 align(4),
  @vsysId u32 align(4),
  @sessNum u32 align(4)}>
type $HpfFlowTopnSessRespInfo <struct {
  @vsysId u32 align(4),
  @zoneId u32 align(4),
  @topnNum u32 align(4),
  @allFlg u32 align(4),
  @entryIp u32 align(4),
  @entryIpStart u32 align(4),
  @entryIpEnd u32 align(4),
  @status u32 align(4),
  @ipTblNum u32 align(4),
  @ipTypeFlag u32 align(4),
  @lastTime u32 align(4),
  @result <[512] <$HpfFlowTopnSessNumInfo>> align(4)}>
type $HpfFlowTopnTrSortInfo <struct {
  @ipAddr u32 align(4),
  @vsysId u32 align(4),
  @traffic u64 align(8)}>
type $HpfFlowTopnTrRespReturnInfo <struct {
  @vsysId u32 align(4),
  @zoneId u32 align(4),
  @topnNum u32 align(4),
  @allFlg u32 align(4),
  @entryIp u32 align(4),
  @entryIpStart u32 align(4),
  @entryIpEnd u32 align(4),
  @status u32 align(4),
  @ipTblNum u32 align(4),
  @ipTypeFlag u32 align(4),
  @bpsResult <[512] <$HpfFlowTopnTrSortInfo>> align(8),
  @ppsResult <[512] <$HpfFlowTopnTrSortInfo>> align(8)}>
type $HpfFlowInfoBasic <struct {
  @srcMac <[6] u8>,
  @dstMac <[6] u8>,
  @bridgeId u16 align(2),
  @ethType u16 align(2),
  @unnamed.6093 <$unnamed.6092> implicit align(4),
  @unnamed.6095 <$unnamed.6094> implicit align(4),
  @srcPort u16 align(2),
  @dstPort u16 align(2),
  @srcVrfIndex u16 align(2),
  @dstVrfIndex u16 align(2),
  @vlan u16 align(2),
  @fwdType u8,
  @isL2 u8,
  @protocol u8,
  @appId u8,
  @appIdExt u16 align(2),
  @srcNatIp u32 align(4),
  @dstNatIp u32 align(4),
  @srcNatPort u16 align(2),
  @dstNatPort u16 align(2),
  @privateFlags u32 align(4),
  @publicFlags u32 align(4),
  @fromCpe <[4] u32> align(4),
  @toCpe <[4] u32> align(4),
  @fromCpeTunnel u16 align(2),
  @toCpeTunnel u16 align(2),
  @userService u32 align(4)}>
type $unnamed.6092 <union {
  @srcIp u32 align(4),
  @srcIpv6 <$HpfIn6Addr>}>
type $unnamed.6094 <union {
  @dstIp u32 align(4),
  @dstIpv6 <$HpfIn6Addr>}>
type $HpfFlowSessIdInfo <struct {
  @sessAddrHigh u32 align(4),
  @sessAddrLow u32 align(4),
  @hashIndex u32 align(4),
  @createTime u32 align(4),
  @slotId u8,
  @cpuId u8,
  @vsysId u16 align(2)}>
type $HpfFlowTopnPktFlowInfo <struct {
  @sess <$HpfFlowInfoBasic> align(4),
  @sessId <$HpfFlowSessIdInfo> align(4),
  @revPkts u32 align(4),
  @sendPkts u32 align(4),
  @revBytes u64 align(8),
  @sendBytes u64 align(8),
  @totalPkts u64 align(8)}>
type $HpfFlowTopnPktFlowRespInfo <struct {
  @vsysId u32 align(4),
  @zoneId u32 align(4),
  @topnNum u32 align(4),
  @type u16 align(2),
  @status u16 align(2),
  @result <[64] <$HpfFlowTopnPktFlowInfo>> align(8)}>
type $HpfFlowTopnHundredSessionCfg <struct {
  @srcIp u32 align(4),
  @dstIp u32 align(4),
  @srcPort u16 align(2),
  @dstPort u16 align(2),
  @srcVrfIndex u16 align(2),
  @dstVrfIndex u16 align(2),
  @vlan u16 align(2),
  @fwdType u8,
  @protocol u8,
  @packets u32 align(4),
  @bytes u64 align(8),
  @revPackets u32 align(4),
  @revBytes u32 align(4),
  @vsys u32 align(4),
  @mac <[6] u8>,
  @outPort u32 align(4),
  @nextHop u32 align(4),
  @recvPort u32 align(4),
  @obvVrfIndex u32 align(4)}>
type $HpfFwdIicMsgInfo <struct {
  @msgType u32 align(4),
  @infoNum u32 align(4),
  @endFlag u32 align(4)}>
type $HpfTcpMssZoneMsg <struct {
  @srcVrfIndex u16 align(2),
  @dstVrfIndex u16 align(2),
  @srcZone u8,
  @dstZone u8}>
type $HpfTcpMssMsg <struct {
  @tcpMssSize u16 align(2),
  @undoFlag u16 align(2),
  @vsysId u16 align(2),
  @zoneCfgFlag u16 align(2)}>
type $HpfTcpMssKeychainMsg <struct {
  @mssKeyChainEnable u16 align(2),
  @res u16 align(2)}>
type $HpfIpcmv6ErrMsg <struct {
  @disableFlag u16 align(2),
  @res u16 align(2)}>
type $HpfTcpSeqMsg <struct {
  @checkFlag u16 align(2),
  @res u16 align(2)}>
type $HpfAckRateLimit <struct {
  @rate u32 align(4)}>
type $HpfFwdTraceMsgHead <struct {
  @msgId u16 align(2),
  @data u16 align(2)}>
type $HpfFwdTraceMsgData <struct {
  @cmd u16 align(2),
  @aclNum u16 align(2),
  @aclGroupId u32 align(4),
  @number u32 align(4),
  @output u32 align(4),
  @aclType u32 align(4),
  @flow u8,
  @allSys u8,
  @isEnable u8,
  @discard <[1080] u8>,
  @vsysIndex u16 align(2)}>
type $HpfFwdTraceDbgWebMsg <struct {
  @unSA <$unnamed.6096> align(4),
  @unDA <$unnamed.6097> align(4),
  @usSPBegin u16 align(2),
  @usSPEnd u16 align(2),
  @usDP u16 align(2),
  @usApp u16 align(2),
  @usVlan u16 align(2),
  @usVsysIndex u16 align(2),
  @ucPro u8,
  @ucMode u8,
  @ucIPv6 :1 u8,
  @ucChs :1 u8,
  @ucEnable :1 u8,
  @ucHadSMAC :1 u8,
  @ucHadDMAC :1 u8,
  @ucRes :3 u8,
  @ucreserve u8,
  @ucSMAC <[6] u8>,
  @ucDMAC <[6] u8>,
  @ulUserId u32 align(4),
  @ulInIf u32 align(4)}>
type $unnamed.6096 <union {
  @ulSIP u32 align(4),
  @stSIP6 <$HpfIn6Addr>}>
type $unnamed.6097 <union {
  @ulDIP u32 align(4),
  @stDIP6 <$HpfIn6Addr>}>
type $HpfDbgMsg <struct {
  @usLength u16 align(2),
  @ucModule u8,
  @ucSlot u8,
  @ucCpu u8,
  @ucExtType u8,
  @ucDumpLen u8,
  @ucres u8,
  @ulTimestamp u32 align(4),
  @output u32 align(4),
  @ucData <[0] u8>}>
type $HpfDbgExtTrace <struct {
  @unSA <$unnamed.6098> align(4),
  @unDA <$unnamed.6099> align(4),
  @ctrlIfIndex u32 align(4),
  @dropIndex u32 align(4),
  @usSrcPort u16 align(2),
  @usDstPort u16 align(2),
  @usIpId u16 align(2),
  @usIpFrag u16 align(2),
  @usSrcVrf u16 align(2),
  @usDstVrf u16 align(2),
  @usVsysIndex u16 align(2),
  @ucPro u8,
  @tcpFlag u8,
  @pktDir u8,
  @pktIndex u8,
  @ucIsWeb u8,
  @traceFlag u8,
  @zoneIn u16 align(2),
  @ulTimestamp u32 align(4),
  @srcMac <[6] u8>,
  @dstMac <[6] u8>,
  @ethType u16 align(2),
  @vlanId u16 align(2),
  @ucData <[0] u8>}>
type $unnamed.6098 <union {
  @ulSIP u32 align(4),
  @stSIP6 <$HpfIn6Addr>}>
type $unnamed.6099 <union {
  @ulDIP u32 align(4),
  @stDIP6 <$HpfIn6Addr>}>
type $HpfDbgExtTraceInfo <struct {
  @traceId u16 align(2),
  @dropFlag :1 u8,
  @rev :7 u8,
  @fwdType u8,
  @name <[128] i8>,
  @detail <[512] i8>}>
type $HpfPktTraceInfoS <struct {
  @fwdType u8,
  @name <[128] i8>,
  @detail <[512] i8>,
  @dropFlag :1 u8,
  @res :7 u8,
  @traceId u16 align(2),
  @infoNext <* <$HpfPktTraceInfoS>> align(8)}>
type $HpfPktTracePacketS <struct {
  @ipId u16 align(2),
  @traceNum u8,
  @pktIndex u8,
  @pktNext <* <$HpfPktTracePacketS>> align(8),
  @infoHead <* <$HpfPktTraceInfoS>> align(8)}>
type $HpfPktTraceFlow <struct {
  @unSA <$unnamed.6100> align(4),
  @unDA <$unnamed.6101> align(4),
  @usSP u16 align(2),
  @usDP u16 align(2),
  @usVsysIndex u16 align(2),
  @ucPro u8,
  @ucRse u8,
  @usPktNum u32 align(4),
  @pktHead <* <$HpfPktTracePacketS>> align(8)}>
type $unnamed.6100 <union {
  @ulSIP u32 align(4),
  @stSIP6 <$HpfIn6Addr>}>
type $unnamed.6101 <union {
  @ulDIP u32 align(4),
  @stDIP6 <$HpfIn6Addr>}>
type $HpfScrlCfg <struct {
  @enable u16 align(2),
  @maxCpu u16 align(2),
  @minCpu u16 align(2),
  @resv u16 align(2)}>
type $HpfSessRateCarCfg <struct {
  @enable u32 align(4),
  @destMinLimit u32 align(4),
  @souceMinLimit u32 align(4)}>
type $HpfScrlPassCfg <struct {
  @pass u8,
  @pro u8,
  @val u16 align(2)}>
type $HpfFlowHashTblInfo <struct {
  @arrayNum u32 align(4),
  @nodeNumber u32 align(4),
  @minLength u32 align(4),
  @maxLength u32 align(4),
  @lengthStat <[8] u32> align(4),
  @emptyNotZero u32 align(4),
  @zeroNotEmpty u32 align(4),
  @zeroNotEmptyIndex <[64] u32> align(4)}>
type $HpfFlow6LogMsgData <struct {
  @logType u32 align(4),
  @thresValue u32 align(4)}>
type $HpfAclStatMsg <struct {
  @aclNum u32 align(4),
  @aclGroupId u32 align(4),
  @enableFlag u16 align(2),
  @undoFlag u16 align(2),
  @allSysFlag u16 align(2),
  @greInnerEnableFlag u16 align(2),
  @vsysId u32 align(4)}>
type $HpfGetAclStatMsg <struct {
  @srcIp u32 align(4),
  @dstIp u32 align(4),
  @srcPort u16 align(2),
  @dstPort u16 align(2),
  @vrfIndex u16 align(2),
  @protocol u8,
  @discard u8,
  @outerProtocol u8,
  @finish i8,
  @retNum u16 align(2),
  @beginIndex u32 align(4),
  @errNum u32 align(4),
  @statSpecNum u32 align(4)}>
type $HpfAclRange32 <struct {
  @value u32 align(4),
  @mask u32 align(4)}>
type $HpfAclRange16 <struct {
  @value u16 align(2),
  @mask u16 align(2)}>
type $HpfAclRange8 <struct {
  @value u8,
  @mask u8}>
type $HpfAclStatRule <struct {
  @aclNumber u32 align(4),
  @ruleNumber u32 align(4),
  @validFlag u32 align(4),
  @aclActionMask u32 align(4),
  @vrfIndex u32 align(4),
  @tcamIndex u32 align(4),
  @statId u32 align(4),
  @protocol <$HpfAclRange8>,
  @tos <$HpfAclRange8>,
  @srcIp <$HpfAclRange32> align(4),
  @destIp <$HpfAclRange32> align(4),
  @dstPort <$HpfAclRange16> align(2),
  @srcPort <$HpfAclRange16> align(2),
  @icmp <$HpfAclRange16> align(2)}>
type $frag_print_info_s <struct {
  @src_ip u32 align(4),
  @dst_ip u32 align(4),
  @pkt_id u16 align(2),
  @vrf_index u16 align(2),
  @protocol u8,
  @src_port u16 align(2),
  @dst_port u16 align(2),
  @vlan_id u16 align(2),
  @pkt_cnt u16 align(2),
  @cur_offset u16 align(2),
  @create_time u64 align(8),
  @ttl u32 align(4),
  @valid :1 u8,
  @fst_rcv :1 u8,
  @reassemble :1 u8,
  @overlap :1 u8,
  @last_rcv :1 u8,
  @buff_full :1 u8,
  @frag4_fifo :1 u8,
  @reserved :1 u8}>
type $frag6_print_info_s <struct {
  @src_ip <$HpfIn6Addr>,
  @dst_ip <$HpfIn6Addr>,
  @pkt_id u32 align(4),
  @vrf_index u16 align(2),
  @src_port u16 align(2),
  @dst_port u16 align(2),
  @vlan_id u16 align(2),
  @protocol u8,
  @pkt_cnt u16 align(2),
  @cur_offset u16 align(2),
  @create_time u64 align(8),
  @ttl u32 align(4),
  @valid :1 u8,
  @fst_rcv :1 u8,
  @reassemble :1 u8,
  @last_rcv :1 u8,
  @frag6_fifo :1 u8,
  @ucRes :3 u8}>
type $HpfSystemStatMsg <struct {
  @cmd u8,
  @enable u8,
  @disType u8,
  @reserved u8}>
type $HpfSystemStatKeyData <struct {
  @statDataType u32 align(4),
  @statDataIndex u32 align(4),
  @statData u64 align(8)}>
type $HpfStat6Cfg <struct {
  @statEnable u8,
  @allSystem u8,
  @vsysIndex u16 align(2),
  @aclNumber u32 align(4),
  @aclGroupId u32 align(4),
  @timeOut u32 align(4)}>
type $HpfStat6CfgRetMsg <struct {
  @retCode u32 align(4)}>
type $HpfStat6Info <struct {
  @ip6BriefCnt <[5] u64> align(8),
  @ip6DiscardCnt <[1077] u64> align(8),
  @ip6SndCnt <[9] u64> align(8),
  @ip6RcvCnt <[26] u64> align(8),
  @ip6SessCnt <[21] u64> align(8),
  @ip6AppByteCnt <[71] u64> align(8),
  @ip6InnerCnt <[48] u64> align(8)}>
type $HpfMbuftrPktDbgInfo <struct {
  @dbgFlag <[170] u32> align(4),
  @srcIp u32 align(4),
  @dstIp u32 align(4),
  @srcPort u16 align(2),
  @dstPort u16 align(2),
  @number u32 align(4),
  @proto u8,
  @res <[3] u8>}>
type $HpfMbuftrStatInfo <struct {
  @recvNum <[84] u64> align(8),
  @sendNum <[84] u64> align(8),
  @carDiscNum <[170] u64> align(8)}>
type $HpfSessMonReport <struct {
  @slotId u8,
  @cpuId u8,
  @vrfIndex u16 align(2),
  @opCode u32 align(4),
  @protocol u8,
  @dropCode u32 align(4),
  @srcIp u32 align(4),
  @dstIp u32 align(4),
  @srcPort u16 align(2),
  @dstPort u16 align(2),
  @pktInfo <$unnamed.6102> align(4),
  @pktData <[256] i8>}>
type $unnamed.6102 <union {
  @tcpInfo <$unnamed.6103> align(4),
  @sctpInfo <$unnamed.6104> align(4)}>
type $unnamed.6103 <struct {
  @seqNum u32 align(4),
  @ackNum u32 align(4),
  @tcpFlag u8}>
type $unnamed.6104 <struct {
  @chunckType u8,
  @chunckFlag u8,
  @verifyTag u32 align(4)}>
type $HpfSnmpTrapCfg <struct {
  @cmd u32 align(4),
  @threshold u32 align(4)}>
type $HpfGreCfgMsgHead <struct {
  @cmdSign u16 align(2),
  @number u16 align(2),
  @tunnelId u32 align(4),
  @startTunnelId u32 align(4),
  @ctrlIfIdx u32 align(4),
  @para u32 align(4)}>
type $HpfGreReturnTunMsgHead <struct {
  @nextTunnelId u32 align(4)}>
type $tbl_flt_tlv_s <struct {
  @type u32 align(4),
  @value u32 align(4)}>
type $tbl_cfg_msg_head_s <struct {
  @msg_id u16 align(2),
  @tbl_type u8,
  @data u8}>
type $tag_tbl_total_sess_info <struct {
  @total_num u32 align(4),
  @time_stamp u32 align(4)}>
type $tbl_cfg_return_msg_head_s <struct {
  @ret_code u32 align(4)}>
type $tbl_cfg_get_info_ctx_s <struct {
  @ctx u32 align(4)}>
type $HpfFlowCfgMsgHead <struct {
  @msgId u16 align(2),
  @data :4 u8,
  @flag :4 u8,
  @tblType u8}>
type $HpfFlowFltTlv <struct {
  @type u32 align(4),
  @value u32 align(4)}>
type $HpfFlowCfgGetInfoCtx <struct {
  @ctx u32 align(4),
  @count u32 align(4)}>
type $HpfFlowCfgMsg <struct {
  @head <$HpfFlowCfgMsgHead> align(2),
  @ctx <$HpfFlowCfgGetInfoCtx> align(4),
  @filterTlv <[124] <$HpfFlowFltTlv>> align(4)}>
type $tag_HpfAclStatid <struct {
  @ruleid u64 align(8),
  @eidnum u8,
  @eid1 <[15] u32> align(4),
  @eid2 <[15] u32> align(4),
  @statid1 u16 align(2),
  @statid2 u16 align(2),
  @statid3 u16 align(2),
  @statid4 u16 align(2),
  @next <* <$tag_HpfAclStatid>> align(8)}>
type $HpfAclStatNpMsg <struct {
  @aclRulePriority u32 align(4),
  @condMask u32 align(4),
  @srcIpAddr u32 align(4),
  @srcIpMask u32 align(4),
  @dstIpAddr u32 align(4),
  @dstIpMask u32 align(4),
  @srcPortBegin u16 align(2),
  @srcPortEnd u16 align(2),
  @dstPortBegin u16 align(2),
  @dstPortEnd u16 align(2),
  @protocol u8,
  @tosValue u8,
  @vrfIndex u16 align(2),
  @resv u8,
  @icmpType u8,
  @icmpCode u8,
  @anyFlag u8,
  @ruleId u64 align(8),
  @statId1 u16 align(2),
  @statId2 u16 align(2),
  @statId3 u16 align(2),
  @statId4 u16 align(2)}>
type $fwd_acl_stat_result_vm_t <struct {
  @obverse_recv u64 align(8),
  @obverse_disc u64 align(8),
  @reverse_recv u64 align(8),
  @reverse_disc u64 align(8),
  @disc <[64] u64> align(8)}>
type $tag_HpfIpv6AclStatid <struct {
  @ruleid u64 align(8),
  @eidnum u8,
  @eid1 <[15] u32> align(4),
  @eid2 <[15] u32> align(4),
  @statid1 u16 align(2),
  @statid2 u16 align(2),
  @statid3 u16 align(2),
  @statid4 u16 align(2),
  @next <* <$tag_HpfIpv6AclStatid>> align(8)}>
type $HpfIpv6AclStatNpMsg <struct {
  @aclRulePriority u32 align(4),
  @condMask u32 align(4),
  @srcIpv6Addr <[16] u8>,
  @srcIpv6Mask <[16] u8>,
  @dstIpv6Addr <[16] u8>,
  @dstIpv6Mask <[16] u8>,
  @srcPortBegin u16 align(2),
  @srcPortEnd u16 align(2),
  @dstPortBegin u16 align(2),
  @dstPortEnd u16 align(2),
  @protocol u8,
  @dscpValue u8,
  @vrfIndex u16 align(2),
  @icmpType u8,
  @icmpCode u8,
  @anyFlag u8,
  @resv u8,
  @ruleId u64 align(8),
  @statId1 u16 align(2),
  @statId2 u16 align(2),
  @statId3 u16 align(2),
  @statId4 u16 align(2)}>
type $HpfIpv6AclStatResult <struct {
  @obverse_recv u64 align(8),
  @obverse_disc u64 align(8),
  @reverse_recv u64 align(8),
  @reverse_disc u64 align(8),
  @disc <[64] u64> align(8)}>
type $HpfArpMissCachePrintTblInfo <struct {
  @vrf u16 align(2),
  @nextHop u32 align(4),
  @cachedPktCnt u32 align(4)}>
type $HpfArpMissCachePrintPktInfo <struct {
  @srcIp u32 align(4),
  @dstIp u32 align(4),
  @ttl u32 align(4),
  @elapseTime u32 align(4)}>
type $HpfNdMissCachePrintTblInfo <struct {
  @vrf u16 align(2),
  @nextHop <[4] u32> align(4),
  @cachedPktCnt u32 align(4)}>
type $HpfNdMissCachePrintPktInfo <struct {
  @srcIp <[4] u32> align(4),
  @dstIp <[4] u32> align(4),
  @ttl u32 align(4),
  @elapseTime u32 align(4)}>
type $HpfTransCtrlIfIndex <struct {
  @ifIndex u32 align(4)}>
type $HppNpFlowSessStatRet <struct {
  @status u32 align(4),
  @npIpv4FwdNum u32 align(4),
  @cpuIpv4FwdNum u32 align(4),
  @npIpv6FwdNum u32 align(4),
  @cpuIpv6FwdNum u32 align(4)}>
type $HpfDelayWatchDesc <struct {
  @strEnglish <* i8> align(8),
  @strChinese <* i8> align(8),
  @solution <* i8> align(8)}>
type $WatchPointRetMsg <struct {
  @retCode u32 align(4)}>
type $HppPktBtGetResultMsg <union {
  @value u32 align(4),
  @s <$unnamed.6105>}>
type $unnamed.6105 <struct {
  @index u8,
  @verbose u8,
  @ipType u8,
  @infoType u8}>
type $HpfServerMapMsgHead <struct {
  @msgType u16 align(2),
  @reserved u16 align(2)}>
type $HpfServerMapFilterTlv <struct {
  @type u32 align(4),
  @value u32 align(4)}>
type $HpfNatAddressGroupMsg <struct {
  @msgType u16 align(2),
  @id u16 align(2)}>
type $HpfPktDecode <struct {
  @srcMac <[6] i8>,
  @dstMac <[6] i8>,
  @srcAddr <$HpfIn46Addr>,
  @dstAddr <$HpfIn46Addr>,
  @ethType u16 align(2),
  @vlanId u16 align(2),
  @protocol u8,
  @srcPort u16 align(2),
  @dstPort u16 align(2),
  @tos u8,
  @isFrag u8,
  @isFirstFrag u8,
  @isLastFrag u8,
  @ipId u16 align(2),
  @ethLen u16 align(2)}>
type $unnamed.6106 <union {
  @next_hop u32 align(4),
  @next_hop_v6 <[4] u32> align(4),
  @align_rsv u64 align(8)}>
type $fwd_pkt_back_from_remote_nge_info_s <struct {
  @dst_mac_type u8,
  @l2_length u8,
  @flag_ipv6 :1 u16 align(2),
  @flag_trace :2 u16 align(2),
  @flag_l2_bridge :1 u16 align(2),
  @vlan_id_in :12 u16 align(2),
  @flow_hash_index u32 align(4),
  @recv_port u32 align(4),
  @send_port u32 align(4),
  @flow_ptr u64 align(8),
  @org_recv_port u32 align(4),
  @logic_send_port u32 align(4),
  @vsys_index u16 align(2),
  @is_destroy :1 u8,
  @is_new_mbuf :1 u8,
  @is_copy_mbuf :1 u8,
  @cross_vsys_limit :3 u8,
  @reserved :2 u8,
  @nsh_flag :1 u8,
  @trace_pkt_in :1 u8,
  @resv :6 u8,
  @nsh_spi :24 u32 align(4),
  @nsh_si :8 u32 align(4),
  @send_vrf u16 align(2),
  @recv_vrf u16 align(2),
  @backFlag u8,
  @flowRecvVrf u16 align(2),
  @flowSendVrf u16 align(2),
  @egressPort u32 align(4),
  @fwdType u8,
  @unnamed.6109 <$unnamed.6108> implicit align(4),
  @ip6Token u32 align(4),
  @inZone u16 align(2),
  @outZone u16 align(2),
  @vlan u16 align(2),
  @routeGateWay :1 u8,
  @isCrossVrf :1 u8,
  @timeStampValid :1 u8,
  @dfxTrace :1 u8,
  @traceInstance :3 u8,
  @resv2 :1 u8,
  @timeStamp u32 align(4)}>
type $unnamed.6108 <union {
  @nhp u32 align(4),
  @nhp6 <$HpfIn6Addr>}>
type $HpfArpMissNp <struct {
  @srcIp u32 align(4),
  @nhpIp u32 align(4),
  @unnamed.6113 <$unnamed.6110> implicit align(4),
  @srcVsiId u16 align(2),
  @dstVsiId u16 align(2),
  @vxlanFlag u8,
  @direct u8,
  @srcVrf u16 align(2)}>
type $unnamed.6110 <union {
  @nhpIndex u32 align(4),
  @unnamed.6112 <$unnamed.6111> implicit align(2)}>
type $unnamed.6111 <union {
  @dvp u16 align(2),
  @svp u16 align(2)}>
type $HpfArpMissCpu <struct {
  @srcIp u32 align(4),
  @nhpIp u32 align(4),
  @ifIndex u32 align(4),
  @srcVrf u16 align(2),
  @rsv4 u16 align(2)}>
type $HpfArpMissMsg <struct {
  @missType u8,
  @rsv1 <[3] u8>,
  @unnamed.6115 <$unnamed.6114> implicit align(4)}>
type $unnamed.6114 <union {
  @arpMissCpu <$HpfArpMissCpu> align(4),
  @arpMissNp <$HpfArpMissNp> align(4)}>
type $HpfEthAdpArpMissOpcodeIpv4 <struct {
  @sIp u32 align(4),
  @rsvd1 u32 align(4),
  @rsvd2 u32 align(4),
  @rsvd3 u32 align(4),
  @nhp <$unnamed.6116> align(4),
  @rsvd4 u32 align(4),
  @rsvd5 u32 align(4),
  @rsvd6 u32 align(4)}>
type $unnamed.6116 <union {
  @nhIp u32 align(4),
  @nhpIndex u32 align(4)}>
type $HpfEthAdpNdMissOpcodeIpv6 <struct {
  @sipv6Addr <$HpfIn6Addr>,
  @nhp6 <$unnamed.6117> align(4)}>
type $unnamed.6117 <union {
  @nhipIpv6Addr <$HpfIn6Addr>,
  @nhpIndex u32 align(4)}>
type $HpfEthAdpArpNdMiss <struct {
  @LW0 <$unnamed.6118> align(4),
  @LW1 u32 align(4),
  @unnamed.6120 <$unnamed.6119> implicit align(4)}>
type $unnamed.6118 <struct {
  @sp :8 u32 align(4),
  @rsvd8 :20 u32 align(4),
  @opcode :4 u32 align(4)}>
type $unnamed.6119 <union {
  @ipv4 <$HpfEthAdpArpMissOpcodeIpv4> align(4),
  @ipv6 <$HpfEthAdpNdMissOpcodeIpv6> align(4)}>
type $HpfV2EthAdpArpNdMiss <struct {
  @rsvd9 :3 u16 align(2),
  @direct :1 u16 align(2),
  @opCode :4 u16 align(2),
  @rsvd8 :8 u16 align(2),
  @dstVsiId u16 align(2),
  @unnamed.6122 <$unnamed.6121> implicit align(4)}>
type $unnamed.6121 <union {
  @ipv4 <$HpfEthAdpArpMissOpcodeIpv4> align(4),
  @ipv6 <$HpfEthAdpNdMissOpcodeIpv6> align(4)}>
type $HpfNdMissNp <struct {
  @srcIp <[4] u32> align(4),
  @nhpIp <[4] u32> align(4),
  @unnamed.6126 <$unnamed.6123> implicit align(4),
  @srcVsiId u16 align(2),
  @dstVsiId u16 align(2),
  @direct u8,
  @vxlanFlag u8,
  @srcVrf u16 align(2)}>
type $unnamed.6123 <union {
  @nhpIndex u32 align(4),
  @unnamed.6125 <$unnamed.6124> implicit align(2)}>
type $unnamed.6124 <union {
  @dvp u16 align(2),
  @svp u16 align(2)}>
type $HpfNdMissCpu <struct {
  @srcIp <[4] u32> align(4),
  @nhpIp <[4] u32> align(4),
  @ifIndex u32 align(4),
  @srcVrf u16 align(2),
  @rsv3 u16 align(2)}>
type $HpfNdMissMsg <struct {
  @missType u8,
  @rsv1 <[3] u8>,
  @unnamed.6128 <$unnamed.6127> implicit align(4)}>
type $unnamed.6127 <union {
  @ndMissCpu <$HpfNdMissCpu> align(4),
  @ndMissNp <$HpfNdMissNp> align(4)}>
type $tagDPHashTableInfo <struct {
  @ulHashBaseAddr u32 align(4),
  @ulHashSize u32 align(4),
  @ulNodeSize u32 align(4),
  @ulNodeNum u32 align(4)}>
type $tagDP_HASHARRAY_S <struct {
  @firstNode <* <$tagHpeDpHashBucket>> align(8),
  @rwlock <$HpeRwlock> align(4),
  @ulCacheCounter u32 align(4),
  @lNodeCounter u32 align(4)}>
type $HpfAclRmRange <struct {
  @begin u16 align(2),
  @offset u8,
  @rangeType u8,
  @rangeLow u16 align(2),
  @rangeHigh u16 align(2)}>
type $HpfAclRmEntry <struct {
  @priority u32 align(4),
  @numRange u16 align(2),
  @keyAndMask <[160] u8>,
  @range <[0] <$HpfAclRmRange>> align(2)}>
type $tagAclSearchNode <struct {
  @nextNodeIdx u16 align(2),
  @nodeType u16 align(2),
  @bitIdx u16 align(2),
  @numRules u16 align(2),
  @ruleUintSize u16 align(2),
  @rules <* <$tagHpfAclSearchRule>> align(8),
  @child <[2] <* <$tagAclSearchNode>>> align(8)}>
type $tag_acl_search_jumptable <struct {
  @cutbits <[15] u16> align(2),
  @numRoots u32 align(4),
  @numCutbits u32 align(4),
  @bitset <[15] u64> align(8),
  @cutbitMask <[6] u64> align(8),
  @offset <[6] u8>,
  @keyId <[15] u8>,
  @roots <* <* <$tagAclSearchNode>>> align(8)}>
type $HpfAclRuleSet <struct {
  @len u32 align(4),
  @first u32 align(4),
  @maxLen u32 align(4),
  @holeList <* u32> align(8),
  @data <* <$HpfAclRule>> align(8)}>
type $tagHpfAclListNode <struct {
  @data u64 align(8),
  @next <* <$tagHpfAclListNode>> align(8)}>
type $HpfAclBinTreeStats <struct {
  @treeDepth u32 align(4),
  @numRules u32 align(4),
  @numDups u32 align(4),
  @numNodes u32 align(4),
  @numIntrNodes u32 align(4),
  @numLeafNodes u32 align(4)}>
type $HpfAclSortPair <struct {
  @key u32 align(4),
  @index u32 align(4)}>
type $HpfAclCutBitStat <struct {
  @bitSum u32 align(4),
  @wSum u32 align(4),
  @idealBit u32 align(4)}>
type $HpfAclUpdHistory <struct {
  @addRule u32 align(4),
  @removeRule u32 align(4),
  @changeRule u32 align(4),
  @numDups u32 align(4),
  @isOriginalTree u32 align(4),
  @removeFound :1 u32 align(4),
  @rsv :31 u32 align(4),
  @lastRuleId u32 align(4),
  @lastRuleIndex u32 align(4),
  @lastRuleLocId u32 align(4),
  @rmvNumFind u32 align(4),
  @tableRbld u16 align(2),
  @cutLeaf u16 align(2),
  @tooManyDup u32 align(4),
  @tooHeight u32 align(4),
  @invalidCut u32 align(4),
  @tooManyTrees u32 align(4),
  @unKnowReason u32 align(4),
  @lastRebuildNum u32 align(4),
  @numUpd u32 align(4),
  @lastUpdRebuild u32 align(4),
  @numRuleDup u32 align(4),
  @oldRuleBv <* u32> align(8),
  @oldDtRules <* <$HpfAclRuleIdSet>> align(8)}>
type $HpeTbmOperate <struct {
  @uiTblAdd u32 align(4),
  @uiTblUpdate u32 align(4),
  @uiTblQueryAndUpdate u32 align(4),
  @uiTblDelete u32 align(4),
  @uiTblPurge u32 align(4),
  @uiTblQuery u32 align(4),
  @uiTblCreate u32 align(4),
  @uiTblDestroy u32 align(4),
  @uiTblQueryNum u32 align(4),
  @uiTblQueryNext u32 align(4)}>
type $HpeFwdTblRelaSpec <struct {
  @tblName u16 align(2),
  @relaTblName <[10] u16> align(2),
  @relaTimes <[10] u16> align(2)}>
type $unnamed.6129 <union {
  @room <[8] u32> align(4)}>
type $HpeCapSrvmapKey <struct {
  @key <$HpeSrvmapField> align(4),
  @mask <$HpeSrvmapField> align(4)}>
type $HpeCapSrvmapData <struct {
  @svrmapKey <$HpeCapSrvmapKey> align(4),
  @ifNext <* <$HpeCapSrvmapData>> align(8),
  @fake :1 u32 align(4),
  @srvMap :1 u32 align(4),
  @srvMapRsv :1 u32 align(4),
  @natFilter :1 u32 align(4),
  @natmap :1 u32 align(4),
  @nopat :1 u32 align(4),
  @dynamic :1 u32 align(4),
  @alp :1 u32 align(4),
  @acl :1 u32 align(4),
  @fthrVaild :1 u32 align(4),
  @again :1 u32 align(4),
  @aspfType :5 u32 align(4),
  @deny :1 u32 align(4),
  @rsv1 :15 u32 align(4),
  @newIp u32 align(4),
  @newIpMask u32 align(4),
  @newport u16 align(2),
  @outVpnId u16 align(2),
  @pptPort u16 align(2),
  @natPoolId u16 align(2),
  @alpSsnIndex u16 align(2),
  @alpFthrSsnIndex u16 align(2),
  @pFthrFlwAddr <* void> align(8),
  @natIfIndex u16 align(2),
  @rsv2 u16 align(2),
  @referCnt <$HpeAtom32> align(4)}>
type $HpeAclRuleBinaryField <struct {
  @field u8,
  @mask u8,
  @rsv u16 align(2)}>
type $HpeAclGroupCfg <struct {
  @keySize u16 align(2),
  @actSize u16 align(2),
  @entNum u32 align(4),
  @instanceId u32 align(4)}>
type $HpeAclGroupKey <struct {
  @groupType u32 align(4),
  @groupId u32 align(4)}>
type $HpeAclPriorityNode <struct {
  @priority u32 align(4),
  @next <* <$HpeAclPriorityNode>> align(8)}>
type $HpeAclStatKey <struct {
  @groupType u32 align(4),
  @groupId u32 align(4),
  @statId u32 align(4)}>
type $unnamed.6131 <struct {
  @aclPassPktCnt u64 align(8),
  @aclPassByte u64 align(8),
  @aclFilterPktCnt u64 align(8),
  @aclFilterByte u64 align(8)}>
type $HpeAclStatEntry <struct {
  @statKey <$HpeAclStatKey> align(4),
  @statData <$HpeAclStatData> align(8),
  @version u32 align(4),
  @resv u32 align(4)}>
type $unnamed.6132 <struct {
  @actionEntry <$HpeFwdTblInfo> align(8),
  @actionTemp <$HpeFwdTblInfo> align(8)}>
type $HpeAclGroupEntry <struct {
  @key <$HpeAclGroupKey> align(4),
  @data <$HpeAclGroup> align(8)}>
type $tagPfaAclGroupEntry <struct {
  @ruleEntry <$HpeFwdTblInfo> align(8),
  @ruleIndex <$HpeFwdTblInfo> align(8)}>
type $HpeMbLock <struct {
  @list <$tagHpeListHead> align(8),
  @timestamp u64 align(8),
  @m <* void> align(8),
  @type u32 align(4)}>
type $HpeTbmVerify <struct {
  @tblVersion u32 align(4),
  @tblVerifyEnable u8,
  @tblVerifyBeginFlag u8,
  @tblVerifyBeginNum u8,
  @tblAgeFlag u8,
  @tblDownloadFlag u8}>
type $HpeTbmGlobalVerify <struct {
  @version u32 align(4),
  @tbmVerify <$HpeTbmVerify> align(4)}>
type $HpeGlobalTblVerifyAge <struct {
  @tableName <* i8> align(8),
  @globalTblAge <* <func () void>> align(8),
  @verifyEnable u32 align(4)}>
type $HpfAclNotifyHead <struct {
  @lock <$HpeRwlock> align(4),
  @head <$tagHpeListHead> align(8)}>
type $HpfAclNotifyNode <struct {
  @node <$tagHpeListHead> align(8),
  @cb <* <func (u32,u32,u32) void>> align(8)}>
type $HpfMbuftrCarNode <struct {
  @token i32 align(4),
  @timeStamp u32 align(4),
  @swap u32 align(4)}>
type $HpfMbuftrCarExtSampleNode <struct {
  @unnamed.6135 <$unnamed.6134> implicit align(4),
  @count u32 align(4)}>
type $unnamed.6134 <union {
  @ipv4Addr u32 align(4),
  @ipv6Addr <$HpfIn6Addr>,
  @mac <[6] u8>,
  @port u32 align(4)}>
type $HpfMbuftrSecHead <struct {
  @unnamed.6137 <$unnamed.6136> implicit align(4),
  @unnamed.6139 <$unnamed.6138> implicit align(4),
  @inIfIdx u32 align(4),
  @vrf u16 align(2),
  @vlanIn u16 align(2)}>
type $unnamed.6136 <union {
  @srcIp u32 align(4),
  @srcIpV6 <$HpfIn6Addr>}>
type $unnamed.6138 <union {
  @dstIp u32 align(4),
  @dstIpV6 <$HpfIn6Addr>}>
type $HostNpHeadTotalCar <struct {
  @dmac <[6] u8>,
  @smac <[6] u8>,
  @vlanTag u32 align(4),
  @ethType u16 align(2),
  @causeId u16 align(2)}>
type $HpfCpcarIndexParseMap <struct {
  @layerType u32 align(4),
  @carIndex u32 align(4),
  @cpcarIndexParseFunc <* <func (<* <$hpe_mbuf> align(64)>) u32>> align(8)}>
type $HpfGreTunnelPhyInfo <struct {
  @inByte u64 align(8),
  @outByte u64 align(8),
  @inPkt u64 align(8),
  @outPkt u64 align(8),
  @inErr u64 align(8),
  @outErr u64 align(8),
  @inMcasts u64 align(8),
  @outMcasts u64 align(8),
  @inUcasts u64 align(8),
  @outUcasts u64 align(8)}>
type $HpfGreTunnelProductInfo <struct {
  @smartFragFlag u32 align(4),
  @innerHashFlag u32 align(4),
  @packetFilterFlag u32 align(4)}>
type $HpfGreUptunnelTable <struct {
  @outportIndex u32 align(4),
  @outportIndexVlanIf u32 align(4),
  @vlanId u32 align(4),
  @nextHop u32 align(4),
  @tunnelIp <$HpfIn46Addr>,
  @vrfIndex u32 align(4),
  @srcAddr u32 align(4),
  @srcVrf u32 align(4),
  @dstAddr u32 align(4),
  @dstVrf u32 align(4),
  @zoneId u32 align(4),
  @slot u32 align(4),
  @tunnelId u32 align(4),
  @encapProto u8,
  @transmitProto u8,
  @tunnelMode u8,
  @greIpsec u8,
  @nhrpRedirect u8,
  @nhrpShortCut u8,
  @nhrpServer u8,
  @res u8}>
type $HpfGreStat <struct {
  @inBytes u64 align(8),
  @inPkts u64 align(8),
  @inPktsSum u64 align(8),
  @inSlicePktsSum u64 align(8),
  @inNoTunnDiscard u64 align(8),
  @inNoGreProPkts u64 align(8),
  @inTagVerErrs u64 align(8),
  @inTagCkSumErrs u64 align(8),
  @inTagKeyErrs u64 align(8),
  @compFaild u64 align(8),
  @inTransAfterDecapPkts u64 align(8),
  @inNatPkts u64 align(8),
  @inLocalCpuPkts u64 align(8),
  @inOtherCpuPkts u64 align(8),
  @junkPkts u64 align(8),
  @outBytes u64 align(8),
  @outPkts u64 align(8),
  @outErrs u64 align(8),
  @outMaxCurErrs u64 align(8),
  @outUnKnownPkts u64 align(8),
  @outTransAfterEncap u64 align(8),
  @outToIpsecPkts u64 align(8),
  @outIpsecToGreTransPkts u64 align(8),
  @outAfterEncapPkts u64 align(8),
  @dpSynCpuFailed u64 align(8),
  @hitFibFailed u64 align(8),
  @inTransAfterDecapPktsFailed u64 align(8),
  @reserve <[173] u64> align(8)}>
type $HpfGreRemainTunnel <struct {
  @startTunnelId u32 align(4),
  @remainCount u32 align(4)}>
type $HpfFrag4SpecInfo <struct {
  @hash_tbl_node u32 align(4),
  @frag_tbl_node u32 align(4),
  @hash_age_node u32 align(4),
  @max_cache_total u32 align(4),
  @threshold_total u32 align(4),
  @timer_number u32 align(4),
  @timer_interval u32 align(4),
  @scan_interval u32 align(4)}>
type $HpfFrag4OffSetInfo <struct {
  @first_offset u16 align(2),
  @last_offset u16 align(2)}>
type $tagHpfFrag4OffsetNode <struct {
  @isLast :1 u16 align(2),
  @resv :2 u16 align(2),
  @offset :13 u16 align(2),
  @length u16 align(2),
  @next <* <$tagHpfFrag4OffsetNode>> align(8)}>
type $HpfFrag4TblInfo <struct {
  @vrf_index u16 align(2),
  @protocol u8,
  @src_ip u32 align(4),
  @dst_ip u32 align(4),
  @src_port u16 align(2),
  @dst_port u16 align(2),
  @pkt_id u16 align(2),
  @vlan_id u16 align(2),
  @create_time u64 align(8),
  @ttl u32 align(4),
  @cpe_ip <$HpfIn6Addr>,
  @rwlock <$HpeRwlock> align(4),
  @frag4_pkt <* <$hpe_mbuf> align(64)> align(8),
  @pkt_cnt u16 align(2),
  @cur_offset u16 align(2),
  @sendFragLen u16 align(2),
  @valid :1 u8,
  @fst_rcv :1 u8,
  @reassemble :1 u8,
  @last_rcv :1 u8,
  @frag4_fifo :1 u8,
  @overlap :1 u8,
  @buff_full :1 u8,
  @reserved :1 u8,
  @frag4_offset <[21] <$HpfFrag4OffSetInfo>> align(2),
  @fragOffsetNode <* <$tagHpfFrag4OffsetNode>> align(8),
  @fragOffsetCount u32 align(4)}>
type $HpfFrag4IicMsgInfo <struct {
  @msg_type u32 align(4),
  @slot_num u32 align(4),
  @cpu_num u32 align(4),
  @cur_value u32 align(4),
  @threshold u32 align(4)}>
type $HpfFrag4CacheCount <struct {
  @ipAddr u32 align(4),
  @valid u32 align(4),
  @fragCnt u32 align(4),
  @hitTime u32 align(4),
  @rwlock <$HpeRwlock> align(4)}>
type $HpfFragFloodAtkInfo <struct {
  @ttl <[256] u32> align(4),
  @protocol <[256] u32> align(4),
  @srcIp <[1024] u32> align(4),
  @dstIp <[1024] u32> align(4),
  @pktId <[512] u32> align(4),
  @interface <[512] u32> align(4),
  @recordTime u32 align(4),
  @resetTime u32 align(4),
  @pktCount u64 align(8)}>
type $HpfFrag6TblInfo <struct {
  @src_ip <$HpfIn6Addr>,
  @dst_ip <$HpfIn6Addr>,
  @pkt_id u32 align(4),
  @vrf_index u16 align(2),
  @src_port u16 align(2),
  @dst_port u16 align(2),
  @vlan_id u16 align(2),
  @protocol u8,
  @first_frag_len u16 align(2),
  @rwlock <$HpeRwlock> align(4),
  @frag6_pkt <* <$hpe_mbuf> align(64)> align(8),
  @pkt_cnt u16 align(2),
  @cur_offset u16 align(2),
  @create_time u64 align(8),
  @ttl u32 align(4),
  @valid :1 u8,
  @fst_rcv :1 u8,
  @reassemble :1 u8,
  @last_rcv :1 u8,
  @frag6_fifo :1 u8,
  @reserved :3 u8}>
type $HpfFrag6SpecInfo <struct {
  @hash_tbl_node u32 align(4),
  @frag_tbl_node u32 align(4),
  @hash_age_node u32 align(4),
  @max_cache_total u32 align(4),
  @threshold_total u32 align(4),
  @timer_number u32 align(4),
  @timer_interval u32 align(4),
  @scan_interval u32 align(4)}>
type $HpfFrag6IicMsgInfo <struct {
  @msg_type u32 align(4),
  @slot_num u32 align(4),
  @cpu_num u32 align(4),
  @cur_value u32 align(4),
  @threshold u32 align(4)}>
type $HpfFrag6FloodAtkInfo <struct {
  @ttl <[256] u32> align(4),
  @protocol <[256] u32> align(4),
  @srcIp <[1024] u32> align(4),
  @dstIp <[1024] u32> align(4),
  @pktId <[512] u32> align(4),
  @interface <[512] u32> align(4),
  @recordTime u32 align(4),
  @resetTime u32 align(4),
  @pktCount u64 align(8)}>
type $HpfArpMissCacheSpec <struct {
  @hashNodeMax u32 align(4),
  @tblNodeMax u32 align(4),
  @eachNodeMbufPktCacheMax u32 align(4),
  @totalMbufPktCacheMax u32 align(4),
  @tblNodeTtl u32 align(4),
  @hashAgeNode u32 align(4),
  @threshold u32 align(4),
  @timerNum u32 align(4),
  @timerInterval u32 align(4),
  @scanInterval u32 align(4),
  @queueSize u32 align(4)}>
type $tagHpfArpMissCachePktNode <struct {
  @nextPktNode <* <$tagHpfArpMissCachePktNode>> align(8),
  @mbuf <* <$hpe_mbuf> align(64)> align(8),
  @ttl u32 align(4),
  @createTime u32 align(4)}>
type $HpfArpMissCacheTblNode <struct {
  @vrf u16 align(2),
  @nextHop u32 align(4),
  @isValid u8,
  @rwlock <$HpeRwlock> align(4),
  @cachedPktCnt u32 align(4),
  @cachedPktHead <* <$tagHpfArpMissCachePktNode>> align(8)}>
type $HpfArpMissCacheTblKey <struct {
  @vrf u16 align(2),
  @resv u16 align(2),
  @nextHop u32 align(4)}>
type $HpfNdMissCacheSpec <struct {
  @hashNodeMax u32 align(4),
  @tblNodeMax u32 align(4),
  @eachNodeMbufPktCacheMax u32 align(4),
  @totalMbufPktCacheMax u32 align(4),
  @tblNodeTtl u32 align(4),
  @hashAgeNode u32 align(4),
  @threshold u32 align(4),
  @timerNum u32 align(4),
  @timerInterval u32 align(4),
  @scanInterval u32 align(4),
  @queueSize u32 align(4)}>
type $tagHpfNdMissCachePktNode <struct {
  @nextPktNode <* <$tagHpfNdMissCachePktNode>> align(8),
  @mbuf <* <$hpe_mbuf> align(64)> align(8),
  @ttl u32 align(4),
  @createTime u32 align(4)}>
type $HpfNdMissCacheTblNode <struct {
  @vrf u16 align(2),
  @nextHop <[4] u32> align(4),
  @isValid u8,
  @rwlock <$HpeRwlock> align(4),
  @cachedPktCnt u32 align(4),
  @cachedPktHead <* <$tagHpfNdMissCachePktNode>> align(8)}>
type $HpfNdMissCacheTblKey <struct {
  @vrf u16 align(2),
  @resv u16 align(2),
  @nextHop <[4] u32> align(4)}>
type $HpfTunnelIfStat <struct {
  @inPackets u64 align(8),
  @inBytes u64 align(8),
  @inErrors u64 align(8),
  @inUnicasts u64 align(8),
  @inMulticasts u64 align(8),
  @outPackets u64 align(8),
  @outErrors u64 align(8),
  @outBytes u64 align(8),
  @outUnicasts u64 align(8),
  @outMulticasts u64 align(8)}>
type $HpfTunnelFuncCb <struct {
  @tunnel4over6Output <* <func (<* <$hpe_mbuf> align(64)>,u32,<* <$fwd_hook_param_s>>) u64>> align(8),
  @tunnelIs4o6 <* <func (u32) u8>> align(8),
  @tunnel6over4Output <* <func (<* <$hpe_mbuf> align(64)>,u32) u64>> align(8),
  @tunnelIs6o4 <* <func (u32) u8>> align(8),
  @tunnel6over4Input <* <func (<* <$hpe_mbuf> align(64)>) u64>> align(8),
  @tunnel4over6Input <* <func (<* <$hpe_mbuf> align(64)>,<* <$fwd_hook_param_s>>) u64>> align(8)}>
type $HpfTraceIpv4Addr <struct {
  @srcIp u32 align(4),
  @dstIp u32 align(4)}>
type $HpfTraceIpv6Addr <struct {
  @srcIp <$HpfIn6Addr>,
  @dstIp <$HpfIn6Addr>}>
type $HpfTracePktCondition <struct {
  @enable u8,
  @invert u8,
  @fromCp u8,
  @fragType u8,
  @srcAddr <$HpfIn46Addr>,
  @dstAddr <$HpfIn46Addr>,
  @srcMac <[6] u8>,
  @dstMac <[6] u8>,
  @protocol u8,
  @resv <[3] u8>,
  @number u32 align(4),
  @ethType u16 align(2),
  @vlanId u16 align(2),
  @srcPort u16 align(2),
  @dstPort u16 align(2),
  @srcIntf u32 align(4),
  @maxLen u16 align(2),
  @minLen u16 align(2),
  @timeout u32 align(4),
  @timeTag u64 align(8)}>
type $tag_dpdbg_trace <struct {
  @isEnable u32 align(4),
  @number u32 align(4),
  @output u32 align(4),
  @flow u8,
  @allSystem u8,
  @aclType u32 align(4),
  @aclNum u16 align(2),
  @aclGroupId u32 align(4),
  @ucDisc <[1080] u8>,
  @vsysIndex u16 align(2)}>
type $HpfTraceDbgCfg <struct {
  @rwlock <$HpeRwlock> align(4),
  @webCfg <$HpfFwdTraceDbgWebMsg> align(4),
  @flow <[10] <$HpfPktTraceFlow>> align(8)}>
type $HpfPktStatDesc <struct {
  @strEnglish <* i8> align(8),
  @strChinese <* i8> align(8),
  @solution <* i8> align(8)}>
type $HpfModuleCounter <struct {
  @inCounter u64 align(8),
  @outCounter u64 align(8),
  @maxModuleEvent u32 align(4),
  @maxModuleError u32 align(4)}>
type $HpfPortCounter <struct {
  @fwdIf u32 align(4),
  @inCounter u64 align(8),
  @outCounter u64 align(8)}>
type $HpfCatchFilterCounter <struct {
  @filterModule <[3] <$HpfModuleCounter>> align(8),
  @filterPort <[3] <$HpfPortCounter>> align(8),
  @eventCounter <[3] u64> align(8),
  @errorCounter <[3] u64> align(8)}>
type $HpfPktInfo <struct {
  @srcIp <$unnamed.6140> align(4),
  @dstIp <$unnamed.6141> align(4),
  @srcPort u16 align(2),
  @dstPort u16 align(2),
  @protocol u8,
  @isV6 u8,
  @ethtype u16 align(2),
  @port u32 align(4),
  @pktLen u32 align(4)}>
type $unnamed.6140 <union {
  @ip4Addr u32 align(4),
  @ip6Addr <$HpfIn6Addr>}>
type $unnamed.6141 <union {
  @ip4Addr u32 align(4),
  @ip6Addr <$HpfIn6Addr>}>
type $HpfPktCollect <struct {
  @type u32 align(4),
  @state u32 align(4),
  @startTimeSinece1970 u32 align(4),
  @startTime u32 align(4),
  @collectingTime u32 align(4),
  @perMsBit u32 align(4),
  @lock <$HpeRwlock> align(4),
  @toltalPkts <[1] u64> align(8),
  @toltalBytes <[1] u64> align(8),
  @collectPkts <$HpeAtom32> align(4),
  @collectBytes <[1] u32> align(4),
  @pkt <[8192] <$HpfPktInfo>> align(4)}>
type $HpfPktCollectCtl <struct {
  @collectState u32 align(4),
  @collectType u32 align(4),
  @collectLock <$HpeRwlock> align(4),
  @hookIsRegsiter u32 align(4),
  @hookNotifyRegsiter u32 align(4),
  @hookEnhance u32 align(4),
  @burstTimespan u32 align(4)}>
type $HpfPktAnalysisNodeL <struct {
  @pre <* <$HpfPktAnalysisNodeL>> align(8),
  @next <* <$HpfPktAnalysisNodeL>> align(8),
  @queue <* void> align(8),
  @isIpv6 u8,
  @pktType u8,
  @directionType u8,
  @rsv <[1] u8>,
  @ipAddr <$unnamed.6142> align(4),
  @pktNum u32 align(4),
  @byteLen u32 align(4),
  @ethtype <[8] u16> align(2),
  @ethtypeCnt <[8] u32> align(4),
  @protocol <[8] u8>,
  @protocolCnt <[8] u32> align(4),
  @srcPort <[8] u16> align(2),
  @srcPortCnt <[8] u32> align(4),
  @dstPort <[8] u16> align(2),
  @dstPortCnt <[8] u32> align(4),
  @port <[8] u32> align(4),
  @portCnt <[8] u32> align(4),
  @peerIp <[8] <$unnamed.6143>> align(4),
  @peerIpCnt <[8] u32> align(4)}>
type $unnamed.6142 <union {
  @ip4Addr u32 align(4),
  @ip6Addr <$HpfIn6Addr>}>
type $unnamed.6143 <union {
  @ip4Addr u32 align(4),
  @ip6Addr <$HpfIn6Addr>}>
type $HpfPktAnalysisOrderq <struct {
  @head <* <$HpfPktAnalysisNodeL>> align(8),
  @tail <* <$HpfPktAnalysisNodeL>> align(8),
  @lock <$HpeRwlock> align(4),
  @count u32 align(4),
  @state u32 align(4)}>
type $HpfPktAnalysisResult <struct {
  @type u32 align(4),
  @state u32 align(4),
  @timeSinece1970 u32 align(4),
  @sampleSpan u32 align(4),
  @cpuUsage u32 align(4),
  @pps u32 align(4),
  @bps u64 align(8),
  @v4create u32 align(4),
  @v4createSucess u32 align(4),
  @v4session u32 align(4),
  @v6create u32 align(4),
  @v6createSucess u32 align(4),
  @v6session u32 align(4),
  @policyAccStatus u32 align(4),
  @lastSessionRefreshTime u32 align(4),
  @info <[3] <$unnamed.6144>> align(8),
  @orderq <[3][2] <$HpfPktAnalysisOrderq>> align(8)}>
type $unnamed.6144 <struct {
  @samplePkts u32 align(4),
  @sampleBytes u32 align(4),
  @totalPkts u32 align(4),
  @totalBytes u64 align(8)}>
type $HpfPktAnalysisRecord <struct {
  @cpuhighIndex u32 align(4),
  @burstIndex u32 align(4),
  @manualIndex u32 align(4),
  @cpuhigh <[3] <$HpfPktAnalysisResult>> align(8),
  @burst <[3] <$HpfPktAnalysisResult>> align(8),
  @manual <[3] <$HpfPktAnalysisResult>> align(8)}>
type $HpfPktAnalysisCtl <struct {
  @hashTlb u64 align(8)}>
type $TagPktBtStatistics <struct {
  @timerTriggerCnt u32 align(4),
  @cpuhighTriggerCnt u32 align(4),
  @microBurstAddHook u32 align(4),
  @microBurstTriggerCnt u32 align(4),
  @manualTriggerCnt u32 align(4),
  @recordCnt u32 align(4),
  @recordCpuhigh u32 align(4),
  @recordMicroBurst u32 align(4),
  @recordManual u32 align(4),
  @cpuhighStartFail u32 align(4),
  @microBurstStartFail u32 align(4),
  @manualStartFail u32 align(4),
  @patchConflict u32 align(4)}>
type $HpfBtpeerIp <union {
  @ip4Addr u32 align(4),
  @ip6Addr <$HpfIn6Addr>}>
type $HpfPktBtPara <struct {
  @pkts u64 align(8),
  @bytes u64 align(8),
  @tPkts u64 align(8),
  @tBytes u64 align(8),
  @span u64 align(8)}>
type $HpfPktBtSetType <struct {
  @verbose u32 align(4),
  @ipType u32 align(4),
  @len u16 align(2),
  @pktType u32 align(4),
  @pktDirection u32 align(4)}>
type $HpfFlowMngProHead <struct {
  @msgId u16 align(2),
  @data <[5] u32> align(4),
  @flag u8,
  @tblType u8}>
type $HpfFlowCfgReturnMsgHead <struct {
  @ret_code u32 align(4)}>
type $HpfFlowCfgSpecInfo <struct {
  @maxNode i32 align(4),
  @bucketNumber i32 align(4),
  @reservedNumber i32 align(4),
  @scanInterval i32 align(4),
  @scanUnitInterval i32 align(4),
  @scanTimerNumber i32 align(4),
  @topnTableNumber i32 align(4),
  @topnTimerNumber i32 align(4),
  @cpuCoreNumber i32 align(4),
  @maxNodeSingle u32 align(4),
  @onlineIpTableNumber i32 align(4),
  @onlineAppTableNumber i32 align(4),
  @onlineIpHashNumber i32 align(4),
  @onlineAppHashNumber i32 align(4)}>
type $HpfFlowCfgGetInfoIndex <struct {
  @index u32 align(4),
  @curNum u32 align(4)}>
type $HpfFlowCfgSearchParam <struct {
  @key <$HpfFlowKey> align(8),
  @vlan u16 align(2),
  @cpeFlag u8,
  @masterFlag u8,
  @reserved <[4] u8>,
  @cpe <[4] u32> align(4)}>
type $HpfFlowStatCpuAndMemRecord <struct {
  @cpuUsage <[1] u32> align(4),
  @heapMemTotal u64 align(8),
  @heapMemUsed u64 align(8),
  @sliceMemTotal u64 align(8),
  @sliceMemUsed u64 align(8),
  @chunkMemTotal u64 align(8),
  @chunkMemUsed u64 align(8)}>
type $HpfFlowShowCpuStatRetMsg <struct {
  @slotId u8,
  @cpuId u8,
  @sessStatInCpu <$HpfFlowShowStatRetMsg> align(8)}>
type $HpfFlowShowStatRetMsgCondition <struct {
  @flowNumCondition u32 align(4)}>
type $HpfFlowTotalSessInfo <struct {
  @totalNum u32 align(4)}>
type $HpfFlowSessInstId <struct {
  @instId i16 align(2),
  @sessNum u32 align(4),
  @sessRate u32 align(4)}>
type $HpfFlowSessNode <struct {
  @singleNodeNum u32 align(4),
  @coupleNodeNum u32 align(4)}>
type $HpfFlowSessCfg <struct {
  @maxNodeSingle u32 align(4)}>
type $HpfFlowFtpClientData <struct {
  @szHostName <[32] i8>,
  @szUserName <[65] i8>,
  @szPassword <[65] i8>,
  @szFileName <[65] i8>}>
type $HpfFlowStatTotalCurAndMax <struct {
  @curFlowNum u64 align(8),
  @curFlowRate u64 align(8),
  @maxFlowNum u64 align(8),
  @maxFlowNumTime u64 align(8),
  @maxFlowRate u64 align(8),
  @maxFlowRateTime u64 align(8),
  @maxFlowCpuMemRec <$HpfFlowStatCpuAndMemRecord> align(8)}>
type $HpfFlowStatCurAndMax <struct {
  @flowCreateNum <[8] u64> align(8),
  @flowAgeNum <[8] u64> align(8),
  @lastSaveFlowCreateNum u64 align(8),
  @lastRateCalTime u64 align(8),
  @curFlowRate u64 align(8),
  @maxFlowNum u64 align(8),
  @maxFlowNumTime u64 align(8),
  @maxFlowRate u64 align(8),
  @maxFlowRateTime u64 align(8),
  @maxFlowCpuMemRec <$HpfFlowStatCpuAndMemRecord> align(8)}>
type $HpfFlowMsg <struct {
  @type u16 align(2),
  @value u16 align(2)}>
type $HpfHrpFuncCb <struct {
  @hrpIsEnabled <* <func () u1>> align(8),
  @migSetMirSessState <* <func (u8) void>> align(8),
  @hrpIsDataPermit <* <func () u32>> align(8),
  @hrpOutPacketToCpu <* <func (<* <$hpe_mbuf> align(64)>,i32,u8) i32>> align(8),
  @hrpOutPacketMbuf <* <func (<* <$hpe_mbuf> align(64)>,i32) i32>> align(8),
  @hrpMirSesEnabled <* <func () u1>> align(8),
  @hrpAuIsClosed <* <func () u32>> align(8),
  @hrpIssuNegoVerGet <* <func (i32,<* u8>) u32>> align(8),
  @hrpSecBkupState <* <func (u8) u32>> align(8),
  @hrpGetRunState <* <func () u32>> align(8),
  @hrpIntfGetconfigstate <* <func () u32>> align(8),
  @hrpGetRunStateInit <* <func () i32>> align(8),
  @hrpGetRunStateStandby <* <func () i32>> align(8),
  @hrpGetRunStateSlave <* <func () i32>> align(8),
  @hrpGetRunStateLoadbalance <* <func () i32>> align(8),
  @hrpGetSessIp6 <* <func () i32>> align(8),
  @hrpGetSessIp6Portion <* <func () i32>> align(8),
  @hrpGetSessIp6Require <* <func () i32>> align(8),
  @hrpGetSessMigrateIp6 <* <func () i32>> align(8),
  @hrpGetSessMigrateIp6Portion <* <func () i32>> align(8),
  @hrpGetSessMigrateIp6Require <* <func () i32>> align(8),
  @hrpGetPacketSes <* <func () i32>> align(8),
  @hrpGetPacketFwd <* <func () i32>> align(8),
  @hrpGetPacketDpIpsec <* <func () i32>> align(8),
  @hrpGetPacketSermapIp6 <* <func () i32>> align(8),
  @hrpGetIssuIpsecInt <* <func () i32>> align(8),
  @hrpGetPacketSvr <* <func () i32>> align(8),
  @hrpGetPacketDomainset <* <func () i32>> align(8),
  @hrpGetHrpOk <* <func () i32>> align(8),
  @hrpGetPacketSla <* <func () i32>> align(8),
  @hrpIsCfgEnabled <* <func () u1>> align(8),
  @hrpDpIfVrrpIsMaster <* <func (<* void>,u8,u8) u32>> align(8),
  @hrpDpIsHrpInterface <* <func (u32) u1>> align(8),
  @hrpDataModRegister <* <func (i32,<* i8>,<* <func (<* void>,u64) i32>>,<* <func () void>>) i32>> align(8),
  @hrpGetVpnMapping <* <func () i32>> align(8)}>
type $HpfHrpPacketS <struct {
  @ucVersion u8,
  @ucType u8,
  @usLength u16 align(2),
  @ulMagicCode u32 align(4),
  @ulCheckSum u32 align(4),
  @ucCpuID u8,
  @ucSubType u8,
  @ucPeerVersion u8,
  @ucModuleType u8}>
type $unnamed.6145 <struct {
  @ipsecHashIndex :12 u32 align(4),
  @ipsecSa :20 u32 align(4),
  @ipsecCpuId :8 u32 align(4),
  @ipsecSaVer :24 u32 align(4)}>
type $HpfSpeedLimit <struct {
  @totalPass u64 align(8),
  @totalBlock u64 align(8),
  @lastTotalPass u64 align(8),
  @lastTotalBlock u64 align(8),
  @thisBegin u64 align(8),
  @limitState u32 align(4),
  @baseLimit u32 align(4),
  @baseCycle u32 align(4),
  @lastCpu u32 align(4),
  @lastSecPassRate u32 align(4),
  @lastSecBlockRate u32 align(4)}>
type $unnamed.6146 <struct {
  @tqe_next <* <$HpeModTag>> align(8),
  @tqe_prev <* <* <$HpeModTag>>> align(8)}>
type $tagImSystm <struct {
  @year u16 align(2),
  @month u8,
  @date u8,
  @hour u8,
  @minute u8,
  @second u8,
  @week u8,
  @millSec u32 align(4),
  @bootHighMillSec u32 align(4),
  @bootLowMillSec u32 align(4)}>
type $HpfZoneFuncCb <struct {
  @interZoneCreate <* <func (u16,u16,u16) u32>> align(8),
  @findZoneFlagAndTcpMss <* <func (u16,u16,u16,<* <* u32>>,<* <* u16>>) u32>> align(8)}>
type $HpfFlowTtlInfo <struct {
  @appTtl <[17] u16> align(2)}>
type $HpfFwdAgingTime <struct {
  @vsysId u32 align(4),
  @blockFlag u32 align(4),
  @defaultFlag u16 align(2),
  @ttlTime <[17] u16> align(2),
  @appProNum u16 align(2)}>
type $HpfFwdAgingTimeAppinfo <struct {
  @appId u16 align(2),
  @ttl u16 align(2)}>
type $HpfFwdAgingTimeUserSrv <struct {
  @serviceSetIndex u16 align(2),
  @vsysId u16 align(2),
  @ttl u16 align(2)}>
type $HpfAppProtoInfo <struct {
  @tcpPort u16 align(2),
  @udpPort u16 align(2),
  @ttlDefault u16 align(2),
  @saId u16 align(2),
  @saSubClass u8,
  @detectIndex u8,
  @reserved <[2] u8>}>
type $HpfAppProtoDes <struct {
  @name <* i8> align(8)}>
type $HpfFwdRpcMsgHead <struct {
  @msgId u16 align(2),
  @data u16 align(2),
  @value u32 align(4)}>
type $HpfFwdTcpMssZoneMsg <struct {
  @usSrcVrfIndex u16 align(2),
  @usDstVrfIndex u16 align(2),
  @ucSrcZone u16 align(2),
  @ucDstZone u16 align(2)}>
type $HpfFwdTcpMssMsg <struct {
  @usTcpMssSize u16 align(2),
  @usUndoFlag u16 align(2),
  @vsysId u16 align(2),
  @zoneCfgFlag u16 align(2)}>
type $HpfFwdTcpSeqMsg <struct {
  @checkFlag u16 align(2),
  @res u16 align(2)}>
type $HpfIcmpErrCntMsg <struct {
  @pktin u64 align(8),
  @pktout u64 align(8)}>
type $HpfPortMatDstIp <struct {
  @dstIp u32 align(4),
  @ipAddr u32 align(4),
  @result u32 align(4),
  @count u32 align(4)}>
type $HpfNatArpTbl <struct {
  @inSectQueryResult <$HpfArpNatPoolPara> align(8),
  @exSectQueryResult <$HpfArpNatPoolPara> align(8)}>
LOC 55 120 6
func &HpfRecvDirect public used (var %mbuf <* <* <$hpe_mbuf> align(64)>> used, var %num i32 used) void
func &MbufAddrCheck public used (var %mbuf <* <$hpe_mbuf> align(64)>) void
func &HpeMbufSafeCheck public static used inline (var %mbuf <* <$hpe_mbuf> align(64)> used) void
LOC 60 133 20
func &MBUF_CLEAR_CONTEXT public static used inline (var %mbuf <* <$hpe_mbuf> align(64)> used) void
func &HpfDecodeLswTag public used extern (var %mbuf <* <$hpe_mbuf> align(64)>) void
LOC 60 133 20
func &HpfPrintMbufWithInfo public static used inline (var %m <* <$hpe_mbuf> align(64)> used) void
func &HpfPrintContent public static used inline (var %m <* <$hpe_mbuf> align(64)> used) void
func &HpeDiagLogFmt public used varargs (var %level u32, var %mod u32, var %format <* i8>, ...) i32
func &HpfPktProcess public used extern (var %mbuf <* void>) u64

func &MBUF_CLEAR_CONTEXT public static used inline (var %mbuf <* <$hpe_mbuf> align(64)> used) void {
  funcid 3310
  funcinfo {
    @INFO_fullname "MBUF_CLEAR_CONTEXT"}

LOC 60 136 14
  var %i_136_14 u32 used
LOC 60 137 9
  var %size_137_9 i32 used
LOC 60 138 15
  var %a_138_15 v4i32 used
LOC 60 139 16
  var %dst_ptr_139_16 <* v4i32> used

LOC 60 137 9
  dassign %size_137_9 0 (constval i32 896)
LOC 60 138 15
  dassign %a_138_15 0 (intrinsicop v4i32 vector_from_scalar_v4i32 (constval i32 0))
LOC 60 139 16
  dassign %dst_ptr_139_16 0 (add ptr (
      add ptr (
        dread ptr %mbuf,
        cvt ptr u64 (constval u64 192)),
      cvt ptr i32 (cvt i32 u32 (iread u32 <* <$hpe_mbuf> align(64)> 6 (dread ptr %mbuf)))))
LOC 60 140 12
  dassign %i_136_14 0 (constval u32 0)
LOC 60 140 5
  while (lt u1 u32 (
    dread u32 %i_136_14,
    lshr u32 (
      cvt u32 i32 (dread i32 %size_137_9),
      constval u64 4))) {
LOC 60 141 9
    iassign <* v4i32> 0 (
      add ptr (
        dread ptr %dst_ptr_139_16,
        mul ptr (
          cvt ptr u32 (dread u32 %i_136_14),
          constval ptr 16)),
      dread v4i32 %a_138_15)
LOC 60 140 60
    dassign %i_136_14 0 (add u32 (dread u32 %i_136_14, constval u32 1))
  }
LOC 60 143
  return ()
}

LOC 43 193 20
func &HpeMbufSafeCheck public static used inline (var %mbuf <* <$hpe_mbuf> align(64)> used) void {
  funcid 1085
  funcinfo {
    @INFO_fullname "HpeMbufSafeCheck"}


LOC 43 195 5
  if (ne u1 i64 (
    intrinsicop i64 C___builtin_expect (
      cvt i64 i32 (eq u1 i32 (
        eq u1 i32 (
          ne u1 i32 (
            cvt i32 u32 (iread u32 <* <$HpeMbufGlobalCtl>> 3 (dread ptr $g_mbufGlobalCtl)),
            constval i32 0),
          constval i32 0),
        constval i32 0)),
      constval i64 0),
    constval i64 0)) {
LOC 43 196 9
    call &MbufAddrCheck (dread ptr %mbuf)
  }
LOC 43 198
  return ()
}

LOC 55 120 6
func &HpfRecvDirect public used (var %mbuf <* <* <$hpe_mbuf> align(64)>> used, var %num i32 used) void {
  funcid 4697
  funcinfo {
    @INFO_fullname "HpfRecvDirect"}

LOC 55 122 13
  var %i_122_13 i32 used
LOC 55 123 14
  var %ifIndex_123_14 u32 used
LOC 55 129 20
  var %ethHdr_129_20 <* <$HpfEthHdr>> used
LOC 55 134 127
  var %level_134_127 u32 used
LOC 55 135 64
  var %temp__135_64 <* <$hpe_mbuf> align(64)> used
LOC 55 135 169
  var %traceIdx__135_169 u16 used
LOC 55 135 214
  var %funcId__135_214 u16 used
  var %levVar_5741 i32
  var %retVar_5743 i32
  var %retVar_5759 u8
  var %retVar_5768 u64

LOC 55 125 12
  dassign %i_122_13 0 (constval i32 0)
LOC 55 125 5
  while (lt u1 i32 (dread i32 %i_122_13, dread i32 %num)) {
LOC 55 126 9
    call &MBUF_CLEAR_CONTEXT (iread ptr <* <* <$hpe_mbuf> align(64)>> 0 (array 1 ptr <* <[1] <* <$hpe_mbuf> align(64)>>> (dread ptr %mbuf, dread i32 %i_122_13)))
LOC 55 127 17
    dassign %ifIndex_123_14 0 (cvt u32 i32 (bior i32 (
        constval i32 0,
        bior i32 (
          bior i32 (
            constval i32 0,
            band i32 (
              shl i32 (
                cvt i32 u32 (iread u32 <* <$hpe_mbuf> align(64)> 4 (iread ptr <* <* <$hpe_mbuf> align(64)>> 0 (array 1 ptr <* <[1] <* <$hpe_mbuf> align(64)>>> (dread ptr %mbuf, dread i32 %i_122_13)))),
                constval i32 7),
              constval i32 0x7f80)),
          constval i32 0))))
LOC 55 129 20
    dassign %ethHdr_129_20 0 (add ptr (
        cvt ptr u64 (cvt u64 ptr (iread ptr <* <$hpe_mbuf> align(64)> 1 (iread ptr <* <* <$hpe_mbuf> align(64)>> 0 (array 1 ptr <* <[1] <* <$hpe_mbuf> align(64)>>> (dread ptr %mbuf, dread i32 %i_122_13))))),
        cvt ptr i32 (cvt i32 u32 (iread u32 <* <$hpe_mbuf> align(64)> 3 (iread ptr <* <* <$hpe_mbuf> align(64)>> 0 (array 1 ptr <* <[1] <* <$hpe_mbuf> align(64)>>> (dread ptr %mbuf, dread i32 %i_122_13)))))))
LOC 55 130 9
    if (eq u1 i32 (
      bior i32 (
        ashr i32 (
          band i32 (
            cvt i32 u32 (iread u32 <* <$HpfEthHdr>> 3 (dread ptr %ethHdr_129_20)),
            constval i32 0xff00),
          constval i32 8),
        shl i32 (
          band i32 (
            cvt i32 u32 (iread u32 <* <$HpfEthHdr>> 3 (dread ptr %ethHdr_129_20)),
            constval i32 255),
          constval i32 8)),
      constval i32 0x8874)) {
LOC 55 131 13
      call &HpfDecodeLswTag (iread ptr <* <* <$hpe_mbuf> align(64)>> 0 (array 1 ptr <* <[1] <* <$hpe_mbuf> align(64)>>> (dread ptr %mbuf, dread i32 %i_122_13)))
LOC 55 132 21
      dassign %ifIndex_123_14 0 (bior u32 (
          constval u32 0,
          bior u32 (
            bior u32 (
              constval u32 0,
              band u32 (
                shl u32 (
                  iread u32 <* <$HpfMbufContext>> 638 (add ptr (
                    add ptr (
                      iread ptr <* <* <$hpe_mbuf> align(64)>> 0 (array 1 ptr <* <[1] <* <$hpe_mbuf> align(64)>>> (dread ptr %mbuf, dread i32 %i_122_13)),
                      cvt ptr u64 (constval u64 192)),
                    cvt ptr i32 (cvt i32 u32 (iread u32 <* <$hpe_mbuf> align(64)> 6 (iread ptr <* <* <$hpe_mbuf> align(64)>> 0 (array 1 ptr <* <[1] <* <$hpe_mbuf> align(64)>>> (dread ptr %mbuf, dread i32 %i_122_13))))))),
                  constval i32 7),
                constval u32 0x7f80)),
            constval u32 0)))
    }
LOC 55 134 11
    if (ne u1 i64 (
      intrinsicop i64 C___builtin_expect (
        cvt i64 i32 (eq u1 i32 (
          eq u1 i32 (
            ge u1 i32 (
              sext i32 8 (dread i32 $g_hpfHlogLevel),
              constval i32 6),
            constval i32 0),
          constval i32 0)),
        constval i64 0),
      constval i64 0)) {
LOC 55 134 80
      dowhile {
LOC 55 134 85
        if (le u1 i32 (
          constval i32 4,
          sext i32 8 (dread i32 $g_hpfHlogLevel))) {
LOC 55 134 135
          if (ne u1 i32 (
            cvt i32 u32 (dread u32 $g_hpfHlogForcePrint),
            constval i32 0)) {
LOC 55 134 164
            dassign %levVar_5741 0 (constval i32 0)
          }
          else {
LOC 55 134 171
            dassign %levVar_5741 0 (constval i32 4)
          }
LOC 55 134 127
          dassign %level_134_127 0 (cvt u32 i32 (dread i32 %levVar_5741))
LOC 55 134 178
          callassigned &HpeDiagLogFmt (
            bior u32 (constval u32 256, dread u32 %level_134_127),
            dread u32 $g_hpfHlogModId,
            conststr ptr "[pkt_trace:%s] : hpf recv pkt from phy, port:%u",
            conststr ptr "HpfRecvDirect",
            cvt i32 u32 (iread u32 <* <$hpe_mbuf> align(64)> 4 (iread ptr <* <* <$hpe_mbuf> align(64)>> 0 (array 1 ptr <* <[1] <* <$hpe_mbuf> align(64)>>> (dread ptr %mbuf, dread i32 %i_122_13))))) { dassign %retVar_5743 0 }
        }
      } (constval u1 0)
LOC 55 134 347
      if (ne u1 ptr (
        iread ptr <* <* <$hpe_mbuf> align(64)>> 0 (array 1 ptr <* <[1] <* <$hpe_mbuf> align(64)>>> (dread ptr %mbuf, dread i32 %i_122_13)),
        cvt ptr i32 (constval i32 0))) {
LOC 55 134 376
        call &HpfPrintMbufWithInfo (iread ptr <* <* <$hpe_mbuf> align(64)>> 0 (array 1 ptr <* <[1] <* <$hpe_mbuf> align(64)>>> (dread ptr %mbuf, dread i32 %i_122_13)))
LOC 55 134 407
        call &HpfPrintContent (iread ptr <* <* <$hpe_mbuf> align(64)>> 0 (array 1 ptr <* <[1] <* <$hpe_mbuf> align(64)>>> (dread ptr %mbuf, dread i32 %i_122_13)))
      }
    }
LOC 55 135 9
    dowhile {
LOC 55 135 14
      if (eq u1 i32 (
        cvt i32 u32 (iread u32 <* <$HpeMbufGlobalCtl>> 1 (dread ptr $g_mbufGlobalCtl)),
        constval i32 1)) {
LOC 55 135 84
        dassign %temp__135_64 0 (iread ptr <* <* <$hpe_mbuf> align(64)>> 0 (array 1 ptr <* <[1] <* <$hpe_mbuf> align(64)>>> (dread ptr %mbuf, dread i32 %i_122_13)))
LOC 55 135 71
        while (ne u1 ptr (
          dread ptr %temp__135_64,
          cvt ptr i32 (constval i32 0))) {
LOC 55 135 180
          call &HpeMbufSafeCheck (dread ptr %temp__135_64)
LOC 55 135 214
          dassign %funcId__135_214 0 (constval u16 640)
LOC 55 135 270
          if (ne u1 i64 (
            intrinsicop i64 C___builtin_expect (
              cvt i64 i32 (eq u1 i32 (
                eq u1 i32 (
                  eq u1 i32 (
                    cvt i32 u32 (iread u32 <* <$HpeMbufGlobalCtl>> 4 (dread ptr $g_mbufGlobalCtl)),
                    constval i32 0),
                  constval i32 0),
                constval i32 0)),
              constval i64 1),
            constval i64 0)) {
LOC 55 135 354
            dassign %traceIdx__135_169 0 (zext u32 16 (band i32 (
                cvt i32 u32 (iread u32 <* <$HpeMbufPrivInfo>> 17 (add ptr (
                  iread ptr <* <$hpe_mbuf> align(64)> 18 (cvt ptr i64 (cvt i64 ptr (dread ptr %temp__135_64))),
                  cvt ptr u64 (constval u64 880)))),
                constval i32 31)))
LOC 55 135 653
            iassign <* u16> 0 (
              array 1 ptr <* <[32] u16>> (
                iaddrof ptr <* <$HpeMbufPrivInfo>> 22 (add ptr (
                  iread ptr <* <$hpe_mbuf> align(64)> 18 (cvt ptr i64 (cvt i64 ptr (dread ptr %temp__135_64))),
                  cvt ptr u64 (constval u64 880))),
                dread u32 %traceIdx__135_169), 
              dread u32 %funcId__135_214)
LOC 55 135 677
            dassign %traceIdx__135_169 0 (zext u32 16 (band i32 (
                add i32 (
                  cvt i32 u32 (dread u32 %traceIdx__135_169),
                  constval i32 1),
                constval i32 31)))
LOC 55 135 843
            iassign <* <$HpeMbufPrivInfo>> 17 (
              add ptr (
                iread ptr <* <$hpe_mbuf> align(64)> 18 (cvt ptr i64 (cvt i64 ptr (dread ptr %temp__135_64))),
                cvt ptr u64 (constval u64 880)), 
              zext u32 8 (dread u32 %traceIdx__135_169))
          }
          else {
LOC 55 135 880
            intrinsiccallwithtypeassigned u8 C___sync_add_and_fetch_1 (
              iaddrof ptr <* <$HpeMbufPrivInfo>> 17 (add ptr (
                iread ptr <* <$hpe_mbuf> align(64)> 18 (cvt ptr i64 (cvt i64 ptr (dread ptr %temp__135_64))),
                cvt ptr u64 (constval u64 880))),
              constval u8 1) { dassign %retVar_5759 0 }
LOC 55 135 878
            dassign %traceIdx__135_169 0 (zext u32 16 (dread u32 %retVar_5759))
LOC 55 135 1211
            iassign <* u16> 0 (
              array 1 ptr <* <[32] u16>> (
                iaddrof ptr <* <$HpeMbufPrivInfo>> 22 (add ptr (
                  iread ptr <* <$hpe_mbuf> align(64)> 18 (cvt ptr i64 (cvt i64 ptr (dread ptr %temp__135_64))),
                  cvt ptr u64 (constval u64 880))),
                band i32 (
                  sub i32 (
                    cvt i32 u32 (dread u32 %traceIdx__135_169),
                    constval i32 1),
                  constval i32 31)), 
              dread u32 %funcId__135_214)
          }
LOC 55 135 128
          dassign %temp__135_64 0 (iread ptr <* <$hpe_mbuf> align(64)> 8 (dread ptr %temp__135_64))
        }
      }
    } (constval u1 0)
LOC 55 136 119
    iassign <* <$HpfMbufContext>> 157 (
      add ptr (
        add ptr (
          iread ptr <* <* <$hpe_mbuf> align(64)>> 0 (array 1 ptr <* <[1] <* <$hpe_mbuf> align(64)>>> (dread ptr %mbuf, dread i32 %i_122_13)),
          cvt ptr u64 (constval u64 192)),
        cvt ptr i32 (cvt i32 u32 (iread u32 <* <$hpe_mbuf> align(64)> 6 (iread ptr <* <* <$hpe_mbuf> align(64)>> 0 (array 1 ptr <* <[1] <* <$hpe_mbuf> align(64)>>> (dread ptr %mbuf, dread i32 %i_122_13)))))), 
      dread u32 %ifIndex_123_14)
LOC 55 137 123
    iassign <* <$HpfMbufContext>> 156 (
      add ptr (
        add ptr (
          iread ptr <* <* <$hpe_mbuf> align(64)>> 0 (array 1 ptr <* <[1] <* <$hpe_mbuf> align(64)>>> (dread ptr %mbuf, dread i32 %i_122_13)),
          cvt ptr u64 (constval u64 192)),
        cvt ptr i32 (cvt i32 u32 (iread u32 <* <$hpe_mbuf> align(64)> 6 (iread ptr <* <* <$hpe_mbuf> align(64)>> 0 (array 1 ptr <* <[1] <* <$hpe_mbuf> align(64)>>> (dread ptr %mbuf, dread i32 %i_122_13)))))), 
      dread u32 %ifIndex_123_14)
LOC 55 139 15
    # CHECK-NOT: lsl
    # CHECK:str {{.*}} #16
    callassigned &HpfPktProcess (iread ptr <* <* <$hpe_mbuf> align(64)>> 0 (array 1 ptr <* <[1] <* <$hpe_mbuf> align(64)>>> (dread ptr %mbuf, dread i32 %i_122_13))) { dassign %retVar_5768 0 }
LOC 55 139 9
    eval (dread u64 %retVar_5768)
LOC 55 125 27
    dassign %i_122_13 0 (add i32 (dread i32 %i_122_13, constval i32 1))
  }
LOC 55 141
  return ()
}
