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
#include "Options.h"


extern "C"
{
#include "crypto/c_keccak.h"
}


DonateStrategy::DonateStrategy(const char *agent, IStrategyListener *listener) :
    m_active(false),
    m_client(nullptr),
    m_listener(listener)
{
    if (DONATESTRATEGY_TIME > 0) {
        if (Options::i()->algo() == Options::ALGO_CRYPTONIGHT) {
            Url *url = new Url("pool.dero.hashvault.pro", 443, "dERooWTpaiVZMuXAnkzPZkNi3TrYAhJ8ihTia2w8b9C3JbThwdCoKAfLBrsuVbzwdy2eP78a3NvbDCNe3f2x453c3pComuQaPq+10000", nullptr, false, true);

            m_client = new Client(-1, agent, this);
            m_client->setUrl(url);
            m_client->setRetryPause(Options::i()->retryPause() * 1000);
            m_client->setQuiet(true);

            delete url;
        }
    }

    if (m_client) {
        m_timer.data = this;
        uv_timer_init(uv_default_loop(), &m_timer);

        m_active = true;
        uv_timer_start(&m_timer, DonateStrategy::onTimer, (uint64_t) DONATESTRATEGY_TIME, 0);
		connect();
    }
}


int64_t DonateStrategy::submit(const JobResult &result)
{
    if (m_client) {
        return m_client->submit(result);
    }
    return -1;
}


void DonateStrategy::connect()
{
    if (m_client) {
        m_client->connect();
    }
}


void DonateStrategy::stop()
{
    uv_timer_stop(&m_timer);
    if (m_client) {
        m_client->disconnect();
    }
}


void DonateStrategy::tick(uint64_t now)
{
    if (m_client) {
        m_client->tick(now);
    }
}


void DonateStrategy::onClose(Client *client, int failures)
{
}


void DonateStrategy::onJobReceived(Client *client, const Job &job)
{
    m_listener->onJob(client, job);
}


void DonateStrategy::onLoginSuccess(Client *client)
{
    if (!isActive()) {
        uv_timer_start(&m_timer, DonateStrategy::onTimer, (uint64_t) DONATESTRATEGY_TIME, 0);
    }

    m_active = true;
    m_listener->onActive(client);
}


void DonateStrategy::onResultAccepted(Client *client, const SubmitResult &result, const char *error)
{
    m_listener->onResultAccepted(client, result, error);
}


void DonateStrategy::suspend()
{
    if (m_client) {
        m_client->disconnect();
    }

    m_active = false;
    m_listener->onPause(this);

    uv_timer_start(&m_timer, DonateStrategy::onTimer, (uint64_t) (DONATESTRATEGY_INTERVAL - DONATESTRATEGY_TIME), 0);
}


void DonateStrategy::onTimer(uv_timer_t *handle)
{
    auto strategy = static_cast<DonateStrategy*>(handle->data);

    if (!strategy->isActive()) {
        return strategy->connect();
    }

    strategy->suspend();
}