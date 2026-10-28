#pragma once
//禁止拷贝
class NoCopy
{
public:
    NoCopy() = default;

    NoCopy(const NoCopy&) = delete;
    NoCopy& operator = (const NoCopy &) = delete;
};