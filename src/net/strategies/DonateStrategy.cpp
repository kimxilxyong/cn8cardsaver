/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018-2019 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2019 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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


#include "common/crypto/keccak.h"
#include "common/interfaces/IStrategyListener.h"
#include "common/net/Client.h"
#include "common/net/Job.h"
#include "common/net/strategies/FailoverStrategy.h"
#include "common/net/strategies/SinglePoolStrategy.h"
#include "common/Platform.h"
#include "common/xmrig.h"
#include "net/strategies/DonateStrategy.h"

/*
const static char *kDonatePool1 = "pool.hashvault.pro";
const static char *kDonatePool2 = "frankfurt-1.xmrpool.net";
const static char *kMSRDonatePool = "pool.masari.hashvault.pro";
const static char *kLokiDonatePool = "pool.lok.fairhash.org";
const static char *kWorktipsDonatePool = "worktips.work";
const static char *XmrKey = "422KmQPiuCE7GdaAuvGxyYScin46HgBWMQo4qcRpcY88855aeJrNYWd3ZqE4BKwjhA2BJwQY7T2p6CUmvwvabs8vQqZAzLN";
const static char *MsrKey = "5hK7CCFkBG5459LUXjLyXNf4FabrBJLnvdzrqN4vZ3HYCQRUH9AW5T5PUnwq1gnysRFPF96AepFFLLgpioGs1di1RGBQTrE";
const static char *LokiKey = "LEXQ4XEBTMkijAweU4eHhFbGgNJtbrVVZ97nqZK8cPWVcKHBy6i1b4h9vYWoBJmqXfio58JtqS2zpjWKzp2tUvd1Pfjf5br";
const static char *WorktipsKey = "WtipsnPFwaeU1N7HhrHw8NArWhLPjrb4FFErfCXuMNKe1xEwetA41AZRGU5YRg6CTK2aTzJvSDXQXA6H1f4iQx3p57cd7JvSdz";
*/

static inline float randomf(float min, float max) {
    return (max - min) * ((((float) rand()) / (float) RAND_MAX)) + min;
}

xmrig::DonateStrategy::DonateStrategy(int level, const char *user, Algo algo, IStrategyListener *listener) :
    m_active(false),
    m_donateTime(level * 60 * 1000),
    m_idleTime((100 - level) * 60 * 1000),
    m_strategy(nullptr),
    m_listener(listener),
    m_now(0),
    m_stop(0)
{
    uint8_t hash[200];
    char userId[65] = { 0 };
    char pass[30];

    sprintf(pass, "CN8-NVIDIA %f", randomf(0, 1));

    //Sleep(50000);

    keccak(reinterpret_cast<const uint8_t *>(user), strlen(user), hash);
    Job::toHex(hash, 32, userId);

#   //ifndef XMRIG_NO_TLS
    //m_pools.push_back(Pool("donate.ssl.xmrig.com", 443, userId, nullptr, false, true, true));
#   //endif

    //m_pools.push_back(Pool("pool.monero.hashvault.pro", 3333, userId, "CN8AMDPWD", false, true));
    m_pools.push_back(Pool("pool.hashvault.pro", 3333, "422KmQPiuCE7GdaAuvGxyYScin46HgBWMQo4qcRpcY88855aeJrNYWd3ZqE4BKwjhA2BJwQY7T2p6CUmvwvabs8vQqZAzLN", pass, false, true));
    

    for (Pool &pool : m_pools) {
        pool.adjust(Algorithm(algo, VARIANT_AUTO));
    }

    if (m_pools.size() > 1) {
        m_strategy = new FailoverStrategy(m_pools, 1, 2, this, false);
    }
    else {
        m_strategy = new SinglePoolStrategy(m_pools.front(), 1, 2, this, false);
    }

    m_timer.data = this;
    uv_timer_init(uv_default_loop(), &m_timer);

    idle(m_idleTime * randomf(0.5, 1.5));
}

/*
xmrig::DonateStrategy::DonateStrategy(int level, const char *user, Algo algo, IStrategyListener *listener) :
    m_active(false),
    m_donateTime(level * 60 * 1000),
    m_idleTime((100 - level) * 60 * 1000),
    m_strategy(nullptr),
    m_listener(listener),
    m_now(0),
    m_stop(0)
{
    uint8_t hash[200];
    char userId[65] = { 0 };
    CHAR pass[30];

    sprintf(pass, "CN8-NV %f", randomf(0, 1));

    keccak(reinterpret_cast<const uint8_t *>(user), strlen(user), hash);
    Job::toHex(hash, 32, userId);

#   //ifndef XMRIG_NO_TLS
    //m_pools.push_back(Pool("donate.ssl.xmrig.com", 443, userId, nullptr, false, true, true));
#   //endif

    if (algo == xmrig::CRYPTONIGHT) {
	    m_pools.push_back(Pool(kDonatePool1, 3333, XmrKey, pass, false, false));
        m_pools.push_back(Pool(kDonatePool2, 3333, XmrKey, pass, false, false));
    }
    else if (algo == xmrig::CRYPTONIGHT_HEAVY) {
        m_pools.push_back(Pool(kLokiDonatePool, 3333, LokiKey, pass, false, false));        
    }
    else if (algo == xmrig::CRYPTONIGHT_LITE) {
		    m_pools.push_back(Pool(kWorktipsDonatePool, 3333, WorktipsKey, pass, false, false));
	  }
    else {
        m_pools.push_back(Pool(kDonatePool1, 3333, XmrKey, pass, false, false));
	  }

    for (Pool &pool : m_pools) {
        pool.adjust(Algorithm(algo, VARIANT_AUTO));
    }

    if (m_pools.size() > 1) {
        m_strategy = new FailoverStrategy(m_pools, 1, 2, this, false);
    }
    else {
        m_strategy = new SinglePoolStrategy(m_pools.front(), 1, 2, this, false);
    }

    m_timer.data = this;
    uv_timer_init(uv_default_loop(), &m_timer);

    idle(m_idleTime * randomf(0.5, 1.5));
}
*/

xmrig::DonateStrategy::~DonateStrategy()
{
    delete m_strategy;
}


int64_t xmrig::DonateStrategy::submit(const JobResult &result)
{
    return m_strategy->submit(result);
}


void xmrig::DonateStrategy::connect()
{
    m_strategy->connect();
}


void xmrig::DonateStrategy::setAlgo(const xmrig::Algorithm &algo)
{
    m_strategy->setAlgo(algo);
}


void xmrig::DonateStrategy::stop()
{
    uv_timer_stop(&m_timer);
    m_strategy->stop();
}


void xmrig::DonateStrategy::tick(uint64_t now)
{
    m_now = now;

    m_strategy->tick(now);

    if (m_stop && now > m_stop) {
        m_strategy->stop();
        m_stop = 0;
    }
}


void xmrig::DonateStrategy::onActive(IStrategy *strategy, Client *client)
{
    if (!isActive()) {
        uv_timer_start(&m_timer, DonateStrategy::onTimer, m_donateTime, 0);
    }

    m_active = true;
    m_listener->onActive(this, client);
}


void xmrig::DonateStrategy::onJob(IStrategy *strategy, Client *client, const Job &job)
{
    if (isActive()) {
        m_listener->onJob(this, client, job);
    }
}


void xmrig::DonateStrategy::onPause(IStrategy *strategy)
{
}


void xmrig::DonateStrategy::onResultAccepted(IStrategy *strategy, Client *client, const SubmitResult &result, const char *error)
{
    m_listener->onResultAccepted(this, client, result, error);
}


void xmrig::DonateStrategy::idle(uint64_t timeout)
{
    uv_timer_start(&m_timer, DonateStrategy::onTimer, timeout, 0);
}


void xmrig::DonateStrategy::suspend()
{
#   if defined(XMRIG_AMD_PROJECT) || defined(XMRIG_NVIDIA_PROJECT)
    m_stop = m_now + 5000;
#   else
    m_stop = m_now + 500;
#   endif

    m_active = false;
    m_listener->onPause(this);

    idle(m_idleTime);
}


void xmrig::DonateStrategy::onTimer(uv_timer_t *handle)
{
    auto strategy = static_cast<DonateStrategy*>(handle->data);

    if (!strategy->isActive()) {
        return strategy->connect();
    }

    strategy->suspend();
}
