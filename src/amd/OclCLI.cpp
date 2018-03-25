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


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <algorithm>


#include "amd/cryptonight.h"
#include "amd/OclCLI.h"
#include "amd/OclGPU.h"
#include "log/Log.h"
#include "Options.h"
#include "workers/OclThread.h"


OclCLI::OclCLI()
{
}


bool OclCLI::setup(std::vector<OclThread*> &threads)
{
    if (isEmpty()) {
        return false;
    }

    for (int i = 0; i < m_devices.size(); i++) {
        threads.push_back(new OclThread(m_devices[i], intensity(i), worksize(i), affinity(i)));
    }

    return true;
}


void OclCLI::autoConf(std::vector<OclThread*> &threads, int *platformIndex, size_t *intensity)
{
    *platformIndex = getAMDPlatformIdx();
    if (*platformIndex == -1) {
        LOG_ERR("No AMD OpenCL platform found");
        return;
    }

    std::vector<GpuContext> devices = getAMDDevices(*platformIndex);
    if (devices.empty()) {
        LOG_ERR("No AMD devices found.");
        return;
    }

    constexpr size_t byteToMiB = 1024u * 1024u;
    const size_t hashMemSize   = Options::i()->algo() == Options::ALGO_CRYPTONIGHT ? MONERO_MEMORY : AEON_MEMORY;

    for (GpuContext &ctx : devices) {
        size_t maxThreads = 1000u;
        int gpuThreads = 1;

        // rx vega
        if (ctx.deviceName.find("687F") == 0 || ctx.deviceName.find("Radeon RX Vega") == 0) {
            maxThreads = 2024u;
            gpuThreads = 2;
        }

        const size_t availableMem      = ctx.freeMem - (128u * byteToMiB);
        const size_t perThread         = hashMemSize + 224u;
        const size_t maxIntensity      = std::min(maxThreads, availableMem / perThread);
        const size_t rawIntensity      = (maxIntensity / (8 * ctx.computeUnits)) * ctx.computeUnits * 8;

        for (int i = 0; i < gpuThreads; i++) {
            threads.push_back(new OclThread(ctx.deviceIdx, *intensity ? *intensity : std::min(rawIntensity, maxIntensity), 8));
        }
    }
}


int OclCLI::get(const std::vector<int> &vector, int index, int defaultValue) const
{
    if (vector.empty()) {
        return defaultValue;
    }

    if (vector.size() <= index) {
        return vector.back();
    }

    return vector[index];
}


void OclCLI::parse(std::vector<int> &vector, const char *arg) const
{
    char *value = strdup(arg);
    char *pch   = strtok(value, ",");

    while (pch != nullptr) {
        vector.push_back((int) strtoul(pch, nullptr, 10));

        pch = strtok(nullptr, ",");
    }

    free(value);
}