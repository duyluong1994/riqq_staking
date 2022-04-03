#pragma once

struct [[eosio::table, eosio::contract(CONTRACT_NAME)]] user
{
    name wallet;
    time_point last_claim;
    asset unclaimed;
    vector<pair<int32_t, uint64_t>> data;

    uint64_t primary_key() const
    {
        return wallet.value;
    }
};

typedef eosio::multi_index<"users"_n, user> users_t;