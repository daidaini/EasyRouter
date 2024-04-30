#pragma once

#include "RtCommDataDefine.h"

#include "T2SyncConnection.h"

class RtHst2Auth
{
public:
    RtHst2Auth() = default;

    ~RtHst2Auth() = default;

    void DoAuth(const AuthRequestParam &params);

private:
};