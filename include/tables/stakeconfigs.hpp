#pragma once

struct [[eosio::table, eosio::contract(CONTRACT_NAME)]] stakeconfig
{
    int32_t template_id;
    name collection_name;
    asset reward_per_duration;
    uint64_t earning_duration_days;

    uint64_t primary_key() const
    {
        return template_id;
    }
};

typedef eosio::multi_index<"stakeconfigs"_n, stakeconfig> stakeconfigs_t;