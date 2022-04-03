#pragma once

struct [[eosio::table, eosio::contract(CONTRACT_NAME)]] stakedasset
{
    uint64_t asset_id;
    name owner;

    uint64_t primary_key() const
    {
        return asset_id;
    }
};

typedef eosio::multi_index<"stakedassets"_n, stakedasset> stakedassets_t;