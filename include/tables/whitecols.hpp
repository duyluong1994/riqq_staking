#pragma once

struct [[eosio::table, eosio::contract(CONTRACT_NAME)]] whitecol
{
    name collection_name;
    uint64_t primary_key() const
    {
        return collection_name.value;
    }
};

typedef eosio::multi_index<"whitecols"_n, whitecol> whitecols_t;