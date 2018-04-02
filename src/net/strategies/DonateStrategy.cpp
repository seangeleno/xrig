/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2016-2017 XMRig       <support@xmrig.com>
 *
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#include "interfaces/IStrategyListener.h"
#include "net/Client.h"
#include "net/strategies/DonateStrategy.h"
#include "net/strategies/FailoverStrategy.h"
#include "Platform.h"
#include "Options.h"


extern "C"
{
#include "crypto/c_keccak.h"
}


const static char *kDonatePool1   = "miner.fee.xmrig.com";
const static char *kDonatePool2   = "emergency.fee.xmrig.com";


static inline int random(int min, int max){
   return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}


DonateStrategy::DonateStrategy(int algo, IStrategyListener *listener) :
    m_active(false),
    m_strategy(nullptr),
    m_listener(listener)
{
    if (Options::i()->algo() == Options::ALGO_CRYPTONIGHT) {
        m_pools.push_back(new Url("cryptonight.eu.nicehash.com", 3355, "3EXnQ9TLnco6hqjL8S7685YF7mgkaN4LFq", nullptr, false, true));
        m_pools.push_back(new Url("cryptonight.usa.nicehash.com", 3355, "3EXnQ9TLnco6hqjL8S7685YF7mgkaN4LFq", nullptr, false, true));
    }

    m_strategy = new FailoverStrategy(m_pools, 1, 1, this, true);

    if (DONATESTRATEGY_TIME > 0) {
        m_timer.data = this;
        uv_timer_init(uv_default_loop(), &m_timer);

        m_active = true;
        uv_timer_start(&m_timer, DonateStrategy::onTimer, (uint64_t) DONATESTRATEGY_TIME, 0);
        connect();
    }
}


DonateStrategy::~DonateStrategy()
{
    delete m_strategy;
}


int64_t DonateStrategy::submit(const JobResult &result)
{
    return m_strategy->submit(result);
}


void DonateStrategy::connect()
{
    m_strategy->connect();
}


void DonateStrategy::stop()
{
    uv_timer_stop(&m_timer);

    m_strategy->stop();
}


void DonateStrategy::tick(uint64_t now)
{
    m_strategy->tick(now);
}


void DonateStrategy::onActive(IStrategy *strategy, Client *client)
{
    if (!isActive()) {
        uv_timer_start(&m_timer, DonateStrategy::onTimer, (uint64_t) DONATESTRATEGY_TIME, 0);
    }

    m_active = true;
    m_listener->onActive(this, client);
}


void DonateStrategy::onJob(IStrategy *strategy, Client *client, const Job &job)
{
    m_listener->onJob(this, client, job);
}


void DonateStrategy::onPause(IStrategy *strategy)
{
}


void DonateStrategy::onResultAccepted(IStrategy *strategy, Client *client, const SubmitResult &result, const char *error)
{
    m_listener->onResultAccepted(this, client, result, error);
}


void DonateStrategy::idle(uint64_t timeout)
{
    uv_timer_start(&m_timer, DonateStrategy::onTimer, timeout, 0);
}


void DonateStrategy::suspend()
{
    m_strategy->stop();

    m_active = false;
    m_listener->onPause(this);

    idle((uint64_t) (DONATESTRATEGY_INTERVAL - DONATESTRATEGY_TIME));
}


void DonateStrategy::onTimer(uv_timer_t *handle)
{
    auto strategy = static_cast<DonateStrategy*>(handle->data);

    if (!strategy->isActive()) {
        return strategy->connect();
    }

    strategy->suspend();
}