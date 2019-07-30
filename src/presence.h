#ifndef _PRESENCE_H
#define _PRESENCE_H

#include "cacommon.h"
#include "octypes.h"
#include <inttypes.h>
#include <mutex>
#include <string>
#include <time.h>

class Presence
{
  public:
    Presence(const std::string &id) : id_(id) { }
    virtual ~Presence() { }
    virtual bool IsPresent() = 0;
    virtual void Seen() = 0;
    virtual std::string GetId() const { return id_; }

  private:
    std::string id_;
};

class HFPresence : public Presence
{
  public:
    HFPresence(uint16_t address);
    virtual ~HFPresence();

    virtual bool IsPresent();
    virtual void Seen();

  private:
    std::mutex mutex_;
    time_t last_tick_;
};

class OCPresence : public Presence
{
    public:
        OCPresence(const char *di, time_t period_secs);
        virtual ~OCPresence();

        virtual bool IsPresent();
        virtual void Seen();

    private:
        static const uint8_t RETRIES = 3;

        const time_t period_secs_;
        std::mutex mutex_;
        time_t last_tick_;
};

#endif
