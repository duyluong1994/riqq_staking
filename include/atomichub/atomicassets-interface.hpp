#pragma once
using namespace atomicdata;
static constexpr name ATOMICASSETS_ACCOUNT = name("atomicassets");
//Scope: collection_name
struct schemas_s
{
    name schema_name;
    vector<FORMAT> format;

    uint64_t primary_key() const { return schema_name.value; }
};

typedef multi_index<name("schemas"), schemas_s> schemas_t;

schemas_t get_schemas(name collection_name)
{
    return schemas_t(ATOMICASSETS_ACCOUNT, collection_name.value);
}

//Scope: collection_name
struct templates_s
{
    int32_t template_id;
    name schema_name;
    bool transferable;
    bool burnable;
    uint32_t max_supply;
    uint32_t issued_supply;
    vector<uint8_t> immutable_serialized_data;

    uint64_t primary_key() const { return (uint64_t)template_id; }
};

typedef multi_index<name("templates"), templates_s> templates_t;

templates_t get_templates(name collection_name)
{
    return templates_t(ATOMICASSETS_ACCOUNT, collection_name.value);
}
struct assertsale_action
{
    uint64_t sale_id;
    vector<uint64_t> asset_ids_to_assert;
    asset listing_price_to_assert;
    symbol settlement_symbol_to_assert;
};

struct assets_s
{
    uint64_t asset_id;
    name collection_name;
    name schema_name;
    int32_t template_id;
    name ram_payer;
    vector<asset> backed_tokens;
    vector<uint8_t> immutable_serialized_data;
    vector<uint8_t> mutable_serialized_data;

    uint64_t primary_key() const
    {
        return asset_id;
    };
};

typedef multi_index<name("assets"), assets_s> assets_t;

struct collections_s
{
    name collection_name;
    name author;
    bool allow_notify;
    vector<name> authorized_accounts;
    vector<name> notify_accounts;
    double market_fee;
    vector<uint8_t> serialized_data;

    uint64_t primary_key() const { return collection_name.value; };
};

typedef multi_index<name("collections"), collections_s> collections_t;
collections_t collections = collections_t(ATOMICASSETS_ACCOUNT, ATOMICASSETS_ACCOUNT.value);