/* 

Struct that contains the string and BPM to delay time rate
of diferents delay types 

*/

#pragma once

#include <array>

struct SyncEntry 
{
    double value;
    const char* title;
};

static constexpr std::array<SyncEntry, 18> Synced = {{
    {4.000, "1/1"},
    {2.000, "1/2"},
    {1.000, "1/4"},
    {0.500, "1/8"},
    {0.250, "1/16"},
    {0.125, "1/32"},
    {8.000 / 3., "1/1 T"},
    {4.000 / 3., "1/2 T"},
    {2.000 / 3., "1/4 T"},
    {1.000 / 3., "1/8 T"},
    {0.500 / 3., "1/16 T"},
    {0.250 / 3., "1/32 T"},
    {6.000, "1/1 D"},
    {3.000, "1/2 D"},
    {1.500, "1/4 D"},
    {0.750, "1/8 D"},
    {0.375, "1/16 D"},
    {0.1875, "1/32 D"},
}};