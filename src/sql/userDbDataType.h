﻿#pragma once

// TODO remove

#include "../global.h"
#include "soci/soci.h"

namespace user_db_dt
{
struct AccountEntry
{
    int isOnline;
    // std::string lastLoginTimeUtc;
};
}

namespace soci
{
template <> struct type_conversion<user_db_dt::AccountEntry>
{
    typedef soci::values base_type;

    static void from_base(
        soci::values const& v, soci::indicator /* ind */, user_db_dt::AccountEntry& pe)
    {
        pe.isOnline = v.get<int>("isOnline");
    }

    static void to_base(
        user_db_dt::AccountEntry const& pe, soci::values& v, soci::indicator& ind)
    {
        v.set("isOnline", pe.isOnline);
        ind = i_ok;
    }
};
}