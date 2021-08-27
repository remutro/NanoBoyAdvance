/*
 * Copyright (C) 2021 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#pragma once

#include <bitset>
#include <nba/integer.hpp>
#include <emulator/core/arm/memory.hpp>
#include <emulator/core/hw/interrupt.hpp>
#include <emulator/core/scheduler.hpp>

namespace nba::core {

struct DMA {
  using Access = arm::MemoryBase::Access;

  DMA(arm::MemoryBase& memory, IRQ& irq, Scheduler& scheduler)
      : memory(memory)
      , irq(irq)
      , scheduler(scheduler) {
    Reset();
  }

  enum class Occasion {
    HBlank,
    VBlank,
    Video,
    FIFO0,
    FIFO1
  };

  void Reset();
  void Request(Occasion occasion);
  void StopVideoXferDMA();
  void Run();
  auto Read (int chan_id, int offset) -> u8;
  void Write(int chan_id, int offset, u8 value);
  bool IsRunning() { return runnable_set.any(); }
  auto GetOpenBusValue() -> u32 { return latch; }

private:
  enum Registers {
    REG_DMAXSAD = 0,
    REG_DMAXDAD = 4,
    REG_DMAXCNT_L = 8,
    REG_DMAXCNT_H = 10
  };

  struct Channel {
    int id;
    bool enable = false;
    bool repeat = false;
    bool interrupt = false;
    bool gamepak = false;

    u16 length = 0;
    u32 dst_addr = 0;
    u32 src_addr = 0;

    enum Control  {
      Increment = 0,
      Decrement = 1,
      Fixed  = 2,
      Reload = 3
    } dst_cntl = Increment, src_cntl = Increment;

    enum Timing {
      Immediate = 0,
      VBlank  = 1,
      HBlank  = 2,
      Special = 3
    } time = Immediate;

    enum Size {
      Half = 0,
      Word  = 1
    } size = Half;

    struct Latch {
      u32 length;
      u32 dst_addr;
      u32 src_addr;

      /// Most recently read (half)word by this channel.
      u32 bus = 0;
    } latch;

    bool is_fifo_dma = false;
    Scheduler::Event* startup_event = nullptr;
  } channels[4];

  constexpr int GetUnaliasedMemoryArea(int page) {
    if (page >= 0x09 && page <= 0x0D) {
      return 0x08;
    }

    if (page == 0x0F) {
      return 0x0E;
    }

    return page;
  }

  void ScheduleDMAs(unsigned int bitset);
  void SelectNextDMA();
  void OnChannelWritten(Channel& channel, bool enable_old);
  void RunChannel(bool first);

  arm::MemoryBase& memory;
  IRQ& irq;
  Scheduler& scheduler;

  int active_dma_id;
  bool early_exit_trigger;

  /// Set of currently enabled H-blank DMAs.
  std::bitset<4> hblank_set;

  /// Set of currently enabled V-blank DMAs.
  std::bitset<4> vblank_set;

  /// Set of currently enabled video transfer DMAs.
  std::bitset<4> video_set;

  /// Set of DMAs which are currently scheduled for execution.
  std::bitset<4> runnable_set;

  /// Most recent value transferred by any DMA channel.
  /// DMAs will read this when reading from unused memory or IO.
  u32 latch;
};

} // namespace nba::core
