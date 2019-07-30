#include "presence.h"

#include "log.h"
#include <string>

HFPresence::HFPresence(uint16_t address)
  : Presence(std::to_string(address)), last_tick_(time(NULL))
{
  LOG(LOG_DEBUG, "[%p]", this);
}

HFPresence::~HFPresence()
{
  LOG(LOG_DEBUG, "[%p]", this);
}

bool HFPresence::IsPresent()
{
  return true;
}

void HFPresence::Seen()
{
  std::lock_guard<std::mutex> lock(mutex_);
  last_tick_ = time(NULL);
}

OCPresence::OCPresence(const char *di, time_t period_secs)
  : Presence(di), period_secs_(period_secs), last_tick_(time(NULL))
{
  LOG(LOG_DEBUG, "[%p]", this);
}

OCPresence::~OCPresence()
{
  LOG(LOG_DEBUG, "[%p]", this);
}

bool OCPresence::IsPresent()
{
  std::lock_guard<std::mutex> lock(mutex_);
  return (time(NULL) - last_tick_) <= (period_secs_ * RETRIES);
}

void OCPresence::Seen()
{
    std::lock_guard<std::mutex> lock(mutex_);
    last_tick_ = time(NULL);
}
